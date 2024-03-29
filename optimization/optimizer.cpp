/*
 * optimizer.cpp
 *
 * This file implements local and global optimizers for miniC language using LLVM
 *
 * The optimizer performs the following transformations:
 * 1. Constant folding: Replace arithmetic operations with constants of the operands result
 * 2. Dead code elimination: Remove instructions that have no uses and no side effects
 * 3. Common subexpression elimination: Replace multiple identical computations with a single computation
 * 4. Constant propagation: Replace load instructions with constants if all the stores that write to the
 * 							memory location being read have the same constant value.
 *
 * Usage: ./optimizer <input-file>
 *
 * Output: The optimized LLVM IR code is save to a file named <basename>_opt.ll in the same directory
 * 		   as the input file
 *
 * Author: Aimen Abdulaziz
 * Date: Spring 2023
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
#include "file_utils.h"

// C++ libraries
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iostream>
using namespace std;

/**
 * Checks if two LLVM instructions have the same operands.
 *
 * @param instruction1 The first instruction.
 * @param instruction2 The second instruction.
 * @return True if both instructions have the same operands, false otherwise.
 */
static bool
isSameOperands(LLVMValueRef instruction1, LLVMValueRef instruction2)
{
	int numberOfOperands1 = LLVMGetNumOperands(instruction1);
	int numberOfOperands2 = LLVMGetNumOperands(instruction2);

	if (numberOfOperands1 != numberOfOperands2)
	{
		return false;
	}

	for (int i = 0; i < numberOfOperands1; i++)
	{
		LLVMValueRef operand1 = LLVMGetOperand(instruction1, i);
		LLVMValueRef operand2 = LLVMGetOperand(instruction2, i);

#ifdef DEBUG
		printf("\nInstr1 - operand %d:\n", i);
		LLVMDumpValue(operand1);
		printf("\nInstr2 - operand %d:\n", i);
		LLVMDumpValue(operand2);
#endif

		// Compare the operands by checking if they have the same value and type
		if (operand1 != operand2 || LLVMTypeOf(operand1) != LLVMTypeOf(operand2))
		{
			return false;
		}
	}
	return true;
}

/**
 * Check if it is safe to replace the first Load instruction with the second Load instruction.
 * It is safe if there are no Store instructions between them that modify the pointer operand.
 *
 * @param instruction1 the first Load instruction
 * @param instruction2 the second Load instruction
 * @return true if it is safe to replace, false otherwise
 */
static bool
safeToReplaceLoadInstructions(LLVMValueRef instruction1, LLVMValueRef instruction2)
{
	// Get the pointer operand
	LLVMValueRef ptr1 = LLVMGetOperand(instruction1, 0);

	// Iterate over all the instructions between the two Load instructions
	// and check if any of them are Store instructions that modify the pointer operand
	LLVMValueRef currentInstr = instruction1;
	while ((currentInstr = LLVMGetNextInstruction(currentInstr)) && (currentInstr != instruction2))
	{
#ifdef DEBUG
		printf("\nCurrent Instruction:\n");
		LLVMDumpValue(currentInstr);
#endif
		if (LLVMIsAStoreInst(currentInstr))
		{
			// Check if the Store instruction has the same pointer as the operands
			LLVMValueRef storePtr = LLVMGetOperand(currentInstr, 1);
			if (storePtr == ptr1)
			{
// the operands has been modified
#ifdef DEBUG
				printf("\nPointer has been modified\n");
#endif
				return false;
			}
		}
	}
#ifdef DEBUG
	printf("\nPointer has not been modified\n");
#endif
	return true;
}

/**
 * Determines whether two instructions represent a common subexpression.
 *
 * Two instructions are considered a common subexpression if they have the same opcode and
 * operands. If both instructions are not load instructions, they are considered a common
 * subexpression. If one or both instructions are load instructions, they are considered
 * a common subexpression only if it is safe to replace one load with the other.
 *
 * @param instruction1 The first instruction.
 * @param instruction2 The second instruction.
 * @return true if the instructions represent a common subexpression, false otherwise.
 */
static bool
isCommonSubexpression(LLVMValueRef instruction1, LLVMValueRef instruction2)
{
	if (isSameOperands(instruction1, instruction2))
	{
		if (!LLVMIsALoadInst(instruction1) && !LLVMIsALoadInst(instruction2))
		{
			return true;
		}
		else if (safeToReplaceLoadInstructions(instruction1, instruction2))
		{
			return true;
		}
	}
	return false;
}

/**
 * Determines whether an LLVM instruction has any uses.
 *
 * @param instruction the instruction to check
 * @return true if the instruction has any uses, false otherwise
 */
static bool
hasUses(LLVMValueRef instruction)
{
	return LLVMGetFirstUse(instruction) != NULL;
}

/**
 * @brief Eliminate common subexpressions in an LLVM basic block.
 *
 * This function iterates over all the instructions in the basic block and keeps track
 * of each opcode and its corresponding instruction in a hashmap. For each instruction,
 * it checks if it is a common subexpression by iterating over all the previous
 * instructions with the same opcode. If it is a common subexpression, it replaces all
 * uses of the instruction with the previous instruction.
 *
 * @param basicBlock The LLVM basic block to perform common subexpression elimination on.
 * @return true if any common subexpression was eliminated, false otherwise.
 */
static bool
commonSubexpressionElimination(LLVMBasicBlockRef basicBlock)
{
	// Create an empty hashmap opcodeMap of vectors of instructions
	unordered_map<LLVMOpcode, vector<LLVMValueRef>> opcodeMap; // <opcode, vector of instructions>
	bool subExpressionEliminated = false;

	// Iterate over all the instructions in the basic block
	for (auto instruction = LLVMGetFirstInstruction(basicBlock);
		 instruction;
		 instruction = LLVMGetNextInstruction(instruction))
	{

		// Get the opcode of the instruction
		LLVMOpcode op = LLVMGetInstructionOpcode(instruction);

		// Skip alloca or useless instructions
		if (op == LLVMAlloca)
		{
			continue;
		}

		// Check if the instruction is already in the hashmap
		if (opcodeMap.find(op) == opcodeMap.end())
		{
			// If not, add it to the hashmap and initialize the vector
			opcodeMap[op] = vector<LLVMValueRef>();
		}

		// Check if the instruction is a common subexpression by iterating over all the
		// previous instructions with the same opcode
		for (auto prevInstruction : opcodeMap[op])
		{
			if (hasUses(prevInstruction) && isCommonSubexpression(prevInstruction, instruction))
			{
				// Replace all uses of the instruction with the previous instruction
				LLVMReplaceAllUsesWith(instruction, prevInstruction);
				subExpressionEliminated = true;

#ifdef DEBUG
				printf("\nReplaced instruction:\n");
				LLVMDumpValue(instruction);
				printf("\nwith instruction:\n");
				LLVMDumpValue(prevInstruction);
#endif

				break;
			}
		}

		opcodeMap[op].push_back(instruction);
	}

	return subExpressionEliminated;
}

/**
 * @brief Checks if removing the given instruction causes any side effects.
 *
 * This function checks if an LLVM instruction has side effects by determining
 * if it is a store instruction or a terminator instruction. These checks are enough
 * for miniC, but depending on the programming language, we might need more checks.
 *
 * @param instruction The LLVMValueRef representing the instruction to check for side effects.
 * @return Returns true if removing instruction has side effects, false otherwise.
 */
static bool
hasSideEffects(LLVMValueRef instruction)
{
	return LLVMIsAStoreInst(instruction) || LLVMIsATerminatorInst(instruction) || LLVMIsACallInst(instruction);
}

/**
 * @brief Deletes all instructions marked for deletion in the toDelete vector.
 *
 * This function iterates through the toDelete vector and erases the corresponding instructions
 * from their parent basic block.
 *
 * @param toDelete The vector that stores the instructions to be deleted.
 */
void deleteMarkedInstructions(vector<LLVMValueRef> &toDelete)
{
	// Now erase the marked instructions from the basic block
	for (auto instruction : toDelete)
	{
#ifdef DEBUG
		printf("\nDeleting instruction:\n");
		LLVMDumpValue(instruction);
		printf("\n");
#endif

		LLVMInstructionEraseFromParent(instruction);
	}
}

/**
 * @brief This function performs dead code elimination on a given basic block.
 *
 * It iterates over all instructions in the basic block, and if an instruction
 * has no uses and no side effects, it is marked for deletion. Afterwards, all
 * the marked instructions are erased from the basic block.
 *
 * @param basicBlock the basic block on which to perform dead code elimination
 * @return true if any instruction was deleted, false otherwise
 */
static bool
deadCodeElimination(LLVMBasicBlockRef basicBlock)
{
	vector<LLVMValueRef> toDelete;

	// Iterate over all the instructions in the basic block
	for (auto instruction = LLVMGetFirstInstruction(basicBlock);
		 instruction; instruction = LLVMGetNextInstruction(instruction))
	{

		// If the instruction has no uses and no side effects, mark it for deletion
		if (!hasUses(instruction) && !hasSideEffects(instruction))
		{
#ifdef DEBUG
			printf("\nMarking instruction for deletion:\n");
			LLVMDumpValue(instruction);
#endif
			toDelete.push_back(instruction);
		}
	}

	// Now erase the marked instructions from the basic block
	deleteMarkedInstructions(toDelete);

	return toDelete.size() > 0; // return true if any instruction was deleted
}

/**
 * Check whether an LLVM instruction is an arithmetic (+-*) or an icmp operation.
 *
 * @param instruction the LLVM instruction to check
 * @return true if the instruction is an arithmetic or an icmp operation, false otherwise
 */
static bool
isArithmeticOrIcmpOperation(LLVMValueRef instruction)
{
	LLVMOpcode op = LLVMGetInstructionOpcode(instruction);
	return (op == LLVMAdd || op == LLVMSub || op == LLVMMul || op == LLVMICmp);
}

/**
 * Checks if all operands of the given LLVM instruction are constant integers.
 *
 * @param instruction The LLVM instruction to check.
 * @return True if all operands are constant integers, false otherwise.
 */
static bool
allOperandsAreConstant(LLVMValueRef instruction)
{
	int numberOfOperands = LLVMGetNumOperands(instruction);

	for (int i = 0; i < numberOfOperands; i++)
	{
		LLVMValueRef operand = LLVMGetOperand(instruction, i);
		if (!LLVMIsAConstantInt(operand))
		{
			return false;
		}
	}

#ifdef DEBUG
	printf("\nAll operands for the following instruction are constants:\n");
	LLVMDumpValue(instruction);
	printf("\n");
#endif

	return true;
}

/**
 * Computes and returns the folded constant for a given arithmetic or icmp instruction.
 *
 * @param instruction The instruction to compute the folded constant for.
 * @return The folded constant as a LLVMValueRef, or NULL if the instruction cannot be folded.
 */
static LLVMValueRef
computeFoldedConstant(LLVMValueRef instruction)
{
	// miniC always has two operands for arithmetic operations
	LLVMValueRef operand1 = LLVMGetOperand(instruction, 0);
	LLVMValueRef operand2 = LLVMGetOperand(instruction, 1);

#ifdef DEBUG
	printf("Computing constant arithmetic for\n");
	LLVMDumpValue(instruction);
	printf("\n");
#endif

	LLVMOpcode op = LLVMGetInstructionOpcode(instruction);

	if (op == LLVMAdd)
	{
		return LLVMConstAdd(operand1, operand2);
	}
	else if (op == LLVMSub)
	{
		return LLVMConstSub(operand1, operand2);
	}
	else if (op == LLVMMul)
	{
		return LLVMConstMul(operand1, operand2);
	}
	else if (op == LLVMICmp)
	{
		LLVMIntPredicate predicate = LLVMGetICmpPredicate(instruction);
		return LLVMConstICmp(predicate, operand1, operand2);
	}
	return NULL;
}

/**
 * Applies constant folding optimization to all arithmetic or icmp instructions in a given basic block where
 * all operands are constants.
 *
 * @param basicBlock the basic block to apply constant folding to.
 * @return true if any constant folding occurred in the basic block, false otherwise.
 */
static bool
constantFolding(LLVMBasicBlockRef basicBlock)
{
	bool codeChanged = false;
	// Iterate over all the instructions in the basic block
	for (auto instruction = LLVMGetFirstInstruction(basicBlock);
		 instruction;
		 instruction = LLVMGetNextInstruction(instruction))
	{

		// If the instruction is an arithmetic operation and all its operands are constants,
		if (isArithmeticOrIcmpOperation(instruction) && allOperandsAreConstant(instruction))
		{
			LLVMValueRef foldedConstant = computeFoldedConstant(instruction);
			LLVMReplaceAllUsesWith(instruction, foldedConstant);
			codeChanged = true;

#ifdef DEBUG
			printf("Folded constant\n");
			LLVMDumpValue(foldedConstant);
			printf("\n");
#endif
		}
	}
	return codeChanged;
}

/**
 * Builds a map of all store instructions that write to the same memory address.
 * @param function The LLVM function for which the store instructions map needs to be built.
 * @return An unordered map of store instructions map, where the key is the memory address being stored
 *         and the value is a vector of all store instructions that write to that memory address.
 */
static unordered_map<LLVMValueRef, vector<LLVMValueRef>>
buildStoreInstructionsMap(LLVMValueRef function)
{
	// <ptr for storing, vector of other store instruction that store in the same ptr>
	unordered_map<LLVMValueRef, vector<LLVMValueRef>> storeInstructionsMap;
	for (auto basicBlock = LLVMGetFirstBasicBlock(function);
		 basicBlock;
		 basicBlock = LLVMGetNextBasicBlock(basicBlock))
	{
		for (auto instruction = LLVMGetFirstInstruction(basicBlock);
			 instruction;
			 instruction = LLVMGetNextInstruction(instruction))
		{

			// Skip non-store instructions
			if (!LLVMIsAStoreInst(instruction))
			{
				continue;
			}

			LLVMValueRef storePtr = LLVMGetOperand(instruction, 1);

			// If the storePtr is not already in storeInstructionsMap, add it with an empty vector
			if (storeInstructionsMap.find(storePtr) == storeInstructionsMap.end())
			{
				storeInstructionsMap[storePtr] = vector<LLVMValueRef>();
			}

			// Add the instruction to the corresponding vector in storeInstructionsMap
			storeInstructionsMap[storePtr].push_back(instruction);
		}
	}

	return storeInstructionsMap;
}

/**
 * @brief Builds kill and gen set maps for a given LLVM function leveraging the store instructions map.
 *
 * This function iterates through all basic blocks and instructions in the given LLVM function,
 * and populates the kill and gen set maps for each basic block. The kill set map contains
 * store instructions that are killed by other store instructions in the basic block, while
 * the gen set map contains the store instructions that are generated in the basic block.
 *
 * @param function The LLVM function for which kill and gen set maps are to be built.
 * @param storeInstructionsMap The map of store instructions affecting the same pointer.
 * @param killSetMap Pointer to the map that will store the kill sets for each basic block.
 * @param genSetMap Pointer to the map that will store the gen sets for each basic block.
 */
static void
buildKillNGenSetMaps(
	LLVMValueRef function,
	unordered_map<LLVMValueRef, vector<LLVMValueRef>> storeInstructionsMap,
	unordered_map<LLVMBasicBlockRef, unordered_set<LLVMValueRef>> &killSetMap,
	unordered_map<LLVMBasicBlockRef, unordered_set<LLVMValueRef>> &genSetMap)
{

	// Iterate through all basic blocks in the function.
	for (auto basicBlock = LLVMGetFirstBasicBlock(function);
		 basicBlock;
		 basicBlock = LLVMGetNextBasicBlock(basicBlock))
	{

		// Iterate through all instructions in the basic block.
		for (auto instruction = LLVMGetFirstInstruction(basicBlock);
			 instruction;
			 instruction = LLVMGetNextInstruction(instruction))
		{

			// Initialize empty kill and gen sets for basic blocks not already in the maps.
			if (killSetMap.find(basicBlock) == killSetMap.end())
			{
				killSetMap[basicBlock] = unordered_set<LLVMValueRef>();
			}
			if (genSetMap.find(basicBlock) == genSetMap.end())
			{
				genSetMap[basicBlock] = unordered_set<LLVMValueRef>();
			}

			// Skip non-store instructions.
			if (!LLVMIsAStoreInst(instruction))
			{
				continue;
			}

			LLVMValueRef storePtr = LLVMGetOperand(instruction, 1);

			// Add the current store instruction to the gen set for the basic block.
			genSetMap[basicBlock].insert(instruction);

			// Iterate over the vector of store instructions that modify the same pointer.
			for (auto storedInstr : storeInstructionsMap[storePtr])
			{

				// Add instructions to the kill set if they are different from the current instruction.
				if (storedInstr != instruction)
				{
					killSetMap[basicBlock].insert(storedInstr);
				}

				// If there is any instruction killed by adding the current instruction, remove it from the GEN set.
				if (genSetMap[basicBlock].find(storedInstr) != genSetMap[basicBlock].end() && storedInstr != instruction)
				{
					genSetMap[basicBlock].erase(storedInstr);

#ifdef DEBUG
					printf("\nRemoved instruction from GEN set:\n");
					LLVMDumpValue(storedInstr);
					printf("\n was killed by instruction:\n");
					LLVMDumpValue(instruction);
					printf("\n");
#endif
				}
			}
		}
	}
}

/**
 * Builds a map of all the predecessors of each basic block in a given function.
 *
 * @param function LLVMValueRef for the function to build the predecessor map for
 * @return unordered_map of LLVMBasicBlockRef to vector of LLVMBasicBlockRef,
 *         where the vector contains all predecessors of a basic block
 */
static unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>>
buildPredMap(LLVMValueRef function)
{
	// Initialize the predecessor map
	unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>> predMap;
	for (auto basicBlock = LLVMGetFirstBasicBlock(function);
		 basicBlock;
		 basicBlock = LLVMGetNextBasicBlock(basicBlock))
	{

		LLVMValueRef bbTerminator = LLVMGetBasicBlockTerminator(basicBlock);

		// Get the number of successors of the basic block
		unsigned numSuccessors = LLVMGetNumSuccessors(bbTerminator);

		// Iterate over all the successors of the basic block
		for (auto i = 0; i < numSuccessors; i++)
		{
			LLVMBasicBlockRef successor = LLVMGetSuccessor(bbTerminator, i);

			// Add the successor to the predecessor map
			predMap[successor].push_back(basicBlock);
		}
	}

// Print the predecessor map
#ifdef DEBUG
	printf("\nPredecessor map:\n");
	for (auto basicBlock = LLVMGetFirstBasicBlock(function);
		 basicBlock;
		 basicBlock = LLVMGetNextBasicBlock(basicBlock))
	{
		printf("\nBasic Block:\n");
		LLVMDumpValue(LLVMBasicBlockAsValue(basicBlock));
		printf("\nPredecessors:\n");
		for (auto predBlock : predMap[basicBlock])
		{
			LLVMDumpValue(LLVMBasicBlockAsValue(predBlock));
			printf("\n");
		}
	}
#endif

	return predMap;
}

/**
 * Given a basic block B, the map of predecessors to B, and the OUT sets of all basic blocks,
 * this function returns the union of all predecessors' OUT sets.
 *
 * @param basicBlock: The basic block whose predecessors' OUT sets are to be unioned
 * @param predMap: The map of predecessors to basicBlock
 * @param outSetMap: The map of basic block to its OUT set
 * @return The union of all predecessors' OUT sets
 */
static unordered_set<LLVMValueRef>
findUnionOfAllPredOuts(LLVMBasicBlockRef basicBlock,
					   unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>> predMap,
					   unordered_map<LLVMBasicBlockRef, unordered_set<LLVMValueRef>> outSetMap)
{

	// Initialize the union of all predecessors' OUT sets
	unordered_set<LLVMValueRef> unionOfAllPredOuts;

	// Iterate over all the predecessors of the basic block
	for (auto predBlock : predMap[basicBlock])
	{
		// Find the OUT set for the predecessor
		unordered_set<LLVMValueRef> predOutSet = outSetMap[predBlock];

		// Add the OUT set of the predecessor to the union of all predecessors' OUT sets
		unionOfAllPredOuts.insert(predOutSet.begin(), predOutSet.end());
	}
	return unionOfAllPredOuts;
}

/**
 * @brief Finds the union of (IN - KILL) and GEN sets for a basic block in a data flow analysis.
 *
 * This function finds the union of (IN - KILL) and GEN sets for a given basic block by
 * first checking if the IN set is empty. If it is empty, the function returns the GEN set.
 * If not, it computes the set difference between the IN and KILL sets and adds the
 * resulting set to the GEN set.
 *
 * @param basicBlock The basic block for which the union of IN and GEN sets is to be found.
 * @param inSetMap A map of IN sets for all basic blocks in the function.
 * @param killSetMap A map of KILL sets for all basic blocks in the function.
 * @param genSetMap A map of GEN sets for all basic blocks in the function.
 *
 * @return The union of IN and GEN sets for the given basic block.
 */
static unordered_set<LLVMValueRef>
findUnionOfInAndGen(LLVMBasicBlockRef basicBlock,
					unordered_map<LLVMBasicBlockRef, unordered_set<LLVMValueRef>> inSetMap,
					unordered_map<LLVMBasicBlockRef, unordered_set<LLVMValueRef>> killSetMap,
					unordered_map<LLVMBasicBlockRef, unordered_set<LLVMValueRef>> genSetMap)
{

	// If IN[B] is empty, return GEN[B]
	if (inSetMap[basicBlock].empty())
	{
		return genSetMap[basicBlock];
	}

	// Find (IN[B] - KILL[B])
	unordered_set<LLVMValueRef> result;
	for (auto instruction : inSetMap[basicBlock])
	{
		// Only add instruction NOT present in KILL[B]
		if (killSetMap[basicBlock].find(instruction) == killSetMap[basicBlock].end())
		{
			result.insert(instruction);
		}
	}

	// Find (IN[B] - KILL[B]) U GEN[B]
	result.insert(genSetMap[basicBlock].begin(), genSetMap[basicBlock].end());

	return result;
}

/**
 * @brief Prints the IN and OUT sets for each basic block in the given LLVM function.
 *
 * @param function The LLVM function for which the IN and OUT sets are to be printed.
 * @param outSetMap The OUT set map for each basic block.
 * @param inSetMap The IN set map for each basic block.
 */
static void
printOutNInMaps(LLVMValueRef function,
				unordered_map<LLVMBasicBlockRef, unordered_set<LLVMValueRef>> outSetMap,
				unordered_map<LLVMBasicBlockRef, unordered_set<LLVMValueRef>> inSetMap)
{

	for (auto basicBlock = LLVMGetFirstBasicBlock(function);
		 basicBlock;
		 basicBlock = LLVMGetNextBasicBlock(basicBlock))
	{
		fprintf(stderr, "\nBasic Block:\n");
		LLVMDumpValue(LLVMBasicBlockAsValue(basicBlock));
		fprintf(stderr, "\nIN set:\n");
		for (auto instruction : inSetMap[basicBlock])
		{
			LLVMDumpValue(instruction);
			printf("\n");
		}
		fprintf(stderr, "\nOUT set:\n");
		for (auto instruction : outSetMap[basicBlock])
		{
			LLVMDumpValue(instruction);
			fprintf(stderr, "\n");
		}
	}
}

/**
 * @brief Builds the IN and OUT sets for each basic block in a given LLVM function.
 *
 * This function initializes the IN and OUT sets for each basic block in the function.
 * It then iteratively computes the IN and OUT sets for each basic block using the kill
 * and gen sets of the basic blocks and the IN and OUT sets of its predecessors.
 *
 * @param function The LLVM function for which IN and OUT sets are to be built.
 * @param predMap A map containing the predecessors of each basic block in the function.
 * @param killSetMap A map containing the kill sets for each basic block in the function.
 * @param genSetMap A map containing the gen sets for each basic block in the function.
 * @param inSetMap A map containing the IN sets for each basic block in the function.
 * @param outSetMap A map containing the OUT sets for each basic block in the function.
 */
static void buildInNOutSets(
	LLVMValueRef function,
	unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>> &predMap,
	unordered_map<LLVMBasicBlockRef, unordered_set<LLVMValueRef>> &killSetMap,
	unordered_map<LLVMBasicBlockRef, unordered_set<LLVMValueRef>> &genSetMap,
	unordered_map<LLVMBasicBlockRef, unordered_set<LLVMValueRef>> &inSetMap,
	unordered_map<LLVMBasicBlockRef, unordered_set<LLVMValueRef>> &outSetMap)
{
	// Initialize the IN and OUT sets for each basic block
	for (auto basicBlock = LLVMGetFirstBasicBlock(function);
		 basicBlock;
		 basicBlock = LLVMGetNextBasicBlock(basicBlock))
	{

		// Initialize the IN and OUT sets for each basic block
		inSetMap[basicBlock] = unordered_set<LLVMValueRef>();
		outSetMap[basicBlock] = genSetMap[basicBlock];
	}

	// IN and OUT sets
	bool codeChanged = true;

#ifdef DEBUG
	fprintf(stderr, "\nInitial IN and OUT sets:\n");
	printOutNInMaps(function, outSetMap, inSetMap);
	int iteration = 0;
#endif

	while (codeChanged)
	{
		codeChanged = false;

		for (auto basicBlock = LLVMGetFirstBasicBlock(function);
			 basicBlock;
			 basicBlock = LLVMGetNextBasicBlock(basicBlock))
		{

			// IN[B] = U OUT[P] for all P in pred(B)
			inSetMap[basicBlock] = findUnionOfAllPredOuts(basicBlock, predMap, outSetMap);

			// OUT[B] = (IN[B] - KILL[B]) U GEN[B]
			unordered_set<LLVMValueRef> newOut = findUnionOfInAndGen(basicBlock, inSetMap, killSetMap, genSetMap);

			// Check if the OUT set has changed
			if (outSetMap[basicBlock] != newOut)
			{
				codeChanged = true;
			}

			outSetMap[basicBlock] = newOut;
		}

#ifdef DEBUG
		fprintf(stderr, "\nIteration %d:\n", iteration);
		printOutNInMaps(function, outSetMap, inSetMap);
		iteration += 1;
#endif
	}
}

/**
 * @brief Checks if all store instructions write the same constant value.
 *
 * This function takes a vector of store instructions and checks if all store instructions write
 * the same constant value into memory. It returns true if all store instructions write the same
 * constant value, and false otherwise.
 *
 * @param allStoresForLoadPtr A vector of store instructions writing to the same memory location.
 * @return True if all store instructions write the same constant value, false otherwise.
 */
static bool
allStoreWriteSameConstant(vector<LLVMValueRef> allStoresForLoadPtr)
{
// Print all the store instructions
#ifdef DEBUG
	printf("\nAll store instructions:\n");
	for (auto storeInstr : allStoresForLoadPtr)
	{
		LLVMDumpValue(storeInstr);
		printf("\n");
	}
#endif

	// Get the constant value written by the first store instruction
	LLVMValueRef firstStoreVal = LLVMGetOperand(allStoresForLoadPtr[0], 0);

	if (!LLVMIsAConstantInt(firstStoreVal))
	{
		return false;
	}

	// Iterate over all the store instructions that write to loadPtr
	for (auto storeInstr : allStoresForLoadPtr)
	{
		// Return false if it is not a constant store instruction or does not write the same constant value
		LLVMValueRef storeVal = LLVMGetOperand(storeInstr, 0);
		if (!LLVMIsAConstantInt(storeVal) || LLVMGetOperand(storeInstr, 0) != firstStoreVal)
		{
			return false;
		}
	}

	// All store instructions write the same constant value
	return true;
}

/**
 * @brief Processes a store instruction, removing any instructions in R that will be killed by the current store instruction.
 *
 * Given a store instruction, this function removes all the instructions in R (the in-set of the current basic block)
 * that will be killed by the current instruction.
 * The store instruction is then added to R (in-set) for future processing.
 *
 * @param instruction The LLVM store instruction to be processed.
 * @param storeInstructionsMap The map of store instructions affecting the same pointer.
 * @param R The in-set for the current basic block.
 */
static void
processStoreInstruction(LLVMValueRef instruction,
						unordered_map<LLVMValueRef, vector<LLVMValueRef>> storeInstructionsMap,
						unordered_set<LLVMValueRef> &R)
{

	// Remove all the instruction in R that will be killed by the current instruction
	LLVMValueRef storePtr = LLVMGetOperand(instruction, 1);

	for (auto killedInstr : storeInstructionsMap[storePtr])
	{
		R.erase(killedInstr);

#ifdef DEBUG
		printf("Instruction(s) killed by the current store instruction\n");
		LLVMDumpValue(killedInstr);
		printf("\n");
#endif
	}

	R.insert(instruction);
}

/**
 * @brief Processes a given load instruction, marking the instruction for deletion if possible.
 *
 * This function processes the given load instruction, and identifies all the store instructions in the IN set map
 * that write to the same memory address as the load instruction. If all these store instructions write the same
 * constant value into memory, the function replaces all uses of the load instruction with the constant value in the
 * store instruction and marks the load instruction for deletion.
 *
 * @param instruction The LLVM load instruction to be processed.
 * @param storeInstructionsMap The map of store instructions affecting the same pointer.
 * @param R The in set of the basic block in which the instruction resides.
 * @param toDelete A vector containing LLVM instructions to be deleted from the basic block.
 */

static void
processLoadInstruction(LLVMValueRef instruction,
					   unordered_map<LLVMValueRef, vector<LLVMValueRef>> storeInstructionsMap,
					   unordered_set<LLVMValueRef> R, vector<LLVMValueRef> &toDelete)
{
#ifdef DEBUG
	printf("Load instruction: \n");
	LLVMDumpValue(instruction);
	printf("\n");
#endif

	LLVMValueRef loadPtr = LLVMGetOperand(instruction, 0);

	// Find all the store instructions in R that write to loadPtr
	vector<LLVMValueRef> allStoresForLoadPtr;
	for (auto strInstr : storeInstructionsMap[loadPtr])
	{
		if (R.find(strInstr) != R.end())
		{
			allStoresForLoadPtr.push_back(strInstr);
		}
	}

	// If all these store instructions are constant store instructions and write
	// the same constant value into memory
	if (allStoreWriteSameConstant(allStoresForLoadPtr))
	{
		// replaces all uses of the load instruction by the constant in the store instruction
		LLVMReplaceAllUsesWith(instruction, LLVMGetOperand(allStoresForLoadPtr[0], 0));
		toDelete.push_back(instruction);

#ifdef DEBUG
		printf("\nReplaced instruction:\n");
		LLVMDumpValue(instruction);
		printf("\nwith instruction:\n");
		LLVMDumpValue(LLVMGetOperand(allStoresForLoadPtr[0], 0));
		printf("\n");
#endif
	}
	else
	{
#ifdef DEBUG
		printf("Not all store instructions write the same constant value\n");
#endif
	}
}

/**
 * @brief Processes all basic blocks in the given LLVM function using the provided storeInstructionsMap and inSetMap.
 *
 * This function iterates through all basic blocks and instructions in the given LLVM function,
 * processes each instruction and updates the in and out sets of each basic block. Instructions marked
 * for deletion are stored in the toDelete vector.
 *
 * @param function The LLVM function for which the basic blocks are to be processed.
 * @param storeInstructionsMap The map of store instructions affecting the same pointer.
 * @param inSetMap The map of in sets for each basic block.
 * @param toDelete The vector that will store instructions marked for deletion.
 */
static void
processBasicBlocks(LLVMValueRef function,
				   unordered_map<LLVMValueRef, vector<LLVMValueRef>> storeInstructionsMap,
				   unordered_map<LLVMBasicBlockRef, unordered_set<LLVMValueRef>> inSetMap,
				   vector<LLVMValueRef> &toDelete)
{

	// Iterate over all the basic blocks in the function
	for (auto basicBlock = LLVMGetFirstBasicBlock(function);
		 basicBlock;
		 basicBlock = LLVMGetNextBasicBlock(basicBlock))
	{

		unordered_set<LLVMValueRef> R = inSetMap[basicBlock];

		// Iterate over all the instructions in the basic block
		for (auto instruction = LLVMGetFirstInstruction(basicBlock);
			 instruction;
			 instruction = LLVMGetNextInstruction(instruction))
		{

#ifdef DEBUG
			printf("Current store instruction: \n");
			LLVMDumpValue(instruction);
			printf("\n");
#endif

			if (LLVMIsAStoreInst(instruction))
			{
				processStoreInstruction(instruction, storeInstructionsMap, R);
			}
			else if (LLVMIsALoadInst(instruction))
			{
				processLoadInstruction(instruction, storeInstructionsMap, R, toDelete);
			}
		}
	}
}

/**
 * @brief Performs constant propagation on the given function.
 *
 * It replaces load instructions with constants if all the stores
 * that write to the memory location being read have the same constant value.
 *
 * @param function The LLVM function to optimize
 *
 * @return True if any instruction was deleted, False otherwise
 */
static bool
constantPropagation(LLVMValueRef function)
{
	// Build a map of store instructions
	unordered_map<LLVMValueRef, vector<LLVMValueRef>> storeInstructionsMap = buildStoreInstructionsMap(function);

	// Declare KILL and GEN Maps and populate them
	unordered_map<LLVMBasicBlockRef, unordered_set<LLVMValueRef>> killSetMap;
	unordered_map<LLVMBasicBlockRef, unordered_set<LLVMValueRef>> genSetMap;
	buildKillNGenSetMaps(function, storeInstructionsMap, killSetMap, genSetMap);

	// Build a map of all the predecessors of each basic block
	unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>> predMap = buildPredMap(function);

	// Initialize the IN and OUT sets for each basic block
	unordered_map<LLVMBasicBlockRef, unordered_set<LLVMValueRef>> inSetMap;
	unordered_map<LLVMBasicBlockRef, unordered_set<LLVMValueRef>> outSetMap;

	buildInNOutSets(function, predMap, killSetMap, genSetMap, inSetMap, outSetMap);

	vector<LLVMValueRef> toDelete;

	// Iterate over all the basic blocks and instructions to perform constant propagation
	processBasicBlocks(function, storeInstructionsMap, inSetMap, toDelete);

	// Now erase the marked instructions from the basic block
	deleteMarkedInstructions(toDelete);

	return toDelete.size() > 0; // return true if any instruction was deleted
}

/**
 * @brief Optimizes a single LLVM function using various optimization techniques.
 *
 * This function performs multiple optimizations on the given LLVM function, including
 * constant propagation, constant folding, common subexpression elimination, and
 * dead code elimination. Optimizations are applied iteratively until no more changes
 * are made to the function.
 *
 * @param function The LLVM function to be optimized.
 */
void optimizeFunction(LLVMValueRef function)
{
	bool codeChanged = true;

	while (codeChanged)
	{
		// Reset codeChanged to false before applying optimizations
		codeChanged = false;

		// perform global optimization
		codeChanged = constantPropagation(function) || codeChanged;
#ifdef DEBUG
		printf("\nConstant propagation: %d\n", codeChanged);
		printf("______________________________________\n");
#endif

		for (auto basicBlock = LLVMGetFirstBasicBlock(function);
			 basicBlock;
			 basicBlock = LLVMGetNextBasicBlock(basicBlock))
		{

			// call local optimization functions
			codeChanged = constantFolding(basicBlock) || codeChanged;
#ifdef DEBUG
			printf("\nConstant folding: %d\n", codeChanged);
			printf("______________________________________\n");
#endif

			codeChanged = commonSubexpressionElimination(basicBlock) || codeChanged;
#ifdef DEBUG
			printf("\nCommon expression: %d\n", codeChanged);
			printf("______________________________________\n");
#endif

			codeChanged = deadCodeElimination(basicBlock) || codeChanged;
#ifdef DEBUG
			printf("\nDead code: %d\n", codeChanged);
			printf("______________________________________\n");
#endif
		}
	}
}

/**
 * @brief Optimizes the entire program (LLVM module) by optimizing each function within.
 *
 * This function iterates through all functions in the provided LLVM module
 * and optimizes each of them by calling the optimizeFunction function.
 *
 * @param module The LLVM module representing the program to be optimized.
 */
void optimizeProgram(LLVMModuleRef module)
{
	// Iterate through all functions in the module
	for (auto function = LLVMGetFirstFunction(module);
		 function;
		 function = LLVMGetNextFunction(function))
	{

		const char *funcName = LLVMGetValueName(function);

#ifdef DEBUG
		printf("Function Name: %s\n", funcName);
#endif

		// Optimize the current function
		optimizeFunction(function);
	}
}

/**
 * @brief The main function of the program.
 *
 * This function is the entry point of the program and is called when the program is executed.
 * It takes command line arguments as input and typically returns an integer value indicating
 * the success or failure of the program execution.
 *
 * @param argc The number of arguments passed to the program.
 * @param argv An array of strings containing the arguments passed to the program.
 * @return An integer value indicating the success or failure of the program execution.
 */
int main(int argc, char **argv)
{
	// Check the number of arguments
	if (argc != 2)
	{
		cout << "Usage: " << argv[0] << " <filename.ll>" << endl;
		return 1;
	}

	// Create LLVM module from IR file
	LLVMModuleRef mod = createLLVMModel(argv[1]);

	// Check if module is valid
	if (mod == NULL)
	{
		cout << "Error: Invalid LLVM IR file" << endl;
		return 2;
	}

	// Optimize the program
	optimizeProgram(mod);

	// Create a string to store the output filename
	std::string outputFilename;

	// Append `_opt` to the basename of the input file and save it in outputFilename
	changeFileExtension(argv[1], outputFilename, "_opt.ll");

	// Writes the LLVM IR to a file named "test.ll"
	LLVMPrintModuleToFile(mod, outputFilename.c_str(), NULL);

	return 0;
}
