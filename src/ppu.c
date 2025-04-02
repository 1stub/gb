#include "../include/ppu.h"
#include "../include/mmu.h"
#include "../include/cpu.h"
#include "../include/interrupt.h"

#include <assert.h>

PPU ppu;
Fetcher bg_win_fetcher;
Fetcher sprites_fetcher;

int sprite_buffer_index = 0;
uint32_t sprite_buffer[10];

void update_bg_win();
void update_sprites();
void populate_sprite_buffer();
void bg_win_fetcher_reset();
void get_bg_win_tilemap_tiledata();
uint32_t get_color(byte tile_high, byte tile_low, int bit_position);

#define HBLANK_FLAG 0x00
#define VBLANK_FLAG 0x01
#define OAM_FLAG 0x02
#define PIXEL_TRANSFER_FLAG 0x03

//Useful for sprite priority
#define COLOR_0 0xE0F8D0

#define SET_STAT_STATE(F)              \
do {                                   \
    byte stat_value = mem_read(STAT);  \
    stat_value &= ~0x03;               \
    stat_value |= F;                   \
    mem_write(STAT, stat_value);       \
} while(0)


#define LY_LYC_COMPARE()                                 \
do {                                                     \
    byte lyc = mem_read(LYC);                            \
    byte ly = mem_read(LY);                              \
    mem_write(STAT,                                      \
        (mem_read(STAT) & ~(1<<2))| ((ly == lyc) << 2)); \
    if ((lyc == ly) && (mem_read(STAT) & (1<<6))) {      \
        request_interrupt(1);                            \
    }                                                    \
} while(0);

void ppu_init() 
{
    ppu.state = OAM_Search;
    ppu.can_render = false;
    ppu.is_window = false;
    ppu.cycles = 0;

    bg_win_fetcher.window_line_counter = 0;
    
    SET_STAT_STATE(OAM_FLAG);
}

void ppu_cycle() 
{
    //is LCD enabled?
    if (!(mem_read(LCDC) & (1<<7))) {
        bg_win_fetcher_reset();
        ppu.can_render = false;
        ppu.cycles = 0;
        mem_write(LY, 0);
        SET_STAT_STATE(VBlank);
        return ;
    }

    int can_interrupt = false;
    switch(ppu.state) {
        case OAM_Search : { //mode 2
            if(ppu.cycles >= 80) { 
                //no stat interrupt for transitioning to pixel transfer
                populate_sprite_buffer();

                bg_win_fetcher_reset();

                SET_STAT_STATE(PIXEL_TRANSFER_FLAG);
                ppu.state = Pixel_Transfer;
            }
        } break;

        case Pixel_Transfer : { //mode 3
            //Each stage of pixel transfer takes 2 Tcycles so we can just run
            //update_fifo twice (for now) to match the 4 Tcycles each ppu cycle 
            //consumes
            update_bg_win();
            update_bg_win();

            if(bg_win_fetcher.pixel == 160) {
                can_interrupt = (mem_read(STAT) & (1 << 3));

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
                    can_interrupt = (mem_read(STAT) & (1 << 4)); //vblank interrupt in stat

                    request_interrupt(0);
                    SET_STAT_STATE(VBLANK_FLAG);
                    ppu.state = VBlank;
                }
                else {
                    can_interrupt = (mem_read(STAT) & (1 << 5)); //oam search interrupt

                    if(ppu.is_window) {
                        bg_win_fetcher.window_line_counter++;
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
                    bg_win_fetcher.window_line_counter = 0;

                    can_interrupt = (mem_read(STAT) & (1 << 5)); //test for oam search interrupt

                    ppu.can_render = true;
                    SET_STAT_STATE(OAM_FLAG);
                    ppu.state = OAM_Search;
                }
                LY_LYC_COMPARE();
            }
        } break;

        default : break;
    }

    if(can_interrupt) {
        request_interrupt(1);
        printf("stat int from ppu\n");
    }

    ppu.cycles += 4;
}

void populate_sprite_buffer()
{
    word base = 0xFE00;
    byte height = mem_read(LCDC) & (1<<1) ? 16 : 8;
    for(int i = 0; i < 80; i+=4) {
        byte y = mem_read(base + i);
        byte x = mem_read(base + i + 1);
        byte tile_no = mem_read(base + i + 2);
        byte flags = mem_read(base + i + 3);

        byte bound = mem_read(LY) + 16;
        if(x > 0 && bound >= y && bound < (height + y) && sprite_buffer_index < 10) {
            uint32_t entry = y | (x >> 8) | (tile_no >> 16) | (flags >> 24);
            sprite_buffer[sprite_buffer_index++] = entry;
        }
    }
}

void bg_win_fetcher_reset() 
{
    bg_win_fetcher.state = Fetch_Tile_Num;
    bg_win_fetcher.pixel = 0;
    ppu.is_window = false;
}

void update_bg_win() 
{
    byte ly = mem_read(LY);
    byte wx = mem_read(WX) - 7;
    //byte wy = mem_read(WY);
    byte scx = mem_read(SCX);
    byte scy = mem_read(SCY);

    switch(bg_win_fetcher.state) {
        case Fetch_Tile_Num : {
            get_bg_win_tilemap_tiledata(); //we need to update every tile

            word xoffset = ((bg_win_fetcher.pixel + scx) / 8) & 0x1F;
            word yoffset = 32 * (((ly + scy) & 0xFF) / 8);

            if(ppu.is_window) {
                xoffset = ((bg_win_fetcher.pixel - wx) / 8) & 0x1F;
                yoffset = 32 * (((bg_win_fetcher.window_line_counter))/ 8);
            }

            word base = (xoffset + yoffset) & 0x3FF;

            if(bg_win_fetcher.is_unsigned) {
                bg_win_fetcher.tilenumber = (byte)mem_read(bg_win_fetcher.tilemap + base);
            }
            else {
                bg_win_fetcher.tilenumber = (int8_t)mem_read(bg_win_fetcher.tilemap + base);
            }
            bg_win_fetcher.state = Fetch_Tile_Data_Low;
        } break;

        case Fetch_Tile_Data_Low : {
            word base = (2 * ((ly + scy) % 8));
            if(ppu.is_window) {
                base = (2 * (((bg_win_fetcher.window_line_counter)) % 8));
            }

            if(bg_win_fetcher.is_unsigned) {
                bg_win_fetcher.tiledata_low = mem_read(base + bg_win_fetcher.tiledata + (bg_win_fetcher.tilenumber * 16));
            }
            else{
                bg_win_fetcher.tiledata_low = mem_read(base + bg_win_fetcher.tiledata + ((bg_win_fetcher.tilenumber+128) * 16));
            }
            bg_win_fetcher.state = Fetch_Tile_Data_High;
        } break;
        
        case Fetch_Tile_Data_High : {
            word base = (2 * ((ly + scy) % 8));
            if(ppu.is_window) {
                base = (2 * (((bg_win_fetcher.window_line_counter)) % 8));
            }

            if(bg_win_fetcher.is_unsigned) {
                bg_win_fetcher.tiledata_high = mem_read(base + bg_win_fetcher.tiledata + (bg_win_fetcher.tilenumber * 16) + 1);
            }
            else{
                bg_win_fetcher.tiledata_high = mem_read(base + bg_win_fetcher.tiledata + ((bg_win_fetcher.tilenumber+128) * 16) + 1);
            }            
            bg_win_fetcher.state = Push_To_FIFO;
        } break;

        case Push_To_FIFO : {
            for(int x = 7; x >= 0 ; x--) {
                if(bg_win_fetcher.pixel == 160) {
                    break;
                }

                uint32_t color = get_color(bg_win_fetcher.tiledata_high, bg_win_fetcher.tiledata_low, x);
                if(!(mem_read(LCDC) & 0x01)) { //bg/win enable bit
                    color = 0xE0F8D0; //white
                }

                ppu.pixel_buffer[ly][bg_win_fetcher.pixel++] = color;
            }

            bg_win_fetcher.state = Fetch_Tile_Num;
        } break;

        default : break;
    }
}

//
//Sprites use a separate fifo
//NOTE: We may need to make the pixel buffer a global storage unit,
//having to access this buffer from the bg fetcher in sprites
//is just weird
//Also will need to figure out how to pixel mix
//
void update_sprites() 
{
    switch(sprites_fetcher.state) {
        case Fetch_Tile_Num : {

            sprites_fetcher.state = Fetch_Tile_Data_Low;
        } break;

        case Fetch_Tile_Data_Low : {

            
            sprites_fetcher.state = Fetch_Tile_Data_High;
        } break;
        
        case Fetch_Tile_Data_High : {


            sprites_fetcher.state = Push_To_FIFO;
        } break;

        case Push_To_FIFO : {
            for(int x = 7; x >= 0 ; x--) {
                
            }
        } break;

        default : break;
    }
}

void get_bg_win_tilemap_tiledata()
{
    bg_win_fetcher.tilemap = 0x9800;
    //default to 8800 method 
    bg_win_fetcher.tiledata = 0x8800;
    bg_win_fetcher.is_unsigned = false;
    ppu.is_window = false;

    byte lcdc = mem_read(LCDC);
    byte ly = mem_read(LY);

    //is the widnow enabled?
    if(lcdc & (1 << 5)) {
        byte wy = mem_read(WY);
        byte wx = mem_read(WX) - 7;
    
        if(ly >= wy && bg_win_fetcher.pixel >= wx) {
            ppu.is_window = true;
        }
    }

    if(lcdc & (1 << 4)) {
        //8000 method
        bg_win_fetcher.is_unsigned = true;
        bg_win_fetcher.tiledata = 0x8000;
    }

    if(!ppu.is_window) {
        //are we rendering the background?
        if(lcdc & (1 << 3)) {
            bg_win_fetcher.tilemap = 0x9C00;
        }
    }   
    else {
        if(lcdc & (1 << 6)) {
            bg_win_fetcher.tilemap = 0x9C00;
        }
    }
}

uint32_t get_color(byte tile_high, byte tile_low, int bit_position) 
{ 
    int color_id = ((tile_high >> bit_position) & 0x01) |
        ((tile_low >> bit_position) & 0x01) << 1;

    //https://lospec.com/palette-list/nintendo-gameboy-bgb
    switch (color_id) {
        case 0: return COLOR_0; // white
        case 1: return 0x88C070; // Light gray
        case 2: return 0x346856; // Dark gray
        case 3: return 0x81820; // Black
        default: return COLOR_0; // Fallback (white)
    }
}
