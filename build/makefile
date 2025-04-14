MAKE_PATH=$(realpath $(dir $(lastword $(MAKEFILE_LIST))))
BUILD_DIR=$(MAKE_PATH)/
SRC_DIR=$(MAKE_PATH)/../src/
INCLUDE_DIR=$(MAKE_PATH)/../include/
UTIL_DIR=$(MAKE_PATH)/../util/

OUT_EXE=$(BUILD_DIR)output/
OUT_OBJ=$(BUILD_DIR)output/obj/

BUILD := release

CC=gcc
CXX=g++

# C-specific flags
CFLAGS=-std=c23 -Wall -Wextra -Wno-unused-parameter -Wuninitialized -Werror -pedantic $(shell sdl2-config --cflags)
CFLAGS_OPT.debug=-O0 -g3 -ggdb -fno-omit-frame-pointer
CFLAGS_OPT.dev=-O0 -g3 -ggdb -fno-omit-frame-pointer
CFLAGS_OPT.release=-O2 -fno-omit-frame-pointer -march=x86-64-v3
CFLAGS+=$(CFLAGS_OPT.$(BUILD))

# C++-specific flags
CXXFLAGS=-std=c++20 -Wall -Wextra -g3
CXXFLAGS+=$(CFLAGS_OPT.$(BUILD))

# Linker flags
LDFLAGS=-lGL -lSDL2 -lSDL2main
HEADERS=$(INCLUDE_DIR)cpu.h $(INCLUDE_DIR)mmu.h $(INCLUDE_DIR)ppu.h $(INCLUDE_DIR)timer.h $(INCLUDE_DIR)interrupt.h $(INCLUDE_DIR)emulate.h $(INCLUDE_DIR)debugger.h $(INCLUDE_DIR)display.h $(INCLUDE_DIR)joypad.h $(UTIL_DIR)json_test.h $(UTIL_DIR)nuklear.h $(UTIL_DIR)nuklear_sdl_gl2.h

OBJS=$(OUT_OBJ)cpu.o $(OUT_OBJ)joypad.o $(OUT_OBJ)mmu.o $(OUT_OBJ)ppu.o $(OUT_OBJ)timer.o $(OUT_OBJ)interrupt.o $(OUT_OBJ)emulate.o $(OUT_OBJ)json_test.o $(OUT_OBJ)debugger.o $(OUT_OBJ)display.o $(OUT_OBJ)main.o

all: $(OUT_EXE)gb

$(OUT_OBJ)cpu.o: $(SRC_DIR)cpu.c $(HEADERS)
	@mkdir -p $(OUT_OBJ)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OUT_OBJ)mmu.o: $(SRC_DIR)mmu.c $(HEADERS)
	@mkdir -p $(OUT_OBJ)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OUT_OBJ)ppu.o: $(SRC_DIR)ppu.c $(HEADERS)
	@mkdir -p $(OUT_OBJ)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OUT_OBJ)timer.o: $(SRC_DIR)timer.c $(HEADERS)
	@mkdir -p $(OUT_OBJ)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OUT_OBJ)interrupt.o: $(SRC_DIR)interrupt.c $(HEADERS)
	@mkdir -p $(OUT_OBJ)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OUT_OBJ)joypad.o: $(SRC_DIR)joypad.c $(HEADERS)
	@mkdir -p $(OUT_OBJ)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OUT_OBJ)emulate.o: $(SRC_DIR)emulate.c $(HEADERS)
	@mkdir -p $(OUT_OBJ)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OUT_OBJ)display.o: $(SRC_DIR)display.c $(HEADERS)
	@mkdir -p $(OUT_OBJ)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OUT_OBJ)debugger.o: $(SRC_DIR)debugger.c $(HEADERS)
	@mkdir -p $(OUT_OBJ)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OUT_OBJ)json_test.o: $(UTIL_DIR)json_test.cpp $(HEADERS)
	@mkdir -p $(OUT_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(OUT_OBJ)main.o: $(SRC_DIR)main.cpp $(HEADERS)
	@mkdir -p $(OUT_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(OUT_EXE)gb: $(OBJS)
	@mkdir -p $(OUT_EXE)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -rf $(OUT_EXE)* $(OUT_OBJ)*.o