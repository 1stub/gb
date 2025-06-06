#include "../include/mmu.h"
#include "../include/cpu.h"
#include "../include/interrupt.h"
#include "../include/joypad.h"
#include "../include/mbc1.h"
#include <assert.h> //assert, duh
#include <string.h> //memset

#include <stdlib.h> //malloc

#define MEM mmu.memory 
MMU mmu;

void do_dma(byte data);

void mmu_init()
{
    MEM[0xFF00] = 0x1F;
    MEM[TIMA] = 0x00 ; 
    MEM[TMA]  = 0x00 ; 
    MEM[TAC]  = 0x00 ; 
    MEM[NR] = 0x80 ;
    MEM[0xFF11] = 0xBF ;
    MEM[0xFF12] = 0xF3 ;
    MEM[0xFF14] = 0xBF ;
    MEM[0xFF16] = 0x3F ;
    MEM[0xFF17] = 0x00 ;
    MEM[0xFF19] = 0xBF ;
    MEM[0xFF1A] = 0x7F ;
    MEM[0xFF1B] = 0xFF ;
    MEM[0xFF1C] = 0x9F ;
    MEM[0xFF1E] = 0xFF ;
    MEM[0xFF20] = 0xFF ;
    MEM[0xFF21] = 0x00 ;
    MEM[0xFF22] = 0x00 ;
    MEM[0xFF23] = 0xBF ;
    MEM[0xFF24] = 0x77 ;
    MEM[0xFF25] = 0xF3 ;
    MEM[0xFF26] = 0xF1 ;
    MEM[LCDC] = 0x91 ; 
    MEM[SCY]  = 0x00 ;
    MEM[SCX]  = 0x00 ;
    MEM[LYC]  = 0x00 ;
    MEM[BGP]  = 0xFC ;
    MEM[JOYP] = 0xFF ;
    MEM[OBP0] = 0xFF ;
    MEM[OBP1] = 0xFF ;
    MEM[0xFF4A] = 0x00 ;
    MEM[0xFF4B] = 0x00 ;
    MEM[IE] = 0x00 ;

    // Not completely sure, but we may need a fresh copy of cart ROM stored

    mbc1.is_enabled = false;

    // Cart header MBC stuff
    switch(MEM[0x147]) {
        case 0: mbc1.is_enabled = false; break;
        case 1: mbc1.is_enabled = true; break;
        case 2: mbc1.is_enabled = true; break;
        case 3: mbc1.is_enabled = true; break;
        default: assert(false);
    }

    // Important initializers
    mbc1.ram_bank_number = 0;
    mbc1.rom_bank_number = 1;
    mbc1.ram_enable = false;

    memset(&mbc1.ram_banks, 0, sizeof mbc1.ram_banks);

    printf("mbc1 %i\n", mbc1.is_enabled);
}

byte mem_read(word address){
    if(GB_ENABLE_JSON_TESTING) {
        return MEM[address];
    }
    if(address < 0x4000) {
        return MEM[address];
    }
    // Check if reading from rom memory bank
    else if(address >= 0x4000 && address <= 0x7FFF) {
        word newaddr = address - 0x4000;
        return mmu.cart_rom[newaddr + (mbc1.rom_bank_number * 0x4000)];
    }
    // Check if reading from ram memory bank
    else if(address >= 0xA000 && address <= 0xBFFF) {
        word newaddr = address - 0xA000;
        return mbc1.ram_banks[newaddr + (mbc1.ram_bank_number * 0x2000)];
    }
    // Capture attempt to read joypad
    else if(address == JOYP) {
        return update_joypad();
    }
    // ECHO memory
    else if(address >= 0xE000 && address <= 0xFDFF) {
        return MEM[address - 0x2000];
    }
    else {
        return MEM[address];
    }
}

word mem_read16(word address){
    byte base = mem_read(address);
    byte next = mem_read(address + 1);

    return base | (next << 8);
}

//TODO: Handle cases where specific bits(or whole registers) are read only
void mem_write(word address, byte value){
    if(GB_ENABLE_JSON_TESTING) {
        MEM[address] = value;
        return ;
    }
    if(address <= 0x7FFF) {
        handle_banking_mbc1(address, value);
    }
    else if(address >= 0xA000 && address <= 0xBFFF) {
        if(mbc1.ram_enable) {
            word newaddr = address - 0xA000;
            mbc1.ram_banks[newaddr + (mbc1.ram_bank_number * 0x2000)] = value;
        }
    }
    else if (address <= 0xFDFF && address >= 0xE000) {
        // No clue if this is how to properly handle ECHO writes
        MEM[address - 0x2000] = value;
    }
    else if(address == LY) {
        MEM[LY] = 0;
    }
    else if (address == LCDC) {
        MEM[address] = value;
        if (!(value & (1 << 7))) {
            MEM[LCDC] = 0x00;
            MEM[STAT] &= 0x7C;
        }
    }
    else if(address == LYC) {
        MEM[address] = value; 

        byte stat = MEM[STAT];
        byte ly = MEM[LY];
        
        stat = (stat & ~(1 << 2)) | ((ly == value) << 2);
        MEM[STAT] = stat;
        
        if((stat & (1 << 6)) && (ly == value)) {
            request_interrupt(1); // STAT interrupt
        }
    }
    else if(address == DIV) {
        MEM[DIV] = 0;
        mmu.divider_counter = 0;
    }
    else if(address == DMA) {
        MEM[DMA] = value;
        do_dma(value);
    }
    else if(address == JOYP) {
        // Preserve lower bits (button states) while allowing writes to bits 4 & 5
        MEM[JOYP] = (value & 0x30);
    }
    else if(address == STAT) {
        // Preserve lower 2 bits (current LCD mode) while allowing writes to other bits
        MEM[STAT] = (value & 0xFC) | (MEM[STAT] & 0x03);
    }
    else {
        MEM[address] = value;
    }
}

void load_rom(char *file){
    FILE *fp;
    fp = fopen(file, "rb");
    if(!fp){
        perror("fopen - unable to open argv[1]");
        return ;
    }

    // We get the size of whole rom and store it in mmu.cart_rom
    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    mmu.cart_rom = (byte*)malloc(file_size);

    size_t crom = fread(mmu.cart_rom, sizeof(byte), file_size, fp);
    size_t ram_size = file_size >= 0x8000 ? 0x8000 : file_size;
    memcpy(MEM, mmu.cart_rom, ram_size);

    if (crom == 0) {
        fprintf(stderr, "fread() failed or file is empty\n");
    }
    if(!crom){
        fprintf(stderr, "fread() failed: %zu\n", crom);
    }

    fclose(fp);
}

void do_dma(byte data)
{
    word address = data << 8;
    for (int i = 0 ; i < 0xA0; i++){
        mem_write(0xFE00 + i, mem_read(address + i));
    }
}

char perform_serial(){
    if(!(MEM[SC] & (1 << 7))){
        return '\0';    
    }
    MEM[SC] &= ~(1 << 7);

    const char data = (char)MEM[SB];
    return data;
}
