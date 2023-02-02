#ifndef DECODE_OPUS_H
#define DECODE_OPUS_H

#include "sav1_settings.h"

#include <opus/opus.h>
#include <opus/opus_types.h>
#include <opus/opus_defines.h>

typedef struct DecodeOpusContext {
    Sav1ThreadQueue *input_queue;
    Sav1ThreadQueue *output_queue;
    thread_atomic_int_t do_decode;
    OpusDecoder *decoder;
    uint8_t *decode_buffer;
    Sav1Settings *settings;

} DecodeOpusContext;

void
decode_opus_init(DecodeOpusContext **context, Sav1Settings *settings,
                 Sav1ThreadQueue *input_queue, Sav1ThreadQueue *output_queue);

void
decode_opus_destroy(DecodeOpusContext *context);

int
decode_opus_start(void *context);

void
decode_opus_stop(DecodeOpusContext *context);

void
decode_opus_drain_output_queue(DecodeOpusContext *context);

#endif
