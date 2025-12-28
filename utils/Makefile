# Variables
CC = gcc
AR = ar
ARFLAGS = rcs
CFLAGS = -Wall -g -Iinclude
SRC_DIR = src
OBJ_DIR = obj
LIB_DIR = lib
TARGET = $(LIB_DIR)/libutils.a

# Archivos fuente y objetos
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Reglas
.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR)