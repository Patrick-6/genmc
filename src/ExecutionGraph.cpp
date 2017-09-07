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

#include "Thread.hpp"
#include "ExecutionGraph.hpp"
#include <llvm/IR/DebugInfo.h>

#include <fstream>
#include <sstream>


/************************************************************
 ** Class Constructors
 ***********************************************************/

ExecutionGraph::ExecutionGraph() : currentT(0) {}


/************************************************************
 ** Basic getter methods
 ***********************************************************/

EventLabel& ExecutionGraph::getEventLabel(Event &e)
{
	return threads[e.thread].eventList[e.index];
}

EventLabel& ExecutionGraph::getPreviousLabel(Event &e)
{
	return threads[e.thread].eventList[e.index-1];
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
	if (e.isInitializer() || e.isThreadStart())
		return View();

	EventLabel &lab = getEventLabel(e);
	return lab.hbView;
}

View ExecutionGraph::getEventMsgView(Event e)
{
	if (e.isInitializer() || e.isThreadStart())
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

std::vector<llvm::ExecutionContext>& ExecutionGraph::getThreadECStack(int thread)
{
	return threads[thread].ECStack;
}

std::vector<Event> ExecutionGraph::getRevisitLoads(Event store)
{
	std::vector<Event> ls;
	std::vector<int> before = getPorfBefore(store);
	EventLabel &sLab = getEventLabel(store);

	BUG_ON(!sLab.isWrite());
	for (auto it = revisit.begin(); it != revisit.end(); ++it) {
		EventLabel &rLab = getEventLabel(revisit.getAtPos(it));
		if (before[rLab.pos.thread] < rLab.pos.index && rLab.addr == sLab.addr)
			ls.push_back(rLab.pos);
	}
	return ls;
}

std::vector<Event> ExecutionGraph::calcOptionalRfs(Event store, std::vector<Event> &locMO)
{
	std::vector<Event> ls;
	for (auto rit = locMO.rbegin(); rit != locMO.rend(); ++rit) {
		if (*rit == store)
			return ls;
		EventLabel &lab = getEventLabel(*rit);
		if (lab.isWrite()) {
			ls.push_back(lab.pos);
			for (auto &l : lab.rfm1)
				ls.push_back(l);
		}
	}
	BUG();
}

std::vector<Event> ExecutionGraph::getRevisitLoadsNonMaximal(Event store)
{
	std::vector<Event> ls;
	std::vector<int> before = getPorfBefore(store);
	EventLabel &sLab = getEventLabel(store);

	BUG_ON(!sLab.isWrite());
	for (auto &e : revisit) {
		EventLabel &lab = getEventLabel(e);
		if (before[e.thread] < e.index && lab.addr == sLab.addr)
			ls.push_back(e);
	}

	std::vector<Event> locMO = modOrder.getAtLoc(sLab.addr);
	std::vector<Event> optRfs = calcOptionalRfs(store, locMO);
	ls.erase(std::remove_if(ls.begin(), ls.end(), [this, &optRfs](Event e)
				{ std::vector<int> before = this->getHbPoBefore(e);
				  return std::any_of(optRfs.begin(), optRfs.end(),
					 [&before](Event ev)
					 { return ev.index <= before[ev.thread]; });
				}), ls.end());
	return ls;
}


/************************************************************
 ** Basic setter methods
 ***********************************************************/

void ExecutionGraph::addEventToGraph(EventLabel &lab)
{
	Thread &thr = threads[currentT];
	thr.eventList.push_back(lab);
	++maxEvents[currentT];
}

void ExecutionGraph::addReadToGraphCommon(EventLabel &lab, Event &rf)
{
	lab.hbView = getEventHbView(getLastThreadEvent(currentT)).getCopy(threads.size());
	lab.hbView[currentT] = maxEvents[currentT];
	if (lab.isAtLeastAcquire()) {
		View mV = getEventMsgView(lab.rf);
		lab.hbView.updateMax(mV);
	}

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
	EventLabel lab(R, Plain, ord, Event(currentT, max), ptr, typ, rf);
	addReadToGraphCommon(lab, rf);
}

void ExecutionGraph::addCASReadToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
				       llvm::GenericValue &val, llvm::Type *typ, Event rf)
{
	int max = maxEvents[currentT];
	EventLabel lab(R, CAS, ord, Event(currentT, max), ptr, val, typ, rf);
	addReadToGraphCommon(lab, rf);
}

void ExecutionGraph::addRMWReadToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
				       llvm::Type *typ, Event rf)
{
	int max = maxEvents[currentT];
	EventLabel lab(R, RMW, ord, Event(currentT, max), ptr, typ, rf);
	addReadToGraphCommon(lab, rf);
}

void ExecutionGraph::addStoreToGraphCommon(EventLabel &lab)
{
	lab.hbView = getEventHbView(getLastThreadEvent(currentT)).getCopy(threads.size());
	lab.hbView[currentT] = maxEvents[currentT];
	if (lab.isRMW()) {
		Event last = getLastThreadEvent(currentT);
		EventLabel &pLab = getEventLabel(last);
		View mV = getEventMsgView(pLab.rf);
		BUG_ON(lab.ord == llvm::NotAtomic);
		if (lab.isAtLeastRelease())
			lab.msgView = mV.getMax(lab.hbView);
		else
			lab.msgView = getEventHbView(
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
	EventLabel lab(W, Plain, ord, Event(currentT, max), ptr, val, typ);
	addStoreToGraphCommon(lab);
}

void ExecutionGraph::addCASStoreToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
					llvm::GenericValue &val, llvm::Type *typ)
{
	int max = maxEvents[currentT];
	EventLabel lab(W, CAS, ord, Event(currentT, max), ptr, val, typ);
	addStoreToGraphCommon(lab);
}

void ExecutionGraph::addRMWStoreToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
					llvm::GenericValue &val, llvm::Type *typ)
{
	int max = maxEvents[currentT];
	EventLabel lab(W, RMW, ord, Event(currentT, max), ptr, val, typ);
	addStoreToGraphCommon(lab);
}

void ExecutionGraph::addFenceToGraph(llvm::AtomicOrdering ord)

{
	int max = maxEvents[currentT];
	EventLabel lab(F, ord, Event(currentT, max));

	lab.hbView = getEventHbView(getLastThreadEvent(currentT)).getCopy(threads.size());
	lab.hbView[currentT] = maxEvents[currentT];
	if (lab.isAtLeastAcquire())
		calcRelRfPoBefore(currentT, getLastThreadEvent(currentT).index, lab.hbView);
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
		if (lab.type == W) /* TODO: Maybe reference before r? */
			for (auto r : lab.rfm1) {
				calcPorfAfter(r, a);
			}
	}
	return;
}

std::vector<int> ExecutionGraph::getPorfAfter(Event e)
{
	std::vector<int> a(maxEvents);
	calcPorfAfter(Event(e.thread, e.index + 1), a);
	return a;
}

std::vector<int> ExecutionGraph::getPorfAfter(const std::vector<Event> &es)
{
	std::vector<int> a(maxEvents);
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
	for (int i = ai; i <= e.index; i++) {
		EventLabel &lab = thr.eventList[i];
		if (lab.type == R && !lab.rf.isInitializer())
			calcPorfBefore(lab.rf, a);
	}
	return;
}

std::vector<int> ExecutionGraph::getPorfBefore(Event e)
{
	std::vector<int> a(threads.size(), 0);

	calcPorfBefore(e, a);
	return a;
}

std::vector<int> ExecutionGraph::getPorfBefore(const std::vector<Event> &es)
{
	std::vector<int> a(threads.size(), 0);

	for (auto &e : es)
		calcPorfBefore(e, a);
	return a;
}

std::vector<int> ExecutionGraph::getHbBefore(Event e)
{
	return getEventHbView(e).toVector();
}

std::vector<int> ExecutionGraph::getHbBefore(const std::vector<Event> &es)
{
	View v(threads.size());

	for (auto &e : es) {
		View o = getEventHbView(e);
		v.updateMax(o);
		if (v[e.thread] < e.index)
			v[e.thread] = e.index;
	}
	return v.toVector();
}

std::vector<int> ExecutionGraph::getHbPoBefore(Event e)
{
	View v = getEventHbView(e.prev()).getCopy(threads.size());
	v[e.thread] = e.index;
	return v.toVector();
}

void ExecutionGraph::calcRelRfPoBefore(int thread, int index, View &v)
{
	for (auto i = index; i > 0; i--) {
		EventLabel &lab = threads[thread].eventList[i];
		if (lab.isFence() && lab.isAtLeastAcquire())
			return;
		if (lab.isRead() && lab.ord == llvm::Monotonic) {
			View o = getEventMsgView(lab.rf);
			v.updateMax(o);
		}
	}
}


/************************************************************
 ** Calculation of writes a read can read from
 ***********************************************************/

bool ExecutionGraph::isWriteRfBefore(std::vector<int> &before, Event e)
{
	if (e.index <= before[e.thread])
		return true;

	EventLabel &lab = getEventLabel(e);
	BUG_ON(!lab.isWrite());
	for (auto &e : lab.rfm1) {
		if (e.index <= before[e.thread] && !revisit.contains(e))
			return true;
	}
	return false;
}

std::vector<Event> ExecutionGraph::findOverwrittenBoundary(llvm::GenericValue *addr, int thread)
{
	std::vector<Event> boundary;
	std::vector<int> before = getHbBefore(getLastThreadEvent(thread));

	if (before.empty())
		return boundary;

	for (auto &e : modOrder.getAtLoc(addr))
		if (isWriteRfBefore(before, e))
			boundary.push_back(e.prev());
	return boundary;
}

std::vector<Event> ExecutionGraph::getStoresWeakRA(llvm::GenericValue *addr)
{
	std::vector<Event> stores;
	std::vector<Event> overwritten = findOverwrittenBoundary(addr, currentT);
	if (overwritten.empty()) {
		std::vector<Event> locMO = modOrder.getAtLoc(addr);
		stores.push_back(Event(0, 0));
		stores.insert(stores.end(), locMO.begin(), locMO.end());
		return stores;
	}

	std::vector<int> before = getHbBefore(overwritten);
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

std::vector<std::vector<Event> > ExecutionGraph::splitLocMOBefore(std::vector<int> &before,
								  std::vector<Event> &locMO)
{
	std::vector<std::vector<Event> > result;
	std::vector<Event> revConcStores;
	std::vector<Event> prevStores;

	for (auto rit = locMO.rbegin(); rit != locMO.rend(); ++rit) {
		if (before.empty() || !isWriteRfBefore(before, *rit)) {
			revConcStores.push_back(*rit);
		} else {
			for (auto rit2 = rit; rit2 != locMO.rend(); ++rit2)
				prevStores.push_back(*rit2);
			result.push_back(revConcStores);
			result.push_back(prevStores);
			return result;
		}
	}
	result.push_back(revConcStores);
	result.push_back({});
	return result;

}

std::vector<Event> ExecutionGraph::getStoresMO(llvm::GenericValue *addr)
{
	std::vector<Event> stores;
	std::vector<int> before = getHbBefore(getLastThreadEvent(currentT));
	std::vector<Event> locMO = modOrder.getAtLoc(addr);

	std::vector<std::vector<Event> > partStores = splitLocMOBefore(before, locMO);
	if (partStores[1].empty())
		stores.push_back(Event(0, 0));
	else
		stores.push_back(partStores[1][0]);
	stores.insert(stores.end(), partStores[0].begin(), partStores[0].end());
	return stores;
}

std::vector<Event> ExecutionGraph::getStoresToLoc(llvm::GenericValue *addr, ModelType model)
{
	switch (model) {
	case wrc11:
		return getStoresWeakRA(addr);
	case rc11:
		return getStoresMO(addr);
	default:
		WARN("Unimplemented model.\n");
		abort();
	}
}


/************************************************************
 ** Graph modification methods
 ***********************************************************/

void ExecutionGraph::cutBefore(std::vector<int> &preds, RevisitSet &rev)
{
	for (auto i = 0u; i < threads.size(); i++) {
		maxEvents[i] = preds[i] + 1;
		Thread &thr = threads[i];
		thr.eventList.erase(thr.eventList.begin() + preds[i] + 1, thr.eventList.end());
		for (int j = 0; j < maxEvents[i]; j++) {
			EventLabel &lab = thr.eventList[j];
			if (lab.type != W)
				continue;
			lab.rfm1.remove_if([&preds](Event &e)
					   { return e.index > preds[e.thread]; });
		}
	}
	revisit = rev;
	for (auto it = modOrder.begin(); it != modOrder.end(); ++it)
		it->second.erase(std::remove_if(it->second.begin(), it->second.end(),
						[&preds](Event &e)
						{ return e.index > preds[e.thread]; }),
				 it->second.end());
	return;
}

void ExecutionGraph::cutToCopyAfter(ExecutionGraph &other, std::vector<int> &after)
{
	for (auto i = 0u; i < other.threads.size(); i++) {
		maxEvents.push_back(after[i]);
		Thread &oThr = other.threads[i];
		threads.push_back(Thread(oThr.threadFun, oThr.id));
		Thread &thr = threads[i];
		for (auto j = 0; j < maxEvents[i]; j++) {
			EventLabel &oLab = oThr.eventList[j];
			if (oLab.type != W) {
				thr.eventList.push_back(oLab);
			} else {
				thr.eventList.push_back(oLab);
				EventLabel &lab = getEventLabel(oLab.pos);
				lab.rfm1.remove_if([&after](Event &e)
						   { return e.index >= after[e.thread]; });
			}
		}
	}
	for (auto it = other.revisit.begin(); it != other.revisit.end(); ++it) {
		Event &e = other.revisit.getAtPos(it);
		if (e.index >= after[e.thread])
			continue;
		revisit.add(e);
	}
	for (auto it = other.modOrder.begin(); it != other.modOrder.end(); ++it) {
		llvm::GenericValue *addr = other.modOrder.getAddrAtPos(it);
		std::vector<Event> locMO = other.modOrder.getAtLoc(addr);
		for (auto &s : locMO)
			if (s.index < after[s.thread])
				modOrder.addAtLocEnd(addr, s);
	}
}

void ExecutionGraph::modifyRfs(std::vector<Event> &es, Event store)
{
	if (es.empty())
		return;

	for (std::vector<Event>::iterator it = es.begin(); it != es.end(); ++it) {
		EventLabel &lab = getEventLabel(*it);
		Event oldRf = lab.rf;
		lab.rf = store;
		if (!oldRf.isInitializer()) {
			EventLabel &oldL = getEventLabel(oldRf);
			oldL.rfm1.remove(*it);
		}
		lab.hbView = getEventHbView(lab.pos.prev()).getCopy(threads.size());
		lab.hbView[lab.pos.thread] = lab.pos.index;
		if (lab.isAtLeastAcquire()) {
			View mV = getEventMsgView(lab.rf);
			lab.hbView.updateMax(mV);
		}
	}
	/* TODO: Do some check here for initializer or if it is already present? */
	EventLabel &sLab = getEventLabel(store);
	sLab.rfm1.insert(sLab.rfm1.end(), es.begin(), es.end());
	return;
}

void ExecutionGraph::clearAllStacks(void)
{
	std::for_each(threads.begin(), threads.end(), [](Thread &t)
		      { t.ECStack.clear(); });
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

bool ExecutionGraph::scheduleNext(void)
{
	for (unsigned int i = 0; i < threads.size(); i++) {
		if (!getThreadECStack(i).empty()) {
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
	if (!revisit.containsPorfBefore(before)) {
		clearAllStacks();
		shouldContinue = false;
		return;
	}

	getThreadECStack(currentT).clear();
	threads[currentT].isBlocked = true;
}


/************************************************************
 ** Race detection methods
 ***********************************************************/

Event ExecutionGraph::findRaceForNewLoad(llvm::AtomicOrdering ord, llvm::GenericValue *ptr)
{
	std::vector<int> before = getHbBefore(getLastThreadEvent(currentT));
	std::vector<Event> stores = modOrder.getAtLoc(ptr);

	/* If there are not any events hb-before the read, there is nothing to do */
	if (before.empty())
		return Event(0, 0);

	/* Check for events that race with the current load */
	for (auto &s : stores) {
		if (s.index > before[s.thread]) {
			EventLabel &sLab = getEventLabel(s);
			if (ord == llvm::NotAtomic || sLab.isNotAtomic())
				return s; /* Race detected! */
		}
	}
	return Event(0, 0); /* Race not found */
}

Event ExecutionGraph::findRaceForNewStore(llvm::AtomicOrdering ord, llvm::GenericValue *ptr)
{
	std::vector<int> before = getHbBefore(getLastThreadEvent(currentT));

	for (auto i = (int) before.size() - 1; i >= 0; i--) {
		for (auto j = maxEvents[i] - 1; j >= before[i] + 1; j--) {
			EventLabel &lab = threads[i].eventList[j];
			if ((lab.isRead() || lab.isWrite()) && lab.addr == ptr)
				if (ord == llvm::NotAtomic || lab.isNotAtomic())
					return lab.pos; /* Race detected! */
		}
	}
	return Event(0, 0); /* Race not found */
}


/************************************************************
 ** PSC calculation
 ***********************************************************/

template <class T>
int binSearch(std::vector<T> &arr, int len, T what)
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

std::vector<llvm::GenericValue *> ExecutionGraph::getDoubleLocs()
{
	std::vector<llvm::GenericValue *> singles, doubles;

	for (auto i = 0u; i < threads.size(); i++) {
		for (auto j = 1; j < maxEvents[i]; j++) { /* Do not consider thread inits */
			EventLabel &lab = threads[i].eventList[j];
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

int calcSCIndex(std::vector<Event> &scs, Event e)
{
	int idx = binSearch(scs, scs.size(), e);
	BUG_ON(idx == -1);
	return idx;
}

std::vector<int> ExecutionGraph::calcSCSuccs(std::vector<Event> &scs, std::vector<Event> &fcs, Event &e)
{
	std::vector<int> succs;
	EventLabel &lab = getEventLabel(e);

	if (lab.isSCExceptRMWLoad()) {
		succs.push_back(calcSCIndex(scs, e));
	} else if (!(lab.isRead() && lab.isRMW())) {
		for (auto &f : fcs) {
			std::vector<int> before = getHbBefore(f);
			if (e.index <= before[e.thread])
				succs.push_back(calcSCIndex(scs, f));
		}
	}
	return succs;
}

std::vector<int> ExecutionGraph::calcSCPreds(std::vector<Event> &scs, std::vector<Event> &fcs, Event &e)
{
	std::vector<int> preds;
	EventLabel &lab = getEventLabel(e);

	if (lab.isSCExceptRMWLoad()) {
		preds.push_back(calcSCIndex(scs, e));
	} else if (!(lab.isRead() && lab.isRMW())) {
		std::vector<int> before = getHbBefore(e);
		for (auto &f : fcs) {
			if (f.index <= before[f.thread])
				preds.push_back(calcSCIndex(scs, f));
		}
	}
	return preds;
}

std::vector<int> ExecutionGraph::addReadsToSCList(std::vector<Event> &scs, std::vector<Event> &fcs,
						  std::vector<int> &moAfter, std::vector<int> &moRfAfter,
						  std::vector<bool> &matrix, std::list<Event> &es)
{
	std::vector<int> rfs;
	for (auto &e : es) {
		std::vector<int> preds = calcSCPreds(scs, fcs, e);
		for (auto i : preds) {
			for (auto j : moAfter)
				matrix[i * scs.size() + j] = true; /* Adds rb-edges */
			for (auto j : moRfAfter)
				matrix[i * scs.size() + j] = true; /* Adds (rb;rf)-edges */
		}
		std::vector<int> succs = calcSCSuccs(scs, fcs, e);
		rfs.insert(rfs.end(), succs.begin(), succs.end());
	}
	return rfs;
}

void ExecutionGraph::addSCEcos(std::vector<Event> &scs, std::vector<Event> &fcs,
			       llvm::GenericValue *addr, std::vector<bool> &matrix)
{
	std::vector<int> moAfter, moRfAfter;
	std::vector<Event> mo = modOrder.getAtLoc(addr);
	for (auto rit = mo.rbegin(); rit != mo.rend(); rit++) {
		EventLabel &lab = getEventLabel(*rit);
		std::vector<int> rfs = addReadsToSCList(scs, fcs, moAfter, moRfAfter, matrix, lab.rfm1);
		std::vector<int> preds = calcSCPreds(scs, fcs, lab.pos);
		for (auto i : preds) {
			for (auto j : moAfter)
				matrix[i * scs.size() + j] = true; /* Adds mo-edges */
			for (auto j : rfs)
				matrix[i * scs.size() + j] = true; /* Adds rf-edges */
			for (auto j : moRfAfter)
				matrix[i * scs.size() + j] = true; /* Adds (mo;rf)-edges */
		}
		std::vector<int> succs = calcSCSuccs(scs, fcs, lab.pos);
		moAfter.insert(moAfter.end(), succs.begin(), succs.end());
		moRfAfter.insert(moRfAfter.end(), rfs.begin(), rfs.end());
	}
}

bool ExecutionGraph::isPscAcyclic()
{
	/* Collect all SC events (except for RMW loads) */
	std::vector<Event> scs, fcs;
	for (auto i = 0u; i < threads.size(); i++) {
		for (auto j = 0; j < maxEvents[i]; j++) {
			EventLabel &lab = threads[i].eventList[j];
			if (lab.isSCExceptRMWLoad())
				scs.push_back(lab.pos);
			if (lab.isFence() && lab.ord == llvm::SequentiallyConsistent)
				fcs.push_back(lab.pos);
		}
	}

	if (scs.empty())
		return true;

	/* Collect duplicate SC memory accesses */
	std::vector<llvm::GenericValue *> scLocs = getDoubleLocs();
	std::sort(scs.begin(), scs.end());

	/* Add SC ecos */
	std::vector<bool> matrix(scs.size() * scs.size(), false);
	for (auto loc : scLocs)
		addSCEcos(scs, fcs, loc, matrix);

	for (auto i = 0u; i < threads.size(); i++) {
		for (auto j = 0; j < maxEvents[i]; j++) {
			EventLabel &lab = threads[i].eventList[j];
			/* Consider only reads that read from the initializer write */
			if (!lab.isRead() || !lab.rf.isInitializer() || lab.isRMW())
				continue;
			std::vector<int> preds = calcSCPreds(scs, fcs, lab.pos);
			for (auto &w : modOrder.getAtLoc(lab.addr)) {
				std::vector<int> wSuccs = calcSCSuccs(scs, fcs, w);
				for (auto i : preds)
					for (auto j : wSuccs)
						matrix[i * scs.size() + j] = true; /* Adds rb-edges */
				for (auto &r : getEventLabel(w).rfm1) {
					std::vector<int> rSuccs = calcSCSuccs(scs, fcs, r);
					for (auto i : preds)
						for (auto j : rSuccs)
							matrix[i * scs.size() + j] = true; /* Adds (rb;rf)-edges */
				}
			}
		}
	}

	for (auto i = 0u; i < scs.size(); i++) {
		for (auto j = 0u; j < scs.size(); j++) {
			if (i == j)
				continue;
			if (scs[i].thread == scs[j].thread) {
				if (scs[i].index < scs[j].index)
					matrix[i * scs.size() + j] = true;
			} else {
				View hb = getEventHbView(scs[j].prev());
				if (!hb.empty() && scs[i].index < hb[scs[i].thread])
					matrix[i * scs.size() + j] = true;
			}
		}
	}

	/* Calculate the transitive closure of the bool matrix */
	calcTransClosure(matrix, scs.size());
	for (auto i = 0u; i < scs.size(); i++)
		if (matrix[i * scs.size() + i])
			return false;
	return true;
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
			if (lab.type == R) {
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
			} else if (lab.type == W) {
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
			}
		}
	}
	return;
}


/************************************************************
 ** Printing facilities
 ***********************************************************/

static void getInstFromMData(std::stringstream &ss, Thread &thr, int i)
{
	auto &prefix = thr.prefixLOC[i];
	for (auto &pair : prefix) {
		int line = pair.first;
		std::string absPath = pair.second;

		std::ifstream ifs(absPath);
		std::string s;
		int curLine = 0;
		while (ifs.good() && curLine < line) {
			std::getline(ifs, s);
			++curLine;
		}

		s.erase(s.begin(), std::find_if(s.begin(), s.end(),
			std::not1(std::ptr_fun<int, int>(std::isspace))));
		s.erase(std::find_if(s.rbegin(), s.rend(),
                        std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());

		ss << "[" << thr.threadFun->getName().str() << "] ";
		auto i = absPath.find_last_of('/');
		if (i == std::string::npos)
			ss << absPath;
		else
			ss << absPath.substr(i + 1);

		ss << ": " << line << ": ";
		ss << s << std::endl;
	}
}

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
		if (lab.type == R && !lab.rf.isInitializer()) {
			calcTraceBefore(lab.rf, a, buf);
			getInstFromMData(buf, thr, i);
		} else if (!lab.pos.isThreadStart()) {
			getInstFromMData(buf, thr, i);
		}
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
			if (lab.type == R)
				s << "\t\treads from: " << lab.rf << "\n";
		}
	}
	s << "Max Events:\n";
	for (unsigned int i = 0; i < g.maxEvents.size(); i++)
		s << "\t" << g.maxEvents[i] << "\n";
	return s;
}

ExecutionGraph initGraph;
ExecutionGraph *currentEG = &initGraph;
