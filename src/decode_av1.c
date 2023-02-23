#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "webm_frame.h"
#include "decode_av1.h"
#include "sav1_internal.h"

void
fake_dealloc(const uint8_t *data, void *user_data)
{
    // NOP
}

void
decode_av1_init(DecodeAv1Context **context, Sav1InternalContext *ctx,
                Sav1ThreadQueue *input_queue, Sav1ThreadQueue *output_queue)
{
    DecodeAv1Context *decode_context =
        (DecodeAv1Context *)malloc(sizeof(DecodeAv1Context));
    *context = decode_context;

    decode_context->ctx = ctx;
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
    thread_atomic_int_store(&(decode_context->do_decode), 1);

    int status;
    int seek_state = 0;
    Dav1dSequenceHeader seq_hdr;
    Dav1dData data;
    Dav1dPicture *picture = (Dav1dPicture *)malloc(sizeof(Dav1dPicture));
    memset(picture, 0, sizeof(Dav1dPicture));

    while (thread_atomic_int_load(&(decode_context->do_decode))) {
        // pull a webm frame from the input queue
        WebMFrame *input_frame =
            (WebMFrame *)sav1_thread_queue_pop(decode_context->input_queue);
        if (input_frame == NULL) {
            sav1_thread_queue_push(decode_context->output_queue, NULL);
            break;
        }

        /*
        seeking finite state machine
            STATE 0: not seeking
            STATE 1: looking for sequence header, don't feed
            STATE 2: looking for sequence header, feed when keyframe
            STATE 3: looking for sequence header, feed now
            STATE 4: found sequence header, don't feed
            STATE 5: found sequence header, feed when keyframe
            STATE 6: found sequence header, feed now
        */
        if (input_frame->sentinel || input_frame->do_discard || seek_state) {
            if (seek_state == 0) {
                // seeking has begun
                dav1d_flush(decode_context->dav1d_context);
                seek_state = 1;
            }
            if (seek_state == 1 && input_frame->sentinel) {
                seek_state = 2;
            }
            if (seek_state == 2 && input_frame->is_key_frame) {
                seek_state = 3;
            }
            if (seek_state > 0 && seek_state <= 3) {
                // look for sequence header OBU
                if (dav1d_parse_sequence_header(&seq_hdr, input_frame->data,
                                                input_frame->size)) {
                    webm_frame_destroy(input_frame);
                    continue;
                }
                else if (seek_state == 1) {
                    seek_state = 4;
                }
                else if (seek_state == 2) {
                    seek_state = 5;
                }
                else {
                    seek_state = 6;
                }
            }
            if (seek_state == 4 || seek_state == 5) {
                if (seek_state == 4 && input_frame->sentinel &&
                    input_frame->is_key_frame) {
                    seek_state = 6;
                }
                else if (seek_state == 4 && input_frame->sentinel) {
                    seek_state = 5;
                    webm_frame_destroy(input_frame);
                    continue;
                }
                else if (seek_state == 5 && input_frame->is_key_frame) {
                    seek_state = 6;
                }
                else {
                    webm_frame_destroy(input_frame);
                    continue;
                }
            }
        }

        // wrap the OBUs in a Dav1dData struct
        status = dav1d_data_wrap(&data, input_frame->data, input_frame->size,
                                 fake_dealloc, NULL);
        if (status) {
            printf("Error wrapping dav1d data\n");
        }

        do {
            // send the OBUs to dav1d
            status = dav1d_send_data(decode_context->dav1d_context, &data);
            if (status && status != DAV1D_ERR(EAGAIN)) {
                webm_frame_destroy(input_frame);
                printf("Error sending dav1d data\n");
                continue;
            }

            do {
                // attempt to get picture from dav1d
                status = dav1d_get_picture(decode_context->dav1d_context, picture);

                // try one more time if dav1d tells us to (we usually have to)
                if (status == DAV1D_ERR(EAGAIN)) {
                    status = dav1d_get_picture(decode_context->dav1d_context, picture);
                }

                // see if we have a picture to output
                if (status == 0) {
                    // save the timecode into the dav1dPicture
                    picture->m.timestamp = input_frame->timecode;

                    if (input_frame->do_discard) {
                        // throw this dav1dPicture away since it was marked for
                        // discard
                        dav1d_picture_unref(picture);
                    }
                    else {
                        if (seek_state == 6) {
                            picture->m.user_data.data = (const uint8_t *)1;
                            seek_state = 0;
                        }
                        else {
                            picture->m.user_data.data = NULL;
                        }

                        // push to the output queue
                        sav1_thread_queue_push(decode_context->output_queue, picture);
                    }

                    // allocate a new dav1d picture
                    picture = (Dav1dPicture *)malloc(sizeof(Dav1dPicture));
                    memset(picture, 0, sizeof(Dav1dPicture));
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
    thread_atomic_int_store(&(context->do_decode), 0);

    // add a fake entry to the input queue if necessary
    if (sav1_thread_queue_get_size(context->input_queue) == 0) {
        sav1_thread_queue_push_timeout(context->input_queue, NULL);
    }

    // drain the output queue
    decode_av1_drain_output_queue(context);
}

void
decode_av1_drain_output_queue(DecodeAv1Context *context)
{
    while (1) {
        Dav1dPicture *picture =
            (Dav1dPicture *)sav1_thread_queue_pop_timeout(context->output_queue);
        if (picture == NULL) {
            break;
        }
        dav1d_picture_unref(picture);
    }
}
