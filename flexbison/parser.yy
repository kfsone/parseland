/* Which rule the grammar starts with */
%start report_error

// Use the C++ model.
%language "C++"
%skeleton "lalr1.cc"
%require "3.8.2"  // bison version

// Track line/column for us
%locations

// Write a header and source file for us
%defines "parser.generated.hh"
%output  "parser.generated.cc"

// Write the location type to "location.generated.hh" instead of "location.hh"
%define	 api.location.file "location.generated.hh"

// Put all our definitions in the 'kfs::parseland' namespace.
%define   api.namespace      {kfs::parseland}

// Imaginativity at its peak
%define   api.parser.class   {Parser}

%define   api.token.raw

// Use Bison's"variant' type for tracking symbol types for us.
%define   api.value.type     variant
%define   api.value.automove

// Tell the parser to assert if it gets into a bad state.
%define   parse.assert
%define   parse.trace

// Set the defail level of errors; by default it would just say "unexpected",
// setting it to detailed tells it to say
//  unexpected IDENTIFIER, expected NUMBER or STRING_LITERAL
%define   parse.error        detailed

// Lookahead correction will volunteer alternatives to unexpected inputs in the case
// of e.g typos etc.
%define	  parse.lac          full


// Code to be inserted at the start of the generated header file.
%code requires
{
}



// parameters passed to the parser method.
%parse-param {yyscan_t scanner}
%parse-param {}

%code
{
}


// Declaration of tokens and for those with types, what value type they have.
%token					YYEOF			0			"end of file"
%token					KW_ENUM						"'enum' keyword"
%token					KW_TYPE						"'type' keyword"
%token					KW_TRUE						"'true'"
%token					KW_FALSE					"'false'"

%token					SCOPE						"scope operator ('::')"
%token					COLON						"colon (':')"
%token					LBRACE						"left brace ('{')"
%token					RBRACE						"right brace ('}')"
%token					LBRACKET					"left bracket ('[')"
%token					RBRACKET					"right bracket (']')"
%token 					EQUALS 						"equals ('=')"
%token					COMMA						"comma (',')"

%token  <string>		FLOAT						"floating-point number"
%token	<string>		INTEGER						"integer"
%token	<string>		IDENTIFIER					"identifier"
%token	<string>		STRING						"string literal"


// Grammar proper

root
	:	definition*
	;

definition
	:	enum_definition
	|	type_definition
	;

%{
struct ParsedText
{
	using self_type = ParsedText;
	using loc_type  = Location;
	using data_type = std::string;

	// Where the data was at in the source document
	loc_type	mLocation {};
	// The value of the data itself.
	data_type	mData     {};

	// Default ctor and dtor
	ParsedText() = default;
	~ParsedText() = default;

	// Assignment ctors
	ParsedText(const loc_type& loc, const data_type& data): mLocation(loc), mData(data) {}
	ParsedText(const loc_type& loc, data_type&& data): mLocation(loc), mData(std::move(data)) {}

	// Move and copy ctors and operators.
	ParsedText(const self_type& rhs) : mLocation(rhs.mLocation), mData(rhs.mData) {}
	ParsedText(self_type&& rhs) noexcept : mLocation(std::move(rhs.mLocation)), mData(std::move(rhs.mData)) {}
	self_type& operator=(const self_type& rhs) { mLocation = rhs.mLocation; mData = rhs.mData; return *this; }
	self_type& operator=(self_type&& rhs) noexcept { mLocation = std::move(rhs.mLocation); mData = std::move(rhs.mData); return *this; }

	// Accessors
	const loc_type& loc_type() const { return mLocation; }
	const data_type& data() const { return mData; }
};

using EnumValue = ParsedText;	// todo: use a string_view?
using EnumValues = std::vector<EnumValue>;
%}

enum_definition
	:	KW_ENUM IDENTIFIER LBRACE enum_values RBRACE
		{ driver.add_enum($2, $4); }
	;

%nterm <EnumValues> enum_values;
enum_values
	:	%empty { $$ = EnumValues(); }
	|	enum_values enum_value
		{ $$ = $1; $$.emplace_back(std::move($2)); }
	|	enum_value
		{ $$ = EnumValues(); $$.emplace_back(std::move($1)); }
	;

%nterm <EnumValue> enum_value;
enum_value
	:	IDENTIFIER COMMA
		{ $$ = $1; }
	|	IDENTIFIER
		{ $$ = $1; }
	;


type_definition
	:	KW_TYPE IDENTIFIER type_parent LBRACE member_definitions RBRACE
		{ driver.add_type($2, $3, $5); }
	;

%nterm <std::optional<std::string>> type_parent
	:
type_parent
	:	%empty
		{ $$ = std::nullopt; }
	|	COLON IDENTIFIER
		{ $$ = $2; }
	;

member_definitions
	:	%empty		/* no members, legal */
		{ $$ = std::vector<TypeMember>(); }
	|	member_definitions member_definition
		{ $$ = $1; $$.push_back($2); }
	;


member_definition
	:	IDENTIFIER EQUALS value
	;

value
	:	KW_TRUE { $$ = ParsedText("true"); }
	|	float_value { $$ = ParsedText("false"); }
	|	integer_value
	|	string_value
	|	compound_value
	;


bool_value
	:	KW_TRUE  { $$ = ParsedText("true");  /* populate yylval in scanner */ }
	|	KW_FALSE { $$ = ParsedText("false"); /* populate yylval in scanner */ }
	;

/// UNDER CONSTRUCTION VVVV