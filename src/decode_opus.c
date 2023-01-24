#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "parse.h"
#include "thread_queue.h"
#include "decode_opus.h"
#include "sav1_audio_frame.h"

// TODO: Should this max decode thing be in terms of bytes, uint16s, something else?
// TODO: move this into the DecodeOpusContext
#define MAX_DECODE_LEN 10000

void
decode_opus_init(DecodeOpusContext **context, Sav1ThreadQueue *input_queue,
                 Sav1ThreadQueue *output_queue)
{
    DecodeOpusContext *decode_context =
        (DecodeOpusContext *)malloc(sizeof(DecodeOpusContext));
    *context = decode_context;

    decode_context->decode_buffer = (uint8_t *)calloc(MAX_DECODE_LEN, sizeof(uint8_t));

    decode_context->input_queue = input_queue;
    decode_context->output_queue = output_queue;

    int error;
    decode_context->decoder = opus_decoder_create(48000, 2, &error);

    if (error != OPUS_OK) {
        printf("decoder failed to create\n");
        exit(0);
    }
}

void
decode_opus_destroy(DecodeOpusContext *context)
{
    opus_decoder_destroy(context->decoder);
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
                        (opus_int16 *)decode_context->decode_buffer, MAX_DECODE_LEN, 0);

        webm_frame_destroy(input_frame);

        // setup the output frame
        // TODO: calculate duration
        Sav1AudioFrame *output_frame = (Sav1AudioFrame *)malloc(sizeof(Sav1AudioFrame));
        output_frame->codec = SAV1_CODEC_OPUS;
        output_frame->timecode = input_frame->timecode;
        output_frame->sampling_frequency = input_frame->opus_sampling_frequency;
        output_frame->num_channels = input_frame->opus_num_channels;

        // copy the decoded audio data
        output_frame->size = num_samples * sizeof(uint16_t) * 2 /* multiply by two */;
        output_frame->data = (uint8_t *)malloc(output_frame->size);
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
    while (1) {
        WebMFrame *output_frame =
            (WebMFrame *)sav1_thread_queue_pop_timeout(context->output_queue);
        if (output_frame == NULL) {
            break;
        }
        webm_frame_destroy(output_frame);
    }
}
