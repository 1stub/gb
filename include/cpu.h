#pragma once

#include "../include/common.h"
#include <stdint.h>

#define FLAG_Z 1<<7
#define FLAG_N 1<<6
#define FLAG_H 1<<5
#define FLAG_C 1<<4

typedef struct{
    struct{
        union{
            struct {
                byte f;
                byte a;
            } ;
            word af;
        };
    };
    struct{
        union {
            struct {
                byte c;
                byte b;
            };
            word bc;
        };
    };
    struct{
        union {
            struct {
                byte e;
                byte d;
            };
            word de;
        };
    };
    struct{
        union {
            struct {
                byte l;
                byte h;
            };
            word hl;
        }; 
    };
} registers;

typedef struct{
    registers regs;
    byte cycles;
    byte ime;
    word sp;
    word pc;
    byte is_halted;
    byte halt_bug;
    byte can_run;
    byte should_step;
} CPU;

extern CPU cpu;

#define PC  cpu.pc
#define SP  cpu.sp
#define IME cpu.ime

#define A  cpu.regs.a
#define F  cpu.regs.f
#define AF cpu.regs.af
#define B  cpu.regs.b
#define C  cpu.regs.c
#define BC cpu.regs.bc
#define D  cpu.regs.d
#define E  cpu.regs.e
#define DE cpu.regs.de
#define H  cpu.regs.h
#define L  cpu.regs.l
#define HL cpu.regs.hl

//This needs to be visible to interrupt.c
extern void PUSH(word *dst);

byte cpu_cycle();
void cpu_init();
void print_registers();
const char* log_registers();
void find_wrong_register (byte a, byte b, byte c, byte d, byte e, byte f, byte h, byte l, word pc, word sp);
void set_cpu_registers(byte a, byte b, byte c, byte d, byte e, byte f, byte h, byte l, word pc, word sp);
int check_cpu_registers(byte a, byte b, byte c, byte d, byte e, byte f, byte h, byte l, word pc, word sp);
