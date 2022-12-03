#ifndef SAV1_VIDEO_FRAME_H
#define SAV1_VIDEO_FRAME_H

#include <cstdint>

#define SAV1_CODEC_AV1 1

typedef struct Sav1VideoFrame {
    uint8_t *data;
    size_t size;
    ptrdiff_t stride;
    size_t width;
    size_t height;
    uint64_t timecode;
    uint8_t color_depth;
    int codec;
} Sav1VideoFrame;

#endif
