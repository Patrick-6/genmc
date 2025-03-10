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

#include "SVal.hpp"
#include "SAddr.hpp"

auto operator<<(llvm::raw_ostream &s, const SVal &v) -> llvm::raw_ostream &
{
	s << v.get();
	// TODO GENMC: how to print the `extra` value?
	return s;
}

static_assert(sizeof(SVal::Value) >= sizeof(SAddr::Width), "SVal needs to be able to hold SAddr");
static_assert(sizeof(SVal::Value) >= sizeof(uintptr_t), "SVal needs to be able to hold pointers");
