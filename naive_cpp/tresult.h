#pragma once
#ifndef INCLUDED_KFS_NAIVE_CPP_TRESULT_H
#define INCLUDED_KFS_NAIVE_CPP_TRESULT_H


#include "token.h"
#include "result.h"


namespace kfs
{


//! TResult is used to encapsulate Token-or-error related returns, and is loosely
//! modelled after Rust's Result type. It is a specific implementation of kfs::Result
//! for Token.
//!
//! It can return "nothing" (None), a Token, an error string, or an error accompanied
//! by the Token that produced it.
//!
//!	 	.is_none()	 => neither error nor token,
//!		.is_error()  => error is present,
//!		.is_token()  => no error, and token is present,
//!		.has_token() => token is present (may also be an error)
//
struct TResult : public Result<Token>
{
	using Result::Result;
	//! is_token will return true only if a token is present without an error.
	bool is_token() const noexcept { return is_value(); }
	//! has_token will return true if a token is present, but it may be an error-related token.
	bool has_token() const noexcept { return has_value(); }

	//! token() will attempt to return the value of the token, you must check is_token() or has_token() first.
	const Token& token() const noexcept { return value(); }

};


}

#endif  // INCLUDED_KFS_NAIVE_CPP_TRESULT_H