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

#ifndef __LOAD_ANNOTATION_PASS_HPP__
#define __LOAD_ANNOTATION_PASS_HPP__

#include "ModuleInfo.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Instructions.h>

#include <unordered_map>

using namespace llvm;

class LoadAnnotationPass : public FunctionPass {

public:
	static char ID;
	AnnotationInfo &LAI;

	LoadAnnotationPass(AnnotationInfo &LAI)
	: FunctionPass(ID), LAI(LAI) {};

	virtual void getAnalysisUsage(AnalysisUsage &AU) const;
	virtual bool runOnFunction(Function &F);

private:
	enum Status { unseen, entered, left };

	using InstStatusMap = DenseMap<Instruction *, Status>;
	using InstAnnotMap = std::unordered_map<Instruction *, std::unique_ptr<SExpr> >;
	using InstPath = std::vector<Instruction *>;

	/*
	 * Returns the source loads of an assume statement, that is,
	 * loads the result of which is used in the assume.
	 */
	std::vector<LoadInst *> getSourceLoads(CallInst *assm) const;

	/*
	 * Given an assume's source loads, returns the annotatable ones.
	 */
	std::vector<LoadInst *>
	filterAnnotatableFromSource(CallInst *assm, const std::vector<LoadInst *> &source) const;

	/*
	 * Returns all of ASSM's annotatable loads
	 */
	std::vector<LoadInst *>
	getAnnotatableLoads(CallInst *assm) const;

	/* Returns the annotation for CURR by propagating SUCC's annotation backwards */
	std::unique_ptr<SExpr> propagateAnnotFromSucc(Instruction *curr, Instruction *succ);

	/* Helper for tryAnnotateDFS */
	void tryAnnotateDFSHelper(Instruction *curr);

	/* Will try and set the annotation for L in ANNOTSMAP */
	void tryAnnotateDFS(LoadInst *l);

	/* A helper status map */
	InstStatusMap statusMap;

	/* A map storing the annotations for this function's annotatable loads */
	InstAnnotMap annotMap;
};

#endif /* __LOAD_ANNOTATION_PASS_HPP__ */
