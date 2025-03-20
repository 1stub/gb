#include "../include/debugger.h"
#include "../include/cpu.h"

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

//
//A fun improvement to the debugger could be to disassemble
//the binary so we can see every instruction running
//
void render_debugger()
{
    if(nk_begin(ctx, "Registers", nk_rect(300, 10, 200, 300), 
        NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
        NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE) ) {
            nk_layout_row_dynamic(ctx, 20, 1); //include this or registers dont print
            char buffer[32];

            snprintf(buffer, sizeof(buffer), "A: 0x%02X", cpu.regs.a);
            nk_label(ctx, buffer, NK_TEXT_LEFT);

            snprintf(buffer, sizeof(buffer), "B: 0x%02X", cpu.regs.b);
            nk_label(ctx, buffer, NK_TEXT_LEFT);

            snprintf(buffer, sizeof(buffer), "C: 0x%02X", cpu.regs.c);
            nk_label(ctx, buffer, NK_TEXT_LEFT);

            snprintf(buffer, sizeof(buffer), "D: 0x%02X", cpu.regs.d);
            nk_label(ctx, buffer, NK_TEXT_LEFT);

            snprintf(buffer, sizeof(buffer), "E: 0x%02X", cpu.regs.e);
            nk_label(ctx, buffer, NK_TEXT_LEFT);

            snprintf(buffer, sizeof(buffer), "F: 0x%02X", cpu.regs.f);
            nk_label(ctx, buffer, NK_TEXT_LEFT);

            snprintf(buffer, sizeof(buffer), "L: 0x%02X", cpu.regs.h);
            nk_label(ctx, buffer, NK_TEXT_LEFT);

            snprintf(buffer, sizeof(buffer), "H: 0x%02X", cpu.regs.l);
            nk_label(ctx, buffer, NK_TEXT_LEFT);

            snprintf(buffer, sizeof(buffer), "SP: 0x%02X", cpu.sp);
            nk_label(ctx, buffer, NK_TEXT_LEFT);

            snprintf(buffer, sizeof(buffer), "PC: 0x%02X", cpu.pc);
            nk_label(ctx, buffer, NK_TEXT_LEFT);

            snprintf(buffer, sizeof(buffer), "IME: 0x%02X", cpu.ime);
            nk_label(ctx, buffer, NK_TEXT_LEFT);
    }
    nk_end(ctx);

    if(nk_begin(ctx, "Control", nk_rect(300, 320, 200, 100),
        NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|
        NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE) ) {
            nk_layout_row_static(ctx, 50, 80, 2);
            if (nk_button_label(ctx, "stop")) {
                cpu.can_run = !cpu.can_run;
            }
            if (nk_button_label(ctx, "step")) {
                cpu.should_step = 1;
                cpu.can_run = 1;
            }
    }
    nk_end(ctx);
    
    nk_sdl_render(NK_ANTI_ALIASING_ON);
}

void debugger_start_input()
{
    nk_input_begin(ctx);
}

void debugger_poll_input(SDL_Event *e)
{
    nk_sdl_handle_event(e);
}

void debugger_end_input() 
{
    nk_sdl_handle_grab(); //optional grabbing behaviour (enabled rn)
    nk_input_end(ctx);
}

void clearGLColorNuklear() 
{
    glClearColor(bg.r, bg.g, bg.b, bg.a);
}

void cleanup_debugger()
{
    nk_sdl_shutdown();
}