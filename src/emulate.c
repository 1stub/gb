#include "../include/emulate.h"

void emulate() {
    while(1) {
        cpu_cycle();
        do_interrupts();
        printf("%c", perform_serial());
    }
}