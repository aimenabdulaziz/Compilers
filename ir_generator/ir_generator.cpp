/*
 * ir_generator.cpp
 *
 * This file implements an Intermediate Representation (IR) generator for the miniC language using LLVM.
 *
 * The IR generator takes a miniC abstract syntax tree (AST) as input and generates LLVM IR code for the given
 * miniC program. The generated LLVM IR can then be further optimized or translated into machine code.
 *
 * The generator performs the following tasks:
 * 1. Translates miniC AST node into corresponding LLVM IR instructions
 * 2. Handles miniC control flow constructs like if statements and while loops
 * 3. Generates appropriate function call instructions for miniC two external functions: read() and print(int)
 *
 * Usage: Include "ir_generator.h" in your project and use the provided functions to generate LLVM IR from a miniC AST.
 *
 * Output: The program generates an output file named 'basename_manual.ll', where 'basename' is derived from the input file,
 *         in the same directory as the input file, containing the LLVM IR code generated for the given miniC program.
 *
 * Author: Aimen Abdulaziz
 * Date: Spring 2023
 */

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include "ast.h"
#include <unordered_map>
#include <string>

using namespace std;

// Local function prototype
static LLVMValueRef traverseASTAndGenerateIR(astNode *node, LLVMModuleRef &module, LLVMBuilderRef &builder, LLVMValueRef &func, unordered_map<string, LLVMValueRef> &varMap, LLVMTypeRef intType);

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
    LLVMFNeg,    // uminus
};

/**
 * @brief Emits LLVM IR code for the given block and branch to the exit block.
 * 
 * @param module       Reference to the LLVM module
 * @param builder      Reference to the LLVM IR builder
 * @param func         Reference to the LLVM function
 * @param varMap       A map of variable names and their LLVM values
 * @param intType      The LLVM integer type (32-bit)
 * @param body         Pointer to the AST node representing the block body
 * @param currBlock    Reference to the current basic block
 * @param exitBlock    Reference to the exit basic block
 */
void emitBlock(LLVMModuleRef &module, LLVMBuilderRef &builder, LLVMValueRef &func, unordered_map<string, LLVMValueRef> &varMap, LLVMTypeRef intType, astNode *body, LLVMBasicBlockRef &currBlock, LLVMBasicBlockRef &exitBlock) {
	if (!body || !currBlock) {
       return;
	}
	// Position the builder at the end of the current block
	LLVMPositionBuilderAtEnd(builder, currBlock);
	
	// Generate LLVM IR code for the body
	traverseASTAndGenerateIR(body, module, builder, func, varMap, intType);
	
	// Unconditionally branch to the exit block
	LLVMBuildBr(builder, exitBlock);
}

/**
 * @brief Traverse the AST and generate LLVM IR code for the input statement.
 * 
 * @param stmt The input statement (AST) to be processed.
 * @param module The LLVM module being constructed.
 * @param builder The LLVM IR builder.
 * @param func The current LLVM function.
 * @param varMap A map storing the LLVM values of variables in the current scope.
 * @param intType The LLVM type of an integer.
 * @return The LLVMValueRef of the resulting LLVM IR or nullptr if the node does not generate a value.
 */
static LLVMValueRef traverseStmtAndGenerateIR(astStmt *stmt, LLVMModuleRef &module, LLVMBuilderRef &builder, LLVMValueRef &func, unordered_map<string, LLVMValueRef> &varMap, LLVMTypeRef intType) {
	LLVMValueRef result = nullptr;
	switch(stmt->type) {
		case ast_call: {
			if (!strcmp(stmt->call.name, "print")) {
				// Get the 'print' function from the module
				LLVMValueRef printFunc = LLVMGetNamedFunction(module, "print");

				// Generate the IR code for the print function argument
				LLVMValueRef arg = traverseASTAndGenerateIR(stmt->call.param, module, builder, func, varMap, intType);
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
            // Handle 'return' statement
            LLVMValueRef returnValue = traverseASTAndGenerateIR(stmt->ret.expr, module, builder, func, varMap, intType);
            LLVMBuildRet(builder, returnValue);
            break;
        }
        case ast_block: {
            // Handle 'block' statement
            for (auto &node : *stmt->block.stmt_list) {
                traverseASTAndGenerateIR(node, module, builder, func, varMap, intType);
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
			LLVMValueRef cmp = traverseASTAndGenerateIR(stmt->whilen.cond, module, builder, func, varMap, intType);

			// Create a conditional branch based on the comparison result in the headerBlock
			LLVMBuildCondBr(builder, cmp, bodyBlock, exitBlock);

			// Emit code for the exit block
			LLVMPositionBuilderAtEnd(builder, exitBlock);
			
			break;
		}
		case ast_if: {
			// Create an if stmt basic block 
			LLVMBasicBlockRef ifBlock = LLVMAppendBasicBlock(func, "");

			// Get the current basic block where the if statement is being inserted into
			LLVMBasicBlockRef insertionBlock = LLVMGetInsertBlock(builder);

			// Position the builder at the end of the ifBlock
			LLVMPositionBuilderAtEnd(builder, ifBlock);

			// Generate LLVM IR code for the if_body
			traverseASTAndGenerateIR(stmt->ifn.if_body, module, builder, func, varMap, intType);

			// Get the last basic block created within the nested constructs of the ifBlock
			LLVMBasicBlockRef lastIfBlock = LLVMGetLastBasicBlock(func);
			
			// Create a basic block for the else block
			LLVMBasicBlockRef elseBlock = nullptr;
			LLVMBasicBlockRef lastElseBlock = nullptr;

			// Emit code for the else block
			if (stmt->ifn.else_body) {
				elseBlock = LLVMAppendBasicBlock(func, "");
				LLVMPositionBuilderAtEnd(builder, elseBlock);

				// Generate LLVM IR code for the else_body
				traverseASTAndGenerateIR(stmt->ifn.else_body, module, builder, func, varMap, intType);

				// Get the last basic block created within the nested constructs of the elseBlock
				lastElseBlock = LLVMGetLastBasicBlock(func);
			}

			// Position the builder at the end of the block where the condition was evaluated
			LLVMPositionBuilderAtEnd(builder, insertionBlock);

			// Create a basic block for the exit block
			LLVMBasicBlockRef exitBlock = nullptr;

			// Get the result of the condition comparison
			LLVMValueRef cmp = traverseASTAndGenerateIR(stmt->ifn.cond, module, builder, func, varMap, intType);
		
			// Create a conditional branch based on the comparison result
			if (stmt->ifn.else_body) {
				LLVMBuildCondBr(builder, cmp, ifBlock, elseBlock);
				exitBlock = LLVMAppendBasicBlock(func, "");
			} else {
				// If there's no else body, branch to exitBlock directly
				exitBlock = LLVMAppendBasicBlock(func, "");
				LLVMBuildCondBr(builder, cmp, ifBlock, exitBlock);
			}

			// Position the builder at the end of the last basic block of the if_block
			LLVMPositionBuilderAtEnd(builder, lastIfBlock);

			// Unconditionally branch to the exit block
			LLVMBuildBr(builder, exitBlock);

			// Unconditionally branch to the exit block if there's an else body
			if (stmt->ifn.else_body) {
				// Position the builder at the end of the last basic block of the else_block
				LLVMPositionBuilderAtEnd(builder, lastElseBlock);

				// Unconditionally branch to the exit block
				LLVMBuildBr(builder, exitBlock);
			}

			// Emit code for the exit block
			LLVMPositionBuilderAtEnd(builder, exitBlock);

			break;
		}
		case ast_asgn: {
			// Generate LLVM IR code for the assignment statement
			LLVMValueRef rhsValue = traverseASTAndGenerateIR(stmt->asgn.rhs, module, builder, func, varMap, intType);
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

/**
 * Traverses the Abstract Syntax Tree (AST) and generates LLVM IR code for each node.
 * 
 * @param node      The current AST node being processed.
 * @param module    Reference to the LLVM module.
 * @param builder   Reference to the LLVM IR builder.
 * @param func      Reference to the current LLVM function being processed.
 * @param varMap    Reference to the map of variable names and their corresponding LLVM values.
 * @param intType   The LLVM integer type used in the generated code.
 * @return          Returns the LLVM value for the given AST node or nullptr if the node does not generate a value.
 */
static LLVMValueRef traverseASTAndGenerateIR(astNode *node, LLVMModuleRef &module, LLVMBuilderRef &builder, LLVMValueRef &func, unordered_map<string, LLVMValueRef> &varMap, LLVMTypeRef intType) {
    LLVMValueRef result = nullptr;
	switch(node->type) {
        case ast_prog: {
            // Traverse the AST nodes for external declarations and functions
            traverseASTAndGenerateIR(node->prog.ext1, module, builder, func, varMap, intType);
            traverseASTAndGenerateIR(node->prog.ext2, module, builder, func, varMap, intType);
            traverseASTAndGenerateIR(node->prog.func, module, builder, func, varMap, intType);
            break;
        }
        case ast_extern: {
            // Handle external declarations (print and read) for the program
            LLVMTypeRef externFuncType;

            if (!strcmp(node->ext.name, "print")) {
                LLVMTypeRef printParamTypes[] = { intType }; // One integer parameter 
                externFuncType = LLVMFunctionType(LLVMVoidType(), printParamTypes, 1, 0); // Return type is void
            } else if (!strcmp(node->ext.name, "read")) {
                externFuncType = LLVMFunctionType(intType, nullptr, 0, 0); // No parameters
            } 

            // Add the function to the module
            func = LLVMAddFunction(module, node->ext.name, externFuncType);
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
			LLVMBasicBlockRef entryBlock = LLVMAppendBasicBlock(func, "");

			// Create a builder to generate instructions with.
			LLVMPositionBuilderAtEnd(builder, entryBlock);

			// Create a variable for the func parameter and store it in the entry block
			LLVMValueRef var = LLVMBuildAlloca(builder, intType, node->func.param->var.name);
			LLVMSetAlignment(var, 4);
			LLVMBuildStore(builder, LLVMGetParam(func, 0), var);

			// Add the variable to the variable map
			varMap[node->func.param->var.name] = var;

			// Traverse the function body
			traverseASTAndGenerateIR(node->func.body, module, builder, func, varMap, intType);
			
			// Clear the variable map for the next function (NOT NECESSARY for MiniC)
			varMap.clear();
			
			break;
		}
		case ast_stmt: {
			// Handle statement
			result = traverseStmtAndGenerateIR(&node->stmt, module, builder, func, varMap, intType);
			break;
		}
		case ast_var: {
			// Load the value of a variable
			result = LLVMBuildLoad2(builder, intType, varMap[node->var.name], "");
			break;
		}
		case ast_cnst: {
			// Handle constant integer value
			result = LLVMConstInt(intType, node->cnst.value, 0);
			break;
		}
		case ast_rexpr: {
			// Handle relational expression
			LLVMValueRef lhs = traverseASTAndGenerateIR(node->rexpr.lhs, module, builder, func, varMap, intType);
			LLVMValueRef rhs = traverseASTAndGenerateIR(node->rexpr.rhs, module, builder, func, varMap, intType);

			// Compare the two values and return the result
			LLVMIntPredicate intPredicate = intPredicates[node->rexpr.op];  
			result = LLVMBuildICmp(builder, intPredicate, lhs, rhs, "");
			break;
		}
		case ast_bexpr: {
			// Handle binary expression
			LLVMValueRef lhs = traverseASTAndGenerateIR(node->bexpr.lhs, module, builder, func, varMap, intType);
			LLVMValueRef rhs = traverseASTAndGenerateIR(node->bexpr.rhs, module, builder, func, varMap, intType);

			// Perform the binary operation and return the result
			LLVMOpcode opcode = opcodes[node->bexpr.op];
			result = LLVMBuildBinOp(builder, opcode, lhs, rhs, "");
			break;
		}
		case ast_uexpr: {
			// Handle unary expression
			LLVMValueRef expr = traverseASTAndGenerateIR(node->uexpr.expr, module, builder, func, varMap, intType);

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


/**
 * @brief Changes the file extension of the given filename.
 *
 * This function takes the input filename and changes its extension to ".ll".
 * The modified filename is stored in the output parameter.
 *
 * @param[in] filename The input filename as a C-style string (const char*).
 * @param[out] output The modified filename with the new extension as a reference to a std::string.
 */
static void changeFileExtension(const char *filename, string &output) {
	// Convert the input C-style string filename to a C++ std::string
    std::string inputFilename(filename);

	// Find the position of the last dot in the filename, which represents the start of the file extension
    size_t dotPos = inputFilename.find_last_of('.');

	// Remove the file extension by taking the substring up to the dot position.
    output = inputFilename.substr(0, dotPos);

	// Append the new file extension ".ll" to the output filename
    output += "_manual.ll";
}

/**
 * Generates LLVM IR code from the given AST and saves it to a file with a '_manual.ll' extension.
 * 
 * @param node      The Abstract Syntax Tree (AST) node to generate LLVM IR code from.
 * @param filename  The input filename, used as the basis for the output file's name.
 * @return          Returns a pointer to the generated LLVM module, or nullptr if there was an error.
 */
LLVMModuleRef generateIRAndSaveToFile(astNode *node, const char *filename) {
    if (!node) {
        printf("Error: AST is empty\n");
        return nullptr;
    }

    // Create LLVM module, builder, int primitive type, and function
    LLVMModuleRef module = LLVMModuleCreateWithName(filename);
    LLVMSetTarget(module, "x86_64-pc-linux-gnu");
    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMTypeRef intType = LLVMInt32Type();
	LLVMValueRef func;

    // Initialize a map to store the value references of variables
    std::unordered_map<string, LLVMValueRef> varMap;

	// Traverse the AST to generate LLVM IR code
    traverseASTAndGenerateIR(node, module, builder, func, varMap, intType);

    // Verify the generated module
    if (LLVMVerifyModule(module, LLVMAbortProcessAction, nullptr)) {
        printf("Error: The module is not valid\n");
        return nullptr;
    }

    // Create a string to store the output filename
   	std::string outputFilename;

    // Call the changeFileExtension function and save the result in outputFilename
    changeFileExtension(filename, outputFilename);

    // Write the generated LLVM IR code to a file called outputFilename
    LLVMPrintModuleToFile(module, outputFilename.c_str(), nullptr);

	#ifdef DEBUG
	// Print the generated LLVM IR code to the console
	LLVMDumpModule(module);
	#endif
	

    // Cleanup
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);

    return module;
}