// Naive implementation of the TypeDef grammar in C++.
//
// Copyright (C) Oliver 'kfsone' Smith, 2024 -- under MIT license terms.


#include "common.h"
#include "scanner.h"
#include "token.h"
#include "tresult.h"


namespace kfs
{


const TResult None = TResult{};


std::string_view Token::type_to_str(Token::Type type)
{
    using namespace std::string_view_literals;
    using Types = Token::Type;
    switch (type)
    {
        case Types::Word:         return "word"sv;
        case Types::Float:        return "float value"sv;
        case Types::Integer:      return "integer value"sv;
        case Types::String:       return "string literal"sv;
        case Types::LBrace:       return "open-brace ('{')"sv;
        case Types::RBrace:       return "close-brace ('}')"sv;
        case Types::LBracket:     return "open-bracket ('[')"sv;
        case Types::RBracket:     return "close-bracket (']')"sv;
        case Types::Equals:       return "equals ('=')"sv;
        case Types::Scope:        return "scope operator ('::')"sv;
        case Types::Colon:        return "colon (':')"sv;
        case Types::Comma:        return "comma (',')"sv;
        case Types::EndOfInput:   return "end-of-input"sv;
        case Types::Whitespace:   return "<whitespace>"sv;
        case Types::LineComment:  return "<line comment>"sv;
        case Types::OpenComment:  return "open comment ('/*')"sv;
        case Types::CloseComment: return "close comment ('*/')"sv;

        default:                  return "<invalid token>"sv;
    }
}


// Helper that creates a token from the current position in the source
// and advances past the token.
//
Token Scanner::make_token(Token::Type type, size_t len) noexcept
{
	Token token = Token{type, current_.substr(0, len)};
	current_.remove_prefix(len);
	return token;
}


// Helper that creates a TResult consuming the character at the front of current.
TResult Scanner::unexpected_result() noexcept
{
	return TResult{make_token(Token::Type::Invalid, 1), "unexpected character"};
}


// Determine if 'token' is from this source document and, if so, calculate its
// byte offset by abusing string-views; otherwise, return nullopt.
std::optional<size_t> Scanner::get_token_offset(const Token& token) const noexcept
{
	// MSVC will do some sanity checking if we try to compare begin/end, so pointers it is!
	const char *outer_begin =       source_.data(), *outer_end =       source_.data() +       source_.length();
	const char *inner_begin = token.source_.data(), *inner_end = token.source_.data() + token.source_.length();
	if (outer_begin <= inner_begin && outer_end >= inner_end)
		return token.source_.data() - source_.data();

	return std::nullopt;
}


// Attempts to identify the next token in the stream.
TResult Scanner::next()
{
	while (!current_.empty())
	{
		if (skip_whitespace())
			continue;

		if (auto result = skip_comment(); !result.is_none())
		{
			if (result.is_error())
				return result;
			// Track comment stats.
			comments_ += 1;
			comments_len_ += result.token().source_.length();
			continue;
		}

		// We're fairly confidence it should be a regular token now.
		switch (const char first = front(); first)
		{
		case '"':
			return scan_string();

		case '{':
			return TResult{make_token(Token::Type::LBrace, 1)};
		case '}':
			return TResult{make_token(Token::Type::RBrace, 1)};

		case '[':
			return TResult{make_token(Token::Type::LBracket, 1)};
		case ']':
			return TResult{make_token(Token::Type::RBracket, 1)};

		case ':':
			if (peek(1) == ':')
				return TResult{make_token(Token::Type::Scope, 2)};
			return TResult{make_token(Token::Type::Colon, 1)};

		case '=':
			return TResult{make_token(Token::Type::Equals, 1)};

		case ',':
			return TResult{make_token(Token::Type::Comma, 1)};

		case '+':
		case '-':
			return scan_signed_number();

		case '.':
			if (peek(1) >= '0' && peek(1) <= '9')
				return scan_number();
			break;

		case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
			return scan_number();

		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
		case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
		case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
		case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
		case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
		case '_':
			return scan_word();

		default:
			break;
		}

		return unexpected_result();
	}

	// We reached end of input.
	return None;
}


// If there is whitespace at the front of current, advance past it and
// return true, otherwise return false.
//
bool Scanner::skip_whitespace()
{
	size_t trimLen = 0;
	for (;!current_.empty(); trimLen++)
	{
		char next = peek(trimLen);
		if (next != ' ' && next != '\t' && next != '\r' && next != '\n')
			break;
	}

	current_.remove_prefix(trimLen);

	return trimLen > 0;
}


// Either return a token representing a comment we find at the front of
// current, return an error if there is an unterminated comment, or return
// None if there is no comment.
//
TResult Scanner::skip_comment()
{
	// Need at least two characters to start a comment.
	if (current_.size() < 2 || front() != '/')
		return None;

	if (peek(1) == '/')
	{
		// line comment.
		if (auto end = current_.find('\n'); end != current_.npos)
			return TResult{make_token(Token::Type::LineComment, end)};

		// There's no newline, so consume the rest of the string.
		return TResult{make_token(Token::Type::LineComment, current_.size())};
	}

	if (peek(1) != '*')
		return None;

	// block comment - simple version, no nesting.
	if (auto end = current_.find("*/", /*skip open*/2); end != current_.npos)
	{
		return TResult{make_token(Token::Type::CloseComment, end + 2)};
	}

	// This will cause the open comment to look like some unknown symbol.
	return TResult{make_token(Token::Type::OpenComment, current_.size()), "unterminated block comment"};
}


// Naive implementation of string scanner, doesn't handle escape sequences.
//
TResult Scanner::scan_string()
{
	auto end = current_.find_first_of("\"\r\n", /*skip open quote*/1);
	if (end != current_.npos)
	{
		if (peek(end) == '"')
			return TResult{make_token(Token::Type::String, end + 1)};

		return TResult{make_token(Token::Type::String, end), "unterminated string"};
	}

	return TResult{make_token(Token::Type::String, current_.size()), "unterminated string"};
}


// Handles a digit sequence that will either be an integer or float if we encounter
// a decimal point.
TResult Scanner::scan_number()
{
	// We take it as read that the caller checked the first character to be numeric,
	// so we start from character 1.
	bool is_float = false;
	size_t len = 1;
	for ( ; len < current_.size(); ++len)
	{
		if (const char c = peek(len); c >= '0' && c <= '9')
			continue;
		// if we see a '.', set is_float to true, and then if it wasn't already set,
		// allow another series of integers to follow.
		else if (c == '.' && !std::exchange(is_float, true))
			continue;
		// anything else is a stop.
		break;
	}
	return TResult{make_token(!is_float ? Token::Type::Integer : Token::Type::Float, len)};
}


// On encountering a +/- sign, optimistically assume it's going to be a number,
// so the next character will either be a digit which we hand off to scan_number
// and allow that to deal with finding out it's a float, or we find a '.' and if
// it's going to be a number, it's a float.
//
TResult Scanner::scan_signed_number()
{
	if (current_.size() > 1 && peek(1) >= '0' && peek(1) <= '9')
		return scan_number();

	if (peek(1) == '.')
	{
		size_t len = 2;
		for ( ; len < current_.size(); len++)
		{
			if (char c = peek(len); c < '0' || c > '9')
				break;
		}
		if (len > 2)	// sign + dot
			return TResult{make_token(Token::Type::Float, len)};
	}

	return unexpected_result();
}


// Handles a sequence of characters that started with an ascii letter or underscore.
TResult Scanner::scan_word()
{
	size_t len = 1;
	for (; len < current_.size(); len++)
	{
		const char c = peek(len);
		if ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && (c < '0' || c > '9') && c != '_')
			break;
	}

	return TResult{make_token(Token::Type::Word, len)};
}


}
