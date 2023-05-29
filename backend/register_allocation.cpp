/*
 * @file register_allocation.cpp
 *
 * @brief This file implements register allocation for LLVM IR code using the linear scan algorithm.
 *
 * The register allocation algorithm performs the following steps:
 * 1. Computes liveness information for each basic block in the function.
 * 2. Allocates registers for each basic block using the linear scan algorithm.
 * 3. If no registers are available, selects an instruction to spill based on the live usage frequency of the instruction.
 *
 * Usage:
 *   AllocatedReg allocateRegisterForFunction(LLVMValueRef function, bool &usedEBX);
 *
 *   This function takes a valid LLVM function and a boolean variable by reference as input. It returns the LLVM IR code with
 *   registers allocated and sets the usedEBX variable to true if the function uses the EBX register.
 *
 * Output:
 *   The allocateRegisterForFunction function returns an std::unordered_map that maps each LLVM value in the function to the
 *   register that it is allocated to. The function also sets the usedEBX boolean variable to true if the function uses the EBX register.
 *
 * @author Aimen Abdulaziz
 * @date Spring 2023
 */

#include "register_allocation.h"

/**
 * Determines whether the given LLVM instruction opcode produces a result (or a LHS).
 * Instructions that do not produce a result are LLVMStore, LLVMBr, LLVMRet and void LLVMCall.
 * Instructions that do not have a LHS are not considered for register allocation.
 *
 * @param instrOpcode The LLVM instruction opcode to check.
 * @return True if the instruction has a result, false otherwise.
 */
static bool
hasResult(LLVMOpcode instrOpcode, LLVMValueRef instr = nullptr)
{
    static OpcodeSet noResultOpCode = {LLVMStore, LLVMBr, LLVMRet};
    if (instrOpcode == LLVMCall)
    {
        // Check if the call instruction has a void return type
        LLVMTypeRef returnType = LLVMTypeOf(instr);
        return LLVMGetTypeKind(returnType) != LLVMVoidTypeKind;
    }
    return noResultOpCode.find(instrOpcode) == noResultOpCode.end();
}

/**
 * Computes liveness information for a given LLVM basic block.
 * Liveness information is useful for register allocation: it determines where each value is live,
 * starting from its definition up to its last usage. This information is used for deciding
 * when a register can be safely reused.
 *
 * @param basicBlock       The LLVM basic block for which to compute liveness.
 * @param liveUsageMap     An empty map which will be populated with LLVM value to a vector of its use points (represented by instruction indices).
 * @param instructionList  An empty list which will be populated with all non-alloca instructions in the current basic block.
 */
static void
computeLiveness(LLVMBasicBlockRef basicBlock,
                LiveUsageMap &liveUsageMap,
                InstIndex &instructionList)
{
    LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock);

    while (instruction)
    {
        // Alloca instructions are ignored for the purpose of liveness analysis
        if (LLVMIsAAllocaInst(instruction))
        {
            instruction = LLVMGetNextInstruction(instruction);
            continue;
        }

        // If instruction generates a result, add its index to the live range
        LLVMOpcode instrOpcode = LLVMGetInstructionOpcode(instruction);
        if (hasResult(instrOpcode, instruction))
        {
            liveUsageMap[instruction].push_back(instructionList.size());
        }

        // Update the live usage of any of the operands in the instruction
        for (auto i = 0; i < LLVMGetNumOperands(instruction); i++)
        {
            LLVMValueRef operand = LLVMGetOperand(instruction, i);
            if (liveUsageMap.count(operand) > 0)
            {
                liveUsageMap[operand].push_back(instructionList.size());
            }
        }

        // Append the instruction to the instruction list
        instructionList.push_back(instruction);

        // Proceed to the next instruction in the basic block
        instruction = LLVMGetNextInstruction(instruction);
    }
}

/**
 * Prints the instruction index vector to the console for debugging purpose.
 *
 * @param instructionIndex The instruction index vector to print.
 */
static void
printInstructionIndexVector(InstIndex &instructionIndex)
{
    // print the instruction index vector
    for (auto i = 0; i < instructionIndex.size(); i++)
    {
        // Print the LLVMValueRef to a string
        char *instruction = LLVMPrintValueToString(instructionIndex[i]);
        cout << i << ": " << instruction << endl;
        LLVMDisposeMessage(instruction);
    }
    cout << endl;
}

/**
 * Prints the live range of each instruction in the given LiveUsageMap to the console.
 *
 * @param liveUsageMap The LiveUsageMap to print.
 */
static void
printLiveUsageMap(LiveUsageMap &liveUsageMap)
{
    // Print the live range of each instruction
    for (auto usages : liveUsageMap)
    {
        // print the vector of all the usages
        cout << "Live range of";
        char *instruction = LLVMPrintValueToString(usages.first);
        cout << instruction << ": ";
        LLVMDisposeMessage(instruction);
        for (auto usage : usages.second)
        {
            cout << usage << " ";
        }
        cout << endl;
    }
}

/**
 * @brief Removes the allocated register for the given instruction operand from the basic block allocated register map.
 *
 * The function iterates over each operand of the instruction and adds the physical register assigned to the
 * operand to the set of available registers.
 * If the operand does not have a physical register assigned to it, the function does nothing.
 *
 * @param instructionIdx The index of the current instruction in the instruction list.
 * @param operandStartIdx The index of the first operand to process.
 * @param instruction The LLVM instruction to remove the allocated register for.
 * @param liveUsageMap The LiveUsageMap for the function.
 * @param instructionIndex The instruction index map for the function.
 * @param bbAllocatedRegisterMap The allocated register map for the basic block.
 * @param availableRegisters The set of available registers.
 */
static void
removeAllocatedRegister(int instructionIdx,
                        int operandStartIdx,
                        LLVMValueRef instruction,
                        LiveUsageMap &liveUsageMap,
                        InstIndex &instructionIndex,
                        AllocatedReg &bbAllocatedRegisterMap,
                        RegisterSet &availableRegisters)
{
    // Get the number of operands of the instruction
    const int numOperands = LLVMGetNumOperands(instruction);

    // Iterate over each operand of the instruction
    for (int i = operandStartIdx; i < numOperands; i++)
    {
        // Get the i-th operand of the instruction
        LLVMValueRef operand = LLVMGetOperand(instruction, i);

        // Skip constants, operands that are not in the liveUsageMap, and operands that are live after the current instruction
        if (LLVMIsAConstant(operand) || liveUsageMap.count(operand) == 0 || liveUsageMap[operand].back() > instructionIdx)
        {
            continue;
        }

        // Check if the operand has a physical register assigned to it
        const auto it = bbAllocatedRegisterMap.find(operand);
        if (it == bbAllocatedRegisterMap.end())
        {
            // If the operand does not have a physical register assigned to it, continue to the next operand
            continue;
        }

        // Get the physical register assigned to the operand
        const Register registerName = it->second;

        // Add the physical register to the set of available registers
        availableRegisters.insert(registerName);
    }
}

/**
 * Returns the name of the given register as a string.
 *
 * @param reg The register to get the name of.
 * @return The name of the register as a string.
 */
std::string
getRegisterName(Register reg)
{
    switch (reg)
    {
    case EAX:
        return "eax";
    case EBX:
        return "ebx";
    case ECX:
        return "ecx";
    case EDX:
        return "edx";
    case SPILL:
        return "SPILL";
    default:
        return "Unknown register";
    }
}

/**
 * Merges the allocated register map for a basic block with the global allocated register map.
 *
 * @param bbAllocatedRegisterMap a reference to the allocated register map for the basic block.
 * @param allocatedRegisterMap a reference to the global allocated register map.
 */
static void
mergeBBWGlobalMap(AllocatedReg &bbAllocatedRegisterMap,
                  AllocatedReg &allocatedRegisterMap)
{
    for (auto &entry : bbAllocatedRegisterMap)
    {
        LLVMValueRef instr = entry.first;
        Register reg = entry.second;

#ifdef DEBUG
        char *str = LLVMPrintValueToString(instr);
        cout << "Assigned register " << getRegisterName(reg) << " to" << str << endl;
        LLVMDisposeMessage(str);
#endif

        allocatedRegisterMap[instr] = reg;
    }
}

/**
 * Determines whether the given LLVM instruction opcode is an arithmetic operation.
 * Arithmetic operations for MiniC are LLVMAdd, LLVMSub, and LLVMMul
 *
 * @param instrOpcode The LLVM instruction opcode to check.
 * @return True if the instruction is an arithmetic operation, false otherwise.
 */
static bool
isArithmetic(LLVMOpcode instrOpcode)
{
    static OpcodeSet arithmeticOpcode = {LLVMAdd, LLVMSub, LLVMMul};
    return arithmeticOpcode.find(instrOpcode) != arithmeticOpcode.end();
}

/**
 * @brief Selects an instruction to spill based on the live usage frequency of its operands.
 *
 * The function iterates over each instruction in the basic block and selects the instruction
 * with the lowest live usage frequency.
 * The selected instruction is returned as the spill instruction.
 *
 * @param liveUsageMap The LiveUsageMap for the function.
 * @param bbAllocatedRegisterMap The allocated register map for the basic block.
 * @param currInstr The current instruction being processed.
 * @return The instruction to spill, or nullptr if no instruction can be spilled.
 */
static LLVMValueRef
selectSpillInstr(LiveUsageMap &liveUsageMap,
                 AllocatedReg &bbAllocatedRegisterMap,
                 LLVMValueRef currInstr)
{
    LLVMValueRef spillVar = nullptr;
    int minFrequency = INT_MAX;
    for (auto it = bbAllocatedRegisterMap.begin(); it != bbAllocatedRegisterMap.end(); ++it)
    {
        LLVMValueRef instr = it->first;
        Register registerName = it->second;
        if (registerName != SPILL && liveUsageMap[instr].size() < minFrequency)
        {
            spillVar = instr;
            minFrequency = liveUsageMap[instr].size();
        }
    }
    return spillVar;
}

/**
 * @brief This function implements the linear scan register allocation algorithm.
 *
 * Allocates registers for the given LLVM basic block using the linear scan algorithm.
 * The function creates an AllocatedReg map to store the register allocated to each instruction in the basic block.
 * It then iterates over each instruction in the basic block and allocates registers for each instruction.
 * The allocated registers are stored in the AllocatedReg map.
 * The function also removes allocated registers for operands whose live range ends in the current instruction.
 * If no registers are available, the function selects an instruction to spill and spills it.
 * The function then merges the basic block allocated register map into the global allocated register map.
 *
 * @param basicBlock The LLVM basic block to allocate registers for.
 * @param instructionList The list of instructions in the basic block.
 * @param liveUsageMap The LiveUsageMap for the function.
 * @param allocatedRegisterMap The global allocated register map.
 *
 * @ret
 */
static bool
allocateRegisterForBasicBlock(LLVMBasicBlockRef &basicBlock,
                              InstIndex &instructionList,
                              LiveUsageMap &liveUsageMap,
                              AllocatedReg &allocatedRegisterMap)
{
    // Create a map of instruction to register for the current basic block
    AllocatedReg bbAllocatedRegisterMap;
    RegisterSet availableRegisters = {EBX, ECX, EDX};
    bool usedEBX = false;

    for (int i = 0; i < instructionList.size(); i++)
    {
        LLVMValueRef currInstr = instructionList[i];
        LLVMOpcode instrOpcode = LLVMGetInstructionOpcode(currInstr);

        // Check if the instruction does NOT generates a result (no LHS)
        if (!hasResult(instrOpcode, currInstr))
        {
            // Check and remove allocated registers for each operand whose live range ends in the current instruction
            removeAllocatedRegister(i, 0, currInstr, liveUsageMap, instructionList, bbAllocatedRegisterMap, availableRegisters);
            continue;
        }

        // Check if the instruction is an arithmetic instruction
        if (isArithmetic(instrOpcode))
        {
            // Get the first operand of the instruction
            LLVMValueRef firstOperand = LLVMGetOperand(currInstr, 0);

            // If the first operand has a physical register assigned to it in and its liveness range ends at
            // the current instruction, assign the same register to the instruction.
            if (bbAllocatedRegisterMap.count(firstOperand) > 0 && liveUsageMap[firstOperand].back() == i)
            {
                // Get the physical register assigned to the first operand
                Register reg = bbAllocatedRegisterMap[firstOperand];

                // Add the reg to the curr instr
                bbAllocatedRegisterMap[currInstr] = reg;

                // Remove the allocated register for the second operand if its live range ends in the current instruction
                removeAllocatedRegister(i, 1, currInstr, liveUsageMap, instructionList, bbAllocatedRegisterMap, availableRegisters);
                continue;
            }
        }

        // If registers are available, allocate one to the current instruction
        if (!availableRegisters.empty())
        {
            Register registerName = *availableRegisters.begin();
            availableRegisters.erase(registerName);
            bbAllocatedRegisterMap[currInstr] = registerName;
            removeAllocatedRegister(i, 0, currInstr, liveUsageMap, instructionList, bbAllocatedRegisterMap, availableRegisters);

            if (registerName == EBX)
            {
                usedEBX = true;
            }
            continue;
        }

        // If there is no available register, find the instr to spill
        LLVMValueRef spillInstr = selectSpillInstr(liveUsageMap, bbAllocatedRegisterMap, currInstr);

        // If no registers are available, select an instruction to spill
        if (liveUsageMap[spillInstr].size() > liveUsageMap[currInstr].size())
        {
            bbAllocatedRegisterMap[currInstr] = SPILL;
            continue;
        }
        else
        {
            // If the current instruction has more uses, spill the selected instruction
            Register spilledRegisterName = bbAllocatedRegisterMap[spillInstr];
            bbAllocatedRegisterMap[currInstr] = spilledRegisterName;
            bbAllocatedRegisterMap[spillInstr] = SPILL;
        }

        removeAllocatedRegister(i, 0, currInstr, liveUsageMap, instructionList, bbAllocatedRegisterMap, availableRegisters);
    }

    // Merge the basic block allocated register map into the global allocated register map
    mergeBBWGlobalMap(bbAllocatedRegisterMap, allocatedRegisterMap);

    return usedEBX;
}

/**
 * Allocates registers for the given LLVM function using the linear scan algorithm.
 * The function creates an AllocatedReg map to store the register allocated to each instruction.
 * It then iterates over each basic block in the function and allocates registers for each basic block.
 * The allocated registers are stored in the AllocatedReg map.
 *
 * @param function The LLVM function to allocate registers for.
 * @param usedEBX A flag to indicate if the EBX register is used in the function.
 * @return The AllocatedReg map that stores the register allocated to each instruction.
 */
AllocatedReg
allocateRegisterForFunction(LLVMValueRef function, bool &usedEBX)
{
    // Create a map to store the register allocated to each instruction
    AllocatedReg allocatedRegisterMap;

    // Get the first basic block
    LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
    while (basicBlock)
    {
        // Create an instruction index for the basic block
        InstIndex instructionList;
        LiveUsageMap liveUsageMap;
        computeLiveness(basicBlock, liveUsageMap, instructionList);

#ifdef DEBUG
        printInstructionIndexVector(instructionList);
        printLiveUsageMap(liveUsageMap);
#endif

        // Allocate register on a basic block level
        bool basicBlockUsedEBX = allocateRegisterForBasicBlock(basicBlock, instructionList, liveUsageMap, allocatedRegisterMap);

        // Set the usedEBX flag to true if the EBX register is used in the basic block
        usedEBX = usedEBX || basicBlockUsedEBX;

        // Get the next basic block
        basicBlock = LLVMGetNextBasicBlock(basicBlock);
    }
    return allocatedRegisterMap;
}