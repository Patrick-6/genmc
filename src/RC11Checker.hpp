/* This file is generated automatically by Kater -- do not edit. */

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

	std::vector<NodeStatus> visitedAcyclic0;
	std::vector<NodeStatus> visitedAcyclic1;
	std::vector<NodeStatus> visitedAcyclic2;
	std::vector<NodeStatus> visitedAcyclic3;
	std::vector<NodeStatus> visitedAcyclic4;
	std::vector<NodeStatus> visitedAcyclic5;
	std::vector<NodeStatus> visitedAcyclic6;
	std::vector<NodeStatus> visitedAcyclic7;
	std::vector<NodeStatus> visitedAcyclic8;
	std::vector<NodeStatus> visitedAcyclic9;
	std::vector<NodeStatus> visitedAcyclic10;
	std::vector<NodeStatus> visitedAcyclic11;
	std::vector<NodeStatus> visitedAcyclic12;
	std::vector<NodeStatus> visitedAcyclic13;
	std::vector<NodeStatus> visitedAcyclic14;
	std::vector<NodeStatus> visitedAcyclic15;
	std::vector<NodeStatus> visitedAcyclic16;
	std::vector<NodeStatus> visitedAcyclic17;
	std::vector<NodeStatus> visitedAcyclic18;
	std::vector<NodeStatus> visitedAcyclic19;
	std::vector<NodeStatus> visitedAcyclic20;
	std::vector<NodeStatus> visitedAcyclic21;
	std::vector<NodeStatus> visitedAcyclic22;
	std::vector<NodeStatus> visitedAcyclic23;
	std::vector<NodeStatus> visitedAcyclic24;
	std::vector<NodeStatus> visitedAcyclic25;
	std::vector<NodeStatus> visitedAcyclic26;
	std::vector<NodeStatus> visitedAcyclic27;
	std::vector<NodeStatus> visitedAcyclic28;
	std::vector<NodeStatus> visitedAcyclic29;
	std::vector<NodeStatus> visitedAcyclic30;

	int visitedAccepting = 0;
	std::vector<VSet<Event>> calculated;

	ExecutionGraph &g;

	ExecutionGraph &getGraph() { return g; }
};

#endif /* __RC11_CHECKER_HPP__ */
