#include "../include/display.h"
#include "../include/common.h"
#include "../include/debugger.h"

#include "SDL/SDL_opengl.h"
#include <assert.h>

SDL_Window* win = NULL;
SDL_Surface* win_surface = NULL;
SDL_Event e;
int win_height, win_width;

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
        DISPLAY_WIDTH, DISPLAY_HEIGHT, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN|SDL_WINDOW_ALLOW_HIGHDPI);

    if(!win) {
        fprintf(stderr, "Failed to create SDL window!\n");
        assert(false);
    }
    SDL_GetWindowSize(win, &win_width, &win_height);

    win_surface = SDL_GetWindowSurface(win);

    init_debugger(win);
}

void render_pixel_buffer()
{
    SDL_FillRect(win_surface, NULL, SDL_MapRGB(win_surface->format, 0x00, 0x00, 0x00));

    //do all sorts of fun

    //we will need to use somethign other than surfacews here since we are using opengl bindings now
    //SDL_UpdateWindowSurface(win);  
}

void update_display(int* quit) 
{
    while( SDL_PollEvent( &e ) ) { 
        switch(e.type) {
            case SDL_QUIT: {
                cleanup();
                *quit = 1;
            }
            break;
            case SDL_KEYDOWN: { 

            }
            break;

            case SDL_KEYUP: {

            }
            break;

            default: break;
        }
    }

    SDL_GetWindowSize(win, &win_width, &win_height);
    glViewport(0, 0, win_width, win_height);

    glClear(GL_COLOR_BUFFER_BIT);
    clearGLColorNuklear() ;

    render_debugger();

    SDL_GL_SwapWindow(win);

    render_pixel_buffer();
}

void cleanup() 
{
    cleanup_debugger();
    SDL_DestroyWindow(win);
    SDL_Quit();
}