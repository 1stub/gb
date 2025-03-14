#include "../include/timer.h"
#include "../include/display.h"

void emulate() {
    int quit = 0;

    while(!quit) {
        cpu_cycle();
        do_interrupts();
        update_timers();
        update_display(&quit);
        printf("%c", perform_serial());
    }
}