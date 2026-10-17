# Compiler
CC      := cc
CFLAGS  := -Wall -Wextra -std=c11 -O2
LDFLAGS :=

# Directories
SRC_DIR   := src
BUILD_DIR := build
BIN_DIR   := $(BUILD_DIR)/bin
OBJ_DIR   := $(BUILD_DIR)/obj

# Executable
TARGET    := $(BIN_DIR)/pong

# Files
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Targets
.PHONY: all build run clean

# Default target
all: build

# Build target
build: $(TARGET)

# Link targets
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compile targets
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Directory targets
$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

# Run target
run: build
	./$(TARGET)

# Clean target
clean:
	rm -rf $(BUILD_DIR)
