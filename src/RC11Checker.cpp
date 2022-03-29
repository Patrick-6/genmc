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

#include "RC11Checker.hpp"

void RC11Checker::visitCalc0_0(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedCalc0_0[lab->getStamp()] = NodeStatus::entered;
	calcRes.insert(e);
	for (const auto &p : lab->calculated(0)) {
		calcRes.erase(p);
		visitedCalc0_0[g.getEventLabel(p)->getStamp()] = NodeStatus::left;
		visitedCalc0_1[g.getEventLabel(p)->getStamp()] = NodeStatus::left;
		visitedCalc0_6[g.getEventLabel(p)->getStamp()] = NodeStatus::left;
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_2[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_2(p, calcRes);
	}
	visitedCalc0_0[lab->getStamp()] = NodeStatus::left;
}

void RC11Checker::visitCalc0_1(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedCalc0_1[lab->getStamp()] = NodeStatus::entered;
	if (auto p = lab->getPos(); lab->isAtLeastRelease()) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	if (auto p = lab->getPos(); lab->isAtLeastRelease()) {
		auto status = visitedCalc0_1[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_1(p, calcRes);
	}
	if (auto p = lab->getPos(); lab->isAtLeastRelease()) {
		auto status = visitedCalc0_6[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_6(p, calcRes);
	}
	visitedCalc0_1[lab->getStamp()] = NodeStatus::left;
}

void RC11Checker::visitCalc0_2(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedCalc0_2[lab->getStamp()] = NodeStatus::entered;
	if (auto p = lab->getPos(); llvm::isa<FenceLabel>(lab)) {
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

void RC11Checker::visitCalc0_3(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedCalc0_3[lab->getStamp()] = NodeStatus::entered;
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto status = visitedCalc0_1[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_1(p, calcRes);
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto status = visitedCalc0_6[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_6(p, calcRes);
	}
	visitedCalc0_3[lab->getStamp()] = NodeStatus::left;
}

void RC11Checker::visitCalc0_4(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedCalc0_4[lab->getStamp()] = NodeStatus::entered;
	if (auto p = lab->getPos(); g.isRMWLoad(lab)) {
		auto status = visitedCalc0_3[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_3(p, calcRes);
	}
	visitedCalc0_4[lab->getStamp()] = NodeStatus::left;
}

void RC11Checker::visitCalc0_5(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedCalc0_5[lab->getStamp()] = NodeStatus::entered;
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_4[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_4(p, calcRes);
	}
	visitedCalc0_5[lab->getStamp()] = NodeStatus::left;
}

void RC11Checker::visitCalc0_6(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedCalc0_6[lab->getStamp()] = NodeStatus::entered;
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_2[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_2(p, calcRes);
	}
	if (auto p = lab->getPos(); g.isRMWStore(lab)) {
		auto status = visitedCalc0_5[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc0_5(p, calcRes);
	}
	visitedCalc0_6[lab->getStamp()] = NodeStatus::left;
}

VSet<Event> RC11Checker::calculate0(const Event &e)
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
	visitedCalc0_6.clear();
	visitedCalc0_6.resize(g.getMaxStamp() + 1, NodeStatus::unseen);

	getGraph().getEventLabel(e)->setCalculated({{}, {}, {}, });
	visitCalc0_0(e, calcRes);
	visitCalc0_1(e, calcRes);
	visitCalc0_6(e, calcRes);
	return calcRes;
}
void RC11Checker::visitCalc1_0(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedCalc1_0[lab->getStamp()] = NodeStatus::entered;
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc1_3[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_3(p, calcRes);
	}
	if (auto p = lab->getPos(); lab->isAtLeastAcquire() && llvm::isa<FenceLabel>(lab)) {
		auto status = visitedCalc1_4[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_4(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc1_5[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_5(p, calcRes);
	}
	visitedCalc1_0[lab->getStamp()] = NodeStatus::left;
}

void RC11Checker::visitCalc1_1(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedCalc1_1[lab->getStamp()] = NodeStatus::entered;
	for (auto &p : lab->calculated(0)) {
		auto status = visitedCalc1_5[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_5(p, calcRes);
	}
	visitedCalc1_1[lab->getStamp()] = NodeStatus::left;
}

void RC11Checker::visitCalc1_2(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedCalc1_2[lab->getStamp()] = NodeStatus::entered;
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto status = visitedCalc1_1[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_1(p, calcRes);
	}
	visitedCalc1_2[lab->getStamp()] = NodeStatus::left;
}

void RC11Checker::visitCalc1_3(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedCalc1_3[lab->getStamp()] = NodeStatus::entered;
	if (auto p = lab->getPos(); lab->isAtLeastAcquire()) {
		auto status = visitedCalc1_2[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_2(p, calcRes);
	}
	visitedCalc1_3[lab->getStamp()] = NodeStatus::left;
}

void RC11Checker::visitCalc1_4(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedCalc1_4[lab->getStamp()] = NodeStatus::entered;
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc1_4[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_4(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc1_2[g.getEventLabel(p)->getStamp()];
		if (status == NodeStatus::unseen)
			visitCalc1_2(p, calcRes);
	}
	visitedCalc1_4[lab->getStamp()] = NodeStatus::left;
}

void RC11Checker::visitCalc1_5(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedCalc1_5[lab->getStamp()] = NodeStatus::entered;
	calcRes.insert(e);
	for (const auto &p : lab->calculated(1)) {
		calcRes.erase(p);
		visitedCalc1_0[g.getEventLabel(p)->getStamp()] = NodeStatus::left;
	}
	visitedCalc1_5[lab->getStamp()] = NodeStatus::left;
}

VSet<Event> RC11Checker::calculate1(const Event &e)
{
	VSet<Event> calcRes;
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

	getGraph().getEventLabel(e)->setCalculated({{}, });
	visitCalc1_0(e, calcRes);
	return calcRes;
}
std::vector<VSet<Event>> RC11Checker::calculateAll(const Event &e)
{
	calculated.push_back(calculate0(e));
	calculated.push_back(calculate1(e));
	return std::move(calculated);
}

bool RC11Checker::visitAcyclic0(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	++visitedAccepting;
	visitedAcyclic0[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	if (auto p = lab->getPos(); lab->isSC() && llvm::isa<FenceLabel>(lab)) {
		auto &node = visitedAcyclic21[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic21(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = lab->getPos(); lab->isSC()) {
		auto &node = visitedAcyclic19[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic19(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = lab->getPos(); lab->isSC()) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = lab->getPos(); lab->isSC()) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	--visitedAccepting;
	visitedAcyclic0[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic1(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic1[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	if (auto p = lab->getPos(); lab->isSC()) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic1[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic2(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic2[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic2[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic2(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic2[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic3(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic3[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic3[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic3(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : lab->calculated(1)) {
		auto &node = visitedAcyclic2[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic2(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = lab->getPos(); lab->isAtLeastAcquire()) {
		auto &node = visitedAcyclic6[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic3[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic4(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic4[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : lab->calculated(1)) {
		auto &node = visitedAcyclic2[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic2(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic4[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic5(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic5[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic2[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic2(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic4[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic4(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic5[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic6(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic6[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic6[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic7(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic7[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	if (auto p = lab->getPos(); llvm::isa<FenceLabel>(lab)) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic7[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic8(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic8[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : lab->calculated(1)) {
		auto &node = visitedAcyclic7[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic7(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = lab->getPos(); lab->isAtLeastAcquire()) {
		auto &node = visitedAcyclic11[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic11(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic8[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic9(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic9[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : lab->calculated(1)) {
		auto &node = visitedAcyclic7[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic7(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic9[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic10(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic10[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic9[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic9(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic7[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic7(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic10[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic11(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic11[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic10[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic10(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic11[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic12(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic12[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic12[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic13(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic13[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_init_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic14[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic14(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_init_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic13[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic14(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic14[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rf_succs(g, lab->getPos())) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_init_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic14[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic14(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_succs(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_init_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic14[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic15(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic15[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : lab->calculated(1)) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : lab->calculated(1)) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = lab->getPos(); lab->isAtLeastAcquire()) {
		auto &node = visitedAcyclic18[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic18(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic15[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic16(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic16[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : co_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_init_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : lab->calculated(1)) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : lab->calculated(1)) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic14[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic14(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_init_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic16[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic17(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic17[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic16[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic16(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic17[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic18(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic18[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic17[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic17(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic18[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic19(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic19[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic3[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic3(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_init_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic14[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic14(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = lab->getPos(); llvm::isa<FenceLabel>(lab)) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_init_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic19[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic20(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic20[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	if (auto p = lab->getPos(); lab->isSC() && llvm::isa<FenceLabel>(lab)) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic20[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic21(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic21[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : lab->calculated(1)) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (auto p = lab->getPos(); lab->isAtLeastAcquire()) {
		auto &node = visitedAcyclic24[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic24(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : lab->calculated(1)) {
		auto &node = visitedAcyclic20[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic20(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic21[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic22(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic22[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : lab->calculated(1)) {
		auto &node = visitedAcyclic20[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic20(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic22[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic23(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic23[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic22[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic22(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic20[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic20(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic23[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic24(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic24[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic30[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic30(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic23[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic23(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic24[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic25(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic25[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	if (auto p = lab->getPos(); lab->isAtLeastAcquire()) {
		auto &node = visitedAcyclic26[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic26(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_init_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_init_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : lab->calculated(1)) {
		auto &node = visitedAcyclic20[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic20(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic25[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic26(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic26[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic23[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic23(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic26[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic27(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic27[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rf_succs(g, lab->getPos())) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_init_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_succs(g, lab->getPos())) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_init_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic27[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic28(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic28[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_init_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_init_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic27[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic27(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic28[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic29(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic29[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : lab->calculated(1)) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_init_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : co_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : fr_init_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic25[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic25(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic29[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic30(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedAcyclic30[lab->getStamp()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic28[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic28(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic29[g.getEventLabel(p)->getStamp()];
		if (node.status == NodeStatus::unseen && !visitAcyclic29(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic30[lab->getStamp()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::isAcyclic(const Event &e)
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
	visitedAcyclic16.clear();
	visitedAcyclic16.resize(g.getMaxStamp() + 1);
	visitedAcyclic17.clear();
	visitedAcyclic17.resize(g.getMaxStamp() + 1);
	visitedAcyclic18.clear();
	visitedAcyclic18.resize(g.getMaxStamp() + 1);
	visitedAcyclic19.clear();
	visitedAcyclic19.resize(g.getMaxStamp() + 1);
	visitedAcyclic20.clear();
	visitedAcyclic20.resize(g.getMaxStamp() + 1);
	visitedAcyclic21.clear();
	visitedAcyclic21.resize(g.getMaxStamp() + 1);
	visitedAcyclic22.clear();
	visitedAcyclic22.resize(g.getMaxStamp() + 1);
	visitedAcyclic23.clear();
	visitedAcyclic23.resize(g.getMaxStamp() + 1);
	visitedAcyclic24.clear();
	visitedAcyclic24.resize(g.getMaxStamp() + 1);
	visitedAcyclic25.clear();
	visitedAcyclic25.resize(g.getMaxStamp() + 1);
	visitedAcyclic26.clear();
	visitedAcyclic26.resize(g.getMaxStamp() + 1);
	visitedAcyclic27.clear();
	visitedAcyclic27.resize(g.getMaxStamp() + 1);
	visitedAcyclic28.clear();
	visitedAcyclic28.resize(g.getMaxStamp() + 1);
	visitedAcyclic29.clear();
	visitedAcyclic29.resize(g.getMaxStamp() + 1);
	visitedAcyclic30.clear();
	visitedAcyclic30.resize(g.getMaxStamp() + 1);
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
		&& visitAcyclic15(e)
		&& visitAcyclic16(e)
		&& visitAcyclic17(e)
		&& visitAcyclic18(e)
		&& visitAcyclic19(e)
		&& visitAcyclic20(e)
		&& visitAcyclic21(e)
		&& visitAcyclic22(e)
		&& visitAcyclic23(e)
		&& visitAcyclic24(e)
		&& visitAcyclic25(e)
		&& visitAcyclic26(e)
		&& visitAcyclic27(e)
		&& visitAcyclic28(e)
		&& visitAcyclic29(e)
		&& visitAcyclic30(e);
}

bool RC11Checker::isConsistent(const Event &e)
{
	return isAcyclic(e);
}

