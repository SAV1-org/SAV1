#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include "thread_queue.h"
#include "thread.h"
#include "sav1_settings.h"
#include "parse.h"
#include "decode_av1.h"
#include "convert_av1.h"
#include "custom_processing_video.h"
#include "decode_opus.h"
#include "custom_processing_audio.h"

typedef struct ThreadManager {
    ParseContext *parse_context;
    DecodeAv1Context *decode_av1_context;
    ConvertAv1Context *convert_av1_context;
    CustomProcessingVideoContext *custom_processing_video_context;
    DecodeOpusContext *decode_opus_context;
    CustomProcessingAudioContext *custom_processing_audio_context;
    Sav1ThreadQueue *video_webm_frame_queue;
    Sav1ThreadQueue *video_dav1d_picture_queue;
    Sav1ThreadQueue *audio_webm_frame_queue;
    Sav1ThreadQueue *video_custom_processing_queue;
    Sav1ThreadQueue *audio_custom_processing_queue;
    Sav1ThreadQueue *video_output_queue;
    Sav1ThreadQueue *audio_output_queue;
    thread_ptr_t parse_thread;
    thread_ptr_t decode_av1_thread;
    thread_ptr_t convert_av1_thread;
    thread_ptr_t custom_processing_video_thread;
    thread_ptr_t decode_opus_thread;
    thread_ptr_t custom_processing_audio_thread;
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

void
thread_manager_seek_to_time(ThreadManager *manager, uint64_t timecode);

uint64_t
thread_manager_get_duration(ThreadManager *manager);

#endif
