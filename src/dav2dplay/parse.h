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

typedef struct ParseContext {
    uint8_t *data;
    size_t size;
    size_t capacity;
    uint64_t timecode;  // the timestamp of the frame in milliseconds
    void *internal_state;
} ParseContext;

void
parse_init(ParseContext **context, char *file_name);

void
parse_destroy(ParseContext *context);

int
parse_get_next_frame(ParseContext *context);

#endif
