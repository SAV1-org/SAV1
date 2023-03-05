#include "sav1_audio_frame.h"
#include "sav1_internal.h"

#include <string.h>

int
sav1_audio_frame_destroy(Sav1Context *context, Sav1AudioFrame *frame)
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
        sav1_set_error(ctx, "frame cannot be NULL in sav1_audio_frame_destroy()");
        return -1;
    }

    // free the custom audio frame data if applicable
    if (ctx->settings->use_custom_processing & SAV1_USE_CUSTOM_PROCESSING_AUDIO &&
        ctx->settings->custom_audio_frame_destroy != NULL && frame->custom_data != NULL) {
        ctx->settings->custom_audio_frame_destroy(
            frame->custom_data, ctx->settings->custom_audio_frame_processing_cookie);
    }

    // free the data
    free(frame->data);
    free(frame);

    return 0;
}

int
sav1_audio_frame_clone(Sav1Context *context, Sav1AudioFrame *src_frame,
                       Sav1AudioFrame **dst_frame)
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
        sav1_set_error(ctx, "src_frame cannot be NULL in sav1_audio_frame_clone()");
        return -1;
    }

    // malloc the destination frame
    Sav1AudioFrame *frame;
    if ((frame = (Sav1AudioFrame *)malloc(sizeof(Sav1AudioFrame))) == NULL) {
        sav1_set_critical_error_flag(ctx);
        sav1_set_error(ctx, "malloc() failed in sav1_audio_frame_clone()");
        return -1;
    }

    // copy over everything but the pixel data
    memcpy(frame, src_frame, sizeof(Sav1AudioFrame));

    // malloc the data buffer
    if ((frame->data = (uint8_t *)malloc(src_frame->size * sizeof(uint8_t))) == NULL) {
        free(frame);
        sav1_set_critical_error_flag(ctx);
        sav1_set_error(ctx, "malloc() failed in sav1_audio_frame_clone()");
        return -1;
    }

    // copy over the audio data
    memcpy(frame->data, src_frame->data, src_frame->size);

    *dst_frame = frame;

    return 0;
}
