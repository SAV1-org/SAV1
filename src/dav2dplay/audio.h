#include <opus/opus.h>
#include <opus/opus_types.h>
#include <opus/opus_defines.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct AudioContext {
    OpusDecoder *dec;
    opus_int16* decoded;
    int Fs;
    int channels;
    int error;
} DecodeContext;

void
audio_init(AudioContext **context, int Fs, int channels, int error);

void
audio_destroy(AudioContext *context);

int
audio_decode(AudioContext *context, uint8_t *data, size_t size);
