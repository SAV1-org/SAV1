#ifndef PARSE_H
#define PARSE_H

#include <cassert>
#include <string.h>

#include <webm/callback.h>
#include <webm/file_reader.h>
#include <webm/status.h>
#include <webm/webm_parser.h>

#define PARSE_OK 0b00000000
#define PARSE_END_OF_FILE 0b00000001
#define PARSE_ERROR 0b00000010
#define PARSE_NO_AV1_TRACK 0b00000100

typedef struct ParseContext {
    std::uint8_t *data;
    std::size_t size;
    std::size_t capacity;
    std::uint64_t timecode;  // the timestamp of the frame in milliseconds
    void *internal_state;
} ParseContext;

void
parse_init(ParseContext *context, char *file_name);

void
parse_destroy(ParseContext *context);

int
parse_get_next_frame(ParseContext *context);

#endif
