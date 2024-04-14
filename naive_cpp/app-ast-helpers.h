#pragma once
#ifndef INCLUDE_NAIVE_CPP_APP_AST_HELPERS_H
#define INCLUDE_NAIVE_CPP_APP_AST_HELPERS_H

#include "app-ast.h"

#include <string_view>

namespace kfs
{

// returns an Error explaining an unexpected end of input.
PResult unexpected_eoi(std::string_view);

// return an error explaining the expectation of an identifier was not met.
PResult expected_identifier(std::string_view what, std::string_view after, std::string_view actual);

// extract and return the front token from a stream if it is a word (identifier) or else return an 'expected identifier' error.
Result<Token> take_identifier(TokenSequence& ts, std::string_view what, std::string_view after);

// common implementation of list-processing.
PResult process_list(std::string_view label, TokenSequence& ts, Token open_brace, const std::function<PResult(TokenSequence&, Token)>& thunk);

// extract and return the front token if it is an open brace.
Result<Token> take_open_brace(TokenSequence& ts, std::string_view after);

}


#endif  //INCLUDE_NAIVE_CPP_APP_AST_HELPERS_H