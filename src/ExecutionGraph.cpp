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
