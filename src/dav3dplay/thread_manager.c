#include <stdio.h>
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

    // populate the thread manager struct
    thread_manager->settings = settings;
    thread_manager->parse_thread = NULL;
    thread_manager->process_av1_thread = NULL;
    thread_manager->process_opus_thread = NULL;
}

// this is just a demo program. I haven't built the thread manager yet
int
main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Expected video input file\n");
        return 1;
    }

    Sav1ThreadQueue *video_frames;
    Sav1ThreadQueue *audio_frames;
    sav1_thread_queue_init(&video_frames, 10);
    sav1_thread_queue_init(&audio_frames, 10);

    ParseContext *context;
    parse_init(&context, argv[1], 0, video_frames, audio_frames);

    thread_ptr_t parse_thread =
        thread_create(parse_start, context, THREAD_STACK_SIZE_DEFAULT);

    uint64_t count = 0;
    while (count < 9999) {
        printf("%llu: Audio frames: %d, Video frames: %d\n", count,
               sav1_thread_queue_get_size(audio_frames),
               sav1_thread_queue_get_size(video_frames));
        count++;
        if (count % 50 == 0) {
            if (sav1_thread_queue_get_size(video_frames)) {
                WebMFrame *video_frame = (WebMFrame *)sav1_thread_queue_pop(video_frames);
                webm_frame_destroy(video_frame);
            }
            if (sav1_thread_queue_get_size(audio_frames)) {
                WebMFrame *audio_frame = (WebMFrame *)sav1_thread_queue_pop(audio_frames);
                webm_frame_destroy(audio_frame);
            }
        }
    }

    parse_stop(context);
    thread_join(parse_thread);
    thread_destroy(parse_thread);

    printf("\nDone!\n");

    return 0;
}
