# Compiler and flags
CC = gcc
CFLAGS = -g -Wall -Wextra -std=c11 -I$(BUILDDIR) -I$(SRCDIR) -Iinclude
LDFLAGS = -lm
FLEX = flex
BISON = bison
# Use Windows commands via cmd /c for better compatibility
RMDIR = cmd /c rmdir /s /q
DEL = cmd /c del /q

# Project settings
TARGET = wizuallc
OUTPUT_C = output.c
OUTPUT_EXE = output_executable
SRCDIR = src
BUILDDIR = build
INCLUDEDIR = include # Added for clarity
RUNTIME_SRCS = $(SRCDIR)/runtime_viz.c # Runtime source files

# Source files
LEX_SRC = $(SRCDIR)/scanner.l
BISON_SRC = $(SRCDIR)/parser.y
C_SRCS = $(SRCDIR)/main.c $(SRCDIR)/ast.c $(SRCDIR)/symtab.c $(SRCDIR)/codegen.c $(RUNTIME_SRCS) # Add runtime source to compiler sources

# Generated files (in build directory)
LEX_GEN_C = $(BUILDDIR)/lex.yy.c
BISON_GEN_C = $(BUILDDIR)/y.tab.c
BISON_GEN_H = $(BUILDDIR)/y.tab.h

# Object files (in build directory)
C_OBJS = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(filter $(SRCDIR)/%.c, $(C_SRCS))) # Object files for compiler source
LEX_OBJ = $(BUILDDIR)/lex.yy.o
BISON_OBJ = $(BUILDDIR)/y.tab.o
RUNTIME_OBJS = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(RUNTIME_SRCS)) # Object files for runtime

OBJS = $(C_OBJS) $(LEX_OBJ) $(BISON_OBJ)

# Default target: build the main compiler executable
all: $(TARGET)

# Rule to build the main compiler executable
$(TARGET): $(OBJS) | $(BUILDDIR)
	@echo "Linking $(TARGET)..."
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# --- Compilation Rules ---

# Compile .c files from SRCDIR into .o files in BUILDDIR
# Updated CFLAGS to include INCLUDEDIR
$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(BISON_GEN_H) | $(BUILDDIR) $(INCLUDEDIR)/ast.h $(INCLUDEDIR)/symtab.h $(INCLUDEDIR)/codegen.h $(INCLUDEDIR)/runtime_viz.h
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -I$(INCLUDEDIR) -c $< -o $@

# Compile the generated lexer C code
$(LEX_OBJ): $(LEX_GEN_C) $(BISON_GEN_H) | $(BUILDDIR)
	@echo "Compiling lexer $(LEX_GEN_C)..."
	$(CC) $(CFLAGS) -I$(INCLUDEDIR) -c $< -o $@

# Compile the generated parser C code
$(BISON_OBJ): $(BISON_GEN_C) $(BISON_GEN_H) | $(BUILDDIR)
	@echo "Compiling parser $(BISON_GEN_C)..."
	$(CC) $(CFLAGS) -I$(INCLUDEDIR) -c $< -o $@

# --- File Generation Rules ---

# Generate the lexer C code from the .l file
# Depends on the Bison header for token definitions.
$(LEX_GEN_C): $(LEX_SRC) $(BISON_GEN_H) | $(BUILDDIR)
	@echo "Running Flex on $(LEX_SRC)..."
	$(FLEX) -o $@ $<

# Generate the parser C code and header file from the .y file
# This rule creates both BISON_GEN_C and BISON_GEN_H.
$(BISON_GEN_C) $(BISON_GEN_H): $(BISON_SRC) | $(BUILDDIR)
	@echo "Running Bison on $(BISON_SRC)..."
	$(BISON) -d -o $(BISON_GEN_C) $<

# --- Directory Creation ---

# Create the build directory if it doesn't exist
$(BUILDDIR):
	@echo "Creating directory $(BUILDDIR) if it does not exist..."
	@if not exist $(subst /,\,$(BUILDDIR)) mkdir $(subst /,\,$(BUILDDIR)) # Windows syntax
	# The @ suppresses echoing the command itself

# --- Rule to compile the generated C code ---
# Now depends on the runtime object file(s) as well
$(OUTPUT_EXE): $(OUTPUT_C) $(RUNTIME_OBJS) | $(BUILDDIR)
	@echo "Compiling generated code $(OUTPUT_C) and linking runtime..."
	$(CC) $(CFLAGS) $(OUTPUT_C) $(RUNTIME_OBJS) -o $@ $(LDFLAGS)
	@echo "Generated executable created: $@"

# Optional: A target to run the whole process (compiler + generated code compilation)
# Requires a default input file
# EXAMPLE_INPUT = examples/test2.wz # Define an example input
# run: $(TARGET) $(OUTPUT_EXE)
# 	./$(TARGET) $(EXAMPLE_INPUT)
# 	$(CC) $(CFLAGS) $(OUTPUT_C) -o $(OUTPUT_EXE) $(LDFLAGS)
# 	./$(OUTPUT_EXE)

# --- Clean Rule ---
clean:
	@echo "Cleaning up..."
	-$(DEL) $(TARGET).exe $(TARGET) 
	-$(DEL) $(OUTPUT_EXE).exe $(OUTPUT_EXE)
	-$(DEL) $(OUTPUT_C)
	-$(DEL) plot_data.txt # Remove generated data file
	-$(DEL) plot_output.png # Remove potential plot output
	-$(RMDIR) $(BUILDDIR)
	@echo "Clean complete."

# Phony targets: prevent conflicts with files named 'all' or 'clean'
.PHONY: all clean 