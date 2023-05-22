/**
 * @file file_utils.h
 * 
 * @brief This header file contains the declarations for utility functions used across the compiler. 
 * These include creating an LLVM model from a given filename and changing the file extension of a given filename.
 * These utilities are used in multiple stages of the compiler process.
 * 
 * The functions are declared in this header and their definitions can be found in file_utils.cpp.
 * Include this header in any file where these utility functions are required.
 *
 * @author Aimen Abdulaziz
 * @date Spring 2023
 */

#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <string>
#include <cstring>
using namespace std;


/**
 * Create LLVM module from the given filename
 * @param filename Path to the LLVM IR file
 * @return LLVMModuleRef representing the module created from the given file, or NULL if error occurs
 */
LLVMModuleRef createLLVMModel(char *filename);
/**
 * @brief Changes the file extension of the given filename.
 *
 * This function takes the input filename and changes its extension to what is passed in
 * the `fileExtension` argument.
 * The modified filename is stored in the output parameter.
 *
 * @param[in] filename The input filename as a C-style string (const char*).
 * @param[in] fileExtension The new file extension as a std::string.
 * @param[out] output The modified filename with the new extension as a reference to a std::string.
 */
void changeFileExtension(const char *filename, string &output, string fileExtension);