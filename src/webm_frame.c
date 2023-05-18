#include <assert.h>
#include <stdlib.h>

#include "webm_frame.h"

int
webm_frame_init(WebMFrame **frame, size_t size)
{
    if ((*frame = (WebMFrame *)malloc(sizeof(WebMFrame))) == NULL) {
        return -1;
    }

    if (((*frame)->data = (uint8_t *)malloc(size * sizeof(uint8_t))) == NULL) {
        free(*frame);
        return -1;
    }

    (*frame)->size = size;
    (*frame)->timecode = 0;
    (*frame)->codec = 0;
    (*frame)->do_discard = 0;
    (*frame)->sentinel = 0;
    (*frame)->is_key_frame = 0;

    return 0;
}

void
webm_frame_destroy(WebMFrame *frame)
{
    assert(frame != NULL);
    assert(frame->data != NULL);
    free(frame->data);
    free(frame);
}
