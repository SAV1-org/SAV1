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
    Sav1InternalContext *ctx = (Sav1InternalContext *)context;

    if (ctx->playing) {
        set_error(ctx, "sav1_start_playback() called when already playing");
        return -1;
    }
}
