// Unit tests for the naive CPP scanner.

#include "scanner.h"

#include <gtest/gtest.h>

using namespace kfs;


// Because I made things private/protected, we'll need to surface things here.
struct TestScanner : public Scanner
{
	using Scanner::Scanner;

	const string_view& source() const noexcept { return source_; }
	const string_view& current() const noexcept { return current_; }

	void seek(size_t i)
	{
		current_.remove_prefix(i);
	}

public:
	using Scanner::make_token;
	using Scanner::unexpected_result;

	using Scanner::peek;
	using Scanner::front;

	using Scanner::skip_whitespace;
	using Scanner::skip_comment;

	using Scanner::scan_string;

	using Scanner::scan_word;
};


// Sanity check the constructor does what it has to do.
TEST(ScannerTest, Construction)
{
	TestScanner scanner{"hello world"};

	ASSERT_EQ("hello world", scanner.source());
	ASSERT_EQ("hello world", scanner.current());

	// Make sure our 'seek' helper works, too.
	scanner.seek(5);
	EXPECT_EQ(" world", scanner.current());
	ASSERT_EQ("hello world", scanner.source());
}


// Since we use make_token to construct our TResults, we want to ensure
// it works as advertised.
TEST(ScannerTest, MakeToken)
{
	// Lets check that 'make token' generates sensible looking tokens,
	// and shrinks 'current'.
	TestScanner scanner("ab");

	// Request a single-character 'word' token, which should take 'a'.
	Token token = scanner.make_token(Token::Type::Word, 1);

	EXPECT_EQ(Token::Type::Word, token.type_);
	EXPECT_EQ("a", token.source_);

	EXPECT_EQ("b",  scanner.current());	
	ASSERT_EQ("ab", scanner.source());
}


// Check the helper for unexpected character reports.
TEST(ScannerTest, UnexpectedResult)
{
	TestScanner scanner("!you");

	TResult result = scanner.unexpected_result();

	EXPECT_EQ(true, result.is_error());
	EXPECT_EQ(true, result.has_token());
	EXPECT_EQ(Token::Type::Invalid, result.token().type_);
	EXPECT_EQ("unexpected character", result.error());
	EXPECT_EQ("!", result.token().source_);
	ASSERT_EQ("you", scanner.current());
}


// Give it some characters.
TEST(ScannerTest, PeekPopulated)
{
	TestScanner scanner("ax");
	EXPECT_EQ('a', scanner.peek(0));
	EXPECT_EQ('x', scanner.peek(1));
	EXPECT_EQ('\0', scanner.peek(2));
	EXPECT_EQ('\0', scanner.peek(3));
}


// Test for different behavior when the string is empty; there are
// often early-outs in code based on ".empty()".
TEST(ScannerTest, PeekEmpty)
{
	TestScanner scanner("");
	EXPECT_EQ('\0', scanner.peek(0));
	EXPECT_EQ('\0', scanner.peek(1));
	EXPECT_EQ('\0', scanner.peek(999));
}


TEST(ScannerTest, FrontPopulated)
{
	TestScanner scanner("4b");
	EXPECT_EQ('4', scanner.front());
	scanner.seek(1);
	EXPECT_EQ('b', scanner.front());
	ASSERT_EQ("4b", scanner.source());
}


TEST(ScannerTest, FrontEmpty)
{
	TestScanner scanner("");
	ASSERT_EQ("", scanner.current());
	EXPECT_EQ('\0', scanner.front());
}


TEST(ScannerTest, SkipWhitespace)
{
	TestScanner scanner(" \t\r\n\n\r\t X");
	// Test it can actually skip whitespace.
	EXPECT_EQ(true, scanner.skip_whitespace());
	EXPECT_EQ('X', scanner.front());
	EXPECT_EQ(false, scanner.skip_whitespace());
	EXPECT_EQ('X', scanner.front());
	
	// test the empty string
	scanner.seek(1);
	EXPECT_EQ(false, scanner.skip_whitespace());
	EXPECT_EQ(0, scanner.front());
}


TEST(ScannerTest, SkipCommentNoCommentEmpty)
{
	TestScanner scanner("");
	EXPECT_EQ(None, scanner.skip_comment());
}

TEST(ScannerTest, SkipCommentNoCommentPopulated)
{
	// If we give it 2 characters that aren't a comment, it should return None,
	// but test it with the first character being a slash first.
	TestScanner scanner("/XZ");	// exceeds the limit of 2 chars.
	EXPECT_EQ(None, scanner.skip_comment());
	EXPECT_EQ("/XZ", scanner.current());  // confirm it didn't advance.
	scanner.seek(1);
	EXPECT_EQ(None, scanner.skip_comment());
	EXPECT_EQ('X', scanner.front());  // confirm it didn't advance.
	scanner.seek(1);
	// And now we should fail the < 2 test.
	EXPECT_EQ(None, scanner.skip_comment());
	EXPECT_EQ('Z', scanner.front());  // confirm it didn't advance.
}

TEST(ScannerTest, SkipCommentLineComment)
{
	// Take a line comment and throw some extra comment-like stuff to ensure we don't
	// have any kind of conflict.
	TestScanner scanner("// // /*\nx");
	TResult result = scanner.skip_comment();
	EXPECT_TRUE(result.is_token());
	EXPECT_EQ(Token::Type::LineComment, result.token().type_);
	EXPECT_EQ("// // /*", result.token().source_);
	EXPECT_EQ('\n', scanner.front());
	EXPECT_EQ(None, scanner.skip_comment());
}


TEST(ScannerTest, SkipCommentBlockComment)
{
	// Simple, empty comment.
	// A couple of IDEs dislike me writing the string directly here
	TestScanner scanner("/**/X/**-**/Y");
	{
		TResult result = scanner.skip_comment();

		EXPECT_TRUE(result.is_token());
		EXPECT_EQ(Token::Type::CloseComment, result.token().type_);
		EXPECT_EQ("/**/", result.token().source_);
		EXPECT_EQ('X', scanner.front());
	}

	scanner.seek(1);

	{
		TResult result = scanner.skip_comment();
		EXPECT_TRUE(result.is_token());
		EXPECT_EQ(Token::Type::CloseComment, result.token().type_);
		EXPECT_EQ("/**-**/", result.token().source_);
	}

	EXPECT_EQ('Y', scanner.front());
}


TEST(ScannerTest, SkipCommentUnterminatedBlock)
{
	TestScanner scanner("/**XYZ/\n*\n/");
	TResult result = scanner.skip_comment();
	EXPECT_TRUE(result.is_error());
	EXPECT_TRUE(result.has_token());
	EXPECT_EQ(Token::Type::OpenComment, result.token().type_);
	EXPECT_EQ("unterminated block comment", result.error());
	EXPECT_EQ("/**XYZ/\n*\n/", result.token().source_);
	EXPECT_EQ(0, scanner.front());
}


TEST(ScannerTest, ScanString)
{
	TestScanner scanner("\"hello world\"");
	TResult result = scanner.scan_string();
	EXPECT_TRUE(result.is_token());
	EXPECT_EQ(Token::Type::String, result.token().type_);
	EXPECT_EQ("\"hello world\"", result.token().source_);
	EXPECT_EQ('\0', scanner.front());
}


TEST(ScannerTest, ScanStringUnterminated)
{
	TestScanner scanner("\"hello");
	TResult result = scanner.scan_string();
	EXPECT_TRUE(result.is_error());
	EXPECT_TRUE(result.has_token());
	EXPECT_EQ(Token::Type::String, result.token().type_);
	EXPECT_EQ("unterminated string", result.error());
	EXPECT_EQ("\"hello", result.token().source_);
	EXPECT_EQ(0, scanner.front());
}


TEST(ScannerTest, ScanStringUnterminatedEOL)
{
	// Test that it detects a newline-before-quote.
	TestScanner scanner("\"hello\n\"world\r\"");
	TResult result = scanner.scan_string();
	EXPECT_TRUE(result.is_error());
	EXPECT_TRUE(result.has_token());
	EXPECT_EQ(Token::Type::String, result.token().type_);
	EXPECT_EQ("unterminated string", result.error());
	EXPECT_EQ("\"hello", result.token().source_);
	EXPECT_EQ('\n', scanner.front());

	// Test with \r
	scanner.seek(1);
	ASSERT_EQ('"', scanner.front());

	TResult result2 = scanner.scan_string();
	EXPECT_TRUE(result2.is_error());
	EXPECT_TRUE(result2.has_token());
	EXPECT_EQ(Token::Type::String, result2.token().type_);
	EXPECT_EQ("unterminated string", result2.error());
	EXPECT_EQ("\"world", result2.token().source_);
	EXPECT_EQ('\r', scanner.front());
}


TEST(ScannerTest, ScanWord)
{
	TestScanner scanner("h _: 1 h1_");

	// Try a single letter (h) word.
	ASSERT_EQ('h', scanner.front());
	{
		TResult result = scanner.scan_word();
		EXPECT_TRUE(result.is_token());
		EXPECT_EQ(Token::Type::Word, result.token().type_);
		EXPECT_EQ("h", result.token().source_);
		EXPECT_EQ(' ', scanner.front());
	}

	// Now try a single underscore.
	EXPECT_TRUE(scanner.skip_whitespace());
	ASSERT_EQ('_', scanner.front());
	{
		TResult result = scanner.scan_word();
		EXPECT_TRUE(result.is_token());
		EXPECT_EQ(Token::Type::Word, result.token().type_);
		EXPECT_EQ("_", result.token().source_);
		EXPECT_EQ(':', scanner.front());
		scanner.seek(1);
	}

	// Try a single digit - this should never happen, because
	// they'll go to scan_number, but it validates expected
	// behavior if we did pass a number to this method.
	EXPECT_TRUE(scanner.skip_whitespace());
	ASSERT_EQ('1', scanner.front());
	{
		TResult result = scanner.scan_word();
		EXPECT_TRUE(result.is_token());
		EXPECT_EQ(Token::Type::Word, result.token().type_);
		EXPECT_EQ("1", result.token().source_);
		EXPECT_EQ(' ', scanner.front());
	}

	// Now try a combo.
	EXPECT_TRUE(scanner.skip_whitespace());
	ASSERT_EQ('h', scanner.front());
	{
		TResult result = scanner.scan_word();
		EXPECT_TRUE(result.is_token());
		EXPECT_EQ(Token::Type::Word, result.token().type_);
		EXPECT_EQ("h1_", result.token().source_);
		EXPECT_EQ('\0', scanner.front());
	}
}
