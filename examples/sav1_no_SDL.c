#include "sav1.h"

int
main(int argc, char *argv[])
{
    // use default settings
    Sav1Settings settings;
    sav1_default_settings(&settings, "video_file.webm");

    // create SAV1 context
    Sav1Context context = {0};
    sav1_create_context(&context, &settings);

    // start playback
    sav1_start_playback(&context);

    // loop forever
    while (1) {
        int frame_ready;

        // video frame
        sav1_get_video_frame_ready(&context, &frame_ready);
        if (frame_ready) {
            Sav1VideoFrame *sav1_frame;
            sav1_get_video_frame(&context, &sav1_frame);
            // do something with sav1_frame...
        }

        // audio frame
        sav1_get_audio_frame_ready(&context, &frame_ready);
        if (frame_ready) {
            Sav1AudioFrame *sav1_frame;
            sav1_get_audio_frame(&context, &sav1_frame);
            // do something with sav1_frame...
        }
    }

    // clean up SAV1 context
    sav1_destroy_context(&context);

    return 0;
}
