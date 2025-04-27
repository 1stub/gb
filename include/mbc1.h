#pragma once

#include "common.h"

typedef struct {
    int is_enabled;
    int ram_enable;
    byte rom_bank_number;
    byte ram_bank_number;
    byte ram_banks[0x8000];
    byte cart_rom[0x8000];
} MBC1;

extern MBC1 mbc1;

void handle_banking_mbc1(word address, byte value);
