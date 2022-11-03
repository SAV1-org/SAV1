#include <stdio.h>
#include <stdint.h>
#include <strings.h>
#include <stdlib.h>

#include <dav1d/dav1d.h>

typedef struct DecodeContext {
    Dav1dContext *dc;
    Dav1dPicture picture;
} DecodeContext;

void
decode_init(DecodeContext *context);

void
decode_destroy(DecodeContext *context);

int
decode_frame(DecodeContext *context, uint8_t *data, size_t size);
