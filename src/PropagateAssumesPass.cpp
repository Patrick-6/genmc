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

#include "config.h"
#include "PropagateAssumesPass.hpp"
#include "Error.hpp"
#include "LLVMUtils.hpp"
#include "InterpreterEnumAPI.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include <numeric>

using namespace llvm;

void PropagateAssumesPass::getAnalysisUsage(AnalysisUsage &au) const
{
	au.setPreservesAll();
}

bool isAssumeFalse(Instruction *i)
{
	auto *ci = dyn_cast<CallInst>(i);
	if (!ci || !isAssumeFunction(getCalledFunOrStripValName(*ci)))
		return false;

	auto *arg = dyn_cast<ConstantInt>(ci->getOperand(0));
	return arg && arg->isZero();
}

bool jumpsOnLoadResult(Value *cond)
{
	auto *sc = stripCastsConstOps(cond);
	if (isa<LoadInst>(sc))
		return true;

	if (auto *ci = dyn_cast<CmpInst>(sc)) {
		auto *op1 = dyn_cast<LoadInst>(stripCastsConstOps(ci->getOperand(0)));
		auto *op2 = dyn_cast<LoadInst>(stripCastsConstOps(ci->getOperand(1)));
		return op1 && op2;
	}
	return false;
}

Value *getOtherSuccCondition(BranchInst *bi, BasicBlock *succ)
{
	if (bi->getSuccessor(0) != succ)
		return bi->getCondition();
	return BinaryOperator::CreateNot(bi->getCondition(), "", bi);
}

bool propagateAssumeToPred(CallInst *assume, BasicBlock *pred)
{
	auto *bi = dyn_cast<BranchInst>(pred->getTerminator());
	if (!bi || bi->isUnconditional())
		return false;

	auto *bb = assume->getParent();
	auto assumeName = getCalledFunOrStripValName(*assume);
        auto *endFun = bb->getParent()->getParent()->getFunction(assumeName);
	BUG_ON(!endFun);

	/* Get the condition that we need to ensure; if it is a user
	 * assumption cast to the exposed type too */
	auto *cond = getOtherSuccCondition(bi, bb);
	if (assumeName == "__VERIFIER_assume") {
		auto *int32Ty = Type::getInt32Ty(bb->getParent()->getParent()->getContext());
		cond = CastInst::CreateZExtOrBitCast(cond, int32Ty, "", bi);
	}

	/* Ensure the condition */
        auto *ci = CallInst::Create(endFun, {cond}, "", bi);
        ci->setMetadata("dbg", bi->getMetadata("dbg"));
	return true;
}

bool propagateAssume(CallInst *assume)
{
	auto *bb = assume->getParent();
	return std::accumulate(pred_begin(bb), pred_end(bb), false,
			       [&](const bool &accum, BasicBlock *p){
				       return accum || propagateAssumeToPred(assume, p);
			       });
}

bool PropagateAssumesPass::runOnFunction(llvm::Function &F)
{
	auto modified = false;

	for (auto &bb : F)
		if (isAssumeFalse(&*bb.begin()))
			modified |= propagateAssume(dyn_cast<CallInst>(&*bb.begin()));
	return modified;
}

Pass *createPropagateAssumesPass()
{
	auto *p = new PropagateAssumesPass();
	return p;
}

char PropagateAssumesPass::ID = 42;
static llvm::RegisterPass<PropagateAssumesPass> P("propagate-assumes",
						  "Propagates assume(0) upwards.");
