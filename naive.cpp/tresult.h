#pragma once
#ifndef INCLUDED_KFS_NAIVE_CPP_TRESULT_H
#define INCLUDED_KFS_NAIVE_CPP_TRESULT_H


#include "common.h"
#include "token.h"

#include <optional>
#include <string>


namespace kfs
{

using std::string;


//! TResult is used to encapsulate Token-or-error related returns, and is loosely
//! modelled after Rust's Result type.
//!
//! It can return "nothing" (None), a Token, an error string, or an error accompanied
//! by the Token that produced it.
//!
//!	 	.is_none()	 => neither error nor token,
//!		.is_error()  => error is present,
//!		.is_token()  => no error, and token is present,
//!		.has_token() => token is present (may also be an error)
//
struct TResult
{
private:
	const std::optional<Token>  token_ {};
	const std::optional<string> error_ {};

public:
	constexpr TResult() = default;
	~TResult() = default;
	TResult(const TResult& other) = default;
	TResult(TResult&&) = default;

	explicit TResult(Token token)
		: token_(token)
	{}

	explicit TResult(string error)
		: error_(std::move(error))
	{}

	explicit TResult(Token token, string error)
		: token_(token)
		, error_(std::move(error))
	{}

	bool operator == (const TResult& other) const noexcept
	{
		if (has_token())
			return other.has_token() && token() == other.token();
		if (is_error())
			return other.is_error() && error() == other.error();
		return true;
	}

	//! is_none will return true if the value has no error or token.
	bool is_none() const noexcept { return !token_ && !error_; }
	//! is_error will return true if an error is present.
	bool is_error() const noexcept { return error_.has_value(); }
	//! is_token will return true only if a token is present without an error.
	bool is_token() const noexcept { return token_.has_value() && !is_error(); }
	//! has_token will return true if a token is present, but it may be an error-related token.
	bool has_token() const noexcept { return token_.has_value(); }

	//! token() will attempt to return the value of the token, you must check is_token() or has_token() first.
	const Token& token() const noexcept { return token_.value(); }
	//! error() will attempt to return the value of the error, you must check is_error() first.
	const string& error() const noexcept { return error_.value(); }

};


//! None is a constant value representing no result for use in returns to look a bit rust-like.
extern const TResult None;


}

#endif  // INCLUDED_KFS_NAIVE_CPP_TRESULT_H