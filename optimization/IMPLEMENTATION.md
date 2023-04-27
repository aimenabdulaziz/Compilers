Common Subexpression Elimination (CSE):
    
1. Iterate over each basic block in the function.    
2. For each instruction in the basic block, check if it has the same opcode and operands as any previous instruction in the same basic block. 
3. If the two instructions (A and B) are not load instructions, replace all uses of B with A using LLVMReplaceAllUsesWith. If they are load instructions, perform a safety check to ensure no instruction between A and B modifies the memory location pointed to by their operands. If the safety check passes, replace all uses of B with A.


Example:
```LLVM
%a = add i32 1, 2
%b = mul i32 3, 4
%c = add i32 1, 2
```

In this example, `%c` is a common subexpression of %a. After CSE, the code would look like:

```LLVM
%a = add i32 1, 2
%b = mul i32 3, 4
%c = %a
```

Dead Code Elimination (DCE):
a. Iterate over each basic block in the function.
b. For each instruction in the basic block, check if it has any uses.
c. If the instruction has no uses and is not an instruction with side effects (e.g., a store instruction), remove the instruction using `LLVMInstructionEraseFromParent`.

Example:

```LLVM
%a = add i32 1, 2
%b = mul i32 3, 4
%c = add i32 1, 2
%d = add i32 %b, 5
```

In this example, %a and %c are dead code. After DCE, the code would look like:

```LLVM
%b = mul i32 3, 4
%d = add i32 %b, 5
```


## Pseudo code
```c
function optimizeFunction(function):
    for basicBlock in function:
        commonSubexpressionElimination(basicBlock)
        deadCodeElimination(basicBlock)
        constantFolding(basicBlock)

function commonSubexpressionElimination(basicBlock):
    for instruction in basicBlock:
        for prevInstruction in instructionsBefore(instruction, basicBlock):
            if isCommonSubexpression(instruction, prevInstruction):
                LLVMReplaceAllUsesWith(instruction, prevInstruction)

function isCommonSubexpression(instruction1, instruction2):
    if sameOperands(instruction1, instruction2):
        if not isLoadInstruction(instruction1) and not isLoadInstruction(instruction2):
            return true
        elif safeToReplaceLoadInstructions(instruction1, instruction2):
            return true
    return false

function deadCodeElimination(basicBlock):
    for instruction in basicBlock:
        if not hasUses(instruction) and not hasSideEffects(instruction):
            toDelete.add(instruction)
    
    for instr in toDelete:
        LLVMInstructionEraseFromParent(instr)

function constantFolding(basicBlock):
    for instruction in basicBlock:
        if isArithmeticOperation(instruction) and allOperandsAreConstant(instruction):
            foldedConstant = computeConstantArithmetic(instruction)
            LLVMReplaceAllUsesWith(instruction, foldedConstant)

function computeConstantArithmetic(instruction):
    if isAddition(instruction):
        return LLVMConstAdd(operand1, operand2)
    elif isSubtraction(instruction):
        return LLVMConstSub(operand1, operand2)
    elif isMultiplication(instruction):
        return LLVMConstMul(operand1, operand2)
```

```bash
static bool safeToReplaceLoadInstructions(LLVMValueRef instruction1, LLVMValueRef instruction2) {
```
For a Load instruction, there is only one operand, which is the pointer operand (the memory address from which the value is loaded). This is why I used LLVMGetOperand(A, 0) and LLVMGetOperand(B, 0) to get the pointer operands of the Load instructions A and B.

For a Store instruction, there are two operands: the value to be stored and the pointer operand (the memory address at which the value is stored). This is why I used LLVMGetOperandLLVMGetOperand(instruction, 1) during the for loop when checking Store instructions. Index 0 would give the value to be stored, while index 1 gives the pointer operand (the memory location being written to).