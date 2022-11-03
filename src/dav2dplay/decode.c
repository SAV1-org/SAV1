#include "decode.h"

void
fake_dealloc(const uint8_t *data, void *cookie)
{
    // NOP
}

void
decode_init(DecodeContext **context)
{
    DecodeContext *decode_context = (DecodeContext *)malloc(sizeof(DecodeContext));
    *context = decode_context;

    Dav1dSettings settings = {0};
    dav1d_default_settings(&settings);
    dav1d_open(&decode_context->dav1d_context, &settings);

    decode_context->dav1d_picture = (Dav1dPicture *)malloc(sizeof(Dav1dPicture));
}

void
decode_destroy(DecodeContext *context)
{
    dav1d_picture_unref(context->dav1d_picture);
    dav1d_close(&context->dav1d_context);
    free(context->dav1d_picture);
    free(context);
}

int
decode_frame(DecodeContext *context, uint8_t *input, size_t size)
{
    Dav1dData data = {0};
    int status;

    dav1d_picture_unref(context->dav1d_picture);

    // wrap the OBUs in a Dav1dData struct
    status = dav1d_data_wrap(&data, input, size, fake_dealloc, NULL);
    if (status) {
        return status;
    }

    do {
        // send the OBUs to dav1d
        status = dav1d_send_data(context->dav1d_context, &data);
        if (status && status != DAV1D_ERR(EAGAIN)) {
            // manually unref the data
            dav1d_data_unref(&data);
            return status;
        }

        // attempt to get picture from dav1d
        int picture_status;
        picture_status =
            dav1d_get_picture(context->dav1d_context, context->dav1d_picture);

        // try one more time if dav1d tells us to (we usually have to)
        if (picture_status == DAV1D_ERR(EAGAIN)) {
            picture_status =
                dav1d_get_picture(context->dav1d_context, context->dav1d_picture);
        }
        if (picture_status == 0) {
            // Successfully got picture
            return 0;
        }
        else {
            return -picture_status;
        }

    } while (status == DAV1D_ERR(EAGAIN) || data.sz > 0);
}