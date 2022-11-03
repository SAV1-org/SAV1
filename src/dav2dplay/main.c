#include <stdio.h>
#include <time.h>

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
    SDL_Surface *frame;

    ParseContext *pc;
    parse_init(&pc, argv[1]);

    DecodeContext *dc;
    decode_init(&dc);

    size_t pixel_buffer_capacity = 4096;
    uint8_t *pixel_buffer = (uint8_t *)malloc(pixel_buffer_capacity * sizeof(uint8_t));
    int width, height;

    struct timespec start_time, curr_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    int frame_is_ready = 0;
    int running = 1;
    SDL_Event event;

    while (running) {
        if (frame_is_ready) {
            // quick and dirty check to see if we should display the next frame
            clock_gettime(CLOCK_MONOTONIC, &curr_time);
            uint64_t total_elapsed_ms =
                ((curr_time.tv_sec - start_time.tv_sec) * 1000) +
                ((curr_time.tv_nsec - start_time.tv_nsec) / 1000000);

            if (total_elapsed_ms >= pc->timecode) {
                frame_is_ready = 0;
                SDL_UpdateWindowSurface(window);
                SDL_FreeSurface(frame);
            }
        }
        else {
            int status;
            if (status = parse_get_next_frame(pc)) {
                running = 0;
                if (status == PARSE_END_OF_FILE) {
                    printf("Successfully parsed entire file\n");
                }
                else if (status == PARSE_ERROR) {
                    printf("Error parsing file\n");
                }
                else if (PARSE_NO_AV1_TRACK) {
                    printf("The requested file does not contain an av1 track\n");
                }
            }
            else {
                decode_frame(dc, pc->data, pc->size);

                width = dc->dav1d_picture->p.w;
                height = dc->dav1d_picture->p.h;
                ptrdiff_t pixel_buffer_stride = 4 * width;
                size_t pixel_buffer_size = pixel_buffer_stride * height;
                // resize the pixel buffer if necessary
                if (pixel_buffer_size > pixel_buffer_capacity) {
                    pixel_buffer_capacity = pixel_buffer_size;
                    free(pixel_buffer);
                    pixel_buffer =
                        (uint8_t *)malloc(pixel_buffer_capacity * sizeof(uint8_t));
                }

                // convert the color space
                convert(dc->dav1d_picture, pixel_buffer, pixel_buffer_stride);

                frame = SDL_CreateRGBSurfaceWithFormatFrom(
                    (void *)pixel_buffer, width, height, 32, pixel_buffer_stride,
                    SDL_PIXELFORMAT_BGRA32);

                SDL_SetWindowSize(window, width, height);
                screen = SDL_GetWindowSurface(window);  // this may be unneccessary
                SDL_BlitSurface(frame, NULL, screen, NULL);

                frame_is_ready = 1;
            }
        }

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
    decode_destroy(dc);

    SDL_Quit();
}
