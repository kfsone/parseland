// Implement methods and helpers for the app's AST handling.
#include "app-ast.h"
#include "app-ast-helpers.h"
#include "app-definitions.h"
#include "app-tokensequence.h"

#include <fmt/core.h>
#include <functional>

/*
 * 🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧
 * 🏗️ ACTIVE CONSTRUCTION 👷 It's all a bit chaotic here on - I want to rapidly flesh out use cases so that I can
 * come back and look at them to determine better organization and helpers.
 */

namespace kfs
{

//! Attempt to parse the next top-level ast node from the TokenSequence.
Result<std::string_view> AST::next(TokenSequence& ts)
{
    // file := definition*;
    // definition := |'enum'| <enum-definition> / |'type'| <type-definition>

    // The first token should be a Word naming the type.
    const auto& [token, ok] = ts.take_front();
    if (!ok)
        return Result<std::string_view>::None();
    if (token.type_ != Token::Type::Word)
        return Result<std::string_view>::Err(fmt::format("unexpected '{}' at top-level, expecting keywords 'enum' or 'type'", token.source_));

    // Call the definition factory which will determine if it was one of the expected values, and if so
    // process the remainder of the definition.
    PResult result = Definition::make(ts, token);
    if (result.is_error())
        return Result<std::string_view>::Err(result.take_error());

    // Validate: this key hasn't already been used.
    Definition* defn = dynamic_cast<Definition*>(result.value().get());
    const auto name = defn->name_.source_;
    if (definitions_.contains(name))
        return Result<std::string_view>::Err(fmt::format("'{}' redefinition", name));
    // Not already present, take ownership and register the name.
    nodes_.emplace_back(result.take_value());
    definitions_[name] = defn;

    // Let the caller know the name of the type defined.
    return Result<std::string_view>::Some(name);
}


// specifics of parsing an individual enum field to be called via list.
PResult parse_enum_member(EnumDefinition& enum_def, TokenSequence&, Token name)
{
    // enum_definition := <word> ','?

    // Validate: check for a word
    if (name.type_ != Token::Type::Word)
        return expected_identifier("member name (identifier), or '}'", "enum member list",
                                   name.source_);

    // Check this isn't a duplicate of an existing type/enum.
    if (enum_def.lookup(name.source_).has_value())
        return PResult::Err(fmt::format("duplicate enum member, '{}'", name.source_));

    // Make sure there's at least one non-'_' in the name.
    if (name.source_.find_first_not_of('_') == std::string_view::npos)
        return PResult::Err(fmt::format("invalid enum member name, '{}'", name.source_));

    // Assign the value of the current 0-based size.
    enum_def.lookup_[name.source_] = enum_def.members_.size();
    enum_def.members_.push_back(name);

    // list doesn't care about values, so just give it None.
    return PResult::None();
}


// EnumDefinition factory, processes
//  enum :- 'enum' name:WORD enum-member-list;
//  enum-member-list :- '{' (WORD ','*)* '}';
//
PResult EnumDefinition::make(TokenSequence& ts, Token first)
{
    auto enum_name = take_identifier(ts, "enum name", "'enum keyword'");
    if (!enum_name.is_value())
        return PResult::Err(enum_name.take_error());

    // We have a name.
    auto ptr = std::make_unique<EnumDefinition>(first, enum_name.value());
    /// todo: log?

    auto open_brace = take_open_brace(ts, "type name");
    if (open_brace.is_error())
        return PResult::Err(open_brace.take_error());

    auto result = process_list(
            fmt::format("member list", enum_name.value().source_), ts, open_brace.value(),
            [&ptr] (TokenSequence& ts, Token name) -> PResult {
                return parse_enum_member(*ptr, ts, name);
            }
    );

    // An enum must have at least one member.
    if (ptr->members_.empty())
        return PResult::Err("enum '{}' has no members: enums must have *at least* one member");

    if (result.is_error())
        return PResult::Err(fmt::format("enum '{}': {}", enum_name.value().source_, result.error()));

    return PResult::Some(std::move(ptr));
}


// specifics of parsing an individual enum field to be called via list.
PResult parse_type_member(TypeDefinition& type_def, TokenSequence& ts, Token member_type_name)
{
    auto field_def = FieldDefinition::make(ts, member_type_name);
    if (!field_def.is_value())
        return field_def;

    FieldDefinition& field = *dynamic_cast<FieldDefinition*>(field_def.value().get());
    // Check this isn't a duplicate of an existing type/enum.
    if (type_def.lookup(field.name_.source_))
        return PResult::Err(fmt::format("duplicate enum member, '{}'", field.name_.source_));

    // Transfer ownership of the allocated field, stored as a generic ASTNode,
    // into the ownership table of the type definition, as a FieldDefinition proper.
    type_def.lookup_.emplace(field.name_.source_, dynamic_cast<FieldDefinition*>(field_def.take_value().release()));
    type_def.members_.push_back(&field);
    fmt::print("- adding member {} {}\n", field.type_name().source_, field.name_.source_);

    // we've handled ownership so just return not-an-error
    return PResult::None();
}


Result<bool> check_array_specifier(TokenSequence& ts)
{
    const auto& [open_token, open_present] = ts.take_front(Token::Type::LBracket);
    if (!open_present)
        return Result<bool>::Some(false);
    if (ts.is_empty())
        return Result<bool>::Err(unexpected_eoi("open-bracket ('[')").take_error());
    const auto& [close_token, close_present] = ts.take_front(Token::Type::RBracket);
    ///TODO: I'd like a nice error if they did `[4]` saying "they're dynamic" or something
    if (!close_present)
        return Result<bool>::Err(fmt::format("expecting close bracket (']') after open bracket ('['). arrays are dynamic and cannot have a fized size."));
    return Result<bool>::Some(true);
}


// Factory for member fields within type definitions.
PResult FieldDefinition::make(TokenSequence& ts, Token member_type_name)
{
    // type_definition := <member-type-name> <member-name> <arity>? <default-value>? ','?
    // Validate: check for a word
    if (member_type_name.type_ != Token::Type::Word)
        return expected_identifier("field type name, or '}'", "type definition",
                                   member_type_name.source_);

    auto member_name = take_identifier(ts, "member name", "field type name");
    if (member_name.is_error())
        return PResult::Err(member_name.take_error());

    // Make sure there's at least one non-'_' in the name.
    if (member_name.value().source_.find_first_not_of('_') == std::string_view::npos)
        return PResult::Err(fmt::format("invalid enum member name, '{}'", member_name.value().source_));

    auto ptr = std::make_unique<FieldDefinition>(member_type_name, member_name.value());

    Result<bool> is_array = check_array_specifier(ts);
    if (is_array.is_error())
        return PResult::Err(is_array.take_error());

    if (is_array.has_value())
        ptr->is_array_ = is_array.value();

    return PResult::Some(std::move(ptr));
}


PResult not_expected(const TokenSequence& ts, std::string_view after, std::string_view expected)
{
    if (ts.is_empty())
        return unexpected_eoi(fmt::format("{}; expected {}", after, expected));
    return PResult::Err(fmt::format("unexpected {} after {}, expected {}", Token::type_to_str(ts.front().type_), after, expected));
}


Result<Token> parse_type_parent(TokenSequence& ts, Token type_name)
{
    // type_parent := ':' word;
    auto colon = ts.take_front(Token::Type::Colon);
    if (!colon.second)
        return Result<Token>::None();

    auto parent_name = take_identifier(ts, "parent type name", Token::type_to_str(colon.first.type_));
    if (!parent_name.is_value())
        return Result<Token>::Err(parent_name.take_error());

    // Validate: parent can't be same as self.
    if (parent_name.value().source_ == type_name.source_)
        return Result<Token>::Err(fmt::format("type {} cannot have itself as a parent", type_name.source_));

    return Result<Token>::Some(parent_name.value());
}


// TypeDefinition factory, processes
//  type :- 'type' name:WORD [ ':' parent:WORD ] type-member-list;
//  type-member-list :- '{' (type-member ','*)* '}';
//
PResult TypeDefinition::make(TokenSequence& ts, Token first)
{
    // type >Foo< { ... }
    auto type_name = take_identifier(ts, "type name", "'type' keyword");
    if (!type_name.is_value())
        return PResult::Err(type_name.take_error());

    if (ts.is_empty())
        return unexpected_eoi("type name, expected ':' or '{'");

    std::optional<Token> parent;
    if (ts.peek_ahead(Token::Type::Colon))
    {
        if (auto result = parse_type_parent(ts, type_name.value()); result.is_error())
            return PResult::Err(result.take_error());
        else
            parent = result.value();
    }
    else if (!ts.peek_ahead(Token::Type::LBrace))
        return not_expected(ts, fmt::format("type name ({})", type_name.value().source_), "':' or '{'");

    // Create a type instance to begin populating.
    auto ptr = std::make_unique<TypeDefinition>(first, type_name.value());
    ptr->parent_type_ = parent;

    auto open_brace = take_open_brace(ts, "type name");
    if (open_brace.is_error())
        return PResult::Err(open_brace.take_error());

    auto result = process_list(
            fmt::format("member list", type_name.value().source_), ts, open_brace.value(),
            [&ptr] (TokenSequence& ts, Token name) -> PResult {
                return parse_type_member(*ptr, ts, name);
            }
    );
    if (result.is_error())
        return PResult::Err(fmt::format("type '{}': {}", type_name.value().source_, result.error()));

    return PResult::Some(std::move(ptr));
}


// Factory for user-defined types which determines whether we are seeing
// an enum or a type definition.
//
PResult Definition::make(TokenSequence &ts, Token first)
{
    // expect 'enum' or 'type' to determine which type we're defining.
    if (const auto& [type_, source_] = first; type_ == Token::Type::Word)
    {
        if (source_ == "enum")
            return EnumDefinition::make(ts, first);
        else if (source_ == "type")
            return TypeDefinition::make(ts, first);
    }

    if (first.type_ == Token::Type::RBrace)
        return PResult::Err("unmatched close-brace at top-level, did you add too many }s?");

    return PResult::Err(fmt::format("expected either 'enum', or 'type'; got '{}'", first.source_));
}


}