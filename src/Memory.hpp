/*
 * GenMC -- Generic Model Checking.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-3.0.html.
 *
 * Author: Michalis Kokologiannakis <michalis@mpi-sws.org>
 */

#ifndef __MEMORY_HPP__
#define __MEMORY_HPP__

#include "config.h"
#include "Error.hpp"

#include <cstdint>

/*
 * This file contains information regarding the symbolic
 * representation of memory in GenMC. Currently only models the
 * address domain, as we use LLVM builtins for values.
 */

namespace llvm {
	class Interpreter;
};

/*******************************************************************************
 **                             SAddr Class
 ******************************************************************************/

/*
 * Represents a memory address. An address is a bitfield with the
 * following structure:
 *
 *     B64: 1 -> static, 0 -> dynamic
 *     B63: 1 -> automatic, 0 -> heap
 *     B62: 1 -> internal, 0 -> user
 *     B0-B61: address
 */
class SAddr {

public:
	/* The implementation is currently not quite portable.
	 * Ideally, this should be equal to the size of a pointer
	 * so that we can easily convert to actual pointers */
	using Width = uint64_t;

protected:
	friend class llvm::Interpreter;

	static constexpr Width staticMask = (Width) 1 << 63;
	static constexpr Width automaticMask = (Width) 1 << 62;
	static constexpr Width internalMask = (Width) 1 << 61;
	static constexpr Width storageMask = staticMask | automaticMask | internalMask;
	static constexpr Width addressMask = internalMask - 1;

	static constexpr Width limit = internalMask - 1;

	SAddr(Width a) : addr(a) {}
	SAddr(void *a) : addr((Width) a) {}

	static SAddr create(Width storageMask, Width value, bool internal) {
		BUG_ON(value >= SAddr::limit);
		Width fresh = 0;
		fresh |= storageMask;
		fresh ^= (-(unsigned long)(!!internal) ^ fresh) & internalMask;
		fresh |= value;
		return SAddr(fresh);
	}

public:
	SAddr() : addr(0) {}

	/* Helper methods to create a new address */
	template <typename... Ts>
	static SAddr createStatic(Ts&&... params) {
		return create(staticMask, std::forward<Ts>(params)...);
	}
	template <typename... Ts>
	static SAddr createHeap(Ts&&... params) {
		return create(0, std::forward<Ts>(params)...);
	}
	template <typename... Ts>
	static SAddr createAutomatic(Ts&&... params) {
		return create(automaticMask, std::forward<Ts>(params)...);
	}

	/* Return information regarding the address */
	bool isStatic() const { return addr & staticMask; }
	bool isDynamic() const { return !isStatic(); }
	bool isAutomatic() const { return addr & automaticMask; }
	bool isHeap() const { return !isAutomatic(); }
	bool isInternal() const { return addr & internalMask; }
	bool isUser() const { return !isInternal(); }
	bool isNull() const { return addr == 0; }

	Width get() const { return addr; }

	inline bool operator==(const SAddr &a) const {
		return a.addr == addr;
	}
	inline bool operator!=(const SAddr &a) const {
		return !(*this == a);
	}
	inline bool operator<=(const SAddr &a) const {
		return addr <= a.addr;
	}
	inline bool operator<(const SAddr &a) const {
		return addr < a.addr;
	}
	inline bool operator>=(const SAddr &a) const {
		return !(*this < a);
	}
	inline bool operator>(const SAddr &a) const {
		return !(*this <= a);
	}
	SAddr operator+(unsigned int num) const {
		SAddr s(*this);
		s.addr += num;
		return s;
	};
	SAddr operator-(unsigned int num) const {
		SAddr s(*this);
		s.addr -= num;
		return s;
	};
	Width operator-(const SAddr &a) const {
		return this->get() - a.get();
	};

	friend llvm::raw_ostream& operator<<(llvm::raw_ostream& rhs,
					     const SAddr &addr);

private:
	/* The actual address */
	Width addr;
};


/*******************************************************************************
 **                         SAddrAllocator Class
 ******************************************************************************/

/*
 * Helper class that allocates addresses within the SAddr domain.
 * A given allocator will never allocate the same address twice.
 */
class SAddrAllocator {

protected:
	/* Allocates a fresh address at the specified pool */
	template <typename F>
	SAddr allocate(F&& allocFun, SAddr::Width &pool, unsigned int size,
		       unsigned int alignment, bool isInternal = false) {
		auto offset = alignment - 1;
		auto oldAddr = pool;
		pool += (offset + size);
		return allocFun(((SAddr::Width) oldAddr + offset) & ~(alignment - 1),
				static_cast<bool&&>(isInternal));
	}

public:
	SAddrAllocator() = default;

	/* Allocating methods */
	template <typename... Ts>
	SAddr allocStatic(Ts&&... params) {
		return allocate(SAddr::createStatic<SAddr::Width, bool>, staticPool,
				std::forward<Ts>(params)...);
	}
	template <typename... Ts>
	SAddr allocAutomatic(Ts&&... params) {
		return allocate(SAddr::createAutomatic<SAddr::Width, bool>, automaticPool,
				std::forward<Ts>(params)...);
	}
	template <typename... Ts>
	SAddr allocHeap(Ts&&... params) {
		return allocate(SAddr::createHeap<SAddr::Width, bool>, heapPool,
				std::forward<Ts>(params)...);
	}

private:
	/* Assuming that we will not run out of addresses, we could get
	 * a more compact representation using static pools, but it's
	 * better to be thread safe */
	SAddr::Width staticPool = 0;
	SAddr::Width automaticPool = 0;
	SAddr::Width heapPool = 1; /* avoid allocating null */
};

namespace std {
	template<>
	struct hash<SAddr> {
		std::size_t operator()(const SAddr &a) const {
			using std::hash;
			return hash<uint64_t>()(a.get());
		};
	};
}

#endif /* __MEMORY_HPP__ */
