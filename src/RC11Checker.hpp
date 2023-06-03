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
#include "VerificationError.hpp"
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

	std::vector<VSet<Event>> calculateSaved(const EventLabel *lab);
	std::vector<View> calculateViews(const EventLabel *lab);
	bool isDepTracking();
	bool isConsistent(const EventLabel *lab);
	VerificationError checkErrors(const EventLabel *lab);
	bool isRecoveryValid(const EventLabel *lab);
	std::unique_ptr<VectorClock> getPPoRfBefore(const EventLabel *lab);
	const View &getHbView(const EventLabel *lab);
	std::vector<Event> getCoherentStores(SAddr addr, Event read);
	std::vector<Event> getCoherentRevisits(const WriteLabel *sLab, const VectorClock &pporf);
	llvm::iterator_range<ExecutionGraph::co_iterator>
	getCoherentPlacings(SAddr addr, Event store, bool isRMW);

private:
	bool isWriteRfBefore(Event a, Event b);
	std::vector<Event> getInitRfsAtLoc(SAddr addr);
	bool isHbOptRfBefore(const Event e, const Event write);
	ExecutionGraph::co_iterator splitLocMOBefore(SAddr addr, Event e);
	ExecutionGraph::co_iterator splitLocMOAfterHb(SAddr addr, const Event read);
	ExecutionGraph::co_iterator splitLocMOAfter(SAddr addr, const Event e);
	std::vector<Event> getMOOptRfAfter(const WriteLabel *sLab);
	std::vector<Event> getMOInvOptRfAfter(const WriteLabel *sLab);

	void visitCalc0_0(const EventLabel *lab, View &calcRes);
	void visitCalc0_1(const EventLabel *lab, View &calcRes);
	void visitCalc0_2(const EventLabel *lab, View &calcRes);
	void visitCalc0_3(const EventLabel *lab, View &calcRes);
	void visitCalc0_4(const EventLabel *lab, View &calcRes);
	void visitCalc0_5(const EventLabel *lab, View &calcRes);
	void visitCalc0_6(const EventLabel *lab, View &calcRes);

	View calculate0(const EventLabel *lab);

	static inline thread_local std::vector<NodeStatus> visitedCalc0_0;
	static inline thread_local std::vector<NodeStatus> visitedCalc0_1;
	static inline thread_local std::vector<NodeStatus> visitedCalc0_2;
	static inline thread_local std::vector<NodeStatus> visitedCalc0_3;
	static inline thread_local std::vector<NodeStatus> visitedCalc0_4;
	static inline thread_local std::vector<NodeStatus> visitedCalc0_5;
	static inline thread_local std::vector<NodeStatus> visitedCalc0_6;

	void visitInclusionLHS0_0(const EventLabel *lab);
	void visitInclusionLHS0_1(const EventLabel *lab);

	bool checkInclusion0(const EventLabel *lab);

	static inline thread_local std::vector<NodeStatus> visitedInclusionLHS0_0;
	static inline thread_local std::vector<NodeStatus> visitedInclusionLHS0_1;

	static inline thread_local std::vector<bool> lhsAccept0;
	static inline thread_local std::vector<bool> rhsAccept0;

	bool visitInclusionLHS1_0(const EventLabel *lab, const View &v);
	bool visitInclusionLHS1_1(const EventLabel *lab, const View &v);

	bool checkInclusion1(const EventLabel *lab);

	static inline thread_local std::vector<NodeStatus> visitedInclusionLHS1_0;
	static inline thread_local std::vector<NodeStatus> visitedInclusionLHS1_1;
	static inline thread_local std::vector<NodeStatus> visitedInclusionRHS1_0;
	static inline thread_local std::vector<NodeStatus> visitedInclusionRHS1_1;

	static inline thread_local std::vector<bool> lhsAccept1;
	static inline thread_local std::vector<bool> rhsAccept1;

	void visitInclusionLHS2_0(const EventLabel *lab);
	void visitInclusionLHS2_1(const EventLabel *lab);

	bool checkInclusion2(const EventLabel *lab);

	static inline thread_local std::vector<NodeStatus> visitedInclusionLHS2_0;
	static inline thread_local std::vector<NodeStatus> visitedInclusionLHS2_1;

	static inline thread_local std::vector<bool> lhsAccept2;
	static inline thread_local std::vector<bool> rhsAccept2;

	bool visitInclusionLHS3_0(const EventLabel *lab, const View &v);
	bool visitInclusionLHS3_1(const EventLabel *lab, const View &v);

	bool checkInclusion3(const EventLabel *lab);

	static inline thread_local std::vector<NodeStatus> visitedInclusionLHS3_0;
	static inline thread_local std::vector<NodeStatus> visitedInclusionLHS3_1;
	static inline thread_local std::vector<NodeStatus> visitedInclusionRHS3_0;
	static inline thread_local std::vector<NodeStatus> visitedInclusionRHS3_1;

	static inline thread_local std::vector<bool> lhsAccept3;
	static inline thread_local std::vector<bool> rhsAccept3;

	void visitInclusionLHS4_0(const EventLabel *lab);
	void visitInclusionLHS4_1(const EventLabel *lab);

	bool checkInclusion4(const EventLabel *lab);

	static inline thread_local std::vector<NodeStatus> visitedInclusionLHS4_0;
	static inline thread_local std::vector<NodeStatus> visitedInclusionLHS4_1;

	static inline thread_local std::vector<bool> lhsAccept4;
	static inline thread_local std::vector<bool> rhsAccept4;

	bool visitInclusionLHS5_0(const EventLabel *lab, const View &v);
	bool visitInclusionLHS5_1(const EventLabel *lab, const View &v);

	bool checkInclusion5(const EventLabel *lab);

	static inline thread_local std::vector<NodeStatus> visitedInclusionLHS5_0;
	static inline thread_local std::vector<NodeStatus> visitedInclusionLHS5_1;
	static inline thread_local std::vector<NodeStatus> visitedInclusionRHS5_0;
	static inline thread_local std::vector<NodeStatus> visitedInclusionRHS5_1;

	static inline thread_local std::vector<bool> lhsAccept5;
	static inline thread_local std::vector<bool> rhsAccept5;

	bool visitAcyclic0(const EventLabel *lab);
	bool visitAcyclic1(const EventLabel *lab);
	bool visitAcyclic2(const EventLabel *lab);
	bool visitAcyclic3(const EventLabel *lab);
	bool visitAcyclic4(const EventLabel *lab);
	bool visitAcyclic5(const EventLabel *lab);
	bool visitAcyclic6(const EventLabel *lab);
	bool visitAcyclic7(const EventLabel *lab);
	bool visitAcyclic8(const EventLabel *lab);
	bool visitAcyclic9(const EventLabel *lab);
	bool visitAcyclic10(const EventLabel *lab);
	bool visitAcyclic11(const EventLabel *lab);
	bool visitAcyclic12(const EventLabel *lab);
	bool visitAcyclic13(const EventLabel *lab);
	bool visitAcyclic14(const EventLabel *lab);
	bool visitAcyclic15(const EventLabel *lab);

	bool isAcyclic(const EventLabel *lab);

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
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic12;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic13;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic14;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic15;

	uint16_t visitedAccepting = 0;

	bool isRecAcyclic(const EventLabel *lab);


	uint16_t visitedRecAccepting = 0;
	void visitPPoRf0(const EventLabel *lab, View &pporf);

	View calcPPoRfBefore(const EventLabel *lab);

	static inline thread_local std::vector<NodeStatus> visitedPPoRf0;

	std::vector<VSet<Event>> saved;
	std::vector<View> views;

	ExecutionGraph &g;

	static inline thread_local unsigned int maxSize = 0;

	ExecutionGraph &getGraph() { return g; }
	const ExecutionGraph &getGraph() const { return g; }
};

#endif /* __RC11_CHECKER_HPP__ */
