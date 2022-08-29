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

#include "config.h"
#include "ExecutionGraph.hpp"
#include "GraphIterators.hpp"
#include "MaximalIterator.hpp"
#include "PersistencyChecker.hpp"
#include "VSet.hpp"
#include <cstdint>
#include <vector>

class RC11Checker {

private:
	enum class NodeStatus : unsigned char { unseen, entered, left };

	struct NodeCountStatus {
		NodeCountStatus() = default;
		NodeCountStatus(uint16_t c, NodeStatus s) : count(c), status(s) {}
		uint16_t count = 0;
		NodeStatus status = NodeStatus::unseen;
	};

public:
	RC11Checker(ExecutionGraph &g) : g(g) {}

	std::vector<VSet<Event>> calculateSaved(const Event &e);
	std::vector<View> calculateViews(const Event &e);
	bool isConsistent(const Event &e);
	bool isRecoveryValid(const Event &e);
	std::unique_ptr<VectorClock> getPPoRfBefore(const Event &e);

private:
	void visitCalc0_0(const Event &e, VSet<Event> &calcRes);
	void visitCalc0_1(const Event &e, VSet<Event> &calcRes);
	void visitCalc0_2(const Event &e, VSet<Event> &calcRes);
	void visitCalc0_3(const Event &e, VSet<Event> &calcRes);
	void visitCalc0_4(const Event &e, VSet<Event> &calcRes);

	VSet<Event> calculate0(const Event &e);

	static inline thread_local std::vector<NodeStatus> visitedCalc0_0;
	static inline thread_local std::vector<NodeStatus> visitedCalc0_1;
	static inline thread_local std::vector<NodeStatus> visitedCalc0_2;
	static inline thread_local std::vector<NodeStatus> visitedCalc0_3;
	static inline thread_local std::vector<NodeStatus> visitedCalc0_4;

	void visitCalc1_0(const Event &e, VSet<Event> &calcRes);
	void visitCalc1_1(const Event &e, VSet<Event> &calcRes);
	void visitCalc1_2(const Event &e, VSet<Event> &calcRes);
	void visitCalc1_3(const Event &e, VSet<Event> &calcRes);
	void visitCalc1_4(const Event &e, VSet<Event> &calcRes);

	VSet<Event> calculate1(const Event &e);

	static inline thread_local std::vector<NodeStatus> visitedCalc1_0;
	static inline thread_local std::vector<NodeStatus> visitedCalc1_1;
	static inline thread_local std::vector<NodeStatus> visitedCalc1_2;
	static inline thread_local std::vector<NodeStatus> visitedCalc1_3;
	static inline thread_local std::vector<NodeStatus> visitedCalc1_4;

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

	bool isAcyclic(const Event &e);

	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic0;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic1;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic2;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic3;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic4;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic5;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic6;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic7;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic8;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic9;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic10;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic11;

	uint16_t visitedAccepting = 0;

	bool isRecAcyclic(const Event &e);


	uint16_t visitedRecAccepting = 0;
	void visitPPoRf0(const Event &e, View &pporf);

	View calcPPoRfBefore(const Event &e);

	static inline thread_local std::vector<NodeStatus> visitedPPoRf0;

	std::vector<VSet<Event>> saved;
	std::vector<View> views;

	ExecutionGraph &g;

	ExecutionGraph &getGraph() { return g; }
};

#endif /* __RC11_CHECKER_HPP__ */
