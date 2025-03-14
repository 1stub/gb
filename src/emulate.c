#include "../include/timer.h"

void emulate() {
    while(1) {
        cpu_cycle();
        do_interrupts();
        //timer updates will need to occur at a rate of ~16384 hz, doesnt right now
        update_timers();
        printf("%c", perform_serial());
    }
}