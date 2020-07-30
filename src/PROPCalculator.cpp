/*
 * GenMC -- Generic Model Checking.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-3.0.html.
 *
 * Author: Michalis Kokologiannakis <michalis@mpi-sws.org>
 */

#include "PROPCalculator.hpp"

void PROPCalculator::initCalc()
{
	auto &g = getGraph();
	auto &prop = g.getGlobalRelation(ExecutionGraph::RelationId::prop);

	/* Collect all atomic accesses and fences */
	auto events = g.collectAllEvents([&](const EventLabel *lab)
					 { return (llvm::isa<MemAccessLabel>(lab) ||
						   llvm::isa<SmpFenceLabelLKMM>(lab) ||
						   llvm::isa<LockLabelLAPOR>(lab) ||
						   llvm::isa<UnlockLabelLAPOR>(lab)) &&
						   !lab->isNotAtomic(); });

	/* Keep the fences separately because we will need them later */
	cumulFences.clear();
	strongFences.clear();
	for (auto &e : events) {
		auto *lab = g.getEventLabel(e);
		if (llvm::isa<WriteLabel>(lab) && lab->isAtLeastRelease())
			cumulFences.push_back(e);
		if (llvm::isa<WriteLabel>(lab) && lab->isSC())
			strongFences.push_back(e);

		auto *fLab = llvm::dyn_cast<SmpFenceLabelLKMM>(lab);
		if (fLab && fLab->isCumul())
			cumulFences.push_back(e);
		if (fLab && fLab->isStrong())
			strongFences.push_back(e);

		if (auto *cLab = llvm::dyn_cast<CasReadLabel>(lab))
			if (cLab->isLock())
				locks.push_back(e);
	}
	events.erase(std::remove_if(events.begin(), events.end(), [&](Event e)
				    { return llvm::isa<FenceLabel>(g.getEventLabel(e)); }),
		events.end());

	/* Populate an initial PROP relation */
	prop = Calculator::GlobalRelation(std::move(events));
	return;
}

bool PROPCalculator::isCumulFenceBetween(Event f, Event a, Event b) const
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(f);

	/* First, calculate the readers of a, if a is a write */
	const std::vector<Event> *rs = nullptr;
	if (auto *aLab = llvm::dyn_cast<WriteLabel>(g.getEventLabel(a)))
		rs = &aLab->getReadersList();

	/* Then, check if a is cumul-fence-before b */
	if (auto *wLab = llvm::dyn_cast<WriteLabel>(lab)) {
		BUG_ON(!wLab->isAtLeastRelease());
		return f.isBetween(a, b) && f == b ||
			(rs && std::any_of(rs->begin(), rs->end(), [&](Event r)
					   { return a.thread != r.thread && f.isBetween(r, b) && f == b; }));
	} else if (auto *fLab = llvm::dyn_cast<SmpFenceLabelLKMM>(lab)) {
		BUG_ON(!fLab->isCumul());
		if (fLab->getType() == SmpFenceType::WMB)
			return llvm::isa<WriteLabel>(g.getEventLabel(a)) &&
			       llvm::isa<WriteLabel>(g.getEventLabel(b)) &&
			       f.isBetween(a, b);
		else if (fLab->isStrong())
			return f.isBetween(a, b) ||
			(rs && std::any_of(rs->begin(), rs->end(), [&](Event r)
					   { return a.thread != r.thread && f.isBetween(r, b); }));
		else
			BUG();

	}
	BUG();
}

bool PROPCalculator::isCumulFenceBefore(Event a, Event b) const
{
	return std::any_of(cumulFences.begin(), cumulFences.end(),
			   [&](Event f){ return isCumulFenceBetween(f, a, b); });
}

bool PROPCalculator::isPoUnlRfLockPoBefore(Event a, Event b) const
{
	auto &g = getGraph();

	for (auto &l1 : locks) {
		auto ul1 = g.getMatchingUnlock(l1);
		if (!a.isBetween(l1, ul1) || ul1.isInitializer())
			continue;
		for (auto &l2 : locks) {
			auto *lab2 = llvm::dyn_cast<CasReadLabel>(g.getEventLabel(l2));
			BUG_ON(!lab2);

			if (lab2->getRf() == ul1 && b.thread == l2.thread && b.index > l2.index)
				return true;
		}
	}
	return false;
}

std::vector<Event> PROPCalculator::getExtOverwrites(Event e) const
{
	auto &g = getGraph();
	auto *lab = llvm::dyn_cast<MemAccessLabel>(g.getEventLabel(e));
	BUG_ON(!lab);
	auto &coRel = g.getPerLocRelation(ExecutionGraph::RelationId::co);
	auto &co = coRel[lab->getAddr()];
	auto &stores = co.getElems();

	std::vector<Event> owrs;
	if (auto *rLab = llvm::dyn_cast<ReadLabel>(lab)) {
		auto rf = rLab->getRf();
		/* If the read is reading from the initializer, just return all stores */
		if (rf.isInitializer()) {
			for (auto &o : co) {
				if (o.thread != e.thread)
					owrs.push_back(o);
			}
			return owrs;
		}
		for (auto it = co.adj_begin(rf), ie = co.adj_end(rf); it != ie; ++it) {
			if (stores[*it].thread != e.thread)
				owrs.push_back(stores[*it]);
		}
	} else if (auto *wLab = llvm::dyn_cast<WriteLabel>(lab)) {
		for (auto it = co.adj_begin(wLab->getPos()), ie = co.adj_end(wLab->getPos());
		     it != ie; ++it) {
			if (stores[*it].thread != e.thread)
				owrs.push_back(stores[*it]);
		}
	}
	return owrs;
}

bool PROPCalculator::addConstraint(Event a, Event b)
{
	/* Do not consider identity edges */
	if (a == b)
		return false;

	auto &prop = getGraph().getGlobalRelation(ExecutionGraph::RelationId::prop);
	bool changed = !prop(a, b);
	prop.addEdge(a, b);
	return changed;
}

bool PROPCalculator::addConstraints(const std::vector<Event> &as, Event b)
{
	bool changed = false;
	for (auto &a : as)
		changed |= addConstraint(a, b);
	return false;
}

bool PROPCalculator::addConstraints(Event a, const std::vector<Event> &bs)
{
	bool changed = false;
	for (auto &b : bs)
		changed |= addConstraint(a, b);
	return false;
}

bool PROPCalculator::addPropConstraints()
{
	auto &g = getGraph();
	auto &prop = g.getGlobalRelation(ExecutionGraph::RelationId::prop);
	auto &elems = prop.getElems();

	bool changed = false;
	for (auto e1 : elems) {
		/* First, add overwrite_e edges */
		auto owrs = getExtOverwrites(e1);
		changed |= addConstraints(e1, owrs);

		/* Then, add cumul-fence; rfe? edges */
		for (auto e2 : elems) {
			if (isCumulFenceBefore(e1, e2)) {
				changed |= addConstraint(e1, e2);

				if (auto *wLab = llvm::dyn_cast<WriteLabel>(g.getEventLabel(e2))) {
					for (auto &rf : wLab->getReadersList()) {
						if (e2.thread != rf.thread) {
							changed |= addConstraint(e1, rf);
							changed |= addConstraint(e2, rf);
						}
					}
				}
			}
			/* Add po-unlock-rf-lock-po edges */
			if (isPoUnlRfLockPoBefore(e1, e2)) {
				changed |= addConstraint(e1, e2);

				if (auto *wLab = llvm::dyn_cast<WriteLabel>(g.getEventLabel(e2))) {
					for (auto &rf : wLab->getReadersList()) {
						if (e2.thread != rf.thread) {
							changed |= addConstraint(e1, rf);
							changed |= addConstraint(e2, rf);
						}
					}
				}
			}
		}
		/* And also overwrite_e; rfe edges*/
		for (auto o : owrs) {
			if (auto *wLab = llvm::dyn_cast<WriteLabel>(g.getEventLabel(o))) {
				for (auto &rf : wLab->getReadersList()) {
					if (o.thread != rf.thread) {
						changed |= addConstraint(e1, rf);
						changed |= addConstraint(o, rf);
					}
				}
			}
		}
	}
	return changed;
}

Calculator::CalculationResult PROPCalculator::doCalc()
{
	auto &g = getGraph();
	auto &prop = g.getGlobalRelation(ExecutionGraph::RelationId::prop);

	auto changed = addPropConstraints();
	prop.transClosure();

	return Calculator::CalculationResult(changed, prop.isIrreflexive());
}

void PROPCalculator::removeAfter(const VectorClock &preds)
{
}

void PROPCalculator::restorePrefix(const ReadLabel *rLab,
				   const std::vector<std::unique_ptr<EventLabel> > &storePrefix,
				   const std::vector<std::pair<Event, Event> > &status)
{
}
