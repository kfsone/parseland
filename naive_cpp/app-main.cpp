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


int main()
{
	///TODO: Read a file, maybe memmap it.
	kfs::Scanner scanner("enum EnumName { A, B C }  enum Bravo { X Y }");

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
        fmt::print("ast node #{}: {}: ", std::distance(ast.nodes_.cbegin(), it), (*it)->node_type());
        if (auto enum_ptr = dynamic_cast<kfs::EnumDefinition*>(it->get()); enum_ptr != nullptr)
        {
            for (const auto& child : enum_ptr->members_)
                fmt::print("'{}', ", child.source_);
        }
        else if (auto type_ptr = dynamic_cast<kfs::TypeDefinition*>(it->get()); type_ptr != nullptr)
        {
            fmt::print("not implemented");
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

/*
 * 🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧🚧
 * 🏗️ ACTIVE CONSTRUCTION 👷 It's all a bit chaotic here on - I want to rapidly flesh out use cases so that I can
 * come back and look at them to determine better organization and helpers.
 */

