#pragma once

#define GB_ENABLE_DEBUGGER
#define GB_ENABLE_JSON_TESTING

#define true 1
#define false 0

typedef unsigned short word;
typedef unsigned char byte;

#define GB_DISPLAY_WIDTH 160
#define GB_DISPLAY_HEIGHT 144

#define WINDOW_WIDTH 512
#define WINDOW_HEIGHT 512

#define JOYP 0xFF00
#define SB   0xFF01
#define SC   0xFF02
#define DIV  0xFF04
#define TIMA 0xFF05
#define TMA  0xFF06
#define TAC  0xFF07
#define NR   0xFF10
#define LCDC 0xFF40
#define STAT 0xFF41
#define SCY  0xFF42
#define SCX  0xFF43
#define LY   0xFF44
#define LYC  0xFF45
#define DMA  0xFF46
#define WY   0xFF4A
#define WX   0xFF4B
#define BGP  0xFF47
#define OBP0 0xFF48
#define OBP1 0xFF49
#define IE   0xFFFF
#define IF   0xFF0F
