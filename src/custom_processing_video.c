#include <stdlib.h>

#include "thread_queue.h"
#include "custom_processing_video.h"
#include "sav1_internal.h"

void
custom_processing_video_init(CustomProcessingVideoContext **context,
                             Sav1InternalContext *ctx,
                             int (*process_function)(Sav1VideoFrame *, void *),
                             Sav1ThreadQueue *input_queue, Sav1ThreadQueue *output_queue)
{
    if (((*context) = (CustomProcessingVideoContext *)malloc(
             sizeof(CustomProcessingVideoContext))) == NULL) {
        sav1_set_error(ctx, "malloc() failed in custom_processing_video_init()");
        sav1_set_critical_error_flag(ctx);
        return;
    }

    (*context)->process_function = process_function;
    (*context)->cookie = ctx->settings->custom_video_frame_processing_cookie;
    (*context)->input_queue = input_queue;
    (*context)->output_queue = output_queue;
    (*context)->ctx = ctx;
}

void
custom_processing_video_destroy(CustomProcessingVideoContext *context)
{
    free(context);
}

int
custom_processing_video_start(void *context)
{
    CustomProcessingVideoContext *process_context =
        (CustomProcessingVideoContext *)context;
    thread_atomic_int_store(&(process_context->do_process), 1);

    while (thread_atomic_int_load(&(process_context->do_process))) {
        Sav1VideoFrame *frame =
            (Sav1VideoFrame *)sav1_thread_queue_pop(process_context->input_queue);

        if (frame == NULL) {
            sav1_thread_queue_push(process_context->output_queue, NULL);
            break;
        }

        // apply the custom processing function
        int status = process_context->process_function(frame, process_context->cookie);

        // check for error
        if (status) {
            sav1_set_error_with_code(process_context->ctx,
                                     "custom_processing_audio function returned %d",
                                     status);
            sav1_thread_queue_push(process_context->output_queue, NULL);
            return status;
        }

        sav1_thread_queue_push(process_context->output_queue, frame);
    }
    return 0;
}

void
custom_processing_video_stop(CustomProcessingVideoContext *context)
{
    thread_atomic_int_store(&(context->do_process), 0);

    // add a fake entry to the input queue if necessary
    if (sav1_thread_queue_get_size(context->input_queue) == 0) {
        sav1_thread_queue_push_timeout(context->input_queue, NULL);
    }

    // drain the output queue
    custom_processing_video_drain_output_queue(context);
}

void
custom_processing_video_drain_output_queue(CustomProcessingVideoContext *context)
{
    while (1) {
        Sav1VideoFrame *frame =
            (Sav1VideoFrame *)sav1_thread_queue_pop_timeout(context->output_queue);
        if (frame == NULL) {
            break;
        }

        sav1_video_frame_destroy(context->ctx->context, frame);
    }
}
