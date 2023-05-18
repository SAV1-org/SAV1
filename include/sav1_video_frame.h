#ifndef SAV1_VIDEO_FRAME_H
#define SAV1_VIDEO_FRAME_H

#include <stdint.h>

#include "sav1.h"

/**
 * @brief Struct to represent one decoded video frame.
 *
 * Contains packed pixel data in the format of the user's choice, as well as metadata to
 * aid in its usage. Ownership of `Sav1VideoFrame`s belongs to SAV1, unless the user calls
 * the @ref sav1_video_frame_take_ownership function, in which case they are responsible
 * for calling the @ref sav1_video_frame_destroy function to destroy it.
 *
 * @sa sav1_video_frame_clone
 * @sa sav1_video_frame_take_ownership
 * @sa sav1_video_frame_destroy
 */
typedef struct Sav1VideoFrame {
    uint8_t *data;       /**< An array of packed pixel data. */
    size_t size;         /**< The length of the pixel data array. */
    ptrdiff_t stride;    /**< The length of a single row of byte data. */
    size_t width;        /**< The width in pixels of the video frame. */
    size_t height;       /**< The height in pixels of the video frame. */
    uint64_t timecode;   /**< The timecode in milliseconds at which the frame should first
                            appear. */
    uint8_t color_depth; /**< The number of bits per color. SAV1 currently only supports
                         8 bits per color. */
    int codec; /**< The video codec this frame was originally stored in. SAV1 currently
                  only supports AV1 video. */
    Sav1PixelFormat pixel_format; /**< The order that the red, green, blue, and alpha
                                     pixels have been packed in. */
    void *custom_data; /**< Optional custom data attached by the user during @ref
                          Sav1Settings.custom_video_frame_processing. */

    int sentinel;           /**< (internal use) */
    int sav1_has_ownership; /**< (internal use) */
} Sav1VideoFrame;

/**
 * @brief Free memory used by a @ref Sav1VideoFrame.
 *
 * Normally, SAV1 maintains ownership over @ref Sav1VideoFrame`s and is responsible for
 * destroying them. If the user has ownership over a frame, either by calling the @ref
 * sav1_video_frame_take_ownership function to claim ownership of an existing frame, or
 * the @ref sav1_video_frame_clone function to create a new clone of an existing frame,
 * then they must call this function to free its memory after the frame is no longer
 * needed.
 *
 * @param[in] context pointer to a created SAV1 context
 * @param[in] frame pointer to a `Sav1VideoFrame` that the user has ownership of
 * @return 0 on success, or < 0 on error
 *
 * @sa sav1_video_frame_take_ownership
 * @sa sav1_video_frame_clone
 */
SAV1_API int
sav1_video_frame_destroy(Sav1Context *context, Sav1VideoFrame *frame);

/**
 * @brief Give the user lifecycle ownership of an existing @ref Sav1VideoFrame.
 *
 * After calling this function, SAV1 will not destroy the frame. The user is responsible
 * for calling the @ref sav1_video_frame_destroy function when they are done with this
 * frame.
 *
 * @param[in] context pointer to a created SAV1 context
 * @param[in] frame pointer to a `Sav1VideoFrame` that SAV1 has ownership of
 * @return 0 on success, or < 0 on error
 *
 * @sa sav1_video_frame_destroy
 */
SAV1_API int
sav1_video_frame_take_ownership(Sav1Context *context, Sav1VideoFrame *frame);

/**
 * @brief Deep clone an existing @ref Sav1VideoFrame.
 *
 * SAV1 will allocate a new video frame and new data within to copy the provided frame.
 * SAV1 will not hold ownership of the newly created frame, so the user is responsible for
 * calling the @ref sav1_video_frame_destroy function when they are done with it.
 *
 * If the user is using @ref Sav1Settings.custom_video_frame_processing, the @ref
 * Sav1VideoFrame.custom_data member will only be shallow-cloned.
 *
 * @param[in] context pointer to a created SAV1 context
 * @param[in] src_frame pointer to a `Sav1VideoFrame` to be cloned
 * @param[out] dst_frame pointer to the new `Sav1VideoFrame`
 * @return 0 on success, or < 0 on error
 *
 * @sa sav1_video_frame_destroy
 */
SAV1_API int
sav1_video_frame_clone(Sav1Context *context, Sav1VideoFrame *src_frame,
                       Sav1VideoFrame **dst_frame);

#endif
