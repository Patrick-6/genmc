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

#ifndef GENMC_MEM_ACCESS_HPP
#define GENMC_MEM_ACCESS_HPP

#include "SAddr.hpp"

#include <climits>

/*******************************************************************************
 **                             AType Enum
 ******************************************************************************/

/**
 * Represents the type of an access: pointer, signed integer, unsigned integer
 */
enum class AType { Pointer, Signed, Unsigned };

/*******************************************************************************
 **                             AAccess Class
 ******************************************************************************/

/**
 * An AAccess comprises an address, a size and a type
 */
class AAccess {

public:
	AAccess() = delete;
	AAccess(SAddr addr, ASize size, AType type) : addr(addr), size(size), type(type) {}

	[[nodiscard]] auto getAddr() const -> SAddr { return addr; }
	[[nodiscard]] auto getSize() const -> ASize { return size; }
	[[nodiscard]] auto getType() const -> AType { return type; }

	[[nodiscard]] auto isPointer() const -> bool { return getType() == AType::Pointer; }
	[[nodiscard]] auto isUnsigned() const -> bool { return getType() == AType::Unsigned; }
	[[nodiscard]] auto isSigned() const -> bool { return getType() == AType::Signed; }

	/** Whether the access contains a given address */
	[[nodiscard]] auto contains(SAddr addr) const -> bool
	{
		if (!getAddr().sameStorageAs(addr))
			return false;
		return getAddr() <= addr && addr < getAddr() + getSize();
	}

	/** Whether the access overlaps with another access */
	[[nodiscard]] auto overlaps(const AAccess &other) const -> bool
	{
		if (!getAddr().sameStorageAs(other.getAddr()))
			return false;
		return getAddr() + getSize() > other.getAddr() &&
		       getAddr() < other.getAddr() + other.getSize();
	}

private:
	SAddr addr;
	ASize size;
	AType type;
};

#endif /* GENMC_MEM_ACCESS_HPP */
