#ifndef CUSTOM_PROCESSING_AUDIO_H
#define CUSTOM_PROCESSING_AUDIO_H

#include "sav1_audio_frame.h"

typedef struct CustomProcessingAudioContext {
    Sav1ThreadQueue *input_queue;
    Sav1ThreadQueue *output_queue;
    thread_atomic_int_t do_process;
    void *(*process_function)(Sav1AudioFrame *, void *);
    void *cookie;
} CustomProcessingAudioContext;

void
custom_processing_audio_init(CustomProcessingAudioContext **context,
                             void *(*process_function)(Sav1AudioFrame *, void *),
                             void *cookie, Sav1ThreadQueue *input_queue,
                             Sav1ThreadQueue *output_queue);

void
custom_processing_audio_destroy(CustomProcessingAudioContext *context);

int
custom_processing_audio_start(void *context);

void
custom_processing_audio_stop(CustomProcessingAudioContext *context);

#endif
