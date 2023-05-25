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
	auto t = 0u;

	visitedCalc0_0[lab->getStamp().get()] = NodeStatus::entered;
	calcRes.insert(e);
	for (const auto &p : lab->calculated(0)) {
		calcRes.erase(p);
		visitedCalc0_6[g.getEventLabel(p)->getStamp().get()] = NodeStatus::left;
	}
	for (auto &p : lab->calculated(0)) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	visitedCalc0_0[lab->getStamp().get()] = NodeStatus::left;
}

void RC11Checker::visitCalc0_1(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_1[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_1[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_1(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_3[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_3(p, calcRes);
	}
	visitedCalc0_1[lab->getStamp().get()] = NodeStatus::left;
}

void RC11Checker::visitCalc0_2(const Event &e, VSet<Event> &calcRes)
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

void RC11Checker::visitCalc0_3(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_3[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto status = visitedCalc0_2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_2(p, calcRes);
	}
	for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastRelease()) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto status = visitedCalc0_4[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_4(p, calcRes);
	}
	visitedCalc0_3[lab->getStamp().get()] = NodeStatus::left;
}

void RC11Checker::visitCalc0_4(const Event &e, VSet<Event> &calcRes)
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

void RC11Checker::visitCalc0_5(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_5[lab->getStamp().get()] = NodeStatus::entered;
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos())) {
		auto status = visitedCalc0_2[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_2(p, calcRes);
	}
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
		auto status = visitedCalc0_4[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_4(p, calcRes);
	}
	visitedCalc0_5[lab->getStamp().get()] = NodeStatus::left;
}

void RC11Checker::visitCalc0_6(const Event &e, VSet<Event> &calcRes)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedCalc0_6[lab->getStamp().get()] = NodeStatus::entered;
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
	if (llvm::isa<FenceLabel>(lab) && lab->isAtLeastAcquire() && !lab->isAtLeastRelease() || llvm::isa<FenceLabel>(lab) && lab->isAtLeastAcquire() && lab->isAtLeastRelease() || llvm::isa<FenceLabel>(lab) && lab->isSC())for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_3[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_3(p, calcRes);
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedCalc0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitCalc0_0(p, calcRes);
	}
	visitedCalc0_6[lab->getStamp().get()] = NodeStatus::left;
}

VSet<Event> RC11Checker::calculate0(const Event &e)
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
	visitedCalc0_4.clear();
	visitedCalc0_4.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedCalc0_5.clear();
	visitedCalc0_5.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedCalc0_6.clear();
	visitedCalc0_6.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);

	getGraph().getEventLabel(e)->setCalculated({{}, });
	visitCalc0_6(e, calcRes);
	return calcRes;
}
std::vector<VSet<Event>> RC11Checker::calculateSaved(const Event &e)
{
	saved.push_back(calculate0(e));
	return std::move(saved);
}

std::vector<View> RC11Checker::calculateViews(const Event &e)
{
	return std::move(views);
}

void RC11Checker::visitInclusionLHS0_0(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedInclusionLHS0_0[lab->getStamp().get()] = NodeStatus::entered;
	lhsAccept0[lab->getStamp().get()] = true;
	visitedInclusionLHS0_0[lab->getStamp().get()] = NodeStatus::left;
}

void RC11Checker::visitInclusionLHS0_1(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedInclusionLHS0_1[lab->getStamp().get()] = NodeStatus::entered;
	if (llvm::isa<ReadLabel>(lab) && llvm::dyn_cast<ReadLabel>(lab)->getRf().isInitializer() && llvm::dyn_cast<ReadLabel>(lab)->getAddr().isDynamic())if (auto p = lab->getPos(); true) {
		auto status = visitedInclusionLHS0_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitInclusionLHS0_0(p);
	}
	visitedInclusionLHS0_1[lab->getStamp().get()] = NodeStatus::left;
}

bool RC11Checker::checkInclusion0(const Event &e)
{
	visitedInclusionLHS0_0.clear();
	visitedInclusionLHS0_0.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedInclusionLHS0_1.clear();
	visitedInclusionLHS0_1.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	lhsAccept0.clear();
	lhsAccept0.resize(g.getMaxStamp().get() + 1, false);
	rhsAccept0.clear();
	rhsAccept0.resize(g.getMaxStamp().get() + 1, false);

	visitInclusionLHS0_1(e);
	for (auto i = 0u; i < lhsAccept0.size(); i++) {
		if (lhsAccept0[i] && !rhsAccept0[i])
			return false;
	}
	return true;
}

void RC11Checker::visitInclusionLHS1_0(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedInclusionLHS1_0[lab->getStamp().get()] = NodeStatus::entered;
	lhsAccept1[lab->getStamp().get()] = true;
	visitedInclusionLHS1_0[lab->getStamp().get()] = NodeStatus::left;
}

void RC11Checker::visitInclusionLHS1_1(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedInclusionLHS1_1[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : allocs(g, lab->getPos())) {
		auto status = visitedInclusionLHS1_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitInclusionLHS1_0(p);
	}
	visitedInclusionLHS1_1[lab->getStamp().get()] = NodeStatus::left;
}

void RC11Checker::visitInclusionRHS1_0(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedInclusionRHS1_0[lab->getStamp().get()] = NodeStatus::entered;
	rhsAccept1[lab->getStamp().get()] = true;
	visitedInclusionRHS1_0[lab->getStamp().get()] = NodeStatus::left;
}

void RC11Checker::visitInclusionRHS1_1(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedInclusionRHS1_1[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : lab->calculated(0)) {
		auto status = visitedInclusionRHS1_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitInclusionRHS1_0(p);
	}
	for (auto &p : lab->calculated(0)) {
		auto status = visitedInclusionRHS1_1[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitInclusionRHS1_1(p);
	}
	visitedInclusionRHS1_1[lab->getStamp().get()] = NodeStatus::left;
}

bool RC11Checker::checkInclusion1(const Event &e)
{
	visitedInclusionLHS1_0.clear();
	visitedInclusionLHS1_0.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedInclusionLHS1_1.clear();
	visitedInclusionLHS1_1.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedInclusionRHS1_0.clear();
	visitedInclusionRHS1_0.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedInclusionRHS1_1.clear();
	visitedInclusionRHS1_1.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	lhsAccept1.clear();
	lhsAccept1.resize(g.getMaxStamp().get() + 1, false);
	rhsAccept1.clear();
	rhsAccept1.resize(g.getMaxStamp().get() + 1, false);

	visitInclusionLHS1_1(e);
	visitInclusionRHS1_1(e);
	for (auto i = 0u; i < lhsAccept1.size(); i++) {
		if (lhsAccept1[i] && !rhsAccept1[i])
			return false;
	}
	return true;
}

void RC11Checker::visitInclusionLHS2_0(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedInclusionLHS2_0[lab->getStamp().get()] = NodeStatus::entered;
	lhsAccept2[lab->getStamp().get()] = true;
	visitedInclusionLHS2_0[lab->getStamp().get()] = NodeStatus::left;
}

void RC11Checker::visitInclusionLHS2_1(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedInclusionLHS2_1[lab->getStamp().get()] = NodeStatus::entered;
	if (llvm::isa<HpRetireLabel>(lab))for (auto &p : samelocs(g, lab->getPos()))if (llvm::isa<HpRetireLabel>(g.getEventLabel(p))) {
		auto status = visitedInclusionLHS2_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitInclusionLHS2_0(p);
	}
	if (llvm::isa<HpRetireLabel>(lab))for (auto &p : samelocs(g, lab->getPos()))if (llvm::isa<FreeLabel>(g.getEventLabel(p)) && !llvm::isa<HpRetireLabel>(g.getEventLabel(p))) {
		auto status = visitedInclusionLHS2_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitInclusionLHS2_0(p);
	}
	if (llvm::isa<FreeLabel>(lab) && !llvm::isa<HpRetireLabel>(lab))for (auto &p : samelocs(g, lab->getPos()))if (llvm::isa<HpRetireLabel>(g.getEventLabel(p))) {
		auto status = visitedInclusionLHS2_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitInclusionLHS2_0(p);
	}
	if (llvm::isa<FreeLabel>(lab) && !llvm::isa<HpRetireLabel>(lab))for (auto &p : samelocs(g, lab->getPos()))if (llvm::isa<FreeLabel>(g.getEventLabel(p)) && !llvm::isa<HpRetireLabel>(g.getEventLabel(p))) {
		auto status = visitedInclusionLHS2_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitInclusionLHS2_0(p);
	}
	visitedInclusionLHS2_1[lab->getStamp().get()] = NodeStatus::left;
}

bool RC11Checker::checkInclusion2(const Event &e)
{
	visitedInclusionLHS2_0.clear();
	visitedInclusionLHS2_0.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedInclusionLHS2_1.clear();
	visitedInclusionLHS2_1.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	lhsAccept2.clear();
	lhsAccept2.resize(g.getMaxStamp().get() + 1, false);
	rhsAccept2.clear();
	rhsAccept2.resize(g.getMaxStamp().get() + 1, false);

	visitInclusionLHS2_1(e);
	for (auto i = 0u; i < lhsAccept2.size(); i++) {
		if (lhsAccept2[i] && !rhsAccept2[i])
			return false;
	}
	return true;
}

void RC11Checker::visitInclusionLHS3_0(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedInclusionLHS3_0[lab->getStamp().get()] = NodeStatus::entered;
	lhsAccept3[lab->getStamp().get()] = true;
	visitedInclusionLHS3_0[lab->getStamp().get()] = NodeStatus::left;
}

void RC11Checker::visitInclusionLHS3_1(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedInclusionLHS3_1[lab->getStamp().get()] = NodeStatus::entered;
	if (llvm::isa<FreeLabel>(lab) && !llvm::isa<HpRetireLabel>(lab))for (auto &p : samelocs(g, lab->getPos())) {
		auto status = visitedInclusionLHS3_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitInclusionLHS3_0(p);
	}
	visitedInclusionLHS3_1[lab->getStamp().get()] = NodeStatus::left;
}

void RC11Checker::visitInclusionRHS3_0(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedInclusionRHS3_0[lab->getStamp().get()] = NodeStatus::entered;
	rhsAccept3[lab->getStamp().get()] = true;
	visitedInclusionRHS3_0[lab->getStamp().get()] = NodeStatus::left;
}

void RC11Checker::visitInclusionRHS3_1(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedInclusionRHS3_1[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : lab->calculated(0)) {
		auto status = visitedInclusionRHS3_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitInclusionRHS3_0(p);
	}
	for (auto &p : lab->calculated(0)) {
		auto status = visitedInclusionRHS3_1[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitInclusionRHS3_1(p);
	}
	visitedInclusionRHS3_1[lab->getStamp().get()] = NodeStatus::left;
}

bool RC11Checker::checkInclusion3(const Event &e)
{
	visitedInclusionLHS3_0.clear();
	visitedInclusionLHS3_0.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedInclusionLHS3_1.clear();
	visitedInclusionLHS3_1.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedInclusionRHS3_0.clear();
	visitedInclusionRHS3_0.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedInclusionRHS3_1.clear();
	visitedInclusionRHS3_1.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	lhsAccept3.clear();
	lhsAccept3.resize(g.getMaxStamp().get() + 1, false);
	rhsAccept3.clear();
	rhsAccept3.resize(g.getMaxStamp().get() + 1, false);

	visitInclusionLHS3_1(e);
	visitInclusionRHS3_1(e);
	for (auto i = 0u; i < lhsAccept3.size(); i++) {
		if (lhsAccept3[i] && !rhsAccept3[i])
			return false;
	}
	return true;
}

void RC11Checker::visitInclusionLHS4_0(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedInclusionLHS4_0[lab->getStamp().get()] = NodeStatus::entered;
	lhsAccept4[lab->getStamp().get()] = true;
	visitedInclusionLHS4_0[lab->getStamp().get()] = NodeStatus::left;
}

void RC11Checker::visitInclusionLHS4_1(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedInclusionLHS4_1[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : samelocs(g, lab->getPos()))if (llvm::isa<FreeLabel>(g.getEventLabel(p)) && !llvm::isa<HpRetireLabel>(g.getEventLabel(p))) {
		auto status = visitedInclusionLHS4_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitInclusionLHS4_0(p);
	}
	visitedInclusionLHS4_1[lab->getStamp().get()] = NodeStatus::left;
}

bool RC11Checker::checkInclusion4(const Event &e)
{
	visitedInclusionLHS4_0.clear();
	visitedInclusionLHS4_0.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedInclusionLHS4_1.clear();
	visitedInclusionLHS4_1.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	lhsAccept4.clear();
	lhsAccept4.resize(g.getMaxStamp().get() + 1, false);
	rhsAccept4.clear();
	rhsAccept4.resize(g.getMaxStamp().get() + 1, false);

	visitInclusionLHS4_1(e);
	for (auto i = 0u; i < lhsAccept4.size(); i++) {
		if (lhsAccept4[i] && !rhsAccept4[i])
			return false;
	}
	return true;
}

void RC11Checker::visitInclusionLHS5_0(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedInclusionLHS5_0[lab->getStamp().get()] = NodeStatus::entered;
	lhsAccept5[lab->getStamp().get()] = true;
	visitedInclusionLHS5_0[lab->getStamp().get()] = NodeStatus::left;
}

void RC11Checker::visitInclusionLHS5_1(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedInclusionLHS5_1[lab->getStamp().get()] = NodeStatus::entered;
	if (llvm::isa<ReadLabel>(lab))for (auto &p : samelocs(g, lab->getPos()))if (llvm::isa<WriteLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isNotAtomic()) {
		auto status = visitedInclusionLHS5_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitInclusionLHS5_0(p);
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : samelocs(g, lab->getPos()))if (llvm::isa<WriteLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isNotAtomic()) {
		auto status = visitedInclusionLHS5_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitInclusionLHS5_0(p);
	}
	if (llvm::isa<WriteLabel>(lab))for (auto &p : samelocs(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isNotAtomic()) {
		auto status = visitedInclusionLHS5_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitInclusionLHS5_0(p);
	}
	if (llvm::isa<WriteLabel>(lab) && lab->isNotAtomic())for (auto &p : samelocs(g, lab->getPos()))if (llvm::isa<ReadLabel>(g.getEventLabel(p))) {
		auto status = visitedInclusionLHS5_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitInclusionLHS5_0(p);
	}
	if (llvm::isa<WriteLabel>(lab) && lab->isNotAtomic())for (auto &p : samelocs(g, lab->getPos()))if (llvm::isa<WriteLabel>(g.getEventLabel(p))) {
		auto status = visitedInclusionLHS5_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitInclusionLHS5_0(p);
	}
	if (llvm::isa<ReadLabel>(lab) && lab->isNotAtomic())for (auto &p : samelocs(g, lab->getPos()))if (llvm::isa<WriteLabel>(g.getEventLabel(p))) {
		auto status = visitedInclusionLHS5_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitInclusionLHS5_0(p);
	}
	visitedInclusionLHS5_1[lab->getStamp().get()] = NodeStatus::left;
}

void RC11Checker::visitInclusionRHS5_0(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedInclusionRHS5_0[lab->getStamp().get()] = NodeStatus::entered;
	rhsAccept5[lab->getStamp().get()] = true;
	visitedInclusionRHS5_0[lab->getStamp().get()] = NodeStatus::left;
}

void RC11Checker::visitInclusionRHS5_1(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);

	visitedInclusionRHS5_1[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &p : lab->calculated(0)) {
		auto status = visitedInclusionRHS5_0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitInclusionRHS5_0(p);
	}
	for (auto &p : lab->calculated(0)) {
		auto status = visitedInclusionRHS5_1[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitInclusionRHS5_1(p);
	}
	visitedInclusionRHS5_1[lab->getStamp().get()] = NodeStatus::left;
}

bool RC11Checker::checkInclusion5(const Event &e)
{
	visitedInclusionLHS5_0.clear();
	visitedInclusionLHS5_0.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedInclusionLHS5_1.clear();
	visitedInclusionLHS5_1.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedInclusionRHS5_0.clear();
	visitedInclusionRHS5_0.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedInclusionRHS5_1.clear();
	visitedInclusionRHS5_1.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	lhsAccept5.clear();
	lhsAccept5.resize(g.getMaxStamp().get() + 1, false);
	rhsAccept5.clear();
	rhsAccept5.resize(g.getMaxStamp().get() + 1, false);

	visitInclusionLHS5_1(e);
	visitInclusionRHS5_1(e);
	for (auto i = 0u; i < lhsAccept5.size(); i++) {
		if (lhsAccept5[i] && !rhsAccept5[i])
			return false;
	}
	return true;
}

VerificationError RC11Checker::checkErrors(const Event &e)
{
	if (!checkInclusion0(e))

		 return VerificationError::VE_UninitializedMem;
	if (!checkInclusion1(e))

		 return VerificationError::VE_AccessNonMalloc;
	if (!checkInclusion2(e))

		 return VerificationError::VE_DoubleFree;
	if (!checkInclusion3(e))

		 return VerificationError::VE_AccessFreed;
	if (!checkInclusion4(e))

		 return VerificationError::VE_AccessFreed;
	if (!checkInclusion5(e))

		 return VerificationError::VE_RaceNotAtomic;
	return VerificationError::VE_OK;
}

bool RC11Checker::visitAcyclic0(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic0[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic0[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic1(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic1[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos())) {
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
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic1[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic2(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic2[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isAtLeastAcquire() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic2[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic2(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isAtLeastAcquire() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic2[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic3(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic3[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic4[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic4(p))
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
	for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastRelease()) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic3[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic4(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic4[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isAtLeastAcquire() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic2[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic2(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isAtLeastAcquire() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (g.isRMWStore(lab))for (auto &p : po_imm_preds(g, lab->getPos()))if (g.isRMWLoad(g.getEventLabel(p))) {
		auto &node = visitedAcyclic3[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic3(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic4[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic5(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic5[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic0[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic0(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic5[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic6(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic6[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
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
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic6[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic7(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic7[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic6[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic7[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic7(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic7[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic8(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic8[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
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
	for (auto &p : co_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
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
	visitedAcyclic8[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic9(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic9[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
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
	if (lab->isAtLeastAcquire())for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic12[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic12(p))
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
	visitedAcyclic9[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic10(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic10[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic10[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic10(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isAtLeastAcquire() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic10[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic11(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic11[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : rf_preds(g, lab->getPos()))if (g.getEventLabel(p)->isAtLeastRelease()) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
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
	visitedAcyclic11[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic12(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic12[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic10[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic10(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (g.isRMWStore(lab))for (auto &p : po_imm_preds(g, lab->getPos()))if (g.isRMWLoad(g.getEventLabel(p))) {
		auto &node = visitedAcyclic11[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic11(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : po_imm_preds(g, lab->getPos()))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastRelease() && !g.getEventLabel(p)->isAtLeastAcquire() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isAtLeastAcquire() && g.getEventLabel(p)->isAtLeastRelease() || llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic12[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic13(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic13[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : lab->calculated(0))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	visitedAcyclic13[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic14(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedAcyclic14[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic7[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic7(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic8[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic8(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic14[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic14(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	for (auto &p : lab->calculated(0))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
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
	for (auto &p : rf_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
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
	visitedAcyclic14[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::visitAcyclic15(const Event &e)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	++visitedAccepting;
	visitedAcyclic15[lab->getStamp().get()] = { visitedAccepting, NodeStatus::entered };
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
	if (lab->isSC())for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic6[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic6(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isSC())for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic1[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic1(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<FenceLabel>(lab) && lab->isSC())for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic7[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic7(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (lab->isSC())for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic7[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic7(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<FenceLabel>(lab) && lab->isSC())for (auto &p : lab->calculated(0)) {
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
	if (lab->isSC())for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic5[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic5(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<FenceLabel>(lab) && lab->isSC())for (auto &p : lab->calculated(0)) {
		auto &node = visitedAcyclic14[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic14(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	if (llvm::isa<FenceLabel>(lab) && lab->isSC())for (auto &p : lab->calculated(0))if (llvm::isa<FenceLabel>(g.getEventLabel(p)) && g.getEventLabel(p)->isSC()) {
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
	if (lab->isSC())for (auto &p : po_imm_preds(g, lab->getPos()))if (g.getEventLabel(p)->isSC()) {
		auto &node = visitedAcyclic15[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic15(p))
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
	if (lab->isSC())for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto &node = visitedAcyclic13[g.getEventLabel(p)->getStamp().get()];
		if (node.status == NodeStatus::unseen && !visitAcyclic13(p))
			return false;
		else if (node.status == NodeStatus::entered && visitedAccepting > node.count)
			return false;
	}
	--visitedAccepting;
	visitedAcyclic15[lab->getStamp().get()] = { visitedAccepting, NodeStatus::left };
	return true;
}

bool RC11Checker::isAcyclic(const Event &e)
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
	return true
		&& visitAcyclic0(e)
		&& visitAcyclic5(e)
		&& visitAcyclic6(e)
		&& visitAcyclic8(e)
		&& visitAcyclic9(e)
		&& visitAcyclic13(e)
		&& visitAcyclic15(e);
}

bool RC11Checker::isConsistent(const Event &e)
{
	return isAcyclic(e);
}

bool RC11Checker::isRecAcyclic(const Event &e)
{
	visitedRecAccepting = 0;
	return true;
}

bool RC11Checker::isRecoveryValid(const Event &e)
{
	return isRecAcyclic(e);
}

void RC11Checker::visitPPoRf0(const Event &e, View &pporf)
{
	auto &g = getGraph();
	auto *lab = g.getEventLabel(e);
	auto t = 0u;

	visitedPPoRf0[lab->getStamp().get()] = NodeStatus::entered;
	pporf.updateIdx(e);
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
	for (auto &p : po_imm_preds(g, lab->getPos())) {
		auto status = visitedPPoRf0[g.getEventLabel(p)->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf0(p, pporf);
	}
	visitedPPoRf0[lab->getStamp().get()] = NodeStatus::left;
}

View RC11Checker::calcPPoRfBefore(const Event &e)
{
	View pporf;
	pporf.updateIdx(e);
	visitedPPoRf0.clear();
	visitedPPoRf0.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);

	visitPPoRf0(e, pporf);
	return pporf;
}
std::unique_ptr<VectorClock> RC11Checker::getPPoRfBefore(const Event &e)
{
	return LLVM_MAKE_UNIQUE<View>(calcPPoRfBefore(e));
}

