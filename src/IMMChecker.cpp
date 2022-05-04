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

#include "IMMChecker.hpp"

void IMMChecker::visitCalc0_0(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_0[lab->getStamp()] = NodeStatus::entered;
	calcRes.insert(e);
	for (const auto &p : lab->calculated(0)) {
		calcRes.erase(p);
		visitedCalc0_1[g.getEventLabel(p)->getStamp()] = NodeStatus::left;
		visitedCalc0_4[g.getEventLabel(p)->getStamp()] = NodeStatus::left;
	}
	for (auto &p : lab->calculated(0)) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	visitedCalc0_0[lab->getStamp()] = NodeStatus::left;
}

void IMMChecker::visitCalc0_1(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_1[lab->getStamp()] = NodeStatus::entered;
	if (auto p = lab->getPos(); true)if (true && g.getEventLabel(p)->isAtLeastRelease()) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	visitedCalc0_1[lab->getStamp()] = NodeStatus::left;
}

void IMMChecker::visitCalc0_2(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_2[lab->getStamp()] = NodeStatus::entered;
	if (auto p = lab->getPos(); true)if (true && llvm::isa<FenceLabel>(g.getEventLabel(p))) {
		auto status = visitedCalc0_1[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_1(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_2[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_2(p, calcRes);
	}
	visitedCalc0_2[lab->getStamp()] = NodeStatus::left;
}

void IMMChecker::visitCalc0_3(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_3[lab->getStamp()] = NodeStatus::entered;
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto status = visitedCalc0_1[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_1(p, calcRes);
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto status = visitedCalc0_4[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_4(p, calcRes);
	}
	visitedCalc0_3[lab->getStamp()] = NodeStatus::left;
}

void IMMChecker::visitCalc0_4(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_4[lab->getStamp()] = NodeStatus::entered;
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_2[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_2(p, calcRes);
	}
	if (true && g.isRMWStore(lab))for (auto &p : po_imm_preds(g, lab->getPos()))if (true && g.isRMWLoad(g.getEventLabel(p))) {
		auto status = visitedCalc0_3[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_3(p, calcRes);
	}
	visitedCalc0_4[lab->getStamp()] = NodeStatus::left;
}

VSet<Event> IMMChecker::calculate0(const Event &e)
{
	VSet<Event> calcRes;
	visitedCalc0_0.clear();
	visitedCalc0_0.resize(g.getMaxStamp() + 1, NodeStatus::unseen);
	visitedCalc0_1.clear();
	visitedCalc0_1.resize(g.getMaxStamp() + 1, NodeStatus::unseen);
	visitedCalc0_2.clear();
	visitedCalc0_2.resize(g.getMaxStamp() + 1, NodeStatus::unseen);
	visitedCalc0_3.clear();
	visitedCalc0_3.resize(g.getMaxStamp() + 1, NodeStatus::unseen);
	visitedCalc0_4.clear();
	visitedCalc0_4.resize(g.getMaxStamp() + 1, NodeStatus::unseen);

	getGraph().getEventLabel(e)->setCalculated({{}, {}, });
	visitCalc0_1(e, calcRes);
	visitCalc0_4(e, calcRes);
	return calcRes;
}
void IMMChecker::visitCalc1_0(const Event &e, View &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc1_0[lab->getStamp()] = NodeStatus::entered;
	calcRes.update(lab->view(0));
	calcRes.updateIdx(lab->getPos());
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc1_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_0(p, calcRes);
	}
	if (auto p = lab->getPos(); true)if (true && g.getEventLabel(p)->isAtLeastAcquire() && llvm::isa<FenceLabel>(g.getEventLabel(p))) {
		auto status = visitedCalc1_5[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_5(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc1_3[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_3(p, calcRes);
	}
	visitedCalc1_0[lab->getStamp()] = NodeStatus::left;
}

void IMMChecker::visitCalc1_1(const Event &e, View &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc1_1[lab->getStamp()] = NodeStatus::entered;
	for (auto &p : lab->calculated(0)) {
		auto status = visitedCalc1_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_0(p, calcRes);
	}
	for (auto &p : lab->calculated(0)) {
		auto status = visitedCalc1_3[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_3(p, calcRes);
	}
	visitedCalc1_1[lab->getStamp()] = NodeStatus::left;
}

void IMMChecker::visitCalc1_2(const Event &e, View &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc1_2[lab->getStamp()] = NodeStatus::entered;
	if (true && lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos())) {
		auto status = visitedCalc1_1[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_1(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc1_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_0(p, calcRes);
	}
	if (auto p = lab->getPos(); true)if (true && g.getEventLabel(p)->isAtLeastAcquire() && llvm::isa<FenceLabel>(g.getEventLabel(p))) {
		auto status = visitedCalc1_5[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_5(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc1_3[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_3(p, calcRes);
	}
	visitedCalc1_2[lab->getStamp()] = NodeStatus::left;
}

void IMMChecker::visitCalc1_3(const Event &e, View &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc1_3[lab->getStamp()] = NodeStatus::entered;
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc1_2[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_2(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc1_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_0(p, calcRes);
	}
	if (auto p = lab->getPos(); true)if (true && g.getEventLabel(p)->isAtLeastAcquire() && llvm::isa<FenceLabel>(g.getEventLabel(p))) {
		auto status = visitedCalc1_5[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_5(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc1_3[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_3(p, calcRes);
	}
	visitedCalc1_3[lab->getStamp()] = NodeStatus::left;
}

void IMMChecker::visitCalc1_4(const Event &e, View &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc1_4[lab->getStamp()] = NodeStatus::entered;
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto status = visitedCalc1_1[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_1(p, calcRes);
	}
	visitedCalc1_4[lab->getStamp()] = NodeStatus::left;
}

void IMMChecker::visitCalc1_5(const Event &e, View &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc1_5[lab->getStamp()] = NodeStatus::entered;
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc1_4[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_4(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc1_5[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_5(p, calcRes);
	}
	visitedCalc1_5[lab->getStamp()] = NodeStatus::left;
}

View IMMChecker::calculate1(const Event &e)
{
	View calcRes;
calcRes.updateIdx(e.prev());
	visitedCalc1_0.clear();
	visitedCalc1_0.resize(g.getMaxStamp() + 1, NodeStatus::unseen);
	visitedCalc1_1.clear();
	visitedCalc1_1.resize(g.getMaxStamp() + 1, NodeStatus::unseen);
	visitedCalc1_2.clear();
	visitedCalc1_2.resize(g.getMaxStamp() + 1, NodeStatus::unseen);
	visitedCalc1_3.clear();
	visitedCalc1_3.resize(g.getMaxStamp() + 1, NodeStatus::unseen);
	visitedCalc1_4.clear();
	visitedCalc1_4.resize(g.getMaxStamp() + 1, NodeStatus::unseen);
	visitedCalc1_5.clear();
	visitedCalc1_5.resize(g.getMaxStamp() + 1, NodeStatus::unseen);

	getGraph().getEventLabel(e)->setViews({{}, });
	visitCalc1_3(e, calcRes);
	return calcRes;
}
std::vector<VSet<Event>> IMMChecker::calculateSaved(const Event &e)
{
	saved.push_back(calculate0(e));
	return std::move(saved);
}

std::vector<View> IMMChecker::calculateViews(const Event &e)
{
	views.push_back(calculate1(e));
	return std::move(views);
}

bool IMMChecker::visitAcyclic0(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	++visitedAccepting;
	visitedAcyclic0[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	if (auto p = lab->getPos(); true)if (true && g.getEventLabel(p)->isAtLeastRelease() && llvm::isa<WriteLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = lab->getPos(); true)if (true && llvm::isa<FenceLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = lab->getPos(); true)if (true && g.getEventLabel(p)->isSC() && llvm::isa<FenceLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = lab->getPos(); true)if (true && llvm::isa<WriteLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic7[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic7(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = lab->getPos(); true)if (true && llvm::isa<WriteLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic6[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = lab->getPos(); true)if (true && llvm::isa<WriteLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic4[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic4(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic2[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic2(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rfe_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : detour_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	--visitedAccepting;
	visitedAcyclic0[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic1(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic1[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic1[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic2(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic2[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic2[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic2(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = lab->getPos(); true)if (true && g.getEventLabel(p)->isAtLeastAcquire() && llvm::isa<ReadLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = lab->getPos(); true)if (true && llvm::isa<FenceLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic2[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic3(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic3[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	if (auto p = lab->getPos(); true)if (true && g.getEventLabel(p)->isAtLeastRelease() && llvm::isa<WriteLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic3[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic4(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic4[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : poloc_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic3[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic3(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : poloc_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic4[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic4(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic4[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic5(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic5[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic7[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic7(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic6[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = lab->getPos(); true)if (true && llvm::isa<ReadLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic5[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic6(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic6[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic7[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic7(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : data_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic7[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic7(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rfi_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic7[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic7(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic6[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : data_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic6[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rfi_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic6[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : data_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rfi_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic6[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic7(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic7[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic7[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic7(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic7[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic7(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : addr_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic7[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic7(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic6[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : addr_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic6[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : addr_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic7[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic8(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic8[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	if (auto p = lab->getPos(); true)if (true && g.getEventLabel(p)->isSC() && llvm::isa<FenceLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic8[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic9(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic9[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (true && lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic11[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic11(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : maximals(lab->view(0))) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic9[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic9(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic9[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic9(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic9[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic10(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic10[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : maximals(lab->view(0))) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic10[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic11(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic11[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic10[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic10(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic11[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic12(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic12[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic9[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic9(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic9[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic9(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic9[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic9(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic12[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic13(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic13[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	if (true && lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : maximals(lab->view(0))) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic13[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic14(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic14[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : maximals(lab->view(0))) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic9[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic9(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic9[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic9(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic14[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic15(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic15[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic14[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic14(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic15[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::isAcyclic(const Event &e)
{
	visitedAccepting = 0;
	visitedAcyclic0.clear();
	visitedAcyclic0.resize(g.getMaxStamp() + 1);
	visitedAcyclic1.clear();
	visitedAcyclic1.resize(g.getMaxStamp() + 1);
	visitedAcyclic2.clear();
	visitedAcyclic2.resize(g.getMaxStamp() + 1);
	visitedAcyclic3.clear();
	visitedAcyclic3.resize(g.getMaxStamp() + 1);
	visitedAcyclic4.clear();
	visitedAcyclic4.resize(g.getMaxStamp() + 1);
	visitedAcyclic5.clear();
	visitedAcyclic5.resize(g.getMaxStamp() + 1);
	visitedAcyclic6.clear();
	visitedAcyclic6.resize(g.getMaxStamp() + 1);
	visitedAcyclic7.clear();
	visitedAcyclic7.resize(g.getMaxStamp() + 1);
	visitedAcyclic8.clear();
	visitedAcyclic8.resize(g.getMaxStamp() + 1);
	visitedAcyclic9.clear();
	visitedAcyclic9.resize(g.getMaxStamp() + 1);
	visitedAcyclic10.clear();
	visitedAcyclic10.resize(g.getMaxStamp() + 1);
	visitedAcyclic11.clear();
	visitedAcyclic11.resize(g.getMaxStamp() + 1);
	visitedAcyclic12.clear();
	visitedAcyclic12.resize(g.getMaxStamp() + 1);
	visitedAcyclic13.clear();
	visitedAcyclic13.resize(g.getMaxStamp() + 1);
	visitedAcyclic14.clear();
	visitedAcyclic14.resize(g.getMaxStamp() + 1);
	visitedAcyclic15.clear();
	visitedAcyclic15.resize(g.getMaxStamp() + 1);
	return true
		&& visitAcyclic0(e)
		&& visitAcyclic1(e)
		&& visitAcyclic2(e)
		&& visitAcyclic3(e)
		&& visitAcyclic4(e)
		&& visitAcyclic5(e)
		&& visitAcyclic6(e)
		&& visitAcyclic7(e)
		&& visitAcyclic8(e)
		&& visitAcyclic9(e)
		&& visitAcyclic10(e)
		&& visitAcyclic11(e)
		&& visitAcyclic12(e)
		&& visitAcyclic13(e)
		&& visitAcyclic14(e)
		&& visitAcyclic15(e);
}

bool IMMChecker::isConsistent(const Event &e)
{
	return isAcyclic(e);
}

