#pragma once
#ifndef INCLUDED_NAIVE_CPP_APP_AST_H
#define INCLUDED_NAIVE_CPP_APP_AST_H

//! Defines the AST types for the naive-cpp app.

#include "app-fwd.h"

#include "result.h"
#include "token.h"

#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>


namespace kfs
{

using namespace std::string_view_literals;

struct ASTNode
{
    using OwningPtr = std::unique_ptr<ASTNode>;

    Token					root_;		// The token that invoked us.
    std::optional<ASTNode*>	parent_;	// Optional reference to our parent.

    constexpr ASTNode() = default;
    virtual ~ASTNode() = default;

    virtual std::string_view node_type() const { return "astnode"sv; }

    explicit constexpr ASTNode(const kfs::Token& root) : root_(root) {}
};


// An owning pointer to an ASTNode.
using PResult = Result<ASTNode::OwningPtr>;

// Type alias for a non-contiguous list of ast nodes.
using ASTOwnedNodes = std::vector<ASTNode::OwningPtr>;

struct AST
{
    ASTOwnedNodes nodes_;

    std::map<std::string_view /*name*/, Definition*> definitions_;

    Result<std::string_view /*name*/> next(TokenSequence& ts);
};

}


#endif  //INCLUDED_NAIVE_CPP_APP_AST_H
