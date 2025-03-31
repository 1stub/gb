#pragma once

#include "common.h"
#include <stdint.h>

typedef enum {
    OAM_Search,
    Pixel_Transfer,
    HBlank,
    VBlank
} PPU_state;

typedef enum {
    Fetch_Tile_Num,
    Fetch_Tile_Data_Low,
    Fetch_Tile_Data_High,
    Push_To_FIFO
} Fetcher_state;

typedef struct {
    PPU_state state;
    uint32_t cycles;
    int is_window;
    int can_render;
    int has_window_triggered;
    int should_irq_block;
    uint32_t pixel_buffer[GB_DISPLAY_HEIGHT][GB_DISPLAY_WIDTH];
} PPU;

typedef struct {
    Fetcher_state state;
    byte pixel;
    byte tile_x;
    int is_unsigned; //8000 method if true
    byte window_line_counter;
    word tilemap; //base pointer
    word tiledata; //base pointer
    int16_t tilenumber; //could be signed or not
    byte tiledata_low;
    byte tiledata_high;
} Fetcher;

extern PPU ppu;

//Our ppu will always just cycle 4 T-Cycles each call
extern void ppu_cycle();
extern void ppu_init();