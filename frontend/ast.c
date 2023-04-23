#include"ast.h"
#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<string.h>
#include <vector>
#include <set>
#include <string>
using namespace std;

/* local helper functions */
char * get_indent_str(int n){
	char * ret = (char *) calloc(n+1, sizeof(char));
	for (int i=0; i < n; i++)
		ret[i] = ' ';
	return ret;
}

/* create and free functions for ast_prog type astNode */
astNode* createProg(astNode *ext1, astNode	*ext2, astNode	*func){
	astNode	*node;
	node = (astNode *)calloc(1, sizeof(astNode));
	node->type = ast_prog;

	node->prog.ext1 = ext1;
	node->prog.ext2 = ext2;
	node->prog.func = func;
	
	return(node); 
}

void freeProg(astNode *node){
	assert(node != NULL && node->type == ast_prog);
	
	freeExtern(node->prog.ext1);
	freeExtern(node->prog.ext2);
	freeFunc(node->prog.func);
	
	free(node);
	return;
}

/*create and free functions for ast_func type astNode */
astNode* createFunc(const char *name, astNode *param, astNode* body){
	astNode *node;
	node = (astNode*)calloc(1, sizeof(astNode));
	node->type = ast_func;

	node->func.name = (char *) calloc(1, sizeof(char) * (strlen(name)+1));
	strcpy(node->func.name, name);

	node->func.param = param;
	node->func.body = body;

	return node;
}

void freeFunc(astNode *node){
	assert(node != NULL && node->type == ast_func);
	
	free(node->func.name);
	if (node->func.param != NULL)
		freeVar(node->func.param);

	freeBlock(node->func.body);
	
	free(node);
	
	return;
}

/*create and free functionns for ast_extern*/

astNode* createExtern(const char *name){
	astNode *node;
	node = (astNode*)calloc(1, sizeof(astNode));
	node->type = ast_extern;
	
	node->ext.name = (char *) calloc(1, sizeof(char) * (strlen(name)+1));
	strcpy(node->ext.name, name);

	return(node);
}

void freeExtern(astNode *node){
	assert(node != NULL && node->type == ast_extern);
	
	free(node->ext.name);
	free(node);

	return;
}

/*create and free functions for ast_var*/

astNode* createVar(const char *name){
	astNode *node;
	node = (astNode*)calloc(1, sizeof(astNode));
	node->type = ast_var;
	
	node->var.name = (char *) calloc(1, sizeof(char) * (strlen(name)+1));
	strcpy(node->var.name, name);
	
	return(node);
}

void freeVar(astNode *node){
	assert(node != NULL && node->type == ast_var);
	
	free(node->var.name);
	free(node);

	return;
}

/*create and free functions for ast_cnst type of node*/
astNode* createCnst(int value){
	astNode *node;
	node = (astNode*)calloc(1, sizeof(astNode));
	node->type = ast_cnst;

	node->cnst.value = value;
	return(node);
}

void freeCnst(astNode *node){
	assert(node != NULL);
	free(node);

	return;
}

/*create and free functions for ast_rexpr type of node*/
astNode* createRExpr(astNode *lhs, astNode *rhs, rop_type op){
	astNode *node;
	node = (astNode*)calloc(1, sizeof(astNode));
	node->type = ast_rexpr;
	
	node->rexpr.lhs = lhs;
	node->rexpr.rhs = rhs;
	node->rexpr.op = op;

	return(node);
}

void freeRExpr(astNode *node){
	assert(node != NULL && node->type == ast_rexpr);
	
	// We call freeNode as we don't know the type of nodes for lhs and rhs
	freeNode(node->rexpr.lhs);
	freeNode(node->rexpr.rhs);
	free(node);

	return;
}


/*create and free functions for ast_bexpr type of node*/
astNode* createBExpr(astNode *lhs, astNode *rhs, op_type op){
	astNode *node;
	node = (astNode*)calloc(1, sizeof(astNode));
	node->type = ast_bexpr;
	
	node->bexpr.lhs = lhs;
	node->bexpr.rhs = rhs;
	node->bexpr.op = op;

	return(node);
}

void freeBExpr(astNode *node){
	assert(node != NULL && node->type == ast_bexpr);
	
	//We call freeNode as we don't know the type of nodes for rhs and lhs
	freeNode(node->bexpr.lhs);
	freeNode(node->bexpr.rhs);

	free(node);

	return;
}

/* create and free functions for ast_uexpr type of node */
astNode* createUExpr(astNode *expr, op_type op){
	astNode *node;
	node = (astNode*)calloc(1, sizeof(astNode));
	node->type = ast_uexpr;
	
	node->uexpr.expr = expr;
	node->uexpr.op = op;
	
	return(node);
}

void freeUExpr(astNode *node){
	assert(node != NULL && node->type == ast_uexpr);
	
	freeNode(node->uexpr.expr);
	free(node);

	return;
}

/* create and free functions for a statement of type ast_call */
astNode* createCall(const char *name, astNode *param){
	astNode *node;
	node = (astNode*) calloc(1, sizeof(astNode));
	node->type = ast_stmt;
	node->stmt.type = ast_call;
	
	node->stmt.call.name = (char *) calloc(strlen(name) + 1, sizeof(char));
	strcpy(node->stmt.call.name, name);
	
	node->stmt.call.param = param;

	return node;
}

void freeCall(astNode *node){
	assert(node != NULL && node->type == ast_stmt);
	assert(node->stmt.type == ast_call);
	
	free(node->stmt.call.name);
	if (node->stmt.call.param != NULL)
		freeNode(node->stmt.call.param);

	free(node);
	return;
}

/*create and free functions for a stmt of type ast_ret*/
astNode* createRet(astNode	*expr){
	astNode *node;
	node = (astNode*) calloc(1, sizeof(astNode));
	node->type = ast_stmt;
	node->stmt.type = ast_ret;
	
	node->stmt.ret.expr = expr;
	return(node);
}

void freeRet(astNode *node){	
	assert(node != NULL && node->type == ast_stmt);
	assert(node->stmt.type == ast_ret);

	freeNode(node->stmt.ret.expr);
	free(node);
	return;
}

/*create and free functions for a stmt of type ast_block*/
astNode* createBlock(vector<astNode*> *stmt_list){
	vector<astNode*> slist;
	astNode* node = (astNode *)calloc(1, sizeof(astNode));
	node->type = ast_stmt;
	node->stmt.type = ast_block;
	
	node->stmt.block.stmt_list = stmt_list;
	
	return(node);
}

void freeBlock(astNode *node){
	assert(node != NULL && node->type == ast_stmt);
	assert(node->stmt.type == ast_block);

	vector<astNode *> slist = *(node->stmt.block.stmt_list);
	vector<astNode*>::iterator it = slist.begin();

	while (it != slist.end()){
		freeNode(*it);
		it++;	
	}
	
	delete(node->stmt.block.stmt_list);
	free(node);
	return;
}

/* create and free functions for stmt of type while*/
astNode* createWhile(astNode *cond, astNode *body){
	astNode* node = (astNode *)calloc(1, sizeof(astNode));
	node->type = ast_stmt;
	node->stmt.type = ast_while;
	
	node->stmt.whilen.cond = cond;
	node->stmt.whilen.body = body;

	return(node);	
}

void freeWhile(astNode *node){
	assert(node != NULL && node->type == ast_stmt);
	assert(node->stmt.type == ast_while);

	freeNode(node->stmt.whilen.cond);
	freeNode(node->stmt.whilen.body);
	
	free(node);
	return;
}

/*create and free functions for stmt of type if*/
astNode* createIf(astNode *cond, astNode *ifbody, astNode *elsebody){
	astNode* node = (astNode *)calloc(1, sizeof(astNode));
	node->type = ast_stmt;
	node->stmt.type = ast_if;

	node->stmt.ifn.cond = cond;
	node->stmt.ifn.if_body = ifbody;
	node->stmt.ifn.else_body = elsebody;
	
	return(node);
}

void freeIf(astNode *node){
	assert(node != NULL && node->type == ast_stmt);
	assert(node->stmt.type == ast_if);
	
	freeNode(node->stmt.ifn.cond);
	freeNode(node->stmt.ifn.if_body);
	if (node->stmt.ifn.else_body != NULL)
		freeNode(node->stmt.ifn.else_body);

	free(node);	

	return;
}

/* create and free functions of stmt type ast_decl */
astNode* createDecl(const char *name){
	astNode* node = (astNode *)calloc(1, sizeof(astNode));
	node->type = ast_stmt;
	node->stmt.type = ast_decl;

	node->stmt.decl.name = (char *)calloc(strlen(name)+1, sizeof(char));
	strcpy(node->stmt.decl.name, name);

	return(node);
}

void freeDecl(astNode *node){
	assert(node != NULL && node->type == ast_stmt);
	assert(node->stmt.type == ast_decl);
	
	free(node->stmt.decl.name);
	free(node);
}

/* create and free functions of stmt type ast_assign */
astNode* createAsgn(astNode *lhs, astNode *rhs){
	astNode* node = (astNode *)calloc(1, sizeof(astNode));
	node->type = ast_stmt;
	node->stmt.type = ast_asgn;

	node->stmt.asgn.lhs = lhs;
	node->stmt.asgn.rhs = rhs;

	return(node);
}

void freeAsgn(astNode *node){
	assert(node != NULL && node->type == ast_stmt);
	assert(node->stmt.type == ast_asgn);
	
	freeVar(node->stmt.asgn.lhs);
	freeNode(node->stmt.asgn.rhs);
	free(node);

	return;
}

/* free function for releasing all the memory assigned to a node based
on the type. This function is called by other free* functions when
the type of a child node is not obvious from the context */

void freeNode(astNode *node){
	assert(node != NULL);

	switch(node->type){
		case ast_prog:{
						freeProg(node);
						break;
					  }
		case ast_func:{
						freeFunc(node);
						break;
					  }
		case ast_stmt:{
						freeStmt(node);
						break;
					  }
		case ast_extern:{
						freeExtern(node);
						break;
					  }
		case ast_var: {	
						freeVar(node);
						break;
					  }
		case ast_cnst: {
						 freeCnst(node);
						 break;
					  }
		case ast_rexpr: {
						freeRExpr(node);
						break;
					  }
		case ast_bexpr: {
						freeBExpr(node);
						break;
					  }
		case ast_uexpr: {
						freeUExpr(node);
						break;
					  }
		default: {
					fprintf(stderr,"Incorrect node type\n");
				 	exit(1);
				 }
	}
}

/* free function to stmt. To be called when stmt type is not obvious
from the context */
void freeStmt(astNode *node){
	assert(node != NULL && node->type == ast_stmt);
	
	switch(node->stmt.type){
		case ast_call: 
						freeCall(node);
						break;
		case ast_ret: 
						freeRet(node);
						break;
		case ast_block:
						freeBlock(node);
						break;
		case ast_while:
						freeWhile(node);
						break;
		case ast_if: 
						freeIf(node);
						break;
		case ast_asgn:	
						freeAsgn(node);
						break;
		case ast_decl:	
						 freeDecl(node);
						 break;
		default: {
					fprintf(stderr,"Incorrect node type\n");
				 	exit(1);
				 };
	}
}

void printNode(astNode *node, int n){
	assert(node != NULL);
	char *indent = get_indent_str(n);
	
	switch(node->type){
		case ast_prog:{
						printf("%sProg:\n",indent);
						printNode(node->prog.func, n+1);
						break;
					  }
		case ast_func:{
						printf("%sFunc: %s\n",indent, node->func.name);
						if (node->func.param != NULL)
							printNode(node->func.param, n+1);

						printNode(node->func.body, n+1);
						break;
					  }
		case ast_stmt:{
						printf("%sStmt: \n",indent);
						astStmt stmt= node->stmt;
						printStmt(&stmt, n+1);
						break;
					  }
		case ast_extern:{
						printf("%sExtern: %s\n", indent, node->ext.name);
						break;
					  }
		case ast_var: {	
						printf("%sVar: %s\n", indent, node->var.name);
						break;
					  }
		case ast_cnst: {
						printf("%sConst: %d\n", indent, node->cnst.value);
						 break;
					  }
		case ast_rexpr: {
						printf("%sRExpr: \n", indent);
						printNode(node->rexpr.lhs, n+1);
						printNode(node->rexpr.rhs, n+1);
						break;
					  }
		case ast_bexpr: {
						printf("%sBExpr: \n", indent);
						printNode(node->bexpr.lhs, n+1);
						printNode(node->bexpr.rhs, n+1);
						break;
					  }
		case ast_uexpr: {
						printf("%sUExpr: \n", indent);
						printNode(node->uexpr.expr, n+1);
						break;
					  }
		default: {
					fprintf(stderr,"Incorrect node type\n");
				 	exit(1);
				 }
	}
	free(indent);
}

void printStmt(astStmt *stmt, int n){
	assert(stmt != NULL);
	char *indent = get_indent_str(n);

	switch(stmt->type){
		case ast_call: { 
							printf("%sCall: name %s\n", indent, stmt->call.name);
							if (stmt->call.param != NULL){
								printf("%sCall: param\n", indent);
								printNode(stmt->call.param, n+1);
							}
							break;
						}
		case ast_ret: {
							printf("%sRet:\n", indent);
							printNode(stmt->ret.expr, n+1);
							break;
						}
		case ast_block: {
							printf("%sBlock:\n", indent);
							vector<astNode*> slist = *(stmt->block.stmt_list);
							vector<astNode*>::iterator it = slist.begin();
							while (it != slist.end()){
								printNode(*it, n+1);
								it++;
							}
							break;
						}
		case ast_while: {
							printf("%sWhile: cond \n", indent);
							printNode(stmt->whilen.cond, n+1);
							printf("%sWhile: body \n", indent);
							printNode(stmt->whilen.body, n+1);
							break;
						}
		case ast_if: {
							printf("%sIf: cond\n", indent);
							printNode(stmt->ifn.cond, n+1);
							printf("%sIf: body\n", indent);
							printNode(stmt->ifn.if_body, n+1);
							if (stmt->ifn.else_body != NULL)
							{
								printf("%sElse: body\n", indent);
								printNode(stmt->ifn.else_body, n+1);
							}
							break;
						}
		case ast_asgn:	{
							printf("%sAsgn: lhs\n", indent);
							printNode(stmt->asgn.lhs, n+1);
							printf("%sAsgn: rhs\n", indent);
							printNode(stmt->asgn.rhs, n+1);
							break;
						}
		case ast_decl:	{
							printf("%sDecl: %s\n", indent, stmt->decl.name);
							break;
						}
		default: {
					fprintf(stderr,"Incorrect node type\n");
				 	exit(1);
				 }
	}
	free(indent);
}

static void traverse(astNode *node, vector<set<string>> *symbol_tables);
static void traverseStmt(astStmt *stmt, vector<set<string>> *symbol_tables);

/*
 * Helper function to print the symbol tables for debugging purposes
 */
void printSymbolTables(const vector<set<string>>& symbol_tables) {
    int index = 0;
    for (const auto& table : symbol_tables) {
        printf("Index %d:\n", index);
        for (const auto& symbol : table) {
            printf("  %s\n", symbol.c_str());
        }
        index++;
    }
}


void semanticAnalysis(astNode *node) {
    if (!node) {
		return;
	}

    // Declare an empty stack of symbol tables
    vector<set<string>> symbol_tables;
	
	// Traverse the AST and populate the symbol tables
	traverse(node, &symbol_tables);
    
}

static void traverse(astNode *node, vector<set<string>> *symbol_tables) {
	switch (node->type) {
		case ast_prog: {
            traverse(node->prog.ext1, symbol_tables); 
			traverse(node->prog.ext2, symbol_tables);
			traverse(node->prog.func, symbol_tables);
			break;
		}

		case ast_extern: {
			// ignore extern nodes
			break;
		}
		
		case ast_func: {
			// create a new symbol table curr_sym_table and push it to symbol table stack
			set<string> curr_sym_table;
			symbol_tables->push_back(curr_sym_table);

			// if func node has a parameter add parameter to curr_sym_table
			if (node->func.param) {
				symbol_tables->back().insert(node->func.param->var.name);
			}

			// traverse the body of the function
			traverse(node->func.body, symbol_tables);

			// pop the symbol table from the stack
			symbol_tables->pop_back();

			break;
		}

		case ast_stmt: {
			traverseStmt(&node->stmt, symbol_tables);
			break;
		}

		case ast_var: {
			// check if the variable appears in one of the symbol tables on the stack
			bool found = false;
			for (auto &table : *symbol_tables) {
				if (table.contains(node->var.name)) {
					// variable found in symbol table
					found = true;
					break;
				}
			}

			if (!found) {
				// variable not found in symbol table
				printf("Error: undeclared variable %s\n", node->var.name);
				// exit(1); do we need to exit here?	
			}

			break;
		}

		case ast_cnst: {
			// ignore the values of constant nodes
			break;
		}

		case ast_rexpr: {
			traverse(node->rexpr.lhs, symbol_tables);
			traverse(node->rexpr.rhs, symbol_tables);
			break;
		}

		case ast_bexpr: {
			traverse(node->bexpr.lhs, symbol_tables);
			traverse(node->bexpr.rhs, symbol_tables);
			break;
		}

		case ast_uexpr: {
			traverse(node->uexpr.expr, symbol_tables);
			break;
		}
	}
}

static void traverseStmt(astStmt *stmt, vector<set<string>> *symbol_tables) {
	if (!stmt) {
		return;
	}

	switch (stmt->type) {
		case ast_call: {
			if (stmt->call.param) {
				traverse(stmt->call.param, symbol_tables);
			}
			break;
		}
		
		case ast_ret: {
			traverse(stmt->ret.expr, symbol_tables);
			break;
		}

		case ast_block: {
			// create a new symbol table curr_sym_table and push it to symbol table stack
			set<string> curr_sym_table;
			symbol_tables->push_back(curr_sym_table);

			// visit all nodes in the statement list of block statement 
			for (auto &node : *stmt->block.stmt_list) {
				traverse(node, symbol_tables);
			}

			// pop the symbol table from the stack
			symbol_tables->pop_back();

			break;
		}

		case ast_while: {
			traverse(stmt->whilen.cond, symbol_tables);
			traverseStmt(&stmt->whilen.body->stmt, symbol_tables);
			break;
		}

		case ast_if: {
			traverse(stmt->ifn.cond, symbol_tables);
			traverseStmt(&stmt->ifn.if_body->stmt, symbol_tables);
			if (stmt->ifn.else_body) {
				traverseStmt(&stmt->ifn.else_body->stmt, symbol_tables);
			}
			break;
		}

		case ast_asgn: {
			traverse(stmt->asgn.lhs, symbol_tables);
			traverse(stmt->asgn.rhs, symbol_tables);
			break;
		}

		case ast_decl: {
			symbol_tables->back().insert(stmt->decl.name);
			break;
		}
	}
}
