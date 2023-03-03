#include <stddef.h>

#include "sav1_settings.h"

void
sav1_default_settings(Sav1Settings *settings, char *file_name)
{
    settings->file_name = file_name;
    settings->codec_target = SAV1_CODEC_AV1 | SAV1_CODEC_OPUS;
    settings->desired_pixel_format = SAV1_PIXEL_FORMAT_RGBA;
    settings->queue_size = 20;
    settings->use_custom_processing = 0;
    settings->custom_video_frame_processing = NULL;
    settings->custom_video_frame_destroy = NULL;
    settings->custom_video_frame_processing_cookie = NULL;
    settings->custom_audio_frame_processing = NULL;
    settings->custom_audio_frame_destroy = NULL;
    settings->custom_audio_frame_processing_cookie = NULL;
    settings->frequency = SAV1_AUDIO_FREQ_48KHZ;
    settings->channels = SAV1_AUDIO_STEREO;
    settings->playback_mode = SAV1_PLAYBACK_TIMED;
    settings->on_file_end = SAV1_FILE_END_WAIT;
}

void
sav1_settings_use_custom_video_processing(
    Sav1Settings *settings,
    int (*processing_function)(Sav1VideoFrame *frame, void *cookie),
    void (*destroy_function)(void *, void *), void *cookie)
{
    settings->use_custom_processing |= SAV1_USE_CUSTOM_PROCESSING_VIDEO;
    settings->custom_video_frame_processing = processing_function;
    settings->custom_video_frame_destroy = destroy_function;
    settings->custom_video_frame_processing_cookie = cookie;
}

void
sav1_settings_use_custom_audio_processing(
    Sav1Settings *settings,
    int (*processing_function)(Sav1AudioFrame *frame, void *cookie),
    void (*destroy_function)(void *, void *), void *cookie)
{
    settings->use_custom_processing |= SAV1_USE_CUSTOM_PROCESSING_AUDIO;
    settings->custom_audio_frame_processing = processing_function;
    settings->custom_audio_frame_destroy = destroy_function;
    settings->custom_audio_frame_processing_cookie = cookie;
}
