#include "../include/ppu.h"

PPU ppu;

void populate_pixel_buffer();

void ppu_cycle() 
{
    //stuff!
    populate_pixel_buffer();
}

void populate_pixel_buffer() 
{
    for(int x = 0; x < GB_DISPLAY_WIDTH; x++) {
        for(int y = 0; y < GB_DISPLAY_HEIGHT; y++) {
            ppu.pixel_buffer[x][y] = 0xFFFFFFFF; //sets whole buffer to white
        }
    }
}