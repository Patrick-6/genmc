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

#include "FunctionInlinerPass.hpp"
#include "Runtime/InterpreterEnumAPI.hpp"
#include <llvm/ADT/SCCIterator.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Transforms/Utils/Cloning.h>

using namespace llvm;

/**
 * Do not inline any functions that are (mutually) recursive.
 *
 * Example transforms that are permitted (below only f1 will be inlined):
 * f->f1->f2->f3->f4->f2->f3->f4... => f->f2->f3->f4->f2->f3->f4
 * f->f1->f2->f2->f2->... => main->f2->f2->f2...
 */
static auto isRecursive(CallGraph &CG, Function &F) -> bool
{
	for (auto sccIt = scc_begin(&CG); !sccIt.isAtEnd(); ++sccIt) {
		if (std::ranges::find(*sccIt, CG[&F]) != sccIt->end() && sccIt.hasCycle())
			return true;
	}
	return false;
}

static auto isInlinable(CallGraph &CG, Function &F) -> bool
{
	return !F.isDeclaration() && !isInternalFunction(F.getName().str()) && !isRecursive(CG, F);
}

static auto inlineCall(CallBase *cb) -> bool
{
	llvm::InlineFunctionInfo ifi;

#if LLVM_VERSION_MAJOR >= 11
	return InlineFunction(*cb, ifi).isSuccess();
#else
	return InlineFunction(*cb, ifi);
#endif
}

static auto inlineFunction(Module &M, Function *toInline) -> bool
{
	std::vector<CallBase *> calls;
	for (auto &F : M) {
		if (&F == toInline) /* No need to inline calls to itself */
			continue;

		for (auto &iit : instructions(F)) {
			if (!isa<InvokeInst>(&iit) && !isa<CallInst>(&iit))
				continue;

			auto *cb = dyn_cast<CallBase>(&iit);
			if (cb->getCalledFunction() == toInline)
				calls.push_back(cb);
		}
	}

	auto changed = false;
	for (auto *ci : calls) {
		changed |= inlineCall(ci);
	}
	return changed;
}

auto FunctionInlinerPass::run(Module &M, ModuleAnalysisManager &AM) -> PreservedAnalyses
{
	CallGraph CG(M);

	auto changed = false;
	for (auto &F : M) {
		/* Don't try on functions with empty bodies, external declarations, GenMCs own
		 * functions and (mutually) recursive functions */
		if (!F.empty() && isInlinable(CG, F))
			changed |= inlineFunction(M, &F);
	}
	return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
