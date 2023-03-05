#ifndef CUSTOM_PROCESSING_VIDEO_H
#define CUSTOM_PROCESSING_VIDEO_H

#include "sav1_video_frame.h"

typedef struct Sav1InternalContext Sav1InternalContext;

typedef struct CustomProcessingVideoContext {
    Sav1ThreadQueue *input_queue;
    Sav1ThreadQueue *output_queue;
    thread_atomic_int_t do_process;
    int (*process_function)(Sav1VideoFrame *, void *);
    void *cookie;
    Sav1InternalContext *ctx;
} CustomProcessingVideoContext;

void
custom_processing_video_init(CustomProcessingVideoContext **context,
                             Sav1InternalContext *ctx,
                             int (*process_function)(Sav1VideoFrame *, void *),
                             Sav1ThreadQueue *input_queue, Sav1ThreadQueue *output_queue);

void
custom_processing_video_destroy(CustomProcessingVideoContext *context);

int
custom_processing_video_start(void *context);

void
custom_processing_video_stop(CustomProcessingVideoContext *context);

void
custom_processing_video_drain_output_queue(CustomProcessingVideoContext *context);

#endif
