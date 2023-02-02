#ifndef SAV1_INTERNAL_H
#define SAV1_INTERNAL_H

#include "sav1.h"
#include "thread_manager.h"

#define SAV1_ERROR_MESSAGE_SIZE 128

typedef struct Sav1InternalContext {
    Sav1Settings *settings;
    ThreadManager *thread_manager;
    char error_message[SAV1_ERROR_MESSAGE_SIZE];
    uint8_t critical_error_flag;
    uint8_t is_playing;
    struct timespec *start_time;
    struct timespec *pause_time;
    Sav1VideoFrame *curr_video_frame;
    Sav1AudioFrame *curr_audio_frame;
} Sav1InternalContext;

void
sav1_set_error(Sav1InternalContext *ctx, const char *message);

void
sav1_set_error_with_code(Sav1InternalContext *ctx, const char *message, int code);

void
sav1_set_critical_error_flag(Sav1InternalContext *ctx);

#endif
