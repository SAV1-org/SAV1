#ifndef SAV1_H
#define SAV1_H

#include <stdint.h>
#include <stddef.h>
#include "common.h"

/**
 * @mainpage SAV1
 *
 * (S)imple (AV1) and Opus playback
 *
 * Library to enable simple and efficient playback of a WEBM file containing AV1 video and
 * Opus audio tracks. Written by Elijah Cirioli, Daniel Wolnick, and Charles Hayden.
 *
 * SAV1 provides a simple interface on top of file parsing, low level AV1 and Opus
 * decoding, and tracking time for playback. SAV1 allows the user to choose their
 * preferred audio frequency / channels, and their preferred video pixelformat, and
 * receive data in those formats. This allows desktop app developers to easily add video
 * playback to their app. For example, a game developer could easily add cutscenes.
 *
 * Currently SAV1 is in beta.
 *
 * The library uses `dav1d` to efficiently decode video, `libopus` to efficiently decode
 * audio-- vendors `libwebm` and `libyuv` for file parsing and color conversion
 * respectively. The decoder and parsing modules are threaded internally so the top level
 * API is non blocking and efficient.
 *
 * Why should I use this instead of another playback library?
 * - FFmpeg: Has great support for all sorts of formats, which makes the full library
 * large (in filesize). Also is primarily a CLI-- code component libavcodec is not easy to
 * use. Since we just support one format, we take up much less filesize, and we have an
 * easy to use API.
 * - OpenCV: Does video decoding well, but not audio. Also has lots of ML related
 * functionality bloating filesize.
 * - pl_mpeg: Just like us, they support only one format (MPEG1 video and MP2 video) and
 * are very lightweight (header only library). However, the codecs we support are cutting
 * edge, so you'll be able to get better looking videos with the same filesize using SAV1.
 *
 * [Check us out our Github repository](https://github.com/SAV1-org/SAV1)
 *
 * [Get in touch with us or report issues on the issue
 * tracker](https://github.com/SAV1-org/SAV1/issues)
 *
 * To get started, you can check out the documentation on this website, and see the Github
 * repo's README for library build instructions. Eventually, we hope to offer prebuilts
 * for download, but for now you need to build from source.
 */

/**
 * @brief Struct to represent one instance of the library.
 *
 * Each instance of the library can only play one video at a time. Multiple instances can
 * be active at the same time.
 *
 * @sa sav1_create_context
 */
typedef struct Sav1Context {
    /** (internal use) Pointer to SAV1's internal state, hidden from the user. */
    void *internal_state;

    /** (interal use) */
    uint8_t is_initialized;
} Sav1Context;

#include "sav1_settings.h"
#include "sav1_video_frame.h"
#include "sav1_audio_frame.h"

/**
 * @brief Initialize SAV1 context
 *
 * @param[in] context pointer to an empty SAV1 context struct
 * @param[in] settings pointer to an initialized SAV1 settings struct
 * @return 0 on success, or < 0 on error
 *
 * @sa sav1_destroy_context
 * @sa sav1_start_playback
 */
SAV1_API int
sav1_create_context(Sav1Context *context, Sav1Settings *settings);

/**
 * @brief Destroy SAV1 Context
 *
 * @param[in] context pointer to a SAV1 context struct
 * @return 0 on success, or < 0 on error
 *
 * @sa sav1_create_context
 */
SAV1_API int
sav1_destroy_context(Sav1Context *context);

/**
 * @brief Get error message from last error
 *
 * Anytime an error occurs in SAV1, the function returns a negative integer
 * and sets a verbose description of the error in the context. Some errors are
 * simply invalid parameters from the function last called, others could be
 * unrecoverable crashes from deep within threading code, unrelated to the last
 * function called.
 *
 * @param[in] context pointer to a SAV1 context struct
 * @return char*
 */
SAV1_API char *
sav1_get_error(Sav1Context *context);

/**
 * @brief Returns a frame to be displayed at the current time, or `NULL`
 *
 * Populates the frame double pointer with a pointer to the @ref Sav1VideoFrame that
 * should be displayed at the current time (timing is controlled by @ref Sav1PlaybackMode
 * and @ref Sav1Settings.playback_speed). If not enough video has been decoded, or SAV1 is
 * actively seeking through the file, the frame pointer may instead be `NULL`.
 *
 * Repeated calls to `sav1_get_video_frame` will return the same video frame, until a new
 * frame has been decoded internally and is ready (time wise) to be played. Ownership of
 * the @ref Sav1VideoFrame is maintained by SAV1, and it will be automatically freed when
 * the next frame is ready. To create a copy of the frame owned by you, use @ref
 * sav1_video_frame_clone.
 *
 * Applications are recommended to use @ref sav1_get_video_frame_ready to determine
 * whether the ready-to-play frame has changed. If the calling application needs to do
 * expensive conversion steps to be able to display the frame, @ref
 * sav1_get_video_frame_ready can be used to run them only when needed.
 *
 * @param[in] context pointer to a created SAV1 context
 * @param[out] frame pointer to a video frame, or `NULL`
 * @return 0 on success, or < 0 on error
 *
 * @sa sav1_get_video_frame_ready
 * @sa sav1_video_frame_clone
 * @sa Sav1VideoFrame
 * @sa sav1_get_audio_frame
 */
SAV1_API int
sav1_get_video_frame(Sav1Context *context, Sav1VideoFrame **frame);

/**
 * @brief Returns a frame to be played at the current time, or `NULL`
 *
 * Populates the frame double pointer with a pointer to the @ref Sav1AudioFrame that
 * should be played at the current time (timing is controlled by @ref Sav1PlaybackMode and
 * @ref Sav1Settings.playback_speed). If not enough audio has been decoded, or SAV1 is
 * actively seeking through the file, the frame pointer may instead be `NULL`.
 *
 * Repeated calls to `sav1_get_audio_frame` will return the same audio frame, until a new
 * frame has been decoded internally and is ready (time wise) to be played. Ownership of
 * the @ref Sav1AudioFrame is maintained by SAV1, and it will be automatically freed when
 * the next frame is ready. To create a copy of the frame owned by you, use @ref
 * sav1_audio_frame_clone.
 *
 * Applications are recommended to use @ref sav1_get_audio_frame_ready to determine
 * whether the ready-to-play frame has changed. If the calling application needs to do
 * expensive conversion steps to be able to play the frame, @ref
 * sav1_get_audio_frame_ready can be used to run them only when needed.
 *
 * @param[in] context pointer to a created SAV1 context
 * @param[out] frame pointer to a audio frame, or `NULL`
 * @return 0 on success, or < 0 on error
 *
 * @sa sav1_get_audio_frame_ready
 * @sa sav1_audio_frame_clone
 * @sa Sav1AudioFrame
 * @sa sav1_get_video_frame
 */
SAV1_API int
sav1_get_audio_frame(Sav1Context *context, Sav1AudioFrame **frame);

/**
 * @brief Queries whether the calling application has a new video frame to display
 *
 * When SAV1 has a new video frame that the calling application should display, it will
 * populate `is_ready` as true. When true, the calling application should call @ref
 * sav1_get_video_frame and display the frame. When false, there are either no frames yet
 * decoded, or it is not yet time to play them (timing is controlled by @ref
 * Sav1PlaybackMode and @ref Sav1Settings.playback_speed).
 *
 * @param[in] context pointer to a created SAV1 context
 * @param[out] is_ready if nonzero, indicates now is the time to pull a new video frame
 * @return 0 on success, or < 0 on error
 *
 * @sa sav1_get_video_frame
 * @sa sav1_get_audio_frame_ready
 */
SAV1_API int
sav1_get_video_frame_ready(Sav1Context *context, int *is_ready);

/**
 * @brief Queries whether the calling application has a new audio frame to play
 *
 * When SAV1 has a new audio frame that the calling application should play, it will
 * populate `is_ready` as true. When true, the calling application should call
 * @ref sav1_get_audio_frame and play the frame. When false, there are either no frames
 * yet decoded, or it is not yet time to play them (timing is controlled by @ref
 * Sav1PlaybackMode and @ref Sav1Settings.playback_speed).
 *
 * @param[in] context pointer to a created SAV1 context
 * @param[out] is_ready if nonzero, indicates now is the time to pull a new audio frame
 * @return 0 on success, or < 0 on error
 *
 * @sa sav1_get_audio_frame
 * @sa sav1_get_video_frame_ready
 */
SAV1_API int
sav1_get_audio_frame_ready(Sav1Context *context, int *is_ready);

/**
 * @brief Starts playback (either initially or to unpause)
 *
 * `sav1_start_playback` signals to SAV1 that the file should begin playing.
 *
 * When a SAV1Context is created, the file begins parsing and decoding, but frames are not
 * output until SAV1 gets a time associated with the start of the video by
 * `sav1_start_playback`.
 *
 * `sav1_stop_playback` / `sav1_start_playback` can be used to pause and resume.
 *
 * If this function is called while SAV1 is seeking or while the file is already playing,
 * an error is returned.
 *
 * @param[in] context pointer to a created SAV1 context
 * @return 0 on success, or < 0 on error
 *
 * @sa sav1_stop_playback
 * @sa sav1_get_playback_time
 * @sa sav1_create_context
 */
SAV1_API int
sav1_start_playback(Sav1Context *context);

/**
 * @brief Stops playback
 *
 * `sav1_stop_playback` signals to SAV1 that the playback should be suspended.
 *
 * `sav1_stop_playback` / `sav1_start_playback` can be used to pause and resume.
 *
 * If this function is called while SAV1 is seeking or while the file is
 * already stopped, an error is returned.
 *
 * @param[in] context pointer to a created SAV1 context
 * @return 0 on success, or < 0 on error
 *
 * @sa sav1_start_playback
 * @sa sav1_get_playback_time
 */
SAV1_API int
sav1_stop_playback(Sav1Context *context);

/**
 * @brief Gets the current playback status
 *
 * @param[in] context pointer to a created SAV1 context
 * @param[out] status 0 when paused or not yet started, 1 when playing
 * @return 0 on success, or < 0 on error
 *
 * @sa sav1_start_playback
 * @sa sav1_stop_playback
 */
SAV1_API int
sav1_get_playback_status(Sav1Context *context, int *status);

/**
 * @brief Gets the current playback time in milliseconds
 *
 * Gets the number of milliseconds into the file that playback has reached.
 * The function will attempt to cap the timecode at the duration of the video
 * if it is available, otherwise the timecode may continue to increase past
 * the end of the video. While paused, the playback time will equal the time
 * that playback had reached when it was initially paused. If playback has
 * not yet begun, the playback time will equal 0.
 *
 * @param[in] context pointer to a created SAV1 context
 * @param[out] timecode_ms the current playback time in milliseconds
 * @return 0 on success, or < 0 on error
 *
 * @sa sav1_start_playback
 * @sa sav1_stop_playback
 */
SAV1_API int
sav1_get_playback_time(Sav1Context *context, uint64_t *timecode_ms);

/**
 * @brief Gets the total duration of the file in milliseconds
 *
 * Gets the number of milliseconds that the file lasts. This is determined
 * through reading an optional field in the webm file format, so the it
 * may not be possible to determine. When the duration is unknown, either
 * because the webm parsing hasn't found it yet or because it isn't listed,
 * then this function will return an error and `duration_ms` will not be set.
 *
 * @param[in] context pointer to a created SAV1 context
 * @param[out] duration_ms the total duration in milliseconds
 * @return 0 on success, or < 0 on error
 *
 * @sa sav1_start_playback
 * @sa sav1_get_playback_time
 */
SAV1_API int
sav1_get_playback_duration(Sav1Context *context, uint64_t *duration_ms);

/**
 * @brief Seeks playback to a different time in the file
 *
 * Restarts the process of decoding audio and video from the point specified
 * by the `timecode_ms`. The function will return after starting this process,
 * but it may take time until the new frames have been located and decoded.
 * The first audio frame returned after seeking will be the first audio frame
 * in the file with a timecode greater than or equal to `timecode_ms`. The
 * first video frame returned after seeking will be the first keyframe in the
 * file with a timecode greater than or equal to `timecode_ms`.
 *
 * While SAV1 is actively seeking, calls to @ref sav1_get_video_frame and
 * @ref sav1_get_audio_frame may return a `NULL` frame. When seeking has finished,
 * playback will remain in the paused or playing state that it was in prior to
 * seeking. The function will return an error if seeking is attempted while SAV1
 * is processing another seek request. Calls to @ref sav1_get_playback_time made
 * while in the middle of seeking will return `timecode_ms`.
 *
 * @param[in] context pointer to a created SAV1 context
 * @param[in] timecode_ms the time to seek to in milliseconds
 * @return 0 on success, or < 0 on error
 *
 * @sa sav1_get_video_frame
 * @sa sav1_get_audio_frame
 * @sa sav1_get_playback_time
 */
SAV1_API int
sav1_seek_playback(Sav1Context *context, uint64_t timecode_ms);

/**
 * @brief Macro (compile time) for SAV1 major version
 */
#define SAV1_MAJOR_VERSION 0
/**
 * @brief Macro (compile time) for SAV1 minor version
 */
#define SAV1_MINOR_VERSION 2
/**
 * @brief Macro (compile time) for SAV1 patch version
 */
#define SAV1_PATCH_VERSION 0

/**
 * @brief Populate out variables with linked SAV1 version
 *
 * @param[out] major populated with SAV1 major version
 * @param[out] minor populated with SAV1 minor version
 * @param[out] patch populated with SAV1 patch version
 *
 * @sa SAV1_MAJOR_VERSION
 * @sa SAV1_MINOR_VERSION
 * @sa SAV1_PATCH_VERSION
 */
SAV1_API void
sav1_get_version(int *major, int *minor, int *patch);

/**
 * @brief Gets dav1d version
 *
 * @returns dav1d version string as self-reported by dav1d
 *
 * @sa sav1_get_opus_version
 */
SAV1_API char *
sav1_get_dav1d_version();

/**
 * @brief Gets opus version
 *
 * @returns opus version string as self-reported by opus
 *
 * @sa sav1_get_dav1d_version
 */
SAV1_API char *
sav1_get_opus_version();

#endif