#include "../util/json_test.h"

extern "C" {
    #include "../include/cpu.h"
}

int main(int argc, char** argv) {
    cpu_init();
    mmu_init();

    run_json_tests();
    
    return 0;
}