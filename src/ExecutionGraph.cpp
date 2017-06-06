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

bool ExecutionGraph::isConsistent()
{
	return true;
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
