%{
#include "parser.generated.hh"	// bison-generated header

using namespace std;

#define yyterminate() return YYEOF

#define YY_USER_ACTION		yylloc->step(); yylloc->columns(yyleng); mLastLoc = *yylloc;

#define YY_VERBOSE
%}


%%


/* Settings to target the C++ implementation of Flex/Bison */
%option c++
%option verbose
%option warn

/* Don't generate a default rule on our behalf, we'll provide one */
%option nodefault

/* Please don't go including unistd and making is non-portable */
%option nounistd

/* Performance tuning */
%option ecs
%option align
%option read

/* Make context stack available */
%option stack

/* Don't try to push tokens back into the input stream or manage any buffers for us */
%option noinput nounput noyymore noyywrap

/* Target a parser rather than an interpreter/repl */
%option never-interactive batch

/* Write a source file but not a header */
%option outfile="scanner.generated.cc"

/* I found that flex/bisons built-in location tracking wasn't great, so ... */

%x BLOCK_COMMENT STRING

%{
// any code that goes in the body of the generated source here.
%}


// Named pattern fragments
digit								([0-9])
sign								([+-])
decimalval							(\.{digit})
float								({sign}?({digit}+\.{digit}*|\.{digit}+))
integer								({sign}?({digit}+))

%%

%{
	// executed every loop of the lexer.
%}

// Nestable block coments
<INITIAL,BLOCK_COMMENT>"/*"		{ yy_push_state(BLOCK_COMMENT); }
<BLOCK_COMMENT><<EOF>>			{ report_error(*yylloc, "unterminated block comment, did you forget the '*/'?"); return YYEOF; }
<BLOCK_COMMENT>"*/"				{ yy_pop_state(); }

// Line comments
<INITIAL>"//"[^\n]*				/* ignore line comment */

<INITIAL>"*/"					{ report_error(*yylloc, "unexpected/unmatched close-comment ('*/')"); }

// Keywords
"enum"							return KW_ENUM;
"type"							return KW_TYPE;
"false"							return KW_FALSE;
"true"							return KW_TRUE;

/* Symbols */
"::"							return SCOPE;
":"								return COLON;
"{"								return LBRACE;
"}"								return RBRACE;
"["								return LBRACKET;
"]"								return RBRACKET;
"="								return EQUALS;
","								return COMMA;

// Abstract terminals
{float}							return FLOAT;
{integer}						return INTEGER;
[A-Za-z_][A-Za-z0-9_]*			return IDENTIFIER;

// String literal
<INITIAL>"\""					{ yy_push_state(STRING); yylval->emplace<string>("\""); }
<STRING>"\\[\\\"0-9A-Za-z_]"	{  }
<STRING>"\\."					{ report_error(*yylloc, "invalid escape in string literal"); return YYEOF; }
<STRING>[^\\\"]+				{ /* todo */ }
<STRING>"\""					{ yy_pop_state(); return STRING; }
<STRING><<EOF>>					{ report_error(*yylloc, "unterminated string literal"); return YYEOF; }

<*><<EOF>>						return YYEOF;

// Whitespace
[ \t\r]+						/*ignore*/
<*>\n							yylloc->lines();

<BLOCK_COMMENT>					/* ignore */
.								{ report_error(*yylloc, "unexpected character: " + std::string(yytext)); }

%%

void report_error(const YYLTYPE& loc, const string& message) {
	std::cerr << "error: " << message << "\n";
}