// Unit tests for the naive CPP parser

#include "parser.h"

#include <gtest/gtest.h>

using namespace kfs;


struct TestParser : public Parser
{
	using Parser::Parser;

	const string_view& source() const noexcept { return source_; }
	const string_view& current() const noexcept { return current_; }

	void seek(size_t i)
	{
		current_.remove_prefix(i);
	}

public:
	using Parser::make_token;
	using Parser::unexpected_result;

	using Parser::peek;
	using Parser::peek_unchecked;
	using Parser::immediate;
	using Parser::immediate_unchecked;

	using Parser::skip_whitespace;
	using Parser::skip_comment;
};


// Sanity check the constructor does what it has to do.
TEST(ParserTest, Construction)
{
	TestParser parser{"hello world"};

	ASSERT_EQ("hello world", parser.source());
	ASSERT_EQ("hello world", parser.current());

	// Make sure our 'seek' helper works, too.
	parser.seek(5);
	EXPECT_EQ(" world", parser.current());
	ASSERT_EQ("hello world", parser.source());
}


TEST(ParserTest, MakeToken)
{
	// Lets check that 'make token' generates sensible looking tokens,
	// and shrinks 'current'.
	TestParser parser("ab");

	// Request a single-character 'word' token, which should take 'a'.
	Token token = parser.make_token(Token::Type::Word, 1);

	EXPECT_EQ(Token::Type::Word, token.type_);
	EXPECT_EQ("a", token.source_);

	EXPECT_EQ("b",  parser.current());	
	ASSERT_EQ("ab", parser.source());
}

TEST(ParserTest, UnexpectedResult)
{
	TestParser parser("!you");

	TResult result = parser.unexpected_result();

	EXPECT_EQ(true, result.is_error());
	EXPECT_EQ(true, result.has_token());
	EXPECT_EQ(Token::Type::Invalid, result.token().type_);
	EXPECT_EQ("unexpected character", result.error());
	EXPECT_EQ("!", result.token().source_);
	ASSERT_EQ("you", parser.current());
}

TEST(ParserTest, PeekEmpty)
{
	TestParser parser("");
	EXPECT_EQ('\0', parser.peek(0));
	EXPECT_EQ('\0', parser.peek(1));
	EXPECT_EQ('\0', parser.peek(999));
}

TEST(ParserTest, PeekPopulated)
{
	TestParser parser("ax");
	EXPECT_EQ('a', parser.peek(0));
	EXPECT_EQ('x', parser.peek(1));
	EXPECT_EQ('\0', parser.peek(2));
	EXPECT_EQ('\0', parser.peek(3));
}

TEST(ParserTest, PeekUncheckedEmpty)
{
	// Create a string view into a larger string so that we can
	// safely exceed bounds without actually crashing.
	std::string_view sv = "+-";
	TestParser parser(sv.substr(1, 0));
	ASSERT_EQ("", parser.source());
	EXPECT_EQ('-', parser.peek_unchecked(0));
}

TEST(ParserTest, PeekUncheckedPopulated)
{
	// Again with the smaller string view
	std::string_view sv = "+Az-";
	TestParser parser(sv.substr(1, 2));
	ASSERT_EQ("Az", parser.current());
	EXPECT_EQ('A', parser.peek_unchecked(0));
	EXPECT_EQ('z', parser.peek_unchecked(1));
	EXPECT_EQ('-', parser.peek_unchecked(2));
}

TEST(ParserTest, ImmediateEmpty)
{
	TestParser parser("");
	ASSERT_EQ("", parser.current());
	EXPECT_EQ('\0', parser.immediate());
}


TEST(ParserTest, ImmediatePopulated)
{
	TestParser parser("4b");
	EXPECT_EQ('4', parser.immediate());
	parser.seek(1);
	EXPECT_EQ('b', parser.immediate());
	ASSERT_EQ("4b", parser.source());
}


TEST(ParserTest, ImmediateUncheckedEmpty)
{
	std::string_view sv = "#:";
	TestParser parser(sv.substr(1, 0));
	ASSERT_EQ("", parser.current());
	EXPECT_EQ(':', parser.immediate_unchecked());
}

TEST(ParserTest, ImmediateUncheckedPopulated)
{
	TestParser parser("~THx");
	EXPECT_EQ('~', parser.immediate_unchecked());
	parser.seek(2);
	EXPECT_EQ('H', parser.immediate_unchecked());
}

TEST(ParserTest, SkipWhitespace)
{
	TestParser parser(" \t\r\n\n\r\t X");
	// Test it can actually skip whitespace.
	EXPECT_EQ(true, parser.skip_whitespace());
	EXPECT_EQ('X', parser.immediate());
	EXPECT_EQ(false, parser.skip_whitespace());
	EXPECT_EQ('X', parser.immediate());
	
	// test the empty string
	parser.seek(1);
	EXPECT_EQ(false, parser.skip_whitespace());
	EXPECT_EQ(0, parser.immediate());
}

TEST(ParserTest, SkipCommentNoComment)
{
	TestParser parser_empty("");
	EXPECT_EQ(None, parser_empty.skip_comment());

	// If we give it 2 characters that aren't a comment, it should return None,
	// but test it with the first character being a slash first.
	TestParser parser_slash_x("/XZ");	// exceeds the limit of 2 chars.
	EXPECT_EQ(None, parser_slash_x.skip_comment());
	EXPECT_EQ("/XZ", parser_slash_x.current());  // confirm it didn't advance.
	parser_slash_x.seek(1);
	EXPECT_EQ(None, parser_slash_x.skip_comment());
	EXPECT_EQ('X', parser_slash_x.immediate());  // confirm it didn't advance.
	parser_slash_x.seek(1);
	// And now we should fail the < 2 test.
	EXPECT_EQ(None, parser_slash_x.skip_comment());
	EXPECT_EQ('Z', parser_slash_x.immediate());  // confirm it didn't advance.
}

TEST(ParserTest, SkipCommentLineComment)
{
	// Take a line comment and throw some extra comment-like stuff to ensure we don't
	// have any kind of conflict.
	TestParser parser("// // /*\nx");
	TResult result = parser.skip_comment();
	EXPECT_TRUE(result.is_token());
	EXPECT_EQ(Token::Type::LineComment, result.token().type_);
	EXPECT_EQ("// // /*", result.token().source_);
	EXPECT_EQ('\n', parser.immediate());
	EXPECT_EQ(None, parser.skip_comment());
}


TEST(ParserTest, SkipCommentBlockComment)
{
	// Simple, empty comment.
	// A couple of IDEs dislike me writing the string directly here
	TestParser parser("/**/X/**-**/Y");
	{
		TResult result = parser.skip_comment();

		EXPECT_TRUE(result.is_token());
		EXPECT_EQ(Token::Type::CloseComment, result.token().type_);
		EXPECT_EQ("/**/", result.token().source_);
		EXPECT_EQ('X', parser.immediate());
	}

	parser.seek(1);

	{
		TResult result = parser.skip_comment();
		EXPECT_TRUE(result.is_token());
		EXPECT_EQ(Token::Type::CloseComment, result.token().type_);
		EXPECT_EQ("/**-**/", result.token().source_);
	}

	EXPECT_EQ('Y', parser.immediate());
}

TEST(ParserTest, SkipCommentUnterminatedBlock)
{
	TestParser parser("/**XYZ/\n*\n/");
	TResult result = parser.skip_comment();
	EXPECT_TRUE(result.is_error());
	EXPECT_TRUE(result.has_token());
	EXPECT_EQ(Token::Type::OpenComment, result.token().type_);
	EXPECT_EQ("unterminated block comment", result.error());
	EXPECT_EQ("/**XYZ/\n*\n/", result.token().source_);
	EXPECT_EQ(0, parser.immediate());
}