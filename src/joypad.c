#include "../include/joypad.h"
#include "../include/common.h"
#include "../include/interrupt.h"
#include "../include/mmu.h"

#include <SDL2/SDL.h>

byte joypad_state = 0xFF;
SDL_Event e;

//
// Looks like the main issue is performance while polling for input.
// See https://stackoverflow.com/questions/58639564/why-is-sdl-pollevent-so-slow
// This likely warrants a switch to SDL3.
//
void poll_joypad_input(int* quit)
{
    while( SDL_PollEvent( &e ) ) { 
        switch(e.type) {
            case SDL_QUIT: {
                *quit = 1;
            }
            break;
            case SDL_KEYDOWN: { 
                key_pressed(keymap(e.key.keysym.sym));
            }
            break;

            case SDL_KEYUP: {
                key_released(keymap(e.key.keysym.sym));
            }
            break;

            default: break;
        }
    }
}

void key_pressed(int keynum)
{
    int prev_unset = false;

    if(joypad_state & (1<<keynum)) {
        prev_unset = true;
    }
    joypad_state &= ~(1<<keynum);

    int was_button_pressed = true;
    if(keynum <= 3) {
        was_button_pressed = false;
    }

    byte key_request = mmu.memory[JOYP];
    int should_interrupt = false;

    if(was_button_pressed && !(key_request & (1<<keynum))) {
        should_interrupt = true;
    }
    else if(!was_button_pressed && !(key_request & (1<<keynum))) {
        should_interrupt = true;
    }

    if(should_interrupt && prev_unset) {
        request_interrupt(4);
    }
}

void key_released(int keynum) 
{
    joypad_state |= (1<<keynum);
}

void update_joypad()
{
    byte internal_state = mmu.memory[JOYP];
    internal_state ^= 0xFF;

    // Are we looking for dpad?
    if(!(internal_state & (1<<4))) {
        byte dpad_state = joypad_state >> 4;
        dpad_state |= 0xF0;
        internal_state &= dpad_state;
    }
    else if(!(internal_state & (1<<5))) {
        byte others_state = joypad_state & 0xF;
        others_state |= 0xF0;
        internal_state &= others_state;
    }

    mmu.memory[JOYP] = internal_state;
}

int keymap(unsigned char k) {
    switch (k) {
        case 'w': return 0x02;
        case 'a': return 0x01;
        case 's': return 0x03;
        case 'd': return 0x00;

        case 'k': return 0x04;
        case 'l': return 0x05;
        case 'g': return 0x06;
        case 'h': return 0x07;

        default:  return -1;  
    }
}