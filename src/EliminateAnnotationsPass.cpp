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
#include "EliminateAnnotationsPass.hpp"
#include "Error.hpp"
#include "LLVMUtils.hpp"
#include "InterpreterEnumAPI.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

using namespace llvm;

void EliminateAnnotationsPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesAll();
}

bool isCASAnnotation(Instruction *i)
{
	auto *ci = llvm::dyn_cast<CallInst>(i);
	if (!ci)
		return false;

	auto name = getCalledFunOrStripValName(*ci);
	return isInternalFunction(name) && internalFunNames.at(name) == InternalFunctions::FN_AtomicCasNoRet;
}

bool eliminateCASAnnotation(CallInst *ci, VSet<Instruction *> &toDelete)
{
	if (!ci)
		return false;

	/* Get the instruction we will be annotating */
	Instruction *casi = ci;
	while (casi && !llvm::isa<AtomicCmpXchgInst>(casi))
		casi = casi->getNextNode();
	ERROR_ON(!llvm::isa<AtomicCmpXchgInst>(casi), "Misuse of helped/helping CAS annotation!\n");

	/* Gather annotation info */
	auto *funArg = llvm::dyn_cast<ConstantInt>(&*ci->getOperand(0));
	BUG_ON(!funArg);
	auto casType = funArg->getValue();

	/* Mark the instruction as "to be deleted" */
	toDelete.insert(ci);

	/* Annotate the CAS */
	auto &ctx = casi->getContext();
	auto *node = MDNode::get(ctx, MDString::get(ctx,
				      (casType.getLimitedValue() == 1) ? "helped" : "helping"));
	casi->setMetadata("cas.attr", node);
	return true;
}

bool isFAIAnnotation(Instruction *i)
{
	auto *ci = llvm::dyn_cast<CallInst>(i);
	if (!ci)
		return false;

	auto name = getCalledFunOrStripValName(*ci);
	return isInternalFunction(name) && internalFunNames.at(name) == InternalFunctions::FN_AtomicRmwNoRet;
	return false;
}

bool eliminateFAIAnnotation(CallInst *ci, VSet<Instruction *> &toDelete)
{
	if (!ci)
		return false;

	/* Get the instruction we will be annotating */
	Instruction *faii = ci;
	while (faii && !llvm::isa<AtomicRMWInst>(faii))
		faii = faii->getNextNode();
	ERROR_ON(!llvm::isa<AtomicRMWInst>(faii), "Badly annotated FAI!\n");

	/* Gather annotation info */
	auto *funArg = llvm::dyn_cast<ConstantInt>(&*ci->getOperand(0));
	BUG_ON(!funArg);
	auto faiType = funArg->getValue();
	BUG_ON(faiType.getLimitedValue() != 0); /* Only plain non-ret FAIs for now */

	/* Mark the instruction as "to be deleted" */
	toDelete.insert(ci);

	/* Annotate the CAS */
	auto &ctx = faii->getContext();
	auto *node = MDNode::get(ctx, MDString::get(ctx, "noret"));
	faii->setMetadata("fai.attr", node);
	return false;
}

bool EliminateAnnotationsPass::runOnFunction(Function &F)
{
	bool changed = false;
	VSet<Instruction *> toDelete;

	for (auto I = inst_begin(F), E = inst_end(F); I != E; ++I) {
		if (isCASAnnotation(&*I))
			changed |= eliminateCASAnnotation(llvm::dyn_cast<CallInst>(&*I), toDelete);
		else if (isFAIAnnotation(&*I))
			changed |= eliminateFAIAnnotation(llvm::dyn_cast<CallInst>(&*I), toDelete);
	}

	for (auto *i : toDelete)
		i->eraseFromParent();
	return changed;
}

Pass *createEliminateAnnotationsPass()
{
	auto *p = new EliminateAnnotationsPass();
	return p;
}

char EliminateAnnotationsPass::ID = 42;
static llvm::RegisterPass<EliminateAnnotationsPass> P("elim-annots", "Eliminates intrinsic annotations.");
