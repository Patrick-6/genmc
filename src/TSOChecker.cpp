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

	visitedCalc0_0[lab->getStamp()] = NodeStatus::entered;
	calcRes.insert(e);
	for (const auto &p : lab->calculated(0)) {
		calcRes.erase(p);
		visitedCalc0_3[g.getEventLabel(p)->getStamp()] = NodeStatus::left;
	}
	for (auto &p : lab->calculated(0)) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	visitedCalc0_0[lab->getStamp()] = NodeStatus::left;
}

void TSOChecker::visitCalc0_1(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_1[lab->getStamp()] = NodeStatus::entered;
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_1[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_1(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (true && g.isRMWStore(g.getEventLabel(p))) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (true && llvm::isa<ReadLabel>(g.getEventLabel(p))) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (true && llvm::isa<FenceLabel>(g.getEventLabel(p))) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	visitedCalc0_1[lab->getStamp()] = NodeStatus::left;
}

void TSOChecker::visitCalc0_2(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_2[lab->getStamp()] = NodeStatus::entered;
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_2[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_2(p, calcRes);
	}
	visitedCalc0_2[lab->getStamp()] = NodeStatus::left;
}

void TSOChecker::visitCalc0_3(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_3[lab->getStamp()] = NodeStatus::entered;
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_1[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_1(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (true && g.isRMWStore(g.getEventLabel(p))) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (true && llvm::isa<ReadLabel>(g.getEventLabel(p))) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (true && llvm::isa<FenceLabel>(g.getEventLabel(p))) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	if (true && llvm::isa<WriteLabel>(lab))for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	if (true && llvm::isa<WriteLabel>(lab))for (auto &p : po_imm_preds(g, lab->getPos()))if (true && llvm::isa<CLFlushLabel>(g.getEventLabel(p))) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	if (true && llvm::isa<FenceLabel>(lab))for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	if (true && SmpFenceLabelLKMM::isType(lab, SmpFenceType::MBAUL))for (auto &p : po_imm_preds(g, lab->getPos()))if (true && llvm::isa<WriteLabel>(g.getEventLabel(p))) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	if (true && SmpFenceLabelLKMM::isType(lab, SmpFenceType::MBAUL))for (auto &p : po_imm_preds(g, lab->getPos()))if (true && llvm::isa<CLFlushLabel>(g.getEventLabel(p))) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	if (true && llvm::isa<WriteLabel>(lab))for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_2[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_2(p, calcRes);
	}
	if (true && llvm::isa<FenceLabel>(lab))for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_2[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_2(p, calcRes);
	}
	if (true && SmpFenceLabelLKMM::isType(lab, SmpFenceType::MBAUL))for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_4[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_4(p, calcRes);
	}
	if (true && llvm::isa<WriteLabel>(lab))for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_5[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_5(p, calcRes);
	}
	if (true && SmpFenceLabelLKMM::isType(lab, SmpFenceType::MBAUL))for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_5[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_5(p, calcRes);
	}
	visitedCalc0_3[lab->getStamp()] = NodeStatus::left;
}

void TSOChecker::visitCalc0_4(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_4[lab->getStamp()] = NodeStatus::entered;
	for (auto &p : po_imm_preds(g, lab->getPos()))if (true && llvm::isa<WriteLabel>(g.getEventLabel(p))) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_4[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_4(p, calcRes);
	}
	visitedCalc0_4[lab->getStamp()] = NodeStatus::left;
}

void TSOChecker::visitCalc0_5(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_5[lab->getStamp()] = NodeStatus::entered;
	for (auto &p : po_imm_preds(g, lab->getPos()))if (true && llvm::isa<CLFlushLabel>(g.getEventLabel(p))) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_5[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_5(p, calcRes);
	}
	visitedCalc0_5[lab->getStamp()] = NodeStatus::left;
}

VSet<Event> TSOChecker::calculate0(const Event &e)
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
	visitedCalc0_5.clear();
	visitedCalc0_5.resize(g.getMaxStamp() + 1, NodeStatus::unseen);

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
	visitedAcyclic0[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
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
	--visitedAccepting;
	visitedAcyclic0[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool TSOChecker::isAcyclic(const Event &e)
{
	visitedAccepting = 0;
	visitedAcyclic0.clear();
	visitedAcyclic0.resize(g.getMaxStamp() + 1);
	return true
		&& visitAcyclic0(e);
}

bool TSOChecker::isConsistent(const Event &e)
{
	return isAcyclic(e);
}

bool TSOChecker::visitRecovery0(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedRecovery0[lab->getStamp()] = { visitedRecAccepting, NodeStatus::entered };
	for (auto &p : poloc_imm_preds(g, lab->getPos()))if (true && llvm::isa<MemAccessLabel>(g.getEventLabel(p)) && llvm::dyn_cast<MemAccessLabel>(g.getEventLabel(p))->getAddr().isDurable() && llvm::isa<WriteLabel>(g.getEventLabel(p))) {
		auto &node = visitedRecovery8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	for (auto &p : poloc_imm_preds(g, lab->getPos())) {
		auto &node = visitedRecovery0[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	visitedRecovery0[lab->getStamp()] = { visitedRecAccepting, NodeStatus::left };
	return true;
}

bool TSOChecker::visitRecovery1(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedRecovery1[lab->getStamp()] = { visitedRecAccepting, NodeStatus::entered };
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedRecovery1[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (true && llvm::isa<CLFlushLabel>(g.getEventLabel(p))) {
		auto &node = visitedRecovery0[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	visitedRecovery1[lab->getStamp()] = { visitedRecAccepting, NodeStatus::left };
	return true;
}

bool TSOChecker::visitRecovery2(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedRecovery2[lab->getStamp()] = { visitedRecAccepting, NodeStatus::entered };
	for (auto &p : lab->calculated(0))if (true && g.getEventLabel(p)->getThread() != g.getRecoveryRoutineId()) {
		auto &node = visitedRecovery3[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery3(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	for (auto &p : rfe_preds(g, lab->getPos()))if (true && g.getEventLabel(p)->getThread() != g.getRecoveryRoutineId()) {
		auto &node = visitedRecovery3[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery3(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	for (auto &p : lab->calculated(0))if (true && llvm::isa<CLFlushLabel>(g.getEventLabel(p))) {
		auto &node = visitedRecovery8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	for (auto &p : lab->calculated(0))if (true && g.getEventLabel(p)->getThread() != g.getRecoveryRoutineId()) {
		auto &node = visitedRecovery2[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery2(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	for (auto &p : rfe_preds(g, lab->getPos()))if (true && g.getEventLabel(p)->getThread() != g.getRecoveryRoutineId()) {
		auto &node = visitedRecovery2[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery2(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	visitedRecovery2[lab->getStamp()] = { visitedRecAccepting, NodeStatus::left };
	return true;
}

bool TSOChecker::visitRecovery3(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedRecovery3[lab->getStamp()] = { visitedRecAccepting, NodeStatus::entered };
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedRecovery3[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery3(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos()))if (true && g.getEventLabel(p)->getThread() != g.getRecoveryRoutineId()) {
		auto &node = visitedRecovery3[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery3(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos()))if (true && g.getEventLabel(p)->getThread() != g.getRecoveryRoutineId()) {
		auto &node = visitedRecovery3[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery3(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos()))if (true && g.getEventLabel(p)->getThread() != g.getRecoveryRoutineId()) {
		auto &node = visitedRecovery2[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery2(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos()))if (true && g.getEventLabel(p)->getThread() != g.getRecoveryRoutineId()) {
		auto &node = visitedRecovery2[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery2(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	visitedRecovery3[lab->getStamp()] = { visitedRecAccepting, NodeStatus::left };
	return true;
}

bool TSOChecker::visitRecovery4(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedRecovery4[lab->getStamp()] = { visitedRecAccepting, NodeStatus::entered };
	if (true && lab->getThread() == g.getRecoveryRoutineId())for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedRecovery8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	visitedRecovery4[lab->getStamp()] = { visitedRecAccepting, NodeStatus::left };
	return true;
}

bool TSOChecker::visitRecovery5(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedRecovery5[lab->getStamp()] = { visitedRecAccepting, NodeStatus::entered };
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedRecovery4[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery4(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedRecovery5[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	visitedRecovery5[lab->getStamp()] = { visitedRecAccepting, NodeStatus::left };
	return true;
}

bool TSOChecker::visitRecovery6(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedRecovery6[lab->getStamp()] = { visitedRecAccepting, NodeStatus::entered };
	for (auto &p : po_imm_succs(g, lab->getPos())) {
		auto &node = visitedRecovery4[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery4(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_succs(g, lab->getPos())) {
		auto &node = visitedRecovery6[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	visitedRecovery6[lab->getStamp()] = { visitedRecAccepting, NodeStatus::left };
	return true;
}

bool TSOChecker::visitRecovery7(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedRecovery7[lab->getStamp()] = { visitedRecAccepting, NodeStatus::entered };
	for (auto &p : co_imm_preds(g, lab->getPos()))if (true && llvm::isa<MemAccessLabel>(g.getEventLabel(p)) && llvm::dyn_cast<MemAccessLabel>(g.getEventLabel(p))->getAddr().isDurable() && llvm::isa<WriteLabel>(g.getEventLabel(p))) {
		auto &node = visitedRecovery8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedRecovery6[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedRecovery5[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedRecovery7[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery7(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	visitedRecovery7[lab->getStamp()] = { visitedRecAccepting, NodeStatus::left };
	return true;
}

bool TSOChecker::visitRecovery8(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	++visitedRecAccepting;
	visitedRecovery8[lab->getStamp()] = { visitedRecAccepting, NodeStatus::entered };
	if (true && llvm::isa<MemAccessLabel>(lab) && llvm::dyn_cast<MemAccessLabel>(lab)->getAddr().isDurable() && llvm::isa<WriteLabel>(lab))for (auto &p : lab->calculated(0))if (true && g.getEventLabel(p)->getThread() != g.getRecoveryRoutineId()) {
		auto &node = visitedRecovery3[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery3(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	if (true && SmpFenceLabelLKMM::isType(lab, SmpFenceType::MBAUL))for (auto &p : lab->calculated(0))if (true && g.getEventLabel(p)->getThread() != g.getRecoveryRoutineId()) {
		auto &node = visitedRecovery3[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery3(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	if (true && llvm::isa<MemAccessLabel>(lab) && llvm::dyn_cast<MemAccessLabel>(lab)->getAddr().isDurable() && llvm::isa<WriteLabel>(lab))for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedRecovery3[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery3(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	if (true && llvm::isa<MemAccessLabel>(lab) && llvm::dyn_cast<MemAccessLabel>(lab)->getAddr().isDurable() && llvm::isa<WriteLabel>(lab))for (auto &p : co_imm_preds(g, lab->getPos()))if (true && g.getEventLabel(p)->getThread() != g.getRecoveryRoutineId()) {
		auto &node = visitedRecovery3[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery3(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	if (true && llvm::isa<MemAccessLabel>(lab) && llvm::dyn_cast<MemAccessLabel>(lab)->getAddr().isDurable() && llvm::isa<WriteLabel>(lab))for (auto &p : fr_imm_preds(g, lab->getPos()))if (true && g.getEventLabel(p)->getThread() != g.getRecoveryRoutineId()) {
		auto &node = visitedRecovery3[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery3(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedRecovery1[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	if (true && llvm::isa<MemAccessLabel>(lab) && llvm::dyn_cast<MemAccessLabel>(lab)->getAddr().isDurable() && llvm::isa<WriteLabel>(lab))for (auto &p : lab->calculated(0))if (true && llvm::isa<CLFlushLabel>(g.getEventLabel(p))) {
		auto &node = visitedRecovery8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	if (true && SmpFenceLabelLKMM::isType(lab, SmpFenceType::MBAUL))for (auto &p : lab->calculated(0))if (true && llvm::isa<CLFlushLabel>(g.getEventLabel(p))) {
		auto &node = visitedRecovery8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos()))if (true && llvm::isa<MemAccessLabel>(g.getEventLabel(p)) && llvm::dyn_cast<MemAccessLabel>(g.getEventLabel(p))->getAddr().isDurable() && llvm::isa<WriteLabel>(g.getEventLabel(p))) {
		auto &node = visitedRecovery8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	if (true && llvm::isa<CLFlushLabel>(lab))for (auto &p : poloc_imm_preds(g, lab->getPos()))if (true && llvm::isa<MemAccessLabel>(g.getEventLabel(p)) && llvm::dyn_cast<MemAccessLabel>(g.getEventLabel(p))->getAddr().isDurable() && llvm::isa<WriteLabel>(g.getEventLabel(p))) {
		auto &node = visitedRecovery8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	if (true && llvm::isa<MemAccessLabel>(lab) && llvm::dyn_cast<MemAccessLabel>(lab)->getAddr().isDurable() && llvm::isa<WriteLabel>(lab))for (auto &p : lab->calculated(0))if (true && g.getEventLabel(p)->getThread() != g.getRecoveryRoutineId()) {
		auto &node = visitedRecovery2[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery2(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	if (true && SmpFenceLabelLKMM::isType(lab, SmpFenceType::MBAUL))for (auto &p : lab->calculated(0))if (true && g.getEventLabel(p)->getThread() != g.getRecoveryRoutineId()) {
		auto &node = visitedRecovery2[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery2(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	if (true && llvm::isa<MemAccessLabel>(lab) && llvm::dyn_cast<MemAccessLabel>(lab)->getAddr().isDurable() && llvm::isa<WriteLabel>(lab))for (auto &p : co_imm_preds(g, lab->getPos()))if (true && g.getEventLabel(p)->getThread() != g.getRecoveryRoutineId()) {
		auto &node = visitedRecovery2[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery2(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	if (true && llvm::isa<MemAccessLabel>(lab) && llvm::dyn_cast<MemAccessLabel>(lab)->getAddr().isDurable() && llvm::isa<WriteLabel>(lab))for (auto &p : fr_imm_preds(g, lab->getPos()))if (true && g.getEventLabel(p)->getThread() != g.getRecoveryRoutineId()) {
		auto &node = visitedRecovery2[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery2(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	if (true && llvm::isa<CLFlushLabel>(lab))for (auto &p : poloc_imm_preds(g, lab->getPos())) {
		auto &node = visitedRecovery0[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (true && llvm::isa<CLFlushLabel>(g.getEventLabel(p))) {
		auto &node = visitedRecovery0[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedRecovery6[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto &node = visitedRecovery5[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedRecovery7[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitRecovery7(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
			return false;
	}
	if (lab->getThread() != g.getRecoveryRoutineId() && !visitHbloc(lab->getPos()))
		return false;
	--visitedRecAccepting;
	visitedRecovery8[lab->getStamp()] = { visitedRecAccepting, NodeStatus::left };
	return true;
}

bool isPbValid(const EventLabel *lab)
{
	return llvm::isa<CLFlushLabel>(lab) ||
		(llvm::isa<MemAccessLabel>(lab) && llvm::dyn_cast<MemAccessLabel>(lab)->getAddr().isDurable() &&
		 llvm::isa<WriteLabel>(lab));
}

SAddr getLoc(const EventLabel *lab)
{
	if (auto *fLab = llvm::dyn_cast<CLFlushLabel>(lab))
		return fLab->getAddr();
	if (auto *wLab = llvm::dyn_cast<MemAccessLabel>(lab))
		if (wLab->getAddr().isDurable())
			return wLab->getAddr();
	BUG();
	return SAddr();
}

bool TSOChecker::visitHbloc(const EventLabel *lab, const SAddr &loc, VSet<Event> &visited)
{
	if (visited.count(lab->getPos()) || lab->getThread() == g.getRecoveryRoutineId())
		return true;

	auto &g = getGraph();
	visited.insert(lab->getPos());

	for (auto &p : lab->calculated(0)) {
		auto *pLab = g.getEventLabel(p);
		if (isPbValid(pLab) && getLoc(pLab) == loc) {
			auto &node = visitedRecovery8[pLab->getStamp()];
			if (node.status == NodeStatus::unseen && !visitRecovery8(p))
				return false;
			else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
				return false;
		}
		if (!visitHbloc(pLab, loc, visited))
			return false;
	}
	for (auto &p : rfe_preds(g, lab->getPos())) {
		auto *pLab = g.getEventLabel(p);
		if (isPbValid(pLab) && getLoc(pLab) == loc) {
			auto &node = visitedRecovery8[pLab->getStamp()];
			if (node.status == NodeStatus::unseen && !visitRecovery8(p))
				return false;
			else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
				return false;
		}
		if (!visitHbloc(pLab, loc, visited))
			return false;
	}
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto *pLab = g.getEventLabel(p);
		if (isPbValid(pLab) && getLoc(pLab) == loc) {
			auto &node = visitedRecovery8[pLab->getStamp()];
			if (node.status == NodeStatus::unseen && !visitRecovery8(p))
				return false;
			else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
				return false;
		}
		if (!visitHbloc(pLab, loc, visited))
			return false;
	}
	for (auto &p : fr_imm_preds(g, lab->getPos())) {
		auto *pLab = g.getEventLabel(p);
		if (isPbValid(pLab) && getLoc(pLab) == loc) {
			auto &node = visitedRecovery8[pLab->getStamp()];
			if (node.status == NodeStatus::unseen && !visitRecovery8(p))
				return false;
			else if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)
				return false;
		}
		if (!visitHbloc(pLab, loc, visited))
			return false;
	}
//	visited.erase(lab->getPos());
	return true;
}

bool TSOChecker::visitHbloc(const Event &e)
{
       auto &g = getGraph();
       auto *lab = g.getEventLabel(e);
       if (!isPbValid(lab))
               return true;

       auto loc = getLoc(lab);
       VSet<Event> visited;
       return visitHbloc(lab, loc, visited);
}

bool TSOChecker::isRecAcyclic(const Event &e)
{
	visitedRecAccepting = 0;
	visitedRecovery0.clear();
	visitedRecovery0.resize(g.getMaxStamp() + 1);
	visitedRecovery1.clear();
	visitedRecovery1.resize(g.getMaxStamp() + 1);
	visitedRecovery2.clear();
	visitedRecovery2.resize(g.getMaxStamp() + 1);
	visitedRecovery3.clear();
	visitedRecovery3.resize(g.getMaxStamp() + 1);
	visitedRecovery4.clear();
	visitedRecovery4.resize(g.getMaxStamp() + 1);
	visitedRecovery5.clear();
	visitedRecovery5.resize(g.getMaxStamp() + 1);
	visitedRecovery6.clear();
	visitedRecovery6.resize(g.getMaxStamp() + 1);
	visitedRecovery7.clear();
	visitedRecovery7.resize(g.getMaxStamp() + 1);
	visitedRecovery8.clear();
	visitedRecovery8.resize(g.getMaxStamp() + 1);
	return true
		&& visitRecovery0(e)
		&& visitRecovery1(e)
		&& visitRecovery2(e)
		&& visitRecovery3(e)
		&& visitRecovery4(e)
		&& visitRecovery5(e)
		&& visitRecovery6(e)
		&& visitRecovery7(e)
		&& visitRecovery8(e);
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

	visitedPPoRf0[lab->getStamp()] = NodeStatus::entered;
	pporf.updateIdx(e);
	for (auto &p : lab->calculated(0)) {
		auto status = visitedPPoRf0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitPPoRf0(p, pporf);
	}
	for (auto &p : tc_preds(g, lab->getPos())) {
		auto status = visitedPPoRf0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitPPoRf0(p, pporf);
	}
	for (auto &p : tj_preds(g, lab->getPos())) {
		auto status = visitedPPoRf0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitPPoRf0(p, pporf);
	}
	for (auto &p : rfe_preds(g, lab->getPos())) {
		auto status = visitedPPoRf0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitPPoRf0(p, pporf);
	}
	visitedPPoRf0[lab->getStamp()] = NodeStatus::left;
}

DepView TSOChecker::calcPPoRfBefore(const Event &e)
{
	DepView pporf;
	pporf.updateIdx(e);
	visitedPPoRf0.clear();
	visitedPPoRf0.resize(g.getMaxStamp() + 1, NodeStatus::unseen);

	visitPPoRf0(e, pporf);
	return pporf;
}
std::unique_ptr<VectorClock> TSOChecker::getPPoRfBefore(const Event &e)
{
	return LLVM_MAKE_UNIQUE<DepView>(calcPPoRfBefore(e));
}
