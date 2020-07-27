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
		auto *fLab = llvm::dyn_cast<SmpFenceLabelLKMM>(g.getEventLabel(e));
		if (fLab && fLab->isCumul())
			cumulFences.push_back(e);
		if (fLab && fLab->isStrong())
			strongFences.push_back(e);
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
	auto *lab = getGraph().getEventLabel(f);

	/* First, calculate the readers of a, if a is a write */
	const std::vector<Event> *rs = nullptr;
	if (auto *aLab = llvm::dyn_cast<WriteLabel>(getGraph().getEventLabel(a)))
		rs = &aLab->getReadersList();

	/* Then, check if a is cumul-fence-before b */
	if (auto *wLab = llvm::dyn_cast<WriteLabel>(lab)) {
		BUG_ON(!wLab->isAtLeastRelease());
		return f.isBetween(a, b) ||
			(rs && std::any_of(rs->begin(), rs->end(),
					   [&](Event r){ return f.isBetween(r, b); }));
	} else if (auto *fLab = llvm::dyn_cast<SmpFenceLabelLKMM>(lab)) {
		BUG_ON(!fLab->isCumul());
		if (fLab->getType() == SmpFenceType::WMB)
			return f.isBetween(a, b);
		else if (fLab->getType() == SmpFenceType::MB)
			return f.isBetween(a, b) ||
			(rs && std::any_of(rs->begin(), rs->end(),
					   [&](Event r){ return f.isBetween(r, b); }));
		else
			BUG();

	}
	BUG();
}

llvm::SmallVector<Event, 4> PROPCalculator::getExtOverwrites(Event e) const
{
	auto &g = getGraph();
	auto *lab = llvm::dyn_cast<MemAccessLabel>(g.getEventLabel(e));
	BUG_ON(!lab);
	auto &coRel = g.getPerLocRelation(ExecutionGraph::RelationId::co);
	auto &co = coRel[lab->getAddr()];
	auto &stores = co.getElems();

	llvm::SmallVector<Event, 4> owrs;
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

bool PROPCalculator::addConstraint(Event a, const std::vector<Event> &bs)
{
	bool changed = false;

	for (auto b : bs)
		changed |= addConstraint(a, b);
	return changed;
}

bool PROPCalculator::addConstraint(const std::vector<Event> &as, Event b)
{
	bool changed = false;

	for (auto a : as)
		changed |= addConstraint(a, b);
	return changed;
}

bool PROPCalculator::addPropConstraints()
{
	auto &g = getGraph();
	auto &prop = g.getGlobalRelation(ExecutionGraph::RelationId::prop);
	auto &elems = prop.getElems();

	bool changed = false;
	for (auto e1 : elems) {
		auto owrs = getExtOverwrites(e1);
		owrs.push_back(e1); /* Add e1 since we don't consider id edges anyway */

		/* Add all edges from owrs to other events in elems */
		for (auto o : owrs) {
			/* First, add overwrite_e edges */
			changed |= addConstraint(e1, o);

			/* Then, add overwrite_e?; cumul-fence; rfe? edges */
			for (auto e2 : elems) {
				if (std::any_of(cumulFences.begin(), cumulFences.end(),
						[&](Event f){ return isCumulFenceBetween(f, o, e2); })) {
					changed |= addConstraint(o, e2);
					if (auto *wLab = llvm::dyn_cast<WriteLabel>(g.getEventLabel(e2)))
						changed |= addConstraint(o, wLab->getReadersList());
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
