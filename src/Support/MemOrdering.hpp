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

/*
 * This file defines the memory orderings supported by GenMC.
 * Inspired by the AtomicOrdering class in LLVM
 */

#ifndef GENMC_MEMORDERING_HPP
#define GENMC_MEMORDERING_HPP

#include "Support/Error.hpp"

#include <llvm/Support/AtomicOrdering.h>
#include <llvm/Support/raw_ostream.h>

#include <concepts>
#include <cstddef>
#include <cstdint>

/** C11 memory ordering */
enum class MemOrdering : std::uint8_t {
	NotAtomic = 0,
	Relaxed = 1,
	// In case we support consume
	Acquire = 3,
	Release = 4,
	AcquireRelease = 5,
	SequentiallyConsistent = 6,
	LAST = SequentiallyConsistent
};

auto operator<(MemOrdering, MemOrdering) -> bool = delete;
auto operator>(MemOrdering, MemOrdering) -> bool = delete;
auto operator<=(MemOrdering, MemOrdering) -> bool = delete;
auto operator>=(MemOrdering, MemOrdering) -> bool = delete;

/** Helper to validate unknown integral types */
template <typename Int>
requires std::integral<Int>
inline auto isValidMemOrdering(Int i) -> bool
{
	return static_cast<Int>(MemOrdering::NotAtomic) <= i &&
	       i <= static_cast<Int>(MemOrdering::LAST) && i != 2;
}

/** Returns whether ord is stronger than other */
inline auto isStrongerThan(MemOrdering ord, MemOrdering other) -> bool
{
	static const bool lookup[7][7] = {
		//               NA      RX     CO     AC     RE     AR     SC
		/* nonatomic */ {false, false, false, false, false, false, false},
		/* relaxed   */ {true, false, false, false, false, false, false},
		/* consume   */ {true, true, false, false, false, false, false},
		/* acquire   */ {true, true, true, false, false, false, false},
		/* release   */ {true, true, false, false, false, false, false},
		/* acq_rel   */ {true, true, true, true, true, false, false},
		/* seq_cst   */ {true, true, true, true, true, true, false},
	};
	return lookup[static_cast<size_t>(ord)][static_cast<size_t>(other)];
}

inline auto isAtLeastOrStrongerThan(MemOrdering ord, MemOrdering other) -> bool
{
	static const bool lookup[7][7] = {
		//               NA     RX     CO     AC     RE     AR     SC
		/* nonatomic */ {true, false, false, false, false, false, false},
		/* relaxed   */ {true, true, false, false, false, false, false},
		/* consume   */ {true, true, true, false, false, false, false},
		/* acquire   */ {true, true, true, true, false, false, false},
		/* release   */ {true, true, false, false, true, false, false},
		/* acq_rel   */ {true, true, true, true, true, true, false},
		/* seq_cst   */ {true, true, true, true, true, true, true},
	};
	return lookup[static_cast<size_t>(ord)][static_cast<size_t>(other)];
}

/** Translates an LLVM ordering to our internal one; assumes the
 * ordering is one we support (i.e., currently not Unordered)*/
inline auto fromLLVMOrdering(llvm::AtomicOrdering ord) -> MemOrdering
{
	static const MemOrdering lookup[8] = {
		/* NotAtomic */ MemOrdering::NotAtomic,
		/* Unordered */ MemOrdering::Relaxed,
		/* monotonic */ MemOrdering::Relaxed,
		/* consume   */ MemOrdering::Acquire,
		/* acquire   */ MemOrdering::Acquire,
		/* release   */ MemOrdering::Release,
		/* acq_rel   */ MemOrdering::AcquireRelease,
		/* seq_cst   */ MemOrdering::SequentiallyConsistent,
	};
	return lookup[static_cast<size_t>(ord)];
}

auto operator<<(llvm::raw_ostream &rhs, MemOrdering ord) -> llvm::raw_ostream &;

#endif /* GENMC_MEMORDERING_HPP */
