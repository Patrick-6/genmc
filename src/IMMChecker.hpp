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

#ifndef __IMM_CHECKER_HPP__
#define __IMM_CHECKER_HPP__

#include "config.h"
#include "ExecutionGraph.hpp"
#include "GraphIterators.hpp"
#include "MaximalIterator.hpp"
#include "PersistencyChecker.hpp"
#include "VerificationError.hpp"
#include "VSet.hpp"
#include <cstdint>
#include <vector>

class IMMChecker {

private:
	enum class NodeStatus : unsigned char { unseen, entered, left };

	struct NodeCountStatus {
		NodeCountStatus() = default;
		NodeCountStatus(uint16_t c, NodeStatus s) : count(c), status(s) {}
		uint16_t count = 0;
		NodeStatus status = NodeStatus::unseen;
	};

public:
	IMMChecker(ExecutionGraph &g) : g(g) {}

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
	bool visitAcyclic16(const EventLabel *lab);
	bool visitAcyclic17(const EventLabel *lab);
	bool visitAcyclic18(const EventLabel *lab);
	bool visitAcyclic19(const EventLabel *lab);
	bool visitAcyclic20(const EventLabel *lab);
	bool visitAcyclic21(const EventLabel *lab);
	bool visitAcyclic22(const EventLabel *lab);
	bool visitAcyclic23(const EventLabel *lab);
	bool visitAcyclic24(const EventLabel *lab);
	bool visitAcyclic25(const EventLabel *lab);
	bool visitAcyclic26(const EventLabel *lab);
	bool visitAcyclic27(const EventLabel *lab);
	bool visitAcyclic28(const EventLabel *lab);

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
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic16;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic17;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic18;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic19;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic20;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic21;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic22;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic23;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic24;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic25;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic26;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic27;
	static inline thread_local std::vector<NodeCountStatus> visitedAcyclic28;

	uint16_t visitedAccepting = 0;

	bool isRecAcyclic(const EventLabel *lab);


	uint16_t visitedRecAccepting = 0;
	void visitPPoRf0(const EventLabel *lab, DepView &pporf);
	void visitPPoRf1(const EventLabel *lab, DepView &pporf);
	void visitPPoRf2(const EventLabel *lab, DepView &pporf);
	void visitPPoRf3(const EventLabel *lab, DepView &pporf);
	void visitPPoRf4(const EventLabel *lab, DepView &pporf);
	void visitPPoRf5(const EventLabel *lab, DepView &pporf);
	void visitPPoRf6(const EventLabel *lab, DepView &pporf);

	DepView calcPPoRfBefore(const EventLabel *lab);

	static inline thread_local std::vector<NodeStatus> visitedPPoRf0;
	static inline thread_local std::vector<NodeStatus> visitedPPoRf1;
	static inline thread_local std::vector<NodeStatus> visitedPPoRf2;
	static inline thread_local std::vector<NodeStatus> visitedPPoRf3;
	static inline thread_local std::vector<NodeStatus> visitedPPoRf4;
	static inline thread_local std::vector<NodeStatus> visitedPPoRf5;
	static inline thread_local std::vector<NodeStatus> visitedPPoRf6;

	std::vector<VSet<Event>> saved;
	std::vector<View> views;

	ExecutionGraph &g;

	ExecutionGraph &getGraph() { return g; }
	const ExecutionGraph &getGraph() const { return g; }
};

#endif /* __IMM_CHECKER_HPP__ */
