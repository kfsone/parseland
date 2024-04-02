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
        std::vector<Token>::iterator begin_;
        std::vector<Token>::iterator end_;

        bool is_empty() const { return (begin_ == end_); }
        auto length()   const { return std::distance(begin_, end_); }

        // Unchecked!
        Token front() const { return *begin_; }

        std::pair<Token, bool> take_front()
        {
            if (is_empty())
                return {};
            Token first = *(begin_);
            std::advance(begin_, 1);
            return {first, true};
        }

        std::optional<const Token> peek(std::vector<Token>::difference_type n) const
        {
            if (n < 0 || n >= length())
                return std::nullopt;
            return *std::next(begin_, n);
        }

        bool peek_ahead(Token::Type type) const
        {
            return !is_empty() && (begin_->type_ == type);
        }
    };

}


#endif  //INCLUDED_NAIVE_CPP_APP_TOKENSEQUENCE_H
