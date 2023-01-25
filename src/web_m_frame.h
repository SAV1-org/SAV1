#ifndef WEB_M_FRAME_H
#define WEB_M_FRAME_H

#include <cstdint>

typedef struct WebMFrame {
    uint8_t *data;      // the frame data bytes
    size_t size;        // the number of bytes in the frame
    uint64_t timecode;  // the timestamp of the frame in milliseconds
    int codec;
} WebMFrame;

void
webm_frame_init(WebMFrame **frame, size_t size);

void
webm_frame_destroy(WebMFrame *frame);

#endif
