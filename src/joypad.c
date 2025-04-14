#include "../include/joypad.h"
#include "../include/common.h"
#include "../include/interrupt.h"

byte joypad_state = 0x00;

void key_pressed()
{
    request_interrupt(4);
}

void key_released() 
{

}

void update_joypad()
{

}