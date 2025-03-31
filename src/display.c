#include "../include/display.h"
#include "../include/debugger.h"
#include "../include/ppu.h"

#include <SDL/SDL_opengl.h>
#include <assert.h>

SDL_Window* win = NULL;
SDL_Texture* gb = NULL;
SDL_Renderer* renderer = NULL;
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

    init_debugger(win);
}

void render_texture()
{
    SDL_Rect destRect = {
        10, 10, 
        GB_DISPLAY_WIDTH, GB_DISPLAY_HEIGHT    
    };

    SDL_RenderCopy(renderer, gb, NULL, &destRect);
}

void render_pixel_buffer()
{
    void* pixels;
    int pitch;

    //Lock the texture to get a pointer to its pixel data
    SDL_LockTexture(gb, NULL, &pixels, &pitch);

    uint32_t* dst = (uint32_t*)pixels;
    for (int y = 0; y < 144; y++) {
        memcpy(dst, ppu.pixel_buffer[y], 160 * sizeof(uint32_t));
        dst += pitch / sizeof(uint32_t); 
    }

    //Unlocking updates our new texture changes
    SDL_UnlockTexture(gb);

    render_texture();
}

void update_display(int* quit) 
{
    debugger_start_input();
    while( SDL_PollEvent( &e ) ) { 
        debugger_poll_input(&e);
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
    debugger_end_input();

    //
    //Need to be careful with rendering order. We dont want to clear our pixel buffer display,
    //but we do want to clear the debugger
    //
    if(ppu.can_render) {
        SDL_GetWindowSize(win, &win_width, &win_height);
        glViewport(0, 0, win_width, win_height);
        clearGLColorNuklear();  //Clears whole display

        render_pixel_buffer();
        ppu.can_render = 0; 
    
        render_debugger();

        SDL_GL_SwapWindow(win);
    }
}

void cleanup() 
{
    cleanup_debugger();
    SDL_DestroyWindow(win);
    SDL_Quit();
}