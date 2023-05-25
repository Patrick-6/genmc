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

void IMMChecker::visitCalc0_0(const Event &e, View &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_0[lab->getStamp().get()] = NodeStatus::entered;
	calcRes.update(lab->view(0));
	calcRes.updateIdx(lab->getPos());
	visitedCalc0_0[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitCalc0_1(const Event &e, View &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_1[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_3[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_3(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_1[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_1(p, calcRes);
	}
	visitedCalc0_1[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitCalc0_2(const Event &e, View &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_2[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto status = visitedCalc0_1[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_1(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isAtLeastAcquire() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto status = visitedCalc0_6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_6(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_2(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isAtLeastAcquire() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	visitedCalc0_2[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitCalc0_3(const Event &e, View &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_3[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastRelease()) {
		auto status = visitedCalc0_6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_6(p, calcRes);
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto status = visitedCalc0_2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_2(p, calcRes);
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto status = visitedCalc0_4[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_4(p, calcRes);
	}
	for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastRelease()) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	visitedCalc0_3[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitCalc0_4(const Event &e, View &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_4[lab->getStamp().get()] = NodeStatus::entered;
	if (g.isRMWStore(lab))for (auto &p : po_imm_preds(g, lab->getPos()))if (g.isRMWLoad(g.getEventLabel(p))) {
		auto status = visitedCalc0_3[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_3(p, calcRes);
	}
	visitedCalc0_4[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitCalc0_5(const Event &e, View &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_5[lab->getStamp().get()] = NodeStatus::entered;
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastRelease()) {
		auto status = visitedCalc0_6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_6(p, calcRes);
	}
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos())) {
		auto status = visitedCalc0_2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_2(p, calcRes);
	}
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos())) {
		auto status = visitedCalc0_4[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_4(p, calcRes);
	}
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastRelease()) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	visitedCalc0_5[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitCalc0_6(const Event &e, View &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_6[lab->getStamp().get()] = NodeStatus::entered;
	if (llvm::isa<FenceLabel>(lab) && lab->isAtLeastAcquire() && !lab->isAtLeastRelease() || llvm::isa<FenceLabel>(lab) && lab->isAtLeastAcquire() && lab->isAtLeastRelease() || llvm::isa<FenceLabel>(lab) && lab->isSC())for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_3[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_3(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && !g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto status = visitedCalc0_1[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_1(p, calcRes);
	}
	if (llvm::isa<FenceLabel>(lab) && lab->isAtLeastAcquire() && !lab->isAtLeastRelease() || llvm::isa<FenceLabel>(lab) && lab->isAtLeastAcquire() && lab->isAtLeastRelease() || llvm::isa<FenceLabel>(lab) && lab->isSC())for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_1[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_1(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_5[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_5(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_6(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	visitedCalc0_6[lab->getStamp().get()] = NodeStatus::left;
}

View IMMChecker::calculate0(const Event &e)
{
	View calcRes;
calcRes.updateIdx(e.prev());
	visitedCalc0_0.clear();
	visitedCalc0_0.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedCalc0_1.clear();
	visitedCalc0_1.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedCalc0_2.clear();
	visitedCalc0_2.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedCalc0_3.clear();
	visitedCalc0_3.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedCalc0_4.clear();
	visitedCalc0_4.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedCalc0_5.clear();
	visitedCalc0_5.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedCalc0_6.clear();
	visitedCalc0_6.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);

	getGraph().getEventLabel(e)->setViews({{}, });
	visitCalc0_6(e, calcRes);
	return calcRes;
}
std::vector<VSet<Event>> IMMChecker::calculateSaved(const Event &e)
{
	return std::move(saved);
}

std::vector<View> IMMChecker::calculateViews(const Event &e)
{
	views.push_back(calculate0(e));
	return std::move(views);
}

VerificationError IMMChecker::checkErrors(const Event &e)
{
	return VerificationError::VE_OK;
}

bool IMMChecker::visitAcyclic0(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic0[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : data_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rfi_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : data_preds(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p)) || g.getEventLabel(p)->isDependable()) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rfi_preds(g, lab->getPos()))if (g.isRMWStore(g.getEventLabel(p))) {
		auto &node = visitedAcyclic2[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic2(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : data_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rfi_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic0[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic1(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic1[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : addr_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : ctrl_preds(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p)) || g.getEventLabel(p)->isDependable()) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : addr_preds(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p)) || g.getEventLabel(p)->isDependable()) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : addr_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic1[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic2(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic2[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : po_imm_preds(g, lab->getPos()))if (g.isRMWLoad(g.getEventLabel(p))) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (g.isRMWLoad(g.getEventLabel(p))) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (g.isRMWLoad(g.getEventLabel(p))) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic2[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic3(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic3[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : poloc_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic3[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic3(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : poloc_imm_preds(g, lab->getPos()))if (llvm::isa<WriteLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isSC() || llvm::isa<WriteLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic3[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic4(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic4[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic4[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic4(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic4[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic5(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic5[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastAcquire()) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic5[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic6(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic6[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic6[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isAtLeastAcquire() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic10[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic10(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic6[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic7(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic7[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastRelease()) {
		auto &node = visitedAcyclic10[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic10(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic7[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic8(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic8[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic6[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (g.isRMWStore(lab))for (auto &p : po_imm_preds(g, lab->getPos()))if (g.isRMWLoad(g.getEventLabel(p))) {
		auto &node = visitedAcyclic7[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic7(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isAtLeastAcquire() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic10[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic10(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic8[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic9(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic9[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic9[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic9(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic9[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic9(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic9[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic9(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic10[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic10(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastRelease()) {
		auto &node = visitedAcyclic10[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic10(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic10[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic10(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic10[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic10(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic9[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic10(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic10[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : maximals(lab->view(0)))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : maximals(lab->view(0))) {
		auto &node = visitedAcyclic10[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic10(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic10[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic11(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic11[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic9[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic9(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic9[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic9(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic9[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic9(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic10[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic10(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic10[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic10(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic10[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic10(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : maximals(lab->view(0))) {
		auto &node = visitedAcyclic11[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic11(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic11[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic12(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	++visitedAccepting;
	visitedAcyclic12[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	if (llvm::isa<WriteLabel>(lab))for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : addr_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : data_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (g.isRMWStore(lab))for (auto &p : po_imm_preds(g, lab->getPos()))if (g.isRMWLoad(g.getEventLabel(p))) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<FenceLabel>(lab))for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic4[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic4(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isAtLeastRelease())for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic4[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic4(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : poloc_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic3[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic3(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rfe_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : ctrl_preds(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p)) || g.getEventLabel(p)->isDependable()) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : addr_preds(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p)) || g.getEventLabel(p)->isDependable()) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : data_preds(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p)) || g.getEventLabel(p)->isDependable()) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : detour_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : poloc_imm_preds(g, lab->getPos()))if (llvm::isa<WriteLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isSC() || llvm::isa<WriteLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastAcquire()) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<FenceLabel>(lab))for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isAtLeastRelease())for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (g.isRMWStore(lab))for (auto &p : po_imm_preds(g, lab->getPos()))if (g.isRMWLoad(g.getEventLabel(p))) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : addr_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : data_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (g.isRMWStore(lab))for (auto &p : po_imm_preds(g, lab->getPos()))if (g.isRMWLoad(g.getEventLabel(p))) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<FenceLabel>(lab) && lab->isSC())for (auto &p : maximals(lab->view(0))) {
		auto &node = visitedAcyclic11[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic11(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	--visitedAccepting;
	visitedAcyclic12[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::isAcyclic(const Event &e)
{
	visitedAccepting = 0;
	visitedAcyclic0.clear();
	visitedAcyclic0.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic1.clear();
	visitedAcyclic1.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic2.clear();
	visitedAcyclic2.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic3.clear();
	visitedAcyclic3.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic4.clear();
	visitedAcyclic4.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic5.clear();
	visitedAcyclic5.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic6.clear();
	visitedAcyclic6.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic7.clear();
	visitedAcyclic7.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic8.clear();
	visitedAcyclic8.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic9.clear();
	visitedAcyclic9.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic10.clear();
	visitedAcyclic10.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic11.clear();
	visitedAcyclic11.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic12.clear();
	visitedAcyclic12.resize(g.getMaxStamp().get() + 1);
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
		&& visitAcyclic12(e);
}

bool IMMChecker::isConsistent(const Event &e)
{
	return isAcyclic(e);
}

bool IMMChecker::isRecAcyclic(const Event &e)
{
	visitedRecAccepting = 0;
	return true;
}

bool IMMChecker::isRecoveryValid(const Event &e)
{
	return isRecAcyclic(e);
}

void IMMChecker::visitPPoRf0(const Event &e, DepView &pporf)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedPPoRf0[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : poloc_imm_preds(g, lab->getPos()))if (llvm::isa<WriteLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isSC() || llvm::isa<WriteLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	for (auto &p : poloc_imm_preds(g, lab->getPos())) {
		auto status = visitedPPoRf0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf0(p, pporf);
	}
	visitedPPoRf0[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitPPoRf1(const Event &e, DepView &pporf)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedPPoRf1[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : rfi_preds(g, lab->getPos()))if (g.isRMWStore(g.getEventLabel(p))) {
		auto status = visitedPPoRf3[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf3(p, pporf);
	}
	for (auto &p : data_preds(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p)) || g.getEventLabel(p)->isDependable()) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	for (auto &p : data_preds(g, lab->getPos())) {
		auto status = visitedPPoRf1[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf1(p, pporf);
	}
	for (auto &p : rfi_preds(g, lab->getPos())) {
		auto status = visitedPPoRf1[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf1(p, pporf);
	}
	for (auto &p : data_preds(g, lab->getPos())) {
		auto status = visitedPPoRf2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf2(p, pporf);
	}
	for (auto &p : rfi_preds(g, lab->getPos())) {
		auto status = visitedPPoRf2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf2(p, pporf);
	}
	visitedPPoRf1[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitPPoRf2(const Event &e, DepView &pporf)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedPPoRf2[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : ctrl_preds(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p)) || g.getEventLabel(p)->isDependable()) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	for (auto &p : addr_preds(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p)) || g.getEventLabel(p)->isDependable()) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto status = visitedPPoRf1[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf1(p, pporf);
	}
	for (auto &p : addr_preds(g, lab->getPos())) {
		auto status = visitedPPoRf1[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf1(p, pporf);
	}
	for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto status = visitedPPoRf2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf2(p, pporf);
	}
	for (auto &p : addr_preds(g, lab->getPos())) {
		auto status = visitedPPoRf2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf2(p, pporf);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedPPoRf2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf2(p, pporf);
	}
	visitedPPoRf2[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitPPoRf3(const Event &e, DepView &pporf)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedPPoRf3[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : po_imm_preds(g, lab->getPos()))if (g.isRMWLoad(g.getEventLabel(p))) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (g.isRMWLoad(g.getEventLabel(p))) {
		auto status = visitedPPoRf1[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf1(p, pporf);
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (g.isRMWLoad(g.getEventLabel(p))) {
		auto status = visitedPPoRf2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf2(p, pporf);
	}
	visitedPPoRf3[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitPPoRf4(const Event &e, DepView &pporf)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedPPoRf4[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedPPoRf4[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf4(p, pporf);
	}
	visitedPPoRf4[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitPPoRf5(const Event &e, DepView &pporf)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedPPoRf5[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p))) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastAcquire()) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedPPoRf5[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf5(p, pporf);
	}
	visitedPPoRf5[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitPPoRf6(const Event &e, DepView &pporf)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedPPoRf6[lab->getStamp().get()] = NodeStatus::entered;
	pporf.updateIdx(e);
	for (auto &p : tc_preds(g, lab->getPos())) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	for (auto &p : tj_preds(g, lab->getPos())) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	for (auto &p : rfe_preds(g, lab->getPos())) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : ctrl_preds(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p)) || g.getEventLabel(p)->isDependable()) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : addr_preds(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p)) || g.getEventLabel(p)->isDependable()) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : data_preds(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p)) || g.getEventLabel(p)->isDependable()) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	for (auto &p : detour_preds(g, lab->getPos())) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : poloc_imm_preds(g, lab->getPos()))if (llvm::isa<WriteLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isSC() || llvm::isa<WriteLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p))) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastAcquire()) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	if (llvm::isa<FenceLabel>(lab))for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	if (lab->isAtLeastRelease())for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	if (g.isRMWStore(lab))for (auto &p : po_imm_preds(g, lab->getPos()))if (g.isRMWLoad(g.getEventLabel(p))) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto status = visitedPPoRf1[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf1(p, pporf);
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : addr_preds(g, lab->getPos())) {
		auto status = visitedPPoRf1[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf1(p, pporf);
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : data_preds(g, lab->getPos())) {
		auto status = visitedPPoRf1[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf1(p, pporf);
	}
	if (g.isRMWStore(lab))for (auto &p : po_imm_preds(g, lab->getPos()))if (g.isRMWLoad(g.getEventLabel(p))) {
		auto status = visitedPPoRf1[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf1(p, pporf);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedPPoRf5[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf5(p, pporf);
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : poloc_imm_preds(g, lab->getPos())) {
		auto status = visitedPPoRf0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf0(p, pporf);
	}
	if (llvm::isa<FenceLabel>(lab))for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedPPoRf4[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf4(p, pporf);
	}
	if (lab->isAtLeastRelease())for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedPPoRf4[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf4(p, pporf);
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto status = visitedPPoRf2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf2(p, pporf);
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : addr_preds(g, lab->getPos())) {
		auto status = visitedPPoRf2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf2(p, pporf);
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : data_preds(g, lab->getPos())) {
		auto status = visitedPPoRf2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf2(p, pporf);
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedPPoRf2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf2(p, pporf);
	}
	if (g.isRMWStore(lab))for (auto &p : po_imm_preds(g, lab->getPos()))if (g.isRMWLoad(g.getEventLabel(p))) {
		auto status = visitedPPoRf2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf2(p, pporf);
	}
	visitedPPoRf6[lab->getStamp().get()] = NodeStatus::left;
}

DepView IMMChecker::calcPPoRfBefore(const Event &e)
{
	DepView pporf;
	pporf.updateIdx(e);
	visitedPPoRf0.clear();
	visitedPPoRf0.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf1.clear();
	visitedPPoRf1.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf2.clear();
	visitedPPoRf2.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf3.clear();
	visitedPPoRf3.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf4.clear();
	visitedPPoRf4.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf5.clear();
	visitedPPoRf5.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf6.clear();
	visitedPPoRf6.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);

	visitPPoRf6(e, pporf);
	return pporf;
}
std::unique_ptr<VectorClock> IMMChecker::getPPoRfBefore(const Event &e)
{
	return LLVM_MAKE_UNIQUE<DepView>(calcPPoRfBefore(e));
}

