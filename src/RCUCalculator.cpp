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

#include "RCUCalculator.hpp"
#include "PROPCalculator.hpp"

void RCUCalculator::initCalc()
{
	auto &g = getGraph();
	auto &rcu = g.getGlobalRelation(ExecutionGraph::RelationId::rcu);

	/* Only collect synchronize_rcu()s and rcu_read_lock()s */
	auto rcuEvents = g.collectAllEvents([&](const EventLabel* lab){
						    return llvm::isa<RCUSyncLabelLKMM>(lab) ||
							   llvm::isa<RCULockLabelLKMM>(lab);
					    });
	auto events = g.collectAllEvents([&](const EventLabel* lab){
						 /* Also collect non-atomic events for rcu-fence */
						 return PROPCalculator::isNonTrivial(lab);
					 });

	rcu = Calculator::GlobalRelation(std::move(rcuEvents));
	rcuLink = Calculator::GlobalRelation(rcu.getElems());
	rcuFence = Calculator::GlobalRelation(std::move(events));
	return;
}

Event RCUCalculator::getMatchingUnlockRCU(Event lock) const
{
	auto &g = getGraph();
	std::vector<Event> locks;

	BUG_ON(!llvm::isa<RCULockLabelLKMM>(g.getEventLabel(lock)));
	for (auto j = lock.index + 1; j < g.getThreadSize(lock.thread); j++) {
		const EventLabel *lab = g.getEventLabel(Event(lock.thread, j));

		if (auto *lLab = llvm::dyn_cast<RCULockLabelLKMM>(lab))
			locks.push_back(lLab->getPos());

		if (auto *uLab = llvm::dyn_cast<RCUUnlockLabelLKMM>(lab)) {
			if (locks.empty())
				return uLab->getPos();
			locks.pop_back();
		}
	}
	return g.getLastThreadEvent(lock.thread).next();
}

/* Returns true if E links to R.
 * R should be a synchronize_rcu() (F-type) or a rcu_read_lock() (L-type) event.
 *  - An event links to an F-type event if it is po-before it.
 *  - An event links to an L-type event if it is po-before its matching unlock.
 **/
bool RCUCalculator::linksTo(Event e, Event r) const
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(r);

	if (llvm::isa<RCUSyncLabelLKMM>(lab)) {
		return e.thread == r.thread && e.index < r.index;
	} else if (llvm::isa<RCULockLabelLKMM>(lab)) {
		auto ul = getMatchingUnlockRCU(r);
		return e.thread == ul.thread && e.index < ul.index;
	}
	BUG();
}

/* Fetches rcu links that are (pb*;prop;po)-after e1 */
std::vector<Event> RCUCalculator::getPbOptPropPoLinks(Event e1) const
{
	auto &g = getGraph();
	auto &prop = g.getGlobalRelation(ExecutionGraph::RelationId::prop);
	auto &pb = g.getGlobalRelation(ExecutionGraph::RelationId::pb);
	auto &rcu = g.getGlobalRelation(ExecutionGraph::RelationId::rcu);
	auto &elems = prop.getElems();
	auto &candidates = rcu.getElems(); /* link candidates */
	std::vector<Event> links;

	for (auto e2 : elems) {
		/* First, add prop;po edges */
		if (prop(e1, e2)) {
			std::copy_if(candidates.begin(), candidates.end(), std::back_inserter(links),
				     [&](Event r){ return linksTo(e2, r); });
		}
		/* Then, add pb*;prop;po edges */
		if (!pb(e1, e2))
			continue;
		for (auto e3 : elems) {
			if (prop(e2, e3)) {
				std::copy_if(candidates.begin(), candidates.end(), std::back_inserter(links),
					     [&](Event r){ return linksTo(e3, r); });
			}
		}
	}
	return links;
}

bool RCUCalculator::addRcuLinks(Event e)
{
	auto &g = getGraph();
	auto &prop = g.getGlobalRelation(ExecutionGraph::RelationId::prop);
	auto &ar = g.getGlobalRelation(ExecutionGraph::RelationId::ar_lkmm);
	auto &pb = g.getGlobalRelation(ExecutionGraph::RelationId::pb);
	auto &elems = prop.getElems();
	bool changed = false;

	/* Calculate the upper limit in po until which we will look for links */
	auto *lab = g.getEventLabel(e);
	auto upperLimit = (llvm::isa<RCULockLabelLKMM>(lab)) ?
		getMatchingUnlockRCU(e) : g.getLastThreadEvent(e.thread).next();

	/* Start looking for links */
	for (auto i = e.index + 1; i < upperLimit.index; i++) {
		auto *lab = g.getEventLabel(Event(e.thread, i));
		if (!PROPCalculator::isNonTrivial(lab) || lab->isNotAtomic())
			continue;

		auto links = getPbOptPropPoLinks(Event(e.thread, i));
		for (auto l : links) {
			if (!rcuLink(e, l)) {
				changed = true;
				rcuLink.addEdge(e, l);
			}
		}
	}
	return changed;
}

bool RCUCalculator::addRcuLinkConstraints()
{
	auto &g = getGraph();
	auto &rcuElems = g.getGlobalRelation(ExecutionGraph::RelationId::rcu).getElems();
	bool changed = false;

	for (auto e : rcuElems)
		changed |= addRcuLinks(e);
	return changed;
}

void RCUCalculator::incRcuCounter(Event e, unsigned int &gps, unsigned int &css) const
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	if (llvm::isa<RCUSyncLabelLKMM>(lab))
		++gps;
	if (llvm::isa<RCULockLabelLKMM>(lab))
		++css;
	return;
}

void RCUCalculator::decRcuCounter(Event e, unsigned int &gps, unsigned int &css) const
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	if (llvm::isa<RCUSyncLabelLKMM>(lab))
		--gps;
	if (llvm::isa<RCULockLabelLKMM>(lab))
		--css;
	return;
}

bool RCUCalculator::checkAddRcuConstraint(Event a, Event b, const unsigned int gps,
					  const unsigned int css)
{
	auto &rcu = getGraph().getGlobalRelation(ExecutionGraph::RelationId::rcu);
	bool changed = false;

	if (gps >= css && !rcu(a, b)) {
		changed = true;
		rcu.addEdge(a, b);
	}
	return changed;
}

bool RCUCalculator::addRcuConstraints()
{
	auto &g = getGraph();
	auto &rcu = g.getGlobalRelation(ExecutionGraph::RelationId::rcu);
	auto &rcuElems = rcu.getElems();

	bool changed = false;
	for (auto e : rcuElems) {
		const EventLabel *lab = g.getEventLabel(e);
		unsigned int gps = 0;
		unsigned int css = 0;

		using NodeId = AdjList<Event, EventHasher>::NodeId;
		using Timestamp = AdjList<Event, EventHasher>::Timestamp;
		using NodeStatus = AdjList<Event, EventHasher>::NodeStatus;

		rcuLink.visitReachable(e,
			[&](NodeId i, Timestamp &t, std::vector<NodeStatus> &m,
			    std::vector<NodeId> &p, std::vector<Timestamp> &d,
			    std::vector<Timestamp> &f){ /* atEntryV */
				incRcuCounter(rcuElems[i], gps, css);
				return;
			},
			[&](NodeId i, NodeId j, Timestamp &t, std::vector<NodeStatus> &m,
			    std::vector<NodeId> &p, std::vector<Timestamp> &d,
			    std::vector<Timestamp> &f){ /* atTreeE */
				incRcuCounter(rcuElems[j], gps, css);
				changed |= checkAddRcuConstraint(e, rcuElems[j], gps, css);
				decRcuCounter(rcuElems[j], gps, css);
				return;
			},
			[&](NodeId i, NodeId j, Timestamp &t, std::vector<NodeStatus> &m,
			    std::vector<NodeId> &p, std::vector<Timestamp> &d,
			    std::vector<Timestamp> &f){ /* atBackE*/
				/* We shouldn't manipulate the counters in back edges,
				 * as such vertices have already been counted */
				changed |= checkAddRcuConstraint(e, rcuElems[j], gps, css);
				return;
			},
			[&](NodeId i, NodeId j, Timestamp &t, std::vector<NodeStatus> &m,
			    std::vector<NodeId> &p, std::vector<Timestamp> &d,
			    std::vector<Timestamp> &f){ /* atForwE*/
				incRcuCounter(rcuElems[j], gps, css);
				changed |= checkAddRcuConstraint(e, rcuElems[j], gps, css);
				decRcuCounter(rcuElems[j], gps, css);
				return;
			},
			[&](NodeId i, Timestamp &t, std::vector<NodeStatus> &m,
			    std::vector<NodeId> &p, std::vector<Timestamp> &d,
			    std::vector<Timestamp> &f){ /* atExitV*/
				decRcuCounter(rcuElems[i], gps, css);
			},
			[&](Timestamp &t, std::vector<NodeStatus> &m,
			    std::vector<NodeId> &p, std::vector<Timestamp> &d,
			    std::vector<Timestamp> &f){ return; }); /* atEnd */
	}
	return changed;
}

bool RCUCalculator::checkAddRcuFenceConstraint(Event a, Event b)
{
	auto &g = getGraph();
	auto &rcufence = getRcuFenceRelation();

	bool changed = false;
	for (auto i = 1; i < a.index; i++) {
		auto *labA = g.getEventLabel(Event(a.thread, i));
		if (!PROPCalculator::isNonTrivial(labA))
			continue;
		for (auto j = b.index + 1; j < g.getThreadSize(b.thread); j++) {
			auto *labB = g.getEventLabel(Event(b.thread, j));
			if (!PROPCalculator::isNonTrivial(labB))
				continue;
			if (!rcufence(labA->getPos(), labB->getPos())) {
				changed = true;
				rcufence.addEdge(labA->getPos(), labB->getPos());
			}
		}
	}
	return changed;
}

bool RCUCalculator::addRcuFenceConstraints()
{
	auto &g = getGraph();
	auto &rcu = g.getGlobalRelation(ExecutionGraph::RelationId::rcu);
	auto &rcuElems = rcu.getElems();

	if (rcuElems.empty())
		return false;

	bool changed = false;
	for (auto r : rcuElems) {
		for (auto it = rcu.adj_begin(r), ie = rcu.adj_end(r); it != ie; ++it)
			changed |= checkAddRcuFenceConstraint(r, rcuElems[*it]);
	}
	return changed;
}

Calculator::CalculationResult RCUCalculator::doCalc()
{
	auto &g = getGraph();
	auto &rcu = g.getGlobalRelation(ExecutionGraph::RelationId::rcu);

	bool changed = addRcuLinkConstraints();
	if (changed) {
		addRcuConstraints();
		rcu.transClosure();
		addRcuFenceConstraints();
	}
	return Calculator::CalculationResult(changed, rcu.isIrreflexive());
}

void RCUCalculator::removeAfter(const VectorClock &preds)
{
}

void RCUCalculator::restorePrefix(const ReadLabel *rLab,
				 const std::vector<std::unique_ptr<EventLabel> > &storePrefix,
				 const std::vector<std::pair<Event, Event> > &status)
{
}
