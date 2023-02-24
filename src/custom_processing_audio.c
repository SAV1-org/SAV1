#include <stdlib.h>

#include "thread_queue.h"
#include "custom_processing_audio.h"
#include "sav1_internal.h"

void
custom_processing_audio_init(CustomProcessingAudioContext **context,
                             Sav1InternalContext *ctx,
                             void *(*process_function)(Sav1AudioFrame *, void *),
                             void (*destroy_function)(void *, void *),
                             Sav1ThreadQueue *input_queue, Sav1ThreadQueue *output_queue)
{
    if (((*context) = (CustomProcessingAudioContext *)malloc(sizeof(CustomProcessingAudioContext))) == NULL) {
        sav1_set_error(ctx, "malloc() failed in custom_processing_audio_init()");
        sav1_set_critical_error_flag(ctx);
    }

    (*context)->process_function = process_function;
    (*context)->destroy_function = destroy_function;
    (*context)->cookie = ctx->settings->custom_audio_frame_processing_cookie;
    (*context)->input_queue = input_queue;
    (*context)->output_queue = output_queue;
    (*context)->ctx = ctx;
}

void
custom_processing_audio_destroy(CustomProcessingAudioContext *context)
{
    free(context);
}

int
custom_processing_audio_start(void *context)
{
    CustomProcessingAudioContext *process_context =
        (CustomProcessingAudioContext *)context;
    thread_atomic_int_store(&(process_context->do_process), 1);

    while (thread_atomic_int_load(&(process_context->do_process))) {
        Sav1AudioFrame *input_frame =
            (Sav1AudioFrame *)sav1_thread_queue_pop(process_context->input_queue);

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
custom_processing_audio_stop(CustomProcessingAudioContext *context)
{
    thread_atomic_int_store(&(context->do_process), 0);

    // add a fake entry to the input queue if necessary
    if (sav1_thread_queue_get_size(context->input_queue) == 0) {
        sav1_thread_queue_push_timeout(context->input_queue, NULL);
    }

    // drain the output queue
    custom_processing_audio_drain_output_queue(context);
}

void
custom_processing_audio_drain_output_queue(CustomProcessingAudioContext *context)
{
    while (1) {
        Sav1AudioFrame *frame =
            (Sav1AudioFrame *)sav1_thread_queue_pop_timeout(context->output_queue);
        if (frame == NULL) {
            break;
        }

        context->destroy_function(frame, context->cookie);
    }
}
