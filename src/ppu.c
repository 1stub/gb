#include "../include/ppu.h"
#include "../include/mmu.h"

PPU ppu;
Fetcher fetcher;

void update_fifo();

//
//NOTE: These cycle counts are not correct.
//Especially for pixel transfer.
//
void ppu_cycle() 
{
    ppu.cycles += 4;

    switch(ppu.state) {
        case OAM_Search : {
            if(ppu.cycles >= 80) {
                ppu.cycles -= 80;
                ppu.state = Pixel_Transfer;
            }
        } break;

        case Pixel_Transfer : {
            update_fifo();
            if(fetcher.pixel >= 160) {
                ppu.state = HBlank;
            }
        } break;

        case HBlank : {
            if(ppu.cycles >= 204) {
                ppu.cycles -= 204;
                mmu.memory[LY]++;

                if(mem_read(LY) == 144) {
                    ppu.state = VBlank;
                }
            }
        } break;

        case VBlank : {
            if(ppu.cycles >= 4560) {
                ppu.cycles -= 4560;
                ppu.state = OAM_Search;
            }
        } break;

        default : break;
    }
}

void update_fifo() 
{
    switch(fetcher.state) {
        case Fetch_Pixel_Num : {

            fetcher.state = Fetch_Tile_Data_Low;
        } break;

        case Fetch_Tile_Data_Low : {

            fetcher.state = Fetch_Tile_Data_High;
        } break;
        
        case Fetch_Tile_Data_High : {

            fetcher.state = Push_To_FIFO;
        } break;

        case Push_To_FIFO : {
            for(int x = 0; x < GB_DISPLAY_WIDTH; x++) {
                for(int y = 0; y < GB_DISPLAY_HEIGHT; y++) {
                    ppu.pixel_buffer[x][y] = 0xFFFFFFFF; //sets whole buffer to white
                }
            }
            fetcher.pixel += 8;

            fetcher.state = Fetch_Pixel_Num;
        } break;

        default : break;
    }
}
