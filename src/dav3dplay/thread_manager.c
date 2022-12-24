#include <cstdlib>

#include "thread_manager.h"

void
thread_manager_init(ThreadManager **manager, Sav1Settings *settings)
{
    ThreadManager *thread_manager = (ThreadManager *)malloc(sizeof(ThreadManager));
    *manager = thread_manager;

    // initialize the thread queues
    sav1_thread_queue_init(&(thread_manager->video_webm_frame_queue),
                           settings->queue_size);
    sav1_thread_queue_init(&(thread_manager->audio_webm_frame_queue),
                           settings->queue_size);
    sav1_thread_queue_init(&(thread_manager->video_dav1d_picture_queue),
                           settings->queue_size);
    sav1_thread_queue_init(&(thread_manager->video_output_queue), settings->queue_size);
    sav1_thread_queue_init(&(thread_manager->audio_output_queue), settings->queue_size);

    if (settings->use_custom_processing & SAV1_USE_CUSTOM_PROCESSING_VIDEO) {
        sav1_thread_queue_init(&(thread_manager->video_custom_processing_queue),
                               settings->queue_size);
        convert_av1_init(&(thread_manager->convert_av1_context),
                         settings->desired_pixel_format,
                         thread_manager->video_dav1d_picture_queue,
                         thread_manager->video_custom_processing_queue);
        custom_processing_video_init(&(thread_manager->custom_processing_video_context),
                                     settings->custom_video_frame_processing,
                                     settings->custom_video_frame_processing_cookie,
                                     thread_manager->video_custom_processing_queue,
                                     thread_manager->video_output_queue);
    }
    else {
        thread_manager->custom_processing_video_context = NULL;
        thread_manager->video_custom_processing_queue = NULL;
        convert_av1_init(&(thread_manager->convert_av1_context),
                         settings->desired_pixel_format,
                         thread_manager->video_dav1d_picture_queue,
                         thread_manager->video_output_queue);
    }

    // initialize the thread contexts
    parse_init(&(thread_manager->parse_context), settings->file_name,
               settings->codec_target, thread_manager->video_webm_frame_queue,
               thread_manager->audio_webm_frame_queue);
    decode_av1_init(&(thread_manager->decode_av1_context),
                    thread_manager->video_webm_frame_queue,
                    thread_manager->video_dav1d_picture_queue);

    // populate the thread manager struct
    thread_manager->settings = settings;
    thread_manager->parse_thread = NULL;
    thread_manager->decode_av1_thread = NULL;
    thread_manager->convert_av1_thread = NULL;
    thread_manager->process_opus_thread = NULL;
    thread_manager->custom_processing_video_thread = NULL;
}

void
thread_manager_destroy(ThreadManager *manager)
{
    // stop the child threads if they are still running
    thread_manager_kill_pipeline(manager);

    // destroy the contexts
    parse_destroy(manager->parse_context);
    decode_av1_destroy(manager->decode_av1_context);
    convert_av1_destroy(manager->convert_av1_context);

    // destroy the thread queues
    sav1_thread_queue_destroy(manager->video_webm_frame_queue);
    sav1_thread_queue_destroy(manager->audio_webm_frame_queue);
    sav1_thread_queue_destroy(manager->video_dav1d_picture_queue);
    sav1_thread_queue_destroy(manager->video_output_queue);
    sav1_thread_queue_destroy(manager->audio_output_queue);

    // optionally destroy custom processing
    if (manager->settings->use_custom_processing & SAV1_USE_CUSTOM_PROCESSING_VIDEO) {
        custom_processing_video_destroy(manager->custom_processing_video_context);
        sav1_thread_queue_destroy(manager->video_custom_processing_queue);
    }

    free(manager);
}

void
thread_manager_start_pipeline(ThreadManager *manager)
{
    // create the webm parsing thread
    manager->parse_thread =
        thread_create(parse_start, manager->parse_context, THREAD_STACK_SIZE_DEFAULT);

    // create the av1 decoding thread
    manager->decode_av1_thread = thread_create(
        decode_av1_start, manager->decode_av1_context, THREAD_STACK_SIZE_DEFAULT);

    // create the av1 conversion thread
    manager->convert_av1_thread = thread_create(
        convert_av1_start, manager->convert_av1_context, THREAD_STACK_SIZE_DEFAULT);

    // optionally create the video custom processing thread
    if (manager->settings->use_custom_processing & SAV1_USE_CUSTOM_PROCESSING_VIDEO) {
        manager->custom_processing_video_thread = thread_create(
            custom_processing_video_start, manager->custom_processing_video_context,
            THREAD_STACK_SIZE_DEFAULT);
    }
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

    if (manager->decode_av1_thread != NULL) {
        decode_av1_stop(manager->decode_av1_context);
        thread_join(manager->decode_av1_thread);
        thread_destroy(manager->decode_av1_thread);
        manager->decode_av1_thread = NULL;
    }

    if (manager->convert_av1_thread != NULL) {
        convert_av1_stop(manager->convert_av1_context);
        thread_join(manager->convert_av1_thread);
        thread_destroy(manager->convert_av1_thread);
        manager->convert_av1_thread = NULL;
    }

    if (manager->custom_processing_video_thread != NULL) {
        custom_processing_video_stop(manager->custom_processing_video_context);
        thread_join(manager->custom_processing_video_thread);
        thread_destroy(manager->custom_processing_video_thread);
        manager->custom_processing_video_thread = NULL;
    }
}

void
thread_manager_lock_pipeline(ThreadManager *manager)
{
    // lock all of the queues
    sav1_thread_queue_lock(manager->video_webm_frame_queue);
    sav1_thread_queue_lock(manager->audio_webm_frame_queue);
    sav1_thread_queue_lock(manager->video_dav1d_picture_queue);
    sav1_thread_queue_lock(manager->video_output_queue);
    sav1_thread_queue_lock(manager->audio_output_queue);
    if (manager->video_custom_processing_queue != NULL) {
        sav1_thread_queue_lock(manager->video_custom_processing_queue);
    }
}

void
thread_manager_unlock_pipeline(ThreadManager *manager)
{
    // unlock all of the queues
    sav1_thread_queue_unlock(manager->video_webm_frame_queue);
    sav1_thread_queue_unlock(manager->audio_webm_frame_queue);
    sav1_thread_queue_unlock(manager->video_dav1d_picture_queue);
    sav1_thread_queue_unlock(manager->video_output_queue);
    sav1_thread_queue_unlock(manager->audio_output_queue);
    if (manager->video_custom_processing_queue != NULL) {
        sav1_thread_queue_unlock(manager->video_custom_processing_queue);
    }
}
