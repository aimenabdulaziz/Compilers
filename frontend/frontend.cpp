/*
 * @file Main file for MiniC Compiler
 *
 * This file contains the main function for the MiniC Compiler,
 * which reads in a MiniC file, parses it, outputs the AST, and perform
 * semantic analysis.
 *
 * @author Aimen Abdulaziz
 * @date Spring, 2023
 */

#include "ast.h"
#include "semantic_analysis.h"
#include "ir_generator.h"
#include <iostream>
#include <string.h>
#include "y.tab.h"

extern void yyerror(const char *);
extern int yylex();
extern FILE *yyin;
extern int yylex_destroy();
extern int yylineno;
extern char *yytext;

astNode *root; // The root node of the Abstract Syntax Tree (AST)

using namespace std;

// Clean up function
void cleanup()
{
    if (yyin != stdin)
    {
        fclose(yyin);
    }
    yylex_destroy();

    freeNode(root);
}

// The main function for the MiniC compiler. It takes in a file name as an
// argument, parses the file, outputs the AST and performs semantic analysis on the AST.
int main(int argc, char *argv[])
{
    if (argc == 2)
    {
        // Open the input file
        yyin = fopen(argv[1], "r");
        if (!yyin)
        {
            cerr << "Could not open file '" << argv[1] << "'" << endl;
            return 1;
        }
    }

    // Parse the input
    if (yyparse() != 0)
    {
        cout << "Result: Parsing unsuccessful." << endl;
        cleanup();
        return 2;
    }

    cout << "Result: Parsing successful." << endl;

#ifdef DEBUG
    // Print the AST for debugging
    printNode(root);
#endif

    // Perform semantic analysis on the root node and check for errors
    bool errorFound = semanticAnalysis(root);
    if (errorFound)
    {
        cout << "Result: Semantic analysis unsuccessful." << endl;
        cleanup();
        return 3;
    }

    cout << "Result: Semantic analysis successful." << endl;
    if (!generateIRAndSaveToFile(root, argv[1]))
    {
        cout << "Result: IR generation unsuccessful." << endl;
        cleanup();
        return 4;
    }

    cout << "Result: Intermediate Representation (IR) generation successful." << endl;

    // Clean up
    cleanup();
    return 0;
}

// This function is called by the parser when it encounters a syntax error.
// It prints out the line number and the last token that was read.
void yyerror(const char *)
{
    cout << "\nSyntax error (line: " << yylineno << "). Last token: " << yytext << endl;
}
