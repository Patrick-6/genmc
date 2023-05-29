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
#include "MOCalculator.hpp"

void IMMChecker::visitCalc0_0(const Event &e, View &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedCalc0_0[lab->getStamp().get()] = NodeStatus::entered;
	calcRes.update(lab->view(0));
	calcRes.updateIdx(lab->getPos());
	visitedCalc0_0[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitCalc0_1(const Event &e, View &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedCalc0_1[lab->getStamp().get()] = NodeStatus::entered;
	if (llvm::isa<ThreadStartLabel>(lab))if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto status = visitedCalc0_4[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_4(p, calcRes);
	}
	if (llvm::isa<ThreadJoinLabel>(lab))if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto status = visitedCalc0_4[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_4(p, calcRes);
	}
	if (llvm::isa<FenceLabel>(lab) && lab->isAtLeastAcquire() && !lab->isAtLeastRelease() || llvm::isa<FenceLabel>(lab) && lab->isAtLeastAcquire() && lab->isAtLeastRelease() || llvm::isa<FenceLabel>(lab) && lab->isSC())if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto status = visitedCalc0_4[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_4(p, calcRes);
	}
	if (llvm::isa<ThreadStartLabel>(lab))if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto status = visitedCalc0_6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_6(p, calcRes);
	}
	if (llvm::isa<ThreadJoinLabel>(lab))if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto status = visitedCalc0_6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_6(p, calcRes);
	}
	if (llvm::isa<FenceLabel>(lab) && lab->isAtLeastAcquire() && !lab->isAtLeastRelease() || llvm::isa<FenceLabel>(lab) && lab->isAtLeastAcquire() && lab->isAtLeastRelease() || llvm::isa<FenceLabel>(lab) && lab->isSC())if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto status = visitedCalc0_6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_6(p, calcRes);
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto status = visitedCalc0_2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_2(p, calcRes);
	}
	visitedCalc0_1[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitCalc0_2(const Event &e, View &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedCalc0_2[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : tc_preds(g, lab->getPos())) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	for (auto &p : tj_preds(g, lab->getPos())) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastRelease()) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos())) {
		auto status = visitedCalc0_5[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_5(p, calcRes);
	}
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos())) {
		auto status = visitedCalc0_3[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_3(p, calcRes);
	}
	visitedCalc0_2[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitCalc0_3(const Event &e, View &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedCalc0_3[lab->getStamp().get()] = NodeStatus::entered;
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<ThreadFinishLabel>(g.getEventLabel(p))) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<ThreadCreateLabel>(g.getEventLabel(p))) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isAtLeastAcquire() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto status = visitedCalc0_3[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_3(p, calcRes);
	}
	visitedCalc0_3[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitCalc0_4(const Event &e, View &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedCalc0_4[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastRelease()) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto status = visitedCalc0_5[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_5(p, calcRes);
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
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

	visitedCalc0_5[lab->getStamp().get()] = NodeStatus::entered;
	if (g.isRMWStore(lab))if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.isRMWLoad(g.getEventLabel(p))) {
		auto status = visitedCalc0_4[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_4(p, calcRes);
	}
	visitedCalc0_5[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitCalc0_6(const Event &e, View &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedCalc0_6[lab->getStamp().get()] = NodeStatus::entered;
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto status = visitedCalc0_4[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_4(p, calcRes);
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto status = visitedCalc0_6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_6(p, calcRes);
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

	visitCalc0_1(e, calcRes);
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

bool IMMChecker::isDepTracking()
{
	return 1;
}

VerificationError IMMChecker::checkErrors(const Event &e)
{
	return VerificationError::VE_OK;
}

bool IMMChecker::visitAcyclic0(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic0[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
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

	visitedAcyclic1[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<ThreadFinishLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<ThreadCreateLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isAtLeastAcquire() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<ThreadFinishLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<ThreadCreateLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isAtLeastAcquire() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
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

	visitedAcyclic2[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastRelease()) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastRelease()) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic3[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic3(p))
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

	visitedAcyclic3[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<ThreadFinishLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<ThreadCreateLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isAtLeastAcquire() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (g.isRMWStore(lab))if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.isRMWLoad(g.getEventLabel(p))) {
		auto &node = visitedAcyclic2[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic2(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<ThreadFinishLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<ThreadCreateLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isAtLeastAcquire() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
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

	visitedAcyclic4[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : tc_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : tj_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastRelease()) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic4[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic4(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : tc_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : tj_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastRelease()) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic3[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic3(p))
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

	visitedAcyclic5[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	FOREACH_MAXIMAL(p, lab->view(0)) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	FOREACH_MAXIMAL(p, lab->view(0)) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
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

	visitedAcyclic6[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic11[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic11(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : tc_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : tj_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastRelease()) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
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

	visitedAcyclic7[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic6[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic7[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic7(p))
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

	visitedAcyclic8[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic6[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic6[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos()))if (g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos()))if (g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
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

	visitedAcyclic9[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<ThreadFinishLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<ThreadCreateLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isAtLeastAcquire() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic9[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic9(p))
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

	visitedAcyclic10[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic11[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic11(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastRelease()) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
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

	visitedAcyclic11[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<ThreadFinishLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<ThreadCreateLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isAtLeastAcquire() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (g.isRMWStore(lab))if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.isRMWLoad(g.getEventLabel(p))) {
		auto &node = visitedAcyclic10[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic10(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic9[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic9(p))
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

	visitedAcyclic12[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic11[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic11(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : tc_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : tj_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastRelease()) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic12[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic13(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic13[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	FOREACH_MAXIMAL(p, lab->view(0)) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	FOREACH_MAXIMAL(p, lab->view(0))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic13[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic14(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic14[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	FOREACH_MAXIMAL(p, lab->view(0)) {
		auto &node = visitedAcyclic14[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic14(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	FOREACH_MAXIMAL(p, lab->view(0))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	FOREACH_MAXIMAL(p, lab->view(0)) {
		auto &node = visitedAcyclic7[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic7(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	FOREACH_MAXIMAL(p, lab->view(0)) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic14[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic15(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	++visitedAccepting;
	visitedAcyclic15[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	if (lab->isSC())if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<FenceLabel>(lab) && lab->isSC())FOREACH_MAXIMAL(p, lab->view(0)) {
		auto &node = visitedAcyclic14[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic14(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isSC())if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic4[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic4(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isSC())for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isSC())for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isSC())for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isSC())if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isSC())for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic6[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isSC())for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic6[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isSC())for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic6[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isSC())if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic6[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<FenceLabel>(lab) && lab->isSC())FOREACH_MAXIMAL(p, lab->view(0))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isSC())for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isSC())for (auto &p : co_imm_preds(g, lab->getPos()))if (g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isSC())for (auto &p : fr_imm_preds(g, lab->getPos()))if (g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isSC())if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<FenceLabel>(lab) && lab->isSC())FOREACH_MAXIMAL(p, lab->view(0)) {
		auto &node = visitedAcyclic7[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic7(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isSC())if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic7[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic7(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<FenceLabel>(lab) && lab->isSC())FOREACH_MAXIMAL(p, lab->view(0)) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isSC())for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	--visitedAccepting;
	visitedAcyclic15[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic16(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic16[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : poloc_imm_preds(g, lab->getPos()))if (llvm::isa<WriteLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isSC() || llvm::isa<WriteLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : poloc_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic16[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic16(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic16[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic17(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic17[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic17[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic17(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<ThreadFinishLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic21[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic21(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<ThreadCreateLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic21[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic21(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isAtLeastAcquire() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic21[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic21(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic17[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic18(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic18[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastRelease()) {
		auto &node = visitedAcyclic21[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic21(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic19[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic19(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic18[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic19(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic19[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (g.isRMWStore(lab))if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.isRMWLoad(g.getEventLabel(p))) {
		auto &node = visitedAcyclic18[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic18(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic17[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic17(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<ThreadFinishLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic21[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic21(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<ThreadCreateLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic21[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic21(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isAtLeastAcquire() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic21[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic21(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic19[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic20(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic20[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic20[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic20(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic20[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic20(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic20[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic20(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : tc_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic21[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic21(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : tj_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic21[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic21(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic21[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic21(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastRelease()) {
		auto &node = visitedAcyclic21[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic21(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic21[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic21(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic21[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic21(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic19[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic19(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic20[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic21(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic21[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	FOREACH_MAXIMAL(p, lab->view(0))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	FOREACH_MAXIMAL(p, lab->view(0)) {
		auto &node = visitedAcyclic21[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic21(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic21[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic22(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic22[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic20[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic20(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic20[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic20(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic20[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic20(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	FOREACH_MAXIMAL(p, lab->view(0)) {
		auto &node = visitedAcyclic22[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic22(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic21[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic21(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic21[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic21(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic21[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic21(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic22[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic23(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic23[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rfi_preds(g, lab->getPos()))if (g.isRMWStore(g.getEventLabel(p))) {
		auto &node = visitedAcyclic24[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic24(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : data_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic23[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic23(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rfi_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic23[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic23(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : data_preds(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p)) || g.getEventLabel(p)->isDependable()) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : data_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rfi_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic23[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic24(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic24[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.isRMWLoad(g.getEventLabel(p))) {
		auto &node = visitedAcyclic23[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic23(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.isRMWLoad(g.getEventLabel(p))) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.isRMWLoad(g.getEventLabel(p))) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic24[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic25(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic25[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic23[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic23(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : addr_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic23[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic23(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : ctrl_preds(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p)) || g.getEventLabel(p)->isDependable()) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : addr_preds(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p)) || g.getEventLabel(p)->isDependable()) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : addr_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic25[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic26(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic26[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.getEventLabel(p)->isAtLeastAcquire()) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<FenceLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic26[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic26(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic26[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic27(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic27[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic27[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic27(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic27[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool IMMChecker::visitAcyclic28(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	++visitedAccepting;
	visitedAcyclic28[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	if (lab->isAtLeastRelease())if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic27[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic27(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<FenceLabel>(lab))if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic27[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic27(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic23[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic23(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : addr_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic23[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic23(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : data_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic23[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic23(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (g.isRMWStore(lab))if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.isRMWLoad(g.getEventLabel(p))) {
		auto &node = visitedAcyclic23[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic23(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rfe_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : ctrl_preds(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p)) || g.getEventLabel(p)->isDependable()) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : addr_preds(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p)) || g.getEventLabel(p)->isDependable()) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : data_preds(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p)) || g.getEventLabel(p)->isDependable()) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : detour_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : poloc_imm_preds(g, lab->getPos()))if (llvm::isa<WriteLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isSC() || llvm::isa<WriteLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.getEventLabel(p)->isAtLeastAcquire()) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<FenceLabel>(g.getEventLabel(p))) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isAtLeastRelease())if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<FenceLabel>(lab))if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (g.isRMWStore(lab))if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.isRMWLoad(g.getEventLabel(p))) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<FenceLabel>(lab) && lab->isSC())FOREACH_MAXIMAL(p, lab->view(0)) {
		auto &node = visitedAcyclic22[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic22(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : addr_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : data_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (g.isRMWStore(lab))if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.isRMWLoad(g.getEventLabel(p))) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto &node = visitedAcyclic26[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic26(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : poloc_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic16[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic16(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	--visitedAccepting;
	visitedAcyclic28[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
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
	visitedAcyclic13.clear();
	visitedAcyclic13.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic14.clear();
	visitedAcyclic14.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic15.clear();
	visitedAcyclic15.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic16.clear();
	visitedAcyclic16.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic17.clear();
	visitedAcyclic17.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic18.clear();
	visitedAcyclic18.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic19.clear();
	visitedAcyclic19.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic20.clear();
	visitedAcyclic20.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic21.clear();
	visitedAcyclic21.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic22.clear();
	visitedAcyclic22.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic23.clear();
	visitedAcyclic23.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic24.clear();
	visitedAcyclic24.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic25.clear();
	visitedAcyclic25.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic26.clear();
	visitedAcyclic26.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic27.clear();
	visitedAcyclic27.resize(g.getMaxStamp().get() + 1);
	visitedAcyclic28.clear();
	visitedAcyclic28.resize(g.getMaxStamp().get() + 1);
	return true
		&& visitAcyclic0(e)
		&& visitAcyclic5(e)
		&& visitAcyclic6(e)
		&& visitAcyclic8(e)
		&& visitAcyclic12(e)
		&& visitAcyclic13(e)
		&& visitAcyclic15(e)
		&& visitAcyclic20(e)
		&& visitAcyclic21(e);
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

	visitedPPoRf0[lab->getStamp().get()] = NodeStatus::entered;
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto status = visitedPPoRf0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf0(p, pporf);
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	visitedPPoRf0[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitPPoRf1(const Event &e, DepView &pporf)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedPPoRf1[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : rfi_preds(g, lab->getPos()))if (g.isRMWStore(g.getEventLabel(p))) {
		auto status = visitedPPoRf2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf2(p, pporf);
	}
	for (auto &p : data_preds(g, lab->getPos())) {
		auto status = visitedPPoRf3[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf3(p, pporf);
	}
	for (auto &p : rfi_preds(g, lab->getPos())) {
		auto status = visitedPPoRf3[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf3(p, pporf);
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
	for (auto &p : data_preds(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p)) || g.getEventLabel(p)->isDependable()) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	visitedPPoRf1[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitPPoRf2(const Event &e, DepView &pporf)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedPPoRf2[lab->getStamp().get()] = NodeStatus::entered;
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.isRMWLoad(g.getEventLabel(p))) {
		auto status = visitedPPoRf3[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf3(p, pporf);
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.isRMWLoad(g.getEventLabel(p))) {
		auto status = visitedPPoRf1[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf1(p, pporf);
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.isRMWLoad(g.getEventLabel(p))) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	visitedPPoRf2[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitPPoRf3(const Event &e, DepView &pporf)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedPPoRf3[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto status = visitedPPoRf3[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf3(p, pporf);
	}
	for (auto &p : addr_preds(g, lab->getPos())) {
		auto status = visitedPPoRf3[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf3(p, pporf);
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto status = visitedPPoRf3[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf3(p, pporf);
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
	visitedPPoRf3[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitPPoRf4(const Event &e, DepView &pporf)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedPPoRf4[lab->getStamp().get()] = NodeStatus::entered;
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto status = visitedPPoRf4[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf4(p, pporf);
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.getEventLabel(p)->isAtLeastAcquire()) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<FenceLabel>(g.getEventLabel(p))) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	visitedPPoRf4[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitPPoRf5(const Event &e, DepView &pporf)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedPPoRf5[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : poloc_imm_preds(g, lab->getPos())) {
		auto status = visitedPPoRf5[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf5(p, pporf);
	}
	for (auto &p : poloc_imm_preds(g, lab->getPos()))if (llvm::isa<WriteLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isSC() || llvm::isa<WriteLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	visitedPPoRf5[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitPPoRf6(const Event &e, DepView &pporf)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedPPoRf6[lab->getStamp().get()] = NodeStatus::entered;
	pporf.updateIdx(e);
	if (lab->isAtLeastRelease())if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto status = visitedPPoRf0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf0(p, pporf);
	}
	if (llvm::isa<FenceLabel>(lab))if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto status = visitedPPoRf0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf0(p, pporf);
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto status = visitedPPoRf4[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf4(p, pporf);
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : poloc_imm_preds(g, lab->getPos())) {
		auto status = visitedPPoRf5[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf5(p, pporf);
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : ctrl_preds(g, lab->getPos())) {
		auto status = visitedPPoRf3[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf3(p, pporf);
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : addr_preds(g, lab->getPos())) {
		auto status = visitedPPoRf3[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf3(p, pporf);
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : data_preds(g, lab->getPos())) {
		auto status = visitedPPoRf3[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf3(p, pporf);
	}
	if (llvm::isa<WriteLabel>(lab))if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto status = visitedPPoRf3[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf3(p, pporf);
	}
	if (g.isRMWStore(lab))if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.isRMWLoad(g.getEventLabel(p))) {
		auto status = visitedPPoRf3[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf3(p, pporf);
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
	if (g.isRMWStore(lab))if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.isRMWLoad(g.getEventLabel(p))) {
		auto status = visitedPPoRf1[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf1(p, pporf);
	}
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
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.getEventLabel(p)->isAtLeastAcquire()) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (llvm::isa<FenceLabel>(g.getEventLabel(p))) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	if (lab->isAtLeastRelease())if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	if (llvm::isa<FenceLabel>(lab))if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer()) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
	}
	if (g.isRMWStore(lab))if (auto p = po_imm_pred(g, lab->getPos()); !p.isInitializer())if (g.isRMWLoad(g.getEventLabel(p))) {
		auto status = visitedPPoRf6[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(p, pporf);
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

const View &IMMChecker::getHbView(const Event &e)
{
	return getGraph().getEventLabel(e)->view(0);
}


bool IMMChecker::isWriteRfBefore(Event a, Event b)
{
	auto &g = getGraph();
	auto &before = g.getEventLabel(b)->view(0);
	if (before.contains(a))
		return true;

	const EventLabel *lab = g.getEventLabel(a);

	BUG_ON(!llvm::isa<WriteLabel>(lab));
	auto *wLab = static_cast<const WriteLabel *>(lab);
	for (auto &r : wLab->getReadersList())
		if (before.contains(r))
			return true;
	return false;
}

std::vector<Event>
IMMChecker::getInitRfsAtLoc(SAddr addr)
{
	std::vector<Event> result;

	for (const auto *lab : labels(getGraph())) {
		if (auto *rLab = llvm::dyn_cast<ReadLabel>(lab))
			if (rLab->getRf().isInitializer() && rLab->getAddr() == addr)
				result.push_back(rLab->getPos());
	}
	return result;
}

bool IMMChecker::isHbOptRfBefore(const Event e, const Event write)
{
	auto &g = getGraph();
	const EventLabel *lab = g.getEventLabel(write);

	BUG_ON(!llvm::isa<WriteLabel>(lab));
	auto *sLab = static_cast<const WriteLabel *>(lab);
	if (sLab->view(0).contains(e))
		return true;

	for (auto &r : sLab->getReadersList()) {
		if (g.getEventLabel(r)->view(0).contains(e))
			return true;
	}
	return false;
}

int IMMChecker::splitLocMOBefore(SAddr addr, Event e)
{
	const auto &g = getGraph();
	auto rit = std::find_if(store_rbegin(g, addr), store_rend(g, addr), [&](const Event &s){
		return isWriteRfBefore(s, e);
	});
	return (rit == store_rend(g, addr)) ? 0 : std::distance(rit, store_rend(g, addr));
}

int IMMChecker::splitLocMOAfterHb(SAddr addr, const Event read)
{
	const auto &g = getGraph();

	auto initRfs = g.getInitRfsAtLoc(addr);
	if (std::any_of(initRfs.begin(), initRfs.end(), [&read,&g](const Event &rf){
		return g.getEventLabel(rf)->view(0).contains(read);
	}))
		return 0;

	auto it = std::find_if(store_begin(g, addr), store_end(g, addr), [&](const Event &s){
		return isHbOptRfBefore(read, s);
	});
	if (it == store_end(g, addr))
		return std::distance(store_begin(g, addr), store_end(g, addr));
	return (g.getEventLabel(*it)->view(0).contains(read)) ?
		std::distance(store_begin(g, addr), it) : std::distance(store_begin(g, addr), it) + 1;
}

int IMMChecker::splitLocMOAfter(SAddr addr, const Event e)
{
	const auto &g = getGraph();
	auto it = std::find_if(store_begin(g, addr), store_end(g, addr), [&](const Event &s){
		return isHbOptRfBefore(e, s);
	});
	return (it == store_end(g, addr)) ? std::distance(store_begin(g, addr), store_end(g, addr)) :
		std::distance(store_begin(g, addr), it);
}

std::vector<Event>
IMMChecker::getCoherentStores(SAddr addr, Event read)
{
	auto &g = getGraph();
	std::vector<Event> stores;

	/*
	 * If there are no stores (rf?;hb)-before the current event
	 * then we can read read from all concurrent stores and the
	 * initializer store. Otherwise, we can read from all concurrent
	 * stores and the mo-latest of the (rf?;hb)-before stores.
	 */
	auto begO = splitLocMOBefore(addr, read);
	if (begO == 0)
		stores.push_back(Event::getInitializer());
	else
		stores.push_back(*(store_begin(g, addr) + begO - 1));

	/*
	 * If the model supports out-of-order execution we have to also
	 * account for the possibility the read is hb-before some other
	 * store, or some read that reads from a store.
	 */
	auto endO = (isDepTracking()) ? splitLocMOAfterHb(addr, read) :
		std::distance(store_begin(g, addr), store_end(g, addr));
	stores.insert(stores.end(), store_begin(g, addr) + begO, store_begin(g, addr) + endO);
	return stores;
}

std::vector<Event>
IMMChecker::getMOOptRfAfter(const WriteLabel *sLab)
{
	std::vector<Event> after;

	auto &g = getGraph();
	std::for_each(co_succ_begin(g, sLab->getAddr(), sLab->getPos()),
		      co_succ_end(g, sLab->getAddr(), sLab->getPos()), [&](const Event &w){
			      auto *wLab = g.getWriteLabel(w);
			      after.push_back(wLab->getPos());
			      after.insert(after.end(), wLab->readers_begin(), wLab->readers_end());
	});
	return after;
}

std::vector<Event>
IMMChecker::getMOInvOptRfAfter(const WriteLabel *sLab)
{
	auto &g = getGraph();
	std::vector<Event> after;

	/* First, add (mo;rf?)-before */
	std::for_each(co_pred_begin(g, sLab->getAddr(), sLab->getPos()),
		      co_pred_end(g, sLab->getAddr(), sLab->getPos()), [&](const Event &w){
			      auto *wLab = g.getWriteLabel(w);
			      after.push_back(wLab->getPos());
			      after.insert(after.end(), wLab->readers_begin(), wLab->readers_end());
	});

	/* Then, we add the reader list for the initializer */
	auto initRfs = g.getInitRfsAtLoc(sLab->getAddr());
	after.insert(after.end(), initRfs.begin(), initRfs.end());
	return after;
}

std::vector<Event>
IMMChecker::getCoherentRevisits(const WriteLabel *sLab, const VectorClock &pporf)
{
	auto &g = getGraph();
	auto ls = g.getRevisitable(sLab, pporf);

	/* If this store is po- and mo-maximal then we are done */
	if (!isDepTracking() && g.isCoMaximal(sLab->getAddr(), sLab->getPos()))
		return ls;

	/* First, we have to exclude (mo;rf?;hb?;sb)-after reads */
	auto optRfs = getMOOptRfAfter(sLab);
	ls.erase(std::remove_if(ls.begin(), ls.end(), [&](Event e)
				{ const View &before = g.getEventLabel(e)->view(0);
				  return std::any_of(optRfs.begin(), optRfs.end(),
					 [&](Event ev)
					 { return before.contains(ev); });
				}), ls.end());

	/* If out-of-order event addition is not supported, then we are done
	 * due to po-maximality */
	if (!isDepTracking())
		return ls;

	/* Otherwise, we also have to exclude hb-before loads */
	ls.erase(std::remove_if(ls.begin(), ls.end(), [&](Event e)
		{ return g.getEventLabel(sLab->getPos())->view(0).contains(e); }),
		ls.end());

	/* ...and also exclude (mo^-1; rf?; (hb^-1)?; sb^-1)-after reads in
	 * the resulting graph */
	auto &before = pporf;
	auto moInvOptRfs = getMOInvOptRfAfter(sLab);
	ls.erase(std::remove_if(ls.begin(), ls.end(), [&](Event e)
				{ auto *eLab = g.getEventLabel(e);
				  auto v = g.getViewFromStamp(eLab->getStamp());
				  v->update(before);
				  return std::any_of(moInvOptRfs.begin(),
						     moInvOptRfs.end(),
						     [&](Event ev)
						     { return v->contains(ev) &&
						       g.getEventLabel(ev)->view(0).contains(e); });
				}),
		 ls.end());

	return ls;
}

std::pair<int, int>
IMMChecker::getCoherentPlacings(SAddr addr, Event store, bool isRMW)
{
	const auto &g = getGraph();
	auto *cc = llvm::dyn_cast<MOCalculator>(g.getCoherenceCalculator());

	/* If it is an RMW store, there is only one possible position in MO */
	if (isRMW) {
		if (auto *rLab = llvm::dyn_cast<ReadLabel>(g.getEventLabel(store.prev()))) {
			auto offset = cc->getStoreOffset(addr, rLab->getRf()) + 1;
			return std::make_pair(offset, offset);
		}
		BUG();
	}

	/* Otherwise, we calculate the full range and add the store */
	auto rangeBegin = splitLocMOBefore(addr, store);
	auto rangeEnd = (isDepTracking()) ? splitLocMOAfter(addr, store) :
		cc->getStoresToLoc(addr).size();
	return std::make_pair(rangeBegin, rangeEnd);

}
