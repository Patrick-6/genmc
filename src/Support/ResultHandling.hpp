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
	uint64_t extra;
	bool is_init;

	GenmcScalar() : value(0), extra(0), is_init(false) {}
	GenmcScalar(uint64_t value, uint64_t extra) : value(value), extra(extra), is_init(true) {}
	GenmcScalar(SVal val) : value(val.get()), extra(val.getExtra()), is_init(true) {}

	auto toSVal() const -> SVal
	{
		if (!is_init)
			LOG(VerbosityLevel::Error)
				<< "attempt to convert uninitialized memory to SVal: " << value
				<< ", " << extra << "\n";
		BUG_ON(!is_init);
		return SVal(value, extra);
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
		return value == other.value && extra == other.extra;
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
		uint64_t extra = value.getExtra();
		auto scalar = GenmcScalar(value_, extra);
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
		return SVal(scalar.value, scalar.extra);
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

struct ReadModifyWriteResult {
	GenmcScalar old_value;
	GenmcScalar new_value;
	bool isCoMaxWrite;

	/// if not `nullptr`, it contains an error encountered during the handling of the RMW op
	std::unique_ptr<ModelCheckerError> error;
	// TODO GENMC(mixed atomics non atomics): also add isCoMaxWrite here

private:
	ReadModifyWriteResult(std::string msg) : error(std::make_unique<ModelCheckerError>(msg))
	{
		old_value = GenmcScalar();
		new_value = GenmcScalar();
	}

public:
	ReadModifyWriteResult(SVal old_value, SVal new_value, bool isCoMaxWrite)
		: old_value(GenmcScalar(old_value)), new_value(GenmcScalar(new_value)),
		  isCoMaxWrite(isCoMaxWrite), error(nullptr)
	{}

	static ReadModifyWriteResult fromError(std::string msg)
	{
		return ReadModifyWriteResult(msg);
	}

	bool is_error() const { return error.get() != nullptr; }
};

struct CompareExchangeResult {
	GenmcScalar read_scalar;
	bool is_success; // TODO: handle this better
	bool isCoMaxWrite;

	/// if not `nullptr`, it contains an error encountered during the handling of the RMW op
	std::unique_ptr<ModelCheckerError> error;
	/// true if compare_exchange op was successful, false otherwise.

	static CompareExchangeResult success(SVal old_value, bool isCoMaxWrite)
	{
		auto read_scalar = GenmcScalar(old_value);
		// TODO GENMC: handle u128, and possibly different endianness
		return CompareExchangeResult{read_scalar, true, isCoMaxWrite, nullptr};
	}

	static CompareExchangeResult failure(SVal old_value)
	{
		auto read_scalar = GenmcScalar(old_value);
		return CompareExchangeResult{read_scalar, false, false, nullptr};
	}

	static CompareExchangeResult fromError(std::string msg)
	{
		auto dummy_scalar = GenmcScalar();
		return CompareExchangeResult{dummy_scalar, false, false,
					     std::make_unique<ModelCheckerError>(msg)};
	}

	bool is_error() const { return error.get() != nullptr; }
};

#endif /* GENMC_RESULT_HANDLING_HPP */
