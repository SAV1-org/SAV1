#include "sav1.h"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

/*
 * Welcome to sav1play
 * This example shows how to create a basic video player program with SAV1 and SDL2.
 *
 * At the top of this file we have utility functions for our setup, `main` is at the
 * bottom of the file.
 */

// Scale an SDL_Rect to fill a target SDL_Rect while maintaining its aspect ratio
void
rect_fit(SDL_Rect *rect, SDL_Rect target)
{
    float x_ratio = (float)rect->w / (float)target.w;
    float y_ratio = (float)rect->h / (float)target.h;
    float max_ratio = (x_ratio > y_ratio) ? x_ratio : y_ratio;

    rect->w = (int)ceil(rect->w / max_ratio);
    rect->h = (int)ceil(rect->h / max_ratio);
    rect->x = target.x + (target.w - rect->w) / 2;
    rect->y = target.y + (target.h - rect->h) / 2;
}

// Setup an SDL window and audio device
int
initialize_sdl(Sav1Settings settings, SDL_Rect screen_rect, SDL_Window **window_p,
               SDL_Surface **surface_p, SDL_AudioDeviceID *device_p)
{
    SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    // Define audio settings
    SDL_AudioSpec desired = {0};
    desired.freq = settings.frequency;
    desired.format = AUDIO_S16SYS;
    desired.channels = settings.channels;
    desired.callback = NULL;

    // Create audio device
    SDL_AudioSpec obtained = {0};
    *device_p = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
    if (*device_p == 0) {
        return -1;
    }

    // Create window
    *window_p = SDL_CreateWindow("SAV1 Video Player Example", SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED, screen_rect.w, screen_rect.h,
                                 SDL_WINDOW_RESIZABLE);
    if (*window_p == NULL) {
        return -1;
    }
    *surface_p = SDL_GetWindowSurface(*window_p);
    if (*surface_p == NULL) {
        return -1;
    }

    return 0;
}

// Destroy resources used by SDL
void
cleanup_sdl(SDL_Window *window, SDL_Surface *screen, SDL_Surface *frame,
            SDL_AudioDeviceID audio_device)
{
    // Fine to pass potential NULL to SDL_FreeSurface()
    SDL_FreeSurface(frame);
    SDL_FreeSurface(screen);

    if (window) {
        SDL_DestroyWindow(window);
    }

    SDL_PauseAudioDevice(audio_device, 1);
    SDL_CloseAudioDevice(audio_device);

    SDL_Quit();
}

#define EXIT_W_SAV1_ERROR                                          \
    fprintf(stderr, "SAV1 error: %s\n", sav1_get_error(&context)); \
    cleanup_sdl(window, screen, frame, audio_device);              \
    sav1_destroy_context(&context);                                \
    exit(2);

#define EXIT_W_SDL_ERROR                                \
    fprintf(stderr, "SDL error: %s\n", SDL_GetError()); \
    cleanup_sdl(window, screen, frame, audio_device);   \
    sav1_destroy_context(&context);                     \
    exit(1);

#define PRINT_SAV1_ERROR \
    fprintf(stderr, "Non-fatal SAV1 error: %s\n", sav1_get_error(&context));

int
main(int argc, char *argv[])
{
    int screen_width = 1200;
    int screen_height = 760;
    int is_fullscreen = 0, is_paused = 0;
    int needs_initial_resize = 1;
    int mouse_x, mouse_y;
    uint64_t mouse_active_timeout = 0;
    int running = 1;

    int frame_width, frame_height;
    int frame_ready;
    uint64_t duration = 0;

    SDL_Window *window = NULL;
    SDL_Surface *screen = NULL, *frame = NULL;
    SDL_AudioDeviceID audio_device;
    SDL_Rect screen_rect = {0, 0, screen_width, screen_height};
    SDL_Rect frame_rect;
    SDL_Event event;

    Sav1Context context = {0};
    Sav1Settings settings;

    // Make sure a video file was provided
    if (argc < 2) {
        fprintf(stderr, "Error: No input file specified\n");
        exit(1);
    }

    // Populate settings struct with default values
    sav1_default_settings(&settings, argv[1]);

    // Setup SAV1 context
    if (sav1_create_context(&context, &settings) < 0) {
        EXIT_W_SDL_ERROR
    }

    // Setup SDL
    if (initialize_sdl(settings, screen_rect, &window, &screen, &audio_device) < 0) {
        EXIT_W_SDL_ERROR
    }

    // Start audio device
    SDL_PauseAudioDevice(audio_device, 0);

    // Start SAV1 video playback
    if (sav1_start_playback(&context) < 0) {
        EXIT_W_SAV1_ERROR
    }

    // Main loop
    while (running) {
        // Fill background color
        if (SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 103, 155, 203)) < 0) {
            EXIT_W_SDL_ERROR
        }

        // Video frame
        if (sav1_get_video_frame_ready(&context, &frame_ready) < 0) {
            EXIT_W_SAV1_ERROR
        }
        if (frame_ready) {
            Sav1VideoFrame *sav1_frame;
            if (sav1_get_video_frame(&context, &sav1_frame) < 0) {
                EXIT_W_SAV1_ERROR
            }
            frame_width = sav1_frame->width;
            frame_height = sav1_frame->height;

            // On the first frame resize the window to be 2/3 the frame resolution
            if (needs_initial_resize) {
                SDL_SetWindowSize(window, 2 * frame_width / 3, 2 * frame_height / 3);
                needs_initial_resize = 0;
            }

            // Create SDL surface from SAV1 video frame
            SDL_FreeSurface(frame);
            frame = SDL_CreateRGBSurfaceWithFormatFrom(
                (void *)sav1_frame->data, frame_width, frame_height, 32,
                sav1_frame->stride, SDL_PIXELFORMAT_RGBA32);
            if (frame == NULL) {
                EXIT_W_SDL_ERROR;
            }
        }

        // Audio frame
        if (sav1_get_audio_frame_ready(&context, &frame_ready) < 0) {
            EXIT_W_SAV1_ERROR
        }
        if (frame_ready) {
            Sav1AudioFrame *sav1_frame;
            if (sav1_get_audio_frame(&context, &sav1_frame) < 0) {
                EXIT_W_SAV1_ERROR
            }

            // Add audio data to SDL queue
            if (SDL_QueueAudio(audio_device, sav1_frame->data, sav1_frame->size) < 0) {
                EXIT_W_SDL_ERROR
            }
        }

        // Display the current frame surface using SDL
        if (frame) {
            frame_rect.x = 0;
            frame_rect.y = 0;
            frame_rect.w = frame_width;
            frame_rect.h = frame_height;
            rect_fit(&frame_rect, screen_rect);
            if (SDL_BlitScaled(frame, NULL, screen, &frame_rect) < 0) {
                EXIT_W_SDL_ERROR
            }
        }

        // Video progress bar
        int padding = screen_width * 0.04;
        if (duration) {
            int mouse_y;
            SDL_GetMouseState(NULL, &mouse_y);

            // Display the progress bar if video is paused or if the mouse moved recently
            if (is_paused || mouse_active_timeout) {
                // Calculate percentage into the video
                uint64_t playback_time;
                sav1_get_playback_time(&context, &playback_time);
                double progress = playback_time * 1.0 / duration;
                if (progress > 1.0) {
                    progress = 1.0;
                }

                // Draw the progress bar using SDL_Rects
                SDL_Rect duration_outline_rect = {padding - 1,
                                                  screen_height - padding - 1,
                                                  screen_width - 2 * padding + 2, 6};
                SDL_FillRect(screen, &duration_outline_rect,
                             SDL_MapRGB(screen->format, 30, 30, 30));
                SDL_Rect duration_rect = {padding, screen_height - padding,
                                          screen_width - 2 * padding, 4};
                SDL_FillRect(screen, &duration_rect,
                             SDL_MapRGB(screen->format, 220, 220, 220));
                SDL_Rect progress_rect = {padding, screen_height - padding,
                                          (int)((screen_width - 2 * padding) * progress),
                                          4};
                SDL_FillRect(screen, &progress_rect,
                             SDL_MapRGB(screen->format, 103, 155, 203));
            }
        }
        else {
            // Duration data is not required in .webm files, so this may not be present
            sav1_get_playback_duration(&context, &duration);
        }

        // See if the mouse movement was long enough ago to hide it
        if (mouse_active_timeout && SDL_GetTicks() >= mouse_active_timeout) {
            mouse_active_timeout = 0;
            // Hide the cursor
            SDL_ShowCursor(SDL_DISABLE);
        }

        // SDL events
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    // Exit main loop when window closed
                    running = 0;
                    break;

                case SDL_KEYDOWN:
                    /*
                     * Key controls:
                     * ESCAPE = quit, shutdown
                     * SPACE = toggle pause
                     * F = toggle fullscreen
                     * LEFT ARROW = seek back 10 seconds
                     * RIGHT ARROW = seek forward 10 seconds
                     */

                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = 0;
                    }
                    else if (event.key.keysym.sym == SDLK_SPACE) {
                        // Toggle pause state
                        if (is_paused && sav1_start_playback(&context) < 0) {
                            PRINT_SAV1_ERROR
                        }
                        else if (!is_paused && sav1_stop_playback(&context) < 0) {
                            PRINT_SAV1_ERROR
                        }
                        is_paused = is_paused ? 0 : 1;
                    }
                    else if (event.key.keysym.sym == SDLK_f) {
                        // Resize window to fullscreen
                        if (SDL_SetWindowFullscreen(
                                window,
                                is_fullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP) < 0) {
                            EXIT_W_SDL_ERROR
                        }
                        SDL_FreeSurface(screen);
                        screen = SDL_GetWindowSurface(window);
                        if (screen == NULL) {
                            EXIT_W_SDL_ERROR
                        }

                        is_fullscreen = is_fullscreen ? 0 : 1;
                    }
                    else if (event.key.keysym.sym == SDLK_LEFT) {
                        SDL_FreeSurface(frame);
                        frame = NULL;

                        uint64_t timecode;
                        if (sav1_get_playback_time(&context, &timecode) < 0) {
                            EXIT_W_SAV1_ERROR
                        }
                        timecode = timecode >= 10000 ? timecode - 10000 : 0;
                        if (sav1_seek_playback(&context, timecode, SAV1_SEEK_MODE_FAST) <
                            0) {
                            PRINT_SAV1_ERROR
                        }
                    }
                    else if (event.key.keysym.sym == SDLK_RIGHT) {
                        SDL_FreeSurface(frame);
                        frame = NULL;

                        uint64_t timecode;
                        if (sav1_get_playback_time(&context, &timecode) < 0) {
                            EXIT_W_SAV1_ERROR
                        }
                        if (sav1_seek_playback(&context, timecode + 10000,
                                               SAV1_SEEK_MODE_FAST) < 0) {
                            PRINT_SAV1_ERROR
                        }
                    }
                    break;

                case SDL_WINDOWEVENT:
                    // Window resizing events

                    if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        screen_rect.w = screen_width = event.window.data1;
                        screen_rect.h = screen_height = event.window.data2;

                        SDL_FreeSurface(screen);
                        screen = SDL_GetWindowSurface(window);
                        if (screen == NULL) {
                            EXIT_W_SDL_ERROR
                        }
                    }
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    // Mouse click events

                    SDL_GetMouseState(&mouse_x, &mouse_y);
                    // If clicking anywhere above the progress bar, toggle pause
                    if (mouse_y < screen_height - 2 * padding) {
                        if (is_paused && sav1_start_playback(&context) < 0) {
                            PRINT_SAV1_ERROR
                        }
                        else if (!is_paused && sav1_stop_playback(&context) < 0) {
                            PRINT_SAV1_ERROR
                        }
                        is_paused = is_paused ? 0 : 1;
                    }
                    else if (duration) {
                        // Clicking on the progress bar, so seek to that point
                        double seek_progress =
                            (mouse_x - padding) * 1.0 / (screen_width - 2 * padding);
                        // Limit the progress to between 0% and 100%
                        seek_progress = seek_progress < 0   ? 0.0
                                        : seek_progress > 1 ? 1.0
                                                            : seek_progress;
                        SDL_FreeSurface(frame);
                        frame = NULL;

                        if (sav1_seek_playback(&context, duration * seek_progress,
                                               SAV1_SEEK_MODE_FAST) < 0) {
                            PRINT_SAV1_ERROR
                        }
                    }
                    break;

                case SDL_MOUSEMOTION:
                    // Mouse move event

                    // Keep cursor and progress bar visible for 4 seconds
                    mouse_active_timeout = SDL_GetTicks() + 4000;
                    // Display the cursor
                    SDL_ShowCursor(SDL_ENABLE);
                    break;

                default:
                    break;
            }
        }

        if (SDL_UpdateWindowSurface(window) < 0) {
            EXIT_W_SDL_ERROR
        }
    }

    // Clean up resources
    sav1_destroy_context(&context);
    cleanup_sdl(window, screen, frame, audio_device);

    return 0;
}
