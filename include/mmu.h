#include "../include/common.h"
#include <stdio.h>

//0x0000-3FFF ROM Bank 0
//0x4000-7FFF ROM Bank 1
//0x8000-9FFF VRAM
//0xA000-BFFF External RAM
//0xC000-0xCFFF Work RAM 0
//0xD000-0xDFFF Work RAM 1
//0xE000-FDFF ECHO (same as C000-DDFF)
//0xFE00-0xFE9F OAM Sprite Attrib Table
//0xFF00-0xFF7F I/O
//0xFF80-0xFFFE HRAM
//0xFFFF Interrupt Enable

typedef struct{
    byte memory[0x10000];
    int divider_counter;
}MMU;

extern MMU mmu;

extern void mmu_init();
extern byte mem_read(word address);
extern word mem_read16(word address);
extern void mem_write(word address, byte value);
extern void load_rom(char *file);
char perform_serial();
