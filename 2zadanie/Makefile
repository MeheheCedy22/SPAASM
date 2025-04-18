# Makefile for SPAASM 2zadanie project

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wformat -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-missing-field-initializers -pthread
DEBUG_FLAGS = -g -DDEBUG

# Executable name
TARGET = shell

# Source files and object files
SOURCES = main.c custom_queue.c
OBJECTS = $(SOURCES:.c=.o)

# Default target builds the shell executable
shell: $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean shell

# Clean build artifacts
clean:
	rm -f $(TARGET) $(OBJECTS)

# Help target
help:
	@echo "SPAASM 2zadanie shell program"
	@echo
	@echo "Usage:"
	@echo "  make           - Build the shell executable"
	@echo "  make debug     - Build with debug symbols and DEBUG defined"
	@echo "  make clean     - Remove all build artifacts"
	@echo "  make help      - Display this help message"

# Phony targets
.PHONY: shell debug clean help