#include "../include/ppu.h"
#include "../include/mmu.h"
#include "../include/cpu.h"
#include "../include/interrupt.h"

PPU ppu;
Fetcher fetcher;

void update_fifo();
uint32_t get_color(byte tile_high, byte tile_low, int bit_position);

#define HBLANK_FLAG 0xFC
#define VBLANK_FLAG 0xFD
#define OAM_FLAG 0xFE
#define PIXEL_TRANSFER_FLAG 0xFF

#define SET_STAT_STATE(F)              \
do {                                   \
    byte stat_value = mem_read(STAT);  \
    stat_value &= ~0x03;               \
    stat_value |= F;                   \
    mem_write(STAT, stat_value);       \
} while(0)

void ppu_init() 
{
    ppu.state = OAM_Search;
    fetcher.state = Fetch_Pixel_Num;
    ppu.can_render = 0;
    ppu.is_window = 0;
    
    SET_STAT_STATE(HBLANK_FLAG);
}

//
//NOTE: These cycle counts are not correct.
//Especially for pixel transfer.
//
void ppu_cycle() 
{
    //is LCD enabled?
    if(!(mem_read(LCDC) & (1<<7))) {
        mem_write(LY, 0);
        return ;
    }

    ppu.cycles += 4;

    switch(ppu.state) {
        case OAM_Search : {
            if(ppu.cycles >= 80) {
                ppu.cycles -= 80;
                SET_STAT_STATE(PIXEL_TRANSFER_FLAG);
                ppu.state = Pixel_Transfer;
            }
        } break;

        case Pixel_Transfer : {
            //Each stage of pixel transfer takes 2 Tcycles so we can just run
            //update_fifo twice (for now) to match the 4 Tcycles each ppu cycle 
            //consumes
            static int elapsed_cycles = 0;
            elapsed_cycles += 4;
            update_fifo();
            update_fifo();
            if(fetcher.pixel >= 160) {
                fetcher.pixel = 0;
                ppu.cycles -= elapsed_cycles;
                elapsed_cycles = 0;
                SET_STAT_STATE(HBLANK_FLAG);
                ppu.state = HBlank;
            }
        } break;

        case HBlank : {
            if(ppu.cycles >= 204) {
                ppu.cycles -= 204;
                mmu.memory[LY]++;

                if(mem_read(LY) == 144) {
                    SET_STAT_STATE(VBLANK_FLAG);
                    ppu.can_render = 1;
                    request_interrupt(0); //vblank interrupt
                    ppu.state = VBlank;
                }
                else {
                    SET_STAT_STATE(OAM_FLAG);
                    ppu.state = OAM_Search;
                }
            }
        } break;

        case VBlank : {
            if(ppu.cycles >= 4560) {
                ppu.cycles -= 4560;
                mem_write(LY, 0);
                SET_STAT_STATE(OAM_FLAG);
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
            for(int x = 0; x < 8; x++) {
                if(fetcher.pixel + x > 160) {
                    return ;
                }
                ppu.pixel_buffer[fetcher.pixel + x][mem_read(LY)] = 0xFFFFFFFF; //sets whole buffer to white
            }
            fetcher.pixel += 8;

            fetcher.state = Fetch_Pixel_Num;
        } break;

        default : break;
    }
}

uint32_t get_color(byte tile_high, byte tile_low, int bit_position) 
{ 
    int color_id = ((tile_high >> bit_position) & 0x01) |
        ((tile_low >> bit_position) & 0x01) << 1;

    //Uses manual pallet, not whatever is in BGP
    switch (color_id) {
        case 0: return 0xFFFFFFFF; // White
        case 1: return 0xAAAAAAFF; // Light gray
        case 2: return 0x555555FF; // Dark gray
        case 3: return 0x000000FF; // Black
        default: return 0xFFFFFFFF;
    }
}
