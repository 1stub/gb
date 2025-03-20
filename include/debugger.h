#pragma once
#include <SDL2/SDL.h>

void init_debugger(SDL_Window* win);
void render_debugger();
void debugger_start_input();
void debugger_poll_input(SDL_Event *e);
void debugger_end_input();
void cleanup_debugger();