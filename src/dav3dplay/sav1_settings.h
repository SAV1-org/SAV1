#ifndef SAV1_SETTINGS_H
#define SAV1_SETTINGS_H

#define SAV1_CODEC_TARGET_AV1 1
#define SAV1_CODEC_TARGET_OPUS 2

typedef struct Sav1Settings {
    char *file_name;
    int codec_target;
    size_t queue_size;
} Sav1Settings;

#endif
