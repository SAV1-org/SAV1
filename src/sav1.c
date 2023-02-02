#include "sav1.h"
#include "sav1_internal.h"

#include <string.h>
#include <time.h>

#define CHECK_CTX_VALID(ctx) \
    if (ctx == NULL) {       \
        return -1;           \
    }

#define CHECK_CTX_INITIALIZED(ctx, context)                                             \
    if (context->is_initialized != 1) {                                                 \
        sav1_set_error(ctx, "Uninitialized context: sav1_create_context() not called"); \
        return -1;                                                                      \
    }

#define CHECK_CTX_CRITICAL_ERROR(ctx) \
    if (ctx->critical_error_flag) {   \
        return -1;                    \
    }

#define CHECK_CONTEXT_VALID(context) \
    if (context == NULL) {           \
        return -1;                   \
    }

#define RAISE(ctx, error)       \
    sav1_set_error(ctx, error); \
    return -1;

#define RAISE_CRITICAL(ctx, error)     \
    sav1_set_critical_error_flag(ctx); \
    RAISE(ctx, error)

int
sav1_create_context(Sav1Context *context, Sav1Settings *settings)
{
    CHECK_CONTEXT_VALID(context)

    if (context->is_initialized == 1) {
        Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
        RAISE(ctx, "Context already created: sav1_create_context() failed")
    }

    // TODO: error check mallocs

    Sav1InternalContext *ctx = (Sav1InternalContext *)malloc(sizeof(Sav1InternalContext));

    ctx->settings = settings;
    ctx->critical_error_flag = 0;
    ctx->is_playing = 0;
    ctx->start_time = (struct timespec *)malloc(sizeof(struct timespec));
    ctx->pause_time = NULL;
    ctx->curr_video_frame = NULL;
    ctx->curr_audio_frame = NULL;
    memset(ctx->error_message, 0, SAV1_ERROR_MESSAGE_SIZE);

    thread_manager_init(&(ctx->thread_manager), ctx->settings);
    thread_manager_start_pipeline(ctx->thread_manager);

    context->internal_state = (void *)ctx;
    context->is_initialized = 1;

    return 0;
}

int
sav1_destroy_context(Sav1Context *context)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)

    // TODO: error check these
    thread_manager_kill_pipeline(ctx->thread_manager);
    thread_manager_destroy(ctx->thread_manager);

    free(ctx->start_time);
    if (ctx->pause_time != NULL) {
        free(ctx->pause_time);
    }

    // TODO: free sav1 video and audio frames

    free(ctx);
    context->is_initialized = 0;

    return 0;
}

char *
sav1_get_error(Sav1Context *context)
{
    if (context == NULL) {
        return (char *)"Sav1Context context passed as NULL";
    }
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;

    if (ctx == NULL) {
        return (char *)"Uninitialized context: sav1_create_context() not called";
    }
    else {
        return ctx->error_message;
    }
}

int
sav1_get_video_frame(Sav1Context *context, Sav1VideoFrame **frame)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    if (ctx->settings->codec_target & SAV1_CODEC_AV1 == 0) {
        RAISE(ctx, "Can't get video when not targeting video in settings")
    }

    // return ctx->curr_video_frame;
    return 0;
}

int
sav1_get_audio_frame(Sav1Context *context, Sav1AudioFrame **frame)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    if (ctx->settings->codec_target & SAV1_CODEC_OPUS == 0) {
        RAISE(ctx, "Can't get audio when not targeting audio in settings")
    }

    // return ctx->curr_audio_frame;
    return 0;
}

int
sav1_get_video_frame_ready(Sav1Context *context, int *is_ready)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    if (ctx->settings->codec_target & SAV1_CODEC_AV1 == 0) {
        RAISE(ctx, "Can't get video when not targeting video in settings")
    }

    return 0;
}

int
sav1_get_audio_frame_ready(Sav1Context *context, int *is_ready)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    if (ctx->settings->codec_target & SAV1_CODEC_OPUS == 0) {
        RAISE(ctx, "Can't get audio when not targeting audio in settings")
    }

    return 0;
}

int
sav1_start_playback(Sav1Context *context)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    // make sure the video isn't already playing
    if (ctx->is_playing) {
        RAISE(ctx, "sav1_start_playback() called when already playing")
    }

    // update the start time value
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
        ctx->start_time->tv_sec += curr_time.tv_sec - ctx->pause_time->tv_sec;
        ctx->start_time->tv_sec +=
            (curr_time.tv_nsec - ctx->pause_time->tv_nsec) / 1000000000;
        ctx->start_time->tv_nsec +=
            (curr_time.tv_nsec - ctx->pause_time->tv_nsec) % 1000000000;

        // free the pause time
        free(ctx->pause_time);
        ctx->pause_time = NULL;
    }

    // set the playback status
    ctx->is_playing = 1;

    return 0;
}

int
sav1_stop_playback(Sav1Context *context)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    // make sure the video is already playing
    if (!ctx->is_playing) {
        RAISE(ctx, "sav1_stop_playback() called when already stopped")
    }

    if ((ctx->pause_time = (struct timespec *)malloc(sizeof(struct timespec))) == NULL) {
        RAISE_CRITICAL(ctx, "malloc() failed in sav1_stop_playback()")
    }
    int status = clock_gettime(CLOCK_MONOTONIC, ctx->pause_time);
    if (status) {
        sav1_set_error_with_code(
            ctx, "clock_gettime() in sav1_stop_playback() returned %d", status);
        return -1;
    }

    // set the playback status
    ctx->is_playing = 0;

    return 0;
}

int
sav1_get_playback_time(Sav1Context *context, uint64_t *time_ms)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    // handle cases when video isn't playing
    if (!ctx->is_playing) {
        // check if video is paused or just hasn't been started
        if (ctx->pause_time == NULL) {
            RAISE(ctx,
                  "sav1_get_playback_time() called without calling sav1_start_playback()")
        }

        // calculate the offset from the pause time to the start time
        *time_ms = ((ctx->pause_time->tv_sec - ctx->start_time->tv_sec) * 1000) +
                   ((ctx->pause_time->tv_nsec - ctx->start_time->tv_nsec) / 1000000);
        return 0;
    }

    // get the current time
    struct timespec curr_time;
    int status = clock_gettime(CLOCK_MONOTONIC, &curr_time);
    if (status) {
        sav1_set_error_with_code(
            ctx, "clock_gettime() in sav1_get_playback_time_ms() returned %d", status);
        return -1;
    }

    // calculate the offset from the current time to the start time
    *time_ms = ((curr_time.tv_sec - ctx->start_time->tv_sec) * 1000) +
               ((curr_time.tv_nsec - ctx->start_time->tv_nsec) / 1000000);
    return 0;
}
