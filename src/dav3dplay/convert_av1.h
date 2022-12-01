#ifndef CONVERT_AV1_H
#define CONVERT_AV1_H

#include <dav1d/dav1d.h>
#include <libyuv.h>

#include "thread_queue.h"

typedef struct ConvertAv1Context {
    Sav1ThreadQueue *input_queue;
    Sav1ThreadQueue *output_queue;
    thread_atomic_int_t do_convert;
} ConvertAv1Context;

void
convert_av1(Dav1dPicture *picture, uint8_t *bgra_data, ptrdiff_t bgra_stride);

void
convert_av1_init(ConvertAv1Context **context, Sav1ThreadQueue *input_queue,
                 Sav1ThreadQueue *output_queue);

void
convert_av1_destroy(ConvertAv1Context *context);

int
convert_av1_start(void *context);

void
convert_av1_stop(ConvertAv1Context *context);

#endif
