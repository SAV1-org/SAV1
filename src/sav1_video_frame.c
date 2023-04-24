#include "sav1_video_frame.h"
#include "sav1_internal.h"

#include <string.h>

int
sav1_video_frame_destroy(Sav1Context *context, Sav1VideoFrame *frame)
{
    // pull out internal context
    if (context == NULL || context->is_initialized != 1) {
        return -1;
    }
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    if (ctx == NULL || ctx->critical_error_flag) {
        return -1;
    }

    // make sure the frame isn't null
    if (frame == NULL) {
        sav1_set_error(ctx, "frame cannot be NULL in sav1_video_frame_destroy()");
        return -1;
    }

    // free the custom video frame data if applicable
    if (ctx->settings->use_custom_processing & SAV1_USE_CUSTOM_PROCESSING_VIDEO &&
        ctx->settings->custom_video_frame_destroy != NULL && frame->custom_data != NULL) {
        ctx->settings->custom_video_frame_destroy(
            frame->custom_data, ctx->settings->custom_video_frame_processing_cookie);
    }

    // free the data
    free(frame->data);
    free(frame);

    return 0;
}

int
sav1_video_frame_take_ownership(Sav1Context *context, Sav1VideoFrame *frame)
{
    // pull out internal context
    if (context == NULL || context->is_initialized != 1) {
        return -1;
    }
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    if (ctx == NULL || ctx->critical_error_flag) {
        return -1;
    }

    // make sure the frame isn't null
    if (frame == NULL) {
        sav1_set_error(ctx, "frame cannot be NULL in sav1_video_frame_take_ownership()");
        return -1;
    }

    // make sure SAV1 controls the frame currently
    if (frame->sav1_has_ownership) {
        sav1_set_error(
            ctx, "SAV1 already doesn't own frame in sav1_video_frame_take_ownership()");
        return -1;
    }

    // give up ownership to the user
    frame->sav1_has_ownership = 0;

    return 0;
}

int
sav1_video_frame_clone(Sav1Context *context, Sav1VideoFrame *src_frame,
                       Sav1VideoFrame **dst_frame)
{
    // pull out internal context
    if (context == NULL || context->is_initialized != 1) {
        return -1;
    }
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    if (ctx == NULL || ctx->critical_error_flag) {
        return -1;
    }

    // make sure the source frame isn't null
    if (src_frame == NULL) {
        sav1_set_error(ctx, "src_frame cannot be NULL in sav1_video_frame_clone()");
        return -1;
    }

    // malloc the destination frame
    Sav1VideoFrame *frame;
    if ((frame = (Sav1VideoFrame *)malloc(sizeof(Sav1VideoFrame))) == NULL) {
        sav1_set_critical_error_flag(ctx);
        sav1_set_error(ctx, "malloc() failed in sav1_video_frame_clone()");
        return -1;
    }

    // copy over everything but the pixel data
    memcpy(frame, src_frame, sizeof(Sav1VideoFrame));

    // malloc the data buffer
    if ((frame->data = (uint8_t *)malloc(src_frame->size * sizeof(uint8_t))) == NULL) {
        free(frame);
        sav1_set_critical_error_flag(ctx);
        sav1_set_error(ctx, "malloc() failed in sav1_video_frame_clone()");
        return -1;
    }

    // copy over the pixel data
    memcpy(frame->data, src_frame->data, src_frame->size);

    *dst_frame = frame;

    return 0;
}
