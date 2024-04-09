#pragma once
#ifndef INCLUDED_KFS_NAIVE_CPP_TOKEN_H
#define INCLUDED_KFS_NAIVE_CPP_TOKEN_H
// Copyright (C) Oliver 'kfsone' Smith, 2024 -- under MIT license terms.


#include "common.h"


namespace kfs
{


//! Token structure, representing a span of text with a specific syntactic meaning.
//!
//! The source text is represented by a string_view, which is a non-owning reference
//! to a c-string; callers are required to ensure that the source text outlives the
//! tokens scanned from it.
//
struct Token
{
	//! Token::Type enumerates the distinct base-types of token.
	enum class Type
	{
		Invalid,
		Word,
		Float,
		Integer,
		String,
		LBrace,
		RBrace,
		LBracket,
		RBracket,
		Equals,
		Scope,
		Colon,
		Comma,

		// Internal values
		EndOfInput,
		Whitespace,
		LineComment,
		OpenComment,
		CloseComment,
	};

	Type     		type_		{ Type::Invalid };
	string_view		source_		{ "" };

	bool operator == (const Token& rhs) const noexcept
	{
		return type_ == rhs.type_ && source_ == rhs.source_;
	}

    // Static helper: maps a type to a human displayable name.
    //#[derive(Debug)] please
    static string_view type_to_str(Token::Type type);
};



}

#endif  // INCLUDED_KFS_NAIVE_CPP_TOKEN_H