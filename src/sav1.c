#include "sav1.h"
#include "sav1_internal.h"

#define CHECK_CTX_VALID(ctx)                                                            \
    if (ctx == NULL) {                                                                  \
        sav1_set_error(ctx, "Uninitialized context: sav1_create_context() not called"); \
        return -1;                                                                      \
    }

#define CHECK_CTX_INITIALIZED(ctx)                                                      \
    if (!ctx->is_initialized) {                                                         \
        sav1_set_error(ctx, "Uninitialized context: sav1_create_context() not called"); \
        return -1;                                                                      \
    }

#define CHECK_CTX_CRITICAL_ERROR(ctx) \
    if (ctx->critical_error_flag) {   \
        return -1;                    \
    }

int
sav1_get_video_frame(Sav1Context *context, Sav1VideoFrame **frame)
{
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx)
    CHECK_CTX_CRITICAL_ERROR(ctx)
}

int
sav1_start_playback(Sav1Context *context)
{
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    // make sure the video isn't already playing
    if (ctx->is_playing) {
        RAISE(ctx, "sav1_start_playback() called when already playing")
    }

    // start playing the video
    ctx->is_playing = 1;

    // set the timestamp based on whether the video is paused
    int status;
    if (ctx->pause_time == NULL) {
        status = clock_gettime(CLOCK_MONOTONIC, ctx->start_time);
        if (status) {
            RAISE(ctx, "clock_gettime() in sav1_start_playback() returned")
        }
    }
    else {
        struct timespec curr_time;
        clock_gettime(CLOCK_MONOTONIC, &curr_time);
        start_time.tv_sec += curr_time.tv_sec - ctx->pause_time->tv_sec;
        start_time.tv_sec += (curr_time.tv_nsec - ctx->pause_time->tv_nsec) / 1000000000;
        start_time.tv_nsec += (curr_time.tv_nsec - ctx->pause_time->tv_nsec) % 1000000000;

        free(ctx->pause_time);
        ctx->pause_time = NULL;
    }

    return 0;
}
