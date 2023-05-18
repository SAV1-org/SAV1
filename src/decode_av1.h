#ifndef DECODE_AV1_H
#define DECODE_AV1_H

#include <dav1d/dav1d.h>

#include "thread_queue.h"

typedef struct Sav1InternalContext Sav1InternalContext;

typedef struct DecodeAv1Context {
    Sav1ThreadQueue *input_queue;
    Sav1ThreadQueue *output_queue;
    thread_atomic_int_t do_decode;
    Dav1dContext *dav1d_context;
    Sav1InternalContext *ctx;
    thread_mutex_t *running;
} DecodeAv1Context;

void
decode_av1_init(DecodeAv1Context **context, Sav1InternalContext *ctx,
                Sav1ThreadQueue *input_queue, Sav1ThreadQueue *output_queue);

void
decode_av1_destroy(DecodeAv1Context *context);

int
decode_av1_start(void *context);

void
decode_av1_stop(DecodeAv1Context *context);

void
decode_av1_drain_output_queue(DecodeAv1Context *context);

#endif
