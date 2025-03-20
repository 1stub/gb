#pragma once

#include "common.h"
#include <stdint.h>

typedef enum{
    OAM_Search,
    Pixel_Transfer,
    HBlank,
    VBlank
} PPU_state;

typedef struct{
    PPU_state state;
    uint32_t cycles;
    int is_window;
    uint32_t pixel_buffer[GB_DISPLAY_WIDTH][GB_DISPLAY_HEIGHT];
} PPU;

extern PPU ppu;

//Our ppu will always just cycle 4 T-Cycles each call
extern void ppu_cycle();