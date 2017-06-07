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

ExecutionGraph::ExecutionGraph() : currentT(0) {}

ExecutionGraph::ExecutionGraph(std::vector<std::vector<llvm::ExecutionContext> > &ECStacks)
{
	currentT = 0;
	for (auto i = 0u; i < threads.size(); i++)
		threads[i].ECStack = ECStacks[i];
}

bool ExecutionGraph::isConsistent(void)
{
	return true;
}

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

void ExecutionGraph::validateGraph(void)
{
	for (auto i = 0u; i < threads.size(); i++) {
		Thread &thr = threads[i];
		WARN_ON(thr.eventList.size() != (unsigned int) maxEvents[i],
			"Max event does not correspond to thread size!\n");
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
					std::cerr << lab.pos << std::endl;
					std::cerr << *this << std::endl;
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
					std::cerr << lab.pos << std::endl;
					std::cerr << *this << std::endl;
					abort();
				}
			}
		}
	}
	return;
}

void ExecutionGraph::cutBefore(std::vector<int> &preds, std::vector<Event> &rev)
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

void ExecutionGraph::cutAfter(std::vector<int> &after)
{
	for (auto i = 0u; i < threads.size(); i++) {
		maxEvents[i] = after[i];
		Thread &thr = threads[i];
		thr.eventList.erase(thr.eventList.begin() + after[i], thr.eventList.end());
		for (int j = 0; j < maxEvents[i]; j++) {
			EventLabel &lab = thr.eventList[j];
			if (lab.type != W) {
				continue;
			}
			lab.rfm1.remove_if([&after](Event &e)
					   { return e.index >= after[e.thread]; });
		}
	}
	revisit.erase(std::remove_if(revisit.begin(), revisit.end(), [&after](Event &e)
				       { return e.index >= after[e.thread]; }),
			revisit.end());
	for (auto it = modOrder.begin(); it != modOrder.end(); ++it)
		it->second.erase(std::remove_if(it->second.begin(), it->second.end(),
						[&after](Event &e)
						{ return e.index >= after[e.thread]; }),
				 it->second.end());
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
	}
	/* TODO: Do some check here for initializer or if it is already present? */
	EventLabel &sLab = getEventLabel(store);
	sLab.rfm1.insert(sLab.rfm1.end(), es.begin(), es.end());
	return;
}

void ExecutionGraph::markReadsAsVisited(std::vector<Event> &K, std::vector<Event> K0,
					Event store)
{
	if (!K0.empty()) {
		K.erase(std::remove_if(K.begin(), K.end(), [&K0](Event &e)
				       { return e == K0.back(); }),
			K.end());
	}

	std::vector<int> before = getPorfBefore(K);
	revisit.erase(std::remove_if(revisit.begin(), revisit.end(), [&before](Event &e)
				     { return e.index <= before[e.thread]; }),
			revisit.end());
	return;
}

std::vector<Event> ExecutionGraph::getRevisitLoads(Event store)
{
	std::vector<Event> ls;
	std::vector<int> before = getPorfBefore(store);
	EventLabel &sLab = getEventLabel(store);

	WARN_ON(sLab.type != W, "getRevisitLoads called with non-store event?");
	for (auto it = revisit.begin(); it != revisit.end(); ++it) {
		EventLabel &rLab = getEventLabel(*it);
		if (before[rLab.pos.thread] < rLab.pos.index && rLab.addr == sLab.addr)
			ls.push_back(*it);
	}
	return ls;
}

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
	std::vector<int> a(threads.size());

	for (auto i = 0u; i < a.size(); i++)
		a[i] = maxEvents[i];

	calcPorfAfter(Event(e.thread, e.index + 1), a);
	return a;
}

std::vector<int> ExecutionGraph::getPorfAfter(const std::vector<Event> &es)
{
	std::vector<int> a(threads.size());

	for (auto i = 0u; i < a.size(); i++)
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

std::vector<int> ExecutionGraph::getPorfBeforeNoRfs(const std::vector<Event> &es)
{
	std::vector<int> a(threads.size(), 0);

	for (auto &e : es)
		calcPorfBefore(Event(e).prev(), a);
	for (auto &e : es)
		if (a[e.thread] < e.index)
			a[e.thread] = e.index;
	return a;
}

void ExecutionGraph::calcHbBefore(const Event &e, std::vector<int> &a)
{
	int ai = a[e.thread];
	if (e.index <= ai)
		return;

	a[e.thread] = e.index;
	Thread &thr = threads[e.thread];
	for (int i = ai; i <= e.index; i++) {
		EventLabel &lab = thr.eventList[i];
		if (lab.isRead() && lab.isAtLeastAcquire() && !lab.rf.isInitializer()) {
			EventLabel &rfLab = getEventLabel(lab.rf);
			if (rfLab.isAtLeastRelease())
				calcHbBefore(rfLab.pos, a);
		}
	}
	return;
}

std::vector<int> ExecutionGraph::getHbBefore(Event e)
{
	std::vector<int> a(threads.size(), 0);
	calcHbBefore(e, a);
	return a;
}

std::vector<int> ExecutionGraph::getHbBefore(const std::vector<Event> &es)
{
	std::vector<int> a(threads.size(), 0);

	for (auto &e : es)
		calcHbBefore(e, a);
	return a;
}

std::ostream& operator<<(std::ostream &s, const ExecutionGraph &g)
{
	s << std::endl;
	for (auto i = 0u; i < g.threads.size(); i++) {
		const Thread &thr = g.threads[i];
		s << thr << std::endl;
		for (auto j = 0; j < g.maxEvents[i]; j++) {
			const EventLabel &lab = thr.eventList[j];
			s << "\t" << lab;
			if (lab.type == R)
				s << "\n\t\treads from: " << lab.rf;
			else if (lab.type == W) {
				for (auto &r : lab.rfm1)
					s << "\n\t\t is read from: " << r;
			}
			s << std::endl;
		}
	}
	s << "Revisit Set:" << std::endl;
	for (auto &l : g.revisit)
		s << "\t" << l << std::endl;
	s << "Max Events:" << std::endl;
	for (unsigned int i = 0; i < g.maxEvents.size(); i++)
		s << "\t" << g.maxEvents[i] << std::endl;
	s << std::endl;
	return s;
}

ExecutionGraph initGraph;
ExecutionGraph *currentEG;
