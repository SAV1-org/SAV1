#ifndef CUSTOM_PROCESSING_VIDEO_H
#define CUSTOM_PROCESSING_VIDEO_H

#include "sav1_video_frame.h"

typedef struct CustomProcessingVideoContext {
    Sav1ThreadQueue *input_queue;
    Sav1ThreadQueue *output_queue;
    thread_atomic_int_t do_process;
    void *(*process_function)(Sav1VideoFrame *, void *);
    void *cookie;
} CustomProcessingVideoContext;

void
custom_processing_video_init(CustomProcessingVideoContext **context,
                             void *(*process_function)(Sav1VideoFrame *, void *),
                             void *cookie, Sav1ThreadQueue *input_queue,
                             Sav1ThreadQueue *output_queue);

void
custom_processing_video_destroy(CustomProcessingVideoContext *context);

int
custom_processing_video_start(void *context);

void
custom_processing_video_stop(CustomProcessingVideoContext *context);

#endif
