#pragma once
#ifndef INCLUDE_NAIVE_CPP_APP_DEFINITIONS_H
#define INCLUDE_NAIVE_CPP_APP_DEFINITIONS_H

#include "app-fwd.h"
#include "app-ast.h"

namespace kfs
{
    //! Common base class for all type definitions.
    struct Definition : public ASTNode
    {
        // Factory.
        static PResult make(TokenSequence& ts, Token first);

        Token  name_ {};

        // Constructor with the first two tokens - 'enum' and the name,
        explicit Definition(Token first, Token name) : ASTNode(first), name_(name) {}
        // Dtor needs to be virtual.
        ~Definition() override = default;
        // do not implement the node_type method, we are not concrete
        std::string_view node_type() const override = 0;
    };

    //! Enum type definition.
    struct EnumDefinition : public Definition
    {
        // Factory.
        static PResult make(TokenSequence& ts, Token first);

        using Lookup  = std::map<std::string_view, size_t>;
        using Members = std::vector<Token>;

        Members     members_ {};
        Lookup      lookup_ {};

        // Constructor with the first two tokens - 'enum' and the name,
        using Definition::Definition;
        // Dtor needs to be virtual.
        ~EnumDefinition() override = default;
        // Report that we're an enum node
        std::string_view node_type() const override { return "enum"sv; }

        //! Returns the value that the enumerator would resolve to if the name exists, otherwise nullopt.
        std::optional<size_t> lookup(std::string_view key)
        {
            if (auto it = lookup_.find(key); it != lookup_.end())
                return it->second;
            return std::nullopt;
        }
    };


    struct FieldDefinition : public Definition
    {
        static PResult make(TokenSequence& ts, Token first);

        bool            is_array_  {false};

        const Token&    type_name() const { return root_; }

        using Definition::Definition;
        ~FieldDefinition() override = default;

        std::string_view node_type() const override { return "field-definition"; }
    };


    //! User-defined type definition.
    struct TypeDefinition : public Definition
    {
        // Factory.
        static PResult make(TokenSequence& ts, Token first);

        // The parent might not be declared at the point we read a child, so
        // we don't presume to try and store a pointer to the object itself.
        using Parent = std::optional<Token>;
        // Ownership and lookup-by-name
        using OwnedField = std::unique_ptr<FieldDefinition>;
        using Lookup  = std::map<std::string_view, OwnedField>;
        // Field order.
        using Members = std::vector<FieldDefinition*>;

        Parent      parent_type_ {};
        Members     members_ {};
        Lookup      lookup_ {};

        // Constructor takes the first two tokens - the 'type' keyword and the type name.
        using Definition::Definition;
        // Dtor needs to be virtual.
        ~TypeDefinition() override = default;
        // Report that we're a type-definition node.
        std::string_view node_type() const override { return "type"sv; };

        //! Returns TypeMember with the give name if registered, otherwise nullptr.
        FieldDefinition* lookup(std::string_view key)
        {
            if (auto it = lookup_.find(key); it != lookup_.end())
                return it->second.get();
            return nullptr;
        }
    };

}

#endif  //INCLUDE_NAIVE_CPP_APP_DEFINITIONS_H
