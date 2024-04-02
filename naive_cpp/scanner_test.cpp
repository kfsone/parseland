// Unit tests for the naive CPP scanner.

#include "scanner.h"

#include <gtest/gtest.h>

using namespace kfs;

static constexpr TResult None{};


// Because I made things private/protected, we'll need to surface things here.
struct TestScanner : public Scanner
{
	using Scanner::Scanner;

	[[nodiscard]] const string_view& source() const noexcept { return source_; }
	[[nodiscard]] const string_view& current() const noexcept { return current_; }
	[[nodiscard]] size_t comments() const noexcept { return comments_; }
	[[nodiscard]] size_t comments_len() const noexcept { return comments_len_; }

	void seek(size_t i)
	{
		current_.remove_prefix(i);
	}

public:
	using Scanner::make_token;
	using Scanner::unexpected_result;

	using Scanner::front;
	using Scanner::peek;

	using Scanner::skip_whitespace;
	using Scanner::skip_comment;

	using Scanner::scan_string;
	using Scanner::scan_number;
	using Scanner::scan_signed_number;
	using Scanner::scan_word;
};


// Sanity check the constructor does what it has to do.
TEST(ScannerTest, Construction)
{
	TestScanner scanner{"hello world"};

	ASSERT_EQ("hello world", scanner.source());
	ASSERT_EQ("hello world", scanner.current());
	ASSERT_EQ(0, scanner.comments());
	ASSERT_EQ(0, scanner.comments_len());

	// Make sure our 'seek' helper works, too.
	scanner.seek(5);
	EXPECT_EQ(" world", scanner.current());
	ASSERT_EQ("hello world", scanner.source());

}


// Since we use make_token to construct our TResults, we want to ensure
// it works as advertised.
TEST(ScannerTest, MakeToken)
{
	// Let's check that 'make token' generates sensible looking tokens,
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
	EXPECT_EQ(0, scanner.comments());

	EXPECT_EQ(false, scanner.skip_whitespace());
	EXPECT_EQ('X', scanner.front());
	EXPECT_EQ(0, scanner.comments());

	// test the empty string
	scanner.seek(1);
	EXPECT_EQ(false, scanner.skip_whitespace());
	EXPECT_EQ(0, scanner.front());
	EXPECT_EQ(0, scanner.comments());
}


TEST(ScannerTest, SkipCommentNoCommentEmpty)
{
	TestScanner scanner("");
	EXPECT_EQ(None, scanner.skip_comment());
	EXPECT_EQ(0, scanner.comments());
}


TEST(ScannerTest, SkipCommentOneSlash)
{
	TestScanner scanner("/");
	EXPECT_EQ(None, scanner.skip_comment());
	EXPECT_EQ(0, scanner.comments());
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


TEST(ScannerTEst, SkipCommentLineCommentEOI)
{
	// Line comment with end of input immediately after the token.
	TestScanner scanner("//");
	EXPECT_EQ(Token::Type::LineComment, scanner.skip_comment().token().type_);
	EXPECT_EQ("", scanner.current());
}


TEST(ScannerTEst, SkipCommentLineCommentNoNewline)
{
	TestScanner scanner("//aaa");
	EXPECT_EQ(Token::Type::LineComment, scanner.skip_comment().token().type_);
	EXPECT_EQ("", scanner.current());
}


TEST(ScannerTest, SkipCommentLineComment)
{
	// Take a line comment and throw some extra comment-like stuff to ensure we don't
	// have any kind of conflict.
	TestScanner scanner("/////*\nx");
	const TResult result = scanner.skip_comment();
	EXPECT_TRUE(result.is_token());
	EXPECT_EQ(Token::Type::LineComment, result.token().type_);
	EXPECT_EQ("/////*", result.token().source_);
	EXPECT_EQ('\n', scanner.front());
	EXPECT_EQ(None, scanner.skip_comment());
}


TEST(ScannerTest, SkipCommentBlockComment)
{
	// Simple, empty comment.
	// A couple of IDEs dislike me writing the string directly here
	TestScanner scanner("/**/X/**-**/Y");
	{
		const TResult result = scanner.skip_comment();

		EXPECT_TRUE(result.is_token());
		EXPECT_EQ(Token::Type::CloseComment, result.token().type_);
		EXPECT_EQ("/**/", result.token().source_);
		EXPECT_EQ('X', scanner.front());
	}

	scanner.seek(1);

	{
		const TResult result = scanner.skip_comment();
		EXPECT_TRUE(result.is_token());
		EXPECT_EQ(Token::Type::CloseComment, result.token().type_);
		EXPECT_EQ("/**-**/", result.token().source_);
	}

	EXPECT_EQ('Y', scanner.front());
}


TEST(ScannerTest, SkipCommentUnterminatedBlock)
{
	TestScanner scanner("/**XYZ/\n*\n/");
	const TResult result = scanner.skip_comment();
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
	const TResult result = scanner.scan_string();
	EXPECT_TRUE(result.is_token());
	EXPECT_EQ(Token::Type::String, result.token().type_);
	EXPECT_EQ("\"hello world\"", result.token().source_);
	EXPECT_EQ('\0', scanner.front());
}


TEST(ScannerTest, ScanStringUnterminated)
{
	TestScanner scanner("\"hello");
	const TResult result = scanner.scan_string();
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
	const TResult result = scanner.scan_string();
	EXPECT_TRUE(result.is_error());
	EXPECT_TRUE(result.has_token());
	EXPECT_EQ(Token::Type::String, result.token().type_);
	EXPECT_EQ("unterminated string", result.error());
	EXPECT_EQ("\"hello", result.token().source_);
	EXPECT_EQ('\n', scanner.front());

	// Test with \r
	scanner.seek(1);
	ASSERT_EQ('"', scanner.front());

	const TResult result2 = scanner.scan_string();
	EXPECT_TRUE(result2.is_error());
	EXPECT_TRUE(result2.has_token());
	EXPECT_EQ(Token::Type::String, result2.token().type_);
	EXPECT_EQ("unterminated string", result2.error());
	EXPECT_EQ("\"world", result2.token().source_);
	EXPECT_EQ('\r', scanner.front());
}


TEST(ScannerTest, ScanNumber)
{
	// Scan number takes it as read that the byte at the front of current is numeric,
	// then scans until it reaches a non-numeric character.
	// If that character is '.', and it hasn't seen a dot yet, it will continue.
	// Otherwise, it ends on the first non-digit or reaching EOI.
	struct PassCase {
		std::string_view input;
		std::string_view capture;
		std::string_view remainder;
		Token::Type      type;
	} cases[] = {
		PassCase { "0",   	"0",	"",  Token::Type::Integer },
		PassCase { "0a",    "0",    "a",   Token::Type::Integer },
		PassCase { "1.",  	"1.",	"",  Token::Type::Float },
		PassCase { "123a",	"123",	"a",   Token::Type::Integer },
		PassCase { "12.a",	"12.",	"a",   Token::Type::Float },
		PassCase { "1..", 	"1.",	".",   Token::Type::Float },
		PassCase { "1.2.",  "1.2",  ".",   Token::Type::Float },
	};
	for (const PassCase& c: cases)
	{
		SCOPED_TRACE(c.input);

		TestScanner scanner(c.input);
		const TResult result = scanner.scan_number();
		ASSERT_TRUE(result.is_token());
		EXPECT_EQ(c.type, result.token().type_);
		EXPECT_EQ(c.capture, result.token().source_);
		EXPECT_EQ(c.remainder, scanner.current());
	}
}


TEST(ScannerTest, ScanSignedNumberNominal)
{
	// Several basic sanity tests
	struct PassCase {
		std::string_view input;
		std::string_view capture;
		std::string_view remainder;
		Token::Type      type;
	} cases[] = {
		// First check if for size > 1 and 0 <= peek <= 9,
		// with heavy lifting done inside san_number.
		PassCase { "+0",			"+0",				"",		Token::Type::Integer },
		PassCase { "-01",          	"-01",				"",		Token::Type::Integer },

		// Next check is for size > 2 and peek == '.', with
		// the consumption of the '.' being handled here.
		PassCase { "+.0", 			"+.0",				"",		Token::Type::Float },
		PassCase { "-.12a",			"-.12",             "a",	Token::Type::Float },
		PassCase { "+.0123.",       "+.0123",			".",	Token::Type::Float },
		PassCase { "+.999+",        "+.999",            "+",	Token::Type::Float },
	};
	for (const auto& c : cases)
	{
		SCOPED_TRACE(c.input);

		TestScanner scanner(c.input);
		const TResult result = scanner.scan_signed_number();
		ASSERT_TRUE(result.is_token());
		EXPECT_EQ(c.type, result.token().type_);
		EXPECT_EQ(c.capture, result.token().source_);
		EXPECT_EQ(c.remainder, scanner.current());
	}
}

TEST(ScannerTest, ScanSignedNumberFail)
{
	std::string_view cases[] = {
		"+", "++", "+a", "+-", "+.", "+.a", "+.+", "+.-",
		"-", "--", "-a", "-+", "-.", "-.a", "-.-", "-.+",
	};
	for (const auto& c : cases)
	{
		SCOPED_TRACE(c);

		TestScanner scanner(c);
		const TResult result = scanner.scan_signed_number();
		ASSERT_TRUE(result.is_error());
		ASSERT_TRUE(result.has_token());
		EXPECT_EQ(Token::Type::Invalid, result.token().type_);
		EXPECT_EQ("unexpected character", result.error());
		EXPECT_EQ(c.substr(0, 1), result.token().source_);
	}
}


TEST(ScannerTest, ScanWord)
{
	struct PassCase {
		std::string_view input;
		std::string_view capture;
		std::string_view remainder;
	} cases[] = {
		PassCase { "h", "h", "" },
		PassCase { "hello", "hello", "" },
		PassCase { "hello world", "hello", " world" },
		PassCase { "h1x_123.a", "h1x_123", ".a" },
		PassCase { "a\"", "a", "\"" },
		PassCase { "123", "123", "" },
	};
	for (const auto& c: cases)
	{
		SCOPED_TRACE(c.input);

		TestScanner scanner(c.input);
		const TResult result = scanner.scan_word();
		ASSERT_TRUE(result.is_token());
		EXPECT_EQ(Token::Type::Word, result.token().type_);
		EXPECT_EQ(c.capture, result.token().source_);
		EXPECT_EQ(c.remainder, scanner.current());
	}
}


// Verify get_token_offset works as expected.
TEST(ScannerTest, GetTokenOffsetNullOpt)
{
	// Create a scanner from a sub-string so we can do a controlled check
	// of outside-of-bounds.
	std::string_view source = "|little picture|";
	TestScanner scanner(source.substr(1, source.length() - 2));
	ASSERT_EQ('l', scanner.front());

	// Try a token whose 'begin' is below the 'begin' of the source substr
	EXPECT_EQ(std::nullopt, scanner.get_token_offset(Token{Token::Type::Word, source}));
	// Try a token whose begin is above the end of the source substr
	EXPECT_EQ(std::nullopt, scanner.get_token_offset(Token{Token::Type::Integer, source.substr(source.length(), 0)}));
	// Try a token whose begin is within the source substr but whose end is beyond it.
	EXPECT_EQ(std::nullopt, scanner.get_token_offset({Token{Token::Type::Word, source.substr(1, source.length() - 1)}}));
	// Try some arbitrary other token unrelated to source.
	EXPECT_EQ(std::nullopt, scanner.get_token_offset(Token{Token::Type::Float, "3.14"}));
	// Finally, with an empty string?
	EXPECT_EQ(std::nullopt, scanner.get_token_offset(Token{Token::Type::Invalid, ""}));
}


// Try some actual token getting.
TEST(ScannerTest, GetTokenOffset)
{
	std::string_view source = "xyz";
	TestScanner scanner(source);

	for (size_t i = 0; i <= source.length(); ++i)
	{
		SCOPED_TRACE(i);
		auto result = scanner.get_token_offset(Token{Token::Type::Word, source.substr(i, 1)});
		ASSERT_TRUE(result.has_value());
		EXPECT_EQ(i, result.value());
	}
}


// And now ... the grand finale ... lets test "next".

TEST(ScannerTest, NextEmpty)
{
	// Calling the scanner on an empty string should indicate EOI via None,
	// and this is not about testing EOI as much as testing that we are
	// graceful in handling an empty input.
	const TResult result = TestScanner("").next();
	ASSERT_EQ(None, result);
}

TEST(ScannerTest, NextWhitespace)
{
	struct PassCase {
		string_view name;
		string_view input;
	} cases[] = {
		PassCase{"space", " "}, PassCase{"tab", "\t"}, PassCase{"cr", "\r"}, PassCase{"lf", "\n"},
		PassCase{"crlf", "\r\n"}, PassCase{"lfcr", "\n\r"},
		PassCase{"space-mix", "      \t  \r  \n"},
		PassCase{"tab-mix",   "\t  \t\t\t\r\t\n"},
		PassCase{"cr-mix",    "\r  \r\t\r\r\r\n"},
		PassCase{"lf-mix",    "\n  \n\t\n\r\n\n"},
	};
	for (const auto& c: cases)
	{
		SCOPED_TRACE(c.name);

		TestScanner scanner(c.input);
		EXPECT_EQ(None, scanner.next());
		ASSERT_EQ(0, scanner.front());
	}
}

TEST(ScannerTest, NextComments)
{
	struct PassCase {
		string_view name;
		string_view input;
		size_t      comments;
		size_t      comments_len;
	} cases[] = {
		PassCase{"empty line comment", "//", 1, 2},
		PassCase{"multiple line comments", "//\n//\n//*/\n", 3, 8},
		PassCase{"eoi empty block comment", "/**/", 1, 4},
		PassCase{"eoi // block comment", "/*//*/", 1, 6},  // this is only valid in this naive version that doesn't do nesting.
		PassCase{"block-hello plus line world", "/* hello */// world\n\n", 2, 19},
		PassCase{"multi comment", "///**/\n/**///\n/*\n\r\n\t\n*/", 4, 21},
	};
	for (const auto& c: cases)
	{
		SCOPED_TRACE(c.name);

		TestScanner scanner(c.input);
		EXPECT_EQ(None, scanner.next());
		EXPECT_EQ(c.comments, scanner.comments());
		EXPECT_EQ(c.comments_len, scanner.comments_len());
	}
}

TEST(ScannerTest, NextUnexpectedSingleChar)
{
	TestScanner scanner("~");
	TResult result = scanner.next();
	EXPECT_TRUE(result.is_error());
	EXPECT_TRUE(result.has_token());
	EXPECT_EQ(Token::Type::Invalid, result.token().type_);
	EXPECT_EQ("~", result.token().source_);  // confirm it didn't advance.
	EXPECT_EQ("unexpected character", result.error());
	EXPECT_EQ(0, scanner.front());
}

TEST(ScannerTest, NextUnexpectedCharPair)
{
	TestScanner scanner("@!");
	const TResult result = scanner.next();
	EXPECT_TRUE(result.is_error());
	EXPECT_TRUE(result.has_token());
	EXPECT_EQ(Token::Type::Invalid, result.token().type_);
	EXPECT_EQ("@", result.token().source_);  // confirm it didn't advance.
	EXPECT_EQ("unexpected character", result.error());
	EXPECT_EQ('!', scanner.front());
}

TEST(ScannerTest, NextSkipViaUnexpected)
{
	// Test that whitespace/comment skipping works and takes us to a non-whitespace/comment char.
	TestScanner scanner(" \t\r\n// \n/* :K~\"*/`");
	const TResult result = scanner.next();
	EXPECT_TRUE(result.is_error());
	EXPECT_TRUE(result.has_token());
	EXPECT_EQ(Token::Type::Invalid, result.token().type_);
	EXPECT_EQ("`", result.token().source_);
	EXPECT_EQ(2, scanner.comments());
}

TEST(ScannerTest, Next)
{
	struct PassCase {
		string_view name;
		string_view input;
		Token::Type type;
		string_view capture;
		string_view remainder;
		size_t      comments;
	} cases[] = {
		// strings
		PassCase{"empty string->eoi", "\"\"", Token::Type::String, "\"\"", "", 0},
		PassCase{"noise->empty string->noise", " \t/**/\"\"//", Token::Type::String, "\"\"", "//", 1},
		PassCase{"hello string->eoi", "\"hello world\"", Token::Type::String, "\"hello world\"", "", 0},
		PassCase{"noise->hello string->symbol", "\r\n\"hello world\"=", Token::Type::String, "\"hello world\"", "=", 0},

		// Symbols
		PassCase{"{", "{", Token::Type::LBrace, "{", "", 0},
		PassCase{"}", "}", Token::Type::RBrace, "}", "", 0},
		PassCase{"[,", "[,", Token::Type::LBracket, "[", ",", 0},	// comma just to be different
		PassCase{"]", "]", Token::Type::RBracket, "]", "", 0},
		PassCase{"::", "::", Token::Type::Scope, "::", "", 0},
		PassCase{":::", ":::", Token::Type::Scope, "::", ":", 0},
		PassCase{":", ":", Token::Type::Colon, ":", "", 0},
		PassCase{"=", "=", Token::Type::Equals, "=", "", 0},
		PassCase{",", ",", Token::Type::Comma, ",", "", 0},
		PassCase{"+1,", "+1", Token::Type::Integer, "+1", "", 0},
		PassCase{"-0", "-0", Token::Type::Integer, "-0", "", 0},
		PassCase{"0a", "0a", Token::Type::Integer, "0", "a", 0},
		PassCase{"1.2", "1.2", Token::Type::Float, "1.2", "", 0},
	};

	for (const auto& c: cases)
	{
		SCOPED_TRACE(c.name);
		TestScanner scanner(c.input);
		const TResult result = scanner.next();
		ASSERT_TRUE(result.is_token());
		EXPECT_EQ(c.type, result.token().type_);
		EXPECT_EQ(c.capture, result.token().source_);
		EXPECT_EQ(c.remainder, scanner.current());
		EXPECT_EQ(c.comments, scanner.comments());
	}
}

void testWords(char prefix, std::string suffix)
{
	suffix.insert(suffix.begin(), prefix);
	{
		SCOPED_TRACE(suffix + "->eoi");
		TestScanner scanner(suffix);
		const TResult result = scanner.next();
		ASSERT_TRUE(result.is_token());
		EXPECT_EQ(Token::Type::Word, result.token().type_);
		EXPECT_EQ(suffix, result.token().source_);
		EXPECT_EQ("", scanner.current());
	}

	{
		SCOPED_TRACE(suffix + "->!");
		const std::string source = suffix + "//";
		TestScanner scanner(source);
		const TResult result = scanner.next();
		ASSERT_TRUE(result.is_token());
		EXPECT_EQ(Token::Type::Word, result.token().type_);
		EXPECT_EQ(suffix, result.token().source_);
		EXPECT_EQ("//", scanner.current());
	}
}

TEST(ScannerTest, NextWords)
{
	for (const std::string suffix: { "", "_", "aZ", "a1_23" })
	{
		for (char c = 'a'; c <= 'z'; ++c)
		{
			testWords(c, suffix);
			testWords(static_cast<char>(std::toupper(c)), suffix);
		}

		testWords('_', suffix);
	}
}


TEST(ScannerTest, NextFailures)
{
	// A few cases we expect to get errors from.
	ASSERT_TRUE(TestScanner(" /*").next().is_error());
	ASSERT_TRUE(TestScanner(" \"").next().is_error());
	ASSERT_TRUE(TestScanner("+").next().is_error());
	ASSERT_TRUE(TestScanner("-").next().is_error());

	// List of characters that are acceptable, so we can test
	// all the others.
	static const char* allowed =
		" \t\r\n"
		"0123456789"
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"_"
		"{}[]:=,"
		"/\""
	;
	char source[3] = {0, 0, 0};
	for (int c = 1; c < 255; c++)
	{
		source[0] = static_cast<char>(c & 0xff);
		if (strchr(allowed, source[0]) != nullptr)
			continue;

		auto label = std::to_string(c);
		if (isprint(c))
		{
			char desc[10];
			snprintf(desc, 10, " (%c)", source[0]);
			label += desc;
		}
		SCOPED_TRACE(label);

		TestScanner scanner(source);
		const TResult result = scanner.next();
		EXPECT_TRUE(result.is_error());
		EXPECT_TRUE(result.has_token());
		EXPECT_EQ("unexpected character", result.error());
		EXPECT_EQ(source, result.token().source_);
	}
}
