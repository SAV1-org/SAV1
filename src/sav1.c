#include "sav1.h"
#include "sav1_internal.h"

#define CHECK_CTX_VALID(ctx, context)                                              \
    if (ctx == NULL) {                                                             \
        sav1_set_error(context,                                                    \
                       "Uninitialized context: sav1_create_context() not called"); \
        return -1;                                                                 \
    }

#define CHECK_CTX_INITIALIZED(ctx, context)                                        \
    if (!ctx->is_initialized) {                                                    \
        sav1_set_error(context,                                                    \
                       "Uninitialized context: sav1_create_context() not called"); \
        return -1;                                                                 \
    }

#define CHECK_CTX_CRITICAL_ERROR(ctx, context) \
    if (ctx->critical_error_flag) {            \
        return -1;                             \
    }

int
sav1_get_video_frame(Sav1Context *context, Sav1VideoFrame **frame)
{
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx, context)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx, context)
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
