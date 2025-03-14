#pragma once

#include "mmu.h"
#include "cpu.h"

extern void request_interrupt(int interrupt);
extern void service_interrupt(int interrupt);
extern void do_interrupts();
