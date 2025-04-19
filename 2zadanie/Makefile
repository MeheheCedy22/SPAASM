# TODO: rename 1st line in the `help` the echoing to "SPAASM 2zadanie shell program"

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wformat -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-missing-field-initializers -pthread
DEBUG_FLAGS = -g -DDEBUG

# Add a variable for arguments for valgrind
ARGS ?=

# Executable names
TARGET = shell
DEBUG_TARGET = shell.out

# Source files and object files
SOURCES = shell.c custom_queue.c
OBJECTS = $(SOURCES:.c=.o)
DEBUG_OBJECTS = $(SOURCES:.c=.debug.o)

# Default target builds the shell executable
shell: $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build with separate object files
%.debug.o: %.c
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c $< -o $@

# Debug build creates a separate executable
debug: $(DEBUG_OBJECTS)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $(DEBUG_TARGET) $(DEBUG_OBJECTS)

# Clean build artifacts
clean:
	rm -f $(TARGET) $(DEBUG_TARGET) $(OBJECTS) $(DEBUG_OBJECTS)

# Valgrind target
valgrind: debug
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose ./$(DEBUG_TARGET) $(ARGS)

# Help target
help:
	@echo "SPAASM 2zadanie shell program"
	@echo
	@echo "Usage:"
	@echo "  make           - Build the shell executable"
	@echo "  make debug     - Build with debug symbols and DEBUG defined as shell.out"
	@echo "  make valgrind  - Build debug version and run with Valgrind"
	@echo "  make clean     - Remove all build artifacts"
	@echo "  make help      - Display this help message"

# Phony targets
.PHONY: shell debug clean help valgrind