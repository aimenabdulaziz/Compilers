/*
 * MiniC Compiler - Semantic Analysis Header
 *
 * This header file contains the declarations for the semantic analysis
 * phase of the MiniC compiler. 
 * 
 * Author: Aimen Abdulaziz
 * Date: Spring 2023
 */

#ifndef SEMANTIC_ANALYSIS_H
#define SEMANTIC_ANALYSIS_H

#include "ast.h"

/************************** semanticAnalysis **************************/
/* Takes an Abstract Syntax Tree (AST) node as an argument and performs 
 * the necessary analysis to ensure all variables have been declared before use.
 * The function returns true if the semantic analysis is successful, and
 * false otherwise.
 */
bool semanticAnalysis(astNode *node);

#endif // SEMANTIC_ANALYSIS_H
