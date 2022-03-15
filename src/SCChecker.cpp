/* This file is generated automatically by Kater -- do not edit. */

#include "SCChecker.hpp"

bool SCChecker::visitAcyclic0(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic0[lab->getStamp()] = NodeStatus::entered;
	++visitedAccepting;
	for (auto &p : po_imm_preds(g, e)) {
		auto status = visitedAcyclic0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (status == NodeStatus::entered && visitedAccepting)
			return false;
	}
	for (auto &p : rf_preds(g, e)) {
		auto status = visitedAcyclic0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (status == NodeStatus::entered && visitedAccepting)
			return false;
	}
	for (auto &p : co_preds(g, e)) {
		auto status = visitedAcyclic0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (status == NodeStatus::entered && visitedAccepting)
			return false;
	}
	for (auto &p : fr_init_preds(g, e)) {
		auto status = visitedAcyclic0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (status == NodeStatus::entered && visitedAccepting)
			return false;
	}
	for (auto &p : co_preds(g, e)) {
		auto status = visitedAcyclic1[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (status == NodeStatus::entered && visitedAccepting)
			return false;
	}
	--visitedAccepting;
	visitedAcyclic0[lab->getStamp()] = NodeStatus::left;
	return true;
}

bool SCChecker::visitAcyclic1(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic1[lab->getStamp()] = NodeStatus::entered;
	for (auto &p : rf_succs(g, e)) {
		auto status = visitedAcyclic0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (status == NodeStatus::entered && visitedAccepting)
			return false;
	}
	for (auto &p : fr_init_preds(g, e)) {
		auto status = visitedAcyclic0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (status == NodeStatus::entered && visitedAccepting)
			return false;
	}
	visitedAcyclic1[lab->getStamp()] = NodeStatus::left;
	return true;
}

bool SCChecker::isConsistent(const Event &e)
{
	visitedAccepting = 0;
	visitedAcyclic0.clear();
	visitedAcyclic0.resize(g.getMaxStamp() + 1, NodeStatus::unseen);
	visitedAcyclic1.clear();
	visitedAcyclic1.resize(g.getMaxStamp() + 1, NodeStatus::unseen);
	return true
		&& visitAcyclic0(e)
		&& visitAcyclic1(e);
}

