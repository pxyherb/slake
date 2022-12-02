%option noinput nounput noyywrap nounistd never-interactive

%top {
#include <spkparse.hh>

SpkC::Syntax::parser::symbol_type yylval;

#define YY_USER_ACTION yylloc.columns(yyleng);
#define YY_DECL SpkC::Syntax::parser::symbol_type yylex()

using namespace SpkC::Syntax;
}

%x COMMENT LINE_COMMENT STRING ESCAPE

%%

%{
yylloc.step();
%}

","			return parser::make_T_COMMA(yylloc);
"?"			return parser::make_OP_TERNARY(yylloc);
":"			return parser::make_T_COLON(yylloc);
";"			return parser::make_T_SEMICOLON(yylloc);
"["			return parser::make_T_LBRACKET(yylloc);
"]"			return parser::make_T_RBRACKET(yylloc);
"{"			return parser::make_T_LBRACE(yylloc);
"}"			return parser::make_T_RBRACE(yylloc);
"("			return parser::make_T_LPARENTHESE(yylloc);
")"			return parser::make_T_RPARENTHESE(yylloc);
"@"			return parser::make_T_AT(yylloc);
"."			return parser::make_T_DOT(yylloc);
"..."		return parser::make_T_VARARG(yylloc);

"+"			return parser::make_OP_ADD(yylloc);
"-"			return parser::make_OP_SUB(yylloc);
"*"			return parser::make_OP_MUL(yylloc);
"/"			return parser::make_OP_DIV(yylloc);
"%"			return parser::make_OP_MOD(yylloc);
"&"			return parser::make_OP_AND(yylloc);
"|"			return parser::make_OP_OR(yylloc);
"^"			return parser::make_OP_XOR(yylloc);
"!"			return parser::make_OP_NOT(yylloc);
"~"			return parser::make_OP_REV(yylloc);
"="			return parser::make_OP_ASSIGN(yylloc);
"+="		return parser::make_OP_ASSIGN_ADD(yylloc);
"-="		return parser::make_OP_ASSIGN_SUB(yylloc);
"*="		return parser::make_OP_ASSIGN_MUL(yylloc);
"/="		return parser::make_OP_ASSIGN_DIV(yylloc);
"%="		return parser::make_OP_ASSIGN_MOD(yylloc);
"&="		return parser::make_OP_ASSIGN_AND(yylloc);
"|="		return parser::make_OP_ASSIGN_OR(yylloc);
"^="		return parser::make_OP_ASSIGN_XOR(yylloc);
"~="		return parser::make_OP_ASSIGN_REV(yylloc);
"=="		return parser::make_OP_EQ(yylloc);
"!="		return parser::make_OP_NEQ(yylloc);
"<="		return parser::make_OP_LTEQ(yylloc);
">="		return parser::make_OP_GTEQ(yylloc);
"&&"		return parser::make_OP_LAND(yylloc);
"||"		return parser::make_OP_LOR(yylloc);
"++"		return parser::make_OP_INC(yylloc);
"--"		return parser::make_OP_DEC(yylloc);
"=>"		return parser::make_OP_INLINE_SW(yylloc);
"->"		return parser::make_OP_WRAP(yylloc);

"async"		return parser::make_KW_ASYNC(yylloc);
"await"		return parser::make_KW_AWAIT(yylloc);
"break"		return parser::make_KW_BREAK(yylloc);
"case"		return parser::make_KW_CASE(yylloc);
"class"		return parser::make_KW_CLASS(yylloc);
"const"		return parser::make_KW_CONST(yylloc);
"continue"	return parser::make_KW_CONTINUE(yylloc);
"default"	return parser::make_KW_DEFAULT(yylloc);
"else"		return parser::make_KW_ELSE(yylloc);
"enum"		return parser::make_KW_ENUM(yylloc);
"fn"		return parser::make_KW_FN(yylloc);
"for"		return parser::make_KW_FOR(yylloc);
"if"		return parser::make_KW_IF(yylloc);
"import"	return parser::make_KW_IMPORT(yylloc);
"interface"	return parser::make_KW_INTERFACE(yylloc);
"null"		return parser::make_KW_NULL(yylloc);
"pub"		return parser::make_KW_PUB(yylloc);
"return"	return parser::make_KW_RETURN(yylloc);
"self"		return parser::make_KW_SELF(yylloc);
"switch"	return parser::make_KW_SWITCH(yylloc);
"times"		return parser::make_KW_TIMES(yylloc);
"while"		return parser::make_KW_WHILE(yylloc);

"i8"		return parser::make_TN_I8(yylloc);
"i16"		return parser::make_TN_I16(yylloc);
"i32"		return parser::make_TN_I32(yylloc);
"i64"		return parser::make_TN_I64(yylloc);
"u8"		return parser::make_TN_U8(yylloc);
"u16"		return parser::make_TN_U16(yylloc);
"u32"		return parser::make_TN_U32(yylloc);
"u64"		return parser::make_TN_U64(yylloc);
"float"		return parser::make_TN_FLOAT(yylloc);
"double"	return parser::make_TN_DOUBLE(yylloc);
"string"	return parser::make_TN_STRING(yylloc);
"auto"		return parser::make_TN_AUTO(yylloc);

"/*"				BEGIN(COMMENT);
<COMMENT>"*/"		BEGIN(INITIAL);
<COMMENT>.*	;

"//"				BEGIN(LINE_COMMENT);
<LINE_COMMENT>\n	BEGIN(INITIAL);
<LINE_COMMENT>.*	;

[a-zA-Z_][a-zA-Z0-9_]* return parser::make_T_ID(yytext, yylloc);

[-]?[0-9]+ return parser::make_L_INT(strtol(yytext, nullptr, 10), yylloc);
[-]?[0-9]+[uU] return parser::make_L_UINT(strtoul(yytext, nullptr, 10), yylloc);
[-]?[0-9]+[lL] return parser::make_L_LONG(strtoll(yytext, nullptr, 10), yylloc);
[-]?[0-9]+(([uU][lL])|([lL][uU])) return parser::make_L_ULONG(strtoull(yytext, nullptr, 10), yylloc);

[-]?[0-9]+(\.([0-9]*f)|f) return parser::make_L_FLOAT(strtof(yytext, nullptr), yylloc);
[-]?[0-9]+(\.[0-9]*) return parser::make_L_DOUBLE(strtod(yytext, nullptr), yylloc);

-0[xX][0-9]+ return parser::make_L_INT(strtol(yytext, nullptr, 16), yylloc);
0[xX][0-9]+ return parser::make_L_UINT(strtoul(yytext, nullptr, 16), yylloc);
-0[xX][0-9]+[lL] return parser::make_L_LONG(strtoll(yytext, nullptr, 16), yylloc);
0[xX][0-9]+[lL] return parser::make_L_ULONG(strtoull(yytext, nullptr, 16), yylloc);

\" {
	BEGIN(STRING);
	parser::make_L_STRING(yytext, yylloc).move(yylval);
}
<STRING>[^\"\n\\]+ {
	yylval.value.as<std::string>() += yytext;
}
<STRING>\n {
	yyparser->error(yylloc, "unterminated string literal");
	BEGIN(INITIAL);
	return parser::make_YYerror(yylloc);
}
<STRING>\" {
	BEGIN(INITIAL);
	return yylval;
}
<STRING>\\ {
	BEGIN(ESCAPE);
}
<ESCAPE>\\ {
	yylval.value.as<std::string>() += "\\";
	BEGIN(STRING);
}
<ESCAPE>n {
	yylval.value.as<std::string>() += "\n";
	BEGIN(STRING);
}
<ESCAPE>r {
	yylval.value.as<std::string>() += "\r";
	BEGIN(STRING);
}
<ESCAPE>\" {
	yylval.value.as<std::string>() += "\"";
	BEGIN(STRING);
}
<ESCAPE>0 {
	yylval.value.as<std::string>() += "\0";
	BEGIN(STRING);
}
<ESCAPE>x[0-9a-fA-F]{2} {
	char s[2] = { (char)strtoul(yytext, nullptr, 16), '\0' };
	yylval.value.as<std::string>() += s;
	BEGIN(STRING);
}
<ESCAPE>[0-7]{3} {
	char s[2] = { (char)strtoul(yytext, nullptr, 8), '\0' };
	yylval.value.as<std::string>() += s;
	BEGIN(STRING);
}

[\t\r ]+ yylloc.step();
\n+ yylloc.lines(yyleng); yylloc.step();
<<EOF>> return parser::make_YYEOF(yylloc);

%%
