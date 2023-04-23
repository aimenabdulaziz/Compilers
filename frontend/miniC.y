/*
 * MiniC Compiler Yacc File
 * Author: Aimen Abdulaziz
 * Date: Spring, 2023
 *
 * This file contains the grammar rules and actions for parsing a
 * subset of the C programming language called MiniC. The grammar
 * is specified using Yacc notation and works in conjunction with
 * the Lex file for lexical analysis.
 */
 
%{
#include "ast.h" 
#include "semantic_analysis.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector> // C++ vector
#include <stack> // C++ stack
#include <set>

extern astNode *root; // root node is defined in ast.h

extern void yyerror(const char *);
extern int yylex();
extern FILE *yyin;
extern int yylex_destroy();
extern int yylineno;
extern char *yytext;
%}

%union {
    int number;
    char *sname;
    astNode *node;
    std::vector<astNode*> *node_list;
}

%token <sname> IDENTIFIER PRINT READ 
%token <number> NUMBER
%token RETURN IF ELSE WHILE EXTERN VOID INT
%token PLUS MINUS MULTIPLY DIVIDE GT LT EQ GE LE
%token LPAREN RPAREN LBRACE RBRACE SEMICOLON COMMA ASSIGN

%type <node> program extern_print extern_read parameter function_definition function_header
%type <node> block_stmt variable_declaration statement assignment_statement
%type <node> return_statement expression term if_statement while_loop function_call condition
%type <node_list> statement_list variable_declarations

%nonassoc IFX
%nonassoc ELSE
%nonassoc UNARY

%%

/* Assumes there are always two external declarations */
program:
    extern_print extern_read function_definition { 
        $$ = createProg($1, $2, $3);
        root = $$;
    }
    ;

extern_print:
    EXTERN VOID PRINT LPAREN extern_parameter RPAREN SEMICOLON { $$ = createExtern($3); free($3); }
    ;

extern_read:
    EXTERN INT READ LPAREN RPAREN SEMICOLON { $$ = createExtern($3); free($3); }
    ;

extern_parameter:
    parameter 
    | INT // Extern function declaration with only parameter type but no name
    ;

/* Function paramter - at most one parameter */
parameter: 
    INT IDENTIFIER  { $$ = createVar($2); free($2); }
    | { $$ = NULL; }
    ;

/* Function definition
 * Assumes there is always one function definition
 * The function always takes at most one parameter
 */
function_definition:
    function_header block_stmt { 
        $1->func.body = $2;
        $$ = $1; 
    }
    ;

function_header:
    INT IDENTIFIER LPAREN parameter RPAREN { 
        $$ = createFunc($2, $4, NULL); 
        free($2);
    }
    ;

block_stmt:
    // Code block with variable declarations at the top
    LBRACE variable_declarations statement_list RBRACE { 
        // add the declarations list to the statement_list and pass it to createBlock
        $3->insert($3->begin(), $2->begin(), $2->end());
        $$ = createBlock($3); 
        delete $2;
    }
    ;

variable_declarations:
    // One or more variable declarations
    variable_declarations variable_declaration {
        // Prepend the current declaration to the list and pass it up the tree
        $1->push_back($2);
        $$ = $1;
    }
    // No variable declarations - empty vector
    | { $$ = new std::vector<astNode*>(); }
    ;

// Declare variables of integer type
variable_declaration:
    INT IDENTIFIER SEMICOLON { $$ = createDecl($2); free($2); }
    ;

statement_list:
    statement_list statement {
        // Prepend the current statement to the list and pass it up the tree
        $1->push_back($2);
        $$ = $1;
    }
    | statement { 
        // Create a new vector and add the current statement to it
        $$ = new std::vector<astNode*>(); 
        $$->push_back($1); 
        }
    ;

statement: 
    assignment_statement SEMICOLON { $$ = $1; }
    | return_statement SEMICOLON { $$ = $1; }
    | function_call SEMICOLON { $$ = $1; }
    | if_statement { $$ = $1; }
    | while_loop { $$ = $1; }
    | block_stmt { $$ = $1; }
    ;

assignment_statement:
    IDENTIFIER ASSIGN expression { 
        astNode *identifier_node = createVar($1);
        $$ = createAsgn(identifier_node, $3);
        free($1);
    }
    ;

return_statement:
    RETURN expression { $$ = createRet($2); } 
    ;

expression:
    term PLUS term { $$ = createBExpr($1, $3, add); }
    | term MINUS term { $$ = createBExpr($1, $3, sub); }
    | term MULTIPLY term { $$ = createBExpr($1, $3, mul); }
    | term DIVIDE term { $$ = createBExpr($1, $3, divide); }
    | term { $$ = $1; } 
    | function_call { $$ = $1; }
    ;

term:
    IDENTIFIER { $$ = createVar($1); free($1); } 
    | NUMBER  { $$ = createCnst($1); } 
    | MINUS term %prec UNARY  { $$ = createUExpr($2, uminus); }

if_statement:
    IF LPAREN condition RPAREN statement %prec IFX { $$ = createIf($3, $5); }
    | IF LPAREN condition RPAREN statement ELSE statement { $$ = createIf($3, $5, $7); }
    ;

while_loop:
    WHILE LPAREN condition RPAREN statement { $$ = createWhile($3, $5); }
    ;

function_call:
    PRINT LPAREN term RPAREN { 
        $$ = createCall($1, $3); 
        free($1);
    }
    | READ LPAREN RPAREN { $$ = createCall($1, NULL); free($1); }
    ;

condition:
    term GT term { $$ = createRExpr($1, $3, gt); }
    | term LT term { $$ = createRExpr($1, $3, lt); }
    | term EQ term { $$ = createRExpr($1, $3, eq); }
    | term GE term { $$ = createRExpr($1, $3, ge); }
    | term LE term { $$ = createRExpr($1, $3, le); }
    ;

%%