#pragma once
#include <SDL2/SDL.h>

extern void init_debugger(SDL_Window* win);
extern void render_debugger();
extern void debugger_start_input();
extern void debugger_poll_input(SDL_Event *e);
extern void debugger_end_input();
extern void cleanup_debugger();