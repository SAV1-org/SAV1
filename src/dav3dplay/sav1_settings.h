#ifndef SAV1_SETTINGS_H
#define SAV1_SETTINGS_H

#include "sav1_video_frame.h"

#define SAV1_CODEC_TARGET_AV1 1
#define SAV1_CODEC_TARGET_OPUS 2

#define SAV1_USE_CUSTOM_PROCESSING_VIDEO 1
#define SAV1_USE_CUSTOM_PROCESSING_AUDIO 2

typedef struct Sav1Settings {
    char *file_name;
    int codec_target;
    size_t queue_size;
    int use_custom_processing;
    void *(*custom_video_frame_processing)(Sav1VideoFrame *);
} Sav1Settings;

void
sav1_default_settings(Sav1Settings *settings, char *file_name);

#endif
