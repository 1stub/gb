#include "../include/timer.h"

//This will operate on the notion our cpu updates ever 4 T-Cycles
void update_timers() 
{
    static int timer_counter = 0;

    mmu.divider_counter += 4;
    if(mmu.divider_counter >- 256) {
        mmu.memory[DIV]++; //dont call write, resets to 0
        mmu.divider_counter -= 256;
    }

    byte TAC_value = mem_read(TAC);
    if(TAC_value & (1 << 2)){
        timer_counter += 4;
        int timer_thresh = 0;
        byte speed = TAC_value & 0x03;
        switch(speed){ //we read the speed at which our clock should update
            case 0x00: timer_thresh = 1024; break; //freq 4096
            case 0x01: timer_thresh = 16; break; //freq 262114
            case 0x02: timer_thresh = 64; break; //freq 65536
            case 0x03: timer_thresh = 256; break; //freq 16382
            default: break;
        }
        if(timer_counter > timer_thresh){
            timer_counter -= timer_thresh;
            if(mem_read(TIMA) == 0xFF){ //timer counter overflow, need interrupt
                mem_write(TIMA, mem_read(TMA));
                request_interrupt(2);
            }else{
                mem_write(TIMA, mem_read(TIMA)+1);
            }
        }
    }
}