#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include "ast.h"
#include <unordered_map>
#include <string>

LLVMValueRef traverseASTtoGenerateIR(astNode *node, LLVMModuleRef &module, LLVMBuilderRef &builder, LLVMValueRef &func, unordered_map<string, LLVMValueRef> &varMap, LLVMTypeRef intType);

/* This array maps rop_type values to corresponding LLVMIntPredicate values.
 *
 * The order of values in this array corresponds to the order of values in
 * the rop_type enumeration defined in ast.h.
 * 
 * Example usage:
 * 		LLVMIntPredicate intPredicate = intPredicates[node->rexpr.op];
 * 		LLVMValueRef result = LLVMBuildICmp(builder, intPredicate, lhs, rhs, "");
 */
LLVMIntPredicate intPredicates[] = {
    LLVMIntSLT, // lt
    LLVMIntSGT, // gt
    LLVMIntSLE, // le
    LLVMIntSGE, // ge
    LLVMIntEQ,  // eq
    LLVMIntNE,  // neq
};

/*
 * This array maps op_type values to corresponding LLVM IR instructions.
 *
 * The order of values in this array corresponds to the order of values in
 * the op_type enumeration defined in ast.h.
 *
 * Example usage:
 *     LLVMOpcode opcode = opcodes[node->op];
 *     LLVMValueRef result = LLVMBuildBinOp(builder, opcode, lhs, rhs, "");
 */
LLVMOpcode opcodes[] = {
    LLVMAdd,     // add
    LLVMSub,     // sub
    LLVMUDiv,    // divide
    LLVMMul,     // mul
    LLVMFNeg,     // uminus
};

void emitBlock(LLVMModuleRef &module, LLVMBuilderRef &builder, LLVMValueRef &func, unordered_map<string, LLVMValueRef> &varMap, LLVMTypeRef intType, astNode *body, LLVMBasicBlockRef &currBlock, LLVMBasicBlockRef &mergeBlock) {
	if (body == nullptr || currBlock == nullptr) {
       return;
	}
	// Position the builder at the end of the current block
	LLVMPositionBuilderAtEnd(builder, currBlock);
	
	// Generate LLVM IR code for the body
	traverseASTtoGenerateIR(body, module, builder, func, varMap, intType);
	
	// Unconditionally branch to the merge block
	LLVMBuildBr(builder, mergeBlock);
}

LLVMValueRef traverseStmttoGenerateIR(astStmt *stmt, LLVMModuleRef &module, LLVMBuilderRef &builder, LLVMValueRef &func, unordered_map<string, LLVMValueRef> &varMap, LLVMTypeRef intType) {
	LLVMValueRef result = nullptr;
	switch(stmt->type) {
		case ast_call: {
			if (!strcmp(stmt->call.name, "print")) {
				// Get the 'print' function from the module
				LLVMValueRef printFunc = LLVMGetNamedFunction(module, "print");

				// Generate the IR code for the print function argument
				LLVMValueRef arg = traverseASTtoGenerateIR(stmt->call.param, module, builder, func, varMap, intType);
				LLVMValueRef args[] = { arg };

				// Function type of the 'print' function
				LLVMTypeRef printParamTypes[] = { intType }; // One integer parameter 
				LLVMTypeRef printFuncType = LLVMFunctionType(LLVMVoidType(), printParamTypes, 1, 0);

				// Build the call instruction for the 'print' function
				LLVMBuildCall2(builder, printFuncType, printFunc, args, 1, "");
			} else if (!strcmp(stmt->call.name, "read")) {
				// Get the 'read' function from the module
				LLVMValueRef readFunc = LLVMGetNamedFunction(module, "read");

				// Function type of the 'read' function
				LLVMTypeRef readFuncType = LLVMFunctionType(intType, nullptr, 0, 0);
				
				// Build the call instruction for the 'read' function
				result = LLVMBuildCall2(builder, readFuncType, readFunc, nullptr, 0, "");
			} 
			break;
		}
		case ast_ret: {
			LLVMValueRef returnValue = traverseASTtoGenerateIR(stmt->ret.expr, module, builder, func, varMap, intType);
			LLVMBuildRet(builder, returnValue);
			
			break;
		}
		case ast_block: {
			// Generate LLVM IR code for the block statement
			// Visit all nodes in the statement list of block statement 
			for (auto &node : *stmt->block.stmt_list) {
				traverseASTtoGenerateIR(node, module, builder, func, varMap, intType);
			}
			break;
		}
		case ast_while: {
			// Create a new basic block to start insertion into (headerBlock).
			LLVMBasicBlockRef headerBlock = LLVMAppendBasicBlock(func, "");

			// Create an unconditional branch instruction to jump to the new bb
			LLVMBuildBr(builder, headerBlock);

			// Create basic blocks for the while body
			LLVMBasicBlockRef bodyBlock = LLVMAppendBasicBlock(func, "");

			// Emit code for the body block
			emitBlock(module, builder, func, varMap, intType, stmt->whilen.body, bodyBlock, headerBlock);

			// Create basic blocks for the while exit
			LLVMBasicBlockRef exitBlock = LLVMAppendBasicBlock(func, "");

			// Position the builder at the end of the headerBlock
			LLVMPositionBuilderAtEnd(builder, headerBlock);

			// Get the result of the condition comparison
			LLVMValueRef cmp = traverseASTtoGenerateIR(stmt->whilen.cond, module, builder, func, varMap, intType);

			// Create a conditional branch based on the comparison result in the headerBlock
			LLVMBuildCondBr(builder, cmp, bodyBlock, exitBlock);

			// Emit code for the exit block
			LLVMPositionBuilderAtEnd(builder, exitBlock);
			
			break;
		}
		case ast_if: {
			// Create basic blocks for the if and (else) cases
			LLVMBasicBlockRef ifBlock = LLVMAppendBasicBlock(func, "");
			LLVMBasicBlockRef elseBlock = NULL;
			LLVMBasicBlockRef mergeBlock = NULL;

			LLVMBasicBlockRef conditionBlock = LLVMGetInsertBlock(builder);

			// Position the builder at the end of the ifBlock
			LLVMPositionBuilderAtEnd(builder, ifBlock);

			// Generate LLVM IR code for the if_body
			traverseASTtoGenerateIR(stmt->ifn.if_body, module, builder, func, varMap, intType);

			// Get the last basic block created within the nested constructs of the ifBlock
			LLVMBasicBlockRef lastIfBlock = LLVMGetLastBasicBlock(func);
			LLVMBasicBlockRef lastElseBlock = nullptr;
			// Emit code for the else block
			if (stmt->ifn.else_body != NULL) {
				elseBlock = LLVMAppendBasicBlock(func, "");
				LLVMPositionBuilderAtEnd(builder, elseBlock);

				// Generate LLVM IR code for the else_body
				traverseASTtoGenerateIR(stmt->ifn.else_body, module, builder, func, varMap, intType);

				// Get the last basic block created within the nested constructs of the elseBlock
				lastElseBlock = LLVMGetLastBasicBlock(func);
			}

			// Position the builder at the end of the block where the condition was evaluated
			LLVMPositionBuilderAtEnd(builder, conditionBlock);

			// Get the result of the condition comparison
			LLVMValueRef cmp = traverseASTtoGenerateIR(stmt->ifn.cond, module, builder, func, varMap, intType);

			// Check if there is an else body
			if (stmt->ifn.else_body != NULL) {
				LLVMBuildCondBr(builder, cmp, ifBlock, elseBlock);
				mergeBlock = LLVMAppendBasicBlock(func, "");
			} else {
				mergeBlock = LLVMAppendBasicBlock(func, "");
				// If there's no else body, branch to mergeBlock directly
				LLVMBuildCondBr(builder, cmp, ifBlock, mergeBlock);
			}

			// Position the builder at the end of the last basic block of the if_block
			LLVMPositionBuilderAtEnd(builder, lastIfBlock);

			// Unconditionally branch to the merge block
			LLVMBuildBr(builder, mergeBlock);

			if (stmt->ifn.else_body != NULL) {
				// Position the builder at the end of the last basic block of the else_block
				LLVMPositionBuilderAtEnd(builder, lastElseBlock);

				// Unconditionally branch to the merge block
				LLVMBuildBr(builder, mergeBlock);
			}

			// Emit code for the merge block
			LLVMPositionBuilderAtEnd(builder, mergeBlock);

			break;
		}
		case ast_asgn: {
			// Generate LLVM IR code for the assignment statement
			LLVMValueRef rhsValue = traverseASTtoGenerateIR(stmt->asgn.rhs, module, builder, func, varMap, intType);
			LLVMValueRef varPtr = varMap[stmt->asgn.lhs->var.name];
			LLVMBuildStore(builder, rhsValue, varPtr);
			break;
		}
		case ast_decl: {
			// Generate LLVM IR code for the declaration statement
			LLVMValueRef var = LLVMBuildAlloca(builder, intType, stmt->decl.name);
			LLVMSetAlignment(var, 4);
			varMap[stmt->decl.name] = var;
			break;
		}
		default: {
			printf("Error: Unknown statement type %d\n", stmt->type);
			break;
		}
	}
	return result;
}

// Helper function to traverse the AST and generate LLVM IR code
LLVMValueRef traverseASTtoGenerateIR(astNode *node, LLVMModuleRef &module, LLVMBuilderRef &builder, LLVMValueRef &func, unordered_map<string, LLVMValueRef> &varMap, LLVMTypeRef intType) {
    LLVMValueRef result = nullptr;
	switch(node->type) {
		case ast_prog: {
			traverseASTtoGenerateIR(node->prog.ext1, module, builder, func, varMap, intType);
			traverseASTtoGenerateIR(node->prog.ext2, module, builder, func, varMap, intType);
			traverseASTtoGenerateIR(node->prog.func, module, builder, func, varMap, intType);
			break;
		}
		case ast_extern: {
			// Create external declarations (print and read) for the program
			LLVMTypeRef externFuncType;

			if (!strcmp(node->ext.name, "print")) {
				LLVMTypeRef printParamTypes[] = { intType }; // One integer parameter 
				externFuncType = LLVMFunctionType(LLVMVoidType(), printParamTypes, 1, 0); // Return type is void
			} else if (!strcmp(node->ext.name, "read")) {
				externFuncType = LLVMFunctionType(intType, nullptr, 0, 0); // No parameters
			} 

			// Add the function to the module
			LLVMValueRef func = LLVMAddFunction(module, node->ext.name, externFuncType);
			break;
		}
		case ast_func: {
			// Create a non-variadic function type that returns an integer and takes one parameter of integer type
			LLVMTypeRef returnType = intType;
			LLVMTypeRef paramTypes[] = { intType };
			LLVMTypeRef funcType = LLVMFunctionType(returnType, paramTypes, 1, 0);

			// Add the function to the module
    		func = LLVMAddFunction(module, node->func.name, funcType);

			// Create a new basic block to start insertion into.
			LLVMBasicBlockRef bb = LLVMAppendBasicBlock(func, "");

			// Create a builder to generate instructions with.
			LLVMPositionBuilderAtEnd(builder, bb);

			// Create a variable for the func parameter and store it in the entry block
			LLVMValueRef var = LLVMBuildAlloca(builder, intType, node->func.param->var.name);
			LLVMSetAlignment(var, 4);
			LLVMBuildStore(builder, LLVMGetParam(func, 0), var);

			// Add the variable to the variable map
			varMap[node->func.param->var.name] = var;

			// traverse the function body
			traverseASTtoGenerateIR(node->func.body, module, builder, func, varMap, intType);
			
			// Clear the variable map for the next function (NOT NECESSARY for MiniC)
			varMap.clear();
			
			break;
		}
		case ast_stmt: {
			result = traverseStmttoGenerateIR(&node->stmt, module, builder, func, varMap, intType);
			break;
		}
		case ast_var: {
			result = LLVMBuildLoad2(builder, intType, varMap[node->var.name], "");
			break;
		}
		case ast_cnst: {
			result = LLVMConstInt(intType, node->cnst.value, 0);
			break;
		}
		case ast_rexpr: {
			LLVMValueRef lhs = traverseASTtoGenerateIR(node->rexpr.lhs, module, builder, func, varMap, intType);
			LLVMValueRef rhs = traverseASTtoGenerateIR(node->rexpr.rhs, module, builder, func, varMap, intType);

			// Compare the two values and return the result
			LLVMIntPredicate intPredicate = intPredicates[node->rexpr.op];	
			result = LLVMBuildICmp(builder, intPredicate, lhs, rhs, "");
			break;
		}
		case ast_bexpr: {
			LLVMValueRef lhs = traverseASTtoGenerateIR(node->bexpr.lhs, module, builder, func, varMap, intType);
			LLVMValueRef rhs = traverseASTtoGenerateIR(node->bexpr.rhs, module, builder, func, varMap, intType);

			// Perform the binary operation and return the result
			LLVMOpcode opcode = opcodes[node->bexpr.op];
			result = LLVMBuildBinOp(builder, opcode, lhs, rhs, "");
			break;
		}
		case ast_uexpr: {
			LLVMValueRef expr = traverseASTtoGenerateIR(node->uexpr.expr, module, builder, func, varMap, intType);

			// Perform the unary operation and return the result
			result = LLVMBuildNeg(builder, expr, "");
			break;
		}
		default: {
			printf("Error: Unknown node type %d\n", node->type);
			break;
		}
	}
	return result;
}

// returns a LLVM module
LLVMModuleRef generateLLVMIR(astNode *node, char *filename) {
	if (!node) {
        printf("Error: AST is empty\n");
        return NULL;
    }
	// Create LLVM module
    LLVMModuleRef module = LLVMModuleCreateWithName(filename);
	LLVMSetTarget(module, "x86_64-pc-linux-gnu");

	// Create LLVM builder
    LLVMBuilderRef builder = LLVMCreateBuilder();

	// Create LLVM int primitive type
    LLVMTypeRef intType = LLVMInt32Type();

	LLVMValueRef func;

	// Initialize a map to store the value references of variables
	// key: variable name, value: value reference of the alloacted memory
	unordered_map<string, LLVMValueRef> varMap;

	traverseASTtoGenerateIR(node, module, builder, func, varMap, intType);

	// Verify the generated module
    if (LLVMVerifyModule(module, LLVMAbortProcessAction, NULL)) {
        printf("Error: The module is not valid\n");
        return NULL;
    }

	// Dump the generated LLVM IR code
    LLVMDumpModule(module);
	// GET THE BASENAME OF THE FILE AND CONCATENATE `.ll` TO IT
	// LLVMPrintModuleToFile (module, basename, NULL); // Writes the LLVM IR to a file named "test.ll"

    // Cleanup
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);

	return module;
}
