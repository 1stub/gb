#include "stdio.h"

#include "../include/emulate.h"

void emulate() {
    int quit = false;

    //
    //A quite important TODO is to make sure we only update the display
    //~60 times a second. Right now we try to render as frequently as 
    //possible which leads to a super slow display; Maybe use some
    //sort of simple timer that triggers once a certain number of hz 
    //have elapsed and then resets after we display? idk. We also need
    //to always have the debugger update even when we are not rendering
    //the actual display pixels.
    //

    //
    //TODO: need to actually understand why this emulation order works
    //and my prev approach of doing exec, render, timer, int did not
    //
    while(!quit) {
        //Order here is VERY important
        cpu_cycle();
        update_timers();
        do_interrupts();
        ppu_cycle();
        if(ppu.can_render) {
            poll_joypad_input(&quit);
            update_display(&quit);
        }
        printf("%c", perform_serial());
    }
}