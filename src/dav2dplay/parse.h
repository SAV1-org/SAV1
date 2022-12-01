#ifndef PARSE_H
#define PARSE_H

#include <cassert>

#include <webm/callback.h>
#include <webm/file_reader.h>
#include <webm/status.h>
#include <webm/webm_parser.h>

#define PARSE_OK 0
#define PARSE_END_OF_FILE 1
#define PARSE_ERROR 2
#define PARSE_NO_AV1_TRACK 3
#define PARSE_BUFFER_FULL 4

typedef struct WebMFrame {
    uint8_t *data;      // the frame data bytes
    size_t size;        // the number of bytes in the frame
    size_t capacity;    // the maximum number of bytes the data array can store
    uint64_t timecode;  // the timestamp of the frame in milliseconds
} WebMFrame;

typedef struct ParseContext {
    WebMFrame *video_frames;       // the buffer of av1 video frames
    WebMFrame *audio_frames;       // the buffer of opus audio frames
    size_t video_frames_size;      // the number of video frames in the buffer
    size_t audio_frames_size;      // the number of audio frames in the buffer
    size_t frame_buffer_capacity;  // the maximum number of frames the buffer can store
    size_t video_frames_index;     // the index of the earliest video frame in the buffer
    size_t audio_frames_index;     // the index of the earliest audio frame in the buffer
    int do_overwrite_buffer;       // whether the earliest frames in the buffers should be
                                   // overwritten as new ones come in
    void *internal_state;          // internal webm_parser variables
} ParseContext;

void
parse_init(ParseContext **context, char *file_name);

void
parse_destroy(ParseContext *context);

int
parse_get_next_video_frame(ParseContext *context, WebMFrame **frame);

int
parse_get_next_audio_frame(ParseContext *context, WebMFrame **frame);

void
parse_clear_audio_buffer(ParseContext *context);

void
parse_clear_video_buffer(ParseContext *context);

std::uint64_t
parse_get_opus_num_channels(ParseContext *context);

double
parse_get_opus_sampling_frequency(ParseContext *context);

std::uint64_t
parse_get_opus_bit_depth(ParseContext *context);

#endif
