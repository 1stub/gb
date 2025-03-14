#include "../include/interrupt.h"

void request_interrupt(int interrupt)
{
    byte data = mem_read(IF);
    data |= (1 << interrupt);
    mem_write(IF, data);
}

void service_interrupt(int interrupt)
{
    IME = 0;
    byte req = mem_read(IF);
    req &= ~(1 << interrupt); //clear interrupt bit
    mem_write(IF, req);

    PUSH(&PC); //this is a direct call to push instruction from our cpu

    switch(interrupt){ 
        case 0: PC = 0x40; break; //vblank
        case 1: PC = 0x48; break; //LCD / STAT
        case 2: PC = 0x50; break; //Timer
        case 3: PC = 0x58; break; //serial
        case 4: PC = 0x60; break; //Joypad
    }
}

void do_interrupts()
{
    if(IME){
        byte req = mem_read(IF);
        byte flag = mem_read(IE);
        if(req > 0){
            for(int i = 0; i < 5; i++){
                if(req & (1 << i)){
                    if(flag & (1 << i)){
                        service_interrupt(i);
                    }
                }
            }
        }
    }
}