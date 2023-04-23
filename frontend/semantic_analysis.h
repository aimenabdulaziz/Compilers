/*
 * MiniC Compiler - Semantic Analysis Header
 *
 * This header file contains the declarations for the semantic analysis
 * phase of the MiniC compiler. 
 * 
 * Author: Aimen Abdulaziz
 * Date: Spring 2023
 */

#ifndef MINIC_SEMANTIC_ANALYSIS_H
#define MINIC_SEMANTIC_ANALYSIS_H

#include "ast.h"

/************************** semanticAnalysis **************************/
/* Takes an Abstract Syntax Tree (AST) node as an argument and performs 
 * the necessary analysis to ensure all variables have been declared before use.
 */
void semanticAnalysis(astNode *node);

#endif // MINIC_SEMANTIC_ANALYSIS_H
