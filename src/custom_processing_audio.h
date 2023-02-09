#ifndef CUSTOM_PROCESSING_AUDIO_H
#define CUSTOM_PROCESSING_AUDIO_H

#include "sav1_audio_frame.h"

typedef struct Sav1InternalContext Sav1InternalContext;

typedef struct CustomProcessingAudioContext {
    Sav1ThreadQueue *input_queue;
    Sav1ThreadQueue *output_queue;
    thread_atomic_int_t do_process;
    void *(*process_function)(Sav1AudioFrame *, void *);
    void (*destroy_function)(void *, void *);
    void *cookie;
    Sav1InternalContext *ctx;
} CustomProcessingAudioContext;

void
custom_processing_audio_init(CustomProcessingAudioContext **context, Sav1InternalContext *ctx,
                             void *(*process_function)(Sav1AudioFrame *, void *),
                             void (*destroy_function)(void *, void *),
                             Sav1ThreadQueue *input_queue, Sav1ThreadQueue *output_queue);

void
custom_processing_audio_destroy(CustomProcessingAudioContext *context);

int
custom_processing_audio_start(void *context);

void
custom_processing_audio_stop(CustomProcessingAudioContext *context);

void
custom_processing_audio_drain_queue(CustomProcessingAudioContext *context);

#endif
