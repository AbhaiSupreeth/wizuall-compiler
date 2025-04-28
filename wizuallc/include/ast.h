#ifndef AST_H
#define AST_H

#include <stdlib.h> // For size_t
#include <stdio.h> // Include stdio for FILE type if needed later, good practice

//------------------------------------------------------------------------------
// Node Type Enum
//------------------------------------------------------------------------------
typedef enum {
    NODE_TYPE_UNKNOWN = 0,
    NODE_TYPE_NUMBER,        // Scalar number (double)
    NODE_TYPE_VECTOR,        // Vector literal (list of expression nodes)
    NODE_TYPE_IDENTIFIER,    // Variable identifier (string)
    NODE_TYPE_BINARY_OP,     // Binary operation (+, -, *, /)
    NODE_TYPE_UNARY_OP,      // Unary operation (e.g., negation '-'/+)
    NODE_TYPE_ASSIGNMENT,    // Assignment (identifier = expression)
    NODE_TYPE_STATEMENT_LIST, // Sequence of statements
    NODE_TYPE_IF,            // If statement
    NODE_TYPE_WHILE,         // While loop
    NODE_TYPE_FUNC_CALL      // External function call
} NodeType;

//------------------------------------------------------------------------------
// Forward Declaration
//------------------------------------------------------------------------------
struct Symbol; // Forward declare Symbol struct
struct ASTNode;

//------------------------------------------------------------------------------
// Node Data Structures (using a union within the main struct)
//------------------------------------------------------------------------------

// Structure for a list of nodes (used for vectors and statement lists)
typedef struct {
    struct ASTNode **items; // Array of ASTNode pointers
    size_t count;           // Number of items currently in the list
    size_t capacity;        // Allocated capacity of the items array
} NodeList;

// Structure for Binary Operation
typedef struct {
    char op;                 // The operator character: '+', '-', '*', '/'
    struct ASTNode *left;    // Left operand
    struct ASTNode *right;   // Right operand
} BinaryOpNode;

// Structure for Unary Operation
typedef struct {
    char op;                 // The operator character: e.g., '-' for negation
    struct ASTNode *operand; // The operand
} UnaryOpNode;

// Structure for Assignment
typedef struct {
    // char *identifier;       // Name of the variable being assigned
    struct Symbol *target_symbol; // Pointer to the symbol table entry
    struct ASTNode *expression; // The expression being assigned
} AssignmentNode;

// Structure for If Statement
typedef struct {
    struct ASTNode *condition;
    struct ASTNode *if_branch;   // Statement list (block)
    struct ASTNode *else_branch; // Optional statement list (block), can be NULL
} IfNode;

// Structure for While Loop
typedef struct {
    struct ASTNode *condition;
    struct ASTNode *loop_body;   // Statement list (block)
} WhileNode;

// Structure for Function Call
typedef struct {
    struct Symbol *function_symbol; // Symbol for the function identifier
    NodeList arguments;             // List of expression nodes for arguments
} FuncCallNode;

// Main AST Node Structure
typedef struct ASTNode {
    NodeType type;
    // Add line number tracking later if needed: int line_number;
    union {
        double number_value;    // For NODE_TYPE_NUMBER
        NodeList vector_elements; // For NODE_TYPE_VECTOR
        // char *identifier_name;  // For NODE_TYPE_IDENTIFIER
        struct Symbol *identifier_symbol; // Pointer to symbol table entry
        BinaryOpNode binary_op;   // For NODE_TYPE_BINARY_OP
        UnaryOpNode unary_op;     // For NODE_TYPE_UNARY_OP
        AssignmentNode assignment; // For NODE_TYPE_ASSIGNMENT
        NodeList statement_list;  // For NODE_TYPE_STATEMENT_LIST
        IfNode if_stmt;         // For NODE_TYPE_IF
        WhileNode while_loop;   // For NODE_TYPE_WHILE
        FuncCallNode func_call; // For NODE_TYPE_FUNC_CALL
    } data;
} ASTNode;

//------------------------------------------------------------------------------
// Constructor Functions (Declarations)
//------------------------------------------------------------------------------

ASTNode* ast_new_node(NodeType type);
ASTNode* ast_new_number(double value);
ASTNode* ast_new_identifier(struct Symbol *sym);
ASTNode* ast_new_binary_op(char op, ASTNode *left, ASTNode *right);
ASTNode* ast_new_unary_op(char op, ASTNode *operand);
ASTNode* ast_new_assignment(struct Symbol *sym, ASTNode *expression);
ASTNode* ast_new_vector();
ASTNode* ast_new_statement_list();
ASTNode* ast_new_if(ASTNode *condition, ASTNode *if_branch, ASTNode *else_branch);
ASTNode* ast_new_while(ASTNode *condition, ASTNode *loop_body);
ASTNode* ast_new_func_call(struct Symbol *func_sym, NodeList args);

// Function to add an element to a vector node
void ast_add_vector_element(ASTNode *vector_node, ASTNode *element);

// Function to add a statement to a statement list node
void ast_add_statement(ASTNode *list_node, ASTNode *statement);

//------------------------------------------------------------------------------
// Destructor Function (Declaration)
//------------------------------------------------------------------------------

void ast_free_node(ASTNode *node);

//------------------------------------------------------------------------------
// AST Printing Function (Declaration)
//------------------------------------------------------------------------------

void print_ast(ASTNode *node, int indent);

#endif // AST_H 