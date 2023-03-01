#ifndef SAV1_H
#define SAV1_H

#include <stdint.h>
#include <stddef.h>
#include "common.h"

typedef struct Sav1Context {
    void *internal_state;
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

SAV1_API int
sav1_get_video_frame(Sav1Context *context, Sav1VideoFrame **frame);

SAV1_API int
sav1_get_audio_frame(Sav1Context *context, Sav1AudioFrame **frame);

SAV1_API int
sav1_get_video_frame_ready(Sav1Context *context, int *is_ready);

SAV1_API int
sav1_get_audio_frame_ready(Sav1Context *context, int *is_ready);

/**
 * @brief Starts playback (either initially or to unpause)
 * 
 * sav1_start_playback signals to SAV1 that the file should begin playing.
 * 
 * When a SAV1Context is created, the file begins parsing and decoding,
 * but frames are not output until SAV1 gets a time associated with the start
 * of the video by sav1_start_playback.
 * 
 * sav1_stop_playback / sav1_start_playback can be used to pause and resume.
 * 
 * If this function is called while SAV1 is seeking or while the file is
 * already playing, an error is returned.
 * 
 * @param[in] context pointer to a created SAV1 context
 * @return 0 on success, or < 0 on error
 * 
 * @sa sav1_stop_playback
 * @sa sav1_create_context
 */
SAV1_API int
sav1_start_playback(Sav1Context *context);

/**
 * @brief Stops playback
 * 
 * sav1_stop_playback signals to SAV1 that the playback should be suspended.
 *
 * sav1_stop_playback / sav1_start_playback can be used to pause and resume.
 * 
 * If this function is called while SAV1 is seeking or while the file is
 * already stopped, an error is returned.
 * 
 * @param[in] context pointer to a created SAV1 context
 * @return 0 on success, or < 0 on error
 * 
 * @sa sav1_start_playback
 */
SAV1_API int
sav1_stop_playback(Sav1Context *context);

SAV1_API int
sav1_get_playback_status(Sav1Context *context, int *status);

SAV1_API int
sav1_get_playback_time(Sav1Context *context, uint64_t *timecode_ms);

SAV1_API int
sav1_get_playback_duration(Sav1Context *context, uint64_t *duration_ms);

SAV1_API int
sav1_seek_playback(Sav1Context *context, uint64_t timecode_ms);

/**
 * @brief Macro (compile time) for SAV1 major version
 */
#define SAV1_MAJOR_VERSION 0
/**
 * @brief Macro (compile time) for SAV1 minor version
 */
#define SAV1_MINOR_VERSION 1
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

#endif