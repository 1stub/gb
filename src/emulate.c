#include "../include/timer.h"

void emulate() {
    while(1) {
        cpu_cycle();
        do_interrupts();
        update_timers();
        printf("%c", perform_serial());
    }
}