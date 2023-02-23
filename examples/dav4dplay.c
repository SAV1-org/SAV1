#include "sav1.h"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

void
rect_fit(SDL_Rect *arg, SDL_Rect target)
{
    int w, h, x, y;
    float xratio, yratio, maxratio;

    xratio = (float)arg->w / (float)target.w;
    yratio = (float)arg->h / (float)target.h;
    maxratio = (xratio > yratio) ? xratio : yratio;

    w = (int)ceil(arg->w / maxratio);
    h = (int)ceil(arg->h / maxratio);
    x = target.x + (target.w - w) / 2;
    y = target.y + (target.h - h) / 2;

    arg->w = w;
    arg->h = h;
    arg->x = x;
    arg->y = y;
}

char *
get_file_name(char *file_path)
{
    int last_slash = -1;
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
    int is_paused = 0;
    int is_fullscreen = 0;
    int is_mouse_active = 0;
    uint64_t mouse_last_active = 0;

    Sav1Settings settings;
    sav1_default_settings(&settings, argv[1]);
    settings.desired_pixel_format = SAV1_PIXEL_FORMAT_BGRA;

    Sav1Context context = {0};
    sav1_create_context(&context, &settings);

    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_AudioSpec desired = {0};
    desired.freq = settings.frequency;
    desired.format = AUDIO_S16SYS;
    desired.channels = settings.channels;
    desired.callback = NULL;

    SDL_AudioSpec obtained = {0};
    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);

    SDL_Window *window = SDL_CreateWindow(get_file_name(argv[1]), SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED, screen_width,
                                          screen_height, SDL_WINDOW_RESIZABLE);
    SDL_Surface *screen = SDL_GetWindowSurface(window);
    SDL_Rect screen_rect = {0, 0, screen_width, screen_height};

    SDL_Surface *frame = NULL;
    SDL_Rect frame_rect;

    /* start audio device */
    SDL_PauseAudioDevice(audio_device, 0);

    sav1_start_playback(&context);
    int running = 1;
    int needs_initial_resize = 1;
    uint64_t duration = 0;
    SDL_Event event;

    while (running) {
        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 103, 155, 203));

        int frame_ready;

        // video frame
        sav1_get_video_frame_ready(&context, &frame_ready);
        if (frame_ready) {
            Sav1VideoFrame *sav1_frame;
            sav1_get_video_frame(&context, &sav1_frame);
            frame_width = sav1_frame->width;
            frame_height = sav1_frame->height;
            if (needs_initial_resize) {
                SDL_SetWindowSize(window, 2 * frame_width / 3, 2 * frame_height / 3);
                needs_initial_resize = 0;
            }
            SDL_FreeSurface(frame);
            frame = SDL_CreateRGBSurfaceWithFormatFrom(
                (void *)sav1_frame->data, frame_width, frame_height, 32,
                sav1_frame->stride, SDL_PIXELFORMAT_BGRA32);
        }

        // audio frame
        sav1_get_audio_frame_ready(&context, &frame_ready);
        if (frame_ready) {
            Sav1AudioFrame *sav1_frame;
            sav1_get_audio_frame(&context, &sav1_frame);
            // SDL_ClearQueuedAudio(audio_device);
            //              OR...
            // if (SDL_GetQueuedAudioSize(audio_device) == 0) {
            //     SDL_QueueAudio(audio_device, sav1_frame->data, sav1_frame->size);
            // }
            SDL_QueueAudio(audio_device, sav1_frame->data, sav1_frame->size);
        }

        // SDL stuff
        if (frame) {
            frame_rect.x = 0;
            frame_rect.y = 0;
            frame_rect.w = frame_width;
            frame_rect.h = frame_height;
            rect_fit(&frame_rect, screen_rect);
            SDL_BlitScaled(frame, NULL, screen, &frame_rect);
        }

        // video progress bar
        int padding = screen_width * 0.04;
        if (duration) {
            int mouse_y;
            SDL_GetMouseState(NULL, &mouse_y);
            if (is_paused || is_mouse_active) {
                uint64_t playback_time;
                sav1_get_playback_time(&context, &playback_time);
                double progress = playback_time * 1.0 / duration;
                if (progress > 1) {
                    progress = 1;
                }

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
            sav1_get_playback_duration(&context, &duration);
        }

        SDL_UpdateWindowSurface(window);

        if (is_mouse_active && (SDL_GetTicks64() - mouse_last_active) > 4000) {
            is_mouse_active = 0;
            SDL_ShowCursor(SDL_DISABLE);
            mouse_last_active = SDL_GetTicks64();
        }

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = 0;
                    break;

                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = 0;
                    }
                    else if (event.key.keysym.sym == SDLK_SPACE) {
                        if (is_paused) {
                            if (sav1_start_playback(&context) == 0) {
                                is_paused = 0;
                            }
                        }
                        else if (sav1_stop_playback(&context) == 0) {
                            is_paused = 1;
                        }
                    }
                    else if (event.key.keysym.sym == SDLK_f) {
                        SDL_SetWindowFullscreen(
                            window, is_fullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                        SDL_FreeSurface(screen);
                        screen = SDL_GetWindowSurface(window);

                        is_fullscreen = is_fullscreen ? 0 : 1;
                    }
                    else if (event.key.keysym.sym == SDLK_LEFT) {
                        // jump back 10 seconds
                        if (frame) {
                            SDL_FreeSurface(frame);
                            frame = NULL;
                        }
                        uint64_t timecode;
                        sav1_get_playback_time(&context, &timecode);
                        timecode = timecode >= 10000 ? timecode - 10000 : 0;
                        sav1_seek_playback(&context, timecode);
                    }
                    else if (event.key.keysym.sym == SDLK_RIGHT) {
                        // jump forward 10 seconds
                        if (frame) {
                            SDL_FreeSurface(frame);
                            frame = NULL;
                        }
                        uint64_t timecode;
                        sav1_get_playback_time(&context, &timecode);
                        sav1_seek_playback(&context, timecode + 10000);
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
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    // seek to mouse click
                    int mouse_x, mouse_y;
                    SDL_GetMouseState(&mouse_x, &mouse_y);
                    if (mouse_y < screen_height - 2 * padding) {
                        if (is_paused) {
                            if (sav1_start_playback(&context) == 0) {
                                is_paused = 0;
                            }
                        }
                        else if (sav1_stop_playback(&context) == 0) {
                            is_paused = 1;
                        }
                    }
                    else if (duration) {
                        double seek_progress =
                            (mouse_x - padding) * 1.0 / (screen_width - 2 * padding);
                        seek_progress = seek_progress < 0   ? 0.0
                                        : seek_progress > 1 ? 1.0
                                                            : seek_progress;
                        if (frame) {
                            SDL_FreeSurface(frame);
                            frame = NULL;
                        }
                        sav1_seek_playback(&context, duration * seek_progress);
                    }
                    break;

                case SDL_MOUSEMOTION:
                    mouse_last_active = SDL_GetTicks64();
                    is_mouse_active = 1;
                    SDL_ShowCursor(SDL_ENABLE);
                    break;

                default:
                    break;
            }
        }
    }

    sav1_destroy_context(&context);

    SDL_DestroyWindow(window);
    SDL_Quit();
}
