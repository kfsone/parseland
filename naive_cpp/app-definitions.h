#pragma once
#ifndef INCLUDE_NAIVE_CPP_APP_DEFINITIONS_H
#define INCLUDE_NAIVE_CPP_APP_DEFINITIONS_H

#include "app-fwd.h"
#include "app-ast.h"

#include <list>

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
    };

    //! Enum type definition.
    struct EnumDefinition final : public Definition
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
        [[nodiscard]]
        std::string_view node_type() const override { return "enum"sv; }

        //! Returns the value that the enumerator would resolve to if the name exists, otherwise nullopt.
        std::optional<size_t> lookup(std::string_view key)
        {
            if (auto it = lookup_.find(key); it != lookup_.end())
                return it->second;
            return std::nullopt;
        }
    };


    //! AST node describing a value, used by Field Definition to describe the default
    //! value. Can be recursive.
    struct Value : public ASTNode
    {
        // Factory.
        static PResult make(TokenSequence& ts, Token first);

        using ASTNode::ASTNode;
    };


    //! Simple literal value - a bool, int, float, or string.
    struct ScalarValue final : public Value
    {
        enum class Type { Bool, Float, Int, String, EnumField };

        explicit ScalarValue(Token first, Type type) : Value(first), type_(type) {}

        // Factory.
        static PResult make(TokenSequence& ts, Token first);

        using Value::Value;
        ~ScalarValue() override = default;
        Type  type_ {};

        [[nodiscard]]
        std::string_view node_type() const override { return "scalar value"sv; }
    };


    struct EnumValue final : public Value
    {
        // Factory.
        static PResult make(TokenSequence& ts, Token first);
        using Value::Value;
        ~EnumValue() override = default;

        Token field_;

        [[nodiscard]]
        std::string_view node_type() const override { return "scoped enum"sv; }

        [[nodiscard]]
        const Token& enum_type() const noexcept { return root_; }
        [[nodiscard]]
        const Token& enum_name() const noexcept { return field_; }
    };


    //! Represents a the default value of a field within an object instance.
    //! e.g { x = 1 }
    struct FieldValue final : public Value
    {
        // Factory.
        static PResult make(TokenSequence& ts, Token first);

        using Value::Value;
        ~FieldValue() override = default;

        // First token is the identifier naming the field, the second
        // part is more important - the value itself which may be complex
        // or simple.
        Value::OwningPtr value_;

        [[nodiscard]]
        std::string_view node_type() const override { return "object member value"sv; }

        [[nodiscard]]
        const Token& field_name() const noexcept { return root_; }
        [[nodiscard]]
        const Value* field_value() const noexcept { return value_->as<Value*>(); }
    };


    //! Represents any of the compound types - ie those enclosed in braces ('{', '}'),
    //! which means we can't always be certain why the type is: we don't know what
    //! '{ {} }' is until we know what the type of the field holding it is.
    struct CompoundValue final : public Value
    {
        // Factory.
        static PResult make(TokenSequence& ts, Token first);

        // std::list stops us being a constexpr in c++17
        using Value::Value;
        explicit CompoundValue(const kfs::Token& root) : Value(root) {}

        enum class Type
        {
            Unknown,        // We haven't/couldn't resolve.
            Unit,           // Empty, so we can't tell.
            Array,          // Contains objects or units,
            Object,         // Contains field-value pairs.
        };

        Type resolved_type_ {Type::Unknown};

        [[nodiscard]]
        bool object_or_unit() const noexcept { return resolved_type_ == Type::Object || resolved_type_ == Type::Unit; }
        [[nodiscard]]
        bool array_or_unit()  const noexcept { return resolved_type_ == Type::Array  || resolved_type_ == Type::Unit; }

        // First will be the opening brace of the token, so we need to also know
        // the list.
        Token last_;
        // And then all the values in-between.
        std::list<Value::OwningPtr> values_;

        [[nodiscard]]
        std::string_view node_type() const override { return "compound"sv; }
    };


    //! Describes a member field of a type definition.
    struct FieldDefinition final : public Definition
    {
        static PResult make(TokenSequence& ts, Token first);
        using ValuePtr = Value::OwningPtr;

        bool            is_array_  {false};
        ValuePtr        default_   {};

        using Definition::Definition;
        ~FieldDefinition() override = default;

        [[nodiscard]]
        const Token&    type_name() const { return root_; }

        [[nodiscard]]
        std::string_view node_type() const override { return "field-definition"sv; }
    };


    //! User-defined type definition.
    struct TypeDefinition final : public Definition
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
        [[nodiscard]]
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
