#include "ast.h"
#include "symtab.h" // Needed for Symbol struct definition
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For strdup

// Initial capacity for dynamic lists (vectors, statement lists)
#define INITIAL_LIST_CAPACITY 8

//------------------------------------------------------------------------------
// Helper function for dynamic list reallocation
//------------------------------------------------------------------------------
static void ensure_list_capacity(NodeList *list) {
    if (list->count >= list->capacity) {
        size_t new_capacity = (list->capacity == 0) ? INITIAL_LIST_CAPACITY : list->capacity * 2;
        ASTNode **new_items = realloc(list->items, new_capacity * sizeof(ASTNode*));
        if (!new_items) {
            perror("Failed to reallocate memory for AST node list");
            // Handle error appropriately - maybe exit or return an error code
            // For now, we'll exit for simplicity
            exit(EXIT_FAILURE);
        }
        list->items = new_items;
        list->capacity = new_capacity;
    }
}

//------------------------------------------------------------------------------
// Constructor Functions (Implementations)
//------------------------------------------------------------------------------

// Generic node allocation
ASTNode* ast_new_node(NodeType type) {
    ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        perror("Failed to allocate memory for AST node");
        exit(EXIT_FAILURE);
    }
    node->type = type;
    // Initialize union members to 0/NULL where applicable
    memset(&node->data, 0, sizeof(node->data)); 
    return node;
}

// Number node
ASTNode* ast_new_number(double value) {
    ASTNode *node = ast_new_node(NODE_TYPE_NUMBER);
    node->data.number_value = value;
    return node;
}

// Identifier node
ASTNode* ast_new_identifier(Symbol *sym) { // Takes Symbol*
    ASTNode *node = ast_new_node(NODE_TYPE_IDENTIFIER);
    // node->data.identifier_name = strdup(name); // No longer needed
    node->data.identifier_symbol = sym;
    return node;
}

// Binary operation node
ASTNode* ast_new_binary_op(char op, ASTNode *left, ASTNode *right) {
    ASTNode *node = ast_new_node(NODE_TYPE_BINARY_OP);
    node->data.binary_op.op = op; // Store the character representing the op
    node->data.binary_op.left = left;
    node->data.binary_op.right = right;
    return node;
}

// Unary operation node
ASTNode* ast_new_unary_op(char op, ASTNode *operand) {
    ASTNode *node = ast_new_node(NODE_TYPE_UNARY_OP);
    node->data.unary_op.op = op;
    node->data.unary_op.operand = operand;
    return node;
}

// Assignment node
ASTNode* ast_new_assignment(Symbol *sym, ASTNode *expression) { // Takes Symbol*
    ASTNode *node = ast_new_node(NODE_TYPE_ASSIGNMENT);
    // node->data.assignment.identifier = strdup(identifier); // No longer needed
    node->data.assignment.target_symbol = sym;
    node->data.assignment.expression = expression;
    return node;
}

// Vector node (initially empty)
ASTNode* ast_new_vector() {
    ASTNode *node = ast_new_node(NODE_TYPE_VECTOR);
    node->data.vector_elements.items = NULL;
    node->data.vector_elements.count = 0;
    node->data.vector_elements.capacity = 0;
    return node;
}

// Statement list node (initially empty)
ASTNode* ast_new_statement_list() {
    ASTNode *node = ast_new_node(NODE_TYPE_STATEMENT_LIST);
    node->data.statement_list.items = NULL;
    node->data.statement_list.count = 0;
    node->data.statement_list.capacity = 0;
    return node;
}

// If statement node
ASTNode* ast_new_if(ASTNode *condition, ASTNode *if_branch, ASTNode *else_branch) {
    ASTNode *node = ast_new_node(NODE_TYPE_IF);
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.if_branch = if_branch;     // Should be a statement list
    node->data.if_stmt.else_branch = else_branch; // Can be NULL or statement list
    return node;
}

// While loop node
ASTNode* ast_new_while(ASTNode *condition, ASTNode *loop_body) {
    ASTNode *node = ast_new_node(NODE_TYPE_WHILE);
    node->data.while_loop.condition = condition;
    node->data.while_loop.loop_body = loop_body; // Should be a statement list
    return node;
}

// Function Call node
ASTNode* ast_new_func_call(Symbol *func_sym, NodeList args) {
    ASTNode *node = ast_new_node(NODE_TYPE_FUNC_CALL);
    node->data.func_call.function_symbol = func_sym;
    node->data.func_call.arguments = args; // Assign the already built NodeList
    return node;
}

// Add element to vector
void ast_add_vector_element(ASTNode *vector_node, ASTNode *element) {
    if (!vector_node || vector_node->type != NODE_TYPE_VECTOR || !element) return;
    NodeList *list = &vector_node->data.vector_elements;
    ensure_list_capacity(list);
    list->items[list->count++] = element;
}

// Add statement to list
void ast_add_statement(ASTNode *list_node, ASTNode *statement) {
    if (!list_node || list_node->type != NODE_TYPE_STATEMENT_LIST || !statement) return;
    NodeList *list = &list_node->data.statement_list;
    ensure_list_capacity(list);
    list->items[list->count++] = statement;
}

// Helper function (copied logic from ast_add_vector_element, maybe generalize later)
void ast_add_argument(NodeList *list, ASTNode *argument) {
    if (!list || !argument) return;
    ensure_list_capacity(list); // Use the existing helper
    list->items[list->count++] = argument;
}

//------------------------------------------------------------------------------
// Destructor Function (Implementation)
//------------------------------------------------------------------------------

void ast_free_node(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_TYPE_NUMBER:
            // No dynamic memory directly in this node type data
            break;
        case NODE_TYPE_IDENTIFIER:
            // free(node->data.identifier_name); // No longer freeing name here
            // Symbol pointer is just a reference, not owned by AST node
            break;
        case NODE_TYPE_BINARY_OP:
            ast_free_node(node->data.binary_op.left);
            ast_free_node(node->data.binary_op.right);
            break;
        case NODE_TYPE_UNARY_OP:
            ast_free_node(node->data.unary_op.operand);
            break;
        case NODE_TYPE_ASSIGNMENT:
            // free(node->data.assignment.identifier); // No longer freeing name here
            // Symbol pointer is just a reference
            ast_free_node(node->data.assignment.expression);
            break;
        case NODE_TYPE_VECTOR:
            // Free all element nodes in the vector
            for (size_t i = 0; i < node->data.vector_elements.count; ++i) {
                ast_free_node(node->data.vector_elements.items[i]);
            }
            free(node->data.vector_elements.items); // Free the list array itself
            break;
        case NODE_TYPE_STATEMENT_LIST:
            // Free all statement nodes in the list
            for (size_t i = 0; i < node->data.statement_list.count; ++i) {
                ast_free_node(node->data.statement_list.items[i]);
            }
            free(node->data.statement_list.items); // Free the list array itself
            break;
        case NODE_TYPE_IF:
            ast_free_node(node->data.if_stmt.condition);
            ast_free_node(node->data.if_stmt.if_branch);
            if (node->data.if_stmt.else_branch) {
                ast_free_node(node->data.if_stmt.else_branch);
            }
            break;
        case NODE_TYPE_WHILE:
            ast_free_node(node->data.while_loop.condition);
            ast_free_node(node->data.while_loop.loop_body);
            break;
        case NODE_TYPE_FUNC_CALL:
             // Don't free the symbol, it's owned by the symbol table
             // Free the argument nodes
            for (size_t i = 0; i < node->data.func_call.arguments.count; ++i) {
                ast_free_node(node->data.func_call.arguments.items[i]);
            }
            free(node->data.func_call.arguments.items); // Free the argument list array
            break;
        case NODE_TYPE_UNKNOWN:
        default:
            // Should not happen in a well-formed tree
            fprintf(stderr, "Warning: Trying to free unknown or invalid node type %d\n", node->type);
            break;
    }

    // Finally, free the node structure itself
    free(node);
}

//------------------------------------------------------------------------------
// AST Printing Function (Implementation)
//------------------------------------------------------------------------------

// Helper to print indentation
static void print_indent(int indent) {
    for (int i = 0; i < indent; ++i) {
        printf("  "); // Print two spaces per indent level
    }
}

void print_ast(ASTNode *node, int indent) {
    if (!node) {
        // Optional: print something for NULL nodes if needed for debugging
        // print_indent(indent);
        // printf("(NULL Node)\n");
        return;
    }

    print_indent(indent);

    switch (node->type) {
        case NODE_TYPE_NUMBER:
            printf("NUMBER: %f\n", node->data.number_value);
            break;

        case NODE_TYPE_IDENTIFIER:
            // Print name from symbol table entry
            printf("IDENTIFIER: %s\n", 
                   node->data.identifier_symbol ? node->data.identifier_symbol->name : "(null symbol!)"); 
            break;

        case NODE_TYPE_BINARY_OP:
            printf("BINARY_OP: %c\n", node->data.binary_op.op);
            print_ast(node->data.binary_op.left, indent + 1);
            print_ast(node->data.binary_op.right, indent + 1);
            break;

        case NODE_TYPE_UNARY_OP:
            printf("UNARY_OP: %c\n", node->data.unary_op.op);
            print_ast(node->data.unary_op.operand, indent + 1);
            break;

        case NODE_TYPE_ASSIGNMENT:
            // Print name from symbol table entry
            printf("ASSIGNMENT: %s =\n", 
                   node->data.assignment.target_symbol ? node->data.assignment.target_symbol->name : "(null symbol!)");
            print_ast(node->data.assignment.expression, indent + 1);
            break;

        case NODE_TYPE_VECTOR:
            printf("VECTOR:\n");
            for (size_t i = 0; i < node->data.vector_elements.count; ++i) {
                print_ast(node->data.vector_elements.items[i], indent + 1);
            }
            break;

        case NODE_TYPE_STATEMENT_LIST:
            printf("STATEMENT_LIST:\n");
            for (size_t i = 0; i < node->data.statement_list.count; ++i) {
                // Check for NULL statement (e.g. from empty ';') before printing
                if (node->data.statement_list.items[i]) { 
                    print_ast(node->data.statement_list.items[i], indent + 1);
                } else {
                     print_indent(indent + 1);
                     printf("(Empty Statement)\n");
                }
            }
            break;

        case NODE_TYPE_IF:
            printf("IF\n");
            print_indent(indent + 1); printf("Condition:\n");
            print_ast(node->data.if_stmt.condition, indent + 2);
            print_indent(indent + 1); printf("Then Branch:\n");
            print_ast(node->data.if_stmt.if_branch, indent + 2);
            if (node->data.if_stmt.else_branch) {
                print_indent(indent + 1); printf("Else Branch:\n");
                print_ast(node->data.if_stmt.else_branch, indent + 2);
            }
            break;

        case NODE_TYPE_WHILE:
            printf("WHILE\n");
            print_indent(indent + 1); printf("Condition:\n");
            print_ast(node->data.while_loop.condition, indent + 2);
            print_indent(indent + 1); printf("Body:\n");
            print_ast(node->data.while_loop.loop_body, indent + 2);
            break;

        case NODE_TYPE_FUNC_CALL:
            printf("FUNC_CALL: %s\n",
                node->data.func_call.function_symbol ? node->data.func_call.function_symbol->name : "(null symbol!)");
            print_indent(indent + 1); printf("Arguments:\n");
            if (node->data.func_call.arguments.count > 0) {
                for (size_t i = 0; i < node->data.func_call.arguments.count; ++i) {
                    print_ast(node->data.func_call.arguments.items[i], indent + 2);
                }
            } else {
                print_indent(indent + 2); printf("(none)\n");
            }
            break;

        case NODE_TYPE_UNKNOWN:
        default:
            printf("UNKNOWN NODE TYPE: %d\n", node->type);
            break;
    }
} 