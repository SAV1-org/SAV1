#ifndef SAV1_AUDIO_FRAME_H
#define SAV1_AUDIO_FRAME_H

#include <cstdint>

#define SAV1_CODEC_OPUS 1

typedef struct Sav1AudioFrame {
    uint8_t *data;
    size_t size;
    uint64_t timecode;
    uint64_t duration;
    double sampling_frequency;
    size_t num_channels;
    int codec;
} Sav1AudioFrame;

#endif
