#ifndef SAV1_SETTINGS_H
#define SAV1_SETTINGS_H

#include <stdint.h>
#include <stddef.h>

typedef struct Sav1VideoFrame Sav1VideoFrame;
typedef struct Sav1AudioFrame Sav1AudioFrame;

#define SAV1_CODEC_AV1 1
#define SAV1_CODEC_OPUS 2

#define SAV1_USE_CUSTOM_PROCESSING_VIDEO 1
#define SAV1_USE_CUSTOM_PROCESSING_AUDIO 2

typedef enum {
    SAV1_PIXEL_FORMAT_RGBA = 0,
    SAV1_PIXEL_FORMAT_ARGB = 1,
    SAV1_PIXEL_FORMAT_BGRA = 2,
    SAV1_PIXEL_FORMAT_ABGR = 3,
    SAV1_PIXEL_FORMAT_RGB = 4,
    SAV1_PIXEL_FORMAT_BGR = 5,
    SAV1_PIXEL_FORMAT_YUY2 = 6,
    SAV1_PIXEL_FORMAT_UYVY = 7,
    SAV1_PIXEL_FORMAT_YVYU = 8,
} Sav1PixelFormat;

typedef enum {
    SAV1_AUDIO_FREQ_8KHZ = 8000,
    SAV1_AUDIO_FREQ_12KHZ = 12000,
    SAV1_AUDIO_FREQ_16KHZ = 16000,
    SAV1_AUDIO_FREQ_24KHZ = 24000,
    SAV1_AUDIO_FREQ_48KHZ = 48000,
} Sav1AudioFrequency;

typedef enum { SAV1_AUDIO_MONO = 1, SAV1_AUDIO_STEREO = 2 } Sav1AudioChannel;

typedef enum { SAV1_PLAYBACK_TIMED, SAV1_PLAYBACK_FAST } Sav1PlaybackMode;

typedef struct Sav1Settings {
    char *file_name;
    int codec_target;
    size_t queue_size;
    int use_custom_processing;
    double playback_speed;
    int on_video_end;
    void *(*custom_video_frame_processing)(Sav1VideoFrame *, void *);
    void (*custom_video_frame_destroy)(void *, void *);
    void *custom_video_frame_processing_cookie;
    void *(*custom_audio_frame_processing)(Sav1AudioFrame *, void *);
    void (*custom_audio_frame_destroy)(void *, void *);
    void *custom_audio_frame_processing_cookie;
    Sav1PixelFormat desired_pixel_format;
    Sav1AudioFrequency frequency;
    Sav1AudioChannel channels;
    Sav1PlaybackMode playback_mode;
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
