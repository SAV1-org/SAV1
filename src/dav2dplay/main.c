#include <stdio.h>

#include <dav1d/dav1d.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "parse.h"
#include "decode.h"
#include "convert.h"


void
switch_window_surface(SDL_Window *window, SDL_Surface **existing)
{
    SDL_FreeSurface(*existing);
    SDL_Surface *new_surf = SDL_GetWindowSurface(window);
    *existing = new_surf;
}

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Error: No input file specified\n");
        exit(1);
    }

    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window = SDL_CreateWindow("!!AV1 != !!False", SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED, 500, 500, 0);
    SDL_Surface *screen = SDL_GetWindowSurface(window);

    ParseContext *pc;
    parse_init(&pc, argv[1]);

    DecodeContext dc;
    decode_init(&dc);

    int running = 1;
    SDL_Event event;

    while (running) {
        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 124, 58, 255));

        int status = parse_get_next_frame(pc);
        if (status != PARSE_OK) {
            printf("PANIC!\n");
            running = 0;
        }
        //printf("size = %i\n", pc->size);
        decode_frame(&dc, pc->data, pc->size);

        SDL_UpdateWindowSurface(window);

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = 0;
                    break;

                default:
                    break;
            }
        }
    }

    parse_destroy(pc);

    decode_destroy(&dc);

    SDL_Quit();
}
