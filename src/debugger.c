#include "../include/debugger.h"

#include <SDL2/SDL_opengl.h>

//per: https://github.com/Immediate-Mode-UI/Nuklear/tree/master/demo/sdl_opengl2
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_GL2_IMPLEMENTATION
#include "../../nuklear.h"
#include "../util/nuklear_sdl_gl2.h"

void init_debugger(SDL_Window* win)
{
    nk_sdl_init(win);
}