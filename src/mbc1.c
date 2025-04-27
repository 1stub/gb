#include "../include/mbc1.h"

MBC1 mbc1;

//
// All of this needs a deeper look and comparison with info in pandocs,
// it seems bank1 is pretty donked up.
//

void enable_ram_banking(word address, byte value);
void change_low_rom_bank(byte value);
void change_high_rom_bank(byte value);
void change_ram_bank(byte value);
void change_rom_or_ram_mode(byte value);

void handle_banking_mbc1(word address, byte value) 
{
    if(address < 0x2000) {
        if(mbc1.is_enabled) {
            enable_ram_banking(address, value);
        }
    }
    else if(address >= 0x2000 && address < 0x4000) {
        if(mbc1.is_enabled) {
            change_low_rom_bank(value);
        }
    }
    else if(address >= 0x4000 && address < 0x6000) {
        if(!mbc1.is_enabled) {
            return ;
        }
        if(mbc1.ram_enable) {
            change_ram_bank(value);
        }
        else {
            change_high_rom_bank(value);
        }
    }
    else if(address >= 0x6000 && address < 0x8000) {
        if(mbc1.is_enabled) {
            change_rom_or_ram_mode(value);
        }
    }
}

void enable_ram_banking(word address, byte value)
{
    byte testdata = value & 0x0F;
    if(testdata == 0x0A) {
        mbc1.ram_enable = true;
    }
    else {
        mbc1.ram_enable = false;
    }
}

void change_low_rom_bank(byte value)
{
    byte lower_five_bits = value & 0x1F;
    mbc1.rom_bank_number &= 0xE0; // Turn off lower five
    mbc1.rom_bank_number |= lower_five_bits;

    switch(mbc1.rom_bank_number) {
        case 0x00: break;
        case 0x20: break;
        case 0x40: break;
        case 0x60: mbc1.rom_bank_number++; break; 
        default: break;
    }
}

void change_high_rom_bank(byte value)
{
    mbc1.rom_bank_number &= 0x1F; // Disable upper 3 bits
    value &= 0xE0;
    mbc1.rom_bank_number |= value;

    switch(mbc1.rom_bank_number) {
        case 0x00: break;
        case 0x20: break;
        case 0x40: break;
        case 0x60: mbc1.rom_bank_number++; break; 
        default: break;
    }
}

void change_ram_bank(byte value) 
{
    mbc1.ram_bank_number = value & 0x03;
}

void change_rom_or_ram_mode(byte value)
{
    mbc1.ram_enable = ((value & 0x01)) ? false : true;
    if(!mbc1.ram_enable) {
        mbc1.ram_bank_number = 0;
    }
}