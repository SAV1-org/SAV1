#ifndef CONVERT_AV1_H
#define CONVERT_AV1_H

#include <libyuv.h>

#include "sav1_settings.h"
#include "thread_queue.h"

typedef struct Sav1InternalContext Sav1InternalContext;

typedef struct ConvertAv1Context {
    Sav1ThreadQueue *input_queue;
    Sav1ThreadQueue *output_queue;
    thread_atomic_int_t do_convert;
    Sav1InternalContext *ctx;
    Sav1PixelFormat desired_pixel_format;
} ConvertAv1Context;

void
convert_av1_init(ConvertAv1Context **context, Sav1InternalContext *ctx,
                 Sav1ThreadQueue *input_queue, Sav1ThreadQueue *output_queue);

void
convert_av1_destroy(ConvertAv1Context *context);

int
convert_av1_start(void *context);

void
convert_av1_stop(ConvertAv1Context *context);

void
convert_av1_drain_output_queue(ConvertAv1Context *context);

#endif
