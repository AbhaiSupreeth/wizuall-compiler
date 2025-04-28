#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h" // Include AST node definitions

/**
 * @brief Generates C code from the given AST and writes it to a file.
 *
 * @param ast_root The root node of the Abstract Syntax Tree.
 * @param output_filename The name of the C file to create.
 */
void generate_code(ASTNode *ast_root, const char *output_filename);


#endif // CODEGEN_H 