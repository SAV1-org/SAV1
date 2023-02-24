#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "webm_frame.h"
#include "thread_queue.h"
#include "decode_opus.h"
#include "sav1_audio_frame.h"
#include "sav1_internal.h"

#define MAX_DECODE_LEN 11520 // ((48000Hz * 120ms) / 1000) * 2

void
decode_opus_init(DecodeOpusContext **context, Sav1InternalContext *ctx,
                 Sav1ThreadQueue *input_queue, Sav1ThreadQueue *output_queue)
{
    if ((*context = (DecodeOpusContext *)malloc(sizeof(DecodeOpusContext))) == NULL) {
        sav1_set_error(ctx, "malloc() failed in decode_opus_init()");
        sav1_set_critical_error_flag(ctx);
    }

    if (((*context)->decode_buffer = (opus_int16 *)malloc(MAX_DECODE_LEN * sizeof(opus_int16))) == NULL) {
        free(*context);
        sav1_set_error(ctx, "malloc() failed in decode_opus_init()");
        sav1_set_critical_error_flag(ctx);      
    }
    (*context)->input_queue = input_queue;
    (*context)->output_queue = output_queue;
    (*context)->frequency = ctx->settings->frequency;
    (*context)->channels = ctx->settings->channels;
    (*context)->ctx = ctx;

    int error;
    (*context)->decoder =
        opus_decoder_create((*context)->frequency, (*context)->channels, &error);

    if (error != OPUS_OK) {
        sav1_set_error(ctx, "opus decoder failed to create in decode_opus_init()");
        sav1_set_critical_error_flag(ctx);
    }
}

void
decode_opus_destroy(DecodeOpusContext *context)
{
    opus_decoder_destroy(context->decoder);
    free(context->decode_buffer);
    free(context);
}

int
decode_opus_start(void *context)
{
    DecodeOpusContext *decode_context = (DecodeOpusContext *)context;
    thread_atomic_int_store(&(decode_context->do_decode), 1);

    while (thread_atomic_int_load(&(decode_context->do_decode))) {
        // pull a webm frame from the input queue
        WebMFrame *input_frame =
            (WebMFrame *)sav1_thread_queue_pop(decode_context->input_queue);
        if (input_frame == NULL) {
            sav1_thread_queue_push(decode_context->output_queue, NULL);
            break;
        }

        int num_samples =
            opus_decode(decode_context->decoder, input_frame->data, input_frame->size,
                        decode_context->decode_buffer, MAX_DECODE_LEN, 0);

        // setup the output frame
        Sav1AudioFrame *output_frame;
        if ((output_frame = (Sav1AudioFrame *)malloc(sizeof(Sav1AudioFrame))) == NULL) {
            sav1_set_error(decode_context->ctx, "malloc() failed in decode_opus_start()");
            sav1_set_critical_error_flag(decode_context->ctx);
        }
        output_frame->codec = SAV1_CODEC_OPUS;
        output_frame->timecode = input_frame->timecode;
        output_frame->sentinel = input_frame->sentinel;
        output_frame->duration = (num_samples * 1000) / (decode_context->frequency);
        webm_frame_destroy(input_frame);

        // copy the decoded audio data
        // if the audio is stereo, each sample is twice as long, so we multiply by 2-
        // this is accomplished by using the values of SAV1_AUDIO_MONO and
        // SAV1_AUDIO_STEREO
        output_frame->size = num_samples * sizeof(uint16_t) * decode_context->channels;
        if ((output_frame->data = (uint8_t *)malloc(output_frame->size)) == NULL) {
            free(output_frame);
            sav1_set_error(decode_context->ctx, "malloc() failed in decode_opus_start()");
            sav1_set_critical_error_flag(decode_context->ctx);
        }
        memcpy(output_frame->data, decode_context->decode_buffer, output_frame->size);

        sav1_thread_queue_push(decode_context->output_queue, output_frame);
    }

    return 0;
}

void
decode_opus_stop(DecodeOpusContext *context)
{
    thread_atomic_int_store(&(context->do_decode), 0);

    // add a fake entry to the input queue if necessary
    if (sav1_thread_queue_get_size(context->input_queue) == 0) {
        sav1_thread_queue_push_timeout(context->input_queue, NULL);
    }

    // drain the output queue
    decode_opus_drain_output_queue(context);
}

void
decode_opus_drain_output_queue(DecodeOpusContext *context)
{
    while (1) {
        Sav1AudioFrame *output_frame =
            (Sav1AudioFrame *)sav1_thread_queue_pop_timeout(context->output_queue);
        if (output_frame == NULL) {
            break;
        }
        sav1_audio_frame_destroy(context->ctx->context, output_frame);
    }
}