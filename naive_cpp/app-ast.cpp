// Implement methods and helpers for the app's AST handling.
#include "app-ast.h"
#include "app-definitions.h"
#include "app-tokensequence.h"

#include <fmt/core.h>
#include <functional>

namespace kfs
{

using namespace std::string_literals;


//! helper to explain an unexpected end of input, describing what you were expecting or doing.
PResult unexpected_eoi(std::string_view after)
{
    return PResult::Err(fmt::format("unexpected end of input {}", after));
}


//! helper to explain you got something other than the expected identifier.
PResult expected_identifier(std::string_view what, std::string_view after, std::string_view actual)
{
    return PResult::Err(fmt::format("expected {} after {}, got '{}'", what, after, actual));
}


//! Attempt to draw the next top-level ast node from the TokenSequence.
Result<std::string_view> AST::next(TokenSequence& ts)
{
    // Draw the first token, which should be the type.
    const auto& [token, ok] = ts.take_front();
    if (!ok)
        return Result<std::string_view>::None();

    // Since it should be a keyword, it has to be a word.
    if (token.type_ != Token::Type::Word)
        return Result<std::string_view>::Err(fmt::format("unexpected '{}' at top-level, expecting keywords 'enum' or 'type'", token.source_));

    // Since we now think this is either an enum or a type, go ahead and call the Definition factory.
    PResult result = kfs::Definition::make(ts, token);
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


//! Helper that implements brace-and-comma handling around calls to a unit of code (the thunk). If the
//! thunk returns an error, then the loop is stopped. Otherwise, we continue.
PResult process_list(std::string_view label, TokenSequence& ts, kfs::Token open_brace, const std::function<PResult(TokenSequence&, kfs::Token)>& thunk)
{
    for (;;)
    {
        const auto& [token, ok] = ts.take_front();
        if (!ok)
        {
            /// TODO: tell the user where the list began (hence open_brace)
            (void) open_brace;
            return unexpected_eoi(fmt::format("during {} list, expected identifier or '}}'", label));
        }

        // Check for end-of-list.
        if (token.type_ == kfs::Token::Type::RBrace)
            return PResult::None();

        // Try the thunk.
        auto result = thunk(ts, token);
        if (result.is_error())
            return result;

        // Consume optional trailing comma.
        while (ts.peek_ahead(kfs::Token::Type::Comma))
            ts.take_front();
    }
}


// EnumDefinition factory, processes
//  enum :- 'enum' name:WORD enum-member-list;
//  enum-member-list :- '{' (WORD ','*)* '}';
//
PResult EnumDefinition::make(TokenSequence& ts, kfs::Token first)
{
    // Grab the first node, see if it checks out.
    const auto &[enum_name, ok] = ts.take_front();
    if (!ok)
        return unexpected_eoi("after 'enum' keyword");
    if (enum_name.type_ != kfs::Token::Type::Word)
        return expected_identifier("enum name", "'enum' keyword", enum_name.source_);

    // We have a name.
    auto ptr = std::make_unique<EnumDefinition>(first, enum_name);
    /// todo: log?

    const auto& [open_brace, brace_ok] = ts.take_front();
    if (!brace_ok)
        return unexpected_eoi("after enum name, expected '{'");

    auto result = process_list(
            fmt::format("member list", enum_name.source_), ts, open_brace,
            [&ptr] (TokenSequence& ts, kfs::Token name) -> PResult {
                // Validate: check for a word
                if (name.type_ != kfs::Token::Type::Word)
                    return expected_identifier("member name (identifier), or '}'", "enum member list",
                                               name.source_);

                if (ptr->lookup(name.source_).has_value())
                    return PResult::Err(fmt::format("duplicate enum member, '{}'", name.source_));

                // Make sure there's at least one non-'_' in the name.
                if (name.source_.find_first_not_of('_') == std::string_view::npos)
                    return PResult::Err(fmt::format("invalid enum member name, '{}'", name.source_));

                // Assign the value of the current 0-based size.
                ptr->lookup_[name.source_] = ptr->members_.size();
                ptr->members_.push_back(name);

                return PResult::None();
            }
    );

    // An enum must have at least one member.
    if (ptr->members_.empty())
        return PResult::Err("enum '{}' has no members: enums must have *at least* one member");

    if (result.is_error())
        // prepend the enum name to the message.
        return PResult::Err(fmt::format("enum '{}': {}", enum_name.source_, result.error()));

    return PResult::Some(std::move(ptr));
}


// TypeDefinition factory, processes
//  type :- 'type' name:WORD [ ':' parent:WORD ] type-member-list;
//  type-member-list :- '{' (type-member ','*)* '}';
//
PResult TypeDefinition::make(TokenSequence& ts, kfs::Token first)
{
    return PResult::Err("not implemented");
}


// Factory for user-defined types which determines whether we are seeing
// an enum or a type definition.
//
PResult Definition::make(TokenSequence &ts, kfs::Token first)
{
    // expect 'enum' or 'type' to determine which type we're defining.
    if (const auto& [type_, source_] = first; type_ == kfs::Token::Type::Word)
    {
        if (source_ == "enum")
            return EnumDefinition::make(ts, first);
        else if (source_ == "type")
            return TypeDefinition::make(ts, first);
    }

    if (first.type_ == kfs::Token::Type::RBrace)
        return PResult::Err("unmatched close-brace at top-level, did you add too many }s?");

    return PResult::Err(fmt::format("expected either 'enum', or 'type'; got '{}'", first.source_));
}


}