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

#include "SAddr.hpp"

auto operator<<(llvm::raw_ostream &s, const SAddr &addr) -> llvm::raw_ostream &
{
	auto internal = addr.isInternal() ? "I" : "";

	if (addr.isStatic())
		s << "G";
	else if (addr.isAutomatic())
		s << "S";
	else if (addr.isHeap())
		s << "H";
	else
		BUG();
	s << internal << "#(" << ((addr.get() & SAddr::threadMask) >> SAddr::threadStartBit) << ", "
	  << (addr.get() & SAddr::indexMask) << ")";
	return s;
}
