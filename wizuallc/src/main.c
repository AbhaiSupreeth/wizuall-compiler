#include <stdio.h>
#include <stdlib.h>
#include "ast.h" // Include AST header for Node type and functions
#include "symtab.h" // Include Symbol Table header
#include "codegen.h" // Include Codegen header

// External declarations for Flex/Bison
extern FILE *yyin; // Input stream for the lexer
extern int yylex();  // Lexer function (though usually called by yyparse)
extern int yyparse(); // Parser function
extern ASTNode *ast_root; // Declare the global AST root from parser.y

int main(int argc, char **argv) {
    // Check for the correct number of command-line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_filename>\n", argv[0]);
        return 1; // Indicate error
    }

    const char* output_c_file = "output.c"; // Default output filename

    // Try to open the input file specified in the arguments
    yyin = fopen(argv[1], "r");
    if (yyin == NULL) {
        perror(argv[1]); // Print system error message (e.g., "file not found")
        return 1; // Indicate error
    }

    printf("Parsing file: %s\n", argv[1]);

    // Call the Bison-generated parser
    int parse_result = yyparse();

    // Close the input file
    fclose(yyin);

    int return_code = 1; // Default to failure

    // Check the result of parsing
    if (parse_result == 0) {
        printf("\nParsing completed successfully.\n");
        printf("--- Abstract Syntax Tree ---\n");
        if (ast_root) {
            print_ast(ast_root, 0);
            printf("--- Generating C code to %s ---\n", output_c_file);
            generate_code(ast_root, output_c_file);
            printf("--- Freeing AST ---\n");
            ast_free_node(ast_root);
            ast_root = NULL; // Avoid dangling pointer
            return_code = 0; // Success
        } else {
            printf("(No AST generated - empty input?)\n");
            return_code = 0; // Technically success if input was empty and parsed
        }
        printf("--------------------------\n");
    } else {
        fprintf(stderr, "Parsing failed.\n");
        // AST might be partially built and leak memory if yyerror exited.
        // Symbol table might also leak.
        return_code = 1; // Failure
    }

    // Clean up symbol table regardless of parsing success/failure
    // (though symbols might not be fully populated on failure)
    printf("--- Freeing Symbol Table ---\n");
    symbol_table_destroy();
    printf("--------------------------\n");

    return return_code;
} 