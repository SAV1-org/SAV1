#ifndef SAV1_SETTINGS_H
#define SAV1_SETTINGS_H

#include <stdint.h>
#include <stddef.h>

#include "common.h"

typedef struct Sav1VideoFrame Sav1VideoFrame;
typedef struct Sav1AudioFrame Sav1AudioFrame;

#define SAV1_CODEC_AV1 1
#define SAV1_CODEC_OPUS 2

#define SAV1_USE_CUSTOM_PROCESSING_VIDEO 1
#define SAV1_USE_CUSTOM_PROCESSING_AUDIO 2

typedef enum {
    SAV1_PIXEL_FORMAT_RGBA = 0, /**< { Red, Green, Blue, Alpha }  */
    SAV1_PIXEL_FORMAT_ARGB = 1, /**< { Alpha, Red, Green, Blue } */
    SAV1_PIXEL_FORMAT_BGRA = 2, /**< { Blue, Green, Red, Alpha } */
    SAV1_PIXEL_FORMAT_ABGR = 3, /**< { Alpha, Blue, Green, Red } */
    SAV1_PIXEL_FORMAT_RGB = 4,  /**< { Red, Green, Blue } */
    SAV1_PIXEL_FORMAT_BGR = 5,  /**< { Red, Green, Blue } */
    SAV1_PIXEL_FORMAT_YUY2 = 6, /**< { Y0, U0, Y1, V0 } in BT.601 limited range. */
    SAV1_PIXEL_FORMAT_UYVY = 7, /**< { U0, Y0, V0, Y1 } in BT.601 limited range. */
    SAV1_PIXEL_FORMAT_YVYU = 8, /**< { Y0, V0, Y1, U0 } in BT.601 limited range. */
} Sav1PixelFormat;

typedef enum {
    SAV1_AUDIO_FREQ_8KHZ = 8000,
    SAV1_AUDIO_FREQ_12KHZ = 12000,
    SAV1_AUDIO_FREQ_16KHZ = 16000,
    SAV1_AUDIO_FREQ_24KHZ = 24000,
    SAV1_AUDIO_FREQ_48KHZ = 48000,
} Sav1AudioFrequency;

typedef enum {
    /** Mono audio output (1 channel). */
    SAV1_AUDIO_MONO = 1,

    /** Stereo audio output (2 channel). Samples are interleaved. */
    SAV1_AUDIO_STEREO = 2
} Sav1AudioChannel;

typedef enum {
    /** Frames are synchronized to real time. Frames may be skipped if the video is
       behind. */
    SAV1_PLAYBACK_TIMED,

    /** For frame iteration: frames are not synchronized to real time, and no frames are
       skipped. */
    SAV1_PLAYBACK_FAST
} Sav1PlaybackMode;

typedef enum {
    /** Wait when the video ends for the user to seek to a different point. */
    SAV1_FILE_END_WAIT,

    /** Restart the video automatically upon reaching the end. */
    SAV1_FILE_END_LOOP
} Sav1OnFileEnd;

/**
 * @brief Settings for SAV1.
 *
 * This struct contains the settings for SAV1. It is used to configure the decoded data's
 * format and customize the processing behavior.
 */
typedef struct Sav1Settings {
    char *file_path;   /**< The path of the .webm file to be decoded. */
    int codec_target;  /**< The bitwise indication for which codecs to decode. */
    size_t queue_size; /**< The size of the internal queues used by SAV1. Larger queues
                          require more memory, but allow SAV1 to have more frames ready to
                          go in the event of a slowdown. */
    int use_custom_processing; /**< The bitwise indication for whether custom
                                  processing should be applied to audio and/or video.
                                */
    double playback_speed;     /**< The multiplier applied to the playback rate. Does not
                                  cause audio to be resampled to be shorter or longer. */
    int (*custom_video_frame_processing)(
        Sav1VideoFrame *, void *); /**< A function for postprocessing the video output
                                      frames in a separate thread,
                                      returning 0 on success, < 0 on error. */
    void (*custom_video_frame_destroy)(
        void *, void *); /**< A function to clean up any custom data added during the
                            video custom postprocessing. */
    void *custom_video_frame_processing_cookie; /**< Optional custom data that is
                                                   passed to video custom
                                                   postprocessing and destroying. */
    int (*custom_audio_frame_processing)(
        Sav1AudioFrame *, void *); /**< A function for postprocessing the audio output
                                                        frames in a separate thread,
                                      returning 0 on success, < 0 on error.
                                       */
    void (*custom_audio_frame_destroy)(
        void *, void *); /**< A function to clean up any custom data added during the
                           audio custom postprocessing. */
    void *custom_audio_frame_processing_cookie; /**< Optional custom data that is
                                                   passed to audio custom
                                                   postprocessing and destroying. */
    Sav1PixelFormat desired_pixel_format;       /**< The desired output pixel format. */
    Sav1AudioFrequency frequency;   /**< The desired audio output frequency. */
    Sav1AudioChannel channels;      /**< The desired audio output number of channels. */
    Sav1PlaybackMode playback_mode; /**< Whether the file should be played back
                                       synchronously or as fast as possible. */
    Sav1OnFileEnd
        on_file_end; /**< Whether playback should loop or wait after the file ends. */
} Sav1Settings;

/**
 * @brief Populate settings struct with defaults
 *
 * This is the easiest way to get your settings up and running to create a
 * @ref Sav1Context with. If you don't call @ref sav1_default_settings, you
 * have to set every setting yourself. It's recommended to use
 * @ref sav1_default_settings to populate your settings struct, and then
 * changing any settings you desire from the defaults after.
 *
 * - @ref Sav1Settings.codec_target defaults to `SAV1_CODEC_AV1 | SAV1_CODEC_OPUS`
 * - @ref Sav1Settings.desired_pixel_format defaults to `SAV1_PIXEL_FORMAT_RGBA`
 * - @ref Sav1Settings.queue_size defaults to `20`
 * - @ref Sav1Settings.frequency defaults to `SAV1_AUDIO_FREQ_48KHZ`
 * - @ref Sav1Settings.channels defaults to `SAV1_AUDIO_STEREO`
 * - @ref Sav1Settings.playback_mode defaults to `SAV1_PLAYBACK_TIMED`
 * - @ref Sav1Settings.on_file_end defaults to `SAV1_FILE_END_WAIT`
 * - @ref Sav1Settings.playback_speed defaults to `1.0`
 *
 * @param[in] settings pointer to a SAV1 settings struct that will be modified
 * @param[in] file_path path to the file that will be played
 *
 * @sa Sav1PixelFormat
 * @sa Sav1AudioFrequency
 * @sa Sav1AudioChannel
 * @sa Sav1PlaybackMode
 */
SAV1_API void
sav1_default_settings(Sav1Settings *settings, char *file_path);

/**
 * @brief Set up custom video processing for every @ref Sav1VideoFrame.
 *
 * Set up a custom video processing function to be applied to every @ref Sav1VideoFrame in
 * an additional thread before it reaches the user. This can be done manually by
 * modifying the @ref Sav1Settings struct directly, but this function provides a
 * convenient way to do so. New frames are not created, but the existing frame passed in
 * can be modified and the @ref Sav1VideoFrame.custom_data property can be used to store
 * additional output data. Include an optional cookie to store necessary information.
 *
 * If `destroy_function` is `NULL`, then no additional steps will be taken when frames are
 * destroyed. If memory is allocated for the @ref Sav1VideoFrame.custom_data property,
 * then a function should be passed for this argument that will free that memory.
 *
 * If `cookie` is `NULL`, then `NULL` will be passed in for the `cookie` argument of the
 * `processing_function` and the `destroy_function`. The cookie can be used to store
 * additional context that is necessary for applying whatever postprocessing is desired.
 *
 * @param[in] settings pointer to a SAV1 settings struct
 * @param[in] processing_function function to modify `Sav1VideoFrame`s and add custom data
 * @param[in] destroy_function function for destroying custom `Sav1VideoFrame` data or
 * `NULL`
 * @param[in] cookie data to be passed in during custom frame processing and destruction
 * or `NULL`
 *
 * @sa Sav1VideoFrame
 * @sa Sav1Settings
 */
SAV1_API void
sav1_settings_use_custom_video_processing(
    Sav1Settings *settings,
    int (*processing_function)(Sav1VideoFrame *frame, void *cookie),
    void (*destroy_function)(void *, void *), void *cookie);

/**
 * @brief Set up custom audio processing for every @ref Sav1AudioFrame.
 *
 * Set up a custom audio processing function to be applied to every @ref Sav1AudioFrame in
 * an additional thread before it reaches the user. This can be done manually by
 * modifying the @ref Sav1Settings struct directly, but this function provides a
 * convenient way to do so. New frames are not created, but the existing frame passed in
 * can be modified and the @ref Sav1AudioFrame.custom_data property can be used to store
 * additional output data. Include an optional cookie to store necessary information.
 *
 * If `destroy_function` is `NULL`, then no additional steps will be taken when frames are
 * destroyed. If memory is allocated for the @ref Sav1AudioFrame.custom_data property,
 * then a function should be passed for this argument that will free that memory.
 *
 * If `cookie` is `NULL`, then `NULL` will be passed in for the `cookie` argument of the
 * `processing_function` and the `destroy_function`. The cookie can be used to store
 * additional context that is necessary for applying whatever postprocessing is desired.
 *
 * @param[in] settings pointer to a SAV1 settings struct
 * @param[in] processing_function function to modify `Sav1AudioFrame`s and add custom data
 * @param[in] destroy_function function for destroying custom `Sav1AudioFrame` data or
 * `NULL`
 * @param[in] cookie data to be passed in during custom frame processing and destruction
 * or `NULL`
 *
 * @sa Sav1AudioFrame
 * @sa Sav1Settings
 */
SAV1_API void
sav1_settings_use_custom_audio_processing(
    Sav1Settings *settings,
    int (*processing_function)(Sav1AudioFrame *frame, void *cookie),
    void (*destroy_function)(void *, void *), void *cookie);

#endif
