#include "decode.h"

void
dealloc_buffer(const uint8_t *data, void *cookie)
{
    free((uint8_t *) data);
}

void
decode_init(DecodeContext *context)
{
    Dav1dSettings settings = {0};
    dav1d_default_settings(&settings);
    dav1d_open(context->dc, &settings);
}

void
decode_destroy(DecodeContext *context)
{
    dav1d_close(&context->dc);
}

int
decode_frame(DecodeContext *context, uint8_t *input, size_t size)
{
    Dav1dData data = {0};
    Dav1dPicture pic = {0};
    int status;

    // wrap the OBUs in a Dav1dData struct
    status = dav1d_data_wrap(&data, input, size, dealloc_buffer, NULL);
    if (status) {
        fprintf(stderr, "dav1d data wrap failed: %d\n", status);
        return status;
    }

    do {
        // send the OBUs to dav1d
        status = dav1d_send_data(context->dc, &data);
        if (status && status != DAV1D_ERR(EAGAIN)) {
            fprintf(stderr, "dav1d data wrap failed: %d\n", status);
            // manually unref the data
            dav1d_data_unref(&data);
            return status;
        }

        // Attempt to get picture from dav1d
        int picture_status;

        picture_status = dav1d_get_picture(context->dc, context->picture);
        // try one more time if dav1d tells us to (we usually have to)
        if (picture_status == DAV1D_ERR(EAGAIN)) {
            picture_status = dav1d_get_picture(context->dc, context->picture);
        }
        if (picture_status == 0) {
            fprintf(stderr, "got picture from dav1d\n");
            return 0;  // Successfully got picture
        }
        else {
            fprintf(stderr, "failed to get picture: %d\n", -picture_status);
            return -picture_status;
        }

    } while (status == DAV1D_ERR(EAGAIN) || data.sz > 0);
}