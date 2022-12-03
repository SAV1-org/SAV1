#include <cstddef>

#include "sav1_settings.h"

void
sav1_default_settings(Sav1Settings *settings, char *file_name)
{
    settings->file_name = file_name;
    settings->codec_target = SAV1_CODEC_TARGET_AV1 | SAV1_CODEC_TARGET_OPUS;
    settings->queue_size = 20;
    settings->use_custom_processing = 0;
    settings->custom_video_frame_processing = NULL;
}
