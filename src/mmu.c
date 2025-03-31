#include "../include/mmu.h"
#include "../include/cpu.h"
#include "../include/interrupt.h"
#include <string.h>

#define MEM mmu.memory 
MMU mmu;

void mmu_init(){
    memset(MEM, 0, sizeof(MEM) * sizeof(byte));

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
    MEM[OBP0] = 0xFF ;
    MEM[OBP1] = 0xFF ;
    MEM[JOYP] = 0x00;
    MEM[0xFF4A] = 0x00 ;
    MEM[0xFF4B] = 0x00 ;
    MEM[IE] = 0x00 ;
}

byte mem_read(word address){
    return MEM[address];
}

word mem_read16(word address){
    return MEM[address] | (MEM[address+1] << 8);
}

//TODO: Handle cases where specific bits(or whole registers) are read only
void mem_write(word address, byte value){
    if(address == LY) {
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
        printf("lyc write %x\n", value);

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
        //executeDmaTransfer(value);
        MEM[address] = value;
    }
    else if(address == JOYP) {
        // Preserve lower bits (button states) while allowing writes to bits 4 & 5
        MEM[JOYP] = (value & 0x30) | (MEM[JOYP] & 0xCF);
    }
    else if(address == STAT) {
        // Preserve lower 2 bits (current LCD mode) while allowing writes to other bits
        MEM[STAT] = (value & 0x7C) | (MEM[STAT] & 0x03);
    }
    else {
        MEM[address] = value;
    }

    #ifdef GB_ENABLE_JSON_TESTING
    MEM[address] = value;
    #endif
    
}

void load_rom(char *file){
    FILE *fp;
    fp = fopen(file, "rb");
    if(!fp){
        perror("fopen - unable to open argv[1]");
        return ;
    }

    size_t ret = fread(MEM, sizeof(byte), 0x10000, fp);
    if (ret == 0) {
        fprintf(stderr, "fread() failed or file is empty\n");
    }
    if(!ret){
        fprintf(stderr, "fread() failed: %zu\n", ret);
    }

    fclose(fp);
}

#if 0
char perform_serial(){
    if(!(MEM[SC] & (1 << 7))){
        return '\0';    
    }
    MEM[SC] &= ~(1 << 7);

    const char data = (char)MEM[SB];
    return data;
}
#endif