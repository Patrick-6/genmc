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

#ifndef GENMC_GENMC_INTERPRETER_CALLBACKS_HPP
#define GENMC_GENMC_INTERPRETER_CALLBACKS_HPP

#include "ExecutionGraph/ExecutionGraph.hpp"
#include "Support/ASize.hpp"
#include "Support/SAddr.hpp"

#include <functional>

/** Returns true if the given allocation is a valid static allocation. */
using IsStaticallyAllocated = std::function<bool(SAddr)>;
/** Returns the name of a static variable if it exists. */
using GetStaticName = std::function<std::optional<std::string>(SAddr)>;

/** Contains callbacks for querying the current interpreter. */
struct InterpreterCallbacks {
	IsStaticallyAllocated isStaticallyAllocated{};
	GetStaticName getStaticName{};
	ExecutionGraph::InitValGetter initValGetter{};
};

#endif /* GENMC_GENMC_INTERPRETER_CALLBACKS_HPP */
