# Makefile for GBACee - Game Boy Emulator
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iincludes -Iincludes/core -Iincludes/hardware -Iincludes/platform -Iincludes/debug -g
LDFLAGS = -IC:/msys64/mingw64/include/SDL2 -LC:/msys64/mingw64/lib -lSDL2

# Directories
SRC_DIR = src
INC_DIR = includes
BUILD_DIR = build
BIN = gbcee

# Source and object files
SRCS := $(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/core/*.c $(SRC_DIR)/hardware/*.c $(SRC_DIR)/platform/*.c $(SRC_DIR)/debug/*.c $(SRC_DIR)/display/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Create build/ if it doesn't exist
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Default target
all: $(BUILD_DIR) $(BIN)

# Link the final executable
$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# Compile .c to .o files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Run emulator
run: all
	./$(BIN)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN)

# SDL test build (optional target)
# test-sdl: testing/SDL2_test.c
# 	$(CC) $(CFLAGS) testing/SDL2_test.c -o testing/testWindow.exe $(LDFLAGS)

.PHONY: all clean run test-sdl tests test-roms

# --- Testing ---
# Build and run all test binaries
TEST_BINS = tests/unit/cpu_tester.exe tests/unit/mmu_tester.exe tests/unit/mbc_tester.exe tests/unit/cpu_opcode_tester.exe tests/unit/ppu_tester.exe

tests: $(TEST_BINS)
	./tests/unit/cpu_tester.exe
	./tests/unit/mmu_tester.exe
	./tests/unit/mbc_tester.exe
	./tests/unit/cpu_opcode_tester.exe
	./tests/unit/ppu_tester.exe

tests/unit/cpu_tester.exe: tests/unit/cpu_test.c
	$(CC) $(CFLAGS) $< src/core/*.c src/hardware/*.c src/platform/*.c src/debug/*.c -o $@ $(LDFLAGS)

tests/unit/mmu_tester.exe: tests/unit/mmu_test.c
	$(CC) $(CFLAGS) $< src/core/*.c src/hardware/*.c src/platform/*.c src/debug/*.c -o $@ $(LDFLAGS)

tests/unit/mbc_tester.exe: tests/unit/mbc_test.c
	$(CC) $(CFLAGS) $< src/core/*.c src/hardware/*.c src/platform/*.c src/debug/*.c -o $@ $(LDFLAGS)

tests/unit/cpu_opcode_tester.exe: tests/unit/cpu_opcode_test.c
	$(CC) $(CFLAGS) $< src/core/*.c src/hardware/*.c src/platform/*.c src/debug/*.c -o $@ $(LDFLAGS)

tests/unit/ppu_tester.exe: tests/unit/ppu_test.c
	$(CC) $(CFLAGS) $< src/core/*.c src/hardware/*.c src/platform/*.c src/debug/*.c -o $@ $(LDFLAGS)

# --- Test ROMs ---
# Compile .asm to .gb using rgbasm and rgblink
ASM_SRCS := $(wildcard tests/roms/test_roms/*.asm tests/roms/test_roms/assembly/*.asm)
TEST_ROMS := $(patsubst %.asm,%.gb,$(ASM_SRCS))

test-roms: $(TEST_ROMS)

%.gb: %.asm
	rgbasm -o $*.o $<
	rgblink -o $@ $*.o
