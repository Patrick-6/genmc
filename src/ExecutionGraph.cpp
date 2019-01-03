/*
 * RCMC -- Model Checking for C11 programs.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * Author: Michalis Kokologiannakis <mixaskok@gmail.com>
 */

#include "Library.hpp"
#include "Parser.hpp"
#include "Thread.hpp"
#include "ExecutionGraph.hpp"
#include <llvm/ADT/StringMap.h>
#include <llvm/IR/DebugInfo.h>

/************************************************************
 ** Class Constructors
 ***********************************************************/

ExecutionGraph::ExecutionGraph(llvm::Interpreter *EE)
	: EE(EE), currentT(0), timestamp(0) {}


/************************************************************
 ** Basic getter methods
 ***********************************************************/

unsigned int ExecutionGraph::nextStamp()
{
	return timestamp++;
}

EventLabel& ExecutionGraph::getEventLabel(const Event &e)
{
	return threads[e.thread].eventList[e.index];
}

EventLabel& ExecutionGraph::getPreviousLabel(const Event &e)
{
	return threads[e.thread].eventList[e.index-1];
}

EventLabel& ExecutionGraph::getLastThreadLabel(int thread)
{
	return threads[thread].eventList[maxEvents[thread] - 1];
}

Event ExecutionGraph::getLastThreadEvent(int thread)
{
	return threads[thread].eventList[maxEvents[thread] - 1].pos;
}

Event ExecutionGraph::getLastThreadRelease(int thread, llvm::GenericValue *addr)
{
	for (int i = maxEvents[thread] - 1; i > 0; i--) {
		EventLabel &lab = threads[thread].eventList[i];
		if (lab.isFence() && lab.isAtLeastRelease())
			return lab.pos;
		if (lab.isWrite() && lab.isAtLeastRelease() && lab.addr == addr)
			return lab.pos;
	}
	return threads[thread].eventList[0].pos;
}

View ExecutionGraph::getGraphState(void)
{
	View state;

	for (auto i = 0u; i < threads.size(); i++)
		state[i] = maxEvents[i] - 1;
	return state;
}

std::vector<llvm::ExecutionContext>& ExecutionGraph::getECStack(int thread)
{
	return threads[thread].ECStack;
}

bool ExecutionGraph::isThreadComplete(int thread)
{
	return threads[thread].eventList[maxEvents[thread] - 1].isFinish();
}

/*
 * Given an write label sLab that is part of an RMW, return all
 * other RMWs that read from the same write. Of course, there must
 * be _at most_ one other RMW reading from the same write (see [Rex] set)
 */
std::vector<Event> ExecutionGraph::getPendingRMWs(EventLabel &sLab)
{
	std::vector<Event> pending;

	/* This function should be called with a write event */
	BUG_ON(!sLab.isWrite());

	/* If this is _not_ an RMW event, return an empty vector */
	if (!sLab.isRMW())
		return pending;

	/* Otherwise, scan for other RMWs that successfully read the same val */
	auto &pLab = getPreviousLabel(sLab.pos);
	auto wVal = EE->loadValueFromWrite(pLab.rf, pLab.valTyp, pLab.addr);
	for (auto i = 0u; i < threads.size(); i++) {
		Thread &thr = threads[i];
		for (auto j = 1; j < maxEvents[i]; j++) { /* Skip thread start */
			EventLabel &lab = thr.eventList[j];
			if (lab.isRead() && lab.isRMW() && lab.addr == pLab.addr &&
			    lab.pos != pLab.pos && lab.rf == pLab.rf &&
			    EE->isSuccessfulRMW(lab, wVal))
				pending.push_back(lab.pos);
		}
	}
	BUG_ON(pending.size() > 1);
	return pending;
}

Event ExecutionGraph::getPendingLibRead(EventLabel &lab)
{
	/* Should only be called with a read of a functional library that doesn't read BOT */
	BUG_ON(!lab.isRead() || lab.rf == Event::getInitializer());

	/* Get the conflicting label */
	auto &sLab = getEventLabel(lab.rf);
	auto it = std::find_if(sLab.rfm1.begin(), sLab.rfm1.end(), [&lab](Event &e){ return e != lab.pos; });
	BUG_ON(it == sLab.rfm1.end());
	return *it;
}

std::vector<Event> ExecutionGraph::getRevisitable(const EventLabel &sLab)
{
	std::vector<Event> loads;

	BUG_ON(!sLab.isWrite());
	auto before = getPorfBefore(sLab.pos);
	for (auto i = 0u; i < threads.size(); i++) {
		for (auto j = before[i] + 1; j < maxEvents[i]; j++) {
			auto &lab = threads[i].eventList[j];
			if (lab.isRead() && lab.addr == sLab.addr && lab.isRevisitable() &&
			    (!lab.hasBeenRevisited() || lab.rf.index <= sLab.porfView[lab.rf.thread]))
				loads.push_back(lab.pos);
		}
	}
	return loads;
}

std::vector<Event> ExecutionGraph::getRMWChainDownTo(const EventLabel &sLab, const Event lower)
{
	std::vector<Event> chain;

	/*
	 * This function should not be called with an event that is not
	 * the store part of an RMW
	 */
	WARN_ON_ONCE(!sLab.isWrite(), "getrmwchaindownto-arg",
		     "WARNING: getRMWChainDownTo() called with non-write argument!\n");
	if (!sLab.isWrite() || !sLab.isRMW())
		return chain;

	/* As long as you find successful RMWs, keep going down the chain */
	auto curr = &sLab;
	while (curr->isWrite() && curr->isRMW() && curr->pos != lower) {
		chain.push_back(curr->pos);
		auto &rLab = getPreviousLabel(curr->pos);

		/*
		 * If the predecessor reads the initializer event, then the
		 * chain is over. This may be equal to lower in some cases,
		 * but we avoid getting the label of the initializer event.
		 */
		if (rLab.rf.isInitializer())
			return chain;

		/* Otherwise, go down one (rmw-1;rf-1)-step */
		curr = &getEventLabel(rLab.rf);
	}

	/*
	 * We arrived at a non-RMW event (which is not the initializer),
	 * so the chain is over
	 */
	return chain;
}

std::vector<Event> ExecutionGraph::getRMWChainUpTo(const EventLabel &sLab, const Event upper)
{
	std::vector<Event> chain;

	/*
	 * This function should not be called with an event that is not
	 * the store part of an RMW
	 */
	WARN_ON_ONCE(!sLab.isWrite(), "getrmwchainupto-arg",
		     "WARNING: getRMWChainUpTo() called with non-write argument!\n");
	if (!sLab.isWrite() || !sLab.isRMW())
		return chain;

	/* As long as you find successful RMWs, keep going down the chain */
	auto curr = &sLab;
	while (curr->isWrite() && curr->isRMW() && curr->pos != upper) {
		/* Push this event into the chain */
		chain.push_back(curr->pos);

		/* Check if other successful RMWs are reading from this write (at most one) */
		std::vector<Event> rmwRfs;
		std::copy_if(curr->rfm1.begin(), curr->rfm1.end(), std::back_inserter(rmwRfs),
			     [this, curr](const Event &r){ auto &rLab = getEventLabel(r);
					             return EE->isSuccessfulRMW(rLab, curr->val); });
		BUG_ON(rmwRfs.size() > 1);

		/* If there is none, we reached the upper limit of the chain */
		if (rmwRfs.size() == 0)
			return chain;

		/* Otherwise, go up one (rf;rmw)-step */
		curr = &getEventLabel(rmwRfs.back().next());
	}

	/* This is a non-RMW event (which is not equal to upper), so the chain is over */
	return chain;
}

std::vector<Event> ExecutionGraph::getStoresHbAfterStores(llvm::GenericValue *loc,
							  const std::vector<Event> &chain)
{
	auto &stores = modOrder[loc];
	std::vector<Event> result;

	for (auto &s : stores) {
		if (std::find(chain.begin(), chain.end(), s) != chain.end())
			continue;
		auto before = getHbBefore(s);
		if (std::any_of(chain.begin(), chain.end(), [&before](Event e)
				{ return e.index < before[e.thread]; }))
			result.push_back(s);
	}
	return result;
}

std::vector<Event> ExecutionGraph::getMoOptRfAfter(Event store)
{
	auto ls = modOrder.getMoAfter(getEventLabel(store).addr, store);
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

void ExecutionGraph::calcLoadPoRfView(EventLabel &lab, Event prev, Event &rf)
{
	lab.porfView = getPorfBefore(prev);
	++lab.porfView[lab.pos.thread];

	View mV = getPorfBefore(lab.rf);
	lab.porfView.updateMax(mV);
}

void ExecutionGraph::calcLoadHbView(EventLabel &lab, Event prev, Event &rf)
{
	lab.hbView = getHbBefore(prev);
	lab.hbView[prev.thread] = prev.index + 1;
	if (lab.isAtLeastAcquire()) {
		auto mV = getMsgView(lab.rf);
		lab.hbView.updateMax(mV);
	}
}

void ExecutionGraph::addEventToGraph(EventLabel &lab)
{
	lab.stamp = nextStamp();
	Thread &thr = threads[currentT];
	thr.eventList.push_back(lab);
	++maxEvents[currentT];
}

void ExecutionGraph::addReadToGraphCommon(EventLabel &lab, Event &rf)
{
	lab.makeRevisitable();
	calcLoadHbView(lab, getLastThreadEvent(currentT), rf);
	calcLoadPoRfView(lab, getLastThreadEvent(currentT), rf);

	addEventToGraph(lab);

	if (rf.isInitializer())
		return;
	Thread &rfThr = threads[rf.thread];
	EventLabel &rfLab = rfThr.eventList[rf.index];
	rfLab.rfm1.push_front(lab.pos);
	return;
}

void ExecutionGraph::addReadToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
				    llvm::Type *typ, Event rf, EventAttr attr,
				    llvm::GenericValue &&cmpVal = llvm::GenericValue(),
				    llvm::GenericValue &&rmwVal = llvm::GenericValue(),
				    llvm::AtomicRMWInst::BinOp op = llvm::AtomicRMWInst::BinOp::BAD_BINOP)
{
	int max = maxEvents[currentT];
	EventLabel lab(ERead, attr, ord, Event(currentT, max), ptr, cmpVal, rmwVal, op, typ, rf);
	addReadToGraphCommon(lab, rf);
}

void ExecutionGraph::addLibReadToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
				       llvm::Type *typ, Event rf, std::string functionName)
{
	int max = maxEvents[currentT];
	EventLabel lab(ERead, Plain, ord, Event(currentT, max), ptr, typ, rf, functionName);
	addReadToGraphCommon(lab, rf);
}

void ExecutionGraph::addStoreToGraphCommon(EventLabel &lab)
{
	lab.hbView = getHbBefore(getLastThreadEvent(currentT));
	lab.hbView[currentT] = maxEvents[currentT];
	lab.porfView = getPorfBefore(getLastThreadEvent(currentT));
	lab.porfView[currentT] = maxEvents[currentT];
	if (lab.isRMW()) {
		Event last = getLastThreadEvent(currentT);
		EventLabel &pLab = getEventLabel(last);
		View mV = getMsgView(pLab.rf);
		BUG_ON(pLab.ord == llvm::NotAtomic);
		if (pLab.isAtLeastRelease())
			/* no need for ctor -- getMax copies the View */
			lab.msgView = mV.getMax(lab.hbView);
		else
			lab.msgView = getHbBefore(
				getLastThreadRelease(currentT, lab.addr)).getMax(mV);
	} else {
		if (lab.isAtLeastRelease())
			lab.msgView = lab.hbView;
		else if (lab.ord == llvm::Monotonic || lab.ord == llvm::Acquire)
			lab.msgView = getHbBefore(getLastThreadRelease(currentT, lab.addr));
	}
	addEventToGraph(lab);
	return;
}

void ExecutionGraph::addStoreToGraph(llvm::Type *typ, llvm::GenericValue *ptr,
				     llvm::GenericValue &val, int offsetMO,
				     EventAttr attr, llvm::AtomicOrdering ord)
{
	int max = maxEvents[currentT];
	EventLabel lab(EWrite, attr, ord, Event(currentT, max), ptr, val, typ);
	addStoreToGraphCommon(lab);
	modOrder[lab.addr].insert(modOrder[lab.addr].begin() + offsetMO, lab.pos);
}

void ExecutionGraph::addLibStoreToGraph(llvm::Type *typ, llvm::GenericValue *ptr,
					llvm::GenericValue &val, int offsetMO,
					EventAttr attr, llvm::AtomicOrdering ord,
					std::string functionName, bool isInit)
{
	int max = maxEvents[currentT];
	EventLabel lab(EWrite, Plain, ord, Event(currentT, max), ptr, val, typ, functionName, isInit);
	addStoreToGraphCommon(lab);
	modOrder[lab.addr].insert(modOrder[lab.addr].begin() + offsetMO, lab.pos);
}

void ExecutionGraph::addFenceToGraph(llvm::AtomicOrdering ord)

{
	int max = maxEvents[currentT];
	EventLabel lab(EFence, ord, Event(currentT, max));

	lab.hbView = getHbBefore(getLastThreadEvent(currentT));
	lab.hbView[currentT] = maxEvents[currentT];
	lab.porfView = getPorfBefore(getLastThreadEvent(currentT));
	lab.porfView[currentT] = maxEvents[currentT];
	if (lab.isAtLeastAcquire())
		calcRelRfPoBefore(currentT, getLastThreadEvent(currentT).index, lab.hbView);
	addEventToGraph(lab);
}

void ExecutionGraph::addTCreateToGraph(int cid)
{
	int max = maxEvents[currentT];
	EventLabel lab(ETCreate, llvm::Release, Event(currentT, max), cid);

	lab.porfView = getPorfBefore(getLastThreadEvent(currentT));
	lab.porfView[currentT] = maxEvents[currentT];

	/* Thread creation has Release semantics */
	lab.hbView = getHbBefore(getLastThreadEvent(currentT));
	lab.hbView[currentT] = maxEvents[currentT];
	lab.msgView = lab.hbView;
	addEventToGraph(lab);
}

void ExecutionGraph::addTJoinToGraph(int cid)
{
	int max = maxEvents[currentT];
	EventLabel lab(ETJoin, llvm::Acquire, Event(currentT, max), cid);

	lab.porfView = getPorfBefore(getLastThreadEvent(currentT));
	lab.porfView[currentT] = max;

	/* Thread joins have acquire semantics -- but we have to wait
	 * for the other thread to finish first, so we do not fully
	 * update the view yet */
	lab.hbView = getHbBefore(getLastThreadEvent(currentT));
	lab.hbView[currentT] = max;
	addEventToGraph(lab);
}

void ExecutionGraph::addStartToGraph(int tid, Event tc)
{
	int max = maxEvents[tid];
	EventLabel lab = EventLabel(EStart, llvm::Acquire, Event(tid, max), tc);

	lab.porfView = getPorfBefore(tc);
	lab.porfView[tid] = max;

	/* Thread start has Acquire semantics */
	lab.hbView = getHbBefore(tc);
	lab.hbView[tid] = max;
	threads[tid].eventList.push_back(lab);
	++maxEvents[tid];

	EventLabel &tcLab = getEventLabel(tc);
	tcLab.rfm1.push_back(lab.pos);
}

void ExecutionGraph::addFinishToGraph()
{
	int max = maxEvents[currentT];
	EventLabel lab(EFinish, llvm::Release, Event(currentT, max));

	lab.porfView = getPorfBefore(getLastThreadEvent(currentT));
	lab.porfView[currentT] = max;

	/* Thread termination has Release semantics */
	lab.hbView = getHbBefore(getLastThreadEvent(currentT));
	lab.hbView[currentT] = max;
	lab.msgView = lab.hbView;
	addEventToGraph(lab);
}

/************************************************************
 ** Calculation of [(po U rf)*] predecessors and successors
 ***********************************************************/

View ExecutionGraph::getMsgView(Event e)
{
	return getEventLabel(e).getMsgView();
}

View ExecutionGraph::getPorfBefore(Event e)
{
	return getEventLabel(e).getPorfView();
}
View ExecutionGraph::getHbBefore(Event e)
{
	return getEventLabel(e).getHbView();
}

View ExecutionGraph::getHbBefore(const std::vector<Event> &es)
{
	View v;

	for (auto &e : es) {
		auto o = getHbBefore(e);
		v.updateMax(o);
	}
	return v;
}

View ExecutionGraph::getHbPoBefore(Event e)
{
	return getHbBefore(e.prev());
}

void ExecutionGraph::calcHbRfBefore(Event &e, llvm::GenericValue *addr,
				    View &a)
{
	int ai = a[e.thread];
	if (e.index <= ai)
		return;

	a[e.thread] = e.index;
	Thread &thr = threads[e.thread];
	for (int i = ai + 1; i <= e.index; i++) {
		EventLabel &lab = thr.eventList[i];
		if (lab.isRead() && !lab.rf.isInitializer() &&
		    (lab.addr == addr || lab.rf.index <= lab.hbView[lab.rf.thread]))
			calcHbRfBefore(lab.rf, addr, a);
	}
	return;
}

View ExecutionGraph::getHbRfBefore(std::vector<Event> &es)
{
	View a;

	for (auto &e : es)
		calcHbRfBefore(e, getEventLabel(e).addr, a);
	return a;
}

void ExecutionGraph::calcRelRfPoBefore(int thread, int index, View &v)
{
	for (auto i = index; i > 0; i--) {
		EventLabel &lab = threads[thread].eventList[i];
		if (lab.isFence() && lab.isAtLeastAcquire())
			return;
		if (lab.isRead() && (lab.ord == llvm::Monotonic || lab.ord == llvm::Release)) {
			View o = getMsgView(lab.rf);
			v.updateMax(o);
		}
	}
}


/************************************************************
 ** Calculation of particular sets of events/event labels
 ***********************************************************/

std::vector<EventLabel>
ExecutionGraph::getPrefixLabelsNotBefore(View &prefix, View &before)
{
	std::vector<EventLabel> result;

	for (auto i = 0u; i < threads.size(); i++) {
		for (auto j = before[i] + 1; j <= prefix[i]; j++) {
			EventLabel &lab = threads[i].eventList[j];
			result.push_back(lab);
			EventLabel &curLab = result.back();
			if (!curLab.hasWriteSem())
				continue;
			curLab.rfm1.remove_if([&](Event &e)
					      { return e.index > prefix[e.thread] &&
						       e.index > before[e.thread]; });
		}
	}
	return result;
}

std::vector<Event>
ExecutionGraph::getRfsNotBefore(const std::vector<EventLabel> &labs,
				View &before)
{
	std::vector<Event> rfs;

	std::for_each(labs.begin(), labs.end(), [&rfs, &before](const EventLabel &lab)
		      { if (lab.isRead() && lab.pos.index > before[lab.pos.thread])
				      rfs.push_back(lab.rf); });
	return rfs;
}

std::vector<std::pair<Event, Event> >
ExecutionGraph::getMOPredsInBefore(const std::vector<EventLabel> &labs,
				   View &before)
{
	std::vector<std::pair<Event, Event> > pairs;

	for (auto &lab : labs) {
		/* Only store MO pairs for labels that are not in before */
		if (!lab.isWrite() || lab.pos.index <= before[lab.pos.thread])
			continue;

		auto &locMO = modOrder[lab.addr];
		auto moPos = std::find(locMO.begin(), locMO.end(), lab.pos);

		/* This store must definitely be in this location's MO */
		BUG_ON(moPos == locMO.end());

		/* We need to find the previous MO store that is in before or
		 * in the vector for which we are getting the predecessors */
		std::reverse_iterator<std::vector<Event>::iterator> predPos(moPos);
		auto predFound = false;
		for (auto rit = predPos; rit != locMO.rend(); ++rit) {
			if (rit->index <= before[rit->thread] ||
			    std::find_if(labs.begin(), labs.end(),
					 [rit](const EventLabel &lab)
					 { return lab.pos == *rit; })
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

bool ExecutionGraph::isWriteRfBefore(View &before, Event e)
{
	if (e.index <= before[e.thread])
		return true;

	auto &lab = getEventLabel(e);
	BUG_ON(!lab.isWrite());
	for (auto &e : lab.rfm1)
		if (e.index <= before[e.thread])
			return true;
	return false;
}

std::vector<Event> ExecutionGraph::findOverwrittenBoundary(llvm::GenericValue *addr, int thread)
{
	std::vector<Event> boundary;
	auto before = getHbBefore(getLastThreadEvent(thread));

	if (before.empty())
		return boundary;

	for (auto &e : modOrder[addr])
		if (isWriteRfBefore(before, e))
			boundary.push_back(e.prev());
	return boundary;
}

/* View before _can_ be implicitly modified */
std::pair<int, int>
ExecutionGraph::splitLocMOBefore(llvm::GenericValue *addr, View &before)
{
	auto &locMO = modOrder[addr];
	for (auto rit = locMO.rbegin(); rit != locMO.rend(); ++rit) {
		if (before.empty() || !isWriteRfBefore(before, *rit))
			continue;
		return std::make_pair(std::distance(rit, locMO.rend()), locMO.size());
	}
	return std::make_pair(0, locMO.size());
}


/************************************************************
 ** Graph modification methods
 ***********************************************************/

void ExecutionGraph::changeRf(EventLabel &lab, Event store)
{
	Event oldRf = lab.rf;
	lab.rf = store;

	/* Make sure that the old write it was reading from still exists */
	if (oldRf.index < maxEvents[oldRf.thread] && !oldRf.isInitializer()) {
		EventLabel &oldLab = getEventLabel(oldRf);
		oldLab.rfm1.remove(lab.pos);
	}
	if (!store.isInitializer()) {
		EventLabel &sLab = getEventLabel(store);
		sLab.rfm1.push_back(lab.pos);
	}
	/* Update the views of the load */
	calcLoadHbView(lab, lab.pos.prev(), store);
	calcLoadPoRfView(lab, lab.pos.prev(), store);
}


void ExecutionGraph::cutToEventView(Event &e, View &preds)
{
	EventLabel &lab = getEventLabel(e);
	for (auto i = 0u; i < threads.size(); i++) {
		maxEvents[i] = preds[i] + 1;
		Thread &thr = threads[i];
		thr.eventList.erase(thr.eventList.begin() + preds[i] + 1, thr.eventList.end());
		for (int j = 0; j < maxEvents[i]; j++) {
			EventLabel &lab = thr.eventList[j];
			if (!lab.hasWriteSem())
				continue;
			lab.rfm1.remove_if([&preds](Event &e)
					   { return e.index > preds[e.thread]; });
		}
	}
	for (auto it = modOrder.begin(); it != modOrder.end(); ++it)
		it->second.erase(std::remove_if(it->second.begin(), it->second.end(),
						[&preds](Event &e)
						{ return e.index > preds[e.thread]; }),
				 it->second.end());
	return;
}

View ExecutionGraph::getViewFromStamp(unsigned int stamp)
{
	View preds;

	for (auto i = 0u; i < threads.size(); i++) {
		for (auto j = maxEvents[i] - 1; j >= 0; j--) {
			auto &lab = threads[i].eventList[j];
			if (lab.getStamp() <= stamp) {
				preds[i] = j;
				break;
			}
		}
	}
	return preds;
}

void ExecutionGraph::cutToEventStamp(Event &e, unsigned int stamp)
{
	auto preds = getViewFromStamp(stamp);
	cutToEventView(e, preds);
	return;
}

void ExecutionGraph::restoreStorePrefix(EventLabel &rLab, View &storePorfBefore,
					std::vector<EventLabel> &storePrefix,
					std::vector<std::pair<Event, Event> > &moPlacings)
{
	for (auto &lab : storePrefix) {
		BUG_ON(lab.pos.index > maxEvents[lab.pos.thread] &&
		       "Events should be added in order!");
		if (lab.pos.index == maxEvents[lab.pos.thread]) {
			maxEvents[lab.pos.thread] = lab.pos.index + 1;
			threads[lab.pos.thread].eventList.push_back(lab);
		}
	}

	for (auto &lab : storePrefix) {
		EventLabel &curLab = threads[lab.pos.thread].eventList[lab.pos.index];
		curLab.makeNotRevisitable();
		if (curLab.isRead() && !curLab.rf.isInitializer()) {
			EventLabel &rfLab = getEventLabel(curLab.rf);
			if (std::find(rfLab.rfm1.begin(), rfLab.rfm1.end(), curLab.pos)
			    == rfLab.rfm1.end())
				rfLab.rfm1.push_back(curLab.pos);
		}
		curLab.rfm1.remove(rLab.pos);
	}

	/* If there are no specific mo placings, just insert all stores */
	if (moPlacings.empty()) {
		std::for_each(storePrefix.begin(), storePrefix.end(), [this](EventLabel &lab)
			      { if (lab.isWrite()) this->modOrder.addAtLocEnd(lab.addr, lab.pos); });
		return;
	}

	/* Otherwise, insert the writes of storePrefix into the appropriate places */
	int inserted = 0;
	while (inserted < moPlacings.size()) {
		for (auto it = moPlacings.begin(); it != moPlacings.end(); ++it) {
			auto &lab = getEventLabel(it->first);
			if (modOrder.locContains(lab.addr, it->second) &&
			    !modOrder.locContains(lab.addr, it->first)) {
				modOrder.addAtLocAfter(lab.addr, it->second, it->first);
				++inserted;
			}
		}
	}
}


/************************************************************
 ** Equivalence checks
 ***********************************************************/

bool ExecutionGraph::equivPrefixes(unsigned int stamp,
				   const std::vector<EventLabel> &oldPrefix,
				   const std::vector<EventLabel> &newPrefix)
{
	for (auto ritN = newPrefix.rbegin(); ritN != newPrefix.rend(); ++ritN) {
		if (std::all_of(oldPrefix.rbegin(), oldPrefix.rend(), [&](const EventLabel &sLab)
				{ return sLab != *ritN; }))
			return false;
	}

	for (auto ritO = oldPrefix.rbegin(); ritO != oldPrefix.rend(); ++ritO) {
		if (std::find(newPrefix.rbegin(), newPrefix.rend(), *ritO) != newPrefix.rend())
			continue;

		if (ritO->getStamp() <= stamp &&
		    ritO->pos.index < maxEvents[ritO->pos.thread] &&
		    *ritO == threads[ritO->pos.thread].eventList[ritO->pos.index])
			continue;
		return false;
	}
	return true;
}

bool ExecutionGraph::equivPlacings(unsigned int stamp,
				   const std::vector<std::pair<Event, Event> > &oldPlacings,
				   const std::vector<std::pair<Event, Event> > &newPlacings)
{
	for (auto ritN = newPlacings.rbegin(); ritN != newPlacings.rend(); ++ritN) {
		if (std::all_of(oldPlacings.rbegin(), oldPlacings.rend(),
				[&](const std::pair<Event, Event> &s)
				{ return s != *ritN; }))
			return false;
	}

	for (auto ritO = oldPlacings.rbegin(); ritO != oldPlacings.rend(); ++ritO) {
		if (std::find(newPlacings.rbegin(), newPlacings.rend(), *ritO) != newPlacings.rend())
			continue;

		auto &sLab1 = getEventLabel(ritO->first);
		auto &sLab2 = getEventLabel(ritO->second);
		if (sLab2.getStamp() <= stamp && modOrder.areOrdered(sLab1.addr, sLab1.pos, sLab2.pos))
			continue;
		return false;
	}
	return true;
}

/************************************************************
 ** Consistency checks
 ***********************************************************/

bool ExecutionGraph::isConsistent(void)
{
	return true;
}


/************************************************************
 ** Scheduling methods
 ***********************************************************/

void ExecutionGraph::spawnAllChildren(int thread)
{
	for (auto i = 0; i < maxEvents[thread]; i++) {
		EventLabel &lab = threads[thread].eventList[i];
		if (lab.isCreate()) {
			EventLabel &child = getEventLabel(lab.rfm1.back());
			Thread &childThr = threads[child.pos.thread];
			childThr.ECStack.push_back(childThr.initSF);
		}
	}
	return;
}

bool ExecutionGraph::scheduleNext(void)
{
	for (unsigned int i = 0; i < threads.size(); i++) {
		if (!getECStack(i).empty() && !threads[i].isBlocked) {
			if (isThreadComplete(i)) {
				spawnAllChildren(i);
				getECStack(i).clear();
				continue;
			}
			currentT = i;
			return true;
		}
	}
	return false;
}

void ExecutionGraph::tryToBacktrack(void)
{
	auto before = getPorfBefore(getLastThreadEvent(currentT));

	threads[currentT].isBlocked = true;
	getECStack(currentT).clear();
}

void ExecutionGraph::clearAllStacks(void)
{
	for (auto i = 0u; i < threads.size(); i++)
		getECStack(i).clear();
}


/************************************************************
 ** Race detection methods
 ***********************************************************/

Event ExecutionGraph::findRaceForNewLoad(llvm::AtomicOrdering ord, llvm::GenericValue *ptr)
{
	auto before = getHbBefore(getLastThreadEvent(currentT));
	auto &stores = modOrder[ptr];

	/* If there are not any events hb-before the read, there is nothing to do */
	if (before.empty())
		return Event::getInitializer();

	/* Check for events that race with the current load */
	for (auto &s : stores) {
		if (s.index > before[s.thread]) {
			EventLabel &sLab = getEventLabel(s);
			if (ord == llvm::NotAtomic || sLab.isNotAtomic())
				return s; /* Race detected! */
		}
	}
	return Event::getInitializer(); /* Race not found */
}

Event ExecutionGraph::findRaceForNewStore(llvm::AtomicOrdering ord, llvm::GenericValue *ptr)
{
	auto before = getHbBefore(getLastThreadEvent(currentT));

	for (auto i = (int) before.size() - 1; i >= 0; i--) {
		for (auto j = maxEvents[i] - 1; j >= before[i] + 1; j--) {
			EventLabel &lab = threads[i].eventList[j];
			if ((lab.isRead() || lab.isWrite()) && lab.addr == ptr)
				if (ord == llvm::NotAtomic || lab.isNotAtomic())
					return lab.pos; /* Race detected! */
		}
	}
	return Event::getInitializer(); /* Race not found */
}


/************************************************************
 ** Calculation utilities
 ***********************************************************/

template <class T>
int binSearch(const std::vector<T> &arr, int len, T what)
{
	int low = 0;
	int high = len - 1;
	while (low <= high) {
		int mid = (low + high) / 2;
		if (arr[mid] > what)
			high = mid - 1;
		else if (arr[mid] < what)
			low = mid + 1;
		else
			return mid;
	}
	return -1; /* not found */
}

void calcTransClosure(std::vector<bool> &matrix, int len)
{
	for (auto k = 0; k < len; k++) {
		for (auto i = 0; i < len; i++) {
			for (auto j = 0; j < len; j++) {
				matrix[i * len + j] = matrix[i * len + j] ||
					(matrix[i * len + k] && matrix[k * len + j]);
			}
		}
	}
}

bool isIrreflexive(std::vector<bool> &matrix, int len)
{
	for (auto i = 0; i < len; i++)
		if (matrix[i * len + i])
			return false;
	return true;
}

/*
 * Get in-degrees for event es, according to adjacency matrix
 */
std::vector<int> getInDegrees(const std::vector<bool> &matrix, const std::vector<Event> &es)
{
	std::vector<int> inDegree(es.size(), 0);

	for (auto i = 0u; i < matrix.size(); i++)
		inDegree[i % es.size()] += (int) matrix[i];
	return inDegree;
}

std::vector<Event> topoSort(const std::vector<bool> &matrix, const std::vector<Event> &es)
{
	std::vector<Event> sorted;
	std::vector<int> stack;

	/* Get in-degrees for es, according to matrix */
	auto inDegree = getInDegrees(matrix, es);

	/* Propagate events with no incoming edges to stack */
	for (auto i = 0u; i < inDegree.size(); i++)
		if (inDegree[i] == 0)
			stack.push_back(i);

	/* Perform topological sorting, filling up sorted */
	while (stack.size() > 0) {
		/* Pop next node-ID, and push node into sorted */
		auto nextI = stack.back();
		sorted.push_back(es[nextI]);
		stack.pop_back();

		for (auto i = 0u; i < es.size(); i++) {
			/* Finds all nodes with incoming edges from nextI */
			if (!matrix[nextI * es.size() + i])
				continue;
			if (--inDegree[i] == 0)
				stack.push_back(i);
		}
	}

	/* Make sure that there is no cycle */
	BUG_ON(std::any_of(inDegree.begin(), inDegree.end(), [](int degI){ return degI > 0; }));
	return sorted;
}

void allTopoSortUtil(std::vector<std::vector<Event> > &sortings, std::vector<Event> &current,
		     std::vector<bool> visited, std::vector<int> &inDegree,
		     const std::vector<bool> &matrix, const std::vector<Event> &es)
{
	/*
	 * The boolean variable 'scheduled' indicates whether this recursive call
	 * has added (scheduled) one event (at least) to the current topological sorting.
	 * If no event was added, a full topological sort has been produced.
	 */
	auto scheduled = false;

	for (auto i = 0u; i < es.size(); i++) {
		/* If ith-event can be added */
		if (inDegree[i] == 0 && !visited[i]) {
			/* Reduce in-degrees of its neighbors */
			for (auto j = 0u; j < es.size(); j++)
				if (matrix[i * es.size() + j])
					--inDegree[j];
			/* Add event in current sorting, mark as visited, and recurse */
			current.push_back(es[i]);
			visited[i] = true;

			allTopoSortUtil(sortings, current, visited, inDegree, matrix, es);

			/* Reset visited, current sorting, and inDegree */
			visited[i] = false;
			current.pop_back();
			for (auto j = 0u; j < es.size(); j++)
				if (matrix[i * es.size() + j])
					++inDegree[j];
			/* Mark that at least one event has been added to the current sorting */
			scheduled = true;
		}
	}

	/*
	 * We reach this point if no events were added in the current sorting, meaning
	 * that this is a complete sorting
	 */
	if (!scheduled)
		sortings.push_back(current);
	return;
}

std::vector<std::vector<Event> > allTopoSort(const std::vector<bool> &matrix, const std::vector<Event> &es)
{
	std::vector<bool> visited(es.size(), false);
	std::vector<std::vector<Event> > sortings;
	std::vector<Event> current;

	auto inDegree = getInDegrees(matrix, es);
	allTopoSortUtil(sortings, current, visited, inDegree, matrix, es);
	return sortings;
}

void addEdgesFromTo(const std::vector<int> &from, const std::vector<int> &to,
		    int len, std::vector<bool> &matrix)
{
	for (auto &i : from)
		for (auto &j: to)
			matrix[i * len + j] = true;
	return;
}


/************************************************************
 ** PSC calculation
 ***********************************************************/

bool ExecutionGraph::isRMWLoad(Event &e)
{
	EventLabel &lab = getEventLabel(e);
	if (lab.isWrite() || !lab.isRMW())
		return false;
	if (e.index == maxEvents[e.thread] - 1)
		return false;

	EventLabel &labNext = threads[e.thread].eventList[e.index + 1];
	if (labNext.isRMW() && labNext.isWrite() && lab.addr == labNext.addr)
		return true;
	return false;
}

std::pair<std::vector<Event>, std::vector<Event> >
ExecutionGraph::getSCs()
{
	std::vector<Event> scs, fcs;

	for (auto i = 0u; i < threads.size(); i++) {
		for (auto j = 0; j < maxEvents[i]; j++) {
			EventLabel &lab = threads[i].eventList[j];
			if (lab.isSC() && !isRMWLoad(lab.pos))
				scs.push_back(lab.pos);
			if (lab.isFence() && lab.isSC())
				fcs.push_back(lab.pos);
		}
	}
	return std::make_pair(scs,fcs);
}

std::vector<llvm::GenericValue *> ExecutionGraph::getDoubleLocs()
{
	std::vector<llvm::GenericValue *> singles, doubles;

	for (auto i = 0u; i < threads.size(); i++) {
		for (auto j = 1; j < maxEvents[i]; j++) { /* Do not consider thread inits */
			EventLabel &lab = threads[i].eventList[j];
			if (!(lab.isRead() || lab.isWrite()))
				continue;
			if (std::find(doubles.begin(), doubles.end(), lab.addr) != doubles.end())
				continue;
			if (std::find(singles.begin(), singles.end(), lab.addr) != singles.end()) {
				singles.erase(std::remove(singles.begin(), singles.end(), lab.addr),
					      singles.end());
				doubles.push_back(lab.addr);
			} else {
				singles.push_back(lab.addr);
			}
		}
	}
	return doubles;
}

int calcEventIndex(const std::vector<Event> &scs, Event e)
{
	int idx = binSearch(scs, scs.size(), e);
	BUG_ON(idx == -1);
	return idx;
}

std::vector<int> ExecutionGraph::calcSCFencesSuccs(std::vector<Event> &scs,
						   std::vector<Event> &fcs, Event &e)
{
	std::vector<int> succs;

	if (isRMWLoad(e))
		return succs;
	for (auto &f : fcs) {
		auto before = getHbBefore(f);
		if (e.index <= before[e.thread])
			succs.push_back(calcEventIndex(scs, f));
	}
	return succs;
}

std::vector<int> ExecutionGraph::calcSCFencesPreds(std::vector<Event> &scs,
						   std::vector<Event> &fcs, Event &e)
{
	std::vector<int> preds;
	auto before = getHbBefore(e);

	if (isRMWLoad(e))
		return preds;
	for (auto &f : fcs) {
		if (f.index <= before[f.thread])
			preds.push_back(calcEventIndex(scs, f));
	}
	return preds;
}

std::vector<int> ExecutionGraph::calcSCSuccs(std::vector<Event> &scs,
					     std::vector<Event> &fcs, Event &e)
{
	EventLabel &lab = getEventLabel(e);

	if (isRMWLoad(e))
		return {};
	if (lab.isSC())
		return {calcEventIndex(scs, e)};
	else
		return calcSCFencesSuccs(scs, fcs, e);
}

std::vector<int> ExecutionGraph::calcSCPreds(std::vector<Event> &scs,
					     std::vector<Event> &fcs, Event &e)
{
	EventLabel &lab = getEventLabel(e);

	if (isRMWLoad(e))
		return {};
	if (lab.isSC())
		return {calcEventIndex(scs, e)};
	else
		return calcSCFencesPreds(scs, fcs, e);
}


std::vector<int> ExecutionGraph::getSCRfSuccs(std::vector<Event> &scs, std::vector<Event> &fcs,
					      EventLabel &lab)
{
	std::vector<int> rfs;

	BUG_ON(!lab.isWrite());
	for (auto &e : lab.rfm1) {
		auto succs = calcSCSuccs(scs, fcs, e);
		rfs.insert(rfs.end(), succs.begin(), succs.end());
	}
	return rfs;
}

std::vector<int> ExecutionGraph::getSCFenceRfSuccs(std::vector<Event> &scs, std::vector<Event> &fcs,
						   EventLabel &lab)
{
	std::vector<int> fenceRfs;

	BUG_ON(!lab.isWrite());
	for (auto &e : lab.rfm1) {
		auto fenceSuccs = calcSCFencesSuccs(scs, fcs, e);
		fenceRfs.insert(fenceRfs.end(), fenceSuccs.begin(), fenceSuccs.end());
	}
	return fenceRfs;
}

void ExecutionGraph::addRbEdges(std::vector<Event> &scs, std::vector<Event> &fcs,
				std::vector<int> &moAfter, std::vector<int> &moRfAfter,
				std::vector<bool> &matrix, EventLabel &lab)
{
	BUG_ON(!lab.isWrite());
	for (auto &e : lab.rfm1) {
		auto preds = calcSCPreds(scs, fcs, e);
		auto fencePreds = calcSCFencesPreds(scs, fcs, e);

		addEdgesFromTo(preds, moAfter, scs.size(), matrix);        /* PSC_base: Adds rb-edges */
		addEdgesFromTo(fencePreds, moRfAfter, scs.size(), matrix); /* PSC_fence: Adds (rb;rf)-edges */
	}
	return;
}

void ExecutionGraph::addMoRfEdges(std::vector<Event> &scs, std::vector<Event> &fcs,
				  std::vector<int> &moAfter, std::vector<int> &moRfAfter,
				  std::vector<bool> &matrix, EventLabel &lab)
{
	auto preds = calcSCPreds(scs, fcs, lab.pos);
	auto fencePreds = calcSCFencesPreds(scs, fcs, lab.pos);
	auto rfs = getSCRfSuccs(scs, fcs, lab);

	addEdgesFromTo(preds, moAfter, scs.size(), matrix);        /* PSC_base:  Adds mo-edges */
	addEdgesFromTo(preds, rfs, scs.size(), matrix);            /* PSC_base:  Adds rf-edges */
	addEdgesFromTo(fencePreds, moRfAfter, scs.size(), matrix); /* PSC_fence: Adds (mo;rf)-edges */
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
void ExecutionGraph::addSCEcos(std::vector<Event> &scs, std::vector<Event> &fcs,
			       std::vector<Event> &mo, std::vector<bool> &matrix)
{
	std::vector<int> moAfter;   /* Ids of mo-after SC writes or writes that reach an SC fence */
	std::vector<int> moRfAfter; /* Ids of SC fences that can be reached by (mo;rf)-after reads */

	for (auto rit = mo.rbegin(); rit != mo.rend(); rit++) {
		EventLabel &lab = getEventLabel(*rit);

		addRbEdges(scs, fcs, moAfter, moRfAfter, matrix, lab);
		addMoRfEdges(scs, fcs, moAfter, moRfAfter, matrix, lab);

		auto succs = calcSCSuccs(scs, fcs, lab.pos);
		auto fenceRfs = getSCFenceRfSuccs(scs, fcs, lab);
		moAfter.insert(moAfter.end(), succs.begin(), succs.end());
		moRfAfter.insert(moRfAfter.end(), fenceRfs.begin(), fenceRfs.end());
	}
}

/*
 * addSCWbEcos is a helper function that calculates a part of PSC_base and PSC_fence,
 * like addSCEcos. The difference between them lies in the fact that addSCEcos
 * uses MO for adding mo and rb edges, addSCWBEcos uses WB for that.
 */
void ExecutionGraph::addSCWbEcos(std::vector<Event> &scs, std::vector<Event> &fcs,
				 std::vector<Event> &stores, std::vector<bool> &wbMatrix,
				 std::vector<bool> &pscMatrix)
{
	for (auto &w : stores) {
		EventLabel &wLab = getEventLabel(w);
		auto wIdx = calcEventIndex(stores, wLab.pos);

		/*
		 * Calculate which of the stores are wb-after the current
		 * write, and then collect wb-after and (wb;rf)-after SC successors
		 */
		std::vector<int> wbAfter, wbRfAfter;
		for (auto &s : stores) {
			EventLabel &sLab = getEventLabel(s);
			auto sIdx = calcEventIndex(stores, sLab.pos);
			if (wbMatrix[wIdx * stores.size() + sIdx]) {
				auto succs = calcSCSuccs(scs, fcs, sLab.pos);
				auto fenceRfs = getSCFenceRfSuccs(scs, fcs, sLab);
				wbAfter.insert(wbAfter.end(), succs.begin(), succs.end());
				wbRfAfter.insert(wbRfAfter.end(), fenceRfs.begin(), fenceRfs.end());
			}
		}

		/* Then, add the proper edges to PSC using wb-after and (wb;rf)-after successors */
		addRbEdges(scs, fcs, wbAfter, wbRfAfter, pscMatrix, wLab);
		addMoRfEdges(scs, fcs, wbAfter, wbRfAfter, pscMatrix, wLab);
	}
}

void ExecutionGraph::addSbHbEdges(std::vector<Event> &scs, std::vector<bool> &matrix)
{
	for (auto i = 0u; i < scs.size(); i++) {
		for (auto j = 0u; j < scs.size(); j++) {
			if (i == j)
				continue;
			EventLabel &ei = getEventLabel(scs[i]);
			EventLabel &ej = getEventLabel(scs[j]);

			/* PSC_base: Adds sb-edges*/
			if (ei.pos.thread == ej.pos.thread) {
				if (ei.pos.index < ej.pos.index)
					matrix[i * scs.size() + j] = true;
				continue;
			}

			/* PSC_base: Adds sb_(<>loc);hb;sb_(<>loc) edges
			 * HACK: Also works for PSC_fence: [Fsc];hb;[Fsc] edges
			 * since fences have a null address, different from the
			 * address of all global variables */
			Event prev = ej.pos.prev();
			EventLabel &ejPrev = getEventLabel(prev);
			if (!ejPrev.hbView.empty() && ej.addr != ejPrev.addr &&
			    ei.pos.index < ejPrev.hbView[ei.pos.thread]) {
				Event next = ei.pos.next();
				EventLabel &eiNext = getEventLabel(next);
				if (ei.addr != eiNext.addr)
					matrix[i * scs.size() + j] = true;
			}
		}
	}
	return;
}

void ExecutionGraph::addInitEdges(std::vector<Event> &scs, std::vector<Event> &fcs,
				  std::vector<bool> &matrix)
{
	for (auto i = 0u; i < threads.size(); i++) {
		for (auto j = 0; j < maxEvents[i]; j++) {
			EventLabel &lab = threads[i].eventList[j];
			/* Consider only reads that read from the initializer write */
			if (!lab.isRead() || !lab.rf.isInitializer() || isRMWLoad(lab.pos))
				continue;
			std::vector<int> preds = calcSCPreds(scs, fcs, lab.pos);
			std::vector<int> fencePreds = calcSCFencesPreds(scs, fcs, lab.pos);
			for (auto &w : modOrder[lab.addr]) {
				std::vector<int> wSuccs = calcSCSuccs(scs, fcs, w);
				addEdgesFromTo(preds, wSuccs, scs.size(), matrix); /* Adds rb-edges */
				for (auto &r : getEventLabel(w).rfm1) {
					std::vector<int> fenceSuccs = calcSCFencesSuccs(scs, fcs, r);
					addEdgesFromTo(fencePreds, fenceSuccs, scs.size(), matrix); /*Adds (rb;rf)-edges */
				}
			}
		}
	}
	return;
}

bool ExecutionGraph::isPscWeakAcyclicWB()
{
	/* Collect all SC events (except for RMW loads) */
	auto[scs, fcs] = getSCs();

	/* If there are no SC events, it is a valid execution */
	if (scs.empty())
		return true;

	/* Sort SC accesses for easier look-up, and create PSC matrix */
	std::vector<bool> matrix(scs.size() * scs.size(), false);
	std::sort(scs.begin(), scs.end());

	/* Add edges from the initializer write (special case) */
	addInitEdges(scs, fcs, matrix);
	/* Add sb and sb_(<>loc);hb;sb_(<>loc) edges (+ Fsc;hb;Fsc) */
	addSbHbEdges(scs, matrix);

	/*
	 * Collect memory locations with more than one SC accesses
	 * and add the rest of PSC_base and PSC_fence for only
	 * _one_ possible extension of WB for each location
	 */
	std::vector<llvm::GenericValue *> scLocs = getDoubleLocs();
	for (auto loc : scLocs) {
		auto [stores, wbMatrix] = calcWb(loc);
		auto sortedStores = topoSort(wbMatrix, stores);
		addSCEcos(scs, fcs, sortedStores, matrix);
	}

	/* Calculate the transitive closure of the bool matrix */
	calcTransClosure(matrix, scs.size());
	return isIrreflexive(matrix, scs.size());
}

bool ExecutionGraph::isPscWbAcyclicWB()
{
	/* Collect all SC events (except for RMW loads) */
	auto[scs, fcs] = getSCs();

	/* If there are no SC events, it is a valid execution */
	if (scs.empty())
		return true;

	/* Sort SC accesses for easier look-up, and create PSC matrix */
	std::vector<bool> matrix(scs.size() * scs.size(), false);
	std::sort(scs.begin(), scs.end());

	/* Add edges from the initializer write (special case) */
	addInitEdges(scs, fcs, matrix);
	/* Add sb and sb_(<>loc);hb;sb_(<>loc) edges (+ Fsc;hb;Fsc) */
	addSbHbEdges(scs, matrix);

	/*
	 * Collect memory locations with more than one SC accesses
	 * and add the rest of PSC_base and PSC_fence using WB
	 * instead of MO
	 */
	std::vector<llvm::GenericValue *> scLocs = getDoubleLocs();
	for (auto loc : scLocs) {
		auto [stores, wbMatrix] = calcWb(loc);
		addSCWbEcos(scs, fcs, stores, wbMatrix, matrix);
	}

	/* Calculate the transitive closure of the bool matrix */
	calcTransClosure(matrix, scs.size());
	return isIrreflexive(matrix, scs.size());
}

bool ExecutionGraph::isPscAcyclicWB()
{
	/* Collect all SC events (except for RMW loads) */
	auto[scs, fcs] = getSCs();

	/* If there are no SC events, it is a valid execution */
	if (scs.empty())
		return true;

	/* Sort SC accesses for easier look-up, and create PSC matrix */
	std::vector<bool> matrix(scs.size() * scs.size(), false);
	std::sort(scs.begin(), scs.end());

	/* Add edges from the initializer write (special case) */
	addInitEdges(scs, fcs, matrix);
	/* Add sb and sb_(<>loc);hb;sb_(<>loc) edges (+ Fsc;hb;Fsc) */
	addSbHbEdges(scs, matrix);

	/*
	 * Collect memory locations that have more than one SC
	 * memory access, and then calculate the possible extensions
	 * of writes-before (WB) for these memory locations
	 */
	std::vector<llvm::GenericValue *> scLocs = getDoubleLocs();
	std::vector<std::vector<std::vector<Event> > > topoSorts(scLocs.size());
	for (auto i = 0u; i < scLocs.size(); i++) {
		auto [stores, wbMatrix] = calcWb(scLocs[i]);
		topoSorts[i] = allTopoSort(wbMatrix, stores);
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
			addSCEcos(scs, fcs, topoSorts[i][count[i]], tentativePSC);
		calcTransClosure(tentativePSC, scs.size());
		if (isIrreflexive(tentativePSC, scs.size()))
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
	auto[scs, fcs] = getSCs();

	/* If there are no SC events, it is a valid execution */
	if (scs.empty())
		return true;

	/* Sort SC accesses for easier look-up, and create PSC matrix */
	std::vector<bool> matrix(scs.size() * scs.size(), false);
	std::sort(scs.begin(), scs.end());

	/* Add edges from the initializer write (special case) */
	addInitEdges(scs, fcs, matrix);
	/* Add sb and sb_(<>loc);hb;sb_(<>loc) edges (+ Fsc;hb;Fsc) */
	addSbHbEdges(scs, matrix);

	/*
	 * Collect memory locations with more than one SC accesses
	 * and add the rest of PSC_base and PSC_fence for only
	 * _one_ possible extension of WB for each location
	 */
	std::vector<llvm::GenericValue *> scLocs = getDoubleLocs();
	for (auto loc : scLocs) {
		auto &stores = modOrder[loc];
		addSCEcos(scs, fcs, stores, matrix);
	}

	/* Calculate the transitive closure of the bool matrix */
	calcTransClosure(matrix, scs.size());
	return isIrreflexive(matrix, scs.size());
}

std::pair<std::vector<Event>, std::vector<bool> >
ExecutionGraph::calcWbRestricted(llvm::GenericValue *addr, View &v)
{
	std::vector<Event> stores;

	std::copy_if(modOrder[addr].begin(), modOrder[addr].end(), std::back_inserter(stores),
		     [&v](Event &s){ return s.index <= v[s.thread]; });
	/* Sort so we can use calcEventIndex() */
	std::sort(stores.begin(), stores.end());

	std::vector<bool> matrix(stores.size() * stores.size(), false);
	for (auto i = 0u; i < stores.size(); i++) {
		auto &lab = getEventLabel(stores[i]);

		std::vector<Event> es;
		std::copy_if(lab.rfm1.begin(), lab.rfm1.end(), std::back_inserter(es),
			     [&v](Event &r){ return r.index <= v[r.thread]; });

		es.push_back(stores[i].prev());
		auto before = getHbBefore(es);
		for (auto j = 0u; j < stores.size(); j++) {
			if (i == j || !isWriteRfBefore(before, stores[j]))
				continue;
			matrix[j * stores.size() + i] = true;

			EventLabel &wLabI = getEventLabel(stores[i]);
			auto rmwsDown = getRMWChainDownTo(wLabI, stores[j]);
			for (auto &u : rmwsDown) {
				int k = calcEventIndex(stores, u);
				matrix[j * stores.size() + k] = true;
			}

			EventLabel &wLabJ = getEventLabel(stores[j]);
			auto rmwsUp = getRMWChainUpTo(wLabJ, stores[i]);
			for (auto &u : rmwsUp) {
				int k = calcEventIndex(stores, u);
				matrix[k * stores.size() + i] = true;
			}
		}
		/* Add wb-edges for chains of RMWs that read from the initializer */
		for (auto j = 0u; j < stores.size(); j++) {
			EventLabel &wLab = getEventLabel(stores[j]);
			EventLabel &pLab = getPreviousLabel(wLab.pos);
			if (i == j || !wLab.isRMW() || !pLab.rf.isInitializer())
				continue;

			auto rmwsUp = getRMWChainUpTo(wLab, stores[i]);
			for (auto &u : rmwsUp) {
				int k = calcEventIndex(stores, u);
				matrix[k * stores.size() + i] = true;
			}
		}
	}
	calcTransClosure(matrix, stores.size());
	return std::make_pair(stores, matrix);
}

std::pair<std::vector<Event>, std::vector<bool> >
ExecutionGraph::calcWb(llvm::GenericValue *addr)
{
	std::vector<Event> stores(modOrder[addr]);
	std::vector<bool> matrix(stores.size() * stores.size(), false);

	/* Sort so we can use calcEventIndex() */
	std::sort(stores.begin(), stores.end());
	for (auto i = 0u; i < stores.size(); i++) {
		auto &lab = getEventLabel(stores[i]);
		std::vector<Event> es(lab.rfm1.begin(), lab.rfm1.end());

		es.push_back(stores[i].prev());
		auto before = getHbBefore(es);
		for (auto j = 0u; j < stores.size(); j++) {
			if (i == j || !isWriteRfBefore(before, stores[j]))
				continue;
			matrix[j * stores.size() + i] = true;

			EventLabel &wLabI = getEventLabel(stores[i]);
			auto rmwsDown = getRMWChainDownTo(wLabI, stores[j]);
			for (auto &u : rmwsDown) {
				int k = calcEventIndex(stores, u);
				matrix[j * stores.size() + k] = true;
			}

			EventLabel &wLabJ = getEventLabel(stores[j]);
			auto rmwsUp = getRMWChainUpTo(wLabJ, stores[i]);
			for (auto &u : rmwsUp) {
				int k = calcEventIndex(stores, u);
				matrix[k * stores.size() + i] = true;
			}
		}
		/* Add wb-edges for chains of RMWs that read from the initializer */
		for (auto j = 0u; j < stores.size(); j++) {
			EventLabel &wLab = getEventLabel(stores[j]);
			EventLabel &pLab = getPreviousLabel(wLab.pos);
			if (i == j || !wLab.isRMW() || !pLab.rf.isInitializer())
				continue;

			auto rmwsUp = getRMWChainUpTo(wLab, stores[i]);
			for (auto &u : rmwsUp) {
				int k = calcEventIndex(stores, u);
				matrix[k * stores.size() + i] = true;
			}
		}
	}
	calcTransClosure(matrix, stores.size());
	return std::make_pair(stores, matrix);
}

bool ExecutionGraph::isWbAcyclic(void)
{
	bool acyclic = true;
	for (auto it = modOrder.begin(); it != modOrder.end(); ++it) {
		auto[stores, matrix] = calcWb(it->first);
		for (auto i = 0u; i < stores.size(); i++)
			if (matrix[i * stores.size() + i])
				acyclic = false;
	}
	return acyclic;
}


/************************************************************
 ** Library consistency checking methods
 ***********************************************************/

std::vector<Event> ExecutionGraph::getLibEventsInView(Library &lib, View &v)
{
	std::vector<Event> result;

	for (auto i = 0u; i < threads.size(); i++) {
		for (auto j = 1; j <= v[i]; j++) { /* Do not consider thread inits */
			EventLabel &lab = threads[i].eventList[j];
			if (lib.hasMember(lab.functionName))
				result.push_back(lab.pos);
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
				auto before = getHbBefore(e);
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
			for (auto &t : tos) {
				auto o = getHbBefore(t);
				v.updateMax(o);
			}

			auto [ss, wb] = calcWbRestricted(lab.addr, v);
			// TODO: Make a map with already calculated WBs??

			if (std::find(ss.begin(), ss.end(), lab.pos) == ss.end())
				continue;

			/* Collect all wb-after stores that are in "tos" range */
			int k = calcEventIndex(ss, lab.pos);
			for (auto l = 0u; l < ss.size(); l++)
				if (wb[k * ss.size() + l])
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
			auto moAfter = modOrder.getMoAfter(lab.addr, lab.pos);
			for (auto &s : moAfter)
				if (std::find(tos.begin(), tos.end(), s) != tos.end())
					buf.push_back(s);
		}
		froms[i].second = buf;
	}
}

void ExecutionGraph::calcSingleStepPairs(std::vector<std::pair<Event, std::vector<Event> > > &froms,
					 std::vector<Event> &tos,
					 llvm::StringMap<std::vector<bool> > &relMap,
					 std::vector<bool> &relMatrix, std::string &step)
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
			  std::vector<Event> &es, std::vector<bool> &matrix)
{
	for (auto &p : pairs) {
		for (auto k = 0u; k < p.second.size(); k++) {
			auto i = calcEventIndex(es, p.first);
			auto j = calcEventIndex(es, p.second[k]);
			matrix[i * es.size() + j] = true;
		}
	}
}

void ExecutionGraph::addStepEdgeToMatrix(std::vector<Event> &es,
					 llvm::StringMap<std::vector<bool> > &relMap,
					 std::vector<bool> &relMatrix,
					 std::vector<std::string> &substeps)
{
	std::vector<std::pair<Event, std::vector<Event> > > edges;

	/* Initialize edges */
	for (auto &e : es)
		edges.push_back(std::make_pair(e, std::vector<Event>({e})));

	for (auto i = 0u; i < substeps.size(); i++)
		calcSingleStepPairs(edges, es, relMap, relMatrix, substeps[i]);
	addEdgePairsToMatrix(edges, es, relMatrix);
}

llvm::StringMap<std::vector<bool> >
ExecutionGraph::calculateAllRelations(Library &lib, std::vector<Event> &es)
{
	llvm::StringMap<std::vector<bool> > relMap;

	std::sort(es.begin(), es.end());
	for (auto &r : lib.getRelations()) {
		std::vector<bool> relMatrix(es.size() * es.size(), false);
		auto &steps = r.getSteps();
		for (auto &s : steps)
			addStepEdgeToMatrix(es, relMap, relMatrix, s);

		if (r.isTransitive())
			calcTransClosure(relMatrix, es.size());
		relMap[r.getName()] = relMatrix;
	}
	return relMap;
}

bool ExecutionGraph::isLibConsistentInView(Library &lib, View &v)
{
	auto es = getLibEventsInView(lib, v);
	auto relations = calculateAllRelations(lib, es);
	auto &constraints = lib.getConstraints();
	if (std::all_of(constraints.begin(), constraints.end(),
			[&relations, &es](Constraint &c)
			{ return isIrreflexive(relations[c.getName()], es.size()); }))
		return true;
	return false;
}

std::vector<Event>
ExecutionGraph::getLibConsRfsInView(Library &lib, EventLabel &rLab,
				    const std::vector<Event> &stores, View &v)
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

void ExecutionGraph::validateGraph(void)
{
	for (auto i = 0u; i < threads.size(); i++) {
		Thread &thr = threads[i];
		WARN_ON_ONCE(thr.eventList.size() != (unsigned int) maxEvents[i],
			     "maxevents-vector-size",
			     "WARNING: Max event does not correspond to thread size!\n");
		for (int j = 0; j < maxEvents[i]; j++) {
			EventLabel &lab = thr.eventList[j];
			if (lab.isRead()) {
				Event &rf = lab.rf;
				if (lab.rf.isInitializer())
					continue;
				bool readExists = false;
				for (auto &r : getEventLabel(rf).rfm1)
					if (r == lab.pos)
						readExists = true;
				if (!readExists) {
					WARN("Read event is not the appropriate rf-1 list!\n");
					llvm::dbgs() << lab.pos << "\n";
					llvm::dbgs() << *this << "\n";
					abort();
				}
			} else if (lab.isWrite()) {
				if (lab.isRMW() && lab.rfm1.size() > 1) {
					WARN("RMW store is read from more than 1 load!\n");
					llvm::dbgs() << "RMW store: " << lab.pos << "\nReads:";
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
					if (getEventLabel(e).rf == lab.pos)
						writeExists = true;
				if (!writeExists) {
					WARN("Write event is not marked in the read event!\n");
					llvm::dbgs() << lab.pos << "\n";
					llvm::dbgs() << *this << "\n";
					abort();
				}
				for (auto &r : lab.rfm1) {
					if (r.thread > threads.size() ||
					    r.index >= maxEvents[r.thread]) {
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
 ** Printing facilities
 ***********************************************************/

void ExecutionGraph::calcTraceBefore(const Event &e, View &a, std::stringstream &buf)
{
	int ai = a[e.thread];
	if (e.index <= ai)
		return;

	a[e.thread] = e.index;
	Thread &thr = threads[e.thread];
	for (int i = ai; i <= e.index; i++) {
		EventLabel &lab = thr.eventList[i];
		if ((lab.isRead() || lab.isStart() || lab.isJoin()) &&
		    !lab.rf.isInitializer())
			calcTraceBefore(lab.rf, a, buf);
		Parser::parseInstFromMData(buf, thr.prefixLOC[i], thr.threadFun->getName().str());
	}
	return;
}

void ExecutionGraph::printTraceBefore(Event e)
{
	std::stringstream buf;
	View a;

	calcTraceBefore(e, a, buf);
	llvm::dbgs() << buf.str();
}

void ExecutionGraph::prettyPrintGraph()
{
	for (auto i = 0u; i < threads.size(); i++) {
		auto &thr = threads[i];
		llvm::dbgs() << "<" << thr.parentId << "," << thr.id
			     << "> " << thr.threadFun->getName() << ": ";
		for (auto j = 0; j < maxEvents[i]; j++) {
			auto &lab = thr.eventList[j];
			if (lab.isRead()) {
				if (lab.isRevisitable())
					llvm::dbgs().changeColor(llvm::buffer_ostream::Colors::GREEN);
				auto val = EE->loadValueFromWrite(lab.rf, lab.valTyp, lab.addr);
				llvm::dbgs() << lab.type << EE->getGlobalName(lab.addr) << ","
					     << val.IntVal << " ";
				llvm::dbgs().resetColor();
			} else if (lab.isWrite()) {
				llvm::dbgs() << lab.type << EE->getGlobalName(lab.addr) << ","
					     << lab.val.IntVal << " ";
			}
		}
		llvm::dbgs() << "\n";
	}
	llvm::dbgs() << "\n";
}

void ExecutionGraph::dotPrintToFile(std::string &filename, View &before, Event e)
{
	std::ofstream fout(filename);
	std::string dump;
	llvm::raw_string_ostream ss(dump);

	/* Create a directed graph graph */
	ss << "strict digraph {\n";
	/* Specify node shape */
	ss << "\tnode [shape=box]\n";
	/* Left-justify labels for clusters */
	ss << "\tlabeljust=l\n";
	/* Create a node for the initializer event */
	ss << "\t\"" << Event::getInitializer() << "\"[label=INIT,root=true]\n";

	/* Print all nodes with each thread represented by a cluster */
	for (auto i = 0u; i < before.size(); i++) {
		auto &thr = threads[i];
		ss << "subgraph cluster_" << thr.id << "{\n";
		ss << "\tlabel=\"" << thr.threadFun->getName().str() << "()\"\n";
		for (auto j = 1; j <= before[i]; j++) {
			std::stringstream buf;
			auto lab = thr.eventList[j];

			Parser::parseInstFromMData(buf, thr.prefixLOC[j], "");
			ss << "\t" << lab.pos << " [label=\"" << buf.str() << "\""
			   << (lab.pos == e ? ",style=filled,fillcolor=yellow" : "") << "]\n";
		}
		ss << "}\n";
	}

	/* Print relations between events (po U rf) */
	for (auto i = 0u; i < before.size(); i++) {
		auto &thr = threads[i];
		for (auto j = 0; j <= before[i]; j++) {
			std::stringstream buf;
			auto lab = thr.eventList[j];

			Parser::parseInstFromMData(buf, thr.prefixLOC[j], "");
			/* Print a po-edge, but skip dummy start events for
			 * all threads except for the first one */
			if (j < before[i] && !(lab.isStart() && i > 0))
				ss << lab.pos << " -> " << lab.pos.next() << "\n";
			if (lab.isRead())
				ss << "\t" << lab.rf << " -> " << lab.pos << "[color=green]\n";
			if (thr.id > 0 && lab.isStart())
				ss << "\t" << lab.rf << " -> " << lab.pos.next() << "[color=blue]\n";
			if (lab.isJoin())
				ss << "\t" << lab.rf << " -> " << lab.pos << "[color=blue]\n";
		}
	}

	ss << "}\n";
	fout << ss.str();
	fout.close();
}


/************************************************************
 ** Overloaded operators
 ***********************************************************/

llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const ExecutionGraph &g)
{
	for (auto i = 0u; i < g.threads.size(); i++) {
		const Thread &thr = g.threads[i];
		s << thr << "\n";
		for (auto j = 0; j < g.maxEvents[i]; j++) {
			const EventLabel &lab = thr.eventList[j];
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
	s << "Max Events:\n\t" << g.maxEvents << "\n";
	return s;
}
