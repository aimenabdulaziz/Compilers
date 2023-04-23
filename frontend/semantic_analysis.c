/**
 * miniC_semantic_analysis.c
 * -------------------------
 * This file contains the implementation of semantic analysis for a
 * Mini-C abstract syntax tree (AST). 
 * 
 * Author: Aimen Abdulaziz
 * Date: Spring 2023
 */

#include "ast.h"
#include "semantic_analysis.h"
#include <stdio.h>
#include <vector>
#include <set>
#include <string>
using namespace std;

/********************** local function prototypes ********************* */
static bool traverse(astNode *node, vector<set<string>> *symbolTables);
static bool traverseStmt(astStmt *stmt, vector<set<string>> *symbolTables);

/*
 * Helper function to print the symbol tables for debugging purposes
 *
 * symbolTables: A reference to the vector containing the symbol tables.
 */
void printSymbolTables(const vector<set<string>>& symbolTables) {
    int index = 0;
    for (const auto& table : symbolTables) {
        printf("Index %d:\n", index);
        for (const auto& symbol : table) {
            printf("  %s\n", symbol.c_str());
        }
        index++;
    }
}

/**************** semanticAnalysis() ****************/
/* It traverses the AST and checks for undeclared variables and 
 * populates symbol tables accordingly. User will only call this 
 * function, and all necessary helper functions will invoked in 
 * this function.
 * 
 */
void semanticAnalysis(astNode *node) {
    if (!node) {
		printf("Error: AST is empty\n");
		return;
	}

    // Declare an empty stack of symbol tables
    vector<set<string>> symbolTables;
	
	// Traverse the AST and populate the symbol tables
	bool errorFound = traverse(node, &symbolTables);

    if (errorFound) {
		printf("Result: Semantic analysis unsuccessful.\n");
    } else {
        printf("Result: Semantic analysis successful.\n");
    }
}

/**************** traverse() ****************/
/**
 * A recursive helper function that traverses the given AST node and its
 * children. It populates the provided symbol tables based on the
 * encountered variables and their declarations.
 *
 * node:          The current AST node being traversed.
 * symbolTables:  A pointer to the vector containing the symbol tables.
 * 
 * returns:       true if an error is found, false otherwise.
 */
static bool traverse(astNode *node, vector<set<string>> *symbolTables) {
	bool errorFound = 0;

	if (!node) {
		return errorFound;
	}

	switch (node->type) {
		case ast_prog: {
			errorFound = traverse(node->prog.ext1, symbolTables) || errorFound; 
			errorFound = traverse(node->prog.ext2, symbolTables) || errorFound; 
			errorFound = traverse(node->prog.func, symbolTables) || errorFound; 
			break;
		}

		case ast_extern: {
			// ignore extern nodes
			break;
		}
		
		case ast_func: {
			// create a new symbol table currSymTable and push it to symbol table stack
			set<string> currSymTable;
			symbolTables->push_back(currSymTable);

			// if func node has a parameter add parameter to currSymTable
			if (node->func.param) {
				symbolTables->back().insert(node->func.param->var.name);
			}

			// traverse the body of the function
			errorFound = traverse(node->func.body, symbolTables) || errorFound; 

			// pop the symbol table from the stack
			symbolTables->pop_back();

			break;
		}

		case ast_stmt: {
			errorFound = traverseStmt(&node->stmt, symbolTables) || errorFound; 
			break;
		}

		case ast_var: {
			// check if the variable appears in one of the symbol tables on the stack
			bool found = false;
			for (auto &table : *symbolTables) {
				if (table.contains(node->var.name)) {
					// variable found in symbol table
					found = true;
					break;
				}
			}

			if (!found) {
				// variable not found in symbol table
				printf("Error: undeclared variable '%s'\n", node->var.name);
				errorFound = true; // Unsuccessful semantic analysis due to undeclared variable
			}

			break;
		}

		case ast_cnst: {
			// ignore the values of constant nodes
			break;
		}

		case ast_rexpr: {
			errorFound = traverse(node->rexpr.lhs, symbolTables) || errorFound; 
			errorFound = traverse(node->rexpr.rhs, symbolTables) || errorFound; 
			break;
		}

		case ast_bexpr: {
			errorFound = traverse(node->bexpr.lhs, symbolTables) || errorFound; 
			errorFound = traverse(node->bexpr.rhs, symbolTables) || errorFound; 
			break;
		}

		case ast_uexpr: {
			errorFound = traverse(node->uexpr.expr, symbolTables) || errorFound; 
			break;
		}
	}
	return errorFound;
}

/**************** traverseStmt() ****************/
/**
 * A recursive helper function that traverses the given AST statement node
 * and its children. It populates the provided symbol tables based on the
 * encountered variables and their declarations.
 *
 * stmt:          The current AST statement node being traversed.
 * symbolTables:  A pointer to the vector containing the symbol tables.
 * 
 * returns:       true if an error is found, false otherwise.
 */
static bool traverseStmt(astStmt *stmt, vector<set<string>> *symbolTables) {
	bool errorFound = 0;
	if (!stmt) {
		return errorFound;
	}

	switch (stmt->type) {
		case ast_call: {
			if (stmt->call.param) {
				errorFound = traverse(stmt->call.param, symbolTables) || errorFound; 
			}
			break;
		}
		
		case ast_ret: {
			errorFound = traverse(stmt->ret.expr, symbolTables) || errorFound; 
			break;
		}

		case ast_block: {
			// create a new symbol table currSymTable and push it to symbol table stack
			set<string> currSymTable;
			symbolTables->push_back(currSymTable);

			// visit all nodes in the statement list of block statement 
			for (auto &node : *stmt->block.stmt_list) {
				errorFound |= traverse(node, symbolTables);
			}

			// pop the symbol table from the stack
			symbolTables->pop_back();

			break;
		}

		case ast_while: {
			errorFound = traverse(stmt->whilen.cond, symbolTables) || errorFound; 
			errorFound = traverseStmt(&stmt->whilen.body->stmt, symbolTables) || errorFound; 
			break;
		}

		case ast_if: {
			errorFound = traverse(stmt->ifn.cond, symbolTables) || errorFound; 
			errorFound = traverseStmt(&stmt->ifn.if_body->stmt, symbolTables) || errorFound; 
			if (stmt->ifn.else_body) {
				errorFound = traverseStmt(&stmt->ifn.else_body->stmt, symbolTables) || errorFound; 
			}
			break;
		}

		case ast_asgn: {
			errorFound = traverse(stmt->asgn.lhs, symbolTables) || errorFound; 
			errorFound = traverse(stmt->asgn.rhs, symbolTables) || errorFound; 
			break;
		}

		case ast_decl: {
			symbolTables->back().insert(stmt->decl.name);
			break;
		}
	}
	return errorFound;
}
