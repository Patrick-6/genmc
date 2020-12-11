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
#include "SCDetectorPass.hpp"
#include "ModuleInfo.hpp"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

using namespace llvm;

void SCDetectorPass::setDeterminedMM(ModelType m)
{
	PI->determinedMM = m;
}

bool isSCNAOrdering(AtomicOrdering o)
{
	return o == AtomicOrdering::SequentiallyConsistent || o == AtomicOrdering::NotAtomic;
}

bool SCDetectorPass::runOnModule(Module &M)
{
	bool isSC = true;
	for (auto &F : M) {
		for (auto &I : F) {
			if (auto *li = dyn_cast<LoadInst>(&I)) {
				if (!isSCNAOrdering(li->getOrdering())) {
					isSC = false;
					break;
				}
			} else if (auto *si = dyn_cast<StoreInst>(&I)) {
				if (!isSCNAOrdering(si->getOrdering())) {
					isSC = false;
					break;
				}
			} else if (auto *casi = dyn_cast<AtomicCmpXchgInst>(&I)) {
				if (!isSCNAOrdering(casi->getSuccessOrdering()) ||
				    !isSCNAOrdering(casi->getFailureOrdering())) {
					isSC = false;
					break;
				}
			} else if (auto *faii = dyn_cast<AtomicRMWInst>(&I)) {
				if (!isSCNAOrdering(faii->getOrdering())) {
					isSC = false;
					break;
				}
			} else if (auto *fi = dyn_cast<FenceInst>(&I)) {
				if (!isSCNAOrdering(fi->getOrdering())) {
					isSC = false;
					break;
				}
			}
		}
	}
	if (isSC)
		setDeterminedMM(ModelType::SC);
	return false;
}

ModulePass *createSCDetectorPass(PassModuleInfo *PI)
{
	auto *p = new SCDetectorPass();
	p->setPassModuleInfo(PI);
	return p;
}

char SCDetectorPass::ID = 42;
static RegisterPass<SCDetectorPass> P("sc-detector-casts", "Checks whether all accesses are SC.");
