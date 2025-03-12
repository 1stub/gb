MAKE_PATH=$(realpath $(dir $(lastword $(MAKEFILE_LIST))))
BUILD_DIR=$(MAKE_PATH)/
SRC_DIR=$(MAKE_PATH)/../src/
INCLUDE_DIR=$(MAKE_PATH)/../include/
UTIL_DIR=$(MAKE_PATH)/../util/

OUT_EXE=$(BUILD_DIR)output/
OUT_OBJ=$(BUILD_DIR)output/obj/

BUILD := dev

CC=gcc
CSTDFLAGS=$(shell sdl2-config --cflags) -Wall -Wextra -Wno-unused-parameter -Wuninitialized -Werror -std=c23 -pedantic -lSDL2 -lSDL2main

CXX=g++
CXXFLAGS=-std=c++20 -Wall -Wextra -g

CFLAGS_OPT.debug=-O0 -g -ggdb -fno-omit-frame-pointer
CFLAGS_OPT.dev=-O0 -g -ggdb -fno-omit-frame-pointer
CFLAGS_OPT.release=-O3 -fno-omit-frame-pointer -march=x86-64-v3
CFLAGS=${CFLAGS_OPT.${BUILD}} ${CSTDFLAGS}
CFLAGS_TEST=${CFLAGS_OPT.dev} ${CSTDFLAGS}

HEADERS=$(INCLUDE_DIR)cpu.h $(INCLUDE_DIR)mmu.h $(UTIL_DIR)json_test.h

OBJS=$(OUT_OBJ)cpu.o $(OUT_OBJ)mmu.o $(OUT_OBJ)json_test.o

all: $(OUT_EXE)gb

$(OUT_OBJ)cpu.o: $(HEADERS) $(SRC_DIR)cpu.c 
	@mkdir -p $(OUT_OBJ)
	$(CC) $(CFLAGS) -o $(OUT_OBJ)cpu.o -c $(SRC_DIR)cpu.c

$(OUT_OBJ)mmu.o: $(HEADERS) $(SRC_DIR)mmu.c 
	@mkdir -p $(OUT_OBJ)
	$(CC) $(CFLAGS) -o $(OUT_OBJ)mmu.o -c $(SRC_DIR)mmu.c

$(OUT_OBJ)json_test.o: $(HEADERS) $(UTIL_DIR)json_test.cpp
	@mkdir -p $(OUT_OBJ)
	$(CXX) $(CXXFLAGS) -o $(OUT_OBJ)json_test.o -c $(UTIL_DIR)json_test.cpp

$(OUT_EXE)gb: $(HEADERS) $(OBJS) $(SRC_DIR)main.c
	@mkdir -p $(OUT_EXE)
	$(CXX) -o $(OUT_EXE)gb $(OBJS) $(SRC_DIR)main.c

clean:
	rm -rf $(OUT_EXE)* $(OUT_OBJ)*.o