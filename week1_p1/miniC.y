/*
 * MiniC Compiler Yacc File
 * Author: Aimen Abdulaziz
 * Date: April 9, 2023
 *
 * This file contains the grammar rules and actions for parsing a
 * subset of the C programming language called MiniC. The grammar
 * is specified using Yacc notation and works in conjunction with
 * the Lex file for lexical analysis.
 */
 
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void yyerror(const char *s);
int yylex();
extern FILE *yyin;
extern int yylex_destroy();
extern int yylineno;
extern char *yytext;
%}

%union {
    int number;
    char *string;
}

%token <string> IDENTIFIER
%token <number> NUMBER
%token EXTERN PRINT VOID INT RETURN IF ELSE WHILE
%token PLUS MINUS MULTIPLY DIVIDE GT LT EQ GE LE
%token LPAREN RPAREN LBRACE RBRACE SEMICOLON COMMA ASSIGN

%%

program:
    declaration_list // Top-level grammar rule
    ;

declaration_list:
    declaration_list declaration
    | declaration
    ;

declaration:
    function_declaration // Function definition
    | extern_declaration // External function declaration
    ;

function_declaration:
    INT IDENTIFIER LPAREN parameter RPAREN LBRACE statement_list RBRACE // Function definition with at most single integer parameter
    ;

extern_declaration:
    EXTERN type IDENTIFIER LPAREN extern_parameter RPAREN SEMICOLON // Extern function declaration
    ;

parameter:
    INT IDENTIFIER // One parameter
    | // No parameter
    ;

    ;

extern_parameter:
    parameter 
    | INT // Extern function declaration with only parameter type but no name
    ;

statement_list:
    statement_list statement
    | statement
    ;

type: // Used for external declaration  
    INT
    | VOID
    ;

statement:
    variable_declaration SEMICOLON  
    | assignment_statement SEMICOLON
    | return_statement SEMICOLON
    | function_call SEMICOLON
    | if_statement
    | while_statement
    ;

variable_declaration:
    INT IDENTIFIER // Declare integer variables
    ;

assignment_statement:
    IDENTIFIER ASSIGN expression // Assign a value to a variable
    ;

return_statement:
    RETURN expression // Return a value from a function
    ;

expression:
    expression PLUS term // Addition
    | expression MINUS term // Subtraction
    | term
    | function_call
    ;

term:
    // Give precedence to multiplication and division
    term MULTIPLY factor // Multiplication
    | term DIVIDE factor // Division
    | factor
    | MINUS factor  // handle negative numbers
    ;

factor:
    IDENTIFIER // Variable
    | NUMBER // Constant number
    | LPAREN expression RPAREN // Parenthesized expression (higher precedence)
    ;

if_statement:
    IF LPAREN condition RPAREN LBRACE statement_list RBRACE // If statement with a code block
    | IF LPAREN condition RPAREN statement // If statement with a single statement
    | IF LPAREN condition RPAREN LBRACE statement_list RBRACE ELSE LBRACE statement_list RBRACE // If-else statement with code blocks
    ;

while_statement:
    WHILE LPAREN condition RPAREN LBRACE statement_list RBRACE // While loop with a code block
    ;

function_call:
    IDENTIFIER LPAREN argument RPAREN // Function call with an optional argument
    ;

condition:
    expression GT expression // Greater than
    | expression LT expression // Less than
    | expression EQ expression // Equal to
    | expression GE expression // Greater than or equal to
    | expression LE expression // Less than or equal to
    ;

argument: // Argument for function call
    IDENTIFIER // Variable as argument
    | NUMBER // Constant number as argument
    | // No argument
    ;

%%

int main(int argc, char* argv[]) {
    if (argc == 2) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            fprintf(stderr, "Could not open file '%s'\n", argv[1]);
            exit(1);
        }
    }

    yyparse();

    if (yyin != stdin) {
        fclose(yyin);
    }

    yylex_destroy();
    
    return 0;
}

void yyerror(const char *s) {
    printf("\nSyntax error (line: %d). Last token: %s\n", yylineno, yytext);
}
