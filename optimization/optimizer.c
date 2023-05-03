#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>

// C++ libraries
#include <unordered_map>
#include <unordered_set>
#include <vector>
using namespace std;

#define prt(x) if(x) { printf("%s\n", x); }

LLVMModuleRef createLLVMModel(char *filename){
	char *err = 0;

	LLVMMemoryBufferRef ll_f = 0;
	LLVMModuleRef m = 0;

	LLVMCreateMemoryBufferWithContentsOfFile(filename, &ll_f, &err);

	if (err != NULL) { 
		prt(err);
		return NULL;
	}
	
	LLVMParseIRInContext(LLVMGetGlobalContext(), ll_f, &m, &err);

	if (err != NULL) {
		prt(err);
	}

	return m;
}

static bool isSameOperands(LLVMValueRef instruction1, LLVMValueRef instruction2) {
	int numberOfOperands1 = LLVMGetNumOperands(instruction1);
	int numberOfOperands2 = LLVMGetNumOperands(instruction2);

	if (numberOfOperands1 != numberOfOperands2) {
		return false;
	}

	for (int i = 0; i < numberOfOperands1; i++) {
		LLVMValueRef operand1 = LLVMGetOperand(instruction1, i);
		LLVMValueRef operand2 = LLVMGetOperand(instruction2, i);

		#ifdef DEBUG
		printf("\nInstr1 - operand %d:\n", i);
		LLVMDumpValue(operand1);
		printf("\nInstr2 - operand %d:\n", i);
		LLVMDumpValue(operand2);
		#endif

		// Compare the operands by checking if they have the same value and type
		if (operand1 != operand2 || LLVMTypeOf(operand1) != LLVMTypeOf(operand2)) {
			return false;
		}
	}
	return true;
}

/* Safety Check to see if the memory address of two Load instructions has been modified in between */
static bool safeToReplaceLoadInstructions(LLVMValueRef instruction1, LLVMValueRef instruction2) {
	// Get the pointer operand 
	LLVMValueRef ptr1 = LLVMGetOperand(instruction1, 0);

	// Iterate over all the instructions between the two Load instructions
	// and check if any of them are Store instructions that modify the pointer operand
	LLVMValueRef currentInstr = instruction1;
	while((currentInstr = LLVMGetNextInstruction(currentInstr)) && (currentInstr != instruction2)) {
		#ifdef DEBUG
		printf("\nCurrent Instruction:\n");
		LLVMDumpValue(currentInstr);
		#endif
		if (LLVMIsAStoreInst(currentInstr)) {
			// Check if the Store instruction has the same pointer as the operands
			LLVMValueRef storePtr = LLVMGetOperand(currentInstr, 1);
			if (storePtr == ptr1) {
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

static bool isCommonSubexpression(LLVMValueRef instruction1, LLVMValueRef instruction2) {
	if (isSameOperands(instruction1, instruction2)) {
		if (!LLVMIsALoadInst(instruction1) && !LLVMIsALoadInst(instruction2)) {
			return true;
		} else if (safeToReplaceLoadInstructions(instruction1, instruction2)) {
			return true;
		}
	}
	return false;

}

// Helper function to check if an instruction has any uses 
static bool hasUses(LLVMValueRef instruction) {
	return LLVMGetFirstUse(instruction) != NULL;
}

static bool commonSubexpressionElimination(LLVMBasicBlockRef basicBlock) {
	// Create an empty hashmap opcodeMap of vectors of instructions
	unordered_map<LLVMOpcode, vector<LLVMValueRef>> opcodeMap; // <opcode, vector of instructions>
	bool subExpressionEliminated = false;

	// Iterate over all the instructions in the basic block
	for (LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock); 
		instruction; instruction = LLVMGetNextInstruction(instruction)) {

		// Get the opcode of the instruction
		LLVMOpcode op = LLVMGetInstructionOpcode(instruction);
		
		// Skip alloca or useless instructions
		if (op == LLVMAlloca) {
			continue;
		}

		// Check if the instruction is already in the hashmap
		if (opcodeMap.find(op) == opcodeMap.end()) {
			// If not, add it to the hashmap and initialize the vector
			opcodeMap[op] = vector<LLVMValueRef>();
		} 

		// Check if the instruction is a common subexpression by iterating over all the 
		// previous instructions with the same opcode
		for (LLVMValueRef prevInstruction : opcodeMap[op]) {
			#ifdef DEBUG
			printf("\nChecking...\n");
			LLVMDumpValue(instruction);
			if (!hasUses(prevInstruction)) {
				printf("\nInstruction has no uses:\n");
			}
			else {	
				printf("\nwith...\n"); 
			}
			LLVMDumpValue(prevInstruction);
			#endif

		if (hasUses(prevInstruction) && isCommonSubexpression(prevInstruction, instruction)) {
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
 * @brief Checks if removing the given instruction causes any side effect.
 *
 * This function checks if an LLVM instruction has side effects by determining 
 * if it is a store instruction or a terminator instruction. These checks are enough
 * for miniC, but depending on the programming language, we might need more checks.
 *
 * @param instruction The LLVMValueRef representing the instruction to check for side effects.
 * @return Returns true if the instruction has side effects, false otherwise.
 */
static bool hasSideEffects(LLVMValueRef instruction) {
	return LLVMIsAStoreInst(instruction) || LLVMIsATerminatorInst(instruction);
}

// Function to perform dead code elimination in a basic block
static bool deadCodeElimination(LLVMBasicBlockRef basicBlock) {
	std::vector<LLVMValueRef> toDelete;

  	// Iterate over all the instructions in the basic block
	for (LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock);
		instruction; instruction = LLVMGetNextInstruction(instruction)) {
		
		// If the instruction has no uses and no side effects, mark it for deletion
		if (!hasUses(instruction) && !hasSideEffects(instruction)) {
			#ifdef DEBUG
			printf("\nMarking instruction for deletion:\n");
			LLVMDumpValue(instruction);
			#endif
			toDelete.push_back(instruction);
		}
	}

	// Now erase the marked instructions from the basic block
	for (LLVMValueRef instruction : toDelete) {
		#ifdef DEBUG
		printf("\nDeleting instruction:\n");
		LLVMDumpValue(instruction);
		#endif
		LLVMInstructionEraseFromParent(instruction);
	}

	return toDelete.size() > 0; // return true if any instruction was deleted
}

static bool isArithmeticOperation(LLVMValueRef instruction) {
	LLVMOpcode op = LLVMGetInstructionOpcode(instruction);
	return (op == LLVMAdd || op == LLVMSub || op == LLVMMul);
}

static bool allOperandsAreConstant(LLVMValueRef instruction) {
	int numberOfOperands = LLVMGetNumOperands(instruction);

	for (int i = 0; i < numberOfOperands; i++){
		LLVMValueRef operand = LLVMGetOperand(instruction, i);
		if (!LLVMIsAConstantInt(operand)) {
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

static LLVMValueRef computeConstantArithmetic(LLVMValueRef instruction) {
	// miniC always has two operands for arithmetic operations
	LLVMValueRef operand1 = LLVMGetOperand(instruction, 0);
	LLVMValueRef operand2 = LLVMGetOperand(instruction, 1);

	#ifdef DEBUG
	printf("Computing constant arithmetic for\n");
	LLVMDumpValue(instruction);
	printf("\n");
	#endif

	LLVMOpcode op = LLVMGetInstructionOpcode(instruction);

	if (op == LLVMAdd) {
		return LLVMConstAdd(operand1, operand2);
	} else if (op == LLVMSub) {
		return LLVMConstSub(operand1, operand2);
	} else if (op == LLVMMul) {
		return LLVMConstMul(operand1, operand2);
	}
	return NULL;
}

static bool constantFolding(LLVMBasicBlockRef basicBlock) {
	bool codeChanged = false;
	// Iterate over all the instructions in the basic block
	for (LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock);
		instruction; instruction = LLVMGetNextInstruction(instruction)) {
		
		// If the instruction is an arithmetic operation and all its operands are constants,
		if (isArithmeticOperation(instruction) && allOperandsAreConstant(instruction)) {
			LLVMValueRef foldedConstant = computeConstantArithmetic(instruction);
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

static unordered_map<LLVMValueRef, vector<LLVMValueRef>> buildStoreInstructionsMap(LLVMValueRef function) {
	// <ptr for storing, vector of other store instruction that store in the same ptr>
	unordered_map<LLVMValueRef, vector<LLVMValueRef>> storeInstructionsMap; 
	for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
		basicBlock;
		basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
		for (LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock);
			instruction; 
			instruction = LLVMGetNextInstruction(instruction)) {

			// Skip non-store instructions
			if (!LLVMIsAStoreInst(instruction)) {
				continue;
			}

			LLVMValueRef storePtr = LLVMGetOperand(instruction, 1);

			// If the storePtr is not already in storeInstructionsMap, add it with an empty vector
			if (storeInstructionsMap.find(storePtr) == storeInstructionsMap.end()) {
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
static void buildKillNGenSetMaps(
    LLVMValueRef function,
    unordered_map<LLVMValueRef, vector<LLVMValueRef>> storeInstructionsMap,
    unordered_map<LLVMBasicBlockRef, vector<LLVMValueRef>> *killSetMap,
    unordered_map<LLVMBasicBlockRef, unordered_set<LLVMValueRef>> *genSetMap) {

    // Iterate through all basic blocks in the function.
    for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
         basicBlock;
         basicBlock = LLVMGetNextBasicBlock(basicBlock)) {

        // Iterate through all instructions in the basic block.
        for (LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock);
             instruction;
             instruction = LLVMGetNextInstruction(instruction)) {

            // Initialize empty kill and gen sets for basic blocks not already in the maps.
            if (killSetMap->find(basicBlock) == killSetMap->end()) {
                (*killSetMap)[basicBlock] = vector<LLVMValueRef>();
            }
            if (genSetMap->find(basicBlock) == genSetMap->end()) {
                (*genSetMap)[basicBlock] = unordered_set<LLVMValueRef>();
            }

            // Skip non-store instructions.
            if (!LLVMIsAStoreInst(instruction)) {
                continue;
            }

            LLVMValueRef storePtr = LLVMGetOperand(instruction, 1);

            // Add the current store instruction to the gen set for the basic block.
            (*genSetMap)[basicBlock].insert(instruction);

            // Iterate over the vector of store instructions that modify the same pointer.
            for (LLVMValueRef storedInstr : storeInstructionsMap[storePtr]) {

                // Add instructions to the kill set if they are different from the current instruction.
                if (storedInstr != instruction) {
                    (*killSetMap)[basicBlock].push_back(storedInstr);
                }

                // If there is any instruction killed by adding the current instruction, remove it from the GEN set.
                if ((*genSetMap)[basicBlock].find(storedInstr) != (*genSetMap)[basicBlock].end() && storedInstr != instruction) {
                    (*genSetMap)[basicBlock].erase(storedInstr);

                    #ifdef DEBUG
                    printf("\nRemoved instruction from GEN set:\n");
                    LLVMDumpValue(killedInstr);
                    printf("\n was killed by instruction:\n");
                    LLVMDumpValue(instruction);
                    printf("\n");
                    #endif
                }
            }
        }
    }
}


static void constantPropagation(LLVMValueRef function) {
	// Build a map of store instructions
	unordered_map<LLVMValueRef, vector<LLVMValueRef>> storeInstructionsMap = buildStoreInstructionsMap(function);

	// Declare KILL and GEN Maps and populate them
	unordered_map<LLVMBasicBlockRef, vector<LLVMValueRef>> killSetMap;
	unordered_map<LLVMBasicBlockRef, unordered_set<LLVMValueRef>> genSetMap;
	buildKillNGenSetMaps(function, storeInstructionsMap, &killSetMap, &genSetMap);	

}

void optimizeFunction(LLVMValueRef function) {
    bool codeChanged = true;

    while (codeChanged) {
        // Reset codeChanged to false before applying optimizations
        codeChanged = false;

        for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
             basicBlock;
             basicBlock = LLVMGetNextBasicBlock(basicBlock)) {

            // perform global optimization
            // codeChanged = constantPropagation(basicBlock) || codeChanged;
            // #ifdef DEBUG
            // printf("\nConstant propagation: %d\n", codeChanged);
            // printf("______________________________________\n");
            // #endif

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

void walkFunctions(LLVMModuleRef module) {
    for (LLVMValueRef function = LLVMGetFirstFunction(module);
         function;
         function = LLVMGetNextFunction(function)) {

        const char* funcName = LLVMGetValueName(function);

        #ifdef DEBUG
        printf("Function Name: %s\n", funcName);
        #endif

		constantPropagation(function);
        optimizeFunction(function);

    }
}

void walkGlobalValues(LLVMModuleRef module){
	for (LLVMValueRef gVal =  LLVMGetFirstGlobal(module);
                        gVal;
                        gVal = LLVMGetNextGlobal(gVal)) {

                const char* gName = LLVMGetValueName(gVal);
                #ifdef DEBUG
				printf("Global variable name: %s\n", gName);
				#endif
        }
}

int main(int argc, char** argv)
{
	LLVMModuleRef mod;

	if (argc == 2){
		mod = createLLVMModel(argv[1]);
	}
	else{
		mod = NULL;
		return 1;
	}

	if (mod != NULL){
		walkFunctions(mod);
	} else {
	    printf("m is NULL\n");
	}

	// Writing out the LLVM IR
	// LLVMDumpModule(mod); // Dumps the LLVM IR to standard output.
	char *filename = argv[1];	
	strcat(argv[1], "_optimized");

	LLVMPrintModuleToFile(mod, filename, NULL); // Writes the LLVM IR to a file named "test.ll"

	return 0;
}


// static void constantPropagation() {
		// LLVMDumpValue(instruction);
		// printf("\n");
		
		// // Handle store instructions with constant values
        // if (LLVMGetInstructionOpcode(instruction) == LLVMStore) {
        //   LLVMValueRef store_val = LLVMGetOperand(instruction, 0);
        //   LLVMValueRef storePtr = LLVMGetOperand(instruction, 1);

        //   if (LLVMIsAConstantInt(store_val)) {
        //     LLVMUseRef user_iter = LLVMGetFirstUse(storePtr);
        //     while (user_iter != NULL) {
        //       LLVMValueRef user_inst = LLVMGetUser(user_iter);
		// 	  printf("User is \n");
		// 	  LLVMDumpValue(user_inst);
        //       if (LLVMIsALoadInst(user_inst)) {
        //         LLVMReplaceAllUsesWith(user_inst, store_val);

		// 		#ifdef DEBUG
		// 		printf("\nReplaced instruction:\n");
		// 		LLVMDumpValue(instruction);
		// 		printf("\nwith instruction:\n");
		// 		LLVMDumpValue(store_val);
		// 		printf("\n");
		// 		#endif
        //       }
        //       user_iter = LLVMGetNextUse(user_iter);
        //     }
        //   }
        // }
// }