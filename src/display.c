#include "../include/display.h"
#include "../include/common.h"

#include <assert.h>
#include <SDL2/SDL.h>

SDL_Window* win = NULL;
SDL_Surface* win_surface = NULL;
SDL_Event e;

void setup_display() 
{
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        SDL_GetError();
        assert(0);
    }

    win = SDL_CreateWindow(
        "Gameboy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
        DISPLAY_WIDTH, DISPLAY_HEIGHT, SDL_WINDOW_HIDDEN);

    if(!win) {
        fprintf(stderr, "Failed to create SDL window!\n");
        assert(false);
    }
    win_surface = SDL_GetWindowSurface(win);
}

void render_pixel_buffer()
{
    SDL_FillRect(win_surface, NULL, SDL_MapRGB(win_surface->format, 0x00, 0x00, 0x00));

    //do all sorts of fun

    SDL_UpdateWindowSurface(win);  
}

void update_display(int* quit) 
{
    while( SDL_PollEvent( &e ) ) { 
        switch(e.type) {
            case SDL_QUIT: {
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

    SDL_ShowWindow(win);
    render_pixel_buffer();
}