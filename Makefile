# Makefile for thread pool project

# Compiler and flags
CC = cc
CFLAGS = -Wall -Wextra -Wpedantic -pthread

# Directories
SRC_DIR = src
OBJ_DIR = obj
INCLUDE_DIR = include

# Files
LIB_NAME = libthreadpool.a

# Sources and object files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Default target
all: $(LIB_NAME)

# Build static library
$(LIB_NAME): $(OBJS)
	ar rcs $@ $^

# Compile object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@


# Clean up build artifacts
clean:
	rm -rf $(OBJ_DIR) $(LIB_NAME)

.PHONY: all clean
