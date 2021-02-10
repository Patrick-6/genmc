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

#include "LoadAnnotPass.hpp"
#include "Error.hpp"
#include "LLVMUtils.hpp"
#include "SExprVisitor.hpp"
#include <llvm/Analysis/PostDominators.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Function.h>

using namespace llvm;

void AnnotateLoadsPass::getAnalysisUsage(llvm::AnalysisUsage &au) const
{
	au.addRequired<PostDominatorTreeWrapperPass>();
	au.setPreservesAll();
}

std::unique_ptr<SExpr> generateOperandExpr(Value *op)
{
	if (auto *c = dyn_cast<Constant>(op)) {
		BUG_ON(!c->getType()->isIntegerTy());
		return ConcreteExpr::create(c->getUniqueInteger());
	}
	return RegisterExpr::create(op);
}

#ifdef LLVM_EXECUTIONENGINE_DATALAYOUT_PTR
# define GET_TYPE_ALLOC_SIZE(M, x)		\
	(M).getDataLayout()->getTypeAllocSize((x))
#else
# define GET_TYPE_ALLOC_SIZE(M, x)		\
	(M).getDataLayout().getTypeAllocSize((x))
#endif

std::unique_ptr<SExpr> generateInstExpr(Instruction *curr)
{
	/*
	 * Next, we try to generate an annotation for a whole bunch of instructions,
	 * apart from function calls, memory instructions, and some pointer casts.
	 * For the cases we do not handle, we simply return false.
	 */

#define HANDLE_INST(op, ...)			\
	case Instruction::op:			\
		return op##Expr::create(__VA_ARGS__)

	switch (curr->getOpcode()) {
		HANDLE_INST(ZExt, curr->getType()->getPrimitiveSizeInBits(),
			    generateOperandExpr(curr->getOperand(0)));
		HANDLE_INST(SExt, curr->getType()->getPrimitiveSizeInBits(),
			    generateOperandExpr(curr->getOperand(0)));
		HANDLE_INST(Trunc, curr->getType()->getPrimitiveSizeInBits(),
			    generateOperandExpr(curr->getOperand(0)));

		HANDLE_INST(Select, generateOperandExpr(curr->getOperand(0)),
			    generateOperandExpr(curr->getOperand(1)),
			    generateOperandExpr(curr->getOperand(2)));

		HANDLE_INST(Add, curr->getType(),
			    generateOperandExpr(curr->getOperand(0)),
			    generateOperandExpr(curr->getOperand(1)));
		HANDLE_INST(Sub, curr->getType(),
			    generateOperandExpr(curr->getOperand(0)),
			    generateOperandExpr(curr->getOperand(1)));
		HANDLE_INST(Mul, curr->getType(),
			    generateOperandExpr(curr->getOperand(0)),
			    generateOperandExpr(curr->getOperand(1)));
		HANDLE_INST(UDiv, curr->getType(),
			    generateOperandExpr(curr->getOperand(0)),
			    generateOperandExpr(curr->getOperand(1)));
		HANDLE_INST(SDiv, curr->getType(),
			    generateOperandExpr(curr->getOperand(0)),
			    generateOperandExpr(curr->getOperand(1)));
		HANDLE_INST(URem, curr->getType(),
			    generateOperandExpr(curr->getOperand(0)),
			    generateOperandExpr(curr->getOperand(1)));
		HANDLE_INST(And, curr->getType(),
			    generateOperandExpr(curr->getOperand(0)),
			    generateOperandExpr(curr->getOperand(1)));
		HANDLE_INST(Or, curr->getType(),
			    generateOperandExpr(curr->getOperand(0)),
			    generateOperandExpr(curr->getOperand(1)));
		HANDLE_INST(Xor, curr->getType(),
			    generateOperandExpr(curr->getOperand(0)),
			    generateOperandExpr(curr->getOperand(1)));
		HANDLE_INST(Shl, curr->getType(),
			    generateOperandExpr(curr->getOperand(0)),
			    generateOperandExpr(curr->getOperand(1)));
		HANDLE_INST(LShr, curr->getType(),
			    generateOperandExpr(curr->getOperand(0)),
			    generateOperandExpr(curr->getOperand(1)));
		HANDLE_INST(AShr, curr->getType(),
			    generateOperandExpr(curr->getOperand(0)),
			    generateOperandExpr(curr->getOperand(1)));

	case Instruction::ICmp: {
		llvm::CmpInst *cmpi = llvm::dyn_cast<llvm::CmpInst>(curr);
		switch(cmpi->getPredicate()) {
		case CmpInst::ICMP_EQ :
			return EqExpr::create(generateOperandExpr(curr->getOperand(0)),
					      generateOperandExpr(curr->getOperand(1)));
		case CmpInst::ICMP_NE :
			return NeExpr::create(generateOperandExpr(curr->getOperand(0)),
					      generateOperandExpr(curr->getOperand(1)));
		case CmpInst::ICMP_UGT :
			return UgtExpr::create(generateOperandExpr(curr->getOperand(0)),
					       generateOperandExpr(curr->getOperand(1)));
		case CmpInst::ICMP_UGE :
			return UgeExpr::create(generateOperandExpr(curr->getOperand(0)),
					       generateOperandExpr(curr->getOperand(1)));
		case CmpInst::ICMP_ULT :
			return UltExpr::create(generateOperandExpr(curr->getOperand(0)),
					       generateOperandExpr(curr->getOperand(1)));
		case CmpInst::ICMP_ULE :
			return UleExpr::create(generateOperandExpr(curr->getOperand(0)),
					       generateOperandExpr(curr->getOperand(1)));
		case CmpInst::ICMP_SGT :
			return SgtExpr::create(generateOperandExpr(curr->getOperand(0)),
					       generateOperandExpr(curr->getOperand(1)));
		case CmpInst::ICMP_SGE :
			return SgeExpr::create(generateOperandExpr(curr->getOperand(0)),
					       generateOperandExpr(curr->getOperand(1)));
		case CmpInst::ICMP_SLT :
			return SltExpr::create(generateOperandExpr(curr->getOperand(0)),
					       generateOperandExpr(curr->getOperand(1)));
		case CmpInst::ICMP_SLE :
			return SleExpr::create(generateOperandExpr(curr->getOperand(0)),
					       generateOperandExpr(curr->getOperand(1)));
		default:
			/* Unsupported compare predicate; quit */
			break;
		}
	}
	default:
		/* Don't know how to annotate this */
		break;
	}
	return ConcreteExpr::create(APInt(1, 0));
}

/*
 * Helper for getSourceLoads() -- see below
*/
void calcSourceLoads(Instruction *i, VSet<PHINode *> phis, std::vector<LoadInst *> &source)
{
	if (!i)
		return;

	/* Don't go past stores or allocas */
	if (isa<StoreInst>(i) || isa<AtomicCmpXchgInst>(i) ||
	    isa<AtomicRMWInst>(i) || isa<AllocaInst>(i))
		return;

	/* If we reached a (source) load, collect it */
	if (auto *li = dyn_cast<LoadInst>(i)) {
		source.push_back(li);
		return;
	}

	/* If this is an already encountered Φ, don't go into circles */
	if (auto *phi = dyn_cast<PHINode>(i)) {
		if (phis.count(phi))
			return;
		phis.insert(phi);
	}

	/* Otherwise, recurse */
	for (auto &u : i->operands()) {
		if (auto *pi = dyn_cast<Instruction>(u.get())) {
			calcSourceLoads(pi, phis, source);
		} else if (auto *c = dyn_cast<Constant>(u.get())) {
			if (auto *phi = dyn_cast<PHINode>(i)) {
				auto *term = phi->getIncomingBlock(u)->getTerminator();
				if (auto *bi = dyn_cast<BranchInst>(term))
					if (bi->isConditional())
						calcSourceLoads(dyn_cast<Instruction>(bi->getCondition()), phis, source);
			}
		}
	}
	return;
}

std::vector<LoadInst *> AnnotateLoadsPass::getSourceLoads(CallInst *assm) const
{
	VSet<PHINode *> phis;
	std::vector<LoadInst *> source;

	if (auto *arg = dyn_cast<Instruction>(assm->getOperand(0)))
		calcSourceLoads(arg, phis, source);
	source.erase(std::unique(source.begin(), source.end()), source.end());
	return source;
}

std::vector<LoadInst *>
AnnotateLoadsPass::filterAnnotatableFromSource(CallInst *assm, const std::vector<LoadInst *> &source) const
{
	auto &PDT = getAnalysis<llvm::PostDominatorTreeWrapperPass>().getPostDomTree();
	std::vector<LoadInst *> candidates;

	/* Annotatable loads should be post-dominated by the assume */
	for (auto *li : source) {
		if (PDT.dominates(assm->getParent(), li->getParent()))
			candidates.push_back(li);
	}

	/* Collect candidates for which the path to the assume is clear */
	std::vector<LoadInst *> result;
	for (auto *li : candidates) {
		auto assumeFound = false;
		auto loadFound = false;
		auto sideEffects = false;
		foreachInBackPathTo(assm->getParent(), li->getParent(), [&](Instruction &i) {
				/* wait until we find the assume */
				if (!assumeFound) {
					assumeFound |= (dyn_cast<CallInst>(&i) == assm);
					return;
				}
				/* we should stop when the load is found */
				if (assumeFound && !loadFound) {
					sideEffects |= hasSideEffects(&i);
					sideEffects |= (isa<LoadInst>(&i) && li != dyn_cast<LoadInst>(&i)) ;
				}
				if (!loadFound) {
					loadFound |= (dyn_cast<LoadInst>(&i) == li);
					if (loadFound) {
						/* reset for next path (except for side-effects) */
						assumeFound = false;
						loadFound = false;
					}
				}
			});
		if (!sideEffects)
			result.push_back(li);
	}
	result.erase(std::unique(result.begin(), result.end()), result.end());
	return result;
}

std::vector<LoadInst *>
AnnotateLoadsPass::getAnnotatableLoads(CallInst *assm) const
{
	if (!isAssumeFunction(getCalledFunOrStripValName(*assm)))
		return std::vector<LoadInst *>(); /* yet another check...x */

	auto sourceLoads = getSourceLoads(assm);
	return filterAnnotatableFromSource(assm, sourceLoads);
}

std::vector<Instruction *> getNextOrBranchSuccessors(Instruction *i)
{
	std::vector<Instruction *> succs;

	/*
	 * Do not return any successors for points beyond which we
	 * cannot annotate (even though the CFG does have edges we can follow)
	 */
	if (isa<LoadInst>(i) || hasSideEffects(i))
		return succs;
	if (auto *ci = dyn_cast<CallInst>(i)) {
		if (isAssumeFunction(getCalledFunOrStripValName(*ci)) ||
		    isErrorFunction(getCalledFunOrStripValName(*ci)))
			return succs;
	}

	if (i->getNextNode())
		succs.push_back(i->getNextNode());
	else if (auto *bi = dyn_cast<BranchInst>(i)) {
		if (bi->isUnconditional()) {
			succs.push_back(&*bi->getSuccessor(0)->begin());
		} else {
			succs.push_back(&*bi->getSuccessor(0)->begin());
			succs.push_back(&*bi->getSuccessor(1)->begin());
		}
	}
	return succs;
}

std::unique_ptr<SExpr>
AnnotateLoadsPass::propagateAnnotFromSucc(Instruction *curr, Instruction *succ)
{
	auto succExp = annotsMap[succ]->clone();
	auto substitutor = SExprRegSubstitutor();

	PHINode *succPhi = nullptr;
	for (auto iit = succ;
	     (succPhi = dyn_cast<PHINode>(iit)) && curr->getParent() != succ->getParent();
	     iit = iit->getNextNode()) {
		auto phiOp = generateOperandExpr(succPhi->getIncomingValueForBlock(curr->getParent()));
		succExp = substitutor.substitute(succExp.get(), iit, phiOp.get());
	}

	if (isa<BranchInst>(curr) || (isa<PHINode>(curr) && curr->getParent() == succ->getParent()))
		return succExp;

	auto currOp = generateInstExpr(curr);
	return substitutor.substitute(succExp.get(), curr, currOp.get());
}

void AnnotateLoadsPass::tryAnnotateDFSHelper(Instruction *curr)
{
	statusMap[curr] = AnnotateLoadsPass::entered;

	std::vector<Instruction *> succs = getNextOrBranchSuccessors(curr);
	BUG_ON(succs.size() > 2);

	for (auto *succ : succs) {
		if (statusMap[succ] == AnnotateLoadsPass::unseen)
			tryAnnotateDFSHelper(succ);
		else if (statusMap[succ] == AnnotateLoadsPass::entered)
			annotsMap[succ] = ConcreteExpr::create(llvm::APInt(1, 0));
	}

	statusMap[curr] = AnnotateLoadsPass::left;

	/* If we cannot get past this instruction, return either FALSE or the assumed expression */
	if (succs.empty()) {
		if (auto *ci = dyn_cast<CallInst>(curr)) {
			if (isAssumeFunction(getCalledFunOrStripValName(*ci))) {
				annotsMap[curr] = generateOperandExpr(ci->getOperand(0));
				return;
			}
		}
		annotsMap[curr] = ConcreteExpr::create(llvm::APInt(1, 0));
		return;
	}
	/* If this is a branch instruction, create a select expression */
	if (succs.size() == 2) {
		auto cond = dyn_cast<BranchInst>(curr)->getCondition();
		annotsMap[curr] = SelectExpr::create(RegisterExpr::create(cond),
						     propagateAnnotFromSucc(curr, succs[0]),
						     propagateAnnotFromSucc(curr, succs[1]));
		return;
	}
	/* At this point we know there is just one successor: substitute */
	annotsMap[curr] = propagateAnnotFromSucc(curr, succs[0]);
	return;
}

void AnnotateLoadsPass::tryAnnotateDFS(LoadInst *curr)
{
	/* Reset DFS data */
	statusMap.clear();
	annotsMap.clear();

	for (auto &i : instructions(curr->getParent()->getParent()))
		statusMap[&i] = AnnotateLoadsPass::unseen;
	tryAnnotateDFSHelper(curr->getNextNode());
	return;
}

bool AnnotateLoadsPass::runOnFunction(llvm::Function &F)
{
	for (auto &i : instructions(F)) {
		if (auto *a = llvm::dyn_cast<llvm::CallInst>(&i)) {
			if (isAssumeFunction(getCalledFunOrStripValName(*a))) {
				auto loads = getAnnotatableLoads(a);
				for (auto *l : loads) {
					tryAnnotateDFS(l);
					LAI.annotsMap[l] = annotsMap[l->getNextNode()]->clone();
				}
			}
		}
	}
	return false;
}

char AnnotateLoadsPass::ID = 42;
//static llvm::RegisterPass<AnnotateLoadsPass> P("annotate-loads",
//					       "Annotates loads directly used by __VERIFIER_assume().");
