#include "../include/ppu.h"
#include "../include/mmu.h"
#include "../include/cpu.h"
#include "../include/interrupt.h"

PPU ppu;
BGWinFetcher bg_win_fetcher;
SpriteFetcher sprite_fetcher;

#define SPRITE_BUFFER_SIZE 10
int sprite_buffer_index = 0;
SpriteEntry sprite_buffer[SPRITE_BUFFER_SIZE];

void update_bg_win();
void update_sprites();
void populate_sprite_buffer();
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
    ppu.pixel = 0;

    sprite_fetcher.full_fifo = false;
    bg_win_fetcher.full_fifo = false;

    bg_win_fetcher.window_line_counter = 0;

    SET_STAT_STATE(OAM_FLAG);
}

void ppu_cycle() 
{
    //is LCD enabled?
    if (!(mem_read(LCDC) & (1<<7))) {
        ppu.can_render = false;
        ppu.cycles = 0;
        ppu.pixel = 0;
        mem_write(LY, 0);
        SET_STAT_STATE(VBlank);
        return ;
    }

    int can_interrupt = false;
    switch(ppu.state) {
        case OAM_Search : { //mode 2
            if(ppu.cycles >= 80) { 
                //no stat interrupt for transitioning to pixel transfer
                // populate_sprite_buffer();

                ppu.is_window = false;
                ppu.pixel = 0;

                SET_STAT_STATE(PIXEL_TRANSFER_FLAG);
                ppu.state = Pixel_Transfer;
            }
        } break;

        case Pixel_Transfer : { //mode 3
            if(ppu.cycles >= 172 + 80) {
                update_bg_win();

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

                    ppu.is_window = false;
                    ppu.can_render = true;

                    can_interrupt = (mem_read(STAT) & (1 << 5)); //test for oam search interrupt

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

//
// We do our rendering on a per-pixel basis rather than a fetcher
// state machine
//
void update_bg_win() 
{    
    // Update tilemap and data on a per-tile basis
    while(ppu.pixel < 160) {
        if(ppu.pixel % 8 == 0) {
            get_bg_win_tilemap_tiledata(); 
        }
        word xoffset = ((ppu.pixel + mem_read(SCX)) / 8) & 0x1F;
        word yoffset = 32 * (((mem_read(LY) + mem_read(SCY)) & 0xFF) / 8);

        if(ppu.is_window) {
            xoffset = ((ppu.pixel - (mem_read(WX) - 7)) / 8) & 0x1F;
            yoffset = 32 * (bg_win_fetcher.window_line_counter / 8);
        }

        word tile_num_base = (xoffset + yoffset) & 0x3FF;
        if(bg_win_fetcher.is_unsigned) {
            bg_win_fetcher.tilenumber = (byte)mem_read(bg_win_fetcher.tilemap + tile_num_base);
        }
        else {
            bg_win_fetcher.tilenumber = (int8_t)mem_read(bg_win_fetcher.tilemap + tile_num_base);
        }

        word tile_data_base = (2 * ((mem_read(LY) + mem_read(SCY)) % 8));
        if(ppu.is_window) {
            tile_data_base = (2 * (bg_win_fetcher.window_line_counter % 8));
        }

        if(bg_win_fetcher.is_unsigned) {
            bg_win_fetcher.tiledata_low = mem_read(tile_data_base + bg_win_fetcher.tiledata + (bg_win_fetcher.tilenumber * 16));
            bg_win_fetcher.tiledata_high = mem_read(tile_data_base + bg_win_fetcher.tiledata + (bg_win_fetcher.tilenumber * 16) + 1);
        }
        else{
            bg_win_fetcher.tiledata_low = mem_read(tile_data_base + bg_win_fetcher.tiledata + ((bg_win_fetcher.tilenumber+128) * 16));
            bg_win_fetcher.tiledata_high = mem_read(tile_data_base + bg_win_fetcher.tiledata + ((bg_win_fetcher.tilenumber+128) * 16) + 1);
        }    
        
        for(int x = 7; x >= 0; x--) {
            uint32_t color = get_color(bg_win_fetcher.tiledata_high, bg_win_fetcher.tiledata_low, x);
            if(!(mem_read(LCDC) & 0x01)) { // bg/win enable bit
                color = COLOR_0;
            }
            ppu.pixel_buffer[mem_read(LY)][ppu.pixel + (7 - x)] = color;
        }
        ppu.pixel += 8;
    }
}

void swap(SpriteEntry* a, SpriteEntry* b)
{
    SpriteEntry c;

    c = *a;
    *a = *b;
    *b = c;
}

void sort_sprites()
{
    int i, swapped;

    do {
        swapped = 0;
        for(i = 0; i < SPRITE_BUFFER_SIZE - 1; i++) {
            if(sprite_buffer[i].x > sprite_buffer[i+1].x) {
                swap(&sprite_buffer[i], &sprite_buffer[i+1]);
                swapped = 1;
            }
        }
    } while(swapped);
}

void populate_sprite_buffer()
{
    sprite_buffer_index = 0; 
    word base = 0xFE00;

    for(int i = 0; i < 40 ; i++) {
        word offset = base + (i * 4);
        byte y = mem_read(offset) - 16;
        byte x = mem_read(offset + 1) - 8;
        byte tile_no = mem_read(offset + 2);
        byte flags = mem_read(offset + 3);

        byte palette = mem_read(OBP0);
        if(flags & (1<<4)) {
            palette = mem_read(OBP1);
        }

        byte bound = mem_read(LY);
        byte height = mem_read(LCDC) & (1<<2) ? 16 : 8;
        if((bound >= y) && (bound < y + height) && sprite_buffer_index < 10) { 
            SpriteEntry s = {
                .y = y,
                .x = x + 8,
                .tile = tile_no,
                .flags = flags,
                .palette = palette,
                .height = height
            };
            printf("Sprite with x: %i and y: %i added to buffer\n", s.x, s.y);
            sprite_buffer[sprite_buffer_index++] = s;
        }
    }

    // Initialize unused entries to invalid positions
    for(int i = sprite_buffer_index; i < 10; i++) {
        sprite_buffer[i].x = 0xFF;
    }

    // Sort our sprite buffer by x position
    if(sprite_buffer_index) {
        sort_sprites();
    }
    sprite_buffer_index = 0; 
}

//
// My current suspicion is that we are unable to handle multiple sprites rendering
// to the fifo in one "cycle". This causes the weird and somewhat cryptic renderin bug
// we are facing where certain tiles are offset by a singular tile, but those that
// are clearly already aligned appear fine.
//
void update_sprites() 
{
    if (sprite_buffer_index == SPRITE_BUFFER_SIZE || sprite_fetcher.full_fifo == true) { 
        return;
    }

    SpriteEntry* sprite = &sprite_buffer[sprite_buffer_index];
    static int x = 7; // So we can render more than once sprite in a tile

    switch(sprite_fetcher.state) {
        case Fetch_Tile_Num: {
            if(sprite->height == 16) { // Ignore bit 0 for 16 bit high tiles
                sprite_fetcher.tilenumber = sprite->tile & 0xFE;
            }
            else {
                sprite_fetcher.tilenumber = sprite->tile;
            }
            sprite_fetcher.flags = sprite->flags;
            sprite_fetcher.state = Fetch_Tile_Data_Low;
        } break;
        
        case Fetch_Tile_Data_Low: {
            int line = mem_read(LY) - sprite->y;
            if(sprite_fetcher.flags & (1<<6)) { // Y-Flip
                line -= (sprite->height - 1);
                line *= -1;
            }
            word addr = 0x8000 + (sprite_fetcher.tilenumber * 16) + (line * 2);
            sprite_fetcher.tiledata_low = mem_read(addr);

            sprite_fetcher.state = Fetch_Tile_Data_High;
        } break;
        
        case Fetch_Tile_Data_High : {
            int line = mem_read(LY) - sprite->y;
            if(sprite_fetcher.flags & (1<<6)) { // Y-Flip
                line -= (sprite->height - 1);
                line *= -1;
            }
            word addr = 0x8000 + (sprite_fetcher.tilenumber * 16) + (line * 2) + 1;
            sprite_fetcher.tiledata_high = mem_read(addr);

            sprite_fetcher.state = Push_To_FIFO;
        } break;

        case Push_To_FIFO : {
            for( ; x >= 0; x--) {
                // Render multiple sprite per tile
                if(sprite_buffer[sprite_buffer_index + 1].x > ppu.pixel + 8 + (7 - x)) {
                    sprite_buffer_index++;
                    sprite_fetcher.state = Fetch_Tile_Num;
                    return ;
                }
                int bit_position = x;
                if(sprite_fetcher.flags & (1<<5)) {
                   bit_position -= 7; 
                   bit_position *= -1;
                }
                uint32_t color = get_color(sprite_fetcher.tiledata_high, sprite_fetcher.tiledata_low, bit_position);
                sprite_fetcher.fifo[x] = color;
            }
            x = 7;
            sprite_buffer_index++;
            sprite_fetcher.full_fifo = true;

            sprite_fetcher.state = Fetch_Tile_Num;
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
    
        if(ly >= wy && ppu.pixel >= wx && wy < 144) {
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
