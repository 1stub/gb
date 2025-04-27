#include "../include/mbc1.h"

MBC1 mbc1;

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
        if(mbc1.enable_rom) {
            change_high_rom_bank(value);
        }
        else {
            change_ram_bank(value);
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
        mbc1.enable_ram = true;
    }
    else if(testdata == 0x00) {
        mbc1.enable_ram = false;
    }
}

void change_low_rom_bank(byte value)
{
    byte lower_five_bits = value & 31;
    mbc1.current_rom_bank &= 224; // TODO make this hex
    mbc1.current_rom_bank |= lower_five_bits;
    if(mbc1.current_rom_bank == 0) mbc1.current_rom_bank++;
}

void change_high_rom_bank(byte value)
{
    mbc1.current_rom_bank &= 31;
    value &= 224;
    mbc1.current_rom_bank |= value;
    if(mbc1.current_rom_bank == 0) mbc1.current_rom_bank++;
}

void change_ram_bank(byte value) 
{
    mbc1.current_ram_bank = value & 0x03;
}

void change_rom_or_ram_mode(byte value)
{
    mbc1.enable_rom = (!(value & 0x01)) ? true : false;
    if(mbc1.enable_rom) {
        mbc1.current_ram_bank = 0;
    }
}