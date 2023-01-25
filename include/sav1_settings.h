#ifndef SAV1_SETTINGS_H
#define SAV1_SETTINGS_H

#include "sav1_video_frame.h"
#include "sav1_audio_frame.h"

#define SAV1_CODEC_TARGET_AV1 1
#define SAV1_CODEC_TARGET_OPUS 2

#define SAV1_USE_CUSTOM_PROCESSING_VIDEO 1
#define SAV1_USE_CUSTOM_PROCESSING_AUDIO 2

#define SAV1_PIXEL_FORMAT_RGBA 0
#define SAV1_PIXEL_FORMAT_ARGB 1
#define SAV1_PIXEL_FORMAT_BGRA 2
#define SAV1_PIXEL_FORMAT_ABGR 3
#define SAV1_PIXEL_FORMAT_RGB 4
#define SAV1_PIXEL_FORMAT_BGR 5
#define SAV1_PIXEL_FORMAT_YUY2 6
#define SAV1_PIXEL_FORMAT_UYVY 7
#define SAV1_PIXEL_FORMAT_YVYU 8

typedef enum {
    SAV1_AUDIO_FREQ_8KHZ = 8000,
    SAV1_AUDIO_FREQ_12KHZ = 12000,
    SAV1_AUDIO_FREQ_16KHZ = 16000,
    SAV1_AUDIO_FREQ_24KHZ = 24000,
    SAV1_AUDIO_FREQ_48KHZ = 48000,
} SAV1_AudioFrequency;

typedef enum { SAV1_AUDIO_MONO = 1, SAV1_AUDIO_STEREO = 2 } SAV1_AudioChannel;

typedef struct Sav1Settings {
    char *file_name;
    int codec_target;
    int desired_pixel_format;
    size_t queue_size;
    int use_custom_processing;
    void *(*custom_video_frame_processing)(Sav1VideoFrame *, void *);
    void (*custom_video_frame_destroy)(void *, void *);
    void *custom_video_frame_processing_cookie;
    void *(*custom_audio_frame_processing)(Sav1AudioFrame *, void *);
    void (*custom_audio_frame_destroy)(void *, void *);
    void *custom_audio_frame_processing_cookie;
    SAV1_AudioFrequency frequency;
    SAV1_AudioChannel channels;
} Sav1Settings;

void
sav1_default_settings(Sav1Settings *settings, char *file_name);

void
sav1_settings_use_custom_video_processing(
    Sav1Settings *settings,
    void *(*processing_function)(Sav1VideoFrame *frame, void *cookie),
    void (*destroy_function)(void *, void *), void *cookie);

void
sav1_settings_use_custom_audio_processing(
    Sav1Settings *settings,
    void *(*processing_function)(Sav1AudioFrame *frame, void *cookie),
    void (*destroy_function)(void *, void *), void *cookie);

#endif
