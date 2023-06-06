#include "sav1.h"
#include "sav1_internal.h"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define COLOR_BLUE 103, 155, 203
#define COLOR_DARK_BLUE 61, 119, 173
#define COLOR_GRAY 30, 30, 30
#define COLOR_BLACK 0, 0, 0
#define COLOR_WHITE 220, 220, 220

#define QUEUE_WIDTH 160
#define DEBUG_PADDING 20
#define DEBUG_HEIGHT 150
#define FONT_SIZE 16

#define USE_AUDIO 1

TTF_Font *font;

void
draw_queue(int x, int y, int cell_size, int num_items, char *title, SDL_Surface *screen,
           SDL_Rect debug_rect)
{
    SDL_Rect background_rect = {x + debug_rect.x, y + 24 + debug_rect.y, QUEUE_WIDTH,
                                cell_size};
    SDL_FillRect(screen, &background_rect, SDL_MapRGB(screen->format, COLOR_WHITE));

    for (int i = 0; i < num_items; i++) {
        SDL_Rect cell_rect = {x + i * cell_size + 1 + debug_rect.x, y + 25 + debug_rect.y,
                              cell_size - 2, cell_size - 2};
        SDL_FillRect(screen, &cell_rect, SDL_MapRGB(screen->format, COLOR_DARK_BLUE));
    }

    SDL_Color font_color = {COLOR_WHITE, 0};
    SDL_Surface *text = TTF_RenderText_Blended(font, title, font_color);
    SDL_Rect font_rect = {x + debug_rect.x + (QUEUE_WIDTH - text->w) / 2,
                          y + debug_rect.y, text->w, text->h};
    SDL_BlitSurface(text, NULL, screen, &font_rect);
    SDL_free(text);
}

void
draw_debug(Sav1Context *context, SDL_Rect screen_rect, SDL_Surface *screen,
           int video_ready, int audio_ready)
{
    int debug_width = QUEUE_WIDTH * 4 + DEBUG_PADDING * 6 + 40;
    int debug_x = (screen->w - debug_width) / 2;
    SDL_Rect debug_rect = {debug_x, screen_rect.h + 4, debug_width, DEBUG_HEIGHT};
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    int cell_size = QUEUE_WIDTH / ctx->settings->queue_size;

    // video queues
    draw_queue(DEBUG_PADDING * 2 + 40, DEBUG_PADDING, cell_size,
               sav1_thread_queue_get_size(ctx->thread_manager->video_output_queue),
               "Postprocessed video", screen, debug_rect);
    draw_queue(
        DEBUG_PADDING * 3 + QUEUE_WIDTH + 40, DEBUG_PADDING, cell_size,
        sav1_thread_queue_get_size(ctx->thread_manager->video_custom_processing_queue),
        "Converted video", screen, debug_rect);
    draw_queue(DEBUG_PADDING * 4 + QUEUE_WIDTH * 2 + 40, DEBUG_PADDING, cell_size,
               sav1_thread_queue_get_size(ctx->thread_manager->video_dav1d_picture_queue),
               "Decoded video", screen, debug_rect);
    draw_queue(DEBUG_PADDING * 5 + QUEUE_WIDTH * 3 + 40, DEBUG_PADDING, cell_size,
               sav1_thread_queue_get_size(ctx->thread_manager->video_webm_frame_queue),
               "Raw video", screen, debug_rect);

    // audio queues
    draw_queue(DEBUG_PADDING * 2 + 40, DEBUG_PADDING + 50, cell_size,
               sav1_thread_queue_get_size(ctx->thread_manager->audio_output_queue),
               "Postprocessed audio", screen, debug_rect);
    draw_queue(
        DEBUG_PADDING * 3 + QUEUE_WIDTH + 40, DEBUG_PADDING + 50, cell_size,
        sav1_thread_queue_get_size(ctx->thread_manager->audio_custom_processing_queue),
        "Decoded audio", screen, debug_rect);
    draw_queue(DEBUG_PADDING * 4 + QUEUE_WIDTH * 2 + 40, DEBUG_PADDING + 50, cell_size,
               sav1_thread_queue_get_size(ctx->thread_manager->audio_webm_frame_queue),
               "Raw audio", screen, debug_rect);

    // frames ready
    SDL_Rect video_ready_background_rect = {debug_rect.x + DEBUG_PADDING + 5,
                                            debug_rect.y + DEBUG_PADDING + 14, 30, 30};
    SDL_FillRect(screen, &video_ready_background_rect,
                 SDL_MapRGB(screen->format, COLOR_WHITE));
    if (video_ready) {
        SDL_Rect video_ready_rect = {debug_rect.x + DEBUG_PADDING + 7,
                                     debug_rect.y + DEBUG_PADDING + 16, 26, 26};
        SDL_FillRect(screen, &video_ready_rect,
                     SDL_MapRGB(screen->format, COLOR_DARK_BLUE));
    }

    SDL_Rect audio_ready_background_rect = {debug_rect.x + DEBUG_PADDING + 5,
                                            debug_rect.y + DEBUG_PADDING + 63, 30, 30};
    SDL_FillRect(screen, &audio_ready_background_rect,
                 SDL_MapRGB(screen->format, COLOR_WHITE));
    if (audio_ready) {
        SDL_Rect audio_ready_rect = {debug_rect.x + DEBUG_PADDING + 7,
                                     debug_rect.y + DEBUG_PADDING + 65, 26, 26};
        SDL_FillRect(screen, &audio_ready_rect,
                     SDL_MapRGB(screen->format, COLOR_DARK_BLUE));
    }

    SDL_Color font_color = {COLOR_WHITE, 0};
    SDL_Surface *text = TTF_RenderText_Blended(font, "Ready", font_color);
    SDL_Rect font_rect = {debug_rect.x + DEBUG_PADDING + (40 - text->w) / 2,
                          debug_rect.y + 10, text->w, text->h};
    SDL_BlitSurface(text, NULL, screen, &font_rect);
    SDL_free(text);
}

int
video_postprocessing_function(Sav1VideoFrame *frame, void *cookie)
{
    return 0;
}

int
audio_postprocessing_function(Sav1AudioFrame *frame, void *cookie)
{
    return 0;
}

char *
get_file_name(char *file_path)
{
    int last_slash = 0;
    int last_dot = strlen(file_path);
    for (int i = 0; file_path[i] != '\0'; i++) {
        if (file_path[i] == '\\' || file_path[i] == '/') {
            last_slash = i;
        }
        else if (file_path[i] == '.') {
            last_dot = i;
        }
    }
    file_path[last_dot] = '\0';
    return file_path + last_slash + 1;
}

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
               SDL_Surface **surface_p, SDL_AudioDeviceID *device_p, char *window_title)
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
    *window_p = SDL_CreateWindow(window_title, SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED, screen_rect.w + 4,
                                 screen_rect.h + DEBUG_HEIGHT + 4, SDL_WINDOW_RESIZABLE);
    if (*window_p == NULL) {
        return -1;
    }
    *surface_p = SDL_GetWindowSurface(*window_p);
    if (*surface_p == NULL) {
        return -1;
    }

    TTF_Init();
    font = TTF_OpenFont("TASAOrbiterText-Regular.otf", FONT_SIZE);
    if (font == NULL) {
        printf("Failed to load font: %s\n", SDL_GetError());
        exit(1);
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

    TTF_CloseFont(font);
    TTF_Quit();

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
    int screen_width = 820;
    int screen_height = 630;
    int is_fullscreen = 0, is_paused = 0;
    int mouse_x, mouse_y;
    uint64_t mouse_active_timeout = 0;
    int running = 1;

    int frame_width, frame_height;
    int video_frame_ready = 0, audio_frame_ready = 0;
    uint64_t duration = 0;

    SDL_Window *window = NULL;
    SDL_Surface *screen = NULL, *frame = NULL;
    SDL_AudioDeviceID audio_device;
    SDL_Rect screen_rect = {2, 2, screen_width - 4, screen_height - DEBUG_HEIGHT - 4};
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
    sav1_settings_use_custom_video_processing(&settings, &video_postprocessing_function,
                                              NULL, NULL);
    sav1_settings_use_custom_audio_processing(&settings, &audio_postprocessing_function,
                                              NULL, NULL);
    if (!USE_AUDIO) {
        settings.codec_target = SAV1_CODEC_AV1;
    }

    // Setup SAV1 context
    if (sav1_create_context(&context, &settings) < 0) {
        EXIT_W_SDL_ERROR
    }

    // Setup SDL
    if (initialize_sdl(settings, screen_rect, &window, &screen, &audio_device,
                       get_file_name(argv[1])) < 0) {
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
        if (SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, COLOR_BLACK)) < 0) {
            EXIT_W_SDL_ERROR
        }

        // Video frame
        if (sav1_get_video_frame_ready(&context, &video_frame_ready) < 0) {
            EXIT_W_SAV1_ERROR
        }
        if (video_frame_ready) {
            Sav1VideoFrame *sav1_frame;
            if (sav1_get_video_frame(&context, &sav1_frame) < 0) {
                EXIT_W_SAV1_ERROR
            }
            frame_width = sav1_frame->width;
            frame_height = sav1_frame->height;

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
        if (USE_AUDIO) {
            if (sav1_get_audio_frame_ready(&context, &audio_frame_ready) < 0) {
                EXIT_W_SAV1_ERROR
            }
            if (audio_frame_ready) {
                Sav1AudioFrame *sav1_frame;
                if (sav1_get_audio_frame(&context, &sav1_frame) < 0) {
                    EXIT_W_SAV1_ERROR
                }

                // Add audio data to SDL queue
                if (SDL_QueueAudio(audio_device, sav1_frame->data, sav1_frame->size) <
                    0) {
                    EXIT_W_SDL_ERROR
                }
            }
        }

        // Display the current frame surface using SDL
        if (frame) {
            frame_rect.x = 0;
            frame_rect.y = 0;
            frame_rect.w = frame_width;
            frame_rect.h = frame_height;
            rect_fit(&frame_rect, screen_rect);
        }

        // outline video
        SDL_Rect video_outline_rect = {frame_rect.x - 2, frame_rect.y - 2,
                                       frame_rect.w + 4, frame_rect.h + 4};
        SDL_FillRect(screen, &video_outline_rect, SDL_MapRGB(screen->format, COLOR_BLUE));

        if (frame) {
            if (SDL_BlitScaled(frame, NULL, screen, &frame_rect) < 0) {
                EXIT_W_SDL_ERROR
            }
        }
        else {
            SDL_FillRect(screen, &frame_rect, SDL_MapRGB(screen->format, COLOR_BLACK));
        }

        // Video progress bar
        int padding = frame_rect.w * 0.04;
        if (duration) {
            int mouse_y;
            SDL_GetMouseState(NULL, &mouse_y);

            // Display the progress bar if video is paused or if the mouse moved
            // recently
            if (is_paused || mouse_active_timeout) {
                // Calculate percentage into the video
                uint64_t playback_time;
                sav1_get_playback_time(&context, &playback_time);
                double progress = playback_time * 1.0 / duration;
                if (progress > 1.0) {
                    progress = 1.0;
                }

                // Draw the progress bar using SDL_Rects
                SDL_Rect duration_outline_rect = {frame_rect.x + padding - 1,
                                                  frame_rect.h - padding - 1,
                                                  frame_rect.w - 2 * padding + 2, 6};
                SDL_FillRect(screen, &duration_outline_rect,
                             SDL_MapRGB(screen->format, COLOR_GRAY));
                SDL_Rect duration_rect = {frame_rect.x + padding, frame_rect.h - padding,
                                          frame_rect.w - 2 * padding, 4};
                SDL_FillRect(screen, &duration_rect,
                             SDL_MapRGB(screen->format, COLOR_WHITE));
                SDL_Rect progress_rect = {frame_rect.x + padding, frame_rect.h - padding,
                                          (int)((frame_rect.w - 2 * padding) * progress),
                                          4};
                SDL_FillRect(screen, &progress_rect,
                             SDL_MapRGB(screen->format, COLOR_BLUE));
            }
        }
        else {
            // Duration data is not required in .webm files, so this may not be
            // present
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
                        screen_rect.w = screen_width = event.window.data1 - 4;
                        screen_rect.h = screen_height =
                            event.window.data2 - DEBUG_HEIGHT - 4;

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
                    if (mouse_x >= frame_rect.x &&
                        mouse_x <= frame_rect.x + frame_rect.w) {
                        // If clicking anywhere above the progress bar, toggle pause
                        if (mouse_y < frame_rect.h - 2 * padding) {
                            if (is_paused && sav1_start_playback(&context) < 0) {
                                PRINT_SAV1_ERROR
                            }
                            else if (!is_paused && sav1_stop_playback(&context) < 0) {
                                PRINT_SAV1_ERROR
                            }
                            is_paused = is_paused ? 0 : 1;
                        }
                        else if (duration && mouse_y < frame_rect.h) {
                            // Clicking on the progress bar, so seek to that point
                            double seek_progress = (mouse_x - padding - frame_rect.x) *
                                                   1.0 / (frame_rect.w - 2 * padding);
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

        // Draw debug information
        draw_debug(&context, screen_rect, screen, video_frame_ready, audio_frame_ready);

        if (SDL_UpdateWindowSurface(window) < 0) {
            EXIT_W_SDL_ERROR
        }
    }

    // Clean up resources
    sav1_destroy_context(&context);
    cleanup_sdl(window, screen, frame, audio_device);

    return 0;
}
