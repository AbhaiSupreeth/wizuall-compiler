#include "codegen.h"
#include "ast.h"
#include "symtab.h" // May need symbol info during generation
#include "runtime_viz.h" // Include runtime declarations
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h> // For va_list, va_start, va_end
#include <string.h>
#include <assert.h> // For assertions

// Structure to hold the result of expression code generation
typedef struct {
    char* code;         // C code fragment (literal, variable name, temporary)
    SymbolType type;    // Type of the result (SCALAR or VECTOR)
    int is_temporary;   // Flag indicating if 'code' refers to a temporary that might need cleanup
} ExprResult;

// Global file pointer for the output C file
static FILE *output_file = NULL;
static int temp_var_counter = 0; // Counter for temporary variable names
static int codegen_error_occurred = 0; // Global flag for semantic errors

//------------------------------------------------------------------------------
// Forward Declarations for All Static Functions
//------------------------------------------------------------------------------
static void report_codegen_error(const char *format, ...);
static void emit(int indent_level, const char *format, ...);
static char* new_temp_scalar_var();
static char* new_temp_vector_var();
static void generate_runtime_helpers();
static void declare_variables();
static void generate_cleanup_code();
static ExprResult generate_expression(ASTNode *node);
static void generate_statement(ASTNode *node);

//------------------------------------------------------------------------------
// Error Reporting Helper
//------------------------------------------------------------------------------
static void report_codegen_error(const char *format, ...) {
    fprintf(stderr, "Semantic Error: ");
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
    codegen_error_occurred = 1; // Set the flag
}

//------------------------------------------------------------------------------
// Code Emission Helper
//------------------------------------------------------------------------------
static void emit(int indent_level, const char *format, ...) {
    if (!output_file) return;
    for (int i = 0; i < indent_level; ++i) {
        fprintf(output_file, "    "); // 4 spaces per indent level
    }
    va_list args;
    va_start(args, format);
    vfprintf(output_file, format, args);
    va_end(args);
    fprintf(output_file, "\n"); // FIX: Use fprintf directly for newline
}

//------------------------------------------------------------------------------
// Temporary Variable Name Helpers
//------------------------------------------------------------------------------
static char* new_temp_scalar_var() {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "_ts%d", temp_var_counter++);
    return strdup(buffer);
}

static char* new_temp_vector_var() {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "_tv%d", temp_var_counter++);
    return strdup(buffer);
}

//------------------------------------------------------------------------------
// Generate Runtime Helper Functions (Vector Ops, etc.)
//------------------------------------------------------------------------------
static void generate_runtime_helpers() {
    emit(0, "// --- Runtime Helper Functions ---");
    // Vector Struct Definition
    emit(0, "typedef struct {");
    emit(1, "double* data;");
    emit(1, "size_t size;");
    emit(0, "} Vector;");
    emit(0, "");
    // Function to create/allocate a vector (simple version)
    emit(0, "// Creates a vector (allocates data). Caller must free using vector_free.");
    emit(0, "Vector vector_create(size_t size) {");
    emit(1, "Vector v;");
    emit(1, "v.size = size;");
    emit(1, "if (size > 0) {");
    emit(2, "v.data = (double*)malloc(size * sizeof(double));");
    emit(2, "if (!v.data) { perror(\"vector_create malloc failed\"); exit(1); }");
    emit(1, "} else {");
    emit(2, "v.data = NULL;");
    emit(1, "}");
    emit(1, "return v;");
    emit(0, "}");
    emit(0, "");
    // Function to free vector data
    emit(0, "// Frees the data array within a vector struct.");
    emit(0, "void vector_free_data(Vector *v) {");
    emit(1, "if (v && v->data) {");
    emit(2, "free(v->data);");
    emit(2, "v->data = NULL;");
    emit(2, "v->size = 0;");
    emit(1, "}");
    emit(0, "}");
    emit(0, "");
    // Function to assign/copy vector data (deep copy)
    emit(0, "// Assigns vector src to dst (deep copy). Frees existing dst data.");
    emit(0, "void vector_assign(Vector *dst, const Vector src) {");
    emit(1, "vector_free_data(dst); // Free existing data in destination");
    emit(1, "dst->size = src.size;");
    emit(1, "if (src.size > 0 && src.data) {");
    emit(2, "dst->data = (double*)malloc(src.size * sizeof(double));");
    emit(2, "if (!dst->data) { perror(\"vector_assign malloc failed\"); exit(1); }");
    emit(2, "memcpy(dst->data, src.data, src.size * sizeof(double));");
    emit(1, "} else {");
    emit(2, "dst->data = NULL;");
    emit(1, "}");
    emit(0, "}");
    emit(0, "");
    // --- Vector Arithmetic --- (Element-wise)
    // Vector Add
    emit(0, "// Adds two vectors element-wise. Creates a new result vector.");
    emit(0, "Vector vector_add(Vector v1, Vector v2) {");
    emit(1, "if (v1.size != v2.size) { fprintf(stderr, \"Runtime Error: Vector size mismatch for add (%ld != %ld)\\n\", (long)v1.size, (long)v2.size); exit(1); }");
    emit(1, "Vector result = vector_create(v1.size);");
    emit(1, "for (size_t i = 0; i < result.size; ++i) {");
    emit(2, "result.data[i] = v1.data[i] + v2.data[i];");
    emit(2, "}");
    emit(1, "return result;");
    emit(0, "}");
    emit(0, "");
    // Vector Subtract
    emit(0, "// Subtracts v2 from v1 element-wise. Creates a new result vector.");
    emit(0, "Vector vector_sub(Vector v1, Vector v2) {");
    emit(1, "if (v1.size != v2.size) { fprintf(stderr, \"Runtime Error: Vector size mismatch for sub (%ld != %ld)\\n\", (long)v1.size, (long)v2.size); exit(1); }");
    emit(1, "Vector result = vector_create(v1.size);");
    emit(1, "for (size_t i = 0; i < result.size; ++i) {");
    emit(2, "result.data[i] = v1.data[i] - v2.data[i];");
    emit(2, "}");
    emit(1, "return result;");
    emit(0, "}");
    emit(0, "");
    // Vector Multiply (Element-wise)
    emit(0, "// Multiplies two vectors element-wise. Creates a new result vector.");
    emit(0, "Vector vector_mul(Vector v1, Vector v2) {");
    emit(1, "if (v1.size != v2.size) { fprintf(stderr, \"Runtime Error: Vector size mismatch for mul (%ld != %ld)\\n\", (long)v1.size, (long)v2.size); exit(1); }");
    emit(1, "Vector result = vector_create(v1.size);");
    emit(1, "for (size_t i = 0; i < result.size; ++i) {");
    emit(2, "result.data[i] = v1.data[i] * v2.data[i];");
    emit(2, "}");
    emit(1, "return result;");
    emit(0, "}");
    emit(0, "");
    // Vector Divide (Element-wise)
    emit(0, "// Divides v1 by v2 element-wise. Creates a new result vector. Checks for division by zero.");
    emit(0, "Vector vector_div(Vector v1, Vector v2) {");
    emit(1, "if (v1.size != v2.size) { fprintf(stderr, \"Runtime Error: Vector size mismatch for div (%ld != %ld)\\n\", (long)v1.size, (long)v2.size); exit(1); }");
    emit(1, "Vector result = vector_create(v1.size);");
    emit(1, "for (size_t i = 0; i < result.size; ++i) {");
    emit(2, "if (v2.data[i] == 0.0) { fprintf(stderr, \"Runtime Error: Division by zero in vector division at index %ld\\n\", (long)i); exit(1); }");
    emit(2, "result.data[i] = v1.data[i] / v2.data[i];");
    emit(2, "}");
    emit(1, "return result;");
    emit(0, "}");
    emit(0, "");
    // --- Scalar-Vector Arithmetic --- (Broadcasting scalar)
    // Add Scalar to Vector
    emit(0, "// Adds scalar to each element of a vector. Creates new vector.");
    emit(0, "Vector vector_add_scalar(Vector v, double s) {");
    emit(1, "Vector result = vector_create(v.size);");
    emit(1, "for (size_t i = 0; i < result.size; ++i) {");
    emit(2, "result.data[i] = v.data[i] + s;");
    emit(1, "}");
    emit(1, "return result;");
    emit(0, "}");
    emit(0, "");
    // --- Runtime Data Reading ---
    emit(0, "// Reads a vector (space-separated doubles) from stdin until newline.");
    emit(0, "Vector runtime_read_vector() {");
    emit(1, "Vector v = vector_create(0); // Start with empty vector");
    emit(1, "double num;");
    emit(1, "size_t capacity = 0;");
    emit(1, "printf(\"\">>> Enter vector elements separated by spaces, then press Enter:\\n\");"); // Prompt with escaped quote
    emit(1, "int status;");
    emit(1, "while ((status = scanf(\"%%lf\", &num)) == 1) { // Escaped % in scanf format string");
    emit(2, "// Resize buffer if needed (simple doubling strategy)");
    emit(2, "if (v.size >= capacity) {");
    emit(3, "capacity = (capacity == 0) ? 8 : capacity * 2;");
    emit(3, "double* new_data = (double*)realloc(v.data, capacity * sizeof(double));");
    emit(3, "if (!new_data) { perror(\"read_vector realloc failed\"); vector_free_data(&v); exit(1); }");
    emit(3, "v.data = new_data;");
    emit(2, "}");
    emit(2, "v.data[v.size++] = num;");
    emit(2, "// Stop reading on newline character");
    emit(2, "char next_char = getchar();");
    emit(2, "if (next_char == '\n' || next_char == EOF) { break; }");
    emit(2, "ungetc(next_char, stdin); // Put back non-newline char");
    emit(1, "}");
    emit(1, "// Handle case where scanf failed before reading any number or after some numbers");
    emit(1, "if (status != 1 && v.size == 0) { // Check if scanf failed immediately");
    emit(2, "fprintf(stderr, \"Runtime Error: Invalid input - expected numbers.\\n\");");
    emit(1, "}");
    emit(1, "// Clear remaining input buffer until newline or EOF");
    emit(1, "int c;");
    emit(1, "while ((c = getchar()) != '\n' && c != EOF);"); 
    emit(1, "printf(\"\"<<< Read %ld elements.\\n\"\", (long)v.size); // Confirmation with escaped quotes");
    emit(1, "return v;");
    emit(0, "}");
    emit(0, "");
    emit(0, "// --- End Runtime Helper Functions ---");
    emit(0, "");
}

//------------------------------------------------------------------------------
// Generate Variable Declarations
//------------------------------------------------------------------------------
static void declare_variables() {
    emit(1, "// --- Variable Declarations ---");
    Symbol *current = symbol_get_list_head();
    while (current != NULL) {
        if (current->type == SYMBOL_TYPE_SCALAR) {
            emit(1, "double %s = 0.0;", current->name);
        } else if (current->type == SYMBOL_TYPE_VECTOR) {
            emit(1, "Vector %s = { NULL, 0 };", current->name);
        } else { // Should not happen
             emit(1, "// WARNING: Undefined symbol type for %s", current->name);
        }
        current = current->next;
    }
    // Declare temporary variables needed by generate_expression
    emit(1, "// Temporary variables (up to %d scalar, %d vector)", 20, 20); // Added comment
    for(int i = 0; i < 20; ++i) { // Increased number of temps
       emit(1, "double _ts%d;", i);
       emit(1, "Vector _tv%d = { NULL, 0 };", i);
    }
    emit(1, "// ---------------------------");
    emit(0, ""); // Add newline after declarations
}

//------------------------------------------------------------------------------
// Generate Cleanup Code (Freeing Vectors)
//------------------------------------------------------------------------------
static void generate_cleanup_code() {
     emit(1, "// --- Cleanup Code ---");
     Symbol *current = symbol_get_list_head();
     while (current != NULL) {
         if (current->type == SYMBOL_TYPE_VECTOR) {
             emit(1, "vector_free_data(&%s);", current->name);
         }
         current = current->next;
     }
     // Free temporary vectors (Approximate - better tracking needed)
     for(int i = 0; i < temp_var_counter; ++i) { 
          emit(1, "vector_free_data(&_tv%d);", i);
     }
     emit(1, "// --------------------");
     emit(0, "");
}

//------------------------------------------------------------------------------
// Generate C code for an Expression Node
// Returns: An ExprResult struct containing C code and type information.
//          The 'code' field must be freed by the caller if is_temporary is true!
//------------------------------------------------------------------------------
static ExprResult generate_expression(ASTNode *node) {
    ExprResult result = {strdup("0.0"), SYMBOL_TYPE_SCALAR, 1}; // Default to scalar 0, mark as temp
    char static_buffer[256]; 

    if (!node) {
        result.code = strdup("/* null expr */"); 
        result.type = SYMBOL_TYPE_SCALAR;
        result.is_temporary = 1;
        return result;
    }

    ExprResult left_res, right_res;
    memset(&left_res, 0, sizeof(ExprResult));
    memset(&right_res, 0, sizeof(ExprResult));

    switch (node->type) {
        case NODE_TYPE_NUMBER:
            snprintf(static_buffer, sizeof(static_buffer), "%f", node->data.number_value);
            result.code = strdup(static_buffer);
            result.type = SYMBOL_TYPE_SCALAR;
            result.is_temporary = 1; // Literal code needs freeing by caller
            break;

        case NODE_TYPE_IDENTIFIER:
            assert(node->data.identifier_symbol != NULL); 
            result.code = strdup(node->data.identifier_symbol->name);
            result.type = node->data.identifier_symbol->type; // Get type from symbol table
            result.is_temporary = 1; // Variable name code needs freeing
            break;

        case NODE_TYPE_VECTOR: { 
            char* temp_vec_var_name = new_temp_vector_var();
            size_t count = node->data.vector_elements.count;
            char temp_array_name[64];
            snprintf(temp_array_name, sizeof(temp_array_name), "%s_init_data", temp_vec_var_name);

            emit(1, "// Generating vector literal for %s", temp_vec_var_name);
            emit(1, "double %s[] = {", temp_array_name);
            for (size_t i = 0; i < count; ++i) {
                 // Assume elements are scalar for now
                 ExprResult elem_res = generate_expression(node->data.vector_elements.items[i]);
                 if(elem_res.type != SYMBOL_TYPE_SCALAR) {
                     emit(2, "/* WARNING: Non-scalar in vector literal */ 0.0%s", (i == count - 1) ? "" : ",");
                 } else {
                     emit(2, "%s%s", elem_res.code, (i == count - 1) ? "" : ",");
                 }
                 if(elem_res.is_temporary) free(elem_res.code);
            }
            emit(1, "};");
            emit(1, "vector_assign(&%s, (Vector){ %s, %ld });", temp_vec_var_name, temp_array_name, count);

            result.code = strdup(temp_vec_var_name);
            result.type = SYMBOL_TYPE_VECTOR;
            result.is_temporary = 0; // It's a declared temp variable, not just code fragment
            break;
        }

        case NODE_TYPE_BINARY_OP: {
            left_res = generate_expression(node->data.binary_op.left);
            right_res = generate_expression(node->data.binary_op.right);

            // Type checking and operation dispatch
            if (left_res.type == SYMBOL_TYPE_SCALAR && right_res.type == SYMBOL_TYPE_SCALAR) {
                char* temp_scalar_var = new_temp_scalar_var();
                emit(1, "%s = (%s) %c (%s);", 
                     temp_scalar_var, left_res.code, node->data.binary_op.op, right_res.code);
                result.code = strdup(temp_scalar_var);
                result.type = SYMBOL_TYPE_SCALAR;
                result.is_temporary = 0; // It's a declared temp variable
            }
            // Vector + Vector
            else if (left_res.type == SYMBOL_TYPE_VECTOR && right_res.type == SYMBOL_TYPE_VECTOR) {
                 char* temp_vector_var = new_temp_vector_var();
                 const char* op_func = "";
                 switch(node->data.binary_op.op) {
                     case '+': op_func = "vector_add"; break;
                     case '-': op_func = "vector_sub"; break;
                     case '*': op_func = "vector_mul"; break;
                     case '/': op_func = "vector_div"; break;
                     default: emit(1, "// ERROR: Unsupported vector binary op '%c'", node->data.binary_op.op); break;
                 }
                 if (strlen(op_func) > 0) {
                    emit(1, "%s = %s(%s, %s);", temp_vector_var, op_func, left_res.code, right_res.code);
                    result.code = strdup(temp_vector_var);
                    result.type = SYMBOL_TYPE_VECTOR;
                    result.is_temporary = 0; // It's a declared temp variable
                 } else {
                    result.code = strdup("/* Invalid vector op */");
                    result.type = SYMBOL_TYPE_VECTOR; // Or undefined?
                    result.is_temporary = 1; 
                 }
            }
            // Vector + Scalar (Example - only handling add for now)
            else if (left_res.type == SYMBOL_TYPE_VECTOR && right_res.type == SYMBOL_TYPE_SCALAR && node->data.binary_op.op == '+') {
                char* temp_vector_var = new_temp_vector_var();
                emit(1, "%s = vector_add_scalar(%s, %s);", temp_vector_var, left_res.code, right_res.code);
                result.code = strdup(temp_vector_var);
                result.type = SYMBOL_TYPE_VECTOR;
                result.is_temporary = 0;
            }
             // Scalar + Vector (Example - only handling add for now)
            else if (left_res.type == SYMBOL_TYPE_SCALAR && right_res.type == SYMBOL_TYPE_VECTOR && node->data.binary_op.op == '+') {
                 char* temp_vector_var = new_temp_vector_var();
                 // Assuming vector_add_scalar is commutative for addition, reuse it
                 emit(1, "%s = vector_add_scalar(%s, %s);", temp_vector_var, right_res.code, left_res.code);
                 result.code = strdup(temp_vector_var);
                 result.type = SYMBOL_TYPE_VECTOR;
                 result.is_temporary = 0;
            }
            else if ( (left_res.type == SYMBOL_TYPE_VECTOR && right_res.type == SYMBOL_TYPE_SCALAR) || 
                      (left_res.type == SYMBOL_TYPE_SCALAR && right_res.type == SYMBOL_TYPE_VECTOR) ) {
                 // Scalar-Vector Operations (Example: Addition only)
                 if (node->data.binary_op.op == '+') {
                     char* temp_vector_var = new_temp_vector_var();
                     emit(1, "%s = vector_add_scalar(%s, %s);", temp_vector_var, left_res.code, right_res.code);
                     result.code = strdup(temp_vector_var);
                     result.type = SYMBOL_TYPE_VECTOR;
                     result.is_temporary = 0;
                 } else {
                     report_codegen_error("Unsupported binary operation '%c' between scalar and vector.", node->data.binary_op.op);
                     // result is already default error value
                 }
            } else {
                report_codegen_error("Type mismatch for binary operation '%c' (LHS: %d, RHS: %d).", 
                    node->data.binary_op.op, left_res.type, right_res.type);
                // result is already default error value
            }

            // Free the code strings returned by recursive calls if they were temps
            if(left_res.is_temporary) free(left_res.code);
            if(right_res.is_temporary) free(right_res.code);
            break;
        }

        case NODE_TYPE_UNARY_OP: {
            left_res = generate_expression(node->data.unary_op.operand);
            // Assuming scalar negation for now
            if (left_res.type == SYMBOL_TYPE_SCALAR && node->data.unary_op.op == '-') {
                 char* temp_scalar_var = new_temp_scalar_var();
                 emit(1, "%s = %c(%s);", 
                      temp_scalar_var, node->data.unary_op.op, left_res.code);
                 result.code = strdup(temp_scalar_var);
                 result.type = SYMBOL_TYPE_SCALAR;
                 result.is_temporary = 0;
            } else {
                report_codegen_error("Unsupported unary operation '%c' or type mismatch (Type: %d).", 
                      node->data.unary_op.op, left_res.type);
                 result.code = strdup("/* type error */ 0.0");
                 result.type = SYMBOL_TYPE_SCALAR;
                 result.is_temporary = 1;
            }
            if(left_res.is_temporary) free(left_res.code);
            break;
        }

        case NODE_TYPE_FUNC_CALL: {
            assert(node->data.func_call.function_symbol != NULL);
            const char* func_name = node->data.func_call.function_symbol->name;
            size_t arg_count = node->data.func_call.arguments.count;
            
            emit(1, "// Generating function call: %s()", func_name);

            // Handle built-in/special functions first
            if (strcmp(func_name, "read_vector") == 0) {
                if (arg_count == 0) {
                    char* temp_vector_var = new_temp_vector_var();
                    emit(1, "%s = runtime_read_vector();", temp_vector_var);
                    result.code = strdup(temp_vector_var);
                    result.type = SYMBOL_TYPE_VECTOR;
                    result.is_temporary = 0; // It's a declared temp variable
                } else {
                    report_codegen_error("read_vector() expects 0 arguments, got %ld.", arg_count);
                    result.code = strdup("/* invalid read_vector call */");
                    result.type = SYMBOL_TYPE_VECTOR; // Still technically returns vector?
                    result.is_temporary = 1;
                }
                 // Skip argument processing & generic call for this specific function
                 break; // Exit the FUNC_CALL case directly
            }
            
            // --- Argument processing and call generation for OTHER functions ---
            // 1. Generate code for all arguments first
            ExprResult* arg_results = (ExprResult*)calloc(arg_count, sizeof(ExprResult));
            if (arg_count > 0 && !arg_results) { perror("malloc failed for arg results"); exit(1); }
            
            for (size_t i = 0; i < arg_count; ++i) {
                arg_results[i] = generate_expression(node->data.func_call.arguments.items[i]);
            }
            
            // 2. Handle specific known functions (like scatter_plot)
            if (strcmp(func_name, "scatter_plot") == 0) {
                if (arg_count == 2 && 
                    arg_results[0].type == SYMBOL_TYPE_VECTOR && 
                    arg_results[1].type == SYMBOL_TYPE_VECTOR) {
                    
                    emit(1, "c_scatter_plot(%s.data, %s.size, %s.data, %s.size);", 
                         arg_results[0].code, arg_results[0].code, 
                         arg_results[1].code, arg_results[1].code);
                    // scatter_plot likely returns void or is used only as statement
                    result.code = strdup("/* scatter_plot call */"); // No meaningful C value
                    result.type = SYMBOL_TYPE_SCALAR; // Or a VOID type? Scalar for now.
                    result.is_temporary = 1; // Needs freeing
                } else {
                    report_codegen_error("scatter_plot() expects 2 vector arguments.");
                    result.code = strdup("/* invalid scatter_plot call */");
                    result.type = SYMBOL_TYPE_SCALAR;
                    result.is_temporary = 1;
                }
            } 
            // Handle generic/other external functions (assuming scalar return)
            else {
                // Build C argument string
                size_t arg_str_capacity = arg_count * 25 + 5; 
                char* arg_str = (char*)malloc(arg_str_capacity);
                if (!arg_str) { perror("malloc failed for arg string"); exit(1); }
                arg_str[0] = '\0'; 
                for (size_t i = 0; i < arg_count; ++i) {
                    if (i > 0) { strncat(arg_str, ", ", arg_str_capacity - strlen(arg_str) - 1); }
                    if (arg_results[i].type == SYMBOL_TYPE_VECTOR) {
                        char vec_arg_part[128];
                        snprintf(vec_arg_part, sizeof(vec_arg_part), "%s.data, %s.size", 
                                 arg_results[i].code, arg_results[i].code);
                        strncat(arg_str, vec_arg_part, arg_str_capacity - strlen(arg_str) - 1);
                    } else {
                        strncat(arg_str, arg_results[i].code, arg_str_capacity - strlen(arg_str) - 1);
                    }
                }

                // Assume scalar return, store in temp
                char* temp_scalar_var = new_temp_scalar_var();
                emit(1, "%s = %s(%s); // Generic function call result", 
                     temp_scalar_var, func_name, arg_str);
                free(arg_str); // Free the built C argument string

                result.code = strdup(temp_scalar_var);
                result.type = SYMBOL_TYPE_SCALAR; 
                result.is_temporary = 0; // It's a declared temp variable
            }

            // 3. Clean up temporary argument results
            for (size_t i = 0; i < arg_count; ++i) {
                if (arg_results[i].is_temporary) {
                    free(arg_results[i].code);
                }
            }
            free(arg_results); // Free the array itself
            break;
        }

        default:
            emit(1, "// Expression generation not implemented for node type %d", node->type);
            snprintf(static_buffer, sizeof(static_buffer), "/* UNIMPL EXPR %d */", node->type);
            result.code = strdup(static_buffer);
            result.type = SYMBOL_TYPE_SCALAR; // Default
            result.is_temporary = 1;
            break;
    }
    return result;
}

//------------------------------------------------------------------------------
// Generate C code for a single Statement Node
//------------------------------------------------------------------------------
static void generate_statement(ASTNode *node) {
    if (!node) return;

    ExprResult expr_res; // Reusable struct for expression results
    memset(&expr_res, 0, sizeof(ExprResult));

    if (codegen_error_occurred) return; // Stop processing if error already found

    switch (node->type) {
        case NODE_TYPE_STATEMENT_LIST: // Handle blocks directly if passed
            emit(1, "{ // Start block");
            for (size_t i = 0; i < node->data.statement_list.count; ++i) {
                 generate_statement(node->data.statement_list.items[i]); // Indentation handled by called generate_statement
            }
            emit(1, "} // End block");
            break;

        case NODE_TYPE_ASSIGNMENT: {
            assert(node->data.assignment.target_symbol != NULL);
            Symbol* target_sym = node->data.assignment.target_symbol;
            const char* target_var = target_sym->name;
            
            emit(1, "// Assignment to %s (Type: %d)", target_var, target_sym->type);
            expr_res = generate_expression(node->data.assignment.expression);
            if (codegen_error_occurred) { // Check if expr generation failed
                if (expr_res.is_temporary) free(expr_res.code);
                break; 
            }
            emit(1, "// RHS Type: %d", expr_res.type);

            // Type checking and assignment
            if (!((target_sym->type == SYMBOL_TYPE_SCALAR && expr_res.type == SYMBOL_TYPE_SCALAR) || 
                  (target_sym->type == SYMBOL_TYPE_VECTOR && expr_res.type == SYMBOL_TYPE_VECTOR))) {
                report_codegen_error("Type mismatch in assignment to '%s' (Target: %d, RHS: %d)", 
                    target_var, target_sym->type, expr_res.type);
            } else {
                // Emit the actual assignment
                if (target_sym->type == SYMBOL_TYPE_SCALAR) {
                    emit(1, "%s = %s;", target_var, expr_res.code);
                } else {
                    emit(1, "vector_assign(&%s, %s);", target_var, expr_res.code);
                     // If RHS was a temporary vector result, it might need freeing *after* assign
                     // This requires more careful temporary management than currently implemented.
                     // emit(1, "// vector_free_data(&%s); // Potentially free RHS temp if needed", expr_res.code);
                }
            }

            // Free the C code string from RHS if it was dynamically allocated
            if (expr_res.is_temporary) {
                free(expr_res.code);
            }
            break;
        }

        // Handle standalone expressions/calls (value discarded)
        case NODE_TYPE_NUMBER:     
        case NODE_TYPE_IDENTIFIER: 
        case NODE_TYPE_VECTOR:     
        case NODE_TYPE_BINARY_OP:  
        case NODE_TYPE_UNARY_OP:   
        case NODE_TYPE_FUNC_CALL:  // generate_expression handles the call
            emit(1, "// Expression/Call statement (value discarded)");
            expr_res = generate_expression(node); 
            // No need to cast void for specific calls like scatter_plot if handled in generate_expression
            if (node->type != NODE_TYPE_FUNC_CALL || strcmp(node->data.func_call.function_symbol->name, "scatter_plot") != 0) {
                 emit(1, "(void)(%s); // Discard result", expr_res.code);
            }
            // Cleanup temporary result code if needed
            if (expr_res.is_temporary) {
                free(expr_res.code);
            }
            // Cleanup temporary *vector data* if the discarded expression result was a temp vector variable
            if (expr_res.type == SYMBOL_TYPE_VECTOR && expr_res.is_temporary == 0) {
                 emit(1, "// vector_free_data(&%s); // Free temp vector result if needed", expr_res.code);
             }
             break;

        case NODE_TYPE_IF: {
            emit(1, "// If statement");
            // Generate condition code
            expr_res = generate_expression(node->data.if_stmt.condition);
            if (codegen_error_occurred) {
                emit(1, "if (0) { // Type error in condition");
                break;
            }
            if (expr_res.type != SYMBOL_TYPE_SCALAR) {
                report_codegen_error("Non-scalar condition used for IF statement.");
                emit(1, "if (0) { // Type error in condition");
            } else {
                // Check scalar result against 0.0 for truthiness
                emit(1, "if ((%s) != 0.0) {", expr_res.code);
            }
            if (expr_res.is_temporary) free(expr_res.code);

            // Generate "then" branch (should be a statement list / block)
            generate_statement(node->data.if_stmt.if_branch); 
            emit(1, "}"); // Close "then" block

            // Generate optional "else" branch
            if (node->data.if_stmt.else_branch) {
                emit(1, "else {");
                generate_statement(node->data.if_stmt.else_branch);
                emit(1, "}"); // Close "else" block
            }
            break;
        }

        case NODE_TYPE_WHILE: {
             emit(1, "// While loop");
             // Generate condition code
             expr_res = generate_expression(node->data.while_loop.condition);
              if (codegen_error_occurred) {
                 emit(1, "while (0) { // Type error in condition");
                 break;
             }
             if (expr_res.type != SYMBOL_TYPE_SCALAR) {
                 report_codegen_error("Non-scalar condition used for WHILE statement.");
                 emit(1, "while (0) { // Type error in condition");
             } else {
                 // Check scalar result against 0.0 for truthiness
                 emit(1, "while ((%s) != 0.0) {", expr_res.code);
             }
             // Note: We don't free expr_res.code here because it's used inside the loop!
             // If it refers to a temporary variable, that variable needs to be available
             // and potentially recalculated in each iteration. This simple codegen 
             // doesn't handle that complexity well. A better approach would re-evaluate
             // the condition *inside* the generated C loop if it involves temps.
             // For now, we assume the condition code refers to variables or literals 
             // that are valid throughout the loop.
             // if (expr_res.is_temporary) free(expr_res.code); // DON'T FREE YET

            // Generate loop body (should be a statement list / block)
             generate_statement(node->data.while_loop.loop_body);
             emit(1, "} // End while");

             // If the condition involved temporary C code that needs freeing, it's complex.
             // We leak it for now in this simple version if is_temporary was true.
             break;
        }

        default:
             emit(1, "// Statement generation not implemented for node type %d", node->type);
            break;
    }
}

//------------------------------------------------------------------------------
// Main code generation function (entry point)
//------------------------------------------------------------------------------
void generate_code(ASTNode *ast_root, const char *output_filename) {
    if (!ast_root || ast_root->type != NODE_TYPE_STATEMENT_LIST) {
        fprintf(stderr, "Error: Cannot generate code from empty or non-statement-list root AST.\n");
        return;
    }

    output_file = fopen(output_filename, "w");
    if (!output_file) {
        perror("Failed to open output C file");
        return;
    }
    temp_var_counter = 0; 
    codegen_error_occurred = 0;

    // Emit C Boilerplate & Helpers
    emit(0, "// Generated by WIZUALL Compiler");
    emit(0, "");
    emit(0, "#include <stdio.h>");
    emit(0, "#include <stdlib.h>");
    emit(0, "#include <math.h>"); 
    emit(0, "#include <string.h> // For memcpy");
    emit(0, "#include <stddef.h> // For size_t");
    emit(0, "#include <assert.h>");
    emit(0, "#include \"runtime_viz.h\" // Include viz function declarations");
    emit(0, "");
    generate_runtime_helpers(); 
    
    // Main function start
    emit(0, "// --- Main Program ---");
    emit(0, "int main() {");
    emit(1, "printf(\"Executing generated code...\\n\");");
    emit(0, "");

    // Declare variables 
    declare_variables();
    
    // Generate code for program statements
    emit(1, "// --- Program Statements ---");
    generate_statement(ast_root); // Use the statement generator for the root list
    emit(1, "// ------------------------");
    emit(0, "");

    // Generate cleanup code (freeing vectors)
    generate_cleanup_code();

    emit(1, "printf(\"Code execution finished.\\n\");");
    emit(1, "return 0;");
    emit(0, "} // end main");

    // Close the file
    fclose(output_file);
    output_file = NULL;

    if (codegen_error_occurred) {
        fprintf(stderr, "Code generation failed due to semantic errors. Output file '%s' may be incomplete or incorrect.\n", output_filename);
        // remove(output_filename);
    } else {
        printf("Code generation complete: %s\n", output_filename);
    }
} 