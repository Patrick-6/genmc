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

#ifndef GENMC_WORK_LIST_HPP
#define GENMC_WORK_LIST_HPP

#include "Verification/Revisit.hpp"
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <vector>

/** Represents a list of TODOs for the driver */
class WorkList {

public:
	using ItemT = std::unique_ptr<Revisit>;

	WorkList() = default;

	/** Returns whether this worklist is empty */
	[[nodiscard]] auto empty() const -> bool { return wlist_.empty(); }

	/** Adds an item to the worklist */
	void add(auto &&item) { wlist_.emplace_back(std::move(item)); }

	/** Returns the next item to examine (NULL if none) */
	auto getNext() -> ItemT
	{
		if (wlist_.empty())
			return {};

		auto item = std::move(wlist_.back());
		wlist_.pop_back();
		return std::move(item);
	}

	friend auto operator<<(llvm::raw_ostream &s, const WorkList &wlist) -> llvm::raw_ostream &;

private:
	/* Each stamp was associated with a bucket of TODOs before.
	 * This is now unnecessary, as even a simple stack suffices */
	std::vector<ItemT> wlist_;
};

#endif /* GENMC_WORK_LIST_HPP */
