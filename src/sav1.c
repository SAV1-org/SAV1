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
    Sav1InternalContext *ctx = (Sav1InternalContext *)context;

    if (ctx->playing) {
        set_error(ctx, "sav1_start_playback() called when already playing");
        return -1;
    }
}
