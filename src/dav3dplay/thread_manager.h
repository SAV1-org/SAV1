#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include "parse.h"
#include "thread_queue.h"
#include "thread.h"
#include "sav1_settings.h"

typedef struct ThreadManager {
    ParseContext *parse_context;
    Sav1ThreadQueue *video_webm_frame_queue;
    Sav1ThreadQueue *audio_webm_frame_queue;
    Sav1ThreadQueue *video_output_queue;
    Sav1ThreadQueue *audio_output_queue;
    thread_ptr_t parse_thread;
    thread_ptr_t process_av1_thread;
    thread_ptr_t process_opus_thread;
    Sav1Settings *settings;
} ThreadManager;

void
thread_manager_init(ThreadManager **manager, Sav1Settings *settings);

void
thread_manager_destroy(ThreadManager *manager);

void
thread_manager_start_pipeline(ThreadManager *manager);

void
thread_manager_kill_pipeline(ThreadManager *manager);

void
thread_manager_lock_pipeline(ThreadManager *manager);

void
thread_manager_unlock_pipeline(ThreadManager *manager);

#endif
