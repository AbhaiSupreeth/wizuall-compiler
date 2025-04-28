#ifndef SYMTAB_H
#define SYMTAB_H

#include <stdlib.h> // For size_t

//------------------------------------------------------------------------------
// Symbol Type Enum
//------------------------------------------------------------------------------
typedef enum {
    SYMBOL_TYPE_UNDEFINED, // Should not happen after insert
    SYMBOL_TYPE_SCALAR,
    SYMBOL_TYPE_VECTOR
} SymbolType;

//------------------------------------------------------------------------------
// Symbol Structure
//------------------------------------------------------------------------------
typedef struct Symbol {
    char *name;            // Symbol name (identifier)
    SymbolType type;       // Current type of the symbol

    union {
        double scalar_value; // Value if type is SCALAR
        struct {
            double *data;   // Dynamically allocated array of vector elements
            size_t size;    // Number of elements in the vector
        } vector_value;     // Value if type is VECTOR
    } value;

    struct Symbol *next;   // Pointer to the next symbol in the linked list
} Symbol;

//------------------------------------------------------------------------------
// Symbol Table Functions (Declarations)
//------------------------------------------------------------------------------

/**
 * @brief Looks up a symbol by name in the global symbol table.
 *
 * @param name The name of the symbol to find.
 * @return Symbol* Pointer to the symbol if found, NULL otherwise.
 */
Symbol *symbol_lookup(const char *name);

/**
 * @brief Inserts a new symbol into the global table or returns it if it exists.
 *        New symbols are initialized to scalar 0.0.
 *
 * @param name The name of the symbol to insert.
 * @return Symbol* Pointer to the newly inserted or existing symbol. Exits on error.
 */
Symbol *symbol_insert(const char *name);

/**
 * @brief Sets the value of a symbol to a scalar.
 *        Frees any existing vector data associated with the symbol.
 *
 * @param sym Pointer to the symbol to modify.
 * @param val The scalar value to set.
 */
void symbol_set_scalar(Symbol *sym, double val);

/**
 * @brief Sets the value of a symbol to a vector.
 *        Makes a copy of the provided data.
 *        Frees any existing vector/scalar data associated with the symbol.
 *
 * @param sym Pointer to the symbol to modify.
 * @param data Pointer to the array of doubles representing the vector data.
 * @param size The number of elements in the data array.
 */
void symbol_set_vector(Symbol *sym, const double *data, size_t size);

/**
 * @brief Prints the contents of the symbol table (for debugging).
 */
void symbol_print_table();

/**
 * @brief Frees all memory associated with the global symbol table.
 */
void symbol_table_destroy();

/**
 * @brief Returns the head of the symbol list (for iteration).
 *        Use with caution, modifying the list directly is not advised.
 * 
 * @return Symbol* The head of the list, or NULL if empty.
 */
Symbol* symbol_get_list_head();


#endif // SYMTAB_H 