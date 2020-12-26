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

#include "BisimilarityCheckerPass.hpp"
#include "Error.hpp"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Instructions.h>

using namespace llvm;
using BsPoint = BisimilarityCheckerPass::BisimilarityPoint;
using Constraint = std::pair<Value *, Value *>;

/*
 * A possible bisimilarity point: p is a bisimilarity point
 * iff constraints.empty() holds
 */
struct ConstrainedBsPoint {
	BsPoint p;
	std::vector<Constraint> constraints;

	ConstrainedBsPoint(BsPoint p) : p(p), constraints() {}
	ConstrainedBsPoint(BsPoint p, const std::vector<Constraint> &cs) : p(p), constraints(cs) {}
};

void BisimilarityCheckerPass::getAnalysisUsage(llvm::AnalysisUsage &au) const
{
	au.setPreservesAll();
}

bool BisimilarityCheckerPass::doInitialization(Module &M)
{
	funcBsPoints.clear();
	return false;
}

/* Given a list of candidates, returns the ones that are satisfiables */
std::vector<BsPoint> getSatisfiableCandidates(const std::vector<ConstrainedBsPoint> &candidates)
{
	std::vector<BsPoint> bsPoints;

	for (auto &c : candidates) {
		if (c.constraints.empty())
			bsPoints.push_back(c.p);
	}
	return bsPoints;
}

/*
 * Returns whether the bisimilarity point BSP will satisfy the constraint C.
 * (We can later refine this using something similar to FunctionComparator.)
 */
bool solvesConstraint(const BsPoint &bsp, const Constraint &c)
{
	if (auto *c1 = llvm::dyn_cast<Instruction>(c.first))
		if (auto *c2 = llvm::dyn_cast<Instruction>(c.second))
			return std::make_pair(c1, c2) == bsp;
	return false;
}

/*
 * Given a list of constrained bisimilarity points (CANDIDATES) and a bisimilarity point (BSP),
 * filters from CANDIDATES all constraints that are satisfied by BSP.
 */
void filterCandidateConstraints(std::vector<ConstrainedBsPoint> &candidates, BsPoint &bsp)
{
	for (auto &c : candidates) {
		c.constraints.erase(std::remove_if(c.constraints.begin(), c.constraints.end(),
						   [&](Constraint &cst){ return solvesConstraint(bsp, cst); }),
				    c.constraints.end());
	}
	return;
}

/*
 * Adds a constrained bisimilarity point to the candidate list; if the same point already exists,
 * the constraint list is modified accordingly
 */
void addConstrainedBsPoint(const BsPoint &bsp, const Constraint &c, std::vector<ConstrainedBsPoint> &candidates)
{
	for (auto &cnd : candidates) {
		if (cnd.p == bsp) {
			if (c.first != nullptr && c.second != nullptr)
				cnd.constraints.push_back(c);
			return;
		}
	}
	if (c.first == nullptr || c.second == nullptr)
		candidates.push_back(ConstrainedBsPoint(bsp));
	else
		candidates.push_back(ConstrainedBsPoint(bsp, {c}));
}

/*
 * Given two (similar) instructions A and B, this function will add a
 * constrained bisimilarity point for each operand that differs
 * between A and B.
 */
void addConstrainedBsPointOps(const BsPoint &bsp,
			      Instruction *a,
			      Instruction *b,
			      std::vector<ConstrainedBsPoint> &candidates)
{
	for (auto i = 0u; i < a->getNumOperands(); i++) {
		auto *opA = a->getOperand(i);
		auto *opB = b->getOperand(i);

		if (auto *cA = dyn_cast<Constant>(opA)) {
			if (cA == opB)
				continue;
			return;
		}
		addConstrainedBsPoint(bsp, std::make_pair(opA, opB), candidates);
	}
}

void calcBsPointCandidates(Instruction *a,
			   Instruction *b,
			   std::vector<ConstrainedBsPoint> &candidates)
{
	/* Make sure a and b are valid instructions */
	if (!a || !b)
		return;

	bool similar = a->isSameOperationAs(b);
	bool identical = a->isIdenticalTo(b);

	/* Case 1: a = b */
	if (identical) {
		auto bsp = BsPoint(std::make_pair(a, b));
		filterCandidateConstraints(candidates, bsp);
		addConstrainedBsPoint(bsp, {}, candidates);
		calcBsPointCandidates(a->getPrevNode(), b->getPrevNode(), candidates);
	}

	/* Case 2: a ~ b */
	if (!identical && similar) {
		auto bsp = BsPoint(std::make_pair(a, b));
		filterCandidateConstraints(candidates, bsp);
		addConstrainedBsPointOps(bsp, a, b, candidates);
		calcBsPointCandidates(a->getPrevNode(), b->getPrevNode(), candidates);
	}
	return;
}

/* Returns the bisimilarity points of a function starting from (A, B)*/
std::vector<BsPoint> getBsPoints(Instruction *a, Instruction *b)
{
	std::vector<ConstrainedBsPoint> candidates;
	calcBsPointCandidates(a, b, candidates);
	return getSatisfiableCandidates(candidates);
}

bool BisimilarityCheckerPass::runOnFunction(Function &F)
{
	for (auto bit = F.begin(), be = F.end(); bit != be; ++bit) {
		/* Only handle 2 preds for the time being */
		if (pred_size(&*bit) != 2)
			continue;

		auto b1 = *pred_begin(&*bit);     /* pred 1 */
		auto b2 = *(++pred_begin(&*bit)); /* pred 2 */

		auto ps = getBsPoints(b1->getTerminator(), b2->getTerminator());
		funcBsPoints[&F].insert(funcBsPoints[&F].end(), ps.begin(), ps.end());
	}

	auto &bsps = funcBsPoints[&F];
	std::sort(bsps.begin(), bsps.end());
	bsps.erase(std::unique(bsps.begin(), bsps.end()), bsps.end());

	return false;
}

char BisimilarityCheckerPass::ID = 42;
static llvm::RegisterPass<BisimilarityCheckerPass> P("bisimilarity-checker",
						     "Calculates bisimilar points in all functions.");
