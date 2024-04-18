// Implement methods and helpers for the app's AST handling.
#include "app-ast.h"
#include "app-ast-helpers.h"
#include "app-definitions.h"
#include "app-tokensequence.h"

#include <fmt/core.h>
#include <functional>

/*
 * 🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧
 * 🏗️ ACTIVE CONSTRUCTION 👷 It's all a bit chaotic here on - I want to rapidly flesh out use cases
 * so I can come back and review them to determine better organization and helpers.
 * 🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧
 */


namespace kfs
{

//! Attempt to parse the next top-level ast node from the TokenSequence.
//
// This is the entry point for the parse tree representing the top level
// of the grammar.
//
Result<std::string_view> AST::next(TokenSequence& ts)
{
    // file <- definition*;
    // definition <- ^ ('enum' <enum-definition> / 'type' <type-definition> )
    // (using '^' to denote 'we are here'

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
    Definition* defn = result.value()->as<Definition*>();
    const auto name = defn->name_.source_;
    if (definitions_.contains(name))
        return Result<std::string_view>::Err(fmt::format("'{}' redefinition", name));
    // Not already present, take ownership and register the name.
    nodes_.emplace_back(result.take_value());
    definitions_[name] = defn;

    // Let the caller know the name of the type defined.
    return Result<std::string_view>::Some(name);
}


// EnumDefinition thunk for `process_list` to invoke for each possible member of
// an enumeration list.
//
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

    // process_list doesn't care about values, so just give it None.
    return PResult::None();
}


//! Factory for EnumDefinitions
//
// EnumDefinition factory, processes
//  enum <- 'enum' ^ name:WORD enum-member-list;
//  enum-member-list <- '{' (WORD ','*)* '}';
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


// TypeDefinition thunk for `process_list` to use to try and parse the member fields in a type body.
//
PResult parse_type_member(TypeDefinition& type_def, TokenSequence& ts, Token member_type_name)
{
    // type_member <- field_definition
    // field_definition <- type_name ^ member_name arity? default?
    auto field_def = FieldDefinition::make(ts, member_type_name);
    if (!field_def.is_value())
        return field_def;

    FieldDefinition& field = *field_def.value()->as<FieldDefinition*>();
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


// TypeDefinition helper that returns true if the parse stream contains an array designator
// ('[' ws* ']').
//
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


//! Factory for FieldDefinition (member fields of types).
//
PResult FieldDefinition::make(TokenSequence& ts, Token member_type_name)
{
    // type_definition := <member-type-name> ^ <member-name> <arity>? <default-value>? ','?
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

    // Check if the next thing is an equals sign, in which case we think we have a default value.
    auto equals = ts.take_front(Token::Type::Equals);
    if (equals.second)
    {
        auto front = ts.take_front();
        PResult value {};
        if (front.second)
            value = Value::make(ts, front.first);
        if (value.is_none())
            return unexpected_eoi("default value after '='");
        if (value.is_error())
            return PResult::Err(value.take_error());
        ptr->default_ = value.take_value();
    }

    return PResult::Some(std::move(ptr));
}


//! TypeDefinition helper to recognize and capture the parent type name.
//
Result<Token> parse_type_parent(TokenSequence& ts, Token type_name)
{
    // type_parent := ^ ( ':' word )?;
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


//! TypeDefinition factory.
//
PResult TypeDefinition::make(TokenSequence& ts, Token first)
{
    //  type :- 'type' ^ name:WORD [ ':' parent:WORD ] type-member-list;
    //  type-member-list :- '{' (type-member ','*)* '}';
    //
    auto type_name = take_identifier(ts, "type name", "'type' keyword");
    if (!type_name.is_value())
        return PResult::Err(type_name.take_error());

    if (ts.is_empty())
        return unexpected_eoi("type name, expected ':' or '{'");

    // If there's a colon here, attempt to capture a parent type-name.
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

    // Now we want the body, which should begin with a brace.
    auto open_brace = take_open_brace(ts, "type name");
    if (open_brace.is_error())
        return PResult::Err(open_brace.take_error());

    // Loop over parse_type_member while looking for the close '}'
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


//! Definition factory - expects 'enum' or 'type' and will use this to infer
//! the type of Definition to create.
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


//! Value factory - infers the derived type to construct.
//
PResult Value::make(TokenSequence& ts, Token first)
{
    // value <- '{' ^ <compound> / ^<scalar>^
    if (first.type_ == Token::Type::LBrace)
        return CompoundValue::make(ts, first);

    if (auto result = ScalarValue::make(ts, first); !result.is_error())
        return result;

    return PResult::Err(
        fmt::format("syntax-error: expected a string, number, boolean, enum::label, array, or object; got {} '{}'"sv,
                    Token::type_to_str(first.type_), first.source_));
}


//! ScalarValue factory - determines whether we are looking at a boolean, scoped enum,
//! float, integer, or string, and forwards to the relevant derived factory.
//
PResult ScalarValue::make(TokenSequence& ts, Token first)
{
    // scalar := ('true' / 'false' / <number> / <string>)^ / (enum_type_name:word ^ '::' enum_field_name:word)
    switch (first.type_)
    {
    case Token::Type::Word:
        if (first.source_ == "true"sv || first.source_ == "false"sv)
            return PResult::Some(std::make_unique<ScalarValue>(first, Type::Bool));

        // scoped_enum <- word scope_operator:'::' word;
        if (!ts.is_empty() && ts.peek_ahead(Token::Type::Scope))
        {
            auto result = EnumValue::make(ts, first);
            if (!result.is_error())
                return result;
        }
        break;

    case Token::Type::Float:
        return PResult::Some(std::make_unique<ScalarValue>(first, Type::Float));

    case Token::Type::Integer:
        return PResult::Some(std::make_unique<ScalarValue>(first, Type::Int));

    case Token::Type::String:
        return PResult::Some(std::make_unique<ScalarValue>(first, Type::String));

    default:
        break;
    }

    return PResult::Err("expected a scalar value");
}


//! CompoundValue helper that tries to resolve/ensure consistency of a
//! compound value.
//
Result<CompoundValue::Type> resolve_compound_type(CompoundValue& compound)
{
    // If it contains no elements, then we can't actually distinguish between
    // it being an array vs an object, so we call it Unit, which is a sort of
    // schroedinger-type.
    if (compound.values_.empty())
        return Result<CompoundValue::Type>::Some(CompoundValue::Type::Unit);

    // Ensure all the list values have the same type: Grab the first type and then
    // ask everything in the list whether it has the same type. Obviously the
    // first element does.
    auto first_type = compound.values_.front()->node_type();
    for (const auto& value : compound.values_)
    {
        // Do we need to allow a mix of float/int? At the moment it assumes
        // you can have {1,2} and {3.0,.4} but not {0.5, 1}
        if (value->node_type() != first_type)
        {
            return Result<CompoundValue::Type>::Err(fmt::format("invalid compound mixes types ({} and {})", first_type, value->node_type()));
        }
    }

    auto sample = compound.values_.front().get();
    // If the list is made of key-value pairs, this must be an object.
    if (const auto first = sample->as<FieldValue*>(); first != nullptr)
        return Result<CompoundValue::Type>::Some(CompoundValue::Type::Object);

    // If the list is made of objects (or unit), this is an array according to the ParseLand dsl.
    if (const auto first = sample->as<CompoundValue*>(); first != nullptr)
        return Result<CompoundValue::Type>::Some(CompoundValue::Type::Array);

    return Result<CompoundValue::Type>::Err(fmt::format("expected object or array of objects, got an array of {}", first_type));
}


//! CompoundValue factory: compounds are the brace-enclosed multi-value types,
//! or the empty variant which I'm calling Unit.
//
PResult CompoundValue::make(TokenSequence& ts, Token first)
{
    // compound <- '{' ^ ( <string> ':' <value> ',' )* '}';
    if (first.type_ != Token::Type::LBrace)
        return PResult::Err("expected a compound value");

    // If the next non-whitespace token after { is the }, then we have an empty
    // entry which we cannot distinguish between an array vs an object at this point.
    auto ptr = std::make_unique<CompoundValue>(first);
    if (auto result = ts.take_front(Token::Type::RBrace); result.second)
    {
        ptr->resolved_type_ = Type::Unit;
        return PResult::Some(std::move(ptr));
    }

    // Collect all the values without trying to assess whether they are valid or not.
    auto result = process_list(
            "compound value", ts, first,
            [&ptr] (TokenSequence& ts, Token first) -> PResult {
                // Compound can be one of three things: unit, array, or object. unit is the
                // empty case ({}), array is a list of Values, object is a list of
                // key=value fields.
                //
                //  compound <- unit / array / object
                //    unit <- '{' '}'
                //    array <- '{' (value ','?)+ '}'
                //    object <- '{' (field:word '=' value:value ','?)+ '}'

                // Check for `word '=' ...` to see if we're seeing field-value
                PResult result{};
                if (first.type_ == Token::Type::Word && ts.peek_ahead(Token::Type::Equals))
                    result = FieldValue::make(ts, first);
                else
                    result = Value::make(ts, first);
                if (result.is_error())
                    return result;

                // Take and keep the value
                ptr->values_.emplace_back(dynamic_cast<Value*>(result.take_value().release()));

                // Tell process_list there was no error.
                return PResult::None();
            }
    );

    if (result.is_error())
        return PResult::Err(fmt::format("compound value: {}", result.error()));

    auto resolve = resolve_compound_type(*ptr);
    if (resolve.is_error())
        return PResult::Err(resolve.take_error());

    ptr->resolved_type_ = resolve.value();

    return PResult::Some(std::move(ptr));
}


//! Factory for scoped enumeration values (`State::Connected`)
//
PResult EnumValue::make(TokenSequence& ts, Token first)
{
    if (const auto& [scope_op, present] = ts.take_front(Token::Type::Scope); !present)
        return not_expected(ts, "enum class name", "scope operator ('::')");
    auto member = take_identifier(ts, "enum member name", "scope operator ('::')");
    if (!member.is_value())
        return PResult::Err(member.take_error());
    
    auto ptr = std::make_unique<EnumValue>(first);
    ptr->field_ = member.take_value();

    return PResult::Some(std::move(ptr));
}         


PResult FieldValue::make(TokenSequence& ts, Token first)
{
    // field_value <- field_name:word ^ '=' value;
    if (auto result = ts.take_front(Token::Type::Equals); !result.second)
        return not_expected(ts, "field name", "equals ('=')");

    auto ptr = std::make_unique<FieldValue>(first);

    auto value_first = ts.take_front();
    if (!value_first.second)
        return unexpected_eoi(fmt::format("field assignment ('{} =')", first.source_));

    auto new_value = Value::make(ts, value_first.first);
    if (new_value.is_error())
        return new_value;

    ptr->value_ = new_value.take_value();

    return PResult::Some(std::move(ptr));
}

}