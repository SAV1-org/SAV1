#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "dav1d/dav1d.h"

#define OBU_ALIGNED 0
#define OBU_MISALIGNED 0
#define OBU_COMBINED 1

// gcc basic.c -I../decode/dav1d/include -L. -ldav1d

void fake_dealloc(const uint8_t* buf, void* cookie) {
    /* everything in this is statically allocated, no need to worry about this yet */
    return;
}

void try_to_get_picture(Dav1dContext *context) {
    Dav1dPicture picture = {0};
    int status = dav1d_get_picture(context, &picture);

    if (status < 0) {
        printf("dav1d_get_picture status = %s\n", strerror(-status));
    }
    else {
        printf("dav1d_get_picture successful\n");
    }
}

int main() {
    Dav1dSettings settings = {0};
    dav1d_default_settings(&settings);
    Dav1dContext *context;
    dav1d_open(&context, &settings);

#if OBU_ALIGNED
    uint8_t obu1[] = {10, 12, 32, 0, 0, 0, 143, 159, 255, 52, 4, 52, 0, 128};
    int obu1_sz = 14;
    uint8_t obu2[] = {50, 25, 16, 0, 191, 0, 0, 2, 71, 128, 2, 197, 197, 249, 197, 230, 143, 255, 118, 119, 34, 30, 9, 197, 234, 167, 128};
    int obu2_sz = 27;
#endif

#if OBU_MISALIGNED
    uint8_t obu1[] = {10, 12, 32, 0, 0, 0, 143, 159, 255, 52, 4, 52, 0, 128, 50, 25};
    int obu1_sz = 16;
    uint8_t obu2[] = {16, 0, 191, 0, 0, 2, 71, 128, 2, 197, 197, 249, 197, 230, 143, 255, 118, 119, 34, 30, 9, 197, 234, 167, 128};
    int obu2_sz = 25;
#endif

#if OBU_COMBINED
    uint8_t obu2[] = {10, 12, 32, 0, 0, 0, 143, 159, 255, 52, 4, 52, 0, 128, 50, 25, 16, 0, 191, 0, 0, 2, 71, 128, 2, 197, 197, 249, 197, 230, 143, 255, 118, 119, 34, 30, 9, 197, 234, 167, 128};
    int obu2_sz = 41;
#endif

    Dav1dData data1 = {0};
    Dav1dData data2 = {0};

    int status;

#if !OBU_COMBINED
    dav1d_data_wrap(&data1, obu1, obu1_sz, fake_dealloc, NULL);
    status = dav1d_send_data(context, &data1);
    printf("dav1d_send_data status = %s\n", strerror(-status));
#endif

    dav1d_data_wrap(&data2, obu2, obu2_sz, fake_dealloc, NULL);
    status = dav1d_send_data(context, &data2);
    printf("dav1d_send_data status = %s\n", strerror(-status));

    try_to_get_picture(context);
    try_to_get_picture(context);

    dav1d_close(&context);

    return 0;
}
