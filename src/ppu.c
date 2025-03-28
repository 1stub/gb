#include "../include/ppu.h"
#include "../include/mmu.h"
#include "../include/cpu.h"
#include "../include/interrupt.h"

PPU ppu;
Fetcher fetcher;

void update_fifo();
void get_tilemap_tiledata_baseptrs();
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
    if (!(mem_read(LCDC) & (1 << 7))) {
        if(ppu.cycles >= 70224){
            ppu.state = OAM_Search;
            ppu.cycles -= 70224;
        }
        return ;
    }

    ppu.cycles += 4;

    switch(ppu.state) {
        case OAM_Search : {
            if(ppu.cycles >= 80) {
                ppu.cycles -= 80;
                SET_STAT_STATE(PIXEL_TRANSFER_FLAG);
                ppu.state = Pixel_Transfer;

                //get ready for pixel transfer
                get_tilemap_tiledata_baseptrs();
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

//
//Currently just handles bg and window, no sprites
//Once sprites are integrated this method will likely
//need to be better named
//
void update_fifo() 
{
    byte scx = mem_read(SCX);
    byte scy = mem_read(SCY);
    byte wy = mem_read(WY);
    //byte wx = mem_read(WX);    I think we need this in actual pixel rendering
    byte ly = mem_read(LY);

    switch(fetcher.state) {
        case Fetch_Pixel_Num : {
            word base = + fetcher.tilemap;

            if(ppu.is_window) {
                base += 32 * (((wy) & 0x3FF) / 8);
            }
            else {
                base += 32 * ((((ly + scy)) & 0xFF) / 8);
                base += ((scx/8) & 0x1f);
            }

            if(fetcher.is_unsigned) {
                fetcher.tilenumber = (byte)mem_read(base);
            }
            else {
                fetcher.tilenumber = (int8_t)mem_read(base);
            }

            fetcher.state = Fetch_Tile_Data_Low;
        } break;

        //will need to handle signed case
        case Fetch_Tile_Data_Low : {
            word base = (2 * ((ly + scy) % 8)) + fetcher.tiledata;
            if(ppu.is_window) {
                base = (2 * ((wy) % 8)) + fetcher.tiledata;
            }

            fetcher.tiledata_low = mem_read(base);
            fetcher.state = Fetch_Tile_Data_High;
        } break;
        
        case Fetch_Tile_Data_High : {
            word base = (2 * ((ly + scy) % 8)) + fetcher.tiledata;
            if(ppu.is_window) {
                base = (2 * ((wy) % 8)) + fetcher.tiledata;
            }

            fetcher.tiledata_high = mem_read(base + 1);
            fetcher.state = Push_To_FIFO;
        } break;

        case Push_To_FIFO : {
            for(int x = 0; x < 8; x++) {
                if(fetcher.pixel + x > 160) {
                    return ;
                }
                ppu.pixel_buffer[fetcher.pixel + x][mem_read(LY)] = get_color(fetcher.tiledata_high, fetcher.tiledata_low, fetcher.pixel + x); 
            }
            fetcher.pixel += 8;

            fetcher.state = Fetch_Pixel_Num;
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
    ppu.is_window = false;

    //are we rendering the window?
    if(mem_read(LCDC) & (1 << 5)) {
        if(mem_read(WY) <= mem_read(LY)) {
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
