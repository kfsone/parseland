#pragma once
#ifndef INCLUDED_KFS_NAIVE_CPP_RESULT_H
#define INCLUDED_KFS_NAIVE_CPP_RESULT_H


#include "common.h"

#include <optional>
#include <string>


namespace kfs
{

//! Result provides 4-way representation of return values:
//!
//!	 	.is_none()	 => neither error nor value, indicating a negative non-error result,
//!		.is_error()  => error is present,
//!		.is_value()  => no error, but a concrete value is present,
//!		.has_value() => true when is_value(), but can also be true with is_error() to
//!						provide additional error context.
//
struct val_t {};
struct err_t {};

template<typename ValueType>
struct Result
///TODO: disallow ValueType == string-like
{
	using data_type = ValueType;
	using err_type  = std::string;
	using self_type = Result<data_type>;

private:
	std::optional<data_type> value_ {};
	std::optional<err_type>	 error_ {};

public:
	constexpr Result() = default;
    static self_type None()  { return self_type(); }

	~Result() = default;
	Result(const Result& other) = default;
	Result(Result&&) = default;

    explicit Result(data_type value, val_t=val_t{})
		: value_(std::forward<data_type>(value))
	{}
    static self_type Some(data_type value)
    {
        return self_type(std::forward<data_type>(value), val_t{});
    }
    template<typename T>
    static self_type Some(Result<T>&& rhs)
    {
        return self_type(rhs.take_value(), val_t{});
    }

	explicit Result(err_type&& error, err_t={})
		: error_(std::move(error))
	{}
    static self_type Err(err_type&& error)
    {
        return self_type(std::move(error), err_t{});
    }

    explicit Result(data_type value, err_type&& error)
        : value_(std::forward<data_type>(value)), error_(std::move(error))
    {}

    [[nodiscard]]
	bool operator == (const self_type& other) const noexcept
	{
		if (has_value())
			return other.has_value() && value() == other.value();
		if (is_error())
			return other.is_error() && error() == other.error();
		return true;
	}

	//! is_none will return true if the value has no error or value.
	[[nodiscard]]
	bool is_none() const noexcept { return !value_ && !error_; }
	//! is_error will return true if an error is present.
    [[nodiscard]]
	bool is_error() const noexcept { return error_.has_value(); }
	//! is_value will return true only if a value is present without an error.
    [[nodiscard]]
	bool is_value() const noexcept { return value_.has_value() && !is_error(); }
	//! has_value will return true if a value is present, but it may be an error-related piece of information.
    [[nodiscard]]
	bool has_value() const noexcept { return value_.has_value(); }

	//! value() will attempt to return the value of the result, you must check is_value() or has_value() first.
    [[nodiscard]]
	const data_type& value() const noexcept { return value_.value(); }
    //! take the value
    [[nodiscard]]
    data_type&& take_value() noexcept { return std::move(*value_); }
	//! error() will attempt to return the error instead of the value, you must check is_error() first.
    [[nodiscard]]
	const err_type&  error() const noexcept { return error_.value(); }
    //! take the error
    err_type&& take_error() noexcept { return std::move(*error_); }

};


}

#endif  // INCLUDED_KFS_NAIVE_CPP_RESULT_H