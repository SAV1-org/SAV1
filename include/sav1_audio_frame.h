#ifndef SAV1_AUDIO_FRAME_H
#define SAV1_AUDIO_FRAME_H

#include "sav1.h"

#include <stdint.h>

typedef struct Sav1AudioFrame {
    uint8_t *data;
    size_t size;
    uint64_t timecode;
    uint64_t duration;
    double sampling_frequency;
    size_t num_channels;
    int codec;
    int sentinel;
} Sav1AudioFrame;

int
sav1_audio_frame_destroy(Sav1Context *context, Sav1AudioFrame *frame);

int
sav1_audio_frame_clone(Sav1Context *context, Sav1AudioFrame *src_frame,
                       Sav1AudioFrame **dst_frame);

#endif
