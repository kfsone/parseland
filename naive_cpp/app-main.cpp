/*
 * ParseLand :: naive_cpp :: main
 * Copyright (C) Oliver 'kfsone' Smith <oliver@kfs.org> 2024, under MIT license.
 *
 * Implementation of a parser based on the naive cpp scanner.
 * 
 * We're not going to be fancy, or efficient: just toss all the tokens into a vector and
 * then worry about them after that.
 * 
 * We can then focus on how we consume/inspect the tokens.
 */


#include "result.h"
#include "scanner.h"
#include "token.h"

#include "app-fwd.h"
#include "app-ast.h"
#include "app-definitions.h"
#include "app-tokensequence.h"

#include <functional>
#include <map>
#include <vector>

// Enable fmt::...
#include <fmt/core.h>

// enable std::string literals
using namespace std::string_literals;
using namespace std::string_view_literals;

// Forward declarations so I can write this in reading order.
std::vector<kfs::Token> collect_tokens(kfs::Scanner& scanner, bool verbose);


void describe_value(const kfs::Value& value)
{
    if (auto scalar = dynamic_cast<const kfs::ScalarValue*>(&value); scalar != nullptr)
    {
        fmt::print("[scalar]type {}: '{}'[/scalar]", int(scalar->type_), scalar->root_.source_);
        return;
    }
    else if (auto enumval = dynamic_cast<const kfs::EnumValue*>(&value); enumval != nullptr)
    {
        fmt::print("[enum]{}::{}[/enum])", enumval->enum_type().source_, enumval->enum_name().source_);
    }
    else if (auto compound = dynamic_cast<const kfs::CompoundValue*>(&value); compound != nullptr)
    {
        switch (compound->resolved_type_)
        {
            case kfs::CompoundValue::Type::Unknown:
                fmt::print("<unidentified compound />");
                return;
            case kfs::CompoundValue::Type::Unit:
                fmt::print("[unit]{{}}[/unit]");
                return;
            case kfs::CompoundValue::Type::Array:
                fmt::print("[array]...[/array]");
                return;
            case kfs::CompoundValue::Type::Object:
                fmt::print("[object]...[/object]");
                return;
            default:
                fmt::print("<ERROR: invalid compound type/>");
                exit(1);
                return;           
        }
    }
    else
    {
        fmt::print("unknown value type\n");
        exit(1);
    }
}

int main()
{
	///TODO: Read a file, maybe memmap it.
	kfs::Scanner scanner(R"(
// test comment
enum EnumName { A, B C }  enum Bravo { X Y }
/* test comment */
enum ConnectionState { DISCONNECTED, CONNECTED, ERROR }
type Connected { ConnectionState state = ConnectionState :: DISCONNECTED}
type Connection : Connected { string name, Users users[] = { { x=1, y=1} } }
)");

	///NAIVE: We could process the tokens as we go, but that would mean
	///having some kind of stream wrapper. So for now, the simple route.
	std::vector<kfs::Token> scanned_tokens = collect_tokens(scanner, true);
	fmt::print("collected {} tokens\n", scanned_tokens.size());

	kfs::TokenSequence tokens{ scanned_tokens.begin(), scanned_tokens.end() };
	kfs::AST ast;
	for (;;)
    {
        if (auto result = ast.next(tokens); result.is_error())
        {
            fmt::print("{}: error: {}", /*filename*/"<input>", result.error());
            return 22;
        }
        else if (result.is_none())
        {
            break;
        }
        else
        {
            fmt::print("- ast added {}\n", result.value());
        }
	}

    fmt::print("collected {} ast nodes\n", ast.nodes_.size());

    for (auto it = ast.nodes_.cbegin(); it != ast.nodes_.cend(); ++it)
    {
        fmt::print("ast node #{}: {}:\n|  ", std::distance(ast.nodes_.cbegin(), it), (*it)->node_type());
        if (auto enum_ptr = dynamic_cast<kfs::EnumDefinition*>(it->get()); enum_ptr != nullptr)
        {
            fmt::print("name={}: ", enum_ptr->name_.source_);
            for (const auto& child : enum_ptr->members_)
                fmt::print("child={}, ", child.source_);
        }
        else if (auto type_ptr = dynamic_cast<kfs::TypeDefinition*>(it->get()); type_ptr != nullptr)
        {
            fmt::print("name={}: ", type_ptr->name_.source_);
            if (type_ptr->parent_type_)
                fmt::print("(derived from {}), ", type_ptr->parent_type_.value().source_);

            if (type_ptr->members_.empty())
                fmt::print("<no members>");
            else
            for (const auto& child: type_ptr->members_)
            {
                fmt::print("\n|  |  {}'{}' '{}'",
                           (child->is_array_ ? "[]" : "scalar"),
                           child->type_name().source_, child->name_.source_);
                if (child->default_)
                {
                    fmt::print("; default=");
                    describe_value(*reinterpret_cast<kfs::Value*>(child->default_.get()));
                }
            }

        }
        else
        {
            fmt::print("unrecognized node type [do you have rtti disabled?]");
        }
        fmt::print("\n");
    }
}


std::vector<kfs::Token> collect_tokens(kfs::Scanner& scanner, bool verbose)
{
	std::vector<kfs::Token> scanned_tokens{};
	for ( ; /*ever*/ ; )
	{
		auto result = scanner.next();
		if (result.is_token())
		{
			auto offset = scanner.get_token_offset(result.token());
			if (!offset.has_value())
			{
				fmt::print(stderr, "unreadable token error\n");
				return {};
			}

			if (verbose)
				fmt::print("token: offset:{} type:{:d} text:|{}|\n", offset.value(), int(result.token().type_), result.token().source_);

			scanned_tokens.emplace_back(result.token());

			continue;
		}

		if (!result.is_error())
		{
			if (verbose)
				fmt::print("end of input\n");
			break;
		}

		fmt::print("error: {}\n", result.error());
	}

	return scanned_tokens;
}
