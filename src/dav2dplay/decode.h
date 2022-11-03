#ifndef DECODE_H
#define DECODE_H

#include <cassert>

#include <dav1d/dav1d.h>

typedef struct DecodeContext {
    Dav1dContext *dav1d_context;
    Dav1dPicture *dav1d_picture;
} DecodeContext;

void
decode_init(DecodeContext **context);

void
decode_destroy(DecodeContext *context);

int
decode_frame(DecodeContext *context, uint8_t *data, size_t size);

#endif