#ifndef CONVERT_AV1_H
#define CONVERT_AV1_H

#include <dav1d/dav1d.h>
#include <libyuv.h>

void
convert_av1(Dav1dPicture *picture, uint8_t *bgra_data, ptrdiff_t bgra_stride);

#endif
