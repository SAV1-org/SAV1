#include <stdio.h>

#include <dav1d/dav1d.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

// gcc main.c -I../../dependencies/dav1d/include -I../../dependencies/SDL2/include -L../../. -lSDL2

void
switch_window_surface(SDL_Window *window, SDL_Surface **existing)
{
    SDL_FreeSurface(*existing);
    SDL_Surface* new = SDL_GetWindowSurface(window);
    *existing = new;
}

int
main()
{
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window = SDL_CreateWindow("!!AV1 != !!False", SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED, 500, 500, 0);
    SDL_Surface *screen = SDL_GetWindowSurface(window);

    int running = 1;
    SDL_Event event;

    while (running) {
        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 124, 58, 255));

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

    SDL_Quit();
}
