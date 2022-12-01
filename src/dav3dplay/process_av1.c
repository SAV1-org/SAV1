#include "process_av1.h"
#include "parse.h"
#include "sav1_video_frame.h"

void
process_av1_init(ProcessAv1Context **context, Sav1ThreadQueue *input_queue,
                 Sav1ThreadQueue *output_queue)
{
    ProcessAv1Context *process_context =
        (ProcessAv1Context *)malloc(sizeof(ProcessAv1Context));
    *context = process_context;

    process_context->input_queue = input_queue;
    process_context->output_queue = output_queue;

    decode_av1_init(&(process_context->decode_context));
}

void
process_av1_destroy(ProcessAv1Context *context)
{
    decode_av1_destroy(context->decode_context);
    free(context);
}

int
process_av1_start(void *context)
{
    ProcessAv1Context *process_context = (ProcessAv1Context *)context;
    thread_atomic_int_store(&(process_context->do_process), 1);

    while (thread_atomic_int_load(&(process_context->do_process))) {
        WebMFrame *input_frame =
            (WebMFrame *)sav1_thread_queue_pop(process_context->input_queue);
        if (input_frame == NULL) {
            break;
        }

        // decode the webm frame
        if (decode_av1_frame(process_context->decode_context, input_frame->data,
                             input_frame->size)) {
            webm_frame_destroy(input_frame);
            continue;
        }

        // create the output frame
        // TODO: move this somewhere else
        Dav1dPicture *dav1d_pic = process_context->decode_context->dav1d_picture;
        Sav1VideoFrame *output_frame = (Sav1VideoFrame *)malloc(sizeof(Sav1VideoFrame));
        output_frame->codec = SAV1_CODEC_AV1;
        output_frame->color_depth = 8;
        output_frame->timecode = input_frame->timecode;
        output_frame->width = dav1d_pic->p.w;
        output_frame->height = dav1d_pic->p.h;
        output_frame->stride = 4 * output_frame->width;
        output_frame->size = output_frame->stride * output_frame->height;
        output_frame->data = (uint8_t *)malloc(output_frame->size * sizeof(uint8_t));

        webm_frame_destroy(input_frame);

        // convert the color space
        convert_av1(dav1d_pic, output_frame->data, output_frame->stride);

        // push to the output queue
        sav1_thread_queue_push(process_context->output_queue, output_frame);
    }

    return 0;
}

void
process_av1_stop(ProcessAv1Context *context)
{
    thread_atomic_int_store(&(context->do_process), 0);

    // add a fake entry to the input queue if necessary
    if (sav1_thread_queue_get_size(context->input_queue) == 0) {
        sav1_thread_queue_push_timeout(context->input_queue, NULL);
    }

    // drain the output queue
    while (1) {
        Sav1VideoFrame *frame =
            (Sav1VideoFrame *)sav1_thread_queue_pop_timeout(context->output_queue);
        if (frame == NULL) {
            break;
        }
        else {
            free(frame->data);
            free(frame);
        }
    }
}
