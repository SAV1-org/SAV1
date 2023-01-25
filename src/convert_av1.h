#ifndef CONVERT_AV1_H
#define CONVERT_AV1_H

#include <libyuv.h>

#include "thread_queue.h"

typedef struct ConvertAv1Context {
    Sav1ThreadQueue *input_queue;
    Sav1ThreadQueue *output_queue;
    thread_atomic_int_t do_convert;
    int desired_pixel_format;
} ConvertAv1Context;

void
convert_av1_init(ConvertAv1Context **context, int desired_pixel_format,
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
