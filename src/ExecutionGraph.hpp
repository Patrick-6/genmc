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

#ifndef __EXECUTION_GRAPH_HPP__
#define __EXECUTION_GRAPH_HPP__

#include "Event.hpp"
#include "Thread.hpp"

#include <unordered_map>

/*
 * ExecutionGraph class - This class represents an execution graph
 */
class ExecutionGraph {
public:
	std::vector<Thread> threads;
	std::vector<int> maxEvents;
	std::vector<Event> revisit;
	std::unordered_map<llvm::GenericValue *, std::vector<Event> > modOrder;
	int currentT;

	ExecutionGraph();
	ExecutionGraph(std::vector<Thread> ts, std::vector<int> es,
		       std::vector<Event> re, int t)
		: threads(ts), maxEvents(es), revisit(re), currentT(t) {};

	bool isConsistent();

	friend std::ostream& operator<<(std::ostream &s, const ExecutionGraph &g);
};

extern ExecutionGraph initGraph;
extern ExecutionGraph *currentEG;

#endif /* __EXECUTION_GRAPH_HPP__ */
