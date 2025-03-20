#include "../include/timer.h"
#include "../include/display.h"
#include "../include/ppu.h"

void emulate() {
    int quit = 0;

    //
    //A quite important TODO is to make sure we only update the display
    //~60 times a second. Right now we try to render as frequently as 
    //possible which leads to a super slow display; Maybe use some
    //sort of simple timer that triggers once a certain number of hz 
    //have elapsed and then resets after we display? idk. We also need
    //to always have the debugger update even when we are not rendering
    //the actual display pixels.
    //
    while(!quit) {
        cpu_cycle();
        ppu_cycle();
        do_interrupts();
        update_timers();
        update_display(&quit);
        printf("%c", perform_serial());
    }
}