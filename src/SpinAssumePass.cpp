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
#include <llvm/Analysis/PostDominators.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include <unordered_set>

#ifdef LLVM_HAS_TERMINATORINST
 typedef llvm::TerminatorInst TerminatorInst;
#else
 typedef llvm::Instruction TerminatorInst;
#endif

void SpinAssumePass::getAnalysisUsage(llvm::AnalysisUsage &au) const
{
	au.addRequired<llvm::DominatorTreeWrapperPass>();
	au.addRequired<llvm::PostDominatorTreeWrapperPass>();
	au.addRequired<DeclareInternalsPass>();
	au.setPreservesAll();
}

const std::unordered_set<std::string> cleanInstrinsics = {
	{"__VERIFIER_assert_fail"},
	{"__VERIFIER_spin_start"},
	{"__VERIFIER_spin_end"},
	{"__VERIFIER_potential_spin_end"},
	{"__VERIFIER_end_loop"},
	{"__VERIFIER_assume"},
	{"__VERIFIER_nondet_int"},
	{"__VERIFIER_thread_self"}
};

bool isIntrinsicCallNoSideEffects(llvm::Instruction &i)
{
	auto *ci = llvm::dyn_cast<llvm::CallInst>(&i);
	if (!ci)
		return false;

	auto *fun = ci->getCalledFunction();
	if (fun)
		return cleanInstrinsics.count(fun->getName().str());

	auto *v = ci->getCalledValue()->stripPointerCasts();
	return cleanInstrinsics.count(v->getName());
}

bool isDependentOn(const llvm::Instruction *i1, const llvm::Instruction *i2, VSet<const llvm::Instruction *> chain)
{
	if (!i1 || !i2 || chain.find(i1) != chain.end())
		return false;

	for (auto &u : i1->operands()) {
		if (auto *i = llvm::dyn_cast<llvm::Instruction>(u.get())) {
			chain.insert(i1);
			if (i == i2 || isDependentOn(i, i2, chain))
				return true;
			chain.erase(i1);
		}
	}
	return false;
}

bool isDependentOn(const llvm::Instruction *i1, const llvm::Instruction *i2)
{
	VSet<const llvm::Instruction *> chain;
	return isDependentOn(i1, i2, chain);
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

bool accessSameVariable(const llvm::Value *p1, const llvm::Value *p2)
{
	if (p1 == p2)
		return true;

	if (auto *bi1 = llvm::dyn_cast<llvm::BitCastInst>(p1)) {
		if (bi1->getOperand(0) == p2)
			return true;
		if (auto *bi2 = llvm::dyn_cast<llvm::BitCastInst>(p2)) {
			if (p1 == bi2->getOperand(0))
				return true;
			if (bi1->getOperand(0) == bi2->getOperand(0))
				return true;
		}
	}
	return false;
}

template<typename F>
bool checkConstantsCondition(const llvm::Value *v1, const llvm::Value *v2, F&& cond)
{
	auto c1 = llvm::dyn_cast<llvm::ConstantInt>(v1);
	auto c2 = llvm::dyn_cast<llvm::ConstantInt>(v2);

	if (!c1 || !c2)
		return false;

	return cond(c1->getValue(), c2->getValue());
}

bool areCancelingBinops(const llvm::AtomicRMWInst *a, const llvm::AtomicRMWInst *b)
{
	using namespace llvm;
	using BinOp = llvm::AtomicRMWInst::BinOp;

	auto op1 = a->getOperation();
	auto op2 = b->getOperation();
	auto v1 = a->getValOperand();
	auto v2 = b->getValOperand();

	/* The two operations need to manipulate the same memory location */
	if (!accessSameVariable(a->getPointerOperand(), b->getPointerOperand()))
		return false;

	/* The operators need to be opposite and the values the same, or vice versa */
	if (((op1 == BinOp::Add && op2 == BinOp::Sub) || (op1 == BinOp::Sub && op2 == BinOp::Add)) &&
	    checkConstantsCondition(v1, v2, [&](const APInt &i1, const APInt &i2) { return i1.abs() == i2.abs(); }))
		return true;

	bool overflow;
	if (op1 == op2 && (op1 == BinOp::Add || op1 == BinOp::Sub) &&
	    checkConstantsCondition(v1, v2, [&](const APInt &i1, const APInt &i2) { return i1.sadd_ov(i2, overflow) == 0; }))
		return true;
	return false;
}

bool SpinAssumePass::isPotentialFaiSpinLoop(const llvm::Loop *l,
					    const std::vector<const llvm::AtomicRMWInst *> fais,
					    const VSet<const llvm::PHINode *> &phis) const
{
	auto &DT = getAnalysis<llvm::DominatorTreeWrapperPass>().getDomTree();
	auto &PDT = getAnalysis<llvm::PostDominatorTreeWrapperPass>().getPostDomTree();

	return DT.dominates(fais[0], fais[1]) &&
	       PDT.dominates(fais[0]->getParent(), fais[1]->getParent()) &&
	       areCancelingBinops(fais[0], fais[1]) &&
	       phis.empty();
}

bool SpinAssumePass::isSpinLoop(const llvm::Loop *l, LoopType &lt) const
{
	std::vector<const llvm::AtomicCmpXchgInst *> cass;
	std::vector<const llvm::AtomicRMWInst *> fais;
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
				} else if (auto *faii = llvm::dyn_cast<llvm::AtomicRMWInst>(&*i)) {
					fais.push_back(faii);
				} else if (auto *ci = llvm::dyn_cast<llvm::CallInst>(&*i)) {
					if (!isIntrinsicCallNoSideEffects(*ci))
						return false;
				} else if (!llvm::isa<llvm::LoadInst>(&*i)) {
					return false;
				}
			}
		}
	}
	/* If there were CASes, there can be only 1, and the spinloop needs to be checked */
	if (cass.size())
		return cass.size() == 1 && fais.empty() && isCasSpinLoop(l, cass[0], phis) &&
			(lt = LoopType::Cas); /* deliberate assignment */
	/* If there were FAIs, there need to be exactly 2.
	 * Best case scenario, this is a potential spin loop (will be determined dynamically) */
	if (fais.size())
		return fais.size() == 2 && cass.empty() && isPotentialFaiSpinLoop(l, fais, phis) &&
			(lt = LoopType::Fai); /* deliberate assignment */

	/* Otherwise, it is a plain spinloop; there should be no PHI nodes */
	return phis.empty() && (lt = LoopType::Plain); /* deliberate assignment */
}

llvm::Value *getOrCreateExitingCondition(llvm::BasicBlock *header, llvm::Instruction *term)
{
	if (auto *ibr = llvm::dyn_cast<llvm::IndirectBrInst>(term))
		return llvm::ConstantInt::getFalse(term->getContext());

	auto *bi = llvm::dyn_cast<llvm::BranchInst>(term);
	BUG_ON(!bi);

	if (bi->isUnconditional())
		return llvm::ConstantInt::getFalse(term->getContext());
	if (bi->getSuccessor(0) != header)
		return bi->getCondition();
	return llvm::BinaryOperator::CreateNot(bi->getCondition(), "", term);
}

void addSpinEndCallBeforeTerm(llvm::BasicBlock *header, llvm::BasicBlock *latch)
{
	auto *term = latch->getTerminator();
        auto *endFun = term->getParent()->getParent()->getParent()->getFunction("__VERIFIER_spin_end");

	auto *cond = getOrCreateExitingCondition(header, term);
        auto *ci = llvm::CallInst::Create(endFun, {cond}, "", term);
        ci->setMetadata("dbg", term->getMetadata("dbg"));
	return;
}

void addPotentialSpinEndCallBeforeLastFai(llvm::Loop *l)
{
	/* Find the last FAI call of the loop */
	std::vector<llvm::AtomicRMWInst *> fais;
	for (auto bb = l->block_begin(); bb != l->block_end(); ++bb) {
		for (auto i = (*bb)->begin(); i != (*bb)->end(); ++i) {
			if (auto *faii = llvm::dyn_cast<llvm::AtomicRMWInst>(&*i))
				fais.push_back(faii);
		}
	}

	BUG_ON(fais.size() != 2);
	auto lastFai = fais.back();
	auto *endFun = lastFai->getParent()->getParent()->getParent()->getFunction("__VERIFIER_potential_spin_end");

	auto *ci = llvm::CallInst::Create(endFun, {}, "", lastFai);
	ci->setMetadata("dbg", lastFai->getMetadata("dbg"));
	return;
}

void addSpinStartCall(llvm::BasicBlock *b)
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

void SpinAssumePass::transformLoop(llvm::Loop *l, const LoopType &typ, llvm::LPPassManager &lpm)
{
	llvm::BasicBlock *header = l->getHeader();
	llvm::SmallVector<llvm::BasicBlock *, 4> latches;

	/* If liveness checks are specified, mark the start of the spinloop */
	if (liveness)
		addSpinStartCall(header);

	if (typ == LoopType::Fai) {
		addPotentialSpinEndCallBeforeLastFai(l);
	} else {
		l->getLoopLatches(latches);
		for (auto &latch : latches)
			addSpinEndCallBeforeTerm(header, latch);
	}
	removeDisconnectedBlocks(l);
	return;
}

bool SpinAssumePass::runOnLoop(llvm::Loop *l, llvm::LPPassManager &lpm)
{
	LoopType t = LoopType::None;

	/* All LLVM loops are natural loops */
	if (!isSpinLoop(l, t))
		return false;

	transformLoop(l, t, lpm);
#ifdef LLVM_HAVE_LOOPINFO_ERASE
	lpm.getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo().erase(l);
	lpm.markLoopAsDeleted(*l);
#elif  LLVM_HAVE_LOOPINFO_MARK_AS_REMOVED
	lpm.getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo().markAsRemoved(l);
#else
	lpm.deleteLoopFromQueue(l);
#endif
	return true;
}

char SpinAssumePass::ID = 42;
// static llvm::RegisterPass<SpinAssumePass> P("spin-assume",
// 					    "Replaces spin-loops with __VERIFIER_assume().");
