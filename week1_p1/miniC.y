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
%token EXTERN PRINT VOID INT RETURN IF ELSE WHILE READ
%token PLUS MINUS MULTIPLY DIVIDE GT LT EQ GE LE
%token LPAREN RPAREN LBRACE RBRACE SEMICOLON COMMA ASSIGN
%nonassoc IFX
%nonassoc ELSE
%nonassoc UNARY

%%

program:
    extern_declaration function_definition // Top-level grammar rule
    ;

extern_declaration:
    extern_print extern_read // Assumes there are always two external declarations
    ;

extern_print:
    EXTERN VOID PRINT LPAREN extern_parameter RPAREN SEMICOLON // Extern print function declaration
    ;

extern_read:
    EXTERN INT READ LPAREN RPAREN SEMICOLON // Extern read function declaration
    ;

extern_parameter:
    parameter 
    | INT // Extern function declaration with only parameter type but no name
    ;

parameter: // Function parameter
    INT IDENTIFIER // One parameter
    | // No parameter
    ;

function_definition:
    function_header block_stmt // Function definition with at most single integer parameter
    ;

function_header:
    INT IDENTIFIER LPAREN parameter RPAREN
    ;

block_stmt:
    LBRACE variable_declarations statement_list RBRACE // Code block with variable declarations at the top
    ;

variable_declarations:
    variable_declaration variable_declarations // One or more variable declarations
    | // No variable declarations
    ;

variable_declaration:
    INT IDENTIFIER SEMICOLON  // Declare variables of integer type
    ;

statement_list:
    statement statement_list 
    | statement
    ;

statement: 
    assignment_statement SEMICOLON
    | return_statement SEMICOLON
    | function_call SEMICOLON
    | if_statement
    | while_loop
    | block_stmt
    ;

assignment_statement:
    IDENTIFIER ASSIGN expression // Assign a value to a variable
    ;

return_statement:
    RETURN expression // Return a value from a function
    ;

expression:
    term PLUS term // Addition
    | term MINUS term // Subtraction
    | term MULTIPLY term // Multiplication
    | term DIVIDE term // Division
    | term // No operation
    | function_call // Function call
    ;

term:
    IDENTIFIER // Variable
    | NUMBER // Constant number
    | MINUS term %prec UNARY // Handle negative numbers
    | LPAREN term RPAREN // Parenthesized term
    ;

if_statement:
    IF LPAREN condition RPAREN statement %prec IFX 
    | IF LPAREN condition RPAREN statement ELSE statement
    ;

while_loop:
    WHILE LPAREN condition RPAREN statement // While loop with a code block
    ;

function_call:
    PRINT LPAREN term RPAREN // Function call with an optional argument
    | READ LPAREN RPAREN
    ;

condition:
    term GT term // Greater than
    | term LT term // Less than
    | term EQ term // Equal to
    | term GE term // Greater than or equal to
    | term LE term // Less than or equal to
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
