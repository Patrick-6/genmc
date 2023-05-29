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

#include "CoherenceCalculator.hpp"
#include "ExecutionGraph.hpp"
#include "GraphIterators.hpp"
#include <vector>

CoherenceCalculator::const_store_iterator
CoherenceCalculator::co_succ_begin(SAddr addr, Event store) const
{
	auto offset = getStoreOffset(addr, store);
	return store_begin(addr) + (offset + 1);
}

CoherenceCalculator::const_store_iterator
CoherenceCalculator::co_succ_end(SAddr addr, Event store) const
{
	return store_end(addr);
}

CoherenceCalculator::const_store_iterator
CoherenceCalculator::co_succ_begin(Event e) const
{
	/* If it's not a write, we're gonna return a dummy
	 * sentinel.  This should not be dereferenced */
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab ? co_succ_begin(wLab->getAddr(), e) : getSentinel();
}

CoherenceCalculator::const_store_iterator
CoherenceCalculator::co_succ_end(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab ? co_succ_end(wLab->getAddr(), e) : getSentinel();
}

CoherenceCalculator::const_store_iterator
CoherenceCalculator::co_imm_succ_begin(SAddr addr, Event store) const
{
	return co_succ_begin(addr, store);
}

CoherenceCalculator::const_store_iterator
CoherenceCalculator::co_imm_succ_end(SAddr addr, Event store) const
{
	auto succ = co_imm_succ_begin(addr, store);
	return succ == co_succ_end(addr, store) ? co_succ_end(addr, store) : ++succ;
}

CoherenceCalculator::const_store_iterator
CoherenceCalculator::co_imm_succ_begin(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab ? co_imm_succ_begin(wLab->getAddr(), e) : getSentinel();
}

CoherenceCalculator::const_store_iterator
CoherenceCalculator::co_imm_succ_end(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab ? co_imm_succ_end(wLab->getAddr(), e) : getSentinel();
}

CoherenceCalculator::const_reverse_store_iterator
CoherenceCalculator::co_pred_begin(SAddr addr, Event store) const
{
	auto offset = getStoreOffset(addr, store);
	if (offset <= 0)
		return store_rend(addr);
	return const_reverse_store_iterator(store_begin(addr) + offset);
}

CoherenceCalculator::const_reverse_store_iterator
CoherenceCalculator::co_pred_end(SAddr addr, Event store) const
{
	return store_rend(addr);
}

CoherenceCalculator::const_reverse_store_iterator
CoherenceCalculator::co_pred_begin(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab ? co_pred_begin(wLab->getAddr(), e) : getRevSentinel();
}

CoherenceCalculator::const_reverse_store_iterator
CoherenceCalculator::co_pred_end(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab ? co_pred_end(wLab->getAddr(), e) : getRevSentinel();
}

CoherenceCalculator::const_reverse_store_iterator
CoherenceCalculator::co_imm_pred_begin(SAddr addr, Event store) const
{
	return co_pred_begin(addr, store);
}

CoherenceCalculator::const_reverse_store_iterator
CoherenceCalculator::co_imm_pred_end(SAddr addr, Event store) const
{
	auto pred = co_pred_begin(addr, store);
	return pred == co_pred_end(addr, store) ? co_pred_end(addr, store) : ++pred;
}

CoherenceCalculator::const_reverse_store_iterator
CoherenceCalculator::co_imm_pred_begin(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab ? co_imm_pred_begin(wLab->getAddr(), e) : getRevSentinel();
}

CoherenceCalculator::const_reverse_store_iterator
CoherenceCalculator::co_imm_pred_end(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab ? co_imm_pred_end(wLab->getAddr(), e) : getRevSentinel();
}

CoherenceCalculator::const_store_iterator
CoherenceCalculator::fr_succ_begin(SAddr addr, Event load) const
{
	auto *rLab = getGraph().getReadLabel(load);
	return co_succ_begin(addr, rLab->getRf());
}
CoherenceCalculator::const_store_iterator
CoherenceCalculator::fr_succ_end(SAddr addr, Event load) const
{
	auto *rLab = getGraph().getReadLabel(load);
	return co_succ_end(addr, rLab->getRf());
}

CoherenceCalculator::const_store_iterator
CoherenceCalculator::fr_succ_begin(Event e) const
{
	auto *rLab = getGraph().getReadLabel(e);
	return rLab ? fr_succ_begin(rLab->getAddr(), e) : getSentinel();
}
CoherenceCalculator::const_store_iterator
CoherenceCalculator::fr_succ_end(Event e) const
{
	auto *rLab = getGraph().getReadLabel(e);
	return rLab ? fr_succ_end(rLab->getAddr(), e) : getSentinel();
}

CoherenceCalculator::const_store_iterator
CoherenceCalculator::fr_imm_succ_begin(SAddr addr, Event load) const
{
	return fr_succ_begin(addr, load);
}
CoherenceCalculator::const_store_iterator
CoherenceCalculator::fr_imm_succ_end(SAddr addr, Event load) const
{
	auto succ = fr_succ_begin(addr, load);
	return succ == fr_succ_end(addr, load) ? fr_succ_end(addr, load) : ++succ;
}

CoherenceCalculator::const_store_iterator
CoherenceCalculator::fr_imm_succ_begin(Event e) const
{
	auto *rLab = getGraph().getReadLabel(e);
	return rLab ? fr_imm_succ_begin(rLab->getAddr(), e) : getSentinel();
}

CoherenceCalculator::const_store_iterator
CoherenceCalculator::fr_imm_succ_end(Event e) const
{
	auto *rLab = getGraph().getReadLabel(e);
	return rLab ? fr_imm_succ_end(rLab->getAddr(), e) : getSentinel();
}

CoherenceCalculator::const_store_iterator
CoherenceCalculator::fr_imm_pred_begin(SAddr addr, Event store) const
{
	return co_pred_begin(addr, store) == co_pred_end(addr, store) ?
		init_rf_begin(addr) :
		getGraph().getWriteLabel(*co_pred_begin(addr, store))->readers_begin();
}

CoherenceCalculator::const_store_iterator
CoherenceCalculator::fr_imm_pred_end(SAddr addr, Event store) const
{
	return co_pred_begin(addr, store) == co_pred_end(addr, store) ?
		init_rf_end(addr) :
		getGraph().getWriteLabel(*co_pred_begin(addr, store))->readers_end();
}

CoherenceCalculator::const_store_iterator
CoherenceCalculator::fr_imm_pred_begin(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab ? fr_imm_pred_begin(wLab->getAddr(), e) : getSentinel();
}

CoherenceCalculator::const_store_iterator
CoherenceCalculator::fr_imm_pred_end(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab ? fr_imm_pred_end(wLab->getAddr(), e) : getSentinel();
}

CoherenceCalculator::const_store_iterator
CoherenceCalculator::fr_init_pred_begin(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab && co_pred_begin(wLab->getAddr(), e) == co_pred_end(wLab->getAddr(), e) ?
		fr_imm_pred_begin(wLab->getAddr(), e) : getSentinel();
}

CoherenceCalculator::const_store_iterator
CoherenceCalculator::fr_init_pred_end(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab && co_pred_begin(wLab->getAddr(), e) == co_pred_end(wLab->getAddr(), e) ?
		fr_imm_pred_end(wLab->getAddr(), e) : getSentinel();
}

void CoherenceCalculator::trackCoherenceAtLoc(SAddr addr)
{
	stores[addr];
	initRfs[addr];
}

void CoherenceCalculator::addInitRfToLoc(SAddr addr, Event read)
{
	initRfs[addr].push_back(read);
}

void CoherenceCalculator::removeInitRfToLoc(SAddr addr, Event read)
{
	auto &locInits = getInitRfsToLoc(addr);
	auto it = std::find(locInits.begin(), locInits.end(), read);
	if (it != locInits.end())
		locInits.erase(it);
}

int CoherenceCalculator::getStoreOffset(SAddr addr, Event e) const
{
	BUG_ON(stores.count(addr) == 0);

	if (e == Event::getInitializer())
		return -1;

	auto *wLab = getGraph().getWriteLabel(e);
	BUG_ON(!wLab || wLab->getMOIdxHint() == -1);
	return wLab->getMOIdxHint();
}

void CoherenceCalculator::addStoreToLoc(SAddr addr, Event store, int offset)
{
	if (offset == -1) {
		getGraph().getWriteLabel(store)->setMOIdxHint(stores[addr].size());
		stores[addr].push_back(store);
	} else {
		stores[addr].insert(store_begin(addr) + offset, store);
		cacheMOIdxHints(addr, offset);
	}
}

void CoherenceCalculator::addStoreToLocAfter(SAddr addr, Event store, Event pred)
{
	int offset = getStoreOffset(addr, pred);
	addStoreToLoc(addr, store, offset + 1);
}

bool CoherenceCalculator::isCoMaximal(SAddr addr, Event store)
{
	auto &locMO = stores[addr];
	return (store.isInitializer() && locMO.empty()) ||
	       (!store.isInitializer() && !locMO.empty() && store == locMO.back());
}

bool CoherenceCalculator::isCachedCoMaximal(SAddr addr, Event store)
{
	return isCoMaximal(addr, store);
}

void CoherenceCalculator::changeStoreOffset(SAddr addr, Event store, int newOffset)
{
	auto &locMO = stores[addr];

	locMO.erase(std::find(store_begin(addr), store_end(addr), store));
	locMO.insert(store_begin(addr) + newOffset, store);
	cacheMOIdxHints(addr, newOffset);
}

#ifdef ENABLE_GENMC_DEBUG
std::vector<std::pair<Event, Event> >
CoherenceCalculator::saveCoherenceStatus(const std::vector<std::unique_ptr<EventLabel> > &labs,
				  const ReadLabel *rLab) const
{
	auto before = getGraph().getPredsView(rLab->getPos());
	std::vector<std::pair<Event, Event> > pairs;

	for (const auto &lab : labs) {
		/* Only store MO pairs for write labels */
		if (!llvm::isa<WriteLabel>(lab.get()))
			continue;

		BUG_ON(before->contains(lab->getPos()));
		auto *wLab = static_cast<const WriteLabel *>(lab.get());
		auto moPos = std::find(store_begin(wLab->getAddr()), store_end(wLab->getAddr()), wLab->getPos());

		/* This store must definitely be in this location's MO */
		BUG_ON(moPos == store_end(wLab->getAddr()));

		/* We need to find the previous MO store that is in before or
		 * in the vector for which we are getting the predecessors */
		decltype(store_rbegin(wLab->getAddr())) predPos(moPos);
		auto predFound = false;
		for (auto rit = predPos; rit != store_rend(wLab->getAddr()); ++rit) {
			if (before->contains(*rit) ||
			    std::find_if(labs.begin(), labs.end(),
					 [&](const std::unique_ptr<EventLabel> &lab)
					 { return lab->getPos() == *rit; })
			    != labs.end()) {
				pairs.push_back(std::make_pair(*moPos, *rit));
				predFound = true;
				break;
			}
		}
		/* If there is not predecessor in the vector or in before,
		 * then INIT is the only valid predecessor */
		if (!predFound)
			pairs.push_back(std::make_pair(*moPos, Event::getInitializer()));
	}
	return pairs;
}
#endif

void CoherenceCalculator::initCalc()
{
	auto &gm = getGraph();
	auto &coRelation = gm.getPerLocRelation(ExecutionGraph::RelationId::co);

	coRelation.clear();
	for (auto locIt = begin(); locIt != end(); locIt++) {
		coRelation[locIt->first] = GlobalRelation(getStoresToLoc(locIt->first));
		if (locIt->second.empty())
			continue;
		for (auto sIt = locIt->second.begin(); sIt != locIt->second.end() - 1; sIt++)
			coRelation[locIt->first].addEdge(*sIt, *(sIt + 1));
		coRelation[locIt->first].transClosure();
	}
	return;
}

Calculator::CalculationResult CoherenceCalculator::doCalc()
{
	auto &gm = getGraph();
	auto &coRelation = gm.getPerLocRelation(ExecutionGraph::RelationId::co);

	for (auto locIt = begin(); locIt != end(); locIt++) {
		if (!coRelation[locIt->first].isIrreflexive())
			return Calculator::CalculationResult(false, false);
	}
	return Calculator::CalculationResult(false, true);
}

void CoherenceCalculator::removeAfter(const VectorClock &preds)
{
	auto &g = getGraph();
	VSet<SAddr> keep;

	/* Check which locations should be kept */
	for (auto i = 0u; i < preds.size(); i++) {
		for (auto j = 0u; j <= preds.getMax(i); j++) {
			auto *lab = g.getEventLabel(Event(i, j));
			if (auto *mLab = llvm::dyn_cast<MemAccessLabel>(lab))
				keep.insert(mLab->getAddr());
		}
	}

	for (auto sIt = begin(), rIt = init_rf_begin(); sIt != end(); /* empty */) {
		/* Should we keep this memory location lying around? */
		if (!keep.count(sIt->first)) {
			sIt = stores.erase(sIt);
			rIt = initRfs.erase(rIt);
		} else {
			sIt->second.erase(std::remove_if(sIt->second.begin(), sIt->second.end(),
							[&](Event &e)
							{ return !preds.contains(e); }),
					 sIt->second.end());
			rIt->second.erase(std::remove_if(rIt->second.begin(), rIt->second.end(),
							[&](Event &e)
							{ return !preds.contains(e); }),
					 rIt->second.end());
			/* Repair label hints */
			cacheMOIdxHints(sIt->first);
			++sIt;
			++rIt;
		}
	}
}

bool CoherenceCalculator::locContains(SAddr addr, Event e) const
{
	BUG_ON(stores.count(addr) == 0);
	return e == Event::getInitializer() ||
		std::any_of(store_begin(addr), store_end(addr),
			    [&e](Event s){ return s == e; });
}

void CoherenceCalculator::cacheMOIdxHints(SAddr addr, int start /* = -1 */) const
{
	auto &bucket = getStoresToLoc(addr);
	for (auto i = (start == -1) ? 0u : unsigned(start); i < bucket.size(); i++)
		getGraph().getWriteLabel(bucket[i])->setMOIdxHint(i);
}
