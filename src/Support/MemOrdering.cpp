/*
 * GenMC -- Generic Model Checking.
 *
 * This project is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 *
 * Apache License 2.0:
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * MIT License:
 *     https://opensource.org/licenses/MIT
 */

#include "Support/MemOrdering.hpp"

auto operator<<(llvm::raw_ostream &rhs, MemOrdering ord) -> llvm::raw_ostream &
{
	switch (ord) {
	case MemOrdering::NotAtomic:
		return rhs << "na";
	case MemOrdering::Relaxed:
		return rhs << "rlx";
	case MemOrdering::Acquire:
		return rhs << "acq";
	case MemOrdering::Release:
		return rhs << "rel";
	case MemOrdering::AcquireRelease:
		return rhs << "ar";
	case MemOrdering::SequentiallyConsistent:
		return rhs << "sc";
	default:
		PRINT_BUGREPORT_INFO_ONCE("print-ordering-type", "Cannot print ordering");
		return rhs;
	}
	return rhs;
}
