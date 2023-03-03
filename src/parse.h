#ifndef PARSE_H
#define PARSE_H

#include "thread_queue.h"
#include "sav1_settings.h"

#include <stddef.h>
#include <stdint.h>

typedef struct Sav1InternalContext Sav1InternalContext;

#define PARSE_STATUS_OK 0
#define PARSE_STATUS_END_OF_FILE 1
#define PARSE_STATUS_ERROR 2

typedef struct ParseContext {
    Sav1ThreadQueue *video_output_queue;
    Sav1ThreadQueue *audio_output_queue;
    int codec_target;
    Sav1OnFileEnd on_file_end;
    thread_atomic_int_t status;
    thread_atomic_int_t do_parse;
    thread_atomic_int_t do_seek;
    thread_mutex_t *duration_lock;
    thread_mutex_t *wait_before_seek;
    thread_mutex_t *wait_after_parse;
    thread_mutex_t *wait_to_acquire;
    uint64_t seek_timecode;
    uint64_t duration;
    void *internal_state;  // internal webm_parser variables
    Sav1InternalContext *ctx;
} ParseContext;

void
parse_init(ParseContext **context, Sav1InternalContext *ctx,
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

void
parse_seek_to_time(ParseContext *context, uint64_t timecode);

#endif
