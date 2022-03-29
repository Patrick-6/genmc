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

#ifndef __RC11_CHECKER_HPP__
#define __RC11_CHECKER_HPP__

#include "ExecutionGraph.hpp"
#include "GraphIterators.hpp"
#include "PersistencyChecker.hpp"
#include "VSet.hpp"
#include <vector>

class RC11Checker {

private:
	enum class NodeStatus { unseen, entered, left };

	struct NodeCountStatus {
		NodeCountStatus() = default;
		NodeCountStatus(unsigned c, NodeStatus s) : count(c), status(s) {}
		unsigned count = 0;
		NodeStatus status = NodeStatus::unseen;
	};

public:
	RC11Checker(ExecutionGraph &g) : g(g) {}

	std::vector<VSet<Event>> calculateAll(const Event &e);
	bool isConsistent(const Event &e);

private:
	void visitCalc0_0(const Event &e, VSet<Event> &calcRes);
	void visitCalc0_1(const Event &e, VSet<Event> &calcRes);
	void visitCalc0_2(const Event &e, VSet<Event> &calcRes);
	void visitCalc0_3(const Event &e, VSet<Event> &calcRes);
	void visitCalc0_4(const Event &e, VSet<Event> &calcRes);
	void visitCalc0_5(const Event &e, VSet<Event> &calcRes);
	void visitCalc0_6(const Event &e, VSet<Event> &calcRes);

	VSet<Event> calculate0(const Event &e);

	std::vector<NodeStatus> visitedCalc0_0;
	std::vector<NodeStatus> visitedCalc0_1;
	std::vector<NodeStatus> visitedCalc0_2;
	std::vector<NodeStatus> visitedCalc0_3;
	std::vector<NodeStatus> visitedCalc0_4;
	std::vector<NodeStatus> visitedCalc0_5;
	std::vector<NodeStatus> visitedCalc0_6;

	void visitCalc1_0(const Event &e, VSet<Event> &calcRes);
	void visitCalc1_1(const Event &e, VSet<Event> &calcRes);
	void visitCalc1_2(const Event &e, VSet<Event> &calcRes);
	void visitCalc1_3(const Event &e, VSet<Event> &calcRes);
	void visitCalc1_4(const Event &e, VSet<Event> &calcRes);
	void visitCalc1_5(const Event &e, VSet<Event> &calcRes);

	VSet<Event> calculate1(const Event &e);

	std::vector<NodeStatus> visitedCalc1_0;
	std::vector<NodeStatus> visitedCalc1_1;
	std::vector<NodeStatus> visitedCalc1_2;
	std::vector<NodeStatus> visitedCalc1_3;
	std::vector<NodeStatus> visitedCalc1_4;
	std::vector<NodeStatus> visitedCalc1_5;

	bool visitAcyclic0(const Event &e);
	bool visitAcyclic1(const Event &e);
	bool visitAcyclic2(const Event &e);
	bool visitAcyclic3(const Event &e);
	bool visitAcyclic4(const Event &e);
	bool visitAcyclic5(const Event &e);
	bool visitAcyclic6(const Event &e);
	bool visitAcyclic7(const Event &e);
	bool visitAcyclic8(const Event &e);
	bool visitAcyclic9(const Event &e);
	bool visitAcyclic10(const Event &e);
	bool visitAcyclic11(const Event &e);
	bool visitAcyclic12(const Event &e);
	bool visitAcyclic13(const Event &e);
	bool visitAcyclic14(const Event &e);
	bool visitAcyclic15(const Event &e);
	bool visitAcyclic16(const Event &e);
	bool visitAcyclic17(const Event &e);
	bool visitAcyclic18(const Event &e);
	bool visitAcyclic19(const Event &e);
	bool visitAcyclic20(const Event &e);
	bool visitAcyclic21(const Event &e);
	bool visitAcyclic22(const Event &e);
	bool visitAcyclic23(const Event &e);
	bool visitAcyclic24(const Event &e);
	bool visitAcyclic25(const Event &e);
	bool visitAcyclic26(const Event &e);
	bool visitAcyclic27(const Event &e);
	bool visitAcyclic28(const Event &e);
	bool visitAcyclic29(const Event &e);
	bool visitAcyclic30(const Event &e);

	bool isAcyclic(const Event &e);

	std::vector<NodeCountStatus> visitedAcyclic0;
	std::vector<NodeCountStatus> visitedAcyclic1;
	std::vector<NodeCountStatus> visitedAcyclic2;
	std::vector<NodeCountStatus> visitedAcyclic3;
	std::vector<NodeCountStatus> visitedAcyclic4;
	std::vector<NodeCountStatus> visitedAcyclic5;
	std::vector<NodeCountStatus> visitedAcyclic6;
	std::vector<NodeCountStatus> visitedAcyclic7;
	std::vector<NodeCountStatus> visitedAcyclic8;
	std::vector<NodeCountStatus> visitedAcyclic9;
	std::vector<NodeCountStatus> visitedAcyclic10;
	std::vector<NodeCountStatus> visitedAcyclic11;
	std::vector<NodeCountStatus> visitedAcyclic12;
	std::vector<NodeCountStatus> visitedAcyclic13;
	std::vector<NodeCountStatus> visitedAcyclic14;
	std::vector<NodeCountStatus> visitedAcyclic15;
	std::vector<NodeCountStatus> visitedAcyclic16;
	std::vector<NodeCountStatus> visitedAcyclic17;
	std::vector<NodeCountStatus> visitedAcyclic18;
	std::vector<NodeCountStatus> visitedAcyclic19;
	std::vector<NodeCountStatus> visitedAcyclic20;
	std::vector<NodeCountStatus> visitedAcyclic21;
	std::vector<NodeCountStatus> visitedAcyclic22;
	std::vector<NodeCountStatus> visitedAcyclic23;
	std::vector<NodeCountStatus> visitedAcyclic24;
	std::vector<NodeCountStatus> visitedAcyclic25;
	std::vector<NodeCountStatus> visitedAcyclic26;
	std::vector<NodeCountStatus> visitedAcyclic27;
	std::vector<NodeCountStatus> visitedAcyclic28;
	std::vector<NodeCountStatus> visitedAcyclic29;
	std::vector<NodeCountStatus> visitedAcyclic30;

	unsigned visitedAccepting = 0;
	std::vector<VSet<Event>> calculated;

	ExecutionGraph &g;

	ExecutionGraph &getGraph() { return g; }
};

#endif /* __RC11_CHECKER_HPP__ */
