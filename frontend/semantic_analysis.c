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
static void traverse(astNode *node, vector<set<string>> *symbolTables);
static void traverseStmt(astStmt *stmt, vector<set<string>> *symbolTables);

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
 * populates symbol tables accordingly.
 * see minic_semantic_analysis.h for more description 
 */
void semanticAnalysis(astNode *node) {
    if (!node) {
		return;
	}

    // Declare an empty stack of symbol tables
    vector<set<string>> symbolTables;
	
	// Traverse the AST and populate the symbol tables
	traverse(node, &symbolTables);
}

/**************** traverse() ****************/
/**
 * A recursive helper function that traverses the given AST node and its
 * children. It populates the provided symbol tables based on the
 * encountered variables and their declarations.
 *
 * node:          The current AST node being traversed.
 * symbolTables:  A pointer to the vector containing the symbol tables.
 */
static void traverse(astNode *node, vector<set<string>> *symbolTables) {
	switch (node->type) {
		case ast_prog: {
            traverse(node->prog.ext1, symbolTables); 
			traverse(node->prog.ext2, symbolTables);
			traverse(node->prog.func, symbolTables);
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
			traverse(node->func.body, symbolTables);

			// pop the symbol table from the stack
			symbolTables->pop_back();

			break;
		}

		case ast_stmt: {
			traverseStmt(&node->stmt, symbolTables);
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
			}

			break;
		}

		case ast_cnst: {
			// ignore the values of constant nodes
			break;
		}

		case ast_rexpr: {
			traverse(node->rexpr.lhs, symbolTables);
			traverse(node->rexpr.rhs, symbolTables);
			break;
		}

		case ast_bexpr: {
			traverse(node->bexpr.lhs, symbolTables);
			traverse(node->bexpr.rhs, symbolTables);
			break;
		}

		case ast_uexpr: {
			traverse(node->uexpr.expr, symbolTables);
			break;
		}
	}
}

/**************** traverseStmt() ****************/
/**
 * A recursive helper function that traverses the given AST statement node
 * and its children. It populates the provided symbol tables based on the
 * encountered variables and their declarations.
 *
 * stmt:          The current AST statement node being traversed.
 * symbolTables:  A pointer to the vector containing the symbol tables.
 */
static void traverseStmt(astStmt *stmt, vector<set<string>> *symbolTables) {
	if (!stmt) {
		return;
	}

	switch (stmt->type) {
		case ast_call: {
			if (stmt->call.param) {
				traverse(stmt->call.param, symbolTables);
			}
			break;
		}
		
		case ast_ret: {
			traverse(stmt->ret.expr, symbolTables);
			break;
		}

		case ast_block: {
			// create a new symbol table currSymTable and push it to symbol table stack
			set<string> currSymTable;
			symbolTables->push_back(currSymTable);

			// visit all nodes in the statement list of block statement 
			for (auto &node : *stmt->block.stmt_list) {
				traverse(node, symbolTables);
			}

			// pop the symbol table from the stack
			symbolTables->pop_back();

			break;
		}

		case ast_while: {
			traverse(stmt->whilen.cond, symbolTables);
			traverseStmt(&stmt->whilen.body->stmt, symbolTables);
			break;
		}

		case ast_if: {
			traverse(stmt->ifn.cond, symbolTables);
			traverseStmt(&stmt->ifn.if_body->stmt, symbolTables);
			if (stmt->ifn.else_body) {
				traverseStmt(&stmt->ifn.else_body->stmt, symbolTables);
			}
			break;
		}

		case ast_asgn: {
			traverse(stmt->asgn.lhs, symbolTables);
			traverse(stmt->asgn.rhs, symbolTables);
			break;
		}

		case ast_decl: {
			symbolTables->back().insert(stmt->decl.name);
			break;
		}
	}
}
