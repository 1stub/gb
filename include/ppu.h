#pragma once

#include "common.h"
#include <stdint.h>

typedef enum {
    OAM_Search,
    Pixel_Transfer,
    HBlank,
    VBlank
} PPU_state;

typedef struct {
    PPU_state state;
    uint32_t cycles;
    int is_window;
    int window_line_counter;
    int can_render;
    byte pixel;
    int is_unsigned;
    uint32_t pixel_buffer[GB_DISPLAY_HEIGHT][GB_DISPLAY_WIDTH];
} PPU;

typedef struct {
    word tilemap; //base pointer
    word tiledata; //base pointer
    int16_t tilenumber; //could be signed or not
    byte tiledata_low;
    byte tiledata_high;
} Renderer;

typedef struct {
    byte y;
    byte x;
    byte tile;
    byte flags;
    byte palette;
    byte height;
} SpriteEntry;

extern PPU ppu;

//Our ppu will always just cycle 4 T-Cycles each call
extern void ppu_cycle(int cycles);
extern void ppu_init();