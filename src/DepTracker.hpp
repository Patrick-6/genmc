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

#ifndef __DEP_TRACKER_HPP__
#define __DEP_TRACKER_HPP__

#include "DepInfo.hpp"
#include <llvm/IR/Instruction.h>

#include <unordered_map>

/*******************************************************************************
 **                            DepTracker Class
 ******************************************************************************/

/*
 * A class to be used from the Interpreter to track dependencies between
 * instructions. By default, it does nothing and derived classes should
 * implement the interface functionality. The dependencies should be
 * stored for each thread.
 */
class DepTracker {

public:
	virtual ~DepTracker() {};

	/* Returns data dependencies for instruction i in thread tid */
	virtual const DepInfo &getDataDeps(unsigned int tid, llvm::Value *i) = 0;

	/* Returns the address dependencies collected so far for tid */
	virtual const DepInfo &getAddrDeps(unsigned int tid) = 0;

	/* Returns the control dependencies collected so far for tid */
	virtual const DepInfo &getCtrlDeps(unsigned int tid) = 0;

	/* Updates data dependencies of dst, as it is dependent on src */
	virtual void updateDataDeps(unsigned int tid, llvm::Value *dst,
			       llvm::Value *src) {};
	virtual void updateDataDeps(unsigned int tid, llvm::Value *dst,
			       DepInfo e) {};

	/* Adds the dependencies from src to the address dependencies */
	virtual void updateAddrDeps(unsigned int tid, llvm::Value *src) {};

	/* Adds the dependencies from src to the control dependencies */
	virtual void updateCtrlDeps(unsigned int tid, llvm::Value *src) {};

	/* Clears the dependencies calculated for thread TID */
	virtual void clearDeps(unsigned int tid) {};
};

#endif /* __DEP_TRACKER_HPP__ */
