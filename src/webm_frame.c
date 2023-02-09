#include "webm_frame.h"

#include <cassert>

void
webm_frame_init(WebMFrame **frame, size_t size)
{
    WebMFrame *webm_frame = (WebMFrame *)malloc(sizeof(WebMFrame));
    *frame = webm_frame;

    webm_frame->data = (uint8_t *)malloc(size * sizeof(uint8_t));
    webm_frame->size = size;
    webm_frame->timecode = 0;
    webm_frame->codec = 0;
    webm_frame->do_discard = 0;
    webm_frame->sentinel = 0;
}

void
webm_frame_destroy(WebMFrame *frame)
{
    assert(frame != NULL);
    assert(frame->data != NULL);
    free(frame->data);
    free(frame);
}
