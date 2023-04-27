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

	for (int i = 0; i < numberOfOperands1; i++){
		LLVMValueRef operand1 = LLVMGetOperand(instruction1, i);
		LLVMValueRef operand2 = LLVMGetOperand(instruction2, i);

		#ifdef DEBUG
		printf("\nOperand 1:\n");
		LLVMDumpValue(operand1);
		printf("\nOperand 2:\n");
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

static void commonSubexpressionElimination(LLVMBasicBlockRef basicBlock) {
	// Create an empty hashmap opcode_map of vectors of instructions
	unordered_map<LLVMOpcode, vector<LLVMValueRef>> opcode_map; // <opcode, vector of instructions>

	// Iterate over all the instructions in the basic block
	for (LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock); instruction;
  				instruction = LLVMGetNextInstruction(instruction)) {

		// Get the opcode of the instruction
		LLVMOpcode op = LLVMGetInstructionOpcode(instruction);
		
		// Skip alloca instructions
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
			printf("\nwith...\n");
			LLVMDumpValue(prev_instruction);
			#endif

			if (isCommonSubexpression(prev_instruction, instruction)) {
				// Replace all uses of the instruction with the previous instruction
				LLVMReplaceAllUsesWith(instruction, prev_instruction);
				
				#ifdef DEBUG
				printf("\nReplaced instruction:\n");
				LLVMDumpValue(instruction);
				printf("\nwith instruction:\n");
				LLVMDumpValue(prev_instruction);
				#endif
			}
		}
		
		opcode_map[op].push_back(instruction);
	}
}

void walkBasicblocks(LLVMValueRef function){
	for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
 			 basicBlock;
  			 basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
		
		#ifdef DEBUG
		printf("In basic block\n");
		#endif
		
		// call local optimization functions
		commonSubexpressionElimination(basicBlock);
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