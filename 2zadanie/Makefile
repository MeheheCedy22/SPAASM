# compiler and flags setup
CC = gcc
CFLAGS = -Wall -Wformat -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-missing-field-initializers -pthread
DEBUG_FLAGS = -g -DDEBUG

# add a variable for arguments for valgrind
ARGS ?=

# executable names
TARGET = shell
DEBUG_TARGET = shell.out

# source files and object files
SOURCES = shell.c custom_queue.c
OBJECTS = $(SOURCES:.c=.o)
DEBUG_OBJECTS = $(SOURCES:.c=.debug.o)

# default target builds the shell executable
shell: $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

# compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# debug build with separate object files
%.debug.o: %.c
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c $< -o $@

# debug build creates a separate executable
debug: $(DEBUG_OBJECTS)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $(DEBUG_TARGET) $(DEBUG_OBJECTS)

# clean build artifacts
clean:
	rm -f $(TARGET) $(DEBUG_TARGET) $(OBJECTS) $(DEBUG_OBJECTS)

valgrind: debug
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose ./$(DEBUG_TARGET) $(ARGS)

help:
	@echo "About: Makefile for building ./shell executable - Simple interactive shell written in C using client-server architecture."
	@echo
	@echo "Usage:"
	@echo "  make           - Build the shell executable"
	@echo "  make debug     - Build with debug symbols and DEBUG defined as shell.out"
	@echo "  make valgrind  - Build debug version and run with Valgrind"
	@echo "  make valgrind ARGS=\"<args>\" - Pass arguments to the shell program"
	@echo "  make clean     - Remove all build artifacts"
	@echo "  make help      - Display this help message"

.PHONY: shell debug clean help valgrind