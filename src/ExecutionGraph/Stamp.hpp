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

#ifndef GENMC_STAMP_HPP
#define GENMC_STAMP_HPP

#include "Support/Error.hpp"

#include <cstdint>

/**
 * Represents a label stamp (positive number).
 */
class Stamp {

protected:
	/* Assuming re-assigning, uint32_t should suffice */
	using Value = uint32_t;

public:
	/* Constructors/destructors */
	Stamp() = delete;
	constexpr Stamp(uint32_t v) : value(v) {}
	constexpr Stamp(const Stamp &other) = default;
	constexpr Stamp(Stamp &&other) = default;

	~Stamp() = default;

	auto operator=(const Stamp &other) -> Stamp & = default;
	auto operator=(Stamp &&other) -> Stamp & = default;

	auto operator<=>(const Stamp &other) const = default;

#define IMPL_STAMP_BINOP(_op)                                                                      \
	Stamp &operator _op##=(uint32_t v)                                                         \
	{                                                                                          \
		value _op## = v;                                                                   \
		return *this;                                                                      \
	}                                                                                          \
	Stamp operator _op(uint32_t v) const                                                       \
	{                                                                                          \
		Stamp n(*this);                                                                    \
		n _op## = v;                                                                       \
		return n;                                                                          \
	}

	IMPL_STAMP_BINOP(+);
	IMPL_STAMP_BINOP(-);

	auto operator++() -> Stamp & { return (*this) += 1; }
	auto operator++(int) -> Stamp
	{
		auto tmp = *this;
		++*this;
		return tmp;
	}

	auto operator--() -> Stamp & { return (*this) -= 1; }
	auto operator--(int) -> Stamp
	{
		auto tmp = *this;
		--*this;
		return tmp;
	}

	/* Type-system hole */
	[[nodiscard]] auto get() const -> uint32_t { return value; }
	auto operator()() const -> uint32_t { return get(); }

	friend auto operator<<(llvm::raw_ostream &rhs, const Stamp &s) -> llvm::raw_ostream &;

private:
	Value value;
};

#endif /* GENMC_STAMP_HPP */
