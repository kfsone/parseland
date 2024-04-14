// Helper methods for AST generators.

#include "app-ast-helpers.h"
#include "app-tokensequence.h"

#include <fmt/core.h>


namespace kfs
{

using namespace std::string_literals;


//! helper to explain an unexpected end of input, describing what you were expecting or doing.
PResult unexpected_eoi(std::string_view after)
{
    return PResult::Err(fmt::format("unexpected end of input {}"sv, after));
}


//! helper to explain you got something other than the expected identifier.
PResult expected_identifier(std::string_view what, std::string_view after, std::string_view actual)
{
    return PResult::Err(fmt::format("expected {} after {}, got '{}'"sv, what, after, actual));
}


//! helper to explain encountering a value that did not match expectations and reports what it
//! did encounter; distinguishes between unexpected end of input and mismatch.
PResult not_expected(const TokenSequence& ts, std::string_view after, std::string_view expected)
{
    if (ts.is_empty())
        return unexpected_eoi(fmt::format("{}; expected {}", after, expected));
    return PResult::Err(fmt::format("unexpected {} after {}, expected {}", Token::type_to_str(ts.front().type_), after, expected));
}


//! helper to attempt take the next keyword on the expectation it's an identifier.
Result<Token> take_identifier(TokenSequence& ts, std::string_view what, std::string_view after)
{
    // EOI check, and grab the token while we're there.

    const auto &[word, ok] = ts.take_front();
    if (!ok)
        return Result<Token>::Err(unexpected_eoi(fmt::format("after {}"sv, after)).take_error());
    if (word.type_ != Token::Type::Word)
        return Result<Token>::Err(expected_identifier(what, after, word.source_).take_error());
    return Result<Token>::Some(word);
}


//! Helper that implements brace-and-comma handling around calls to a unit of code (the thunk). If the
//! thunk returns an error, then the loop is stopped. Otherwise, we continue.
PResult process_list(std::string_view label, TokenSequence& ts, Token open_brace, const std::function<PResult(TokenSequence&, Token)>& thunk)
{
    for (;;)
    {
        const auto& [token, ok] = ts.take_front();
        if (!ok)
        {
            /// TODO: tell the user where the list began (hence open_brace)
            (void) open_brace;
            return unexpected_eoi(fmt::format("during {} list, expected identifier or '}}'"sv, label));
        }

        // Check for end-of-list.
        if (token.type_ == Token::Type::RBrace)
            return PResult::None();

        // Try the thunk.
        auto result = thunk(ts, token);
        if (result.is_error())
            return result;

        // Consume optional trailing comma.
        while (ts.peek_ahead(Token::Type::Comma))
            ts.take_front();
    }
}

Result<Token> take_open_brace(TokenSequence& ts, std::string_view after)
{
    const auto& [open_brace, brace_ok] = ts.take_front();
    if (brace_ok)
        return Result<Token>::Some(open_brace);

	auto err = fmt::format("after {}, expected open brace ('{{')"sv, after);
    return Result<Token>::Err(unexpected_eoi(err).take_error());
}


}
