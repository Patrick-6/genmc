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

ExecutionGraph::ExecutionGraph(llvm::Interpreter *EE) : EE(EE), currentT(0) {}


/************************************************************
 ** Basic getter methods
 ***********************************************************/

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

View ExecutionGraph::getEventHbView(Event e)
{
	if (e.isInitializer())
		return View();

	EventLabel &lab = getEventLabel(e);
	return lab.hbView;
}

View ExecutionGraph::getEventMsgView(Event e)
{
	if (e.isInitializer())
		return View();

	EventLabel &lab = getEventLabel(e);
	BUG_ON(!lab.isWrite());
	return lab.msgView;
}

std::vector<int> ExecutionGraph::getGraphState(void)
{
	std::vector<int> state;

	for (auto i = 0u; i < threads.size(); i++)
		state.push_back(maxEvents[i] - 1);
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

std::vector<Event> ExecutionGraph::getRevisitLoadsWB(EventLabel &sLab)
{
	std::vector<int> before = getPorfBefore(sLab.pos);
	std::vector<Event> ls;

	BUG_ON(!sLab.isWrite());
	for (auto i = 0u; i < threads.size(); i++) {
		for (auto j = before[i] + 1; j < maxEvents[i]; j++) {
			EventLabel &lab = threads[i].eventList[j];
			if (lab.isRead() && lab.addr == sLab.addr && lab.isRevisitable())
				ls.push_back(lab.pos);
		}
	}
	return ls;
}

std::vector<Event> ExecutionGraph::getRMWChain(Event &store)
{
	std::vector<Event> chain;
	auto sLab = getEventLabel(store);

	BUG_ON(!(sLab.isWrite() && sLab.isRMW()));
	while (sLab.isWrite() && sLab.isRMW()) {
		chain.push_back(sLab.pos);
		auto prev = sLab.pos.prev();
		auto rLab = getEventLabel(prev);
		/* If the predecessor reads the initializer
		 * event, then the chain is over */
		if (rLab.rf.isInitializer()) {
			chain.push_back(Event::getInitializer());
			return chain;
		}
		sLab = getEventLabel(rLab.rf);
	}
	/* We arrived at a non-RMW event (which is not the initializer),
	 * so the chain is over */
	chain.push_back(sLab.pos);
	return chain;
}

std::vector<Event> ExecutionGraph::getStoresHbAfterStores(llvm::GenericValue *loc,
							  std::vector<Event> &chain)
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

std::vector<Event> ExecutionGraph::getRevisitLoadsRMWWB(EventLabel &sLab)
{
	std::vector<Event> ls;
	auto before = getPorfBefore(sLab.pos);
	auto chain = getRMWChain(sLab.pos);
	auto hbAfterStores = getStoresHbAfterStores(sLab.addr, chain);

	BUG_ON(!sLab.isWrite());
	for (auto i = 0u; i < threads.size(); i++) {
		for (auto j = before[i] + 1; j < maxEvents[i]; j++) {
			EventLabel &lab = threads[i].eventList[j];
			if (lab.isRead() && lab.addr == sLab.addr && lab.isRevisitable()) {
				auto prev = lab.pos.prev();
				auto &pLab = getEventLabel(prev);
				if (std::all_of(hbAfterStores.begin(), hbAfterStores.end(),
						[&pLab, this](Event w)
						{ return !this->isWriteRfBefore(pLab.hbView, w); }))
					ls.push_back(lab.pos);
			}

		}
	}
	return ls;
}

std::vector<Event> ExecutionGraph::calcOptionalRfs(const std::vector<Event> &locMO, Event store)
{
	std::vector<Event> ls;
	for (auto rit = locMO.rbegin(); rit != locMO.rend(); ++rit) {
		if (*rit == store)
			return ls;
		auto &lab = getEventLabel(*rit);
		if (lab.isWrite()) {
			ls.push_back(lab.pos);
			for (auto &l : lab.rfm1)
				ls.push_back(l);
		}
	}
	BUG();
}

std::vector<Event> ExecutionGraph::getRevisitLoadsMO(EventLabel &sLab)
{
	std::vector<int> before = getPorfBefore(sLab.pos);
	std::vector<Event> ls;

	BUG_ON(!sLab.isWrite());
	for (auto i = 0u; i < threads.size(); i++) {
		for (auto j = before[i] + 1; j < maxEvents[i]; j++) {
			EventLabel &lab = threads[i].eventList[j];
			if (lab.isRead() && lab.addr == sLab.addr && lab.isRevisitable())
				ls.push_back(lab.pos);
		}
	}

	auto &locMO = modOrder[sLab.addr];

	/* If this store is mo-maximal then we are done */
	if (locMO.back() == sLab.pos)
		return ls;

	/* Otherwise, we have to exclude (mo;rf?;hb?;sb)-after reads */
	std::vector<Event> optRfs = calcOptionalRfs(locMO, sLab.pos);
	ls.erase(std::remove_if(ls.begin(), ls.end(), [this, &optRfs](Event e)
				{ View before = this->getHbPoBefore(e);
				  return std::any_of(optRfs.begin(), optRfs.end(),
					 [&before](Event ev)
					 { return ev.index <= before[ev.thread]; });
				}), ls.end());
	return ls;
}


/************************************************************
 ** Basic setter methods
 ***********************************************************/

void ExecutionGraph::calcLoadHbView(EventLabel &lab, Event prev, Event &rf)
{
	lab.hbView = View(getEventHbView(prev));
	lab.hbView[prev.thread] = prev.index + 1;
	if (lab.isAtLeastAcquire()) {
		View mV = getEventMsgView(lab.rf);
		lab.hbView.updateMax(mV);
	}
}

void ExecutionGraph::addEventToGraph(EventLabel &lab)
{
	Thread &thr = threads[currentT];
	thr.eventList.push_back(lab);
	++maxEvents[currentT];
}

void ExecutionGraph::addReadToGraphCommon(EventLabel &lab, Event &rf)
{
	lab.revType = Normal;
	lab.loadPreds = getGraphState();
	++lab.loadPreds[currentT];
	calcLoadHbView(lab, getLastThreadEvent(currentT), rf);

	addEventToGraph(lab);

	if (rf.isInitializer())
		return;
	Thread &rfThr = threads[rf.thread];
	EventLabel &rfLab = rfThr.eventList[rf.index];
	rfLab.rfm1.push_front(lab.pos);
	return;
}

void ExecutionGraph::addReadToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
				    llvm::Type *typ, Event rf)
{
	int max = maxEvents[currentT];
	EventLabel lab(ERead, Plain, ord, Event(currentT, max), ptr, typ, rf);
	addReadToGraphCommon(lab, rf);
}

void ExecutionGraph::addGReadToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
				     llvm::Type *typ, Event rf, std::string functionName)
{
	int max = maxEvents[currentT];
	EventLabel lab(ERead, Plain, ord, Event(currentT, max), ptr, typ, rf, functionName);
	addReadToGraphCommon(lab, rf);
}

void ExecutionGraph::addCASReadToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
				       llvm::GenericValue &val, llvm::GenericValue &nextVal,
				       llvm::Type *typ, Event rf)
{
	int max = maxEvents[currentT];
	EventLabel lab(ERead, CAS, ord, Event(currentT, max), ptr, val, nextVal, typ, rf);
	addReadToGraphCommon(lab, rf);
}

void ExecutionGraph::addRMWReadToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
				       llvm::GenericValue &nextVal,
				       llvm::AtomicRMWInst::BinOp op,
				       llvm::Type *typ, Event rf)
{
	int max = maxEvents[currentT];
	EventLabel lab(ERead, RMW, ord, Event(currentT, max), ptr, nextVal, op, typ, rf);
	addReadToGraphCommon(lab, rf);
}

void ExecutionGraph::addStoreToGraphCommon(EventLabel &lab)
{
	lab.hbView = View(getEventHbView(getLastThreadEvent(currentT)));
	lab.hbView[currentT] = maxEvents[currentT];
	if (lab.isRMW()) {
		Event last = getLastThreadEvent(currentT);
		EventLabel &pLab = getEventLabel(last);
		View mV = getEventMsgView(pLab.rf);
		BUG_ON(pLab.ord == llvm::NotAtomic);
		if (pLab.isAtLeastRelease())
			lab.msgView = mV.getMax(lab.hbView);
		else
			lab.msgView = getEventHbView( /* no need for ctor -- getMax will copy the View */
				getLastThreadRelease(currentT, lab.addr)).getMax(mV);
	} else {
		if (lab.isAtLeastRelease())
			lab.msgView = lab.hbView;
		else if (lab.ord == llvm::Monotonic || lab.ord == llvm::Acquire)
			lab.msgView = getEventHbView(getLastThreadRelease(currentT, lab.addr));
	}
	addEventToGraph(lab);
	return;
}

void ExecutionGraph::addStoreToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
				     llvm::GenericValue &val, llvm::Type *typ)
{
	int max = maxEvents[currentT];
	EventLabel lab(EWrite, Plain, ord, Event(currentT, max), ptr, val, typ);
	addStoreToGraphCommon(lab);
}

void ExecutionGraph::addGStoreToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
				      llvm::GenericValue &val, llvm::Type *typ,
				      std::string functionName)
{
	int max = maxEvents[currentT];
	EventLabel lab(EWrite, Plain, ord, Event(currentT, max), ptr, val, typ, functionName);
	addStoreToGraphCommon(lab);
}

void ExecutionGraph::addCASStoreToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
					llvm::GenericValue &val, llvm::Type *typ)
{
	int max = maxEvents[currentT];
	EventLabel lab(EWrite, CAS, ord, Event(currentT, max), ptr, val, typ);
	addStoreToGraphCommon(lab);
}

void ExecutionGraph::addRMWStoreToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
					llvm::GenericValue &val, llvm::Type *typ)
{
	int max = maxEvents[currentT];
	EventLabel lab(EWrite, RMW, ord, Event(currentT, max), ptr, val, typ);
	addStoreToGraphCommon(lab);
}

void ExecutionGraph::addFenceToGraph(llvm::AtomicOrdering ord)

{
	int max = maxEvents[currentT];
	EventLabel lab(EFence, ord, Event(currentT, max));

	lab.hbView = getEventHbView(getLastThreadEvent(currentT));
	lab.hbView[currentT] = maxEvents[currentT];
	if (lab.isAtLeastAcquire())
		calcRelRfPoBefore(currentT, getLastThreadEvent(currentT).index, lab.hbView);
	addEventToGraph(lab);
}

void ExecutionGraph::addTCreateToGraph(int cid)
{
	int max = maxEvents[currentT];
	EventLabel lab(ETCreate, llvm::Release, Event(currentT, max), cid);

	/* Thread creation has Release semantics */
	lab.hbView = View(getEventHbView(getLastThreadEvent(currentT)));
	lab.hbView[currentT] = maxEvents[currentT];
	lab.msgView = lab.hbView;
	addEventToGraph(lab);
}

void ExecutionGraph::addTJoinToGraph(int cid)
{
	int max = maxEvents[currentT];
	EventLabel lab(ETJoin, llvm::Acquire, Event(currentT, max), cid);

	/* Thread joins have acquire semantics -- but we have to wait
	 * for the other thread to finish first, so we do not fully
	 * update the view yet */
	lab.hbView = View(getEventHbView(getLastThreadEvent(currentT)));
	lab.hbView[currentT] = max;
	addEventToGraph(lab);
}

void ExecutionGraph::addStartToGraph(int tid, Event tc)
{
	int max = maxEvents[tid];
	EventLabel lab = EventLabel(EStart, llvm::Acquire, Event(tid, max), tc);

	/* Thread start has Acquire semantics */
	lab.hbView = getEventHbView(tc);
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

	/* Thread termination has Release semantics */
	lab.hbView = View(getEventHbView(getLastThreadEvent(currentT)));
	lab.hbView[currentT] = max;
	lab.msgView = lab.hbView;
	addEventToGraph(lab);
}

/************************************************************
 ** Calculation of [(po U rf)*] predecessors and successors
 ***********************************************************/

void ExecutionGraph::calcPorfAfter(const Event &e, std::vector<int> &a)
{
	int ai = a[e.thread];
	if (e.index >= ai)
		return;

	a[e.thread] = e.index;
	Thread &thr = threads[e.thread];
	for (int i = e.index; i <= ai; i++) {
		if (i >= maxEvents[e.thread])
			break;
		EventLabel &lab = thr.eventList[i];
		if (lab.isWrite() || lab.isCreate() || lab.isFinish()) /* TODO: Maybe reference before r? */
			for (auto r : lab.rfm1) {
				calcPorfAfter(r, a);
			}
	}
	return;
}

std::vector<int> ExecutionGraph::getPorfAfter(Event e)
{
	std::vector<int> a(threads.size(), 0);
	for (auto i = 0u; i < maxEvents.size(); i++)
		a[i] = maxEvents[i];

	calcPorfAfter(Event(e.thread, e.index + 1), a);
	return a;
}

std::vector<int> ExecutionGraph::getPorfAfter(const std::vector<Event> &es)
{
	std::vector<int> a(threads.size(), 0);
	for (auto i = 0u; i < maxEvents.size(); i++)
		a[i] = maxEvents[i];

	for (auto &e : es)
		calcPorfAfter(Event(e.thread, e.index + 1), a);
	return a;
}

void ExecutionGraph::calcPorfBefore(const Event &e, std::vector<int> &a)
{
	int ai = a[e.thread];
	if (e.index <= ai)
		return;

	a[e.thread] = e.index;
	Thread &thr = threads[e.thread];
	for (int i = ai + 1; i <= e.index; i++) {
		EventLabel &lab = thr.eventList[i];
		if ((lab.isRead() || lab.isStart() || lab.isJoin())
		    && !lab.rf.isInitializer())
			calcPorfBefore(lab.rf, a);
	}
	return;
}

std::vector<int> ExecutionGraph::getPorfBefore(Event e)
{
	std::vector<int> a(threads.size(), -1);

	calcPorfBefore(e, a);
	return a;
}

std::vector<int> ExecutionGraph::getPorfBefore(const std::vector<Event> &es)
{
	std::vector<int> a(threads.size(), -1);

	for (auto &e : es)
		calcPorfBefore(e, a);
	return a;
}

View ExecutionGraph::getHbBefore(Event e)
{
	return getEventHbView(e);
}

View ExecutionGraph::getHbBefore(const std::vector<Event> &es)
{
	View v;

	for (auto &e : es) {
		View o = getEventHbView(e);
		v.updateMax(o);
		if (v[e.thread] < e.index)
			v[e.thread] = e.index;
	}
	return v;
}

View ExecutionGraph::getHbPoBefore(Event e)
{
	View v(getEventHbView(e.prev()));
	return v;
}

void ExecutionGraph::calcHbRfBefore(Event &e, llvm::GenericValue *addr,
				    std::vector<int> &a)
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

std::vector<int> ExecutionGraph::getHbRfBefore(std::vector<Event> &es)
{
	std::vector<int> a(threads.size(), -1);

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
			View o = getEventMsgView(lab.rf);
			v.updateMax(o);
		}
	}
}


/************************************************************
 ** Calculation of particular sets of events/event labels
 ***********************************************************/

std::vector<EventLabel>
ExecutionGraph::getPrefixLabelsNotBefore(const std::vector<int> &prefix,
					 const std::vector<int> &before)
{
	std::vector<EventLabel> result;

	for (auto i = 0u; i < threads.size(); i++) {
		for (auto j = before[i] + 1; j <= prefix[i]; j++) {
			EventLabel &lab = threads[i].eventList[j];
			result.push_back(lab);
		}
	}
	return result;
}

std::vector<Event>
ExecutionGraph::getRfsNotBefore(const std::vector<EventLabel> &labs,
				const std::vector<int> &before)
{
	std::vector<Event> rfs;

	std::for_each(labs.begin(), labs.end(), [&rfs, &before](const EventLabel &lab)
		      { if (lab.isRead() && lab.pos.index > before[lab.pos.thread])
				      rfs.push_back(lab.rf); });
	return rfs;
}

std::vector<std::pair<Event, Event> >
ExecutionGraph::getMOPredsInBefore(const std::vector<EventLabel> &labs,
				   const std::vector<int> &before)
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

	EventLabel &lab = getEventLabel(e);
	BUG_ON(!lab.isWrite());
	for (auto &e : lab.rfm1)
		if (e.index <= before[e.thread])
			return true;
	return false;
}

std::vector<Event> ExecutionGraph::findOverwrittenBoundary(llvm::GenericValue *addr, int thread)
{
	std::vector<Event> boundary;
	View before = getHbBefore(getLastThreadEvent(thread));

	if (before.empty())
		return boundary;

	for (auto &e : modOrder[addr])
		if (isWriteRfBefore(before, e))
			boundary.push_back(e.prev());
	return boundary;
}

std::vector<Event> ExecutionGraph::getStoresToLocWeakRA(llvm::GenericValue *addr)
{
	std::vector<Event> stores;
	std::vector<Event> overwritten = findOverwrittenBoundary(addr, currentT);
	if (overwritten.empty()) {
		auto &locMO = modOrder[addr];
		stores.push_back(Event::getInitializer());
		stores.insert(stores.end(), locMO.begin(), locMO.end());
		return stores;
	}

	std::vector<int> before = getHbRfBefore(overwritten);
	for (auto i = 0u; i < threads.size(); i++) {
		Thread &thr = threads[i];
		for (auto j = before[i] + 1; j < maxEvents[i]; j++) {
			EventLabel &lab = thr.eventList[j];
			if (lab.isWrite() && lab.addr == addr)
				stores.push_back(lab.pos);
		}
	}
	return stores;
}

/* View before _can_ be implicitly modified */
std::pair<std::vector<Event>, std::vector<Event> >
ExecutionGraph::splitLocMOBefore(const std::vector<Event> &locMO, View &before)
{
	std::vector<Event> concurrent, previous;

	for (auto rit = locMO.rbegin(); rit != locMO.rend(); ++rit) {
		if (before.empty() || !isWriteRfBefore(before, *rit)) {
			concurrent.push_back(*rit);
		} else {
			std::reverse_copy(rit, locMO.rend(), std::back_inserter(previous));
			break;
		}
	}
	std::reverse(concurrent.begin(), concurrent.end());
	return std::make_pair(concurrent, previous);
}

std::vector<Event> ExecutionGraph::getStoresToLocMO(llvm::GenericValue *addr)
{
	std::vector<Event> stores;
	std::vector<Event> &locMO = modOrder[addr];
	View before = getHbBefore(getLastThreadEvent(currentT));

	auto [concurrent, previous] = splitLocMOBefore(locMO, before);

	/*
	 * If there are not stores (hb;rf?)-before the current event
	 * then we can read read from all concurrent stores and the
	 * initializer store. Otherwise, we can read from all concurrent
	 * stores and the mo-latest of the (hb;rf?)-before stores.
	 */
	if (previous.empty())
		stores.push_back(Event::getInitializer());
	else
		stores.push_back(previous.back());
	stores.insert(stores.end(), concurrent.begin(), concurrent.end());
	return stores;
}

std::vector<Event> ExecutionGraph::getStoresToLocWB(llvm::GenericValue *addr)
{
	auto[stores, wb] = calcWb(addr);
	auto hbBefore = getHbBefore(getLastThreadEvent(currentT));
	auto porfBefore = getPorfBefore(getLastThreadEvent(currentT));

	// Find the stores from which we can read-from
	std::vector<Event> result;
	for (auto i = 0u; i < stores.size(); i++) {
		bool allowed = true;
		for (auto j = 0u; j < stores.size(); j++) {
			if (wb[i * stores.size() + j] &&
			    isWriteRfBefore(hbBefore, stores[j]))
				allowed = false;
		}
		if (allowed)
			result.push_back(stores[i]);
	}

	// Also check the initializer event
	bool allowed = true;
	for (auto j = 0u; j < stores.size(); j++)
		if (isWriteRfBefore(hbBefore, stores[j]))
			allowed = false;
	if (allowed)
		result.insert(result.begin(), Event::getInitializer());
	return result;
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
	/* Update the hb-view of the load */
	calcLoadHbView(lab, lab.pos.prev(), store);
}


void ExecutionGraph::cutToEventView(Event &e, const std::vector<int> &preds)
{
	EventLabel &lab = getEventLabel(e);
	for (auto i = 0u; i < threads.size(); i++) {
		maxEvents[i] = preds[i] + 1;
		Thread &thr = threads[i];
		thr.eventList.erase(thr.eventList.begin() + preds[i] + 1, thr.eventList.end());
		for (int j = 0; j < maxEvents[i]; j++) {
			EventLabel &lab = thr.eventList[j];
			if (!(lab.isWrite() || lab.isCreate() || lab.isFinish()))
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

void ExecutionGraph::restoreStorePrefix(EventLabel &rLab, std::vector<int> &storePorfBefore,
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
		if (curLab.pos.index <= storePorfBefore[curLab.pos.thread] &&
		    curLab.pos.index > rLab.loadPreds[curLab.pos.thread])
			curLab.makeNotRevisitable();
		if (curLab.isWrite() || curLab.isFinish() || curLab.isCreate())
			curLab.rfm1.remove_if([&rLab, &storePorfBefore](Event &e)
					      { return e.index > storePorfBefore[e.thread] &&
						       e.index > rLab.loadPreds[e.thread]; });
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

void ExecutionGraph::clearAllStacks(void)
{
	for (auto i = 0u; i < threads.size(); i++)
		getECStack(i).clear();
}


/************************************************************
 ** Consistency checks
 ***********************************************************/

bool ExecutionGraph::isConsistent(void)
{
	return true;
}


/************************************************************
 ** Graph exploration methods
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
	Event e = getLastThreadEvent(currentT);
	std::vector<int> before = getPorfBefore(e);
	// if (!revisit.containsPorfBefore(before)) {
	// 	threads[currentT].isBlocked = true;
	// 	clearAllStacks();
	// 	return;
	// }

	threads[currentT].isBlocked = true;
	getECStack(currentT).clear();
}


/************************************************************
 ** Race detection methods
 ***********************************************************/

Event ExecutionGraph::findRaceForNewLoad(llvm::AtomicOrdering ord, llvm::GenericValue *ptr)
{
	View before = getHbBefore(getLastThreadEvent(currentT));
	std::vector<Event> &stores = modOrder[ptr];

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
	View before = getHbBefore(getLastThreadEvent(currentT));

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
		View before = getHbBefore(f);
		if (e.index <= before[e.thread])
			succs.push_back(calcEventIndex(scs, f));
	}
	return succs;
}

std::vector<int> ExecutionGraph::calcSCFencesPreds(std::vector<Event> &scs,
						   std::vector<Event> &fcs, Event &e)
{
	std::vector<int> preds;
	View before = getHbBefore(e);

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

			/* Go up the RMW chain */
			EventLabel wLab = getEventLabel(stores[i]); /* Not a ref! */
			while (wLab.isWrite() && wLab.isRMW() && wLab.pos != stores[j]) {
				int k = calcEventIndex(stores, wLab.pos);
				matrix[j * stores.size() + k] = true;
				Event p = wLab.pos.prev();
				EventLabel &pLab = getEventLabel(p);
				wLab = getEventLabel(pLab.rf);
			}
			/* Go down the RMW chain */
			wLab = getEventLabel(stores[j]); /* Not a ref! */
			while (wLab.isWrite() && wLab.isRMW() && wLab.pos != stores[i]) {
				int k = calcEventIndex(stores, wLab.pos);
				matrix[k * stores.size() + i] = true;
				std::vector<Event> rmwRfs;
				std::copy_if(wLab.rfm1.begin(), wLab.rfm1.end(),
					     std::back_inserter(rmwRfs),
					     [this, &wLab](Event &r)
					     { EventLabel &rLab = this->getEventLabel(r);
					       return this->EE->isSuccessfulRMW(rLab, wLab.val); });
				BUG_ON(rmwRfs.size() > 1);
				if (rmwRfs.size() == 0)
					break;
				auto nextW = rmwRfs.back().next();
				wLab = getEventLabel(nextW);
			}
		}
		/* Add wb-edges for chains of RMWs that read from the initializer */
		for (auto j = 0u; j < stores.size(); j++) {
			EventLabel wLab = getEventLabel(stores[j]);
			Event prev = wLab.pos.prev();
			EventLabel &pLab = getEventLabel(prev);
			if (i == j || !wLab.isRMW() || !pLab.rf.isInitializer())
				continue;

			while (wLab.isWrite() && wLab.isRMW() && wLab.pos != stores[i]) {
				int k = calcEventIndex(stores, wLab.pos);
				matrix[k * stores.size() + i] = true;
				std::vector<Event> rmwRfs;
				std::copy_if(wLab.rfm1.begin(), wLab.rfm1.end(),
					     std::back_inserter(rmwRfs),
					     [this, &wLab](Event &r)
					     { EventLabel &rLab = this->getEventLabel(r);
					       return this->EE->isSuccessfulRMW(rLab, wLab.val); });
				BUG_ON(rmwRfs.size() > 1);
				if (rmwRfs.size() == 0)
					break;
				auto nextW = rmwRfs.back().next();
				wLab = getEventLabel(nextW);
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

std::vector<Event> ExecutionGraph::getLibraryEvents(Library &lib)
{
	std::vector<Event> result;

	for (auto i = 0u; i < threads.size(); i++) {
		for (auto j = 1; j < maxEvents[i]; j++) { /* Do not consider thread inits */
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
			if (lab.isRead())
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
			buf.insert(buf.end(), lab.rfm1.begin(), lab.rfm1.end());
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

			// TODO: Make a map with already calculated WBs??
			auto [stores, wb] = calcWb(lab.addr);

			int k = calcEventIndex(stores, lab.pos);
			for (auto l = 0u; l < stores.size(); l++)
				if (wb[k * stores.size() + l])
					buf.push_back(stores[l]);
		}
		froms[i].second = buf;
	}
}

void ExecutionGraph::calcSingleStepPairs(Library &lib,
					 std::vector<std::pair<Event, std::vector<Event> > > &froms,
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

void ExecutionGraph::addStepEdgeToMatrix(Library &lib, std::vector<Event> &es,
					 llvm::StringMap<std::vector<bool> > &relMap,
					 std::vector<bool> &relMatrix,
					 std::vector<std::string> &substeps)
{
	std::vector<std::pair<Event, std::vector<Event> > > edges;

	/* Initialize edges */
	for (auto &e : es)
		edges.push_back(std::make_pair(e, std::vector<Event>({e})));

	for (auto i = 0u; i < substeps.size(); i++)
		calcSingleStepPairs(lib, edges, es, relMap, relMatrix, substeps[i]);
	addEdgePairsToMatrix(edges, es, relMatrix);
}

llvm::StringMap<std::vector<bool> >
ExecutionGraph::calculateAllRelations(Library &lib, std::vector<Event> &es)
{
	llvm::StringMap<std::vector<bool> > relMap;

	for (auto &r : lib.getRelations()) {
		std::vector<bool> relMatrix(es.size() * es.size(), false);
		auto &steps = r.getSteps();
		for (auto &s : steps)
			addStepEdgeToMatrix(lib, es, relMap, relMatrix, s);

		if (r.isTransitive())
			calcTransClosure(relMatrix, es.size());
		relMap[r.getName()] = relMatrix;

		llvm::dbgs() << "Gonna print map for relation " << r.getName() << "\n"
			     << "This relation is " << (r.isTransitive() ? "" : "not") << "transitive\n";
		llvm::dbgs() << "Library events are: \n";
		for (auto &e : es)
			llvm::dbgs() << e << " ";
		llvm::dbgs() << "\n";

		for (auto i = 0u; i < es.size(); i++) {
			for (auto j = 0u; j < es.size(); j++)
				llvm::dbgs() << (relMatrix[i * es.size() + j] ? 1 : 0) << " ";
			llvm::dbgs() << "\n";
		}
		llvm::dbgs() << "\n";
	}
	return relMap;
}

std::vector<Event>
ExecutionGraph::filterLibConstraints(Library &lib, Event &load, const std::vector<Event> &stores)
{
	std::vector<Event> filtered;
	EventLabel &lab = getEventLabel(load);
	std::vector<Event> es = getLibraryEvents(lib);

	std::sort(es.begin(), es.end());
	for (auto &s : stores) {
		changeRf(lab, s);
		auto relations = calculateAllRelations(lib, es);
		auto &constraints = lib.getConstraints();
		if (std::all_of(constraints.begin(), constraints.end(),
				[&relations, &es](Constraint &c)
				{ return isIrreflexive(relations[c.getName()], es.size()); }))
			filtered.push_back(s);
	}

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

void ExecutionGraph::calcTraceBefore(const Event &e, std::vector<int> &a,
				     std::stringstream &buf)
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
	std::vector<int> a(threads.size(), 0);

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

void ExecutionGraph::dotPrintToFile(std::string &filename, std::vector<int> &before, Event e)
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
