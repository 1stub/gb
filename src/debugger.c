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

struct nk_context *ctx;
struct nk_colorf bg;

void init_debugger(SDL_Window* win)
{
    ctx = nk_sdl_init(win);
    bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;

    //dont forget to set font
    struct nk_font_atlas *atlas;
    struct nk_font_config config = nk_font_config(0);
    struct nk_font *font;

    nk_sdl_font_stash_begin(&atlas);
    font = nk_font_atlas_add_default(atlas, 13.0f, &config);
    nk_sdl_font_stash_end();
    nk_style_set_font(ctx, &font->handle);
}

void render_debugger()
{
    /* GUI */
    if(nk_begin(ctx, "Registers", nk_rect(300, 10, 200, 200), NK_WINDOW_TITLE | NK_WINDOW_MOVABLE )){

    }
    nk_end(ctx);
    
    nk_sdl_render(NK_ANTI_ALIASING_ON);
}

void clearGLColorNuklear() 
{
    glClearColor(bg.r, bg.g, bg.b, bg.a);
}

void cleanup_debugger()
{
    nk_sdl_shutdown();
}