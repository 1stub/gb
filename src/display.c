#include "../include/display.h"
#include "../include/debugger.h"
#include "../include/ppu.h"
#include "../include/joypad.h"

#include <SDL2/SDL_opengl.h>
#include <assert.h>

SDL_Window* win = NULL;
SDL_Texture* gb = NULL;
SDL_Renderer* renderer = NULL;
int win_height, win_width;

#define WINDOW_SCALE 4
#define WINDOW_WIDTH (GB_DISPLAY_WIDTH * WINDOW_SCALE)
#define WINDOW_HEIGHT (GB_DISPLAY_HEIGHT * WINDOW_SCALE)

#define HANDLE_DEBUGGER_INPUT() \
do {\
    debugger_start_input();\
    while( SDL_PollEvent( &e ) ) {\
        debugger_poll_input(&e);\
        switch(e.type) {\
            case SDL_QUIT: {\
                cleanup();\
                *quit = 1;\
            }\
            break;\
            case SDL_KEYDOWN: {\
            }\
            break;\
            case SDL_KEYUP: {\
            }\
            break;\
            default: break;\
        }\
    }\
    debugger_end_input();\
} while(0);

//
// NOTE: It would be way better if we didnt use if stmts to enable/disable debugger but rather pound define
// all debugger methods so we dont emit any code for it when disabled. Would be good for json tests aswell.
//
void cleanup();

void setup_display() 
{
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        SDL_GetError();
        assert(0);
    }

    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    win = SDL_CreateWindow("gameboy",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN|SDL_WINDOW_ALLOW_HIGHDPI);

    if(!win) {
        fprintf(stderr, "Failed to create SDL window!\n");
        assert(false);
    }
    SDL_GetWindowSize(win, &win_width, &win_height);

    //Not confident on renderer accelerated being necessary
    renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    gb = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_STREAMING, GB_DISPLAY_WIDTH, GB_DISPLAY_HEIGHT);
    
    if(GB_ENABLE_DEBUGGER) {
        init_debugger(win);
    }
}

void render_pixel_buffer()
{
    SDL_RenderClear(renderer);
    SDL_UpdateTexture(gb, NULL, ppu.pixel_buffer, 160 * sizeof(uint32_t));
    SDL_RenderCopy(renderer, gb, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void update_display(int* quit) 
{
    if(*quit) {
        cleanup();
        return ;
    }
    if(GB_ENABLE_DEBUGGER) {
        clearGLColorNuklear();  //Clears whole display
    }

    render_pixel_buffer();
    ppu.can_render = 0; 

    if(GB_ENABLE_DEBUGGER) {
        render_debugger();
    }

    SDL_GL_SwapWindow(win);
}

void cleanup() 
{
    if(GB_ENABLE_DEBUGGER) {
        cleanup_debugger();
    }
    SDL_DestroyWindow(win);
    SDL_Quit();
}