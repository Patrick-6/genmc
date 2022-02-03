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
#include "LabelIterator.hpp"
#include <vector>

MOCalculator::LocStores::const_iterator
MOCalculator::succ_begin(SAddr addr, Event store) const
{
	auto &locMO = getModOrderAtLoc(addr);
	auto offset = getStoreOffset(addr, store);
	return locMO.begin() + (offset + 1);
}

MOCalculator::LocStores::const_iterator
MOCalculator::succ_end(SAddr addr, Event store) const
{
	auto &locMO = getModOrderAtLoc(addr);
	return locMO.end();
}

MOCalculator::LocStores::const_iterator
MOCalculator::pred_begin(SAddr addr, Event store) const
{
	auto &locMO = getModOrderAtLoc(addr);
	return locMO.begin();
}

MOCalculator::LocStores::const_iterator
MOCalculator::pred_end(SAddr addr, Event store) const
{
	auto &locMO = getModOrderAtLoc(addr);
	auto offset = getStoreOffset(addr, store);
	return locMO.begin() + (offset >= 0 ? offset : 0);
}

void MOCalculator::trackCoherenceAtLoc(SAddr addr)
{
	mo_[addr];
}

int MOCalculator::getStoreOffset(SAddr addr, Event e) const
{
	BUG_ON(mo_.count(addr) == 0);

	if (e == Event::getInitializer())
		return -1;

	auto &locMO = mo_.at(addr);
	auto oIt = std::find(locMO.begin(), locMO.end(), e);
	BUG_ON(oIt == locMO.end());
	return std::distance(locMO.begin(), oIt);
}

const std::vector<Event>&
MOCalculator::getModOrderAtLoc(SAddr addr) const
{
	return getStoresToLoc(addr);
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
	mo_[addr].insert(mo_[addr].begin() + offset, store);
}

void MOCalculator::addStoreToLocAfter(SAddr addr, Event store, Event pred)
{
	int offset = getStoreOffset(addr, pred);
	addStoreToLoc(addr, store, offset + 1);
}

bool MOCalculator::isCoMaximal(SAddr addr, Event store)
{
	auto &locMO = mo_[addr];
	return (store.isInitializer() && locMO.empty()) ||
	       (!store.isInitializer() && !locMO.empty() && store == locMO.back());
}

bool MOCalculator::isCachedCoMaximal(SAddr addr, Event store)
{
	return isCoMaximal(addr, store);
}

void MOCalculator::changeStoreOffset(SAddr addr, Event store, int newOffset)
{
	auto &locMO = mo_[addr];

	locMO.erase(std::find(locMO.begin(), locMO.end(), store));
	locMO.insert(locMO.begin() + newOffset, store);
}

const std::vector<Event>&
MOCalculator::getStoresToLoc(SAddr addr) const
{
	BUG_ON(mo_.count(addr) == 0);
	return mo_.at(addr);
}

int MOCalculator::splitLocMOBefore(SAddr addr, Event e)
{
	const auto &g = getGraph();
	auto &locMO = getStoresToLoc(addr);

	auto rit = std::find_if(locMO.rbegin(), locMO.rend(), [&](const Event &s){
		return g.isWriteRfBefore(s, e.prev());
	});
	return (rit == locMO.rend()) ? 0 : std::distance(rit, locMO.rend());
}

int MOCalculator::splitLocMOAfterHb(SAddr addr, const Event read)
{
	const auto &g = getGraph();
	auto &locMO = getStoresToLoc(addr);

	auto initRfs = g.getInitRfsAtLoc(addr);
	if (std::any_of(initRfs.begin(), initRfs.end(), [&read,&g](const Event &rf){
		return g.getEventLabel(rf)->getHbView().contains(read);
	}))
		return 0;

	auto it = std::find_if(locMO.begin(), locMO.end(), [&](const Event &s){
		return g.isHbOptRfBefore(read, s);
	});
	if (it == locMO.end())
		return locMO.size();
	return (g.getEventLabel(*it)->getHbView().contains(read)) ?
		std::distance(locMO.begin(), it) : std::distance(locMO.begin(), it) + 1;
}

int MOCalculator::splitLocMOAfter(SAddr addr, const Event e)
{
	const auto &g = getGraph();
	auto &locMO = getStoresToLoc(addr);

	auto it = std::find_if(locMO.begin(), locMO.end(), [&](const Event &s){
		return g.isHbOptRfBefore(e, s);
	});
	return (it == locMO.end()) ? locMO.size() : std::distance(locMO.begin(), it);
}

std::vector<Event>
MOCalculator::getCoherentStores(SAddr addr, Event read)
{
	auto &g = getGraph();
	auto &locMO = getStoresToLoc(addr);
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
		stores.push_back(*(locMO.begin() + begO - 1));

	/*
	 * If the model supports out-of-order execution we have to also
	 * account for the possibility the read is hb-before some other
	 * store, or some read that reads from a store.
	 */
	auto endO = (supportsOutOfOrder()) ? splitLocMOAfterHb(addr, read) :
		std::distance(locMO.begin(), locMO.end());
	stores.insert(stores.end(), locMO.begin() + begO, locMO.begin() + endO);
	return stores;
}

std::vector<Event>
MOCalculator::getMOOptRfAfter(const WriteLabel *sLab)
{
	std::vector<Event> after;

	std::for_each(succ_begin(sLab->getAddr(), sLab->getPos()),
		      succ_end(sLab->getAddr(), sLab->getPos()), [&](const Event &w){
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
	std::for_each(pred_begin(sLab->getAddr(), sLab->getPos()),
		      pred_end(sLab->getAddr(), sLab->getPos()), [&](const Event &w){
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
		auto &locMO = getStoresToLoc(wLab->getAddr());
		auto moPos = std::find(locMO.begin(), locMO.end(), wLab->getPos());

		/* This store must definitely be in this location's MO */
		BUG_ON(moPos == locMO.end());

		/* We need to find the previous MO store that is in before or
		 * in the vector for which we are getting the predecessors */
		decltype(locMO.crbegin()) predPos(moPos);
		auto predFound = false;
		for (auto rit = predPos; rit != locMO.rend(); ++rit) {
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
	return any_of(succ_begin(mLab->getAddr(), w),
		      succ_end(mLab->getAddr(), w), [&](const Event &s){
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

	auto succIt = succ_begin(wLab->getAddr(), wLab->getPos());
	auto succE = succ_end(wLab->getAddr(), wLab->getPos());
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
	for (auto locIt = mo_.begin(); locIt != mo_.end(); locIt++) {
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

	for (auto it = begin(); it != end(); /* empty */) {
		it->second.erase(std::remove_if(it->second.begin(), it->second.end(),
						[&](Event &e)
						{ return !preds.contains(e); }),
				 it->second.end());

		/* Should we keep this memory location lying around? */
		if (!keep.count(it->first)) {
			BUG_ON(!it->second.empty());
			it = mo_.erase(it);
		} else {
			++it;
		}
	}
}

bool MOCalculator::locContains(SAddr addr, Event e) const
{
	BUG_ON(mo_.count(addr) == 0);
	return e == Event::getInitializer() ||
	       std::any_of(mo_.at(addr).begin(), mo_.at(addr).end(),
			   [&e](Event s){ return s == e; });
}
