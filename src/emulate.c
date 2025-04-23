#include "stdio.h"
#include <SDL2/SDL.h>

#include "../include/emulate.h"

#define CYCLES_PER_FRAME 70224

//
// The issue currently with flickering is related to how our ppu handles
// being turned off and how this creates issues down the pipeline. There
// will be slight inconsistencies that appear slowly as we run the ppu
// and the number of cycles progressively become more and more off.
//
// Also catrap decided to fucking break...
//
// Maybe current rendering bug is related to STAT IRQ blocking? i didnt implement it
//

void emulate() {
    int quit = false;
    const Uint32 frame_delay = 16; // Force 60fps
    Uint32 frame_start;
    Uint32 frame_time;
    
    while(!quit) {
        frame_start = SDL_GetTicks();

        poll_joypad_input(&quit);
        if(ppu.can_render) {
            update_display(&quit);
        }

        // Render full frame first then handle inputs
        int curr_cycles = 0;
        while (curr_cycles < CYCLES_PER_FRAME) { 
            if (cpu.is_halted) {
                int cycles = 4;
                curr_cycles += cycles;
                update_timers(cycles);
                ppu_cycle(cycles);

                //
                // This logic will need to be embedded better into cpu 
                // The issue was our cpu became disabled and never met the condidtion
                // to unhalt. I will leave this here for now so I dont forget this
                // behaviour.
                //
                if (IME & mmu.memory[0xFF0F] & mmu.memory[0xFFFF] & 0x1F) {
                    cpu.is_halted = false;
                }

                do_interrupts();
            }
            else {
                int cycles = cpu_cycle();
                curr_cycles += cycles;
                update_timers(4);
                ppu_cycle(cycles);
                do_interrupts();
            }
        }
        // printf("%c", perform_serial());

        frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < frame_delay) {
            SDL_Delay(frame_delay - frame_time);
        }
    }
}