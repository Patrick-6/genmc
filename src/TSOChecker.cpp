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

#include "TSOChecker.hpp"

void TSOChecker::visitCalc0_0(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_0[lab->getStamp().get()] = NodeStatus::entered;
	calcRes.insert(e);
	for (const auto &p : lab->calculated(0)) {
		calcRes.erase(p);
	}
	for (auto &p : lab->calculated(0)) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	visitedCalc0_0[lab->getStamp().get()] = NodeStatus::left;
}

void TSOChecker::visitCalc0_1(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_1[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p))) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (g.isRMWStore(g.getEventLabel(p))) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p))) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_1[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_1(p, calcRes);
	}
	visitedCalc0_1[lab->getStamp().get()] = NodeStatus::left;
}

void TSOChecker::visitCalc0_2(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_2[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_2(p, calcRes);
	}
	visitedCalc0_2[lab->getStamp().get()] = NodeStatus::left;
}

void TSOChecker::visitCalc0_3(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_3[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p))) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (g.isRMWStore(g.getEventLabel(p))) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p))) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	if (llvm::isa<FenceLabel>(lab))for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	if (lab->isSC())for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	if (llvm::isa<FenceLabel>(lab))for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_2(p, calcRes);
	}
	if (lab->isSC())for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_2(p, calcRes);
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_2(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_1[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_1(p, calcRes);
	}
	visitedCalc0_3[lab->getStamp().get()] = NodeStatus::left;
}

VSet<Event> TSOChecker::calculate0(const Event &e)
{
	VSet<Event> calcRes;
	visitedCalc0_0.clear();
	visitedCalc0_0.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedCalc0_1.clear();
	visitedCalc0_1.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedCalc0_2.clear();
	visitedCalc0_2.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedCalc0_3.clear();
	visitedCalc0_3.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);

	getGraph().getEventLabel(e)->setCalculated({{}, });
	visitCalc0_3(e, calcRes);
	return calcRes;
}
std::vector<VSet<Event>> TSOChecker::calculateSaved(const Event &e)
{
	saved.push_back(calculate0(e));
	return std::move(saved);
}

std::vector<View> TSOChecker::calculateViews(const Event &e)
{
	return std::move(views);
}

bool TSOChecker::visitAcyclic0(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	++visitedAccepting;
	visitedAcyclic0[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rfe_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	--visitedAccepting;
	visitedAcyclic0[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool TSOChecker::isAcyclic(const Event &e)
{
	visitedAccepting = 0;
	visitedAcyclic0.clear();
	visitedAcyclic0.resize(g.getMaxStamp().get() + 1);
	return true
		&& visitAcyclic0(e);
}

bool TSOChecker::isConsistent(const Event &e)
{
	return isAcyclic(e);
}

bool TSOChecker::isRecAcyclic(const Event &e)
{
	visitedRecAccepting = 0;
	return true;
}

bool TSOChecker::isRecoveryValid(const Event &e)
{
	return isRecAcyclic(e);
}

void TSOChecker::visitPPoRf0(const Event &e, DepView &pporf)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedPPoRf0[lab->getStamp().get()] = NodeStatus::entered;
	pporf.updateIdx(e);
	for (auto &p : lab->calculated(0)) {
		auto status = visitedPPoRf0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf0(p, pporf);
	}
	for (auto &p : tc_preds(g, lab->getPos())) {
		auto status = visitedPPoRf0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf0(p, pporf);
	}
	for (auto &p : tj_preds(g, lab->getPos())) {
		auto status = visitedPPoRf0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf0(p, pporf);
	}
	for (auto &p : rfe_preds(g, lab->getPos())) {
		auto status = visitedPPoRf0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf0(p, pporf);
	}
	visitedPPoRf0[lab->getStamp().get()] = NodeStatus::left;
}

DepView TSOChecker::calcPPoRfBefore(const Event &e)
{
	DepView pporf;
	pporf.updateIdx(e);
	visitedPPoRf0.clear();
	visitedPPoRf0.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);

	visitPPoRf0(e, pporf);
	return pporf;
}
std::unique_ptr<VectorClock> TSOChecker::getPPoRfBefore(const Event &e)
{
	return LLVM_MAKE_UNIQUE<DepView>(calcPPoRfBefore(e));
}
