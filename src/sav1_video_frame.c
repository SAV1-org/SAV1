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

    // free the data
    free(frame->data);
    free(frame);

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

void
_sav1_pump_video_frames(Sav1Context *context, uint64_t curr_ms)
{
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;

    if (ctx->curr_video_frame == NULL) {
        if (sav1_thread_queue_get_size(ctx->thread_manager->video_output_queue) > 0) {
            ctx->curr_video_frame = (Sav1VideoFrame *)sav1_thread_queue_pop(
                ctx->thread_manager->video_output_queue);
        }
        ctx->video_frame_ready = 1;  // mark ready, there is new content
    }

    if (ctx->next_video_frame == NULL) {
        if (sav1_thread_queue_get_size(ctx->thread_manager->video_output_queue) > 0) {
            ctx->curr_video_frame = (Sav1VideoFrame *)sav1_thread_queue_pop(
                ctx->thread_manager->video_output_queue);
        }
    }

    // If next video frame is still NULL by now, we can't go into the cycler loop
    if (ctx->next_video_frame == NULL) {
        return;
    }

    // while there is a "next frame" that should be displayed
    while (ctx->next_video_frame->timecode < curr_ms) {
        // clean up current frame before replacing it
        sav1_video_frame_destroy(context, ctx->curr_video_frame);

        ctx->curr_video_frame = ctx->next_video_frame;
        ctx->video_frame_ready = 1;  // mark ready, there is new content

        // If there is another frame to pull, pull it to check whether it should be the
        // current frame
        if (sav1_thread_queue_get_size(ctx->thread_manager->video_output_queue) > 0) {
            ctx->next_video_frame = (Sav1VideoFrame *)sav1_thread_queue_pop(
                ctx->thread_manager->video_output_queue);
        }
    }
}
