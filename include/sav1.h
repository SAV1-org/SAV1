#ifndef SAV1_H
#define SAV1_H

#include <stdint.h>
#include <stddef.h>

typedef struct Sav1Context {
    void *internal_state;
    uint8_t is_initialized;
} Sav1Context;

#include "sav1_settings.h"
#include "sav1_video_frame.h"
#include "sav1_audio_frame.h"

int
sav1_create_context(Sav1Context *context, Sav1Settings *settings);

int
sav1_destroy_context(Sav1Context *context);

char *
sav1_get_error(Sav1Context *context);

int
sav1_get_video_frame(Sav1Context *context, Sav1VideoFrame **frame);

int
sav1_get_audio_frame(Sav1Context *context, Sav1AudioFrame **frame);

int
sav1_get_video_frame_ready(Sav1Context *context, int *is_ready);

int
sav1_get_audio_frame_ready(Sav1Context *context, int *is_ready);

int
sav1_start_playback(Sav1Context *context);

int
sav1_stop_playback(Sav1Context *context);

int
sav1_get_playback_status(Sav1Context *context, int *status);

int
sav1_get_playback_time(Sav1Context *context, uint64_t *timecode_ms);

int
sav1_get_playback_duration(Sav1Context *context, uint64_t *duration_ms);

int
sav1_seek_playback(Sav1Context *context, uint64_t timecode_ms);

#endif