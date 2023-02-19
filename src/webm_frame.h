#ifndef WEBM_FRAME_H
#define WEBM_FRAME_H

#include <stdint.h>
#include <stddef.h>

typedef struct WebMFrame {
    uint8_t *data;      // the frame data bytes
    size_t size;        // the number of bytes in the frame
    uint64_t timecode;  // the timestamp of the frame in milliseconds
    int codec;
    int do_discard;
    int sentinel;
} WebMFrame;

void
webm_frame_init(WebMFrame **frame, size_t size);

void
webm_frame_destroy(WebMFrame *frame);

#endif
