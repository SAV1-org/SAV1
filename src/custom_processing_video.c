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
    if (((*context)->running = (thread_mutex_t *)malloc(sizeof(thread_mutex_t))) ==
        NULL) {
        free(*context);
        sav1_set_error(ctx, "malloc() failed in custom_processing_video_init()");
        sav1_set_critical_error_flag(ctx);
        return;
    }
    thread_mutex_init((*context)->running);

    (*context)->process_function = process_function;
    (*context)->cookie = ctx->settings->custom_video_frame_processing_cookie;
    (*context)->input_queue = input_queue;
    (*context)->output_queue = output_queue;
    (*context)->ctx = ctx;
}

void
custom_processing_video_destroy(CustomProcessingVideoContext *context)
{
    thread_mutex_term(context->running);
    free(context->running);
    free(context);
}

int
custom_processing_video_start(void *context)
{
    CustomProcessingVideoContext *process_context =
        (CustomProcessingVideoContext *)context;
    thread_atomic_int_store(&(process_context->do_process), 1);
    thread_mutex_lock(process_context->running);

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
                                     "custom_processing_video function returned %d",
                                     status);
            sav1_thread_queue_push(process_context->output_queue, NULL);
            thread_mutex_unlock(process_context->running);
            return status;
        }

        sav1_thread_queue_push(process_context->output_queue, frame);
    }
    thread_mutex_unlock(process_context->running);

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

    // pop an entry from the output queue so it doesn't hang on a push
    Sav1VideoFrame *frame =
        (Sav1VideoFrame *)sav1_thread_queue_pop_timeout(context->output_queue);
    if (frame != NULL && frame->sav1_has_ownership) {
        sav1_video_frame_destroy(context->ctx->context, frame);
    }

    // wait for the decoding to officially stop
    thread_mutex_lock(context->running);
    thread_mutex_unlock(context->running);

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

        if (frame->sav1_has_ownership) {
            sav1_video_frame_destroy(context->ctx->context, frame);
        }
    }
}
