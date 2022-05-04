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

/*******************************************************************************
 * CAUTION: This file is generated automatically by Kater -- DO NOT EDIT.
 *******************************************************************************/

#ifndef __SC_CHECKER_HPP__
#define __SC_CHECKER_HPP__

#include "ExecutionGraph.hpp"
#include "GraphIterators.hpp"
#include "MaximalIterator.hpp"
#include "PersistencyChecker.hpp"
#include "VSet.hpp"
#include <vector>

class SCChecker {

private:
	enum class NodeStatus { unseen, entered, left };

	struct NodeCountStatus {
		NodeCountStatus() = default;
		NodeCountStatus(unsigned c, NodeStatus s) : count(c), status(s) {}
		unsigned count = 0;
		NodeStatus status = NodeStatus::unseen;
	};

public:
	SCChecker(ExecutionGraph &g) : g(g) {}

	std::vector<VSet<Event>> calculateSaved(const Event &e);
	std::vector<View> calculateViews(const Event &e);
	bool isConsistent(const Event &e);

private:
	bool visitAcyclic0(const Event &e);

	bool isAcyclic(const Event &e);

	std::vector<NodeCountStatus> visitedAcyclic0;

	unsigned visitedAccepting = 0;
	std::vector<VSet<Event>> saved;
	std::vector<View> views;

	ExecutionGraph &g;

	ExecutionGraph &getGraph() { return g; }
};

#endif /* __SC_CHECKER_HPP__ */
