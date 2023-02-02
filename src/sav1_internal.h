#ifndef SAV1_INTERNAL_H
#define SAV1_INTERNAL_H

#include "sav1.h"
#include "thread_manager.h"

typedef struct Sav1InternalContext {
    Sav1Settings *settings;
    ThreadManager *thread_manager;
    char error_message[128];
    uint8_t critical_error_flag;
    uint8_t is_initialized;
    uint8_t is_playing;
    uint64_t start_time;
    uint64_t pause_time;
} Sav1InternalContext;

void
sav1_set_error(Sav1Context *context, char *message);

void
sav1_set_error_flag();

#endif
