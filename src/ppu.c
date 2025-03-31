#include "../include/ppu.h"
#include "../include/mmu.h"
#include "../include/cpu.h"
#include "../include/interrupt.h"

#include <assert.h>

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


void LY_LYC_COMPARE()    {                               \
do {                                                     \
    byte lyc = mem_read(LYC);                            \
    byte ly = mem_read(LY);                              \
    mem_write(STAT,                                      \
        (mem_read(STAT) & ~(1<<2))| ((ly == lyc) << 2)); \
    if ((lyc == ly) && (mem_read(STAT) & (1<<6))) {      \
        request_interrupt(1);                            \
        printf("LY==LYC CONDIDTION MET!\n");
    }                                                    \
} while(0);
}

void ppu_init() 
{
    ppu.state = OAM_Search;
    ppu.can_render = true;
    ppu.is_window = false;
    ppu.should_irq_block = false;
    ppu.cycles = 0;
    
    SET_STAT_STATE(OAM_FLAG);
}

//TODO: add support for irq blocking and investigate vblank timing
//in regargs to triggering vblank interupts correctly. I feel they
//may need to trigger when we finish having ppu disabled.

//current dmg acid bug is most definetly intyerupt related

//Looks like it is most closely related to the ly==lyc interrupt
void ppu_cycle() 
{
    //is LCD enabled?
    if (!(mem_read(LCDC) & (1<<7))) {
        fetcher_reset();
        mem_write(LY, 0);
        SET_STAT_STATE(VBlank);

        return ;
    }

    ppu.cycles += 4;

    switch(ppu.state) {
        case OAM_Search : { //mode 2
            if(ppu.cycles >= 80) { 
                //no stat interrupt for transitioning to pixel transfer

                fetcher_reset();

                SET_STAT_STATE(PIXEL_TRANSFER_FLAG);
                ppu.state = Pixel_Transfer;
            }
        } break;

        case Pixel_Transfer : { //mode 3
            //Each stage of pixel transfer takes 2 Tcycles so we can just run
            //update_fifo twice (for now) to match the 4 Tcycles each ppu cycle 
            //consumes
            update_fifo();
            update_fifo();
            
            if(fetcher.pixel == 160) {
                if(mem_read(STAT) & (1 << 3)) { //hblank flag
                    request_interrupt(1);
                }

                SET_STAT_STATE(HBLANK_FLAG);
                ppu.state = HBlank;
            }
        } break;

        case HBlank : { //mode 0
            if(ppu.cycles >= 456) {
                ppu.cycles -= 456;
                mmu.memory[LY]++;
                LY_LYC_COMPARE();

                if(mem_read(LY) == 144) {
                    request_interrupt(0); //set vblank int flag
                    if(mem_read(STAT) & (1 << 4)) { //vblank interrupt in stat
                        request_interrupt(1);
                    }    

                    SET_STAT_STATE(VBLANK_FLAG);
                    ppu.state = VBlank;
                }
                else {
                    if(mem_read(STAT) & (1 << 5)) { //oam search interrupt
                        request_interrupt(1);
                    } 

                    SET_STAT_STATE(OAM_FLAG);
                    ppu.state = OAM_Search;
                }
            }
        } break;

        case VBlank : { //mode 1
            if(ppu.cycles >= 456) {
                ppu.cycles -= 456;
                mmu.memory[LY]++;

                if(mem_read(LY) == 153) {
                    mem_write(LY, 0);

                    if(mem_read(STAT) & (1 << 5)) { //test for oam search interrupt
                        request_interrupt(1);
                    } 

                    ppu.can_render = true;
                    SET_STAT_STATE(OAM_FLAG);
                    ppu.state = OAM_Search;
                }
                LY_LYC_COMPARE();
            }
        } break;

        default : break;
    }
}

void fetcher_reset() 
{
    fetcher.state = Fetch_Tile_Num;
    fetcher.pixel = 0;
    fetcher.tile_x = 0;
    fetcher.window_line_counter = 0;
    ppu.is_window = false;
    ppu.has_window_triggered = false;  // Reset window trigger each line
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

    //TODO: figure out how to implement window and undetstand better how it
    //actually triggers.
    switch(fetcher.state) {
        case Fetch_Tile_Num : {
            get_tilemap_tiledata_baseptrs(); //we need to update every tile

            word xoffset = (fetcher.tile_x + (scx / 8)) & 0x1F;
            word yoffset = 32 * (((ly + scy) & 0xFF) / 8);

            if(ppu.is_window) {
                xoffset = fetcher.tile_x & 0x1F;
                yoffset = 32 * ((fetcher.window_line_counter & 0xFF)/ 8);
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

void get_tilemap_tiledata_baseptrs()
{
    fetcher.tilemap = 0x9800;
    //default to 8800 method 
    fetcher.tiledata = 0x9000;
    fetcher.is_unsigned = false;

    byte lcdc = mem_read(LCDC);

    //is the widnow enabled?
    if(lcdc & (1 << 5)) {
        byte wx = mem_read(WX) - 7;
        byte wy = mem_read(WY);
    
        if(!ppu.has_window_triggered && (mem_read(LY) >= wy)) {
            if (fetcher.pixel >= wx) {
                ppu.is_window = true;
                ppu.has_window_triggered = true;
                fetcher.tile_x = 0;  // Reset tile X for window
            }
        }
    }

    if(lcdc & (1 << 4)) {
        //8000 method
        fetcher.is_unsigned = true;
        fetcher.tiledata = 0x8000;
    }

    if(!ppu.is_window) {
        //are we rendering the background?
        if(lcdc & (1 << 3)) {
            fetcher.tilemap = 0x9C00;
        }
    }   
    else {
        if(lcdc & (1 << 6)) {
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
