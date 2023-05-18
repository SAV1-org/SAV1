#ifndef DECODE_OPUS_H
#define DECODE_OPUS_H

#include <opus/opus.h>
#include <opus/opus_types.h>
#include <opus/opus_defines.h>

#include "sav1_settings.h"

typedef struct Sav1InternalContext Sav1InternalContext;

typedef struct DecodeOpusContext {
    Sav1ThreadQueue *input_queue;
    Sav1ThreadQueue *output_queue;
    thread_atomic_int_t do_decode;
    OpusDecoder *decoder;
    opus_int16 *decode_buffer;
    Sav1AudioFrequency frequency;
    Sav1AudioChannel channels;
    Sav1InternalContext *ctx;
    thread_mutex_t *running;
} DecodeOpusContext;

void
decode_opus_init(DecodeOpusContext **context, Sav1InternalContext *ctx,
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
