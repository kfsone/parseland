#pragma once
#ifndef INCLUDED_NAIVE_CPP_APP_TOKENSEQUENCE_H
#define INCLUDED_NAIVE_CPP_APP_TOKENSEQUENCE_H

#include "token.h"

#include <iterator>
#include <optional>
#include <vector>


namespace kfs
{

    //! A consecutive sequence of scanner tokens that may be some or all of a document.
    struct TokenSequence
    {
        using data_type = std::vector<Token>;
        using iterator  = data_type::iterator;
        using difference_type = data_type::difference_type;

        iterator begin_;
        iterator end_;

        bool is_empty() const { return (begin_ == end_); }
        auto length()   const { return std::distance(begin_, end_); }

        // Unchecked!
        Token front() const { return *begin_; }

        const Token& advance()
        {
            std::advance(begin_, 1);
            return *(begin_ - 1);
        }

        std::pair<Token, bool> take_front()
        {
            if (is_empty())
                return {};
            Token first = *(begin_);
            std::advance(begin_, 1);
            return {first, true};
        }

        // Take the front-most token, but only if it matches type.
        std::pair<Token, bool> take_front(Token::Type type)
        {
            if (!peek_ahead(type))
                return {};
            Token first = *(begin_);
            std::advance(begin_, 1);
            return {first, true};
        }

        std::optional<const Token> peek(difference_type n) const
        {
            if (n < 0 || n >= length())
                return std::nullopt;
            return *std::next(begin_, n);
        }

        bool peek_ahead(Token::Type type) const
        {
            return !is_empty() && (begin_->type_ == type);
        }

        //! returns true if there is a token `n` tokens ahead which has type `type`.
        //! false if n is outside the range of the current view.
        bool peek_ahead(difference_type n, Token::Type type) const
        {
            if (n < 0 || n >= length())
                return false;
            return std::next(begin_, n)->type_ == type;
        }
    };

}


#endif  //INCLUDED_NAIVE_CPP_APP_TOKENSEQUENCE_H
