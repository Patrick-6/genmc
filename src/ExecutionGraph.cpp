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

#include "Library.hpp"
#include "Parser.hpp"
#include "ExecutionGraph.hpp"
#include <llvm/IR/DebugInfo.h>

/************************************************************
 ** Class Constructors
 ***********************************************************/

ExecutionGraph::ExecutionGraph() : timestamp(1)
{
	/* Create an entry for main() and push the "initializer" label */
	events.push_back({});
	events[0].emplace_back( std::make_unique<EventLabel>(
				     EStart, llvm::AtomicOrdering::Acquire,
				     Event(0, 0), Event::getInitializer()
				) );
}


/************************************************************
 ** Basic getter methods
 ***********************************************************/

unsigned int ExecutionGraph::nextStamp()
{
	return timestamp++;
}

EventLabel& ExecutionGraph::getEventLabel(Event e)
{
	return *events[e.thread][e.index];
}

EventLabel& ExecutionGraph::getPreviousLabel(Event e)
{
	return *events[e.thread][e.index-1];
}

EventLabel& ExecutionGraph::getLastThreadLabel(int thread)
{
	return *events[thread][events[thread].size() - 1];
}

Event ExecutionGraph::getLastThreadEvent(int thread) const
{
	return Event(thread, events[thread].size() - 1);
}

Event ExecutionGraph::getLastThreadRelease(int thread, const llvm::GenericValue *addr) const
{
	for (int i = events[thread].size() - 1; i > 0; i--) {
		const auto &lab = *events[thread][i];
		if (lab.isFence() && lab.isAtLeastRelease())
			return Event(thread, i);
		if (lab.isWrite() && lab.isAtLeastRelease() && lab.getAddr() == addr)
			return Event(thread, i);
	}
	return Event(thread, 0);
}

/*
 * Given an write label sLab that is part of an RMW, return all
 * other RMWs that read from the same write. Of course, there must
 * be _at most_ one other RMW reading from the same write (see [Rex] set)
 */
std::vector<Event> ExecutionGraph::getPendingRMWs(const EventLabel &sLab)
{
	std::vector<Event> pending;

	/* This function should be called with a write event */
	BUG_ON(!sLab.isWrite());

	/* If this is _not_ an RMW event, return an empty vector */
	if (!sLab.isRMW())
		return pending;

	/* Otherwise, scan for other RMWs that successfully read the same val */
	auto &pLab = getPreviousLabel(sLab.getPos());
	for (auto i = 0u; i < events.size(); i++) {
		for (auto j = 1u; j < events[i].size(); j++) { /* Skip thread start */
			EventLabel &lab = getEventLabel(Event(i, j));
			if (isRMWLoad(lab.getPos()) && lab.rf == pLab.rf &&
			    lab.getAddr() == pLab.getAddr() && lab.getPos() != pLab.getPos())
				pending.push_back(lab.getPos());
		}
	}
	BUG_ON(pending.size() > 1);
	return pending;
}

Event ExecutionGraph::getPendingLibRead(const EventLabel &lab)
{
	/* Should only be called with a read of a functional library that doesn't read BOT */
	BUG_ON(!lab.isRead() || lab.rf == Event::getInitializer());

	/* Get the conflicting label */
	auto &sLab = getEventLabel(lab.rf);
	auto it = std::find_if(sLab.rfm1.begin(), sLab.rfm1.end(), [&lab](Event &e){ return e != lab.getPos(); });
	BUG_ON(it == sLab.rfm1.end());
	return *it;
}

std::vector<Event> ExecutionGraph::getRevisitable(const EventLabel &sLab)
{
	std::vector<Event> loads;

	BUG_ON(!sLab.isWrite());
	auto &before = getPorfBefore(sLab.getPos());
	for (auto i = 0u; i < events.size(); i++) {
		for (auto j = before[i] + 1u; j < events[i].size(); j++) {
			auto &lab = getEventLabel(Event(i, j));
			if (lab.isRead() && lab.getAddr() == sLab.getAddr() && lab.isRevisitable())
				loads.push_back(lab.getPos());
		}
	}
	return loads;
}

std::vector<Event> ExecutionGraph::getMoOptRfAfter(Event store)
{
	auto ls = modOrder.getMoAfter(getEventLabel(store).getAddr(), store);
	std::vector<Event> rfs;

	/*
	 * We push the RFs to a different vector in order
	 * not to invalidate the iterator
	 */
	for (auto it = ls.begin(); it != ls.end(); ++it) {
		auto &lab = getEventLabel(*it);
		if (lab.isWrite()) {
			for (auto &l : lab.rfm1)
				rfs.push_back(l);
		}
	}
	std::move(rfs.begin(), rfs.end(), std::back_inserter(ls));
	return ls;
}


/************************************************************
 ** Basic setter methods
 ***********************************************************/

void ExecutionGraph::calcLoadPoRfView(EventLabel &lab)
{
	lab.porfView = getPreviousLabel(lab.getPos()).getPorfView();
	++lab.porfView[lab.getThread()];

	lab.porfView.update(getEventLabel(lab.rf).getPorfView());
}

void ExecutionGraph::calcLoadHbView(EventLabel &lab)
{
	lab.hbView = getPreviousLabel(lab.getPos()).getHbView();
	++lab.hbView[lab.getThread()];

	if (lab.isAtLeastAcquire())
		lab.hbView.update(getEventLabel(lab.rf).getMsgView());
}

EventLabel& ExecutionGraph::addEventToGraph(std::unique_ptr<EventLabel> lab)
{
	auto thread = lab->getThread();
	lab->stamp = nextStamp();
	events[thread].push_back(std::move(lab));
	return getLastThreadLabel(thread);
}

EventLabel& ExecutionGraph::addReadToGraphCommon(std::unique_ptr<EventLabel> lab, Event rf)
{
	lab->makeRevisitable();
	calcLoadHbView(*lab);
	calcLoadPoRfView(*lab);

	auto &nLab = addEventToGraph(std::move(lab));

	if (rf.isInitializer())
		return nLab;

	auto &rfLab = *events[rf.thread][rf.index];
	rfLab.rfm1.push_back(nLab.getPos());
	return nLab;
}

EventLabel& ExecutionGraph::addReadToGraph(int tid, EventAttr attr, llvm::AtomicOrdering ord,
					   const llvm::GenericValue *ptr, llvm::Type *typ, Event rf,
					   llvm::GenericValue &&cmpVal, llvm::GenericValue &&rmwVal,
					   llvm::AtomicRMWInst::BinOp op)
{
	int max = events[tid].size();
	auto lab = std::make_unique<EventLabel>(
		ERead, attr, ord, Event(tid, max), ptr, typ, rf, cmpVal, rmwVal, op);
	return addReadToGraphCommon(std::move(lab), rf);
}

EventLabel& ExecutionGraph::addLibReadToGraph(int tid, EventAttr attr, llvm::AtomicOrdering ord,
					      const llvm::GenericValue *ptr, llvm::Type *typ, Event rf,
					      std::string functionName)
{
	int max = events[tid].size();
	auto lab = std::make_unique<EventLabel>(
		ERead, attr, ord, Event(tid, max), ptr, typ, rf, functionName);
	return addReadToGraphCommon(std::move(lab), rf);
}

EventLabel& ExecutionGraph::addStoreToGraphCommon(std::unique_ptr<EventLabel> lab)
{
	auto thread = lab->getThread();
	auto last = getLastThreadEvent(thread);

	lab->hbView = getHbBefore(last);
	++lab->hbView[thread];
	lab->porfView = getPorfBefore(last);
	++lab->porfView[thread];
	if (lab->isRMW()) {
		EventLabel &pLab = getEventLabel(last);
		const View &mV = getMsgView(pLab.rf);
		BUG_ON(pLab.ord == llvm::AtomicOrdering::NotAtomic);
		if (pLab.isAtLeastRelease())
			/* no need for ctor -- getMax copies the View */
			lab->msgView = mV.getMax(lab->hbView);
		else {
			lab->msgView = mV.getMax(getHbBefore(getLastThreadRelease(thread, lab->getAddr())));
		}
	} else {
		if (lab->isAtLeastRelease())
			lab->msgView = lab->hbView;
		else if (lab->ord == llvm::AtomicOrdering::Monotonic ||
			 lab->ord == llvm::AtomicOrdering::Acquire)
			lab->msgView = getHbBefore(getLastThreadRelease(thread, lab->getAddr()));
	}
	return addEventToGraph(std::move(lab));
}

EventLabel& ExecutionGraph::addStoreToGraph(int tid, EventAttr attr, llvm::AtomicOrdering ord,
					    const llvm::GenericValue *ptr, llvm::Type *typ,
					    llvm::GenericValue &val, int offsetMO)
{
	int max = events[tid].size();
	auto lab = std::make_unique<EventLabel>(
		EWrite, attr, ord, Event(tid, max), ptr, typ, val);
	modOrder[lab->getAddr()].insert(modOrder[lab->getAddr()].begin() + offsetMO, lab->getPos());
	return addStoreToGraphCommon(std::move(lab));
}

EventLabel& ExecutionGraph::addLibStoreToGraph(int tid, EventAttr attr, llvm::AtomicOrdering ord,
					       const llvm::GenericValue *ptr, llvm::Type *typ,
					       llvm::GenericValue &val, int offsetMO,
					       std::string functionName, bool isInit)
{
	int max = events[tid].size();
	auto lab = std::make_unique<EventLabel>(
		EWrite, ATTR_PLAIN, ord, Event(tid, max), ptr, typ, val, functionName, isInit);
	modOrder[lab->getAddr()].insert(modOrder[lab->getAddr()].begin() + offsetMO, lab->getPos());
	return addStoreToGraphCommon(std::move(lab));
}

EventLabel& ExecutionGraph::addFenceToGraph(int tid, llvm::AtomicOrdering ord)

{
	int max = events[tid].size();
	auto lab = std::make_unique<EventLabel>(EFence, ord, Event(tid, max));
	Event last = getLastThreadEvent(tid);

	lab->hbView = getHbBefore(last);
	++lab->hbView[tid];
	lab->porfView = getPorfBefore(last);
	++lab->porfView[tid];
	if (lab->isAtLeastAcquire())
		calcRelRfPoBefore(tid, last.index, lab->hbView);
	return addEventToGraph(std::move(lab));
}

EventLabel& ExecutionGraph::addMallocToGraph(int tid, const llvm::GenericValue *addr, const llvm::GenericValue &val)
{
	int max = events[tid].size();
	auto lab = std::make_unique<EventLabel>(EMalloc, Event(tid, max), addr, val);
	Event last = getLastThreadEvent(tid);

	lab->hbView = getHbBefore(last);
	++lab->hbView[tid];
	lab->porfView = getPorfBefore(last);
	++lab->porfView[tid];
	return addEventToGraph(std::move(lab));
}

EventLabel& ExecutionGraph::addFreeToGraph(int tid, const llvm::GenericValue *addr, const llvm::GenericValue &val)
{
	int max = events[tid].size();
	auto lab = std::make_unique<EventLabel>(EFree, Event(tid, max), addr, val);
	Event last = getLastThreadEvent(tid);

	lab->hbView = getHbBefore(last);
	++lab->hbView[tid];
	lab->porfView = getPorfBefore(last);
	++lab->porfView[tid];
	return addEventToGraph(std::move(lab));
}

EventLabel& ExecutionGraph::addTCreateToGraph(int tid, int cid)
{
	int max = events[tid].size();
	auto lab = std::make_unique<EventLabel>(ETCreate, llvm::AtomicOrdering::Release, Event(tid, max), cid);
	Event last = getLastThreadEvent(tid);

	lab->rfm1.push_back(Event(cid, 0));

	lab->porfView = getPorfBefore(last);
	++lab->porfView[tid];

	/* Thread creation has Release semantics */
	lab->hbView = getHbBefore(last);
	++lab->hbView[tid];
	lab->msgView = lab->hbView;
	return addEventToGraph(std::move(lab));
}

EventLabel& ExecutionGraph::addTJoinToGraph(int tid, int cid)
{
	int max = events[tid].size();
	auto lab = std::make_unique<EventLabel>(ETJoin, llvm::AtomicOrdering::Acquire, Event(tid, max), cid);
	Event last = getLastThreadEvent(tid);

	lab->rf = Event::getInitializer();
	lab->porfView = getPorfBefore(last);
	lab->porfView[tid] = max;

	/* Thread joins have acquire semantics -- but we have to wait
	 * for the other thread to finish first, so we do not fully
	 * update the view yet */
	lab->hbView = getHbBefore(last);
	lab->hbView[tid] = max;
	return addEventToGraph(std::move(lab));
}

EventLabel& ExecutionGraph::addStartToGraph(int tid, Event tc)
{
	int max = events[tid].size();
	auto lab = std::make_unique<EventLabel>(EStart, llvm::AtomicOrdering::Acquire, Event(tid, max), tc);

	lab->porfView = getPorfBefore(tc);
	lab->porfView[tid] = max;

	/* Thread start has Acquire semantics */
	lab->hbView = getHbBefore(tc);
	lab->hbView[tid] = max;

	return addEventToGraph(std::move(lab));
}

EventLabel& ExecutionGraph::addFinishToGraph(int tid)
{
	int max = events[tid].size();
	auto lab = std::make_unique<EventLabel>(EFinish, llvm::AtomicOrdering::Release, Event(tid, max));
	Event last = getLastThreadEvent(tid);

	lab->porfView = getPorfBefore(last);
	lab->porfView[tid] = max;

	/* Thread termination has Release semantics */
	lab->hbView = getHbBefore(last);
	lab->hbView[tid] = max;
	lab->msgView = lab->hbView;
	return addEventToGraph(std::move(lab));
}

/************************************************************
 ** Calculation of [(po U rf)*] predecessors and successors
 ***********************************************************/

std::vector<Event> ExecutionGraph::getStoresHbAfterStores(const llvm::GenericValue *loc,
							  const std::vector<Event> &chain)
{
	auto &stores = modOrder[loc];
	std::vector<Event> result;

	for (auto &s : stores) {
		if (std::find(chain.begin(), chain.end(), s) != chain.end())
			continue;
		auto &before = getHbBefore(s);
		if (std::any_of(chain.begin(), chain.end(), [&before](Event e)
				{ return e.index < before[e.thread]; }))
			result.push_back(s);
	}
	return result;
}

const View &ExecutionGraph::getMsgView(Event e)
{
	return getEventLabel(e).getMsgView();
}

const View &ExecutionGraph::getPorfBefore(Event e)
{
	return getEventLabel(e).getPorfView();
}

const View &ExecutionGraph::getHbBefore(Event e)
{
	return getEventLabel(e).getHbView();
}

View ExecutionGraph::getHbBefore(const std::vector<Event> &es)
{
	View v;

	for (auto &e : es)
		v.update(getEventLabel(e).getHbView());
	return v;
}

const View &ExecutionGraph::getHbPoBefore(Event e)
{
	return getHbBefore(e.prev());
}

void ExecutionGraph::calcHbRfBefore(Event e, const llvm::GenericValue *addr,
				    View &a)
{
	if (a.contains(e))
		return;
	int ai = a[e.thread];
	a[e.thread] = e.index;
	for (int i = ai + 1; i <= e.index; i++) {
		EventLabel &lab = getEventLabel(Event(e.thread, i));
		if (lab.isRead() && !lab.rf.isInitializer() &&
		    (lab.getAddr() == addr || lab.hbView.contains(lab.rf)))
			calcHbRfBefore(lab.rf, addr, a);
	}
	return;
}

View ExecutionGraph::getHbRfBefore(const std::vector<Event> &es)
{
	View a;

	for (auto &e : es)
		calcHbRfBefore(e, getEventLabel(e).getAddr(), a);
	return a;
}

void ExecutionGraph::calcRelRfPoBefore(int thread, int index, View &v)
{
	for (auto i = index; i > 0; i--) {
		EventLabel &lab = getEventLabel(Event(thread, i));
		if (lab.isFence() && lab.isAtLeastAcquire())
			return;
		if (lab.isRead() && (lab.ord == llvm::AtomicOrdering::Monotonic ||
				     lab.ord == llvm::AtomicOrdering::Release))
			v.update(getEventLabel(lab.rf).getMsgView());
	}
}


/************************************************************
 ** Calculation of particular sets of events/event labels
 ***********************************************************/

std::vector<std::unique_ptr<EventLabel> >
ExecutionGraph::getPrefixLabelsNotBefore(const View &prefix, const View &before)
{
	std::vector<std::unique_ptr<EventLabel> > result;

	for (auto i = 0u; i < events.size(); i++) {
		for (auto j = before[i] + 1; j <= prefix[i]; j++) {
			EventLabel &lab = getEventLabel(Event(i, j));
			result.push_back(std::make_unique<EventLabel>(lab));

			auto &curLab = result.back();
			if (!curLab->hasWriteSem())
				continue;
			curLab->rfm1.erase(std::remove_if(curLab->rfm1.begin(), curLab->rfm1.end(),
							 [&](Event &e)
							 { return !prefix.contains(e) &&
								  !before.contains(e); }),
					  curLab->rfm1.end());
		}
	}
	return result;
}

std::vector<Event>
ExecutionGraph::getRfsNotBefore(const std::vector<std::unique_ptr<EventLabel> > &labs,
				const View &before)
{
	std::vector<Event> rfs;

	for (auto const &lab : labs) {
		if (lab->isRead() && !before.contains(lab->getPos()))
			rfs.push_back(lab->rf);
	}
	return rfs;
}

std::vector<std::pair<Event, Event> >
ExecutionGraph::getMOPredsInBefore(const std::vector<std::unique_ptr<EventLabel> > &labs,
				   const View &before)
{
	std::vector<std::pair<Event, Event> > pairs;

	for (auto const &lab : labs) {
		/* Only store MO pairs for labels that are not in before */
		if (!lab->isWrite() || before.contains(lab->getPos()))
			continue;

		auto &locMO = modOrder[lab->getAddr()];
		auto moPos = std::find(locMO.begin(), locMO.end(), lab->getPos());

		/* This store must definitely be in this location's MO */
		BUG_ON(moPos == locMO.end());

		/* We need to find the previous MO store that is in before or
		 * in the vector for which we are getting the predecessors */
		std::reverse_iterator<std::vector<Event>::iterator> predPos(moPos);
		auto predFound = false;
		for (auto rit = predPos; rit != locMO.rend(); ++rit) {
			if (before.contains(*rit) ||
			    std::find_if(labs.begin(), labs.end(),
					 [rit](const std::unique_ptr<EventLabel> &lab)
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


/************************************************************
 ** Calculation of writes a read can read from
 ***********************************************************/

bool ExecutionGraph::isWriteRfBefore(const View &before, Event e)
{
	if (before.contains(e))
		return true;

	auto &lab = getEventLabel(e);
	BUG_ON(!lab.isWrite());
	for (auto &e : lab.rfm1)
		if (before.contains(e))
			return true;
	return false;
}

bool ExecutionGraph::isStoreReadByExclusiveRead(Event store, const llvm::GenericValue *ptr)
{
	for (auto i = 0u; i < events.size(); i++) {
		for (auto j = 0u; j < events[i].size(); j++) {
			auto &lab = getEventLabel(Event(i, j));
			if (isRMWLoad(lab.getPos()) && lab.rf == store && lab.getAddr() == ptr)
				return true;
		}
	}
	return false;
}

bool ExecutionGraph::isStoreReadBySettledRMW(Event store, const llvm::GenericValue *ptr, const View &porfBefore)
{
	for (auto i = 0u; i < events.size(); i++) {
		for (auto j = 0u; j < events[i].size(); j++) {
			auto &lab = getEventLabel(Event(i, j));
			if (isRMWLoad(lab.getPos()) && lab.rf == store && lab.getAddr() == ptr) {
				if (!lab.isRevisitable())
					return true;
				if (porfBefore.contains(lab.getPos()))
					return true;
			}
		}
	}
	return false;
}

std::vector<Event> ExecutionGraph::findOverwrittenBoundary(const llvm::GenericValue *addr, int thread)
{
	std::vector<Event> boundary;
	auto &before = getHbBefore(getLastThreadEvent(thread));

	if (before.empty())
		return boundary;

	for (auto &e : modOrder[addr])
		if (isWriteRfBefore(before, e))
			boundary.push_back(e.prev());
	return boundary;
}


/************************************************************
 ** Graph modification methods
 ***********************************************************/

void ExecutionGraph::changeRf(EventLabel &lab, Event store)
{
	Event oldRf = lab.rf;
	lab.rf = store;

	/* Make sure that the old write it was reading from still exists */
	if (oldRf.index < (int) events[oldRf.thread].size() && !oldRf.isInitializer()) {
		EventLabel &oldLab = getEventLabel(oldRf);
		oldLab.rfm1.erase(std::remove(oldLab.rfm1.begin(), oldLab.rfm1.end(), lab.getPos()),
				  oldLab.rfm1.end());
	}
	if (!store.isInitializer()) {
		EventLabel &sLab = getEventLabel(store);
		sLab.rfm1.push_back(lab.getPos());
	}
	/* Update the views of the load */
	calcLoadHbView(lab);
	calcLoadPoRfView(lab);
}


void ExecutionGraph::cutToView(const View &preds)
{
	/* Restrict the graph according to the view */
	for (auto i = 0u; i < events.size(); i++) {
		auto &thr = events[i];
		thr.erase(thr.begin() + preds[i] + 1, thr.end());
	}

	/* Remove any 'pointers' to events that have been removed */
	for (auto i = 0u; i < events.size(); i++) {
		auto &thr = events[i];
		for (auto j = 0u; j < thr.size(); j++) {
			auto &lab = *thr[j];
			/*
			 * If it is a join and the respective Finish has been
			 * removed, renew the Views of this label and continue
			 */
			if (lab.isJoin() &&
			    lab.rf.index >= (int) events[lab.rf.thread].size()) {
				auto init = Event::getInitializer();
				lab.rf = init;
				calcLoadHbView(lab);
				calcLoadPoRfView(lab);
				continue;
			}

			/* If it hasn't write semantics, nothing to do */
			if (!lab.hasWriteSem())
				continue;

			lab.rfm1.erase(std::remove_if(lab.rfm1.begin(), lab.rfm1.end(),
						      [&](Event &e)
						      { return !preds.contains(e); }),
				       lab.rfm1.end());
		}
	}

	/* Remove cutted events from the modification order as well */
	for (auto it = modOrder.begin(); it != modOrder.end(); ++it)
		it->second.erase(std::remove_if(it->second.begin(), it->second.end(),
						[&preds](Event &e)
						{ return !preds.contains(e); }),
				 it->second.end());
	return;
}

View ExecutionGraph::getViewFromStamp(unsigned int stamp)
{
	View preds;

	for (auto i = 0u; i < events.size(); i++) {
		for (auto j = (int) events[i].size() - 1; j >= 0; j--) {
			auto &lab = getEventLabel(Event(i, j));
			if (lab.getStamp() <= stamp) {
				preds[i] = j;
				break;
			}
		}
	}
	return preds;
}

void ExecutionGraph::cutToStamp(unsigned int stamp)
{
	auto preds = getViewFromStamp(stamp);
	cutToView(preds);
	return;
}

void ExecutionGraph::restoreStorePrefix(EventLabel &rLab, std::vector<std::unique_ptr<EventLabel> > &storePrefix,
					std::vector<std::pair<Event, Event> > &moPlacings)
{
	std::vector<Event> inserted;

	for (auto &lab : storePrefix) {
		BUG_ON(lab->getIndex() != (int) events[lab->getThread()].size() &&
		       "Events should be added in order!");
		inserted.push_back(lab->getPos());
		events[lab->getThread()].push_back(std::move(lab));
	}

	for (auto const &e : inserted) {
		EventLabel &curLab = getEventLabel(e);
		curLab.makeNotRevisitable();
		if (curLab.isRead() && !curLab.rf.isInitializer()) {
			EventLabel &rfLab = getEventLabel(curLab.rf);
			if (std::find(rfLab.rfm1.begin(), rfLab.rfm1.end(), curLab.getPos())
			    == rfLab.rfm1.end())
				rfLab.rfm1.push_back(curLab.getPos());
		}
		curLab.rfm1.erase(std::remove(curLab.rfm1.begin(), curLab.rfm1.end(), rLab.getPos()),
				  curLab.rfm1.end());
	}

	/* If there are no specific mo placings, just insert all stores */
	if (moPlacings.empty()) {
		for (auto const &e : inserted) {
			auto &lab = getEventLabel(e);
			if (lab.isWrite())
				modOrder.addAtLocEnd(lab.getAddr(), lab.getPos());
		}
		return;
	}

	/* Otherwise, insert the writes of storePrefix into the appropriate places */
	auto insertedMO = 0u;
	while (insertedMO < moPlacings.size()) {
		for (auto it = moPlacings.begin(); it != moPlacings.end(); ++it) {
			auto &lab = getEventLabel(it->first);
			if (modOrder.locContains(lab.getAddr(), it->second) &&
			    !modOrder.locContains(lab.getAddr(), it->first)) {
				modOrder.addAtLocAfter(lab.getAddr(), it->second, it->first);
				++insertedMO;
			}
		}
	}
}


/************************************************************
 ** Consistency checks
 ***********************************************************/

bool ExecutionGraph::isConsistent(void)
{
	return true;
}


/************************************************************
 ** Race detection methods
 ***********************************************************/

Event ExecutionGraph::findRaceForNewLoad(Event e)
{
	auto &lab = getEventLabel(e);
	auto &before = getHbBefore(lab.getPos().prev());
	auto &stores = modOrder[lab.getAddr()];

	/* If there are not any events hb-before the read, there is nothing to do */
	if (before.empty())
		return Event::getInitializer();

	/* Check for events that race with the current load */
	for (auto &s : stores) {
		if (s.index > before[s.thread]) {
			EventLabel &sLab = getEventLabel(s);
			if ((lab.isNotAtomic() || sLab.isNotAtomic()) &&
			    lab.getPos() != sLab.getPos())
				return s; /* Race detected! */
		}
	}
	return Event::getInitializer(); /* Race not found */
}

Event ExecutionGraph::findRaceForNewStore(Event e)
{
	auto &lab = getEventLabel(e);
	auto &before = getHbBefore(lab.getPos().prev());

	for (auto i = 0u; i < events.size(); i++) {
		for (auto j = before[i] + 1u; j < events[i].size(); j++) {
			EventLabel &oLab = getEventLabel(Event(i, j));
			if ((oLab.isRead() || oLab.isWrite()) &&
			    oLab.getAddr() == lab.getAddr() &&
			    oLab.getPos() != lab.getPos())
				if (lab.isNotAtomic() || oLab.isNotAtomic())
					return oLab.getPos(); /* Race detected! */
		}
	}
	return Event::getInitializer(); /* Race not found */
}


/************************************************************
 ** Calculation utilities
 ***********************************************************/

/************************************************************
 ** PSC calculation
 ***********************************************************/

bool ExecutionGraph::isRMWLoad(Event e)
{
	EventLabel &lab = getEventLabel(e);
	if (lab.isWrite() || !lab.isRMW())
		return false;
	if (e.index == (int) events[e.thread].size() - 1)
		return false;

	EventLabel &labNext = getEventLabel(Event(e.thread, e.index + 1));
	if (labNext.isRMW() && labNext.isWrite() && lab.getAddr() == labNext.getAddr())
		return true;
	return false;
}

std::pair<std::vector<Event>, std::vector<Event> >
ExecutionGraph::getSCs()
{
	std::vector<Event> scs, fcs;

	for (auto i = 0u; i < events.size(); i++) {
		for (auto j = 0u; j < events[i].size(); j++) {
			EventLabel &lab = getEventLabel(Event(i, j));
			if (lab.isSC() && !isRMWLoad(lab.getPos()))
				scs.push_back(lab.getPos());
			if (lab.isFence() && lab.isSC())
				fcs.push_back(lab.getPos());
		}
	}
	return std::make_pair(scs,fcs);
}

std::vector<const llvm::GenericValue *> ExecutionGraph::getDoubleLocs()
{
	std::vector<const llvm::GenericValue *> singles, doubles;

	for (auto i = 0u; i < events.size(); i++) {
		for (auto j = 1u; j < events[i].size(); j++) { /* Do not consider thread inits */
			EventLabel &lab = getEventLabel(Event(i, j));
			if (!(lab.isRead() || lab.isWrite()))
				continue;
			if (std::find(doubles.begin(), doubles.end(), lab.getAddr()) != doubles.end())
				continue;
			if (std::find(singles.begin(), singles.end(), lab.getAddr()) != singles.end()) {
				singles.erase(std::remove(singles.begin(), singles.end(), lab.getAddr()),
					      singles.end());
				doubles.push_back(lab.getAddr());
			} else {
				singles.push_back(lab.getAddr());
			}
		}
	}
	return doubles;
}

std::vector<Event> ExecutionGraph::calcSCFencesSuccs(const std::vector<Event> &fcs, const Event e)
{
	std::vector<Event> succs;

	if (isRMWLoad(e))
		return succs;
	for (auto &f : fcs) {
		if (getHbBefore(f).contains(e))
			succs.push_back(f);
	}
	return succs;
}

std::vector<Event> ExecutionGraph::calcSCFencesPreds(const std::vector<Event> &fcs, const Event e)
{
	std::vector<Event> preds;
	auto &before = getHbBefore(e);

	if (isRMWLoad(e))
		return preds;
	for (auto &f : fcs) {
		if (before.contains(f))
			preds.push_back(f);
	}
	return preds;
}

std::vector<Event> ExecutionGraph::calcSCSuccs(const std::vector<Event> &fcs, const Event e)
{
	EventLabel &lab = getEventLabel(e);

	if (isRMWLoad(e))
		return {};
	if (lab.isSC())
		return {e};
	else
		return calcSCFencesSuccs(fcs, e);
}

std::vector<Event> ExecutionGraph::calcSCPreds(const std::vector<Event> &fcs, const Event e)
{
	EventLabel &lab = getEventLabel(e);

	if (isRMWLoad(e))
		return {};
	if (lab.isSC())
		return {e};
	else
		return calcSCFencesPreds(fcs, e);
}

std::vector<Event> ExecutionGraph::getSCRfSuccs(const std::vector<Event> &fcs, const Event ev)
{
	auto &lab = getEventLabel(ev);
	std::vector<Event> rfs;

	BUG_ON(!lab.isWrite());
	for (auto &e : lab.rfm1) {
		auto succs = calcSCSuccs(fcs, e);
		rfs.insert(rfs.end(), succs.begin(), succs.end());
	}
	return rfs;
}

std::vector<Event> ExecutionGraph::getSCFenceRfSuccs(const std::vector<Event> &fcs, const Event ev)
{
	auto &lab = getEventLabel(ev);
	std::vector<Event> fenceRfs;

	BUG_ON(!lab.isWrite());
	for (auto &e : lab.rfm1) {
		auto fenceSuccs = calcSCFencesSuccs(fcs, e);
		fenceRfs.insert(fenceRfs.end(), fenceSuccs.begin(), fenceSuccs.end());
	}
	return fenceRfs;
}

void ExecutionGraph::addRbEdges(std::vector<Event> &fcs, std::vector<Event> &moAfter,
				std::vector<Event> &moRfAfter, Matrix2D<Event> &matrix,
				const Event &ev)
{
	auto &lab = getEventLabel(ev);

	BUG_ON(!lab.isWrite());
	for (auto &e : lab.rfm1) {
		auto preds = calcSCPreds(fcs, e);
		auto fencePreds = calcSCFencesPreds(fcs, e);

		matrix.addEdgesFromTo(preds, moAfter);        /* PSC_base: Adds rb-edges */
		matrix.addEdgesFromTo(fencePreds, moRfAfter); /* PSC_fence: Adds (rb;rf)-edges */
	}
	return;
}

void ExecutionGraph::addMoRfEdges(std::vector<Event> &fcs, std::vector<Event> &moAfter,
				  std::vector<Event> &moRfAfter, Matrix2D<Event> &matrix,
				  const Event &ev)
{
	auto preds = calcSCPreds(fcs, ev);
	auto fencePreds = calcSCFencesPreds(fcs, ev);
	auto rfs = getSCRfSuccs(fcs, ev);

	matrix.addEdgesFromTo(preds, moAfter);        /* PSC_base:  Adds mo-edges */
	matrix.addEdgesFromTo(preds, rfs);            /* PSC_base:  Adds rf-edges */
	matrix.addEdgesFromTo(fencePreds, moRfAfter); /* PSC_fence: Adds (mo;rf)-edges */
	return;
}

/*
 * addSCEcos - Helper function that calculates a part of PSC_base and PSC_fence
 *
 * For PSC_base, it adds mo, rb, and hb_loc edges. The procedure for mo and rb
 * is straightforward: at each point, we only need to keep a list of all the
 * mo-after writes that are either SC, or can reach an SC fence. For hb_loc,
 * however, we only consider rf-edges because the other cases are implicitly
 * covered (sb, mo, etc).
 *
 * For PSC_fence, it adds (mo;rf)- and (rb;rf)-edges. Simple cases like
 * mo, rf, and rb are covered by PSC_base, and all other combinations with
 * more than one step either do not compose, or lead to an already added
 * single-step relation (e.g, (rf;rb) => mo, (rb;mo) => rb)
 */
void ExecutionGraph::addSCEcos(std::vector<Event> &fcs, std::vector<Event> &mo, Matrix2D<Event> &matrix)
{
	std::vector<Event> moAfter;   /* mo-after SC writes or writes that reach an SC fence */
	std::vector<Event> moRfAfter; /* SC fences that can be reached by (mo;rf)-after reads */

	for (auto rit = mo.rbegin(); rit != mo.rend(); rit++) {

		addRbEdges(fcs, moAfter, moRfAfter, matrix, *rit);
		addMoRfEdges(fcs, moAfter, moRfAfter, matrix, *rit);

		auto succs = calcSCSuccs(fcs, *rit);
		auto fenceRfs = getSCFenceRfSuccs(fcs, *rit);
		moAfter.insert(moAfter.end(), succs.begin(), succs.end());
		moRfAfter.insert(moRfAfter.end(), fenceRfs.begin(), fenceRfs.end());
	}
}

/*
 * addSCWbEcos is a helper function that calculates a part of PSC_base and PSC_fence,
 * like addSCEcos. The difference between them lies in the fact that addSCEcos
 * uses MO for adding mo and rb edges, addSCWBEcos uses WB for that.
 */
void ExecutionGraph::addSCWbEcos(std::vector<Event> &fcs,
				 Matrix2D<Event> &wbMatrix,
				 Matrix2D<Event> &pscMatrix)
{
	auto &stores = wbMatrix.getElems();
	for (auto i = 0u; i < stores.size(); i++) {

		/*
		 * Calculate which of the stores are wb-after the current
		 * write, and then collect wb-after and (wb;rf)-after SC successors
		 */
		std::vector<Event> wbAfter, wbRfAfter;
		for (auto j = 0u; j < stores.size(); j++) {
			if (wbMatrix(i, j)) {
				auto succs = calcSCSuccs(fcs, stores[j]);
				auto fenceRfs = getSCFenceRfSuccs(fcs, stores[j]);
				wbAfter.insert(wbAfter.end(), succs.begin(), succs.end());
				wbRfAfter.insert(wbRfAfter.end(), fenceRfs.begin(), fenceRfs.end());
			}
		}

		/* Then, add the proper edges to PSC using wb-after and (wb;rf)-after successors */
		addRbEdges(fcs, wbAfter, wbRfAfter, pscMatrix, stores[i]);
		addMoRfEdges(fcs, wbAfter, wbRfAfter, pscMatrix, stores[i]);
	}
}

void ExecutionGraph::addSbHbEdges(Matrix2D<Event> &matrix)
{
	auto &scs = matrix.getElems();
	for (auto i = 0u; i < scs.size(); i++) {
		for (auto j = 0u; j < scs.size(); j++) {
			if (i == j)
				continue;
			EventLabel &ei = getEventLabel(scs[i]);
			EventLabel &ej = getEventLabel(scs[j]);

			/* PSC_base: Adds sb-edges*/
			if (ei.getThread() == ej.getThread()) {
				if (ei.getIndex() < ej.getIndex())
					matrix(i, j) = true;
				continue;
			}

			/* PSC_base: Adds sb_(<>loc);hb;sb_(<>loc) edges
			 * HACK: Also works for PSC_fence: [Fsc];hb;[Fsc] edges
			 * since fences have a null address, different from the
			 * address of all global variables */
			Event prev = ej.getPos().prev();
			EventLabel &ejPrev = getEventLabel(prev);
			if (!ejPrev.hbView.empty() && ej.getAddr() != ejPrev.getAddr() &&
			    ei.getIndex() < ejPrev.hbView[ei.getThread()]) {
				Event next = ei.getPos().next();
				EventLabel &eiNext = getEventLabel(next);
				if (ei.getAddr() != eiNext.getAddr())
					matrix(i, j) = true;
			}
		}
	}
	return;
}

void ExecutionGraph::addInitEdges(std::vector<Event> &fcs, Matrix2D<Event> &matrix)
{
	for (auto i = 0u; i < events.size(); i++) {
		for (auto j = 0u; j < events[i].size(); j++) {
			EventLabel &lab = getEventLabel(Event(i, j));
			/* Consider only reads that read from the initializer write */
			if (!lab.isRead() || !lab.rf.isInitializer() || isRMWLoad(lab.getPos()))
				continue;
			auto preds = calcSCPreds(fcs, lab.pos);
			auto fencePreds = calcSCFencesPreds(fcs, lab.getPos());
			for (auto &w : modOrder[lab.getAddr()]) {
				auto wSuccs = calcSCSuccs(fcs, w);
				matrix.addEdgesFromTo(preds, wSuccs); /* Adds rb-edges */
				for (auto &r : getEventLabel(w).rfm1) {
					auto fenceSuccs = calcSCFencesSuccs(fcs, r);
					matrix.addEdgesFromTo(fencePreds, fenceSuccs); /*Adds (rb;rf)-edges */
				}
			}
		}
	}
	return;
}

bool ExecutionGraph::isPscWeakAcyclicWB()
{
	/* Collect all SC events (except for RMW loads) */
	auto accesses = getSCs();
	auto &scs = accesses.first;
	auto &fcs = accesses.second;

	/* If there are no SC events, it is a valid execution */
	if (scs.empty())
		return true;

	Matrix2D<Event> matrix(scs);

	/* Add edges from the initializer write (special case) */
	addInitEdges(fcs, matrix);
	/* Add sb and sb_(<>loc);hb;sb_(<>loc) edges (+ Fsc;hb;Fsc) */
	addSbHbEdges(matrix);

	/*
	 * Collect memory locations with more than one SC accesses
	 * and add the rest of PSC_base and PSC_fence for only
	 * _one_ possible extension of WB for each location
	 */
	std::vector<const llvm::GenericValue *> scLocs = getDoubleLocs();
	for (auto loc : scLocs) {
		auto wb = calcWb(loc);
		auto sortedStores = wb.topoSort();
		addSCEcos(fcs, sortedStores, matrix);
	}

	matrix.transClosure();
	return !matrix.isReflexive();
}

bool ExecutionGraph::isPscWbAcyclicWB()
{
	/* Collect all SC events (except for RMW loads) */
	auto accesses = getSCs();
	auto &scs = accesses.first;
	auto &fcs = accesses.second;

	/* If there are no SC events, it is a valid execution */
	if (scs.empty())
		return true;

	Matrix2D<Event> matrix(scs);

	/* Add edges from the initializer write (special case) */
	addInitEdges(fcs, matrix);
	/* Add sb and sb_(<>loc);hb;sb_(<>loc) edges (+ Fsc;hb;Fsc) */
	addSbHbEdges(matrix);

	/*
	 * Collect memory locations with more than one SC accesses
	 * and add the rest of PSC_base and PSC_fence using WB
	 * instead of MO
	 */
	std::vector<const llvm::GenericValue *> scLocs = getDoubleLocs();
	for (auto loc : scLocs) {
		auto wb = calcWb(loc);
		addSCWbEcos(fcs, wb, matrix);
	}

	matrix.transClosure();
	return !matrix.isReflexive();
}

bool ExecutionGraph::isPscAcyclicWB()
{
	/* Collect all SC events (except for RMW loads) */
	auto accesses = getSCs();
	auto &scs = accesses.first;
	auto &fcs = accesses.second;

	/* If there are no SC events, it is a valid execution */
	if (scs.empty())
		return true;

	Matrix2D<Event> matrix(scs);

	/* Add edges from the initializer write (special case) */
	addInitEdges(fcs, matrix);
	/* Add sb and sb_(<>loc);hb;sb_(<>loc) edges (+ Fsc;hb;Fsc) */
	addSbHbEdges(matrix);

	/*
	 * Collect memory locations that have more than one SC
	 * memory access, and then calculate the possible extensions
	 * of writes-before (WB) for these memory locations
	 */
	std::vector<const llvm::GenericValue *> scLocs = getDoubleLocs();
	std::vector<std::vector<std::vector<Event> > > topoSorts(scLocs.size());
	for (auto i = 0u; i < scLocs.size(); i++) {
		auto wb = calcWb(scLocs[i]);
		topoSorts[i] = wb.allTopoSort();
	}

	unsigned int K = topoSorts.size();
	std::vector<unsigned int> count(K, 0);

	/*
	 * It suffices to find one combination for the WB extensions of all
	 * locations, for which PSC is acyclic. This loop is like an odometer:
	 * given an array that contains K vectors, we keep a counter for each
	 * vector, and proceed by incrementing the rightmost counter. Like in
	 * addition, if a carry is created, this is propagated to the left.
	 */
	while (count[0] < topoSorts[0].size()) {
		/* Process current combination */
		auto tentativePSC(matrix);
		for (auto i = 0u; i < K; i++)
			addSCEcos(fcs, topoSorts[i][count[i]], tentativePSC);

		tentativePSC.transClosure();
		if (!tentativePSC.isReflexive())
			return true;

		/* Find next combination */
		++count[K - 1];
		for (auto i = K - 1; (i > 0) && (count[i] == topoSorts[i].size()); --i) {
			count[i] = 0;
			++count[i - 1];
		}
	}

	/* No valid MO combination found */
	return false;
}

bool ExecutionGraph::isPscAcyclicMO()
{
	/* Collect all SC events (except for RMW loads) */
	auto accesses = getSCs();
	auto &scs = accesses.first;
	auto &fcs = accesses.second;

	/* If there are no SC events, it is a valid execution */
	if (scs.empty())
		return true;

	Matrix2D<Event> matrix(scs);

	/* Add edges from the initializer write (special case) */
	addInitEdges(fcs, matrix);
	/* Add sb and sb_(<>loc);hb;sb_(<>loc) edges (+ Fsc;hb;Fsc) */
	addSbHbEdges(matrix);

	/*
	 * Collect memory locations with more than one SC accesses
	 * and add the rest of PSC_base and PSC_fence for only
	 * _one_ possible extension of WB for each location
	 */
	std::vector<const llvm::GenericValue *> scLocs = getDoubleLocs();
	for (auto loc : scLocs) {
		auto &stores = modOrder[loc];
		addSCEcos(fcs, stores, matrix);
	}

	matrix.transClosure();
	return !matrix.isReflexive();
}

/*
 * Given a WB matrix returns a vector that, for each store in the WB
 * matrix, contains the index (in the WB matrix) of the upper and the
 * lower limit of the RMW chain that store belongs to. We use N for as
 * the index of the initializer write, where N is the number of stores
 * in the WB matrix.
 *
 * The vector is partitioned into 3 parts: | UPPER | LOWER | LOWER_I |
 *
 * The first part contains the upper limits, the second part the lower
 * limits and the last part the lower limit for the initiazizer write,
 * which is not part of the WB matrix.
 *
 * If there is an atomicity violation in the graph, the returned
 * vector is empty. (This may happen as part of some optimization,
 * e.g., in getRevisitLoads(), where the view of the resulting graph
 * is not calculated.)
 */
std::vector<unsigned int> ExecutionGraph::calcRMWLimits(const Matrix2D<Event> &wb)
{
	auto &s = wb.getElems();
	auto size = s.size();

	/* upperL is the vector to return, with size (2 * N + 1) */
	std::vector<unsigned int> upperL(2 * size + 1);
	std::vector<unsigned int>::iterator lowerL = upperL.begin() + size;

	/*
	 * First, we initialize the vector. For the upper limit of a
	 * non-RMW store we set the store itself, and for an RMW-store
	 * its predecessor in the RMW chain. For the lower limit of
	 * any store we set the store itself.  For the initializer
	 * write, we use "size" as its index, since it does not exist
	 * in the WB matrix.
	 */
	for (auto i = 0u; i < size; i++) {
		auto &lab = getEventLabel(s[i]);
		if (lab.isRMW()) {
			auto prev = getPreviousLabel(s[i]).rf;
			upperL[i] = (prev == Event::getInitializer()) ? size : wb.getIndex(prev);
		} else {
			upperL[i] = i;
		}
		lowerL[i] = i;
	}
	lowerL[size] = size;

	/*
	 * Next, we set the lower limit of the upper limit of an RMW
	 * store to be the store itself.
	 */
	for (auto i = 0u; i < size; i++) {
		auto ui = upperL[i];
		if (ui == i)
			continue;
		if (lowerL[ui] != ui) {
			/* If the lower limit of this upper limit has already
			 * been set, we have two RMWs reading from the same write */
			upperL.clear();
			return upperL;
		}
		lowerL[upperL[i]] = i;
	}

	/*
	 * Calculate the actual upper limit, by taking the
	 * predecessor's predecessor as an upper limit, until the end
	 * of the chain is reached.
	 */
        bool changed;
	do {
		changed = false;
		for (auto i = 0u; i < size; i++) {
			auto j = upperL[i];
			if (j == size || j == i)
				continue;

			auto k = upperL[j];
			if (j == k)
				continue;
			upperL[i] = k;
			changed = true;
		}
	} while (changed);

	/* Similarly for the lower limits */
	do {
		changed = false;
		for (auto i = 0u; i <= size; i++) {
			auto j = lowerL[i];
			if (j == i)
				continue;
			auto k = lowerL[j];
			if (j == k)
				continue;
			lowerL[i] = k;
			changed = true;
		}
	} while (changed);
	return upperL;
}

Matrix2D<Event> ExecutionGraph::calcWbRestricted(const llvm::GenericValue *addr, const View &v)
{
	std::vector<Event> storesInView;

	std::copy_if(modOrder[addr].begin(), modOrder[addr].end(), std::back_inserter(storesInView),
		     [&](Event &s){ return v.contains(s); });

	Matrix2D<Event> matrix(std::move(storesInView));
	auto &stores = matrix.getElems();

	/* Optimization */
	if (stores.size() <= 1)
		return matrix;

	auto upperLimit = calcRMWLimits(matrix);
	if (upperLimit.empty()) {
		for (auto i = 0u; i < stores.size(); i++)
			matrix(i,i) = true;
		return matrix;
	}

	auto lowerLimit = upperLimit.begin() + stores.size();

	for (auto i = 0u; i < stores.size(); i++) {
		auto &lab = getEventLabel(stores[i]);

		std::vector<Event> es;
		std::copy_if(lab.rfm1.begin(), lab.rfm1.end(), std::back_inserter(es),
			     [&v](Event &r){ return v.contains(r); });

		auto before = getHbBefore(es).
			update(getPreviousLabel(stores[i]).getHbView());
		auto upi = upperLimit[i];
		for (auto j = 0u; j < stores.size(); j++) {
			if (i == j || !isWriteRfBefore(before, stores[j]))
				continue;
			matrix(j, i) = true;

			if (upi == stores.size() || upi == upperLimit[j])
				continue;
			matrix(lowerLimit[j], upi) = true;
		}

		if (lowerLimit[stores.size()] == stores.size() || upi == stores.size())
			continue;
		matrix(lowerLimit[stores.size()], i) = true;
	}
	matrix.transClosure();
	return matrix;
}

Matrix2D<Event> ExecutionGraph::calcWb(const llvm::GenericValue *addr)
{
	Matrix2D<Event> matrix(modOrder[addr]);
	auto &stores = matrix.getElems();

	/* Optimization */
	if (stores.size() <= 1)
		return matrix;

	auto upperLimit = calcRMWLimits(matrix);
	if (upperLimit.empty()) {
		for (auto i = 0u; i < stores.size(); i++)
			matrix(i,i) = true;
		return matrix;
	}

	auto lowerLimit = upperLimit.begin() + stores.size();

	for (auto i = 0u; i < stores.size(); i++) {
		auto &lab = getEventLabel(stores[i]);
		auto before = getHbBefore(lab.rfm1).
			update(getPreviousLabel(stores[i]).getHbView());
		auto upi = upperLimit[i];
		for (auto j = 0u; j < stores.size(); j++) {
			if (i == j || !isWriteRfBefore(before, stores[j]))
				continue;
			matrix(j, i) = true;
			if (upi == stores.size() || upi == upperLimit[j])
				continue;
			matrix(lowerLimit[j], upi) = true;
		}

		if (lowerLimit[stores.size()] == stores.size() || upi == stores.size())
			continue;
		matrix(lowerLimit[stores.size()], i) = true;
	}
	matrix.transClosure();
	return matrix;
}

bool ExecutionGraph::isWbAcyclic(void)
{
	for (auto it = modOrder.begin(); it != modOrder.end(); ++it) {
		auto wb = calcWb(it->first);
		if (wb.isReflexive())
			return false;
	}
	return true;
}


/************************************************************
 ** Library consistency checking methods
 ***********************************************************/

std::vector<Event> ExecutionGraph::getLibEventsInView(Library &lib, const View &v)
{
	std::vector<Event> result;

	for (auto i = 0u; i < events.size(); i++) {
		for (auto j = 1; j <= v[i]; j++) { /* Do not consider thread inits */
			EventLabel &lab = getEventLabel(Event(i, j));
			if (lib.hasMember(lab.functionName))
				result.push_back(lab.getPos());
		}
	}
	return result;
}

void ExecutionGraph::getPoEdgePairs(std::vector<std::pair<Event, std::vector<Event> > > &froms,
				    std::vector<Event> &tos)
{
	std::vector<Event> buf;

	for (auto i = 0u; i < froms.size(); i++) {
		buf = {};
		for (auto j = 0u; j < froms[i].second.size(); j++) {
			for (auto &e : tos)
				if (froms[i].second[j].thread == e.thread &&
				    froms[i].second[j].index < e.index)
					buf.push_back(e);
		}
		froms[i].second = buf;
	}
}

void ExecutionGraph::getRfm1EdgePairs(std::vector<std::pair<Event, std::vector<Event> > > &froms,
				      std::vector<Event> &tos)
{
	std::vector<Event> buf;

	for (auto i = 0u; i < froms.size(); i++) {
		buf = {};
		for (auto j = 0u; j < froms[i].second.size(); j++) {
			EventLabel &lab = getEventLabel(froms[i].second[j]);
			if (lab.isRead() && std::find(tos.begin(), tos.end(), lab.rf) != tos.end())
				buf.push_back(lab.rf);
		}
		froms[i].second = buf;
	}
}

void ExecutionGraph::getRfEdgePairs(std::vector<std::pair<Event, std::vector<Event> > > &froms,
				    std::vector<Event> &tos)
{
	std::vector<Event> buf;

	for (auto i = 0u; i < froms.size(); i++) {
		buf = {};
		for (auto j = 0u; j < froms[i].second.size(); j++) {
			EventLabel &lab = getEventLabel(froms[i].second[j]);
			if (!lab.isWrite())
				continue;
			std::copy_if(lab.rfm1.begin(), lab.rfm1.end(), std::back_inserter(buf),
				     [&tos](Event &e){ return std::find(tos.begin(), tos.end(), e) !=
						     tos.end(); });

		}
		froms[i].second = buf;
	}
}

void ExecutionGraph::getHbEdgePairs(std::vector<std::pair<Event, std::vector<Event> > > &froms,
				    std::vector<Event> &tos)
{
	std::vector<Event> buf;

	for (auto i = 0u; i < froms.size(); i++) {
		buf = {};
		for (auto j = 0u; j < froms[i].second.size(); j++) {
			for (auto &e : tos) {
				auto &before = getHbBefore(e);
				if (froms[i].second[j].index <
				    before[froms[i].second[j].thread])
					buf.push_back(e);
			}
		}
		froms[i].second = buf;
	}
}

void ExecutionGraph::getWbEdgePairs(std::vector<std::pair<Event, std::vector<Event> > > &froms,
				    std::vector<Event> &tos)
{
	std::vector<Event> buf;

	for (auto i = 0u; i < froms.size(); i++) {
		buf = {};
		for (auto j = 0u; j < froms[i].second.size(); j++) {
			EventLabel &lab = getEventLabel(froms[i].second[j]);
			if (!lab.isWrite())
				continue;

			View v(getHbBefore(tos[0]));
			for (auto &t : tos)
				v.update(getEventLabel(t).getHbView());

			auto wb = calcWbRestricted(lab.getAddr(), v);
			auto &ss = wb.getElems();
			// TODO: Make a map with already calculated WBs??

			if (std::find(ss.begin(), ss.end(), lab.getPos()) == ss.end())
				continue;

			/* Collect all wb-after stores that are in "tos" range */
			auto k = wb.getIndex(lab.getPos());
			for (auto l = 0u; l < ss.size(); l++)
				if (wb(k, l))
					buf.push_back(ss[l]);
		}
		froms[i].second = buf;
	}
}

void ExecutionGraph::getMoEdgePairs(std::vector<std::pair<Event, std::vector<Event> > > &froms,
				    std::vector<Event> &tos)
{
	std::vector<Event> buf;

	for (auto i = 0u; i < froms.size(); i++) {
		buf = {};
		for (auto j = 0u; j < froms[i].second.size(); j++) {
			EventLabel &lab = getEventLabel(froms[i].second[j]);
			if (!lab.isWrite())
				continue;

			/* Collect all mo-after events that are in the "tos" range */
			auto moAfter = modOrder.getMoAfter(lab.getAddr(), lab.getPos());
			for (auto &s : moAfter)
				if (std::find(tos.begin(), tos.end(), s) != tos.end())
					buf.push_back(s);
		}
		froms[i].second = buf;
	}
}

void ExecutionGraph::calcSingleStepPairs(std::vector<std::pair<Event, std::vector<Event> > > &froms,
					 std::string &step, std::vector<Event> &tos)
{
	if (step == "po")
		return getPoEdgePairs(froms, tos);
	else if (step == "rf")
		return getRfEdgePairs(froms, tos);
	else if (step == "hb")
		return getHbEdgePairs(froms, tos);
	else if (step == "rf-1")
		return getRfm1EdgePairs(froms, tos);
	else if (step == "wb")
		return getWbEdgePairs(froms, tos);
	else if (step == "mo")
		return getMoEdgePairs(froms, tos);
	else
		BUG();
}

void addEdgePairsToMatrix(std::vector<std::pair<Event, std::vector<Event> > > &pairs,
			  Matrix2D<Event> &matrix)
{
	for (auto &p : pairs) {
		for (auto k = 0u; k < p.second.size(); k++) {
			matrix(p.first, p.second[k]) = true;
		}
	}
}

void ExecutionGraph::addStepEdgeToMatrix(std::vector<Event> &es,
					 Matrix2D<Event> &relMatrix,
					 std::vector<std::string> &substeps)
{
	std::vector<std::pair<Event, std::vector<Event> > > edges;

	/* Initialize edges */
	for (auto &e : es)
		edges.push_back(std::make_pair(e, std::vector<Event>({e})));

	for (auto i = 0u; i < substeps.size(); i++)
		calcSingleStepPairs(edges, substeps[i], es);
	addEdgePairsToMatrix(edges, relMatrix);
}

llvm::StringMap<Matrix2D<Event> >
ExecutionGraph::calculateAllRelations(Library &lib, std::vector<Event> &es)
{
	llvm::StringMap<Matrix2D<Event> > relMap;

	for (auto &r : lib.getRelations()) {
		Matrix2D<Event> relMatrix(es);
		auto &steps = r.getSteps();
		for (auto &s : steps)
			addStepEdgeToMatrix(es, relMatrix, s);

		if (r.isTransitive())
			relMatrix.transClosure();
		relMap[r.getName()] = relMatrix;
	}
	return relMap;
}

bool ExecutionGraph::isLibConsistentInView(Library &lib, const View &v)
{
	auto es = getLibEventsInView(lib, v);
	auto relations = calculateAllRelations(lib, es);
	auto &constraints = lib.getConstraints();
	if (std::all_of(constraints.begin(), constraints.end(),
			[&relations, &es](Constraint &c)
			{ return !relations[c.getName()].isReflexive(); }))
		return true;
	return false;
}

std::vector<Event>
ExecutionGraph::getLibConsRfsInView(Library &lib, EventLabel &rLab,
				    const std::vector<Event> &stores, const View &v)
{
	auto oldRf = rLab.rf;
	std::vector<Event> filtered;

	for (auto &s : stores) {
		changeRf(rLab, s);
		if (isLibConsistentInView(lib, v))
			filtered.push_back(s);
	}
	/* Restore the old reads-from, and eturn all the valid reads-from options */
	changeRf(rLab, oldRf);
	return filtered;
}


/************************************************************
 ** Debugging methods
 ***********************************************************/

void ExecutionGraph::validate(void)
{
	for (auto i = 0u; i < events.size(); i++) {
		for (auto j = 0u; j < events[i].size(); j++) {
			EventLabel &lab = getEventLabel(Event(i, j));
			if (lab.isRead()) {
				if (lab.rf.isInitializer())
					continue;
				bool readExists = false;
				for (auto &r : getEventLabel(lab.rf).rfm1)
					if (r == lab.getPos())
						readExists = true;
				if (!readExists) {
					WARN("Read event is not the appropriate rf-1 list!\n");
					llvm::dbgs() << lab.getPos() << "\n";
					llvm::dbgs() << *this << "\n";
					abort();
				}
			} else if (lab.isWrite()) {
				if (lab.isRMW() && std::count_if(lab.rfm1.begin(), lab.rfm1.end(),
						  [&](Event &load){ return isRMWLoad(load); }) > 1) {
					WARN("RMW store is read from more than 1 load!\n");
					llvm::dbgs() << "RMW store: " << lab.getPos() << "\nReads:";
					for (auto &r : lab.rfm1)
						llvm::dbgs() << r << " ";
					llvm::dbgs() << "\n";
					llvm::dbgs() << *this << "\n";
					abort();
				}
				if (lab.rfm1.empty())
					continue;
				bool writeExists = false;
				for (auto &e : lab.rfm1)
					if (getEventLabel(e).rf == lab.getPos())
						writeExists = true;
				if (!writeExists) {
					WARN("Write event is not marked in the read event!\n");
					llvm::dbgs() << lab.getPos() << "\n";
					llvm::dbgs() << *this << "\n";
					abort();
				}
				for (auto &r : lab.rfm1) {
					if (r.thread > (int) events.size() ||
					    r.index >= (int) events[r.thread].size()) {
						WARN("Event in write's rf-list does not exist!\n");
						llvm::dbgs() << r << "\n";
						llvm::dbgs() << *this << "\n";
						abort();
					}
				}

			}
		}
	}
	return;
}


/************************************************************
 ** Overloaded operators
 ***********************************************************/

llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const ExecutionGraph &g)
{
	for (auto i = 0u; i < g.events.size(); i++) {
		for (auto j = 0u; j < g.events[i].size(); j++) {
			auto &lab = *g.events[i][j];
			s << "\t" << lab << "\n";
			if (lab.isRead())
				s << "\t\treads from: " << lab.rf << "\n";
			else if (lab.isStart())
				s << "\t\tforked from: " << lab.rf << "\n";
			else if (lab.isJoin())
				s << "\t\tawaits for: " << lab.cid << ", (rf: "
				  << lab.rf << ")\n";
		}
	}
	s << "Thread sizes:\n\t";
	for (auto i = 0u; i < g.events.size(); i++)
		s << g.events[i].size() << " ";
	s << "\n";
	return s;
}
