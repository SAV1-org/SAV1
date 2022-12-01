#ifndef DECODE_AV1_H
#define DECODE_AV1_H

#include <dav1d/dav1d.h>

typedef struct DecodeAv1Context {
    Dav1dContext *dav1d_context;
    Dav1dPicture *dav1d_picture;
} DecodeAv1Context;

void
decode_av1_init(DecodeAv1Context **context);

void
decode_av1_destroy(DecodeAv1Context *context);

int
decode_av1_frame(DecodeAv1Context *context, uint8_t *data, size_t size);

#endif
