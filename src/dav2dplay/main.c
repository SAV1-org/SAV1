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
get_frame_display_ready(struct timespec *start_time,
                        WebMFrame *web_m_frame)
{
    // quick and dirty check to see if we should display the next frame
    struct timespec curr_time;
    clock_gettime(CLOCK_MONOTONIC, &curr_time);
    uint64_t total_elapsed_ms = ((curr_time.tv_sec - start_time->tv_sec) * 1000) +
                                ((curr_time.tv_nsec - start_time->tv_nsec) / 1000000);

    return total_elapsed_ms >= web_m_frame->timecode;
}

void
get_frame_bytes(int *width, int *height, int *running, uint8_t **pixel_buffer,
                ParseContext *pc, DecodeContext *dc, size_t *pixel_buffer_capacity,
                WebMFrame **web_m_frame, ptrdiff_t *pixel_buffer_stride)
{
    int status;
    if ((status = parse_get_next_video_frame(pc, web_m_frame))) {
        *running = 0;
        if (status == PARSE_END_OF_FILE) {
            printf("Successfully parsed entire file\n");
        }
        else if (status == PARSE_ERROR) {
            printf("Error parsing file\n");
        }
        else if (status == PARSE_NO_AV1_TRACK) {
            printf("The requested file does not contain an av1 track\n");
        }
        else if (status == PARSE_BUFFER_FULL) {
            printf("The audio parsing buffer is full\n");
        }
    }
    else {
        decode_frame(dc, (*web_m_frame)->data, (*web_m_frame)->size);

        *width = dc->dav1d_picture->p.w;
        *height = dc->dav1d_picture->p.h;
        *pixel_buffer_stride = 4 * *width;
        size_t pixel_buffer_size = *pixel_buffer_stride * *height;
        // resize the pixel buffer if necessary
        if (pixel_buffer_size > *pixel_buffer_capacity) {
            *pixel_buffer_capacity = pixel_buffer_size;
            free(*pixel_buffer);
            *pixel_buffer = (uint8_t *)malloc(*pixel_buffer_capacity * sizeof(uint8_t));
        }

        // convert the color space
        convert(dc->dav1d_picture, *pixel_buffer, *pixel_buffer_stride);
    }
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
    SDL_Surface *frame = NULL;

    ParseContext *pc;
    parse_init(&pc, argv[1]);
    pc->do_overwrite_buffer = 1;

    DecodeContext *dc;
    decode_init(&dc);

    size_t pixel_buffer_capacity = 4096;
    uint8_t *pixel_buffer = (uint8_t *)malloc(pixel_buffer_capacity * sizeof(uint8_t));
    int width, height;

    WebMFrame* web_m_frame;
    ptrdiff_t pixel_buffer_stride;

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    int running = 1;
    int has_frame_bytes = 0;
    SDL_Event event;

    while (running) {
        if (has_frame_bytes) {
            if (get_frame_display_ready(&start_time, web_m_frame)) {
                SDL_BlitSurface(frame, NULL, screen, NULL);
                SDL_UpdateWindowSurface(window);
                has_frame_bytes = 0;
            }
        }
        else {
            get_frame_bytes(&width, &height, &running, &pixel_buffer, pc, dc,
                            &pixel_buffer_capacity, &web_m_frame, &pixel_buffer_stride);
            SDL_FreeSurface(frame);
            frame = SDL_CreateRGBSurfaceWithFormatFrom((void *)pixel_buffer, width,
                                                       height, 32, pixel_buffer_stride,
                                                       SDL_PIXELFORMAT_BGRA32);
            has_frame_bytes = 1;
        }

        // SDL_CreateRGBSurface(0, )
        // SDL_BlitSurface(frame, NULL, screen, NULL);
        // SDL_UpdateWindowSurface(window);

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
