#pragma once
#ifndef INCLUDED_KFS_NAIVE_CPP_PARSER_H
#define INCLUDED_KFS_NAIVE_CPP_PARSER_H


#include "common.h"
#include "token.h"
#include "tresult.h"


namespace kfs
{


struct Parser
{
public:
	Parser(string_view source)
		: source_(source)
		, current_(source)
	{
	}

	//! next will attempt to fetch and return the next token. If end-of-input is reached,
	//! it will return None; if a valid token is found, it will return the token; if an
	//! error occurs it will return an error string describing the error, and possibly
	//! an accompanying Token describing the problem text.
	TResult next();

	struct Location
	{
		size_t	byte_offset_;
		size_t	line_;
		size_t	column_;
	};

	//! Expensive: Returns the location of the given Token in the source text, it does
	//! this by counting characters in the text preceeding the token...
	Location locate(Token) const noexcept;


protected:
	string_view		source_;				//! Original unmodified source view.
	string_view		current_;				//! Reduced source view as we parse.

protected:
	/* ---------- Internal Methods, I have pimpls ---------- */
	//! make_token is a helper to create a token from the current view and advance the cursor.
	Token   make_token(Token::Type type, size_t len) noexcept;

	//! indicate an unexpected character at the front of the current view.
	TResult unexpected_result() noexcept;

	//! immediate_unchecked will return the first character of the current view without,
	//! but does not check that the view is not empty.
	char immediate_unchecked() const noexcept { return current_.data()[0]; }
	//! immediate will return the first character of the current view, or '\0' at eoi.
	char immediate() const noexcept { return peek(0); }

	//! peek_unchecked will return the Nth character of the current view, without doing
	//! bounds checks first.
	char peek_unchecked(size_t offset=1) const noexcept { return current_.data()[offset]; }
	//! peek will return the Nth character of the current view, or '\0' if at/beyond oei.
	char peek(size_t offset=1) const noexcept { return offset < current_.size() ? current_[offset] : '\0'; }

	//! skip_whitespace will advance past any whitespace characters or return false.
	bool skip_whitespace();

	//! skip_comment will advance past a line or block comment at the current cursor.
	TResult skip_comment();

	TResult parse_string();
	TResult parse_signed_number();
	TResult parse_number(size_t offset);
	TResult parse_word();
};


}


#endif  // INCLUDED_KFS_NAIVE_CPP_PARSER_H