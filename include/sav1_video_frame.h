#ifndef SAV1_VIDEO_FRAME_H
#define SAV1_VIDEO_FRAME_H

#include <cstdint>

// forward declaration to avoid circular dependency
typedef struct Sav1Context Sav1Context;

// storing this here to again avoid a circular dependency
// TODO: figure out a better solution
typedef enum {
    SAV1_PIXEL_FORMAT_RGBA = 0,
    SAV1_PIXEL_FORMAT_ARGB = 1,
    SAV1_PIXEL_FORMAT_BGRA = 2,
    SAV1_PIXEL_FORMAT_ABGR = 3,
    SAV1_PIXEL_FORMAT_RGB = 4,
    SAV1_PIXEL_FORMAT_BGR = 5,
    SAV1_PIXEL_FORMAT_YUY2 = 6,
    SAV1_PIXEL_FORMAT_UYVY = 7,
    SAV1_PIXEL_FORMAT_YVYU = 8,
} Sav1PixelFormat;

typedef struct Sav1VideoFrame {
    uint8_t *data;
    size_t size;
    ptrdiff_t stride;
    size_t width;
    size_t height;
    uint64_t timecode;
    uint8_t color_depth;
    int codec;
    Sav1PixelFormat pixel_format;
} Sav1VideoFrame;

int
sav1_video_frame_destroy(Sav1Context *context, Sav1VideoFrame *frame);

int
sav1_video_frame_clone(Sav1Context *context, Sav1VideoFrame *src_frame,
                       Sav1VideoFrame **dst_frame);

#endif
