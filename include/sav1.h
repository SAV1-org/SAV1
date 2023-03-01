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
 * @param context pointer to an empty SAV1 context struct
 * @param settings pointer to an initialized SAV1 settings struct
 * @return 0 on success, or <0 on error
 */
SAV1_API int
sav1_create_context(Sav1Context *context, Sav1Settings *settings);

/**
 * @brief Destroy SAV1 Context
 * 
 * @param context pointer to a SAV1 context struct
 * @return 0 on success, or <0 on error
 */
SAV1_API int
sav1_destroy_context(Sav1Context *context);

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

SAV1_API int
sav1_start_playback(Sav1Context *context);

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

#define SAV1_MAJOR_VERSION 0
#define SAV1_MINOR_VERSION 1
#define SAV1_PATCH_VERSION 0

/**
 * @brief Populate out variables with linked SAV1 version
 * 
 * @param[out] major populated with SAV1 major version
 * @param[out] minor populated with SAV1 minor version
 * @param[out] patch populated with SAV1 patch version
 */
SAV1_API void
sav1_get_version(int *major, int *minor, int *patch);

#endif