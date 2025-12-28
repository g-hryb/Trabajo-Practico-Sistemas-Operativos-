# Variables
CC = gcc
CFLAGS = -g -Wall -pthread
LIBS = -lcommons -lpthread
INCLUDE_DIRS = -I../utils/include -Iinclude
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
UTILS_LIB = ../utils/lib/libutils.a

# Archivos fuente y objetos
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
TARGET = $(BIN_DIR)/cpu.out

# Reglas
all: $(TARGET)

$(TARGET): $(OBJS) $(UTILS_LIB) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) -lm

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

$(UTILS_LIB):
	$(MAKE) -C ../utils

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	$(MAKE) -C ../utils clean

.PHONY: all clean