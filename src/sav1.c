#include "sav1.h"
#include "sav1_internal.h"

#include "string.h"

#define CHECK_CTX_VALID(ctx)                                                            \
    if (ctx == NULL) {                                                                  \
        sav1_set_error(ctx, "Uninitialized context: sav1_create_context() not called"); \
        return -1;                                                                      \
    }

#define CHECK_CTX_INITIALIZED(ctx, context)                                             \
    if (!context->is_initialized) {                                                     \
        sav1_set_error(ctx, "Uninitialized context: sav1_create_context() not called"); \
        return -1;                                                                      \
    }

#define CHECK_CTX_CRITICAL_ERROR(ctx) \
    if (ctx->critical_error_flag) {   \
        return -1;                    \
    }

#define RAISE(ctx, error)       \
    sav1_set_error(ctx, error); \
    return -1;

int
sav1_create_context(Sav1Context *context, Sav1Settings *settings)
{
    Sav1InternalContext *internal_context =
        (Sav1InternalContext *)malloc(sizeof(Sav1InternalContext));
    ThreadManager *manager;
    internal_context->settings = settings;
    internal_context->thread_manager = manager;
    internal_context->critical_error_flag = 0;
    internal_context->is_playing = 0;
    internal_context->start_time = 0;
    internal_context->pause_time = 0;
    memset(internal_context->error_message, 0, sizeof(internal_context->error_message));

    thread_manager_init(&internal_context->thread_manager, internal_context->settings);
    thread_manager_start_pipeline(internal_context->thread_manager);

    context->internal_state = internal_context;
}

int
sav1_get_video_frame(Sav1Context *context, Sav1VideoFrame **frame)
{
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    if (ctx->settings->codec_target && SAV1_CODEC_AV1 != 0) {
        RAISE(ctx, "Can't get video when not targeting video in settings")
    }
}

int
sav1_get_audio_frame(Sav1Context *context, Sav1AudioFrame **frame)
{
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    if (ctx->settings->codec_target && SAV1_CODEC_OPUS != 0) {
        RAISE(ctx, "Can't get audio when not targeting audio in settings")
    }
}

int
sav1_get_video_frame_ready(Sav1Context *context, int *is_ready)
{
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    if (ctx->settings->codec_target && SAV1_CODEC_AV1 != 0) {
        RAISE(ctx, "Can't get video when not targeting video in settings")
    }
}

int
sav1_get_audio_frame_ready(Sav1Context *context, int *is_ready)
{
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    if (ctx->settings->codec_target && SAV1_CODEC_OPUS != 0) {
        RAISE(ctx, "Can't get audio when not targeting audio in settings")
    }
}

int
sav1_start_playback(Sav1Context *context)
{
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    // make sure the video isn't already playing
    if (ctx->is_playing) {
        RAISE(ctx, "sav1_start_playback() called when already playing")
    }

    // start playing the video
    ctx->is_playing = 1;

    int status;
    if (ctx->pause_time == NULL) {
        // video is not paused so make the beginning now
        status = clock_gettime(CLOCK_MONOTONIC, ctx->start_time);
        if (status) {
            sav1_set_error_with_code(
                ctx, "clock_gettime() in sav1_start_playback() returned %d", status);
            return -1;
        }
    }
    else {
        // video is paused so set the start relative to that
        struct timespec curr_time;
        status = clock_gettime(CLOCK_MONOTONIC, &curr_time);
        if (status) {
            sav1_set_error_with_code(
                ctx, "clock_gettime() in sav1_start_playback() returned %d", status);
            return -1;
        }
        start_time.tv_sec += curr_time.tv_sec - ctx->pause_time->tv_sec;
        start_time.tv_sec += (curr_time.tv_nsec - ctx->pause_time->tv_nsec) / 1000000000;
        start_time.tv_nsec += (curr_time.tv_nsec - ctx->pause_time->tv_nsec) % 1000000000;

        // free the pause time
        free(ctx->pause_time);
        ctx->pause_time = NULL;
    }

    return 0;
}
