#pragma once
#ifndef INCLUDED_KFS_NAIVE_CPP_SCANNER_H
#define INCLUDED_KFS_NAIVE_CPP_SCANNER_H
// Copyright (C) Oliver 'kfsone' Smith, 2024 -- under MIT license terms.

#include "common.h"
#include "token.h"
#include "tresult.h"


namespace kfs
{


struct Scanner
{
public:
	Scanner(string_view source)
		: source_(source)
		, current_(source)
	{
	}

	//! next will attempt to fetch and return the next token. If end-of-input is reached,
	//! it will return None; if a valid token is found, it will return the token; if an
	//! error occurs it will return an error string describing the error, and possibly
	//! an accompanying Token describing the problem text.
	TResult next();

protected:
	string_view		source_;				// Original unmodified source view.
	string_view		current_;				// Reduced source view as we scan.

protected:
	/* ---------- Internal Methods, I hate pimpls ---------- */
	// make_token is a helper to create a token from the current view and advance the cursor.
	Token   make_token(Token::Type type, size_t len) noexcept;

	// indicate an unexpected character at the front of the current view.
	TResult unexpected_result() noexcept;

	// front will return the first character of the current view, or '\0' at eoi.
	char front() const noexcept { return peek(0); }

	// peek will return the Nth character of the current view, or '\0' if at/beyond oei.
	// note: the 0th character is 'front'.
	char peek(size_t offset) const noexcept { return offset < current_.size() ? current_[offset] : '\0'; }

	// skip_whitespace will advance past any whitespace characters or return false.
	bool skip_whitespace();

	// skip_comment will advance past a line or block comment at the current cursor.
	TResult skip_comment();

	// Try to produce a single-line quoted string from the current view at the opening quote.
	// If the string is unterminated by EOI/EOL, an error is returned.
	TResult scan_string();

	// Optimistic attempt to scan a number from either a digit or a decimal point.
	TResult scan_number();

	// Optimistic attempt to scan a signed integer/float from a leading sign (+/-).
	TResult scan_signed_number();

	// Scan a word token from the current view at the initial character. Note that
	// this will happily accept a digit as the first character, it's assumed that the
	// caller will already have made the distinction.
	TResult scan_word();
};


}


#endif  // INCLUDED_KFS_NAIVE_CPP_SCANNER_H