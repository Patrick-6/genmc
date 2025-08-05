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

#ifndef GENMC_NAME_INFO_HPP
#define GENMC_NAME_INFO_HPP

#include "Error.hpp"

#include <string>

/**
 * Represents naming information for a specific type/allocation
 */
class NameInfo {

public:
	NameInfo() = default;

	/** Mark name at offset O as N */
	void addOffsetInfo(unsigned int o, std::string n);

	/** Returns name at offset O */
	[[nodiscard]] auto getNameAtOffset(unsigned int o) const -> std::string;

	/** Returns the number of different offset information registered */
	[[nodiscard]] auto size() const -> size_t { return info.size(); }

	/** Whether we have any information stored */
	[[nodiscard]] auto empty() const -> bool { return info.empty(); }

	friend auto operator<<(llvm::raw_ostream &rhs, const NameInfo &info) -> llvm::raw_ostream &;

private:
	/*
	 * We keep a map (Values -> (offset, name_at_offset)), and after
	 * the interpreter and the variables are allocated and initialized,
	 * we use the map to dynamically find out the name corresponding to
	 * a particular address.
	 */
	using OffsetInfo = std::vector<std::pair<unsigned, std::string>>;

	/** Naming information at different offsets */
	OffsetInfo info;
};

#endif /* GENMC_NAME_INFO_HPP */
