#pragma once

#include "common.h"

typedef struct {
    int is_enabled;
    int enable_ram;
    int enable_rom;
    byte ram_banks[0x8000];
    byte current_rom_bank;
    byte current_ram_bank;
} MBC1;

extern MBC1 mbc1;

void handle_banking_mbc1(word address, byte value);
