/*
 * MiniC Compiler Lex File
 * Author: Aimen Abdulaziz
 * Date: Spring, 2023
 *
 * This file contains the lexical analyzer rules for the MiniC compiler.
 * It works in conjunction with the Yacc file, which contains
 * the grammar rules and actions for parsing the MiniC language.
 */

%{
#include "ast.h"
#include "y.tab.h"
%}

%option yylineno

%%

"int"                   { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return INT; 
                        }
"extern"                { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return EXTERN; 
                        }
"return"                { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return RETURN; 
                        }
"if"                    { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return IF; 
                        }
"else"                  { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return ELSE; 
                        }
"while"                 { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return WHILE; 
                        }
"void"                  { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return VOID; 
                        }
"print"                 { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            yylval.sname = strdup(yytext); 
                            return PRINT; 
                        }
"read"                  { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            yylval.sname = strdup(yytext); 
                            return READ; 
                        }
"+"                     { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return PLUS; 
                        }
"-"                     { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return MINUS; 
                        }
"*"                     { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return MULTIPLY; 
                        }
"/"                     { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return DIVIDE; 
                        }
">"                     { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return GT; 
                        }
"<"                     { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return LT; 
                        }
"=="                    { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return EQ; 
                        }
">="                    { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return GE; 
                        }
"<="                    { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return LE; 
                        }
"("                     { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return LPAREN; 
                        }
")"                     { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return RPAREN; 
                        }
"{"                     { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return LBRACE; 
                        }
"}"                     { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return RBRACE; 
                        }
";"                     { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return SEMICOLON; 
                        }
","                     { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return COMMA; 
                        }
"="                     { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return ASSIGN; 
                        }
[0-9]+                  { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            yylval.number = atoi(yytext); 
                            return NUMBER; 
                        }
[a-zA-Z_][a-zA-Z_0-9]*  { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            yylval.sname = strdup(yytext); 
                            return IDENTIFIER; 
                        }
[ \t\n]+                { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            /* ignore whitespace */ 
                        }
.                       { 
                            #ifdef DEBUG 
                            printf("%s", yytext); 
                            #endif 
                            return yytext[0]; 
                        }

%%

int yywrap(void) {
    return 1;
}