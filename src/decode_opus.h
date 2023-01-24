#ifndef DECODE_OPUS_H
#define DECODE_OPUS_H

#include <opus/opus.h>
#include <opus/opus_types.h>
#include <opus/opus_defines.h>

typedef struct DecodeOpusContext {
    Sav1ThreadQueue *input_queue;
    Sav1ThreadQueue *output_queue;
    thread_atomic_int_t do_decode;
    OpusDecoder* decoder;
    uint8_t *decode_buffer;

} DecodeOpusContext;

void
decode_opus_init(DecodeOpusContext **context, Sav1ThreadQueue *input_queue,
                Sav1ThreadQueue *output_queue);

void
decode_opus_destroy(DecodeOpusContext *context);

int
decode_opus_start(void *context);

void
decode_opus_stop(DecodeOpusContext *context);

#endif
