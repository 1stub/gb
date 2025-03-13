#include "../include/emulate.h"

void emulate() {
    while(1) {
        cpu_cycle();
        printf("%c", perform_serial());
    }
}