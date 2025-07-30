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

#include "Support/SAddrAllocator.hpp"
#include "ADT/VectorClock.hpp"

void SAddrAllocator::restrict(const VectorClock &view)
{
	for (auto &[tid, index] : dynamicPool_) {
		index = std::max(1, view.getMax(tid)); // don't allocate null
	}
}

auto operator<<(llvm::raw_ostream &rhs, const SAddrAllocator &alloctor) -> llvm::raw_ostream &
{
	rhs << "static: ";
	for (const auto &[tid, idx] : alloctor.staticPool_)
		rhs << "(" << tid << ", " << idx << ") ";
	rhs << "\ndynamic: ";
	for (const auto &[tid, idx] : alloctor.dynamicPool_)
		rhs << "(" << tid << ", " << idx << ") ";
	return rhs << "\n";
}
