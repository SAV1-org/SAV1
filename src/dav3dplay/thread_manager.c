#include <cstdlib>
#include "thread_manager.h"

#define THREAD_QUEUE_SIZE 25

void
thread_manager_init(ThreadManager **manager, Sav1Settings *settings)
{
    ThreadManager *thread_manager = (ThreadManager *)malloc(sizeof(ThreadManager));
    *manager = thread_manager;

    // initialize the thread queues
    sav1_thread_queue_init(&(thread_manager->video_webm_frame_queue), THREAD_QUEUE_SIZE);
    sav1_thread_queue_init(&(thread_manager->audio_webm_frame_queue), THREAD_QUEUE_SIZE);
    sav1_thread_queue_init(&(thread_manager->video_output_queue), THREAD_QUEUE_SIZE);
    sav1_thread_queue_init(&(thread_manager->audio_output_queue), THREAD_QUEUE_SIZE);

    // initialize the thread contexts
    parse_init(&(thread_manager->parse_context), settings->file_name,
               settings->codec_target, thread_manager->video_webm_frame_queue,
               thread_manager->audio_webm_frame_queue);
    process_av1_init(&(thread_manager->process_av1_context),
                     thread_manager->video_webm_frame_queue,
                     thread_manager->video_output_queue);

    // populate the thread manager struct
    thread_manager->settings = settings;
    thread_manager->parse_thread = NULL;
    thread_manager->process_av1_thread = NULL;
    thread_manager->process_opus_thread = NULL;
}

void
thread_manager_destroy(ThreadManager *manager)
{
    // stop the child threads if they are still running
    thread_manager_kill_pipeline(manager);

    // destroy the contexts
    parse_destroy(manager->parse_context);
    process_av1_destroy(manager->process_av1_context);

    // destroy the thread queues
    sav1_thread_queue_destroy(manager->video_webm_frame_queue);
    sav1_thread_queue_destroy(manager->audio_webm_frame_queue);
    sav1_thread_queue_destroy(manager->video_output_queue);
    sav1_thread_queue_destroy(manager->audio_output_queue);

    free(manager);
}

void
thread_manager_start_pipeline(ThreadManager *manager)
{
    // create the webm parsing thread
    manager->parse_thread =
        thread_create(parse_start, manager->parse_context, THREAD_STACK_SIZE_DEFAULT);

    // create the av1 processing thread
    manager->process_av1_thread = thread_create(
        process_av1_start, manager->process_av1_context, THREAD_STACK_SIZE_DEFAULT);
}

void
thread_manager_kill_pipeline(ThreadManager *manager)
{
    if (manager->parse_thread != NULL) {
        parse_stop(manager->parse_context);
        thread_join(manager->parse_thread);
        thread_destroy(manager->parse_thread);
        manager->parse_thread = NULL;
    }

    if (manager->process_av1_thread != NULL) {
        process_av1_stop(manager->process_av1_context);
        thread_join(manager->process_av1_thread);
        thread_destroy(manager->process_av1_thread);
        manager->process_av1_thread = NULL;
    }
}

void
thread_manager_lock_pipeline(ThreadManager *manager)
{
    // lock all of the queues
    sav1_thread_queue_lock(manager->video_webm_frame_queue);
    sav1_thread_queue_lock(manager->audio_webm_frame_queue);
    sav1_thread_queue_lock(manager->video_output_queue);
    sav1_thread_queue_lock(manager->audio_output_queue);
}

void
thread_manager_unlock_pipeline(ThreadManager *manager)
{
    // unlock all of the queues
    sav1_thread_queue_unlock(manager->video_webm_frame_queue);
    sav1_thread_queue_unlock(manager->audio_webm_frame_queue);
    sav1_thread_queue_unlock(manager->video_output_queue);
    sav1_thread_queue_unlock(manager->audio_output_queue);
}
