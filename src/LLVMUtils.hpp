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

#ifndef __LLVM_UTILS_HPP__
#define __LLVM_UTILS_HPP__

#include "config.h"
#include "VSet.hpp"
#include <llvm/IR/Value.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/CFG.h>

#ifdef LLVM_HAS_TERMINATORINST
 typedef llvm::TerminatorInst TerminatorInst;
#else
 typedef llvm::Instruction TerminatorInst;
#endif

/*
 * Returns true if o1 and o2 are the same ordering as far as a load
 * operation is concerned. This catches cases where e.g.,
 * o1 is acq_rel and o2 is acq.
 * */
bool areSameLoadOrdering(llvm::AtomicOrdering o1, llvm::AtomicOrdering o2);

/*
 * Strips all kinds of casts from val (including trunc, zext ops, etc)
 */
llvm::Value *stripCasts(llvm::Value *val);

/*
 * Returns the name of the function ci calls
 */
llvm::StringRef getCalledFunOrStripValName(const llvm::CallInst &ci);

/*
 * Returns true if its argument is an intrinsic call that does
 * not have any side-effects.
 */
bool isIntrinsicCallNoSideEffects(const llvm::Instruction &i);

// FIXME: Change name
/*
 * Returns true if i1 depends on o2
 */
bool isDependentOn(const llvm::Instruction *i1, const llvm::Instruction *i2);

/*
 * Returns true if its argument has side-effects.
 * Calls to non-intrinsic functions are considered to produce side-effects.
 */
bool hasSideEffects(const llvm::Instruction *i);

namespace details {
	template<typename F>
	void foreachInPathToHeader(const llvm::BasicBlock *curr,
				   const llvm::BasicBlock *header,
				   llvm::SmallVector<const llvm::BasicBlock *, 4> &path,
				   F&& fun)
	{
		std::for_each(curr->rbegin(), curr->rend(), fun);

		if (curr == header)
			return;

		path.push_back(curr);
		for (auto *pred: predecessors(curr))
			if (std::find(path.begin(), path.end(), pred) == path.end())
				foreachInPathToHeader(pred, header, path, fun);
		path.pop_back();
		return;
	}
};

/*
 * Executes fun for all instructions from curr to header.
 * header needs to be a predecessor of curr.
 * fun is applied in reverse iteration order within a block.
 */

template<typename F>
void foreachInPathToHeader(const llvm::BasicBlock *curr, const llvm::BasicBlock *header, F&& fun)
{
	llvm::SmallVector<const llvm::BasicBlock *, 4> path;
	details::foreachInPathToHeader(curr, header, path, fun);
}

#endif /* __LLVM_UTILS_HPP__ */
