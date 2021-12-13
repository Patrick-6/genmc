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
	auto *intTy = Type::getInt32Ty(succ->getParent()->getParent()->getContext());
	auto *ci = CastInst::CreateZExtOrBitCast(bi->getCondition(), intTy, "", bi);
	if (bi->getSuccessor(0) != succ)
		return ci;
	return BinaryOperator::CreateNot(ci, "", bi);
}

bool propagateAssumeToPred(BasicBlock *bb, BasicBlock *pred)
{
	auto *bi = dyn_cast<BranchInst>(pred->getTerminator());
	if (!bi || bi->isUnconditional())
		return false;

        auto *endFun = bb->getParent()->getParent()->getFunction("__VERIFIER_assume");
	BUG_ON(!endFun);

	auto *cond = getOtherSuccCondition(bi, bb);
        auto *ci = CallInst::Create(endFun, {cond}, "", bi);
        ci->setMetadata("dbg", bi->getMetadata("dbg"));
	return true;
}

bool propagateAssume(BasicBlock *bb)
{
	return std::accumulate(pred_begin(bb), pred_end(bb), false,
			       [&](const bool &accum, BasicBlock *p){
				       return accum || propagateAssumeToPred(bb, p);
			       });
}

bool PropagateAssumesPass::runOnFunction(llvm::Function &F)
{
	auto modified = false;

	for (auto &bb : F)
		if (isAssumeFalse(&*bb.begin()))
			modified |= propagateAssume(&bb);
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
