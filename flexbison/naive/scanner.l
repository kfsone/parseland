/* Declare parser states. */
%x COMMENT

/* Named pattern fragments */
sign							[+-]
alpha							[A-Za-z]
letter							[A-Za-z_]
digit							[0-9]
escape							[\\]
quote							[\"]
eol								([\r\n]|<<EOF>>)		/* eof ends a line too. */

not_escape_quote_or_newline		[^"\\\r\n]
quote_body						({escape}({escape}|{quote}|{alpha_digit})|{not_escape_quote_or_newline})*

integer							{sign}?{digit}+
float							{integer}'.'{digit}*|{sign}?{digit}+

identifier						{letter}(letter|digit)*)

%%

								/* If we see an open-comment, use the state stack to track depth */
<INITIAL,COMMENT>"/*"			{ yy_push_state(BLOCK_COMMENT); }
<COMMENT><<EOF>>				{ yyerrror(yyloc, _("unterminated block-comment, missing '*/'")); return YYerror; }
<COMMENT>"*/"					{ yy_pop_state(); }
<COMMENT>.						/* ignored */

"//"[^\n]*						/* ignore: line comment */

{quote}{quote_body}{quote}		{ yylval->string = strdup(yytext); return STRING; }
								/*{quote}{quote_body}{eol}		{ yyerror(yyloc, _("unterminated string literal, missing '"')); return STRING; }*/

								/* Declare keywords before we declare identifier. */
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


{float}							{ yylval->string = strdup(yytext); return FLOAT; }
{integer}						{ yylval->string = strdup(yytext); return INTEGER; }
{identifier}					{ yylval->string = strdup(yytext); return NAME; }

// Whitespace
[ \t\r\n]+						/* ignore, does this mess line counting up? */

.								{ yyerror(yyloc, _("unexpected character")); return YYerror; }
