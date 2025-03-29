#include "../include/ppu.h"
#include "../include/mmu.h"
#include "../include/cpu.h"
#include "../include/interrupt.h"

PPU ppu;
Fetcher fetcher;

void update_fifo();
void fetcher_reset();
void get_tilemap_tiledata_baseptrs();
uint32_t get_color(byte tile_high, byte tile_low, int bit_position);

#define HBLANK_FLAG 0x00
#define VBLANK_FLAG 0x01
#define OAM_FLAG 0x02
#define PIXEL_TRANSFER_FLAG 0x03

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
    ppu.can_render = false;
    ppu.is_window = false;
    ppu.cycles = 0;
    
    SET_STAT_STATE(OAM_FLAG);
}

void ppu_cycle() 
{
    //is LCD enabled?
    if (!(mem_read(LCDC) & 0x80)) {
        if (ppu.state != VBlank) {
            mem_write(LY, 0);
            SET_STAT_STATE(VBlank);
        }
        ppu.cycles += 4;
        return;
    }

    int should_interrupt = false;
    switch(ppu.state) {
        case OAM_Search : {
            if(ppu.cycles >= 80) {
                SET_STAT_STATE(PIXEL_TRANSFER_FLAG);

                //no stat interrupt for transitioning to pixel transfer

                fetcher_reset();
                ppu.state = Pixel_Transfer;
            }
        } break;

        case Pixel_Transfer : {
            //Each stage of pixel transfer takes 2 Tcycles so we can just run
            //update_fifo twice (for now) to match the 4 Tcycles each ppu cycle 
            //consumes
            update_fifo();
            update_fifo();
            
            if(fetcher.pixel == 160) {
                SET_STAT_STATE(HBLANK_FLAG);
                should_interrupt = mem_read(STAT) & (1 << 3); //test for hblank interrupt
                mmu.memory[LY]++;
                ppu.state = HBlank;
            }
        } break;

        case HBlank : {
            if(ppu.cycles >= 456) {
                ppu.cycles -= 456;

                if(mem_read(LY) == 144) {
                    SET_STAT_STATE(VBLANK_FLAG);
                    should_interrupt = mem_read(STAT) & (1 << 4); //test for vblank interrupt
                    request_interrupt(0); //do vblank int
                    ppu.state = VBlank;
                }
                else {
                    SET_STAT_STATE(OAM_FLAG);
                    should_interrupt = mem_read(STAT) & (1 << 5); //test for oam search interrupt
                    ppu.state = OAM_Search;
                }
            }
        } break;

        case VBlank : {
            if(ppu.cycles >= 456) {
                ppu.cycles -= 456;
                mmu.memory[LY]++;

                if(mem_read(LY) == 154) {
                    mem_write(LY, 0);
                    SET_STAT_STATE(OAM_FLAG);
                    should_interrupt = mem_read(STAT) & (1 << 5); //test for oam search interrupt
                    ppu.can_render = 1;
                    ppu.state = OAM_Search;
                }
            }
        } break;

        default : break;
    }

    //
    //For this to be triggered (a STAT interrupt) we need to test
    //whether the corresponding bit for modes 0-2 in STAT
    //(bits 3, 4, 5) are set. if this is set then we can request
    //a stat interrupt. this will only be set on transition into
    //other states (not including pixel transfer)
    //
    if(should_interrupt) {
        //request stat interrupt
        request_interrupt(1);
    }

    byte stat = mem_read(STAT);
    if(mem_read(LY) == mem_read(LYC)) {
        stat |= (1<<2); //ly==lyc flag
        mem_write(STAT, stat);
        if(mem_read(STAT) & (1<<6)) {
            request_interrupt(1);
        }
    }
    else {
        //reset flag
        stat &= ~(1<<2); //ly==lyc flag
        mem_write(STAT, stat);
    }

    ppu.cycles += 4;
}

inline void fetcher_reset() 
{
    fetcher.state = Fetch_Tile_Num;
    fetcher.pixel = 0;
    fetcher.tile_x = 0;
    fetcher.window_line_counter = 0;
    ppu.is_window = 0;
}

//
//Currently just handles bg and window, no sprites
//Once sprites are integrated this method will likely
//need to be better named
//
void update_fifo() 
{
    byte ly = mem_read(LY);
    byte scx = mem_read(SCX);
    byte scy = mem_read(SCY);
    byte wx = mem_read(WX) - 7;

    //TODO: figure out how to implement window and undetstand better how it
    //actually triggers.
    switch(fetcher.state) {
        case Fetch_Tile_Num : {
            get_tilemap_tiledata_baseptrs(); //we need to update every tile

            word xoffset = (fetcher.tile_x + (scx / 8)) & 0x1F;
            word yoffset = 32 * (((ly + scy) & 0xFF) / 8);

            if(ppu.is_window) {
                xoffset = fetcher.tile_x;
                
                //if we hit wx in our scanline
                //wx is in pixels, so we can compare our current pixel
                if(fetcher.pixel >= wx) {
                    xoffset = (fetcher.tile_x - (wx / 8));
                }
                yoffset = 32 * (fetcher.window_line_counter / 8);
                fetcher.window_line_counter++;
            }
            fetcher.tile_x++;

            word base = (xoffset + yoffset) & 0x3FF;

            if(fetcher.is_unsigned) {
                fetcher.tilenumber = (byte)mem_read(fetcher.tilemap + base);
            }
            else {
                fetcher.tilenumber = (int8_t)mem_read(fetcher.tilemap + base);
            }
            fetcher.state = Fetch_Tile_Data_Low;
        } break;

        case Fetch_Tile_Data_Low : {
            word base = (2 * ((ly + scy) % 8));
            if(ppu.is_window) {
                base = (2 * (fetcher.window_line_counter % 8));
            }

            fetcher.tiledata_low = mem_read(base + fetcher.tiledata + (fetcher.tilenumber * 16));
            fetcher.state = Fetch_Tile_Data_High;
        } break;
        
        case Fetch_Tile_Data_High : {
            word base = (2 * ((ly + scy) % 8));
            if(ppu.is_window) {
                base = (2 * (fetcher.window_line_counter % 8));
            }

            fetcher.tiledata_high = mem_read(base + fetcher.tiledata + (fetcher.tilenumber * 16) + 1);
            fetcher.state = Push_To_FIFO;
        } break;

        case Push_To_FIFO : {
            for(int x = 7; x >= 0 ; x--) {
                if(fetcher.pixel == 160) {
                    break;
                }
                ppu.pixel_buffer[ly][fetcher.pixel++] = get_color(fetcher.tiledata_high, fetcher.tiledata_low, x);
            }

            fetcher.state = Fetch_Tile_Num;
        } break;

        default : break;
    }
}

inline void get_tilemap_tiledata_baseptrs()
{
    fetcher.tilemap = 0x9800;
    //default to 8800 method 
    fetcher.tiledata = 0x9000;
    fetcher.is_unsigned = false;
    ppu.is_window = false;

    //are we rendering the window?
    if(mem_read(LCDC) & (1 << 5)) {
        //only set if we are rendering window pixels
        if(mem_read(LY) >= mem_read(WY)) {
            ppu.is_window = true;
        }
    }

    if(mem_read(LCDC) & (1 << 4)) {
        //8000 method
        fetcher.is_unsigned = true;
        fetcher.tiledata = 0x8000;
    }

    if(!ppu.is_window) {
        //are we rendering the background?
        if(mem_read(LCDC) & (1 << 3)) {
            fetcher.tilemap = 0x9C00;
        }
    }   
    else {
        if(mem_read(LCDC) & (1 << 6)) {
            fetcher.tilemap = 0x9C00;
        }
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
