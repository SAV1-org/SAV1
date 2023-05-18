#ifndef SAV1_AUDIO_FRAME_H
#define SAV1_AUDIO_FRAME_H

#include <stdint.h>

#include "sav1.h"

/**
 * @brief Struct to represent one decoded audio frame.
 *
 * Contains raw audio data, as well as metadata to aid in its usage. Ownership of
 * `Sav1AudioFrame`s belongs to SAV1, unless the user calls the @ref
 * sav1_audio_frame_take_ownership function, in which case they are responsible for
 * calling the @ref sav1_audio_frame_destroy function to destroy it.
 *
 * @sa sav1_audio_frame_clone
 * @sa sav1_audio_frame_take_ownership
 * @sa sav1_audio_frame_destroy
 */
typedef struct Sav1AudioFrame {
    uint8_t *data;     /**< An array of PCM audio samples. */
    size_t size;       /**< The length of the data array. */
    uint64_t timecode; /**< The timecode in milliseconds at which the frame should begin
                          playing. */
    uint64_t duration; /**< The duration in milliseconds of the audio frame. */
    Sav1AudioFrequency frequency; /**< The sampling frequency of the audio frame. */
    Sav1AudioChannel channels;    /**< The number of audio channels. */
    int codec; /**< The audio codec this frame was originally stored in. SAV1 currently
only supports Opus audio. */
    void *custom_data; /**< Optional custom data attached by the user during @ref
                          Sav1Settings.custom_audio_frame_processing. */

    int sentinel;           /**< (internal use) */
    int sav1_has_ownership; /**< (internal use) */
} Sav1AudioFrame;

/**
 * @brief Free memory used by a @ref Sav1AudioFrame.
 *
 * Normally, SAV1 maintains ownership over @ref Sav1AudioFrame`s and is responsible for
 * destroying them. If the user has ownership over a frame, either by calling the @ref
 * sav1_audio_frame_take_ownership function to claim ownership of an existing frame, or
 * the @ref sav1_audio_frame_clone function to create a new clone of an existing frame,
 * then they must call this function to free its memory after the frame is no longer
 * needed.
 *
 * @param[in] context pointer to a created SAV1 context
 * @param[in] frame pointer to a `Sav1AudioFrame` that the user has ownership of
 * @return 0 on success, or < 0 on error
 *
 * @sa sav1_audio_frame_take_ownership
 * @sa sav1_audio_frame_clone
 */
SAV1_API int
sav1_audio_frame_destroy(Sav1Context *context, Sav1AudioFrame *frame);

/**
 * @brief Give the user lifecycle ownership of an existing @ref Sav1AudioFrame.
 *
 * After calling this function, SAV1 will not destroy the frame. The user is responsible
 * for calling the @ref sav1_audio_frame_destroy function when they are done with this
 * frame.
 *
 * @param[in] context pointer to a created SAV1 context
 * @param[in] frame pointer to a `Sav1AudioFrame` that SAV1 has ownership of
 * @return 0 on success, or < 0 on error
 *
 * @sa sav1_audio_frame_destroy
 */
SAV1_API int
sav1_audio_frame_take_ownership(Sav1Context *context, Sav1AudioFrame *frame);

/**
 * @brief Deep clone an existing @ref Sav1AudioFrame.
 *
 * SAV1 will allocate a new audio frame and new data within to copy the provided frame.
 * SAV1 will not hold ownership of the newly created frame, so the user is responsible for
 * calling the @ref sav1_audio_frame_destroy function when they are done with it.
 *
 * If the user is using @ref Sav1Settings.custom_audio_frame_processing, the @ref
 * Sav1AudioFrame.custom_data member will only be shallow-cloned.
 *
 * @param[in] context pointer to a created SAV1 context
 * @param[in] src_frame pointer to a `Sav1AudioFrame` to be cloned
 * @param[out] dst_frame pointer to the new `Sav1AudioFrame`
 * @return 0 on success, or < 0 on error
 *
 * @sa sav1_audio_frame_destroy
 */
SAV1_API int
sav1_audio_frame_clone(Sav1Context *context, Sav1AudioFrame *src_frame,
                       Sav1AudioFrame **dst_frame);

#endif
