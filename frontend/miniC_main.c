/*
 * Main file for MiniC Compiler
 *
 * This file contains the main function for the MiniC Compiler,
 * which reads in a MiniC file, parses it, outputs the AST, and perform 
 * semantic analysis.
 * 
 * Author: Aimen Abdulaziz
 * Date: Spring, 2023
 */

#include "ast.h" 
#include "semantic_analysis.h"
#include "ir_generator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "y.tab.h"

astNode *root;  // The root node of the AST

extern void yyerror(const char *);
extern int yylex();
extern FILE *yyin;
extern int yylex_destroy();
extern int yylineno;
extern char *yytext;

/* ******************** main ******************** */
/* The main function for the MiniC compiler. It takes in a file name as an
 * argument and parses the file. It then outputs the AST and performs
 * semantic analysis on the AST.
 */
int main(int argc, char* argv[]) {
    int exitCode = 0;

    if (argc == 2) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            fprintf(stderr, "Could not open file '%s'\n", argv[1]);
            exit(1);
        }
    }

    // Parse the input
    yyparse();

    // Print the AST
    printNode(root);

    // Perform semantic analysis on the root node and check for errors
    bool errorFound = semanticAnalysis(root);
    if (errorFound) {
		printf("Result: Semantic analysis unsuccessful.\n");
        exitCode = 1;
    } else {
        printf("Result: Semantic analysis successful.\n");
        generateIRAndSaveToFile(root, argv[1]);
    }

    if (yyin != stdin) {
        fclose(yyin);
    }

    yylex_destroy();
    
    freeNode(root);

    return exitCode;
}

/* ******************** yyerror ******************** */
/* This function is called by the parser when it encounters a syntax error.
 * It prints out the line number and the last token that was read.
 */
void yyerror(const char *) {
    printf("\nSyntax error (line: %d). Last token: %s\n", yylineno, yytext);
}
