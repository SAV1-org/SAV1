#include <cstdlib>
#include <cstdio>

#include "thread_queue.h"
#include "decode_av1.h"

void
fake_dealloc(const uint8_t *, void *)
{
    // NOP
}

void
decode_av1_init(DecodeAv1Context **context, Sav1ThreadQueue *input_queue,
                Sav1ThreadQueue *output_queue)
{
    DecodeAv1Context *decode_context =
        (DecodeAv1Context *)malloc(sizeof(DecodeAv1Context));
    *context = decode_context;

    decode_context->input_queue = input_queue;
    decode_context->output_queue = output_queue;

    Dav1dSettings settings;
    dav1d_default_settings(&settings);
    dav1d_open(&decode_context->dav1d_context, &settings);
}

void
decode_av1_destroy(DecodeAv1Context *context)
{
    dav1d_close(&context->dav1d_context);
    free(context);
}

int
decode_av1_start(void *context)
{
    DecodeAv1Context *decode_context = (DecodeAv1Context *)context;
    thread_atomic_int_store(&(decode_context->do_convert), 1);

    Dav1dData data;
    Dav1dData *picture = (Dav1dPicture *)malloc(sizeof(Dav1dPicture));
    int status;

    while (thread_atomic_int_load(&(decode_context->do_convert))) {
        // pull a webm frame from the input queue
        WebMFrame *input_frame =
            (WebMFrame *)sav1_thread_queue_pop(decode_context->input_queue);
        if (input_frame == NULL) {
            sav1_thread_queue_push(process_context->output_queue, NULL);
            break;
        }

        // decode the webm frame
        if (decode_av1_frame(process_context->decode_context, input_frame->data,
                             input_frame->size)) {
            webm_frame_destroy(input_frame);
            continue;
        }

        // wrap the OBUs in a Dav1dData struct
        status = dav1d_data_wrap(&data, input_frame->data, input_frame->size,
                                 fake_dealloc, NULL);
        if (status) {
            printf("Error wrapping dav1d data\n");
        }

        do {
            // send the OBUs to dav1d
            status = dav1d_send_data(context->dav1d_context, &data);
            if (status && status != DAV1D_ERR(EAGAIN)) {
                // manually unref the data
                dav1d_data_unref(&data);
                printf("Error sending dav1d data\n");
            }

            do {
                // attempt to get picture from dav1d
                status = dav1d_get_picture(context->dav1d_context, picture);

                // try one more time if dav1d tells us to (we usually have to)
                if (status == DAV1D_ERR(EAGAIN)) {
                    status = dav1d_get_picture(context->dav1d_context, picture);
                }

                // see if we have a picture to output
                if (status == 0) {
                    // save the timecode into the dav1dPicture
                    picture->m.timestamp = input_frame->timecode;

                    // push to the output queue
                    sav1_thread_queue_push(process_context->output_queue, picture);

                    // allocate a new dav1d picture
                    picture = (Dav1dPicture *)malloc(sizeof(Dav1dPicture));
                }
            } while (status == 0);
        } while (data.sz > 0);

        webm_frame_destroy(input_frame);
    }

    free(picture);
    return 0;
}

void
decode_av1_stop(DecodeAv1Context *context)
{
    thread_atomic_int_store(&(context->do_convert), 0);

    // add a fake entry to the input queue if necessary
    if (sav1_thread_queue_get_size(context->input_queue) == 0) {
        sav1_thread_queue_push_timeout(context->input_queue, NULL);
    }

    // drain the output queue
    while (1) {
        DavidPicture *picture =
            (DavidPicture *)sav1_thread_queue_pop_timeout(context->output_queue);
        if (picture == NULL) {
            break;
        }
        dav1d_picture_unref(picture);
    }
}
