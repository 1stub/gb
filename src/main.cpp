#include "../util/json_test.h"

extern "C" {
    #include "../include/cpu.h"
}

int main(int argc, char** argv) {
    cpu_init();
    mmu_init();

    #ifdef GB_ENABLE_JSON_TESTING
    run_json_tests();
    #endif

    load_rom(argv[1]);
    
    return 0;
}