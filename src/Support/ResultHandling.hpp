#ifndef GENMC_RESULT_HANDLING_HPP
#define GENMC_RESULT_HANDLING_HPP

#include "SVal.hpp"

#include <cstdint>
#include <memory>
#include <ostream>
#include <string>

// TODO GENMC: improve this, add more info, maybe use `ErrorDetails` struct or similar
using ModelCheckerError = std::string;

/// 128 bit integer, encoded as bytes for FFI reasons: (TODO GENMC: find a better solution)
using AccessValue = uint8_t[16];

// TODO GENMC: maybe remove this type and just expose `SVal`
struct GenmcScalar {
	uint64_t value;
	bool is_init;

	GenmcScalar() : value(0), is_init(false) {}
	GenmcScalar(uint64_t value) : value(value), is_init(true) {}
	GenmcScalar(SVal val) : value(val.get()), is_init(true) {}

	auto toSVal() const -> SVal
	{
		BUG_ON(!is_init);
		return SVal(value);
	}

	bool operator==(const GenmcScalar &other) const
	{
		// TODO GENMC: should all uninitialized values be idential?
		if (!is_init && !other.is_init)
			return true;

		// An initialized scalar is never equal to an uninitialized one
		if (is_init != other.is_init)
			return false;

		// Compare the actual values
		return value == other.value;
	}

	friend auto operator<<(llvm::raw_ostream &rhs, const GenmcScalar &v) -> llvm::raw_ostream &;
};

// TODO GENMC: use this:
struct LoadResult {
	// __uint128_t value; // TODO GENMC: how to handle this? CXX doesn't support u128 (yet)
	/// Invalid if `error` contains a value
	// alignas(16) AccessValue value;
	// TODO GENMC: use `SVal` here? Or add method to convert to/from it?
	bool is_read_opt;
	GenmcScalar scalar;
	/// If there is an error, it will be stored in `error`, otherwise it is `None`
	std::unique_ptr<ModelCheckerError> error;

	// TODO GENMC (ERROR HANDLING): could return this, then Miri can ask GenMC:
	// enum Result {Ok, Error, Warning}

public:
	/// TODO GENMC: check if this can be made private to prevent accidential use of this:
	// LoadResult() : scalar(std::nullopt), error(nullptr) {}
	LoadResult() : is_read_opt(true), scalar(SVal(0)), error(nullptr) {}
	LoadResult(const LoadResult &res)
	{
		is_read_opt = false;
		scalar = res.scalar;
		if (res.error.get() != nullptr) {
			error = std::make_unique<ModelCheckerError>(*(res.error));
		} else {
			error = nullptr;
		}
	}
	LoadResult &operator=(const LoadResult &rhs)
	{
		is_read_opt = rhs.is_read_opt;
		scalar = rhs.scalar;
		if (rhs.error.get() != nullptr) {
			error = std::make_unique<ModelCheckerError>(*rhs.error);
		} else {
			error = nullptr;
		}
		return *this;
	}

	static LoadResult fromValue(SVal value)
	{
		uint64_t value_ = value.get();
		auto scalar = GenmcScalar(value_);
		auto load_result = LoadResult{};
		// TODO GENMC: handle u128, and possibly different endianness
		load_result.is_read_opt = false;
		load_result.scalar = scalar;
		load_result.error = nullptr;
		return load_result;
	}

	static LoadResult fromError(std::string msg)
	{
		auto load_result = LoadResult{};
		load_result.error = std::make_unique<ModelCheckerError>(msg);
		return load_result;
	}

	bool is_error() const { return error.get() != nullptr; }

	bool has_value() const { return !is_error() && !is_read_opt; }

	SVal value() const
	{
		// TODO GENMC: u128 handling
		BUG_ON(!has_value());
		return SVal(scalar.value);
	}
	void setValue(SVal val) { scalar = GenmcScalar(val); }
	void setValue(GenmcScalar val) { scalar = val; }

private:
};

// TODO GENMC: use this:
struct StoreResult {
	std::unique_ptr<ModelCheckerError> error;
	bool isCoMaxWrite;

	static StoreResult ok(bool isCoMaxWrite) { return StoreResult{nullptr, isCoMaxWrite}; }

	static StoreResult fromError(std::string msg)
	{
		auto store_result = StoreResult{};
		store_result.error = std::make_unique<ModelCheckerError>(msg);
		return store_result;
	}

	bool is_error() const { return error.get() != nullptr; }
};

#endif /* GENMC_RESULT_HANDLING_HPP */
