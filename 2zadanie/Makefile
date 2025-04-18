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

# Default target
all: $(TARGET)

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGET)

# Link the program
$(TARGET): $(OBJECTS)
    $(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

# Compile source files
%.o: %.c
    $(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
    rm -f $(TARGET) $(OBJECTS)

# Install (optional, adjust destination as needed)
install: $(TARGET)
    mkdir -p $(DESTDIR)/usr/local/bin
    cp $(TARGET) $(DESTDIR)/usr/local/bin/

# Help target
help:
    @echo "SPAASM 2zadanie shell program"
    @echo
    @echo "Targets:"
    @echo "  all     - Build the shell executable (default)"
    @echo "  debug   - Build with debug symbols and DEBUG defined"
    @echo "  clean   - Remove all build artifacts"
    @echo "  install - Install the program to /usr/local/bin"
    @echo "  help    - Display this help message"

# Phony targets
.PHONY: all debug clean install help