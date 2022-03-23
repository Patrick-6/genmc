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

#include "MOCalculator.hpp"
#include "ExecutionGraph.hpp"
#include "GraphIterators.hpp"
#include <vector>

CoherenceCalculator::const_store_iterator
MOCalculator::co_succ_begin(SAddr addr, Event store) const
{
	auto offset = getStoreOffset(addr, store);
	return store_begin(addr) + (offset + 1);
}

CoherenceCalculator::const_store_iterator
MOCalculator::co_succ_end(SAddr addr, Event store) const
{
	return store_end(addr);
}

CoherenceCalculator::const_store_iterator
MOCalculator::co_succ_begin(Event e) const
{
	/* If it's not a write, we're gonna return a dummy
	 * sentinel.  This should not be dereferenced */
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab ? co_succ_begin(wLab->getAddr(), e) : getSentinel();
}

CoherenceCalculator::const_store_iterator
MOCalculator::co_succ_end(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab ? co_succ_end(wLab->getAddr(), e) : getSentinel();
}

CoherenceCalculator::const_store_iterator
MOCalculator::co_imm_succ_begin(SAddr addr, Event store) const
{
	return co_succ_begin(addr, store);
}

CoherenceCalculator::const_store_iterator
MOCalculator::co_imm_succ_end(SAddr addr, Event store) const
{
	auto succ = co_imm_succ_begin(addr, store);
	return succ == co_succ_end(addr, store) ? co_succ_end(addr, store) : ++succ;
}

CoherenceCalculator::const_store_iterator
MOCalculator::co_imm_succ_begin(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab ? co_imm_succ_begin(wLab->getAddr(), e) : getSentinel();
}

CoherenceCalculator::const_store_iterator
MOCalculator::co_imm_succ_end(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab ? co_imm_succ_end(wLab->getAddr(), e) : getSentinel();
}

CoherenceCalculator::const_reverse_store_iterator
MOCalculator::co_pred_begin(SAddr addr, Event store) const
{
	auto offset = getStoreOffset(addr, store);
	if (offset <= 0)
		return store_rend(addr);
	return const_reverse_store_iterator(store_begin(addr) + offset);
}

CoherenceCalculator::const_reverse_store_iterator
MOCalculator::co_pred_end(SAddr addr, Event store) const
{
	return store_rend(addr);
}

CoherenceCalculator::const_reverse_store_iterator
MOCalculator::co_pred_begin(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab ? co_pred_begin(wLab->getAddr(), e) : getRevSentinel();
}

CoherenceCalculator::const_reverse_store_iterator
MOCalculator::co_pred_end(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab ? co_pred_end(wLab->getAddr(), e) : getRevSentinel();
}

CoherenceCalculator::const_reverse_store_iterator
MOCalculator::co_imm_pred_begin(SAddr addr, Event store) const
{
	return co_pred_begin(addr, store);
}

CoherenceCalculator::const_reverse_store_iterator
MOCalculator::co_imm_pred_end(SAddr addr, Event store) const
{
	auto pred = co_pred_begin(addr, store);
	return pred == co_pred_end(addr, store) ? co_pred_end(addr, store) : ++pred;
}

CoherenceCalculator::const_reverse_store_iterator
MOCalculator::co_imm_pred_begin(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab ? co_imm_pred_begin(wLab->getAddr(), e) : getRevSentinel();
}

CoherenceCalculator::const_reverse_store_iterator
MOCalculator::co_imm_pred_end(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab ? co_imm_pred_end(wLab->getAddr(), e) : getRevSentinel();
}

CoherenceCalculator::const_store_iterator
MOCalculator::fr_succ_begin(SAddr addr, Event load) const
{
	auto *rLab = getGraph().getReadLabel(load);
	return co_succ_begin(addr, rLab->getRf());
}
CoherenceCalculator::const_store_iterator
MOCalculator::fr_succ_end(SAddr addr, Event load) const
{
	auto *rLab = getGraph().getReadLabel(load);
	return co_succ_end(addr, rLab->getRf());
}

CoherenceCalculator::const_store_iterator
MOCalculator::fr_succ_begin(Event e) const
{
	auto *rLab = getGraph().getReadLabel(e);
	return rLab ? fr_succ_begin(rLab->getAddr(), e) : getSentinel();
}
CoherenceCalculator::const_store_iterator
MOCalculator::fr_succ_end(Event e) const
{
	auto *rLab = getGraph().getReadLabel(e);
	return rLab ? fr_succ_end(rLab->getAddr(), e) : getSentinel();
}

CoherenceCalculator::const_store_iterator
MOCalculator::fr_imm_succ_begin(SAddr addr, Event load) const
{
	return fr_succ_begin(addr, load);
}
CoherenceCalculator::const_store_iterator
MOCalculator::fr_imm_succ_end(SAddr addr, Event load) const
{
	auto succ = fr_succ_begin(addr, load);
	return succ == fr_succ_end(addr, load) ? fr_succ_end(addr, load) : ++succ;
}

CoherenceCalculator::const_store_iterator
MOCalculator::fr_imm_succ_begin(Event e) const
{
	auto *rLab = getGraph().getReadLabel(e);
	return rLab ? fr_imm_succ_begin(rLab->getAddr(), e) : getSentinel();
}

CoherenceCalculator::const_store_iterator
MOCalculator::fr_imm_succ_end(Event e) const
{
	auto *rLab = getGraph().getReadLabel(e);
	return rLab ? fr_imm_succ_end(rLab->getAddr(), e) : getSentinel();
}

CoherenceCalculator::const_store_iterator
MOCalculator::fr_imm_pred_begin(SAddr addr, Event store) const
{
	return co_pred_begin(addr, store) == co_pred_end(addr, store) ?
		init_rf_begin(addr) :
		getGraph().getWriteLabel(*co_pred_begin(addr, store))->readers_begin();
}

CoherenceCalculator::const_store_iterator
MOCalculator::fr_imm_pred_end(SAddr addr, Event store) const
{
	return co_pred_begin(addr, store) == co_pred_end(addr, store) ?
		init_rf_end(addr) :
		getGraph().getWriteLabel(*co_pred_begin(addr, store))->readers_end();
}

CoherenceCalculator::const_store_iterator
MOCalculator::fr_imm_pred_begin(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab ? fr_imm_pred_begin(wLab->getAddr(), e) : getSentinel();
}

CoherenceCalculator::const_store_iterator
MOCalculator::fr_imm_pred_end(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab ? fr_imm_pred_end(wLab->getAddr(), e) : getSentinel();
}

CoherenceCalculator::const_store_iterator
MOCalculator::fr_init_pred_begin(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab && co_pred_begin(wLab->getAddr(), e) == co_pred_end(wLab->getAddr(), e) ?
		fr_imm_pred_begin(wLab->getAddr(), e) : getSentinel();
}

CoherenceCalculator::const_store_iterator
MOCalculator::fr_init_pred_end(Event e) const
{
	auto *wLab = getGraph().getWriteLabel(e);
	return wLab && co_pred_begin(wLab->getAddr(), e) == co_pred_end(wLab->getAddr(), e) ?
		fr_imm_pred_end(wLab->getAddr(), e) : getSentinel();
}

void MOCalculator::trackCoherenceAtLoc(SAddr addr)
{
	stores[addr];
	initRfs[addr];
}

void MOCalculator::addInitRfToLoc(SAddr addr, Event read)
{
	initRfs[addr].push_back(read);
}

void MOCalculator::removeInitRfToLoc(SAddr addr, Event read)
{
	auto &locInits = getInitRfsToLoc(addr);
	auto it = std::find(locInits.begin(), locInits.end(), read);
	if (it != locInits.end())
		locInits.erase(it);
}

int MOCalculator::getStoreOffset(SAddr addr, Event e) const
{
	BUG_ON(stores.count(addr) == 0);

	if (e == Event::getInitializer())
		return -1;

	auto *wLab = getGraph().getWriteLabel(e);
	BUG_ON(!wLab || wLab->getMOIdxHint() == -1);
	return wLab->getMOIdxHint();
}

std::pair<int, int>
MOCalculator::getPossiblePlacings(SAddr addr, Event store, bool isRMW)
{
	const auto &g = getGraph();

	/* If it is an RMW store, there is only one possible position in MO */
	if (isRMW) {
		if (auto *rLab = llvm::dyn_cast<ReadLabel>(g.getEventLabel(store.prev()))) {
			auto offset = getStoreOffset(addr, rLab->getRf()) + 1;
			return std::make_pair(offset, offset);
		}
		BUG();
	}

	/* Otherwise, we calculate the full range and add the store */
	auto rangeBegin = splitLocMOBefore(addr, store);
	auto rangeEnd = (supportsOutOfOrder()) ? splitLocMOAfter(addr, store) :
		getStoresToLoc(addr).size();
	return std::make_pair(rangeBegin, rangeEnd);

}

void MOCalculator::addStoreToLoc(SAddr addr, Event store, int offset)
{
	if (offset == -1) {
		getGraph().getWriteLabel(store)->setMOIdxHint(stores[addr].size());
		stores[addr].push_back(store);
	} else {
		stores[addr].insert(store_begin(addr) + offset, store);
		cacheMOIdxHints(addr, offset);
	}
}

void MOCalculator::addStoreToLocAfter(SAddr addr, Event store, Event pred)
{
	int offset = getStoreOffset(addr, pred);
	addStoreToLoc(addr, store, offset + 1);
}

bool MOCalculator::isCoMaximal(SAddr addr, Event store)
{
	auto &locMO = stores[addr];
	return (store.isInitializer() && locMO.empty()) ||
	       (!store.isInitializer() && !locMO.empty() && store == locMO.back());
}

bool MOCalculator::isCachedCoMaximal(SAddr addr, Event store)
{
	return isCoMaximal(addr, store);
}

void MOCalculator::changeStoreOffset(SAddr addr, Event store, int newOffset)
{
	auto &locMO = stores[addr];

	locMO.erase(std::find(store_begin(addr), store_end(addr), store));
	locMO.insert(store_begin(addr) + newOffset, store);
	cacheMOIdxHints(addr, newOffset);
}

int MOCalculator::splitLocMOBefore(SAddr addr, Event e)
{
	const auto &g = getGraph();
	auto rit = std::find_if(store_rbegin(addr), store_rend(addr), [&](const Event &s){
		return g.isWriteRfBefore(s, e.prev());
	});
	return (rit == store_rend(addr)) ? 0 : std::distance(rit, store_rend(addr));
}

int MOCalculator::splitLocMOAfterHb(SAddr addr, const Event read)
{
	const auto &g = getGraph();

	auto initRfs = g.getInitRfsAtLoc(addr);
	if (std::any_of(initRfs.begin(), initRfs.end(), [&read,&g](const Event &rf){
		return g.getEventLabel(rf)->getHbView().contains(read);
	}))
		return 0;

	auto it = std::find_if(store_begin(addr), store_end(addr), [&](const Event &s){
		return g.isHbOptRfBefore(read, s);
	});
	if (it == store_end(addr))
		return std::distance(store_begin(addr), store_end(addr));
	return (g.getEventLabel(*it)->getHbView().contains(read)) ?
		std::distance(store_begin(addr), it) : std::distance(store_begin(addr), it) + 1;
}

int MOCalculator::splitLocMOAfter(SAddr addr, const Event e)
{
	const auto &g = getGraph();
	auto it = std::find_if(store_begin(addr), store_end(addr), [&](const Event &s){
		return g.isHbOptRfBefore(e, s);
	});
	return (it == store_end(addr)) ? std::distance(store_begin(addr), store_end(addr)) :
		std::distance(store_begin(addr), it);
}

std::vector<Event>
MOCalculator::getCoherentStores(SAddr addr, Event read)
{
	auto &g = getGraph();
	std::vector<Event> stores;

	/*
	 * If there are no stores (rf?;hb)-before the current event
	 * then we can read read from all concurrent stores and the
	 * initializer store. Otherwise, we can read from all concurrent
	 * stores and the mo-latest of the (rf?;hb)-before stores.
	 */
	auto begO = splitLocMOBefore(addr, read);
	if (begO == 0)
		stores.push_back(Event::getInitializer());
	else
		stores.push_back(*(store_begin(addr) + begO - 1));

	/*
	 * If the model supports out-of-order execution we have to also
	 * account for the possibility the read is hb-before some other
	 * store, or some read that reads from a store.
	 */
	auto endO = (supportsOutOfOrder()) ? splitLocMOAfterHb(addr, read) :
		std::distance(store_begin(addr), store_end(addr));
	stores.insert(stores.end(), store_begin(addr) + begO, store_begin(addr) + endO);
	return stores;
}

std::vector<Event>
MOCalculator::getMOOptRfAfter(const WriteLabel *sLab)
{
	std::vector<Event> after;

	std::for_each(co_succ_begin(sLab->getAddr(), sLab->getPos()),
		      co_succ_end(sLab->getAddr(), sLab->getPos()), [&](const Event &w){
			      auto *wLab = getGraph().getWriteLabel(w);
			      after.push_back(wLab->getPos());
			      after.insert(after.end(), wLab->readers_begin(), wLab->readers_end());
	});
	return after;
}

std::vector<Event>
MOCalculator::getMOInvOptRfAfter(const WriteLabel *sLab)
{
	std::vector<Event> after;

	/* First, add (mo;rf?)-before */
	std::for_each(co_pred_begin(sLab->getAddr(), sLab->getPos()),
		      co_pred_end(sLab->getAddr(), sLab->getPos()), [&](const Event &w){
			      auto *wLab = getGraph().getWriteLabel(w);
			      after.push_back(wLab->getPos());
			      after.insert(after.end(), wLab->readers_begin(), wLab->readers_end());
	});

	/* Then, we add the reader list for the initializer */
	auto initRfs = getGraph().getInitRfsAtLoc(sLab->getAddr());
	after.insert(after.end(), initRfs.begin(), initRfs.end());
	return after;
}

std::vector<Event>
MOCalculator::getCoherentRevisits(const WriteLabel *sLab)
{
	const auto &g = getGraph();
	auto ls = g.getRevisitable(sLab);

	/* If this store is po- and mo-maximal then we are done */
	if (!supportsOutOfOrder() && isCoMaximal(sLab->getAddr(), sLab->getPos()))
		return ls;

	/* First, we have to exclude (mo;rf?;hb?;sb)-after reads */
	auto optRfs = getMOOptRfAfter(sLab);
	ls.erase(std::remove_if(ls.begin(), ls.end(), [&](Event e)
				{ const View &before = g.getHbPoBefore(e);
				  return std::any_of(optRfs.begin(), optRfs.end(),
					 [&](Event ev)
					 { return before.contains(ev); });
				}), ls.end());

	/* If out-of-order event addition is not supported, then we are done
	 * due to po-maximality */
	if (!supportsOutOfOrder())
		return ls;

	/* Otherwise, we also have to exclude hb-before loads */
	ls.erase(std::remove_if(ls.begin(), ls.end(), [&](Event e)
		{ return g.getEventLabel(sLab->getPos())->getHbView().contains(e); }),
		ls.end());

	/* ...and also exclude (mo^-1; rf?; (hb^-1)?; sb^-1)-after reads in
	 * the resulting graph */
	auto &before = g.getPPoRfBefore(sLab->getPos());
	auto moInvOptRfs = getMOInvOptRfAfter(sLab);
	ls.erase(std::remove_if(ls.begin(), ls.end(), [&](Event e)
				{ auto *eLab = g.getEventLabel(e);
				  auto v = g.getDepViewFromStamp(eLab->getStamp());
				  v.update(before);
				  return std::any_of(moInvOptRfs.begin(),
						     moInvOptRfs.end(),
						     [&](Event ev)
						     { return v.contains(ev) &&
						       g.getHbPoBefore(ev).contains(e); });
				}),
		 ls.end());

	return ls;
}

#ifdef ENABLE_GENMC_DEBUG
std::vector<std::pair<Event, Event> >
MOCalculator::saveCoherenceStatus(const std::vector<std::unique_ptr<EventLabel> > &labs,
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

bool MOCalculator::isCoBeforeSavedPrefix(const BackwardRevisit &r, const EventLabel *lab)
{
	auto *mLab = llvm::dyn_cast<MemAccessLabel>(lab);
	if (!mLab)
		return false;

	auto &g = getGraph();
        auto v = g.getRevisitView(r);
	auto w = llvm::isa<ReadLabel>(mLab) ? llvm::dyn_cast<ReadLabel>(mLab)->getRf() : mLab->getPos();
	return any_of(co_succ_begin(mLab->getAddr(), w),
		      co_succ_end(mLab->getAddr(), w), [&](const Event &s){
			      auto *sLab = g.getEventLabel(s);
			      return v->contains(sLab->getPos()) &&
				     mLab->getIndex() > sLab->getPPoRfView()[mLab->getThread()] &&
				     sLab->getPos() != r.getRev();
		      });
}

bool MOCalculator::coherenceSuccRemainInGraph(const BackwardRevisit &r)
{
	auto &g = getGraph();
	auto *wLab = g.getWriteLabel(r.getRev());
	if (g.isRMWStore(wLab))
		return true;

	auto succIt = co_succ_begin(wLab->getAddr(), wLab->getPos());
	auto succE = co_succ_end(wLab->getAddr(), wLab->getPos());
	if (succIt == succE)
		return true;

	return g.getRevisitView(r)->contains(*succIt);
}

bool MOCalculator::wasAddedMaximally(const EventLabel *lab)
{
	if (auto *mLab = llvm::dyn_cast<MemAccessLabel>(lab))
		return mLab->wasAddedMax();
	if (auto *oLab = llvm::dyn_cast<OptionalLabel>(lab))
		return !oLab->isExpanded();
	return true;
}

bool MOCalculator::inMaximalPath(const BackwardRevisit &r)
{
	if (!coherenceSuccRemainInGraph(r))
		return false;

	auto &g = getGraph();
        auto v = g.getRevisitView(r);

	for (const auto *lab : labels(g)) {
		if ((lab->getPos() != r.getPos() && v->contains(lab->getPos())) ||
		    g.prefixContainsSameLoc(r, lab) ||
		    g.isOptBlockedRead(lab))
			continue;

		if (isCoBeforeSavedPrefix(r, lab))
			return false;
		if (g.hasBeenRevisitedByDeleted(r, lab))
			return false;
		if (!wasAddedMaximally(lab))
			return false;
	}
	return true;
}

void MOCalculator::initCalc()
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

Calculator::CalculationResult MOCalculator::doCalc()
{
	auto &gm = getGraph();
	auto &coRelation = gm.getPerLocRelation(ExecutionGraph::RelationId::co);

	for (auto locIt = begin(); locIt != end(); locIt++) {
		if (!coRelation[locIt->first].isIrreflexive())
			return Calculator::CalculationResult(false, false);
	}
	return Calculator::CalculationResult(false, true);
}

void MOCalculator::removeAfter(const VectorClock &preds)
{
	auto &g = getGraph();
	VSet<SAddr> keep;

	/* Check which locations should be kept */
	for (auto i = 0u; i < preds.size(); i++) {
		for (auto j = 0u; j <= preds[i]; j++) {
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

bool MOCalculator::locContains(SAddr addr, Event e) const
{
	BUG_ON(stores.count(addr) == 0);
	return e == Event::getInitializer() ||
		std::any_of(store_begin(addr), store_end(addr),
			    [&e](Event s){ return s == e; });
}

void MOCalculator::cacheMOIdxHints(SAddr addr, int start /* = -1 */) const
{
	auto &bucket = getStoresToLoc(addr);
	for (auto i = (start == -1) ? 0u : unsigned(start); i < bucket.size(); i++)
		getGraph().getWriteLabel(bucket[i])->setMOIdxHint(i);
}
