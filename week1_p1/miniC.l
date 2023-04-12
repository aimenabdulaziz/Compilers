/*
 * MiniC Compiler Lex File
 * Author: Aimen Abdulaziz
 * Date: April 9, 2023
 *
 * This file contains the lexical analyzer rules for the MiniC compiler.
 * It works in conjunction with the Yacc file, which contains
 * the grammar rules and actions for parsing the MiniC language.
 */

%{
#include "y.tab.h"
%}

%option yylineno

%%

"int"                   { printf("%s", yytext); return INT; }
"extern"                { printf("%s", yytext); return EXTERN; }
"return"                { printf("%s", yytext); return RETURN; }
"if"                    { printf("%s", yytext); return IF; }
"else"                  { printf("%s", yytext); return ELSE; }
"while"                 { printf("%s", yytext); return WHILE; }
"void"                  { printf("%s", yytext); return VOID; }
"print"                 { printf("%s", yytext); return PRINT; }
"read"                  { printf("%s", yytext); return READ; }
"+"                     { printf("%s", yytext); return PLUS; }
"-"                     { printf("%s", yytext); return MINUS; }
"*"                     { printf("%s", yytext); return MULTIPLY; }
"/"                     { printf("%s", yytext); return DIVIDE; }
">"                     { printf("%s", yytext); return GT; }
"<"                     { printf("%s", yytext); return LT; }
"=="                    { printf("%s", yytext); return EQ; }
">="                    { printf("%s", yytext); return GE; }
"<="                    { printf("%s", yytext); return LE; }
"("                     { printf("%s", yytext); return LPAREN; }
")"                     { printf("%s", yytext); return RPAREN; }
"{"                     { printf("%s", yytext); return LBRACE; }
"}"                     { printf("%s", yytext); return RBRACE; }
";"                     { printf("%s", yytext); return SEMICOLON; }
","                     { printf("%s", yytext); return COMMA; }
"="                     { printf("%s", yytext); return ASSIGN; }
[0-9]+                  { printf("%s", yytext); yylval.number = atoi(yytext); return NUMBER; }
[a-zA-Z_][a-zA-Z_0-9]*  { printf("%s", yytext); yylval.string = strdup(yytext); return IDENTIFIER; }
[ \t\n]+                { printf("%s", yytext); /* ignore whitespace */ }
.                       { printf("%s", yytext); return yytext[0]; }

%%

int yywrap(void) {
    return 1;
}
