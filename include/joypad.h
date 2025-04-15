#pragma once

#include "common.h"

void poll_joypad_input(int* quit);
void key_pressed(int keynum);
void key_released(int keynum);
byte update_joypad();
int keymap(unsigned char k);