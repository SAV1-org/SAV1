#include <stdio.h>
#include "thread.h"
#include "parse.h"
#include "thread_queue.h"

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
    parse_init(&context, argv[1], PARSE_TARGET_AV1 | PARSE_TARGET_OPUS, video_frames,
               audio_frames);

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
