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

#include "Stamp.hpp"

auto operator<<(llvm::raw_ostream &rhs, const Stamp &s) -> llvm::raw_ostream &
{
	rhs << s.get();
	return rhs;
}
