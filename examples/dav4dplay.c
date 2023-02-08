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

    Sav1Settings settings;
    sav1_default_settings(&settings, argv[1]);
    settings.desired_pixel_format = SAV1_PIXEL_FORMAT_BGRA;
    settings.channels = SAV1_AUDIO_STEREO;

    Sav1Context context;
    sav1_create_context(&context, &settings);

    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_AudioSpec desired = {0};
    desired.freq = settings.frequency;
    desired.format = AUDIO_S16SYS;
    desired.channels = settings.channels;
    desired.callback = NULL;

    SDL_AudioSpec obtained = {0};
    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);

    SDL_Window *window = SDL_CreateWindow("Dav4d video player", SDL_WINDOWPOS_UNDEFINED,
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

    sav1_destroy_context(&context);

    SDL_Quit();
}
