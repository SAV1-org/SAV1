#include <stdlib.h>
#include <stdio.h>

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
    if (((*context) = (DecodeAv1Context *)malloc(sizeof(DecodeAv1Context))) == NULL) {
        sav1_set_error(ctx, "malloc() failed in decode_av1_init()");
        sav1_set_critical_error_flag(ctx);
        return;
    }

    (*context)->ctx = ctx;
    (*context)->input_queue = input_queue;
    (*context)->output_queue = output_queue;

    Dav1dSettings settings;
    dav1d_default_settings(&settings);
    dav1d_open(&(*context)->dav1d_context, &settings);
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

    /*
    seeking finite state machine:
    seek_state: 0=not seeking, 1=looking for sequence header, 2=found sequence header
    seek_feed_state: 0=do not feed yet, 1=feed when we find a keyframe, 2=do feed
    */
    int seek_state = 0;
    int seek_feed_state = 0;
    Dav1dSequenceHeader seq_hdr;
    Dav1dData data;
    Dav1dPicture *picture;
    if ((picture = (Dav1dPicture *)calloc(1, sizeof(Dav1dPicture))) == NULL) {
        sav1_set_error(decode_context->ctx, "malloc() failed in decode_av1_start()");
        sav1_set_critical_error_flag(decode_context->ctx);
        return -1;
    }

    while (thread_atomic_int_load(&(decode_context->do_decode))) {
        // pull a webm frame from the input queue
        WebMFrame *input_frame =
            (WebMFrame *)sav1_thread_queue_pop(decode_context->input_queue);
        if (input_frame == NULL) {
            sav1_thread_queue_push(decode_context->output_queue, NULL);
            break;
        }

        if (seek_state || input_frame->do_discard || input_frame->sentinel) {
            if (seek_state == 0) {
                // seeking has begun
                dav1d_flush(decode_context->dav1d_context);
                seek_state = 1;
            }
            if (seek_feed_state == 0 && input_frame->sentinel) {
                // feed starting at next keyframe
                seek_feed_state = 1;
            }
            if (seek_feed_state == 1 && input_frame->is_key_frame) {
                // feed starting now
                seek_feed_state = 2;
            }
            if (seek_state == 1) {
                // look for sequence header OBU
                if (dav1d_parse_sequence_header(&seq_hdr, input_frame->data,
                                                input_frame->size)) {
                    webm_frame_destroy(input_frame);
                    continue;
                }
                else {
                    // found one
                    seek_state = 2;
                }
            }
            if (seek_state == 2 && seek_feed_state != 2) {
                // throw this frame away since it's pre sentinel+keyframe
                webm_frame_destroy(input_frame);
                continue;
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
                        // throw this dav1dPicture away
                        dav1d_picture_unref(picture);
                        free(picture);
                    }
                    else {
                        if (seek_feed_state == 2) {
                            picture->m.user_data.data = (const uint8_t *)1;
                            // reset seeking finite state machine
                            seek_state = 0;
                            seek_feed_state = 0;
                        }
                        else {
                            picture->m.user_data.data = NULL;
                        }

                        // push to the output queue
                        sav1_thread_queue_push(decode_context->output_queue, picture);
                    }

                    // allocate a new dav1d picture
                    if ((picture = (Dav1dPicture *)calloc(1, sizeof(Dav1dPicture))) ==
                        NULL) {
                        sav1_set_error(decode_context->ctx,
                                       "malloc() failed in decode_av1_start()");
                        sav1_set_critical_error_flag(decode_context->ctx);
                        return -1;
                    }
                }
            } while (status == 0);
        } while (data.sz > 0);

        webm_frame_destroy(input_frame);
    }

    dav1d_picture_unref(picture);
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
        free(picture);
    }
}
