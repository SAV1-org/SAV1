#include <stdlib.h>

#include "thread_queue.h"
#include "custom_processing_video.h"
#include "sav1_internal.h"

void
custom_processing_video_init(CustomProcessingVideoContext **context,
                             Sav1InternalContext *ctx,
                             void *(*process_function)(Sav1VideoFrame *, void *),
                             void (*destroy_function)(void *, void *),
                             Sav1ThreadQueue *input_queue, Sav1ThreadQueue *output_queue)
{
    CustomProcessingVideoContext *process_context =
        (CustomProcessingVideoContext *)malloc(sizeof(CustomProcessingVideoContext));
    *context = process_context;

    process_context->process_function = process_function;
    process_context->destroy_function = destroy_function;
    process_context->cookie = ctx->settings->custom_video_frame_processing_cookie;
    process_context->input_queue = input_queue;
    process_context->output_queue = output_queue;
    process_context->ctx = ctx;
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
        Sav1VideoFrame *input_frame =
            (Sav1VideoFrame *)sav1_thread_queue_pop(process_context->input_queue);

        if (input_frame == NULL) {
            sav1_thread_queue_push(process_context->output_queue, NULL);
            break;
        }

        void *output_frame =
            process_context->process_function(input_frame, process_context->cookie);

        sav1_thread_queue_push(process_context->output_queue, output_frame);
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

        context->destroy_function(frame, context->cookie);
    }
}
