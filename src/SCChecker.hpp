/* This file is generated automatically by Kater -- do not edit. */

#ifndef __SC_CHECKER_HPP__
#define __SC_CHECKER_HPP__

#include "ExecutionGraph.hpp"
#include "GraphIterators.hpp"
#include "PersistencyChecker.hpp"
#include "VSet.hpp"
#include <vector>

class SCChecker {

private:
	enum class NodeStatus { unseen, entered, left };
public:

	SCChecker(ExecutionGraph &g) : g(g) {}

	bool isConsistent(const Event &e);

private:

	bool visitAcyclic0(const Event &e);
	bool visitAcyclic1(const Event &e);

	std::vector<NodeStatus> visitedAcyclic0;
	std::vector<NodeStatus> visitedAcyclic1;

	ExecutionGraph &getGraph() { return g; }

	int visitedAccepting = 0;
	ExecutionGraph &g;
};

#endif /* __SC_CHECKER_HPP__ */
