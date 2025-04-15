#include "stdio.h"
#include <SDL2/SDL.h>

#include "../include/emulate.h"

void emulate() {
    int quit = false;
    const Uint32 frame_delay = 32; // Force 60fps
    Uint32 frame_start;
    Uint32 frame_time;
    
    while(!quit) {
        frame_start = SDL_GetTicks();
        
        // Render full frame first then handle inputs
        for (int i = 0; i < 70224; i++) { 
            cpu_cycle();
            update_timers();
            do_interrupts();
            ppu_cycle();
        }
        
        poll_joypad_input(&quit);
        update_display(&quit);
        // printf("%c", perform_serial());
        
        frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < frame_delay) {
            SDL_Delay(frame_delay - frame_time);
        }
    }
}