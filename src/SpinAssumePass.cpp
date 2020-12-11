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

#include "VSet.hpp"
#include "Error.hpp"
#include "SpinAssumePass.hpp"
#include "DeclareInternalsPass.hpp"
#include <llvm/Pass.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#ifdef LLVM_HAS_TERMINATORINST
 typedef llvm::TerminatorInst TerminatorInst;
#else
 typedef llvm::Instruction TerminatorInst;
#endif

void SpinAssumePass::getAnalysisUsage(llvm::AnalysisUsage &au) const
{
	au.addRequired<llvm::DominatorTreeWrapperPass>();
	au.addRequired<DeclareInternalsPass>();
	au.addPreserved<DeclareInternalsPass>();
}

bool isAssumeStatement(llvm::Instruction &i)
{
	auto *ci = llvm::dyn_cast<llvm::CallInst>(&i);
	if (!ci)
		return false;

	llvm::Function *fun = ci->getCalledFunction();
	return fun && fun->getName().str() == "__VERIFIER_assume";
}

bool isDependentOn(const llvm::Instruction *i1, const llvm::Instruction *i2)
{
	if (!i1 || !i2)
		return false;

	for (auto &u : i1->operands()) {
		if (auto *i = llvm::dyn_cast<llvm::Instruction>(u.get()))
			return i == i2 || isDependentOn(i, i2);
	}
	return false;
}

bool areSameLoadOrdering(llvm::AtomicOrdering o1, llvm::AtomicOrdering o2)
{
	return o1 == o2 ||
	       (o1 == llvm::AtomicOrdering::Acquire && o2 == llvm::AtomicOrdering::AcquireRelease) ||
	       (o1 == llvm::AtomicOrdering::AcquireRelease && o2 == llvm::AtomicOrdering::Acquire);
}

bool isPhiTiedToCasCmp(const llvm::PHINode *curPhi, const std::vector<llvm::Value *> &inVals,
		       const llvm::AtomicCmpXchgInst *casi, const llvm::PHINode *cmpPhi)
{
	return (inVals[0] == cmpPhi && inVals[1] == casi->getNextNode()) ||
		(inVals[1] == cmpPhi && inVals[0] == casi->getNextNode());
}

bool checkPhis(const llvm::PHINode *cmpPhi, const llvm::PHINode *curPhi,
	       const llvm::AtomicCmpXchgInst *casi, VSet<const llvm::PHINode *> &phis)
{
	std::vector<llvm::Value *> inVals;

	BUG_ON(curPhi->getNumIncomingValues() != 2);
	inVals.push_back(curPhi->getIncomingValue(0));
	inVals.push_back(curPhi->getIncomingValue(1));

	/* Base case: the current PHI is the one right after the CAS */
	if (isPhiTiedToCasCmp(curPhi, inVals, casi, cmpPhi)) {
		phis.erase(curPhi);
		return true;
	}

	/* Recursive step: the current PHI is not the one right after the CAS */
	for (unsigned i = 0; i < inVals.size(); i++) {
		if (auto *li = llvm::dyn_cast<llvm::LoadInst>(inVals[i])) {
			if (li->getPointerOperand() != casi->getPointerOperand() ||
			    !areSameLoadOrdering(li->getOrdering(), casi->getSuccessOrdering()))
				return false;
			continue;
		}
		if (auto *extract = llvm::dyn_cast<llvm::ExtractValueInst>(inVals[i])) {
			BUG_ON(!extract->getType()->isIntegerTy());
			BUG_ON(extract->getType()->getIntegerBitWidth() == 1);

			auto *ecasi = llvm::dyn_cast<llvm::AtomicCmpXchgInst>(extract->getAggregateOperand());
			if (!ecasi || ecasi->getPointerOperand() != casi->getPointerOperand() ||
			    !areSameLoadOrdering(ecasi->getSuccessOrdering(), casi->getSuccessOrdering()))
				return false;

			continue;
		}

		auto *phi = llvm::dyn_cast<llvm::PHINode>(inVals[i]);
		if (!phi || !checkPhis(cmpPhi, phi, casi, phis))
			return false;
	}

	phis.erase(curPhi);
	return true;
}

bool isCasSpinLoop(const llvm::Loop *l, const llvm::AtomicCmpXchgInst *casi, VSet<const llvm::PHINode *> &phis)
{
	llvm::BasicBlock *eb = l->getExitingBlock();
	if (!eb)
		return false;

	llvm::BranchInst *bi = llvm::dyn_cast<llvm::BranchInst>(eb->getTerminator());
	if (!bi || !bi->isConditional() ||
	    !isDependentOn(llvm::dyn_cast<llvm::Instruction>(bi->getCondition()), casi))
		return false;

	auto *cmpOp = casi->getCompareOperand();
	if (llvm::isa<llvm::Constant>(cmpOp))
		return phis.empty();

	auto *cmpPhi = llvm::dyn_cast<llvm::PHINode>(cmpOp);
	if (!cmpPhi)
		return false;

	return checkPhis(cmpPhi, cmpPhi, casi, phis) && phis.empty();
}

bool SpinAssumePass::isSpinLoop(const llvm::Loop *l) const
{
	std::vector<const llvm::AtomicCmpXchgInst *> cass;
	VSet<const llvm::PHINode *> phis;

	/* A spinloop has no side effects, so all instructions need to have no
	 * side effects too. The only exceptions are calls to __VERIFIER_assume()
	 * and CAS/PHI instructions (as these may belong to a CAS spinloop). */
	for (auto bb = l->block_begin(); bb != l->block_end(); ++bb) {
		for (auto i = (*bb)->begin(); i != (*bb)->end(); ++i) {
			if (llvm::isa<llvm::AllocaInst>(*i))
				return false;
			if (auto *phii = llvm::dyn_cast<llvm::PHINode>(&*i))
				phis.insert(phii);
			if (i->mayHaveSideEffects()) {
				if (auto *casi = llvm::dyn_cast<llvm::AtomicCmpXchgInst>(&*i)) {
					cass.push_back(casi);
				} else if (auto *ci = llvm::dyn_cast<llvm::CallInst>(&*i)) {
					if (!isAssumeStatement(*ci))
						return false;
				} else if (!llvm::isa<llvm::LoadInst>(&*i)) {
					return false;
				}
			}
		}
	}
	/* If there were CASes, there can be only 1, and the spinloop needs to be checked */
	if (cass.size())
		return cass.size() == 1 && isCasSpinLoop(l, cass[0], phis);

	/* Otherwise, it is a plain spinloop; there should be no PHI nodes */
	return phis.empty();
}

void SpinAssumePass::addSpinEndCallBeforeInstruction(llvm::Instruction *bi)
{
        auto *endFun = bi->getParent()->getParent()->getParent()->getFunction("__VERIFIER_spin_end");

        auto *ci = llvm::CallInst::Create(endFun, {}, "", bi);
        ci->setMetadata("dbg", bi->getMetadata("dbg"));
	return;
}

void SpinAssumePass::addSpinStartCall(llvm::BasicBlock *b)
{
        auto *startFun = b->getParent()->getParent()->getFunction("__VERIFIER_spin_start");

	auto *i = b->getFirstNonPHI();
        auto *ci = llvm::CallInst::Create(startFun, {}, "", i);
}

void SpinAssumePass::removeDisconnectedBlocks(llvm::Loop *l)
{
	bool done;

	while (l) {
		done = false;
		while (!done) {
			done = true;
			VSet<llvm::BasicBlock*> hasPredecessor;

			/*
			 * We collect blocks with predecessors in l, and we also
			 * search for BBs without successors in l
			 */
			for (auto it = l->block_begin(); done && it != l->block_end(); ++it) {
				TerminatorInst *T = (*it)->getTerminator();
				bool hasLoopSuccessor = false;

				for(auto i = 0u; i < T->getNumSuccessors(); ++i) {
					if(l->contains(T->getSuccessor(i))){
						hasLoopSuccessor = true;
						hasPredecessor.insert(T->getSuccessor(i));
					}
				}

				if (!hasLoopSuccessor) {
					done = false;
					l->removeBlockFromLoop(*it);
				}
			}

			/* Find BBs without predecessors in l */
			for(auto it = l->block_begin(); done && it != l->block_end(); ++it){
				if(hasPredecessor.count(*it) == 0) {
					done = false;
					l->removeBlockFromLoop(*it);
				}
			}
		}
		l = l->getParentLoop();
	}
}

#ifndef LLVM_HAVE_LOOPINFO_GETINCANDBACKEDGE
bool getIncomingAndBackEdge(llvm::Loop *l, llvm::BasicBlock *&Incoming, llvm::BasicBlock *&Backedge)
{
	llvm::BasicBlock *H = l->getHeader();

	Incoming = nullptr;
	Backedge = nullptr;
	llvm::pred_iterator PI = pred_begin(H);
	assert(PI != pred_end(H) && "Loop must have at least one backedge!");
	Backedge = *PI++;
	if (PI == pred_end(H))
		return false; // dead loop
	Incoming = *PI++;
	if (PI != pred_end(H))
		return false; // multiple backedges?

	if (l->contains(Incoming)) {
		if (l->contains(Backedge))
			return false;
		std::swap(Incoming, Backedge);
	} else if (!l->contains(Backedge))
		return false;

	assert(Incoming && Backedge && "expected non-null incoming and backedges");
	return true;
}
# define GET_INCOMING_AND_BACK_EDGE(l, i, b) getIncomingAndBackEdge(l, i, b)
#else
# define GET_INCOMING_AND_BACK_EDGE(l, i, b) l->getIncomingAndBackEdge(i, b)
#endif

bool SpinAssumePass::transformLoop(llvm::Loop *l, llvm::LPPassManager &lpm)
{
        llvm::BasicBlock *incoming, *backedge;
	TerminatorInst *ei;
	llvm::BranchInst *bi;

	/* Is the incoming/backedge unique? */
	if (!GET_INCOMING_AND_BACK_EDGE(l, incoming, backedge))
		return false;

	/* Is the last instruction of the loop a branch? */
	ei = backedge->getTerminator();
	BUG_ON(!ei);
	bi = llvm::dyn_cast<llvm::BranchInst>(ei);
	if (!bi || bi->isConditional())
		return false;

	/* If liveness checks are specified, also mark the start of the spinloop */
	if (liveness)
		addSpinStartCall(l->getHeader());
	addSpinEndCallBeforeInstruction(bi);
	removeDisconnectedBlocks(l);
	return true;
}

bool SpinAssumePass::runOnLoop(llvm::Loop *l, llvm::LPPassManager &lpm)
{
	bool modified = false;
	if (isSpinLoop(l)) {
		if (transformLoop(l, lpm)) {
#ifdef LLVM_HAVE_LOOPINFO_ERASE
			lpm.getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo().erase(l);
			lpm.markLoopAsDeleted(*l);
#elif  LLVM_HAVE_LOOPINFO_MARK_AS_REMOVED
			lpm.getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo().markAsRemoved(l);
#else
			lpm.deleteLoopFromQueue(l);
#endif
			modified = true;
		}
	}
	return modified;
}

char SpinAssumePass::ID = 42;
// static llvm::RegisterPass<SpinAssumePass> P("spin-assume",
// 					    "Replaces spin-loops with __VERIFIER_assume().");
