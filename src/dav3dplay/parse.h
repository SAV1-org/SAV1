#ifndef PARSE_H
#define PARSE_H

#include <stddef.h>
#include <cstdint>
#include "thread_queue.h"

#define PARSE_STATUS_OK 0
#define PARSE_STATUS_END_OF_FILE 1
#define PARSE_STATUS_ERROR 2
#define PARSE_TARGET_AV1 1
#define PARSE_TARGET_OPUS 2
#define PARSE_FRAME_TYPE_AV1 1
#define PARSE_FRAME_TYPE_OPUS 2

typedef struct WebMFrame {
    uint8_t *data;      // the frame data bytes
    size_t size;        // the number of bytes in the frame
    uint64_t timecode;  // the timestamp of the frame in milliseconds
    double opus_sampling_frequency;
    uint64_t opus_num_channels;
    int type;
} WebMFrame;

void
webm_frame_init(WebMFrame **frame, std::size_t size);

void
webm_frame_destroy(WebMFrame *frame);

typedef struct ParseContext {
    Sav1ThreadQueue *video_output_queue;
    Sav1ThreadQueue *audio_output_queue;
    int codec_target;
    thread_atomic_int_t status;
    thread_atomic_int_t do_parse;
    void *internal_state;  // internal webm_parser variables
} ParseContext;

void
parse_init(ParseContext **context, char *file_name, int codec_target,
           Sav1ThreadQueue *video_output_queue, Sav1ThreadQueue *audio_output_queue);

void
parse_destroy(ParseContext *context);

int
parse_start(void *context);

void
parse_stop(ParseContext *context);

int
parse_get_status(ParseContext *context);

int
parse_found_av1_track(ParseContext *context);

int
parse_found_opus_track(ParseContext *context);

#endif
