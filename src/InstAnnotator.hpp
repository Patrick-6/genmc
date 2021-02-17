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

#ifndef __INST_ANNOTATOR_HPP__
#define __INST_ANNOTATOR_HPP__

#include "ModuleInfo.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Instructions.h>

#include <unordered_map>

using namespace llvm;

class SExpr;

/*
 * A class that annotates loads by performing a DFS-like propagation procedure.
 * Also exports utilities for calculating symbolic expressions across basic blocks.
 */

class InstAnnotator {

public:
	/* Returns the annotation for a load L */
	std::unique_ptr<SExpr> annotate(LoadInst *l);

private:
	enum Status { unseen, entered, left };

	using InstStatusMap = DenseMap<Instruction *, Status>;
	using InstAnnotMap = std::unordered_map<Instruction *, std::unique_ptr<SExpr> >;

	/* Helper that returns the annotation for CURR by propagating SUCC's annotation backwards */
	std::unique_ptr<SExpr> propagateAnnotFromSucc(Instruction *curr, Instruction *succ);

	/* Helper for tryAnnotateDFS; performs the actual DFS procedure */
	void annotateDFS(Instruction *curr);

	/* A helper status map */
	InstStatusMap statusMap;

	/* A map storing the annotations for this function's annotatable loads */
	InstAnnotMap annotMap;
};

#endif /* __INST_ANNOTATOR_HPP__ */
