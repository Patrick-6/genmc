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

#ifndef __ANNOTATE_LOADS_PASS_HPP__
#define __ANNOTATE_LOADS_PASS_HPP__

#include "AnnotExpr.hpp"
#include "ModuleInfo.hpp"
#include <set>
#include <llvm/Pass.h>
#include <llvm/IR/Instructions.h>

class AnnotateLoadsPass : public llvm::FunctionPass {

private:

	class AnnotateInfo{
		public:
	                llvm::CallInst *callInst;
	                std::vector<llvm::Instruction*> loadInsts;
			std::set<llvm::BasicBlock*> sweptBBs;

	                AnnotateInfo();
			void reset();
	};

	AnnotateInfo currAnnotateInfo;

	/* Check if there are other loads and stores between "loadInst" and "callInst" */
	bool findOtherMemoryInsts(llvm::Instruction *nextInst);

	/* check if there is one loadInst which is dominated by all other loads recorded in the currAnnotateInfo. */
	llvm::Instruction *findAnnotateInst();

protected:
	/* Generated ASTs for the dependent "loadInst" */
	std::shared_ptr<AnnotationExpr> __generateASTs(llvm::Instruction *annotatedInst, llvm::Value *v);
	void generateASTs(llvm::Instruction *annotateInst);

	/* Finds and annotates loads on which the assume "callInst" depends on */
	void annotateSourceLoads();
        void __annotateSourceLoads(llvm::Instruction *i);

public:
	static char ID;
	LoadAnnotateInfo &LAI;

	AnnotateLoadsPass(LoadAnnotateInfo &LAI)
	: llvm::FunctionPass(ID), LAI(LAI) {};

	virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
	virtual bool runOnFunction(llvm::Function &F);
};

#endif /* __ANNOTATE_LOADS_PASS_HPP__ */
