#include "symtab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Head of the global symbol list
static Symbol *symbol_list_head = NULL;

//------------------------------------------------------------------------------
// Helper Function to free symbol data (vector)
//------------------------------------------------------------------------------
static void free_symbol_value_data(Symbol *sym) {
    if (sym && sym->type == SYMBOL_TYPE_VECTOR && sym->value.vector_value.data) {
        free(sym->value.vector_value.data);
        sym->value.vector_value.data = NULL;
        sym->value.vector_value.size = 0;
    }
    // No need to explicitly free scalar, it's part of the union
}

//------------------------------------------------------------------------------
// Symbol Table Functions (Implementations)
//------------------------------------------------------------------------------

Symbol *symbol_lookup(const char *name) {
    if (!name) return NULL;
    Symbol *current = symbol_list_head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL; // Not found
}

Symbol *symbol_insert(const char *name) {
    if (!name) {
        fprintf(stderr, "Error: Attempted to insert NULL symbol name.\n");
        exit(EXIT_FAILURE);
    }

    Symbol *sym = symbol_lookup(name);
    if (sym) {
        return sym; // Symbol already exists
    }

    // Symbol not found, create a new one
    sym = (Symbol *)malloc(sizeof(Symbol));
    if (!sym) {
        perror("Failed to allocate memory for new symbol");
        exit(EXIT_FAILURE);
    }

    sym->name = strdup(name);
    if (!sym->name) {
        perror("Failed to duplicate symbol name");
        free(sym);
        exit(EXIT_FAILURE);
    }

    // Initialize to default: scalar 0.0
    sym->type = SYMBOL_TYPE_SCALAR;
    sym->value.scalar_value = 0.0;
    sym->next = NULL; // Initialize next pointer

    // Insert at the head of the list (simple approach)
    sym->next = symbol_list_head;
    symbol_list_head = sym;

    return sym;
}

void symbol_set_scalar(Symbol *sym, double val) {
    if (!sym) return;
    
    // Free old data if it was a vector
    free_symbol_value_data(sym);

    sym->type = SYMBOL_TYPE_SCALAR;
    sym->value.scalar_value = val;
}

void symbol_set_vector(Symbol *sym, const double *data, size_t size) {
    if (!sym) return;

    // Free old data (scalar or vector)
    free_symbol_value_data(sym);

    sym->type = SYMBOL_TYPE_VECTOR;
    sym->value.vector_value.size = size;

    if (size == 0 || !data) {
        // Handle empty vector case
        sym->value.vector_value.data = NULL; 
    } else {
        // Allocate memory and copy the data
        sym->value.vector_value.data = (double *)malloc(size * sizeof(double));
        if (!sym->value.vector_value.data) {
            perror("Failed to allocate memory for vector data");
            // Should ideally handle error more gracefully, but exit for now
            exit(EXIT_FAILURE);
        }
        memcpy(sym->value.vector_value.data, data, size * sizeof(double));
    }
}

void symbol_print_table() {
    printf("--- Symbol Table ---\n");
    Symbol *current = symbol_list_head;
    if (!current) {
        printf("(empty)\n");
    } else {
        while (current != NULL) {
            printf(" '%s' (%s): ", current->name, 
                   (current->type == SYMBOL_TYPE_SCALAR) ? "scalar" : 
                   (current->type == SYMBOL_TYPE_VECTOR) ? "vector" : "undefined");
            
            if (current->type == SYMBOL_TYPE_SCALAR) {
                printf("%f\n", current->value.scalar_value);
            } else if (current->type == SYMBOL_TYPE_VECTOR) {
                printf("[%ld] = {", current->value.vector_value.size);
                for (size_t i = 0; i < current->value.vector_value.size; ++i) {
                    printf("%f%s", current->value.vector_value.data[i], 
                           (i == current->value.vector_value.size - 1) ? "" : ", ");
                }
                printf("}\n");
            } else {
                printf("(no value)\n");
            }
            current = current->next;
        }
    }
    printf("--------------------\n");
}

void symbol_table_destroy() {
    Symbol *current = symbol_list_head;
    Symbol *next_sym;
    while (current != NULL) {
        next_sym = current->next;
        
        free(current->name); // Free the duplicated name
        free_symbol_value_data(current); // Free vector data if any
        free(current); // Free the symbol struct itself
        
        current = next_sym;
    }
    symbol_list_head = NULL; // Reset the head pointer
} 