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

    // Default construction.
    constexpr ASTNode() = default;
    // We're expecting derived types so we need a virtual dtor.
    virtual ~ASTNode() = default;

    // Construct with a single, explicit, token as the root token for the node.
    explicit constexpr ASTNode(const kfs::Token& root) : root_(root) {}

    // Default copy/move operators.
    constexpr ASTNode(const ASTNode&) = default;
    ASTNode(ASTNode&&) = default;
    constexpr ASTNode& operator = (const ASTNode&) = default;
    ASTNode& operator = (ASTNode&&) = default;

    //! Return a human readable name for this type of node, overriden by each derived type.
    [[nodiscard]]
    virtual std::string_view node_type() const = 0;

    //! Polymorphism helper: Dynamically cast this to a derived type, or null.
    template<typename T>
    constexpr T as() const noexcept { return dynamic_cast<const T>(this); }

    //! Polymorphism helper: Dynamically cast this to a derived type, or null.
    template<typename T>
    T as() noexcept { return dynamic_cast<T>(this); }

    //! Polymorphism helper: Return true if this is an instance of a derived type.
    template<typename T>
    [[nodiscard]]
    constexpr bool is() const noexcept { return this->as<const T*>() != nullptr; }
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
