#include "../util/json_test.h"

extern "C" {
    #include "../include/emulate.h"
}

int main(int argc, char** argv) {
    cpu_init();
    mmu_init();
    ppu_init();
    setup_display();

    //TODO: Make sure this only compiles in test/debug mode or somethign
    #ifdef GB_ENABLE_JSON_TESTING
    run_json_tests();
    #endif

    load_rom(argv[1]);
    emulate();
    
    return 0;
}