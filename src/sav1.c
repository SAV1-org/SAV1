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

#define CHECK_CONTEXT_VALID(context) \
    if (context == NULL) {           \
        return -1;                   \
    }

#define RAISE(ctx, error)       \
    sav1_set_error(ctx, error); \
    return -1;

int
sav1_create_context(Sav1Context *context, Sav1Settings *settings)
{
    CHECK_CONTEXT_VALID(context)

    if (context->is_initialized == 1) {
        sav1_set_error(context->internal_state,
                       "Context already created: sav1_create_context() failed");
        return -1;
    }

    Sav1InternalContext *internal_context =
        (Sav1InternalContext *)malloc(sizeof(Sav1InternalContext));
    ThreadManager *manager;
    struct timespec start;

    internal_context->settings = settings;
    internal_context->thread_manager = manager;
    internal_context->critical_error_flag = 0;
    internal_context->is_playing = 0;
    internal_context->start_time = start;
    internal_context->pause_time = NULL;
    memset(internal_context->error_message, 0, sizeof(internal_context->error_message));

    thread_manager_init(&internal_context->thread_manager, internal_context->settings);
    thread_manager_start_pipeline(internal_context->thread_manager);

    context->internal_state = internal_context;
    context->is_initialized = 1;

    return 0;
}

int
sav1_destroy_context(Sav1Context *context)
{
    CHECK_CONTEXT_VALID(context)
    
    if (CHECK_CTX_INITIALIZED(context->internal_state, context) <
        0) {  // CHECK_CTX_INITIALIZED doesn't make sense right now
        return -1;
    }

    free(context->internal_state->settings);
    free(context->internal_state->thread_manager);
    free(context->internal_state->start_time);
    if (context->internal_state->pause_time != NULL) {
        free(context->internal_state->pause_time);
    }

    free(context->internal_state);
    context->is_initialized = 0;

    return 0;
}

char *
sav1_get_error(Sav1Context *context)
{
    if (context == NULL) {
        return "Sav1Context context passed as NULL";
    }
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;

    if (ctx == NULL) {
        return "Uninitialized context: sav1_create_context() not called";
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

    if (ctx->settings->codec_target && SAV1_CODEC_AV1 != 0) {
        RAISE(ctx, "Can't get video when not targeting video in settings")
    }

    return ctx->curr_video_frame;
}

int
sav1_get_audio_frame(Sav1Context *context, Sav1AudioFrame **frame)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    if (ctx->settings->codec_target && SAV1_CODEC_OPUS != 0) {
        RAISE(ctx, "Can't get audio when not targeting audio in settings")
    }

    return ctx->curr_audio_frame;
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
