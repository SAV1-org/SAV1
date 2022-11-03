#include <stdio.h>
#include <stdint.h>
#include <strings.h>
#include <stdlib.h>

#include <dav1d/dav1d.h>
#include <libyuv.h>

void
convert(Dav1dPicture *picture, uint8_t *bgra_data, size_t bgra_stride);
