#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "parse.h"
#include "thread_queue.h"
#include "decode_opus.h"

// Should this max decode thing be in terms of bytes, uint16s, something else?
#define MAX_DECODE_LEN 10000
uint8_t dec_buf[MAX_DECODE_LEN] = {0};

void
decode_opus_init(DecodeOpusContext **context, Sav1ThreadQueue *input_queue,
                 Sav1ThreadQueue *output_queue)
{
    DecodeOpusContext *decode_context =
        (DecodeOpusContext *)malloc(sizeof(DecodeOpusContext));
    *context = decode_context;

    decode_context->input_queue = input_queue;
    decode_context->output_queue = output_queue;

    int error;
    decode_context->decoder = opus_decoder_create(48000, 1, &error);

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
                        (opus_int16 *)dec_buf, MAX_DECODE_LEN, 0);

        webm_frame_destroy(input_frame);

        WebMFrame *output_frame = (WebMFrame *)malloc(sizeof(WebMFrame));

        size_t data_size = num_samples * sizeof(uint16_t);
        uint8_t *output_buffer = (uint8_t *)malloc(data_size);

        memcpy(output_buffer, dec_buf, data_size);

        output_frame->data = output_buffer;
        output_frame->size = data_size;

        sav1_thread_queue_push(decode_context->output_queue, output_frame);
    }
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
