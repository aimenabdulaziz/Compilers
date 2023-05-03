#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>

// C++ libraries
#include <unordered_map>
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
	// Create an empty hashmap opcode_map of vectors of instructions
	unordered_map<LLVMOpcode, vector<LLVMValueRef>> opcode_map; // <opcode, vector of instructions>
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
		if (opcode_map.find(op) == opcode_map.end()) {
			// If not, add it to the hashmap and initialize the vector
			opcode_map[op] = vector<LLVMValueRef>();
		} 

		// Check if the instruction is a common subexpression by iterating over all the 
		// previous instructions with the same opcode
		for (LLVMValueRef prev_instruction : opcode_map[op]) {
			#ifdef DEBUG
			printf("\nChecking...\n");
			LLVMDumpValue(instruction);
			if (!hasUses(prev_instruction)) {
				printf("\nInstruction has no uses:\n");
			}
			else {	
				printf("\nwith...\n"); 
			}
			LLVMDumpValue(prev_instruction);
			#endif

		if (hasUses(prev_instruction) && isCommonSubexpression(prev_instruction, instruction)) {
			// Replace all uses of the instruction with the previous instruction
			LLVMReplaceAllUsesWith(instruction, prev_instruction);
			subExpressionEliminated = true;

			#ifdef DEBUG
			printf("\nReplaced instruction:\n");
			LLVMDumpValue(instruction);
			printf("\nwith instruction:\n");
			LLVMDumpValue(prev_instruction);
			#endif

			break;
		}
	}
		
		opcode_map[op].push_back(instruction);
	}

	return subExpressionEliminated;
}

/**
 * @brief Checks if the given instruction has side effects.
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

void walkBasicblocks(LLVMValueRef function) {
	bool codeChanged = true;

	while (codeChanged) {
		// Reset codeChanged to false before applying optimizations
		codeChanged = false;

		for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
 			 basicBlock;
  			 basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
		
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

void walkFunctions(LLVMModuleRef module){
	for (LLVMValueRef function =  LLVMGetFirstFunction(module); 
			function; 
			function = LLVMGetNextFunction(function)) {

		const char* funcName = LLVMGetValueName(function);	

		#ifdef DEBUG
		printf("Function Name: %s\n", funcName);
		#endif

		walkBasicblocks(function);

		/*
		* Constant folding
		* Common Subexpression Elimination
		* Dead Code Elimination
		* Constant Propagation
		*/
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
		//LLVMDumpModule(m);
		walkFunctions(mod);
	}
	else {
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
        //   LLVMValueRef store_ptr = LLVMGetOperand(instruction, 1);

        //   if (LLVMIsAConstantInt(store_val)) {
        //     LLVMUseRef user_iter = LLVMGetFirstUse(store_ptr);
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