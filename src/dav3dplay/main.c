#include "thread_manager.h"
#include "thread_queue.h"
#include "sav1_video_frame.h"
#include "sav1_settings.h"

#include <stdio.h>
#include <time.h>
#include <math.h>

#include <dav1d/dav1d.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

void *
postprocessing_func(Sav1VideoFrame *frame, void *cookie)
{
    for (size_t y = 0; y < frame->height; y++) {
        for (size_t x = 0; x < frame->width; x++) {
            size_t pos = y * frame->stride + x * 4;
            if (x < frame->width / 3) {
                frame->data[pos + 1] = 0;
                frame->data[pos + 2] = 0;
            }
            else if (x < 2 * frame->width / 3) {
                frame->data[pos] = 0;
                frame->data[pos + 2] = 0;
            }
            else {
                frame->data[pos] = 0;
                frame->data[pos + 1] = 0;
            }
        }
    }

    return (void *)frame;
}

void
switch_window_surface(SDL_Window *window, SDL_Surface **existing)
{
    SDL_FreeSurface(*existing);
    SDL_Surface *new_surf = SDL_GetWindowSurface(window);
    *existing = new_surf;
}

void
rect_fit(SDL_Rect *arg, SDL_Rect target)
{
    /* Stolen from pygame source: src_c/Rect.c */

    int w, h, x, y;
    float xratio, yratio, maxratio;

    xratio = (float)arg->w / (float)target.w;
    yratio = (float)arg->h / (float)target.h;
    maxratio = (xratio > yratio) ? xratio : yratio;

    // w = (int)(arg->w / maxratio);
    // h = (int)(arg->h / maxratio);
    w = (int)ceil(arg->w / maxratio);
    h = (int)ceil(arg->h / maxratio);
    x = target.x + (target.w - w) / 2;
    y = target.y + (target.h - h) / 2;

    arg->w = w;
    arg->h = h;
    arg->x = x;
    arg->y = y;
}

int
get_frame_display_ready(struct timespec *start_time, Sav1VideoFrame *frame)
{
    // quick and dirty check to see if we should display the next frame
    struct timespec curr_time;
    clock_gettime(CLOCK_MONOTONIC, &curr_time);
    uint64_t total_elapsed_ms = ((curr_time.tv_sec - start_time->tv_sec) * 1000) +
                                ((curr_time.tv_nsec - start_time->tv_nsec) / 1000000);

    return total_elapsed_ms >= frame->timecode;
}

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Error: No input file specified\n");
        exit(1);
    }

    int screen_width = 1200;
    int screen_height = 760;
    int frame_width, frame_height;

    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_AudioSpec desired = {0};
    desired.freq = 24000;
    desired.format = AUDIO_S32LSB;
    desired.channels = 1; /* Only single channel, despite source being stereo? */
    desired.samples = 4096;
    desired.callback = NULL;

    SDL_AudioSpec obtained = {0};
    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);

    SDL_Window *window = SDL_CreateWindow("Dav3d video player", SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED, screen_width,
                                          screen_height, SDL_WINDOW_RESIZABLE);
    SDL_Surface *screen = SDL_GetWindowSurface(window);
    SDL_Rect screen_rect = {0, 0, screen_width, screen_height};

    SDL_Surface *frame = NULL;
    SDL_Rect frame_rect;

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    /* start audio device */
    SDL_PauseAudioDevice(audio_device, 0);

    int running = 1;
    SDL_Event event;

    Sav1Settings settings;
    sav1_default_settings(&settings, argv[1]);
    settings.codec_target = SAV1_CODEC_TARGET_AV1 | SAV1_CODEC_TARGET_OPUS;
    settings.desired_pixel_format = SAV1_PIXEL_FORMAT_BGRA;
    // sav1_settings_use_custom_video_processing(&settings, postprocessing_func, NULL);

    ThreadManager *manager;
    Sav1VideoFrame *sav1_frame = NULL;
    thread_manager_init(&manager, &settings);
    thread_manager_start_pipeline(manager);
    uint8_t *pixel_buffer = NULL;
    uint8_t *next_pixel_buffer = NULL;

    while (running) {
        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 127, 68, 255));

        if (sav1_frame == NULL) {
            sav1_frame =
                (Sav1VideoFrame *)sav1_thread_queue_pop(manager->video_output_queue);
            if (sav1_frame == NULL) {
                running = 0;
            }
            else {
                frame_width = sav1_frame->width;
                frame_height = sav1_frame->height;
                next_pixel_buffer = sav1_frame->data;
            }
        }
        else if (get_frame_display_ready(&start_time, sav1_frame)) {
            SDL_FreeSurface(frame);
            if (pixel_buffer != NULL) {
                free(pixel_buffer);
            }
            pixel_buffer = next_pixel_buffer;
            frame = SDL_CreateRGBSurfaceWithFormatFrom(
                (void *)pixel_buffer, frame_width, frame_height, 32, sav1_frame->stride,
                SDL_PIXELFORMAT_BGRA32);
            free(sav1_frame);
            sav1_frame = NULL;
        }

        /* SDL Queue Audio does unlimited queueing, so for now we just want to get
         * everything out and queued ASAP */
        if (sav1_thread_queue_get_size(manager->audio_output_queue)) {
            WebMFrame *audio_frame =
                (WebMFrame *)sav1_thread_queue_pop(manager->audio_output_queue);
            SDL_QueueAudio(audio_device, audio_frame->data, audio_frame->size);
            webm_frame_destroy(audio_frame);
        }

        if (frame) {
            frame_rect.x = 0;
            frame_rect.y = 0;
            frame_rect.w = frame_width;
            frame_rect.h = frame_height;
            rect_fit(&frame_rect, screen_rect);
            SDL_BlitScaled(frame, NULL, screen, &frame_rect);
        }
        SDL_UpdateWindowSurface(window);

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = 0;
                    break;

                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = 0;
                    }
                    break;

                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        screen_width = event.window.data1;
                        screen_height = event.window.data2;
                        screen_rect.w = screen_width;
                        screen_rect.h = screen_height;

                        SDL_FreeSurface(screen);
                        screen = SDL_GetWindowSurface(window);
                    }

                default:
                    break;
            }
        }
    }

    thread_manager_kill_pipeline(manager);
    thread_manager_destroy(manager);
    if (pixel_buffer != NULL) {
        free(pixel_buffer);
    }

    SDL_Quit();
}
