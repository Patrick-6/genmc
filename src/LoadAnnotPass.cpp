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

#include <config.h>
#include "LoadAnnotPass.hpp"
#include "LLVMUtils.hpp"
#include "DeclareInternalsPass.hpp"
#include "Error.hpp"
#ifdef HAVE_LLVM_IR_DEBUGINFOMETADATA_H
# include <llvm/IR/DebugInfoMetadata.h>
# else
# include <llvm/IR/Metadata.h>
#endif
#include <llvm/IR/Function.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Support/Debug.h>

AnnotateLoadsPass::AnnotateInfo::AnnotateInfo()
{
	reset();
}

void AnnotateLoadsPass::AnnotateInfo::reset()
{
	callInst = nullptr;
	loadInsts.clear();
	sweptBBs.clear();
}

void AnnotateLoadsPass::getAnalysisUsage(llvm::AnalysisUsage &au) const
{
	au.addRequired<llvm::DominatorTreeWrapperPass>();
	au.addRequired<llvm::PostDominatorTreeWrapperPass>();
	au.addRequired<DeclareInternalsPass>();
	au.setPreservesAll();
}

std::shared_ptr<AnnotationExpr> AnnotateLoadsPass::__generateASTs(llvm::Instruction *annotateInst, llvm::Value *v)
{
	if (auto *constVal = llvm::dyn_cast<llvm::Constant>(v)) {
		BUG_ON(!(constVal->getType()->isIntegerTy()));
		llvm::GenericValue genericVal = llvm::GenericValue();
		genericVal.IntVal = constVal->getUniqueInteger();
		return ConcreteExpr::alloc(genericVal, constVal->getType());
	}

	BUG_ON(!llvm::isa<llvm::Instruction>(v));
	auto *inst = llvm::dyn_cast<llvm::Instruction>(v);

	if (llvm::isa<llvm::LoadInst>(inst) || llvm::isa<llvm::PHINode>(inst) || llvm::isa<llvm::SelectInst>(inst)) {
		std::shared_ptr<AnnotationExpr> symbolicVal;
		if (LAI.instSymbolicVals.count(inst) != 0) {
			symbolicVal = std::shared_ptr<AnnotationExpr>(LAI.instSymbolicVals[inst]);
		} else {
			symbolicVal = SymbolicValue::alloc(inst->getType(), inst == annotateInst, inst->getName());
			LAI.instSymbolicVals[inst] = symbolicVal.get();
		}

		return symbolicVal;
	}

	if (auto *extract = llvm::dyn_cast<llvm::ExtractValueInst>(inst)) {
		BUG_ON(!extract->getType()->isIntegerTy());
		BUG_ON(extract->getType()->getIntegerBitWidth() != 1);

		llvm::AtomicCmpXchgInst *atomicCmpXchg = llvm::dyn_cast<llvm::AtomicCmpXchgInst>(extract->getAggregateOperand());
		BUG_ON(!atomicCmpXchg);

		std::shared_ptr<AnnotationExpr> symbolicVal;
		if (LAI.instSymbolicVals.count(atomicCmpXchg) != 0) {
			symbolicVal = std::shared_ptr<AnnotationExpr>(LAI.instSymbolicVals[atomicCmpXchg]);
		} else {
		        symbolicVal = SymbolicValue::alloc(atomicCmpXchg->getType()->getStructElementType(0),
					                   atomicCmpXchg == annotateInst);
			LAI.instSymbolicVals[atomicCmpXchg] = symbolicVal.get();
		}

		llvm::Value *cmpVal = atomicCmpXchg->getCompareOperand();
		return EqExpr::alloc(symbolicVal, __generateASTs(annotateInst, cmpVal));
	}

	std::vector<std::shared_ptr<AnnotationExpr> > operandExprs;
	for (auto &u : inst->operands()) {
		auto *val = u.get();
		operandExprs.push_back(__generateASTs(annotateInst, val));
	}

	switch (inst->getOpcode()) {
	case llvm::Instruction::ZExt :
		return ZExtExpr::alloc(operandExprs[0], inst->getType()->getPrimitiveSizeInBits());
	case llvm::Instruction::SExt :
		return SExtExpr::alloc(operandExprs[0], inst->getType()->getPrimitiveSizeInBits());
	case llvm::Instruction::Trunc :
		return TruncExpr::alloc(operandExprs[0], inst->getType()->getPrimitiveSizeInBits());

	case llvm::Instruction::Add :
		return AddExpr::alloc(operandExprs[0], operandExprs[1]);
	case llvm::Instruction::Sub :
		return SubExpr::alloc(operandExprs[0], operandExprs[1]);
	case llvm::Instruction::Mul :
		return MulExpr::alloc(operandExprs[0], operandExprs[1]);
	case llvm::Instruction::UDiv :
		return UDivExpr::alloc(operandExprs[0], operandExprs[1]);
	case llvm::Instruction::SDiv :
		return SDivExpr::alloc(operandExprs[0], operandExprs[1]);
	case llvm::Instruction::URem :
		return URemExpr::alloc(operandExprs[0], operandExprs[1]);
	case llvm::Instruction::SRem :
		return SRemExpr::alloc(operandExprs[0], operandExprs[1]);
	case llvm::Instruction::And :
		return AndExpr::alloc(operandExprs[0], operandExprs[1]);
	case llvm::Instruction::Or :
		return OrExpr::alloc(operandExprs[0], operandExprs[1]);
	case llvm::Instruction::Xor :
		return XorExpr::alloc(operandExprs[0], operandExprs[1]);
	case llvm::Instruction::Shl :
		return ShlExpr::alloc(operandExprs[0], operandExprs[1]);
	case llvm::Instruction::LShr :
		return LShrExpr::alloc(operandExprs[0], operandExprs[1]);
	case llvm::Instruction::AShr :
		return AShrExpr::alloc(operandExprs[0], operandExprs[1]);

	case llvm::Instruction::ICmp : {
		llvm::CmpInst *cmpInst = llvm::dyn_cast<llvm::CmpInst>(inst);
		switch(cmpInst->getPredicate()) {
		case llvm::CmpInst::ICMP_EQ :
			return EqExpr::alloc(operandExprs[0], operandExprs[1]);
		case llvm::CmpInst::ICMP_NE :
			return NeExpr::alloc(operandExprs[0], operandExprs[1]);
		case llvm::CmpInst::ICMP_UGT :
			return UgtExpr::alloc(operandExprs[0], operandExprs[1]);
		case llvm::CmpInst::ICMP_UGE :
			return UgeExpr::alloc(operandExprs[0], operandExprs[1]);
		case llvm::CmpInst::ICMP_ULT :
			return UltExpr::alloc(operandExprs[0], operandExprs[1]);
		case llvm::CmpInst::ICMP_ULE :
			return UleExpr::alloc(operandExprs[0], operandExprs[1]);
		case llvm::CmpInst::ICMP_SGT :
			return SgtExpr::alloc(operandExprs[0], operandExprs[1]);
		case llvm::CmpInst::ICMP_SGE :
			return SgeExpr::alloc(operandExprs[0], operandExprs[1]);
		case llvm::CmpInst::ICMP_SLT :
			return SltExpr::alloc(operandExprs[0], operandExprs[1]);
		case llvm::CmpInst::ICMP_SLE :
			return SleExpr::alloc(operandExprs[0], operandExprs[1]);
		default :
			BUG_ON(true && "Unsupported compare predicate");
		}
	}
	default :
		llvm::dbgs() << "unsupported_inst " << *inst << "\n";
		BUG_ON(true && "Unsupported instruction type");
	}

	return nullptr;
}

void AnnotateLoadsPass::generateASTs(llvm::Instruction *annotateInst)
{
	/* Should only have one operand... */
	for (auto &u : currAnnotateInfo.callInst->operands()) {
		auto *v = u.get();
		if (auto *i = llvm::dyn_cast<llvm::Instruction>(v)) {
			std::shared_ptr<AnnotationExpr> annotateExpr = __generateASTs(annotateInst, i);
			LAI.loadASTInfo[annotateInst] = annotateExpr;
			llvm::dbgs() << *annotateInst << " is annotated with " << annotateExpr->toString() << "\n";
		}
	}
}

llvm::Instruction* AnnotateLoadsPass::findAnnotateInst()
{
	if (currAnnotateInfo.loadInsts.size() == 1)
		return currAnnotateInfo.loadInsts[0];

	auto &DT = getAnalysis<llvm::DominatorTreeWrapperPass>().getDomTree();
	unsigned numLoadInsts = currAnnotateInfo.loadInsts.size();
	unsigned i, j;
	for (i = 0, j = 1; i < numLoadInsts && j < numLoadInsts;) {
		if (!llvm::isa<llvm::LoadInst>(currAnnotateInfo.loadInsts[i]) &&
		    !llvm::isa<llvm::AtomicCmpXchgInst>(currAnnotateInfo.loadInsts[i])) {
			i++;
			j = 0;
			continue;
		}

		if (i == j || DT.dominates(currAnnotateInfo.loadInsts[j], currAnnotateInfo.loadInsts[i])) {
			j++;
		} else {
			i++;
			j = 0;
		}
	}

	if (j == numLoadInsts)
		return currAnnotateInfo.loadInsts[i];
	else
		return nullptr;
}

bool AnnotateLoadsPass::findOtherMemoryInsts(llvm::Instruction *nextInst)
{
        std::vector<llvm::Instruction*> nextInsts;

	BUG_ON(!nextInst);

        while (true) {
                if (nextInst == currAnnotateInfo.callInst)
			return false;

                if (llvm::isa<llvm::LoadInst>(nextInst) || llvm::isa<llvm::StoreInst>(nextInst)) {
			llvm::dbgs() << "Find another memory inst" << *nextInst << "\n";
                        return true;
		}

                if (auto *termInst = llvm::dyn_cast<TerminatorInst>(nextInst)) {
                        for (auto i = 0u; i < termInst->getNumSuccessors(); ++i) {
				if (currAnnotateInfo.sweptBBs.count(termInst->getSuccessor(i)) == 0) {
                                        currAnnotateInfo.sweptBBs.insert(termInst->getSuccessor(i));
					nextInsts.push_back(&(termInst->getSuccessor(i)->front()));
				}
			}
                        break;
                } else {
                        nextInst = nextInst->getNextNode();
                        BUG_ON(!nextInst);
                }
        }

        if (nextInsts.size() > 0)
		for (auto iter = nextInsts.begin(); iter != nextInsts.end(); iter++)
			if (findOtherMemoryInsts(*iter))
				return true;

        return false;
}

void AnnotateLoadsPass::__annotateSourceLoads(llvm::Instruction *i)
{
	/* TODO: Proper condition + DFS */
	if (llvm::isa<llvm::AllocaInst>(i) || llvm::isa<llvm::StoreInst>(i))
		return;

	if (llvm::isa<llvm::LoadInst>(i) || llvm::isa<llvm::PHINode>(i) ||
	    llvm::isa<llvm::AtomicCmpXchgInst>(i) || llvm::isa<llvm::SelectInst>(i)) {
		currAnnotateInfo.loadInsts.push_back(i);
		return;
	}

	for (auto &u : i->operands()) {
		auto *v = u.get();
		if (auto *i = llvm::dyn_cast<llvm::Instruction>(v))
			__annotateSourceLoads(i);
	}
}

void AnnotateLoadsPass::annotateSourceLoads()
{
	/* Should only have one operand... */
	for (auto &u : currAnnotateInfo.callInst->operands()) {
		auto *v = u.get();
		if (auto *i = llvm::dyn_cast<llvm::Instruction>(v)) {
			__annotateSourceLoads(i);
		}
	}

	// Currently, we assume the preconditions of annotation are
	// (1) "loadInst" is post-dominated by the "callInst"
	// (2) if multiple loadInsts exist, the annotated "loadInst" should be dominated by all others
	llvm::Instruction *annotateInst = findAnnotateInst();
	if (annotateInst) {
		llvm::dbgs() << *annotateInst << " can be annotated\n";

                currAnnotateInfo.sweptBBs.insert(annotateInst->getParent());
		if (!findOtherMemoryInsts(annotateInst->getNextNode())) {
		        auto &PDT = getAnalysis<llvm::PostDominatorTreeWrapperPass>().getPostDomTree();

		        bool postDominates = PDT.dominates(currAnnotateInfo.callInst->getParent(),
					                   annotateInst->getParent());

		        if (postDominates)
				generateASTs(annotateInst);
		        else
				llvm::dbgs() << *currAnnotateInfo.callInst << "  does not post-dominates"
			                     << *annotateInst << "\n";
		}
	} else {
		llvm::dbgs() << "There are no load instructions that can be annotated\n";
	}

	// Reset annotate info
	currAnnotateInfo.reset();
}

bool AnnotateLoadsPass::runOnFunction(llvm::Function &F)
{
	for (auto &BB : F) {
		for (auto &I : BB) {
			if (auto *a = llvm::dyn_cast<llvm::CallInst>(&I)) {
				if (getCalledFunOrStripValName(*a) == "__VERIFIER_assume") {
					currAnnotateInfo.callInst = a;
					annotateSourceLoads();
				}
			}
		}
	}
	return false;
}

char AnnotateLoadsPass::ID = 42;
//static llvm::RegisterPass<AnnotateLoadsPass> P("annotate-loads",
//					       "Annotates loads directly used by __VERIFIER_assume().");
