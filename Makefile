CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g -Iinclude
SRC     = src/main.c src/lexer.c src/parser.c src/symtable.c src/codegen.c src/error.c
OBJ     = $(addprefix build/, $(notdir $(SRC:.c=.o)))
TARGET  = c8l

# Create build directory if it doesn't exist
BUILD_DIR = build

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files from src/ to build/
build/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR) $(TARGET) *.rom

test: $(TARGET)
	./$(TARGET) examples/counter.c8l -o counter.rom
	./$(TARGET) examples/loop.c8l    -o loop.rom

.PHONY: all clean test