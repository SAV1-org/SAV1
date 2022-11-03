#ifndef CONVERT_H
#define CONVERT_H

#include <cassert>

#include <dav1d/dav1d.h>
#include <libyuv.h>

void
convert(Dav1dPicture *picture, uint8_t *bgra_data, ptrdiff_t bgra_stride);

#endif
