#include "sav1.h"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

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

void
move_towards_targeted_slide(int targeted_slide, int *curr_slide, Sav1Context *context,
                            Sav1VideoFrame **sav1_frame)
{
    *sav1_frame = NULL;

    if (targeted_slide < *curr_slide) {
        // printf("seeking 0\n");
        sav1_seek_playback(context, 0, SAV1_SEEK_MODE_FAST);
        *curr_slide = -1;
    }
    // printf("seek_err=%s\n", sav1_get_error(context));

    int is_ready;

    if (targeted_slide > *curr_slide) {
        sav1_get_video_frame_ready(context, &is_ready);
        if (is_ready) {
            // printf("getting frame\n");
            sav1_get_video_frame(context, sav1_frame);
            (*curr_slide)++;
        }
    }
    // printf("targeted_err=%s\n", sav1_get_error(context));
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
    int is_fullscreen = 0;

    int targeted_slide = 0;
    int current_slide = -1;

    Sav1Settings settings;
    sav1_default_settings(&settings, argv[1]);
    settings.desired_pixel_format = SAV1_PIXEL_FORMAT_BGRA;
    settings.codec_target = SAV1_CODEC_AV1;
    settings.playback_mode = SAV1_PLAYBACK_FAST;

    Sav1Context context = {0};
    sav1_create_context(&context, &settings);

    SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    SDL_Window *window =
        SDL_CreateWindow(argv[1], SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                         screen_width, screen_height, SDL_WINDOW_RESIZABLE);
    SDL_Surface *screen = SDL_GetWindowSurface(window);
    SDL_Rect screen_rect = {0, 0, screen_width, screen_height};

    SDL_Surface *frame = NULL;
    uint8_t *frame_pixels = NULL;
    SDL_Rect frame_rect;

    sav1_start_playback(&context);
    int running = 1;
    int needs_initial_resize = 1;
    SDL_Event event;
    Sav1VideoFrame *sav1_frame;

    while (running) {
        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 20, 20, 20));

        // video frame
        // printf("current=%i, targeted=%i, error=%s\n", current_slide, targeted_slide,
        // sav1_get_error(&context));
        move_towards_targeted_slide(targeted_slide, &current_slide, &context,
                                    &sav1_frame);
        // printf("sav1_frame=%p\n", sav1_frame);
        // printf("current=%i\n", current_slide);
        if (sav1_frame && current_slide == targeted_slide) {
            frame_width = sav1_frame->width;
            frame_height = sav1_frame->height;
            if (needs_initial_resize) {
                SDL_SetWindowSize(window, 2 * frame_width / 3, 2 * frame_height / 3);
                needs_initial_resize = 0;
            }
            SDL_FreeSurface(frame);
            if (frame_pixels) {
                free(frame_pixels);
            }
            frame_pixels = (uint8_t *)malloc(sav1_frame->size);
            memcpy(frame_pixels, sav1_frame->data, sav1_frame->size);

            frame = SDL_CreateRGBSurfaceWithFormatFrom(
                (void *)frame_pixels, frame_width, frame_height, 32, sav1_frame->stride,
                SDL_PIXELFORMAT_BGRA32);
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
                    else if (event.key.keysym.sym == SDLK_f) {
                        SDL_SetWindowFullscreen(
                            window, is_fullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                        SDL_FreeSurface(screen);
                        screen = SDL_GetWindowSurface(window);

                        is_fullscreen = is_fullscreen ? 0 : 1;
                    }
                    else if (event.key.keysym.sym == SDLK_LEFT) {
                        targeted_slide--;
                        if (targeted_slide < 0) {
                            targeted_slide = 0;
                        }
                    }
                    else if (event.key.keysym.sym == SDLK_RIGHT) {
                        targeted_slide++;
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

                default:
                    break;
            }
        }
    }

    // clean up SAV1
    sav1_destroy_context(&context);

    // clean up SDL
    if (frame) {
        SDL_FreeSurface(frame);
    }
    if (frame_pixels) {
        free(frame_pixels);
    }
    if (screen) {
        SDL_FreeSurface(screen);
    }
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
