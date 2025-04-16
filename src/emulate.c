#include "stdio.h"
#include <SDL2/SDL.h>

#include "../include/emulate.h"

//
// Something kinda odd is even though the timing feels right
// it is very clear we are rendering at ~30fps rather than 60.
// Will need to give this a better look
//

void emulate() {
    int quit = false;
    //
    // This number should be 16 ms but it appears we are
    // runnign 4x speed at 16ms per frame
    //
    const Uint32 frame_delay = 64; // Force 60fps
    Uint32 frame_start;
    Uint32 frame_time;
    
    while(!quit) {
        frame_start = SDL_GetTicks();
        
        // Render full frame first then handle inputs
        for (int i = 0; i <= 70224; i++) { 
            cpu_cycle();
            update_timers();
            do_interrupts();
            ppu_cycle();
        }
        
        poll_joypad_input(&quit);
        update_display(&quit);
        print_registers();
        // printf("%c", perform_serial());
        
        frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < frame_delay) {
            SDL_Delay(frame_delay - frame_time);
        }
    }
}