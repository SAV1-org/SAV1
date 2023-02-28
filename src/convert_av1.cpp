#include <cassert>
#include <cstdio>

/* If compiling the entire project in C++, the extern C should not be used */
#ifndef CPPTHROUGHANDTHROUGH
extern "C" {
#endif
#include <dav1d/dav1d.h>

#include "convert_av1.h"
#include "sav1_video_frame.h"
#include "sav1_settings.h"
#include "sav1_internal.h"
#ifndef CPPTHROUGHANDTHROUGH
}
#endif

using namespace libyuv;

void
convert_yuv_to_rgba_with_identity_matrix(uint8_t *Y_data, ptrdiff_t Y_stride,
                                         uint8_t *U_data, uint8_t *V_data,
                                         ptrdiff_t UV_stride,
                                         Sav1VideoFrame *output_frame,
                                         Dav1dPixelLayout layout,
                                         Sav1PixelFormat desired_pixel_format)
{
    // determine chroma sampling in both axes
    size_t chroma_sampling_horizontal = 1;
    size_t chroma_sampling_vertical = 1;
    if (layout != DAV1D_PIXEL_LAYOUT_I444) {
        chroma_sampling_horizontal = 2;
        if (layout == DAV1D_PIXEL_LAYOUT_I420) {
            chroma_sampling_vertical = 2;
        }
    }

    // where do A, R, G, and B go within one pixel
    uint8_t byte_offsets_lookup[6][4] = {
        [SAV1_PIXEL_FORMAT_RGBA] = {3, 0, 1, 2}, [SAV1_PIXEL_FORMAT_ARGB] = {0, 1, 2, 3},
        [SAV1_PIXEL_FORMAT_BGRA] = {3, 2, 1, 0}, [SAV1_PIXEL_FORMAT_ABGR] = {0, 3, 2, 1},
        [SAV1_PIXEL_FORMAT_RGB] = {0, 0, 1, 2},  [SAV1_PIXEL_FORMAT_BGR] = {0, 2, 1, 0}};
    uint8_t *byte_offsets = byte_offsets_lookup[desired_pixel_format];

    // how many bytes does one pixel take up
    uint8_t pixel_size = desired_pixel_format == SAV1_PIXEL_FORMAT_RGB ||
                                 desired_pixel_format == SAV1_PIXEL_FORMAT_BGR
                             ? 3
                             : 4;

    // iterate over all pixels
    for (size_t y = 0; y < output_frame->height; y++) {
        for (int x = 0; x < output_frame->width; x++) {
            // get the indices for the YUV data
            size_t Y_index = y * Y_stride + x;
            size_t UV_index =
                y / chroma_sampling_vertical * UV_stride + x / chroma_sampling_horizontal;

            // get the index for the output data
            size_t dest_index = y * output_frame->stride + x * pixel_size;

            if (layout == DAV1D_PIXEL_LAYOUT_I400) {
                // grayscale
                output_frame->data[dest_index + byte_offsets[0]] = 255;  // alpha
                output_frame->data[dest_index + byte_offsets[1]] =
                    Y_data[Y_index];  // red
                output_frame->data[dest_index + byte_offsets[2]] =
                    Y_data[Y_index];  // green
                output_frame->data[dest_index + byte_offsets[3]] =
                    Y_data[Y_index];  // blue
            }
            else {
                // color
                output_frame->data[dest_index + byte_offsets[0]] = 255;  // alpha
                output_frame->data[dest_index + byte_offsets[1]] =
                    V_data[UV_index];  // red
                output_frame->data[dest_index + byte_offsets[2]] =
                    Y_data[Y_index];  // green
                output_frame->data[dest_index + byte_offsets[3]] =
                    U_data[UV_index];  // blue
            }
        }
    }
}

void
convert_yuv_to_packed(uint8_t *Y_data, ptrdiff_t Y_stride, uint8_t *U_data,
                      uint8_t *V_data, ptrdiff_t UV_stride, Sav1VideoFrame *output_frame,
                      Dav1dPixelLayout layout)
{
    // determine chroma sampling in both axes
    size_t chroma_sampling_horizontal = 4;
    size_t chroma_sampling_vertical = 4;
    if (layout != DAV1D_PIXEL_LAYOUT_I444) {
        chroma_sampling_horizontal = 2;
        if (layout == DAV1D_PIXEL_LAYOUT_I420) {
            chroma_sampling_vertical = 2;
        }
    }

    // iterate over all pixels
    for (size_t y = 0; y < output_frame->height; y++) {
        for (int x = 0; x < output_frame->width; x++) {
            // get the indices for the YUV data
            size_t Y_index = y * Y_stride + x;
            size_t UV_index =
                y / chroma_sampling_vertical * UV_stride + x / chroma_sampling_horizontal;

            // get the index for the output data
            size_t dest_index = y * output_frame->stride + x * 2;
            size_t Y_offset =
                output_frame->pixel_format == SAV1_PIXEL_FORMAT_UYVY ? 1 : 0;
            size_t UV_offset = 1 - Y_offset;

            output_frame->data[dest_index + Y_offset] = Y_data[Y_index];
            if (layout == DAV1D_PIXEL_LAYOUT_I400) {
                // grayscale
                output_frame->data[dest_index + UV_offset] = 0;
            }
            else {
                // alternate using U and V data for each pixel
                if ((y * output_frame->width + x) % 2 ^
                    (output_frame->pixel_format == SAV1_PIXEL_FORMAT_YVYU)) {
                    output_frame->data[dest_index + UV_offset] = V_data[UV_index];
                }
                else {
                    output_frame->data[dest_index + UV_offset] = U_data[UV_index];
                }
            }
        }
    }
}

int
RGB24ToBGR24(const uint8_t *src_rgb24, int src_stride_rgb24, uint8_t *dst_bgr24,
             int dst_stride_bgr24, int width, int height)
{
    // iterate over all pixels
    for (size_t y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // swap the R and B bytes
            size_t i = y * src_stride_rgb24 + x * 3;
            uint8_t temp = src_rgb24[i];
            dst_bgr24[i] = src_rgb24[i + 2];
            dst_bgr24[i + 2] = temp;
        }
    }
    return 0;
}

const struct YuvConstants *
get_matrix_coefficients(Dav1dSequenceHeader *seq_hdr)
{
    if (seq_hdr->color_range) {
        switch (seq_hdr->mtrx) {
            case DAV1D_MC_BT709:
                return &kYuvF709Constants;
            case DAV1D_MC_BT470BG:
            case DAV1D_MC_BT601:
            case DAV1D_MC_UNKNOWN:
                return &kYuvJPEGConstants;
            case DAV1D_MC_BT2020_NCL:
                return &kYuvV2020Constants;
            case DAV1D_MC_CHROMAT_NCL:
                switch (seq_hdr->pri) {
                    case DAV1D_COLOR_PRI_BT709:
                    case DAV1D_COLOR_PRI_UNKNOWN:
                        return &kYuvF709Constants;
                    case DAV1D_COLOR_PRI_BT470BG:
                    case DAV1D_COLOR_PRI_BT601:
                        return &kYuvJPEGConstants;
                    case DAV1D_COLOR_PRI_BT2020:
                        return &kYuvV2020Constants;
                    default:
                        return &kYuvJPEGConstants;
                }
            default:
                return &kYuvJPEGConstants;
        }
    }
    else {
        switch (seq_hdr->mtrx) {
            case DAV1D_MC_BT709:
                return &kYuvH709Constants;
            case DAV1D_MC_BT470BG:
            case DAV1D_MC_BT601:
            case DAV1D_MC_UNKNOWN:
                return &kYuvI601Constants;
            case DAV1D_MC_BT2020_NCL:
                return &kYuv2020Constants;
            case DAV1D_MC_CHROMAT_NCL:
                switch (seq_hdr->pri) {
                    case DAV1D_COLOR_PRI_BT709:
                    case DAV1D_COLOR_PRI_UNKNOWN:
                        return &kYuvH709Constants;
                    case DAV1D_COLOR_PRI_BT470BG:
                    case DAV1D_COLOR_PRI_BT601:
                        return &kYuvI601Constants;
                    case DAV1D_COLOR_PRI_BT2020:
                        return &kYuv2020Constants;
                    default:
                        return &kYuvI601Constants;
                }
            default:
                return &kYuvI601Constants;
        }
    }
    return &kYuvJPEGConstants;
}

void
convert_dav1d_picture(Dav1dPicture *picture, Sav1VideoFrame *output_frame)
{
    int width = picture->p.w;
    int height = picture->p.h;
    output_frame->width = picture->p.w;
    output_frame->height = picture->p.h;

    Sav1PixelFormat desired_pixel_format = output_frame->pixel_format;

    uint8_t *Y_data = (uint8_t *)picture->data[0];
    uint8_t *U_data = (uint8_t *)picture->data[1];
    uint8_t *V_data = (uint8_t *)picture->data[2];
    ptrdiff_t Y_stride = picture->stride[0];
    ptrdiff_t UV_stride = picture->stride[1];

    Dav1dSequenceHeader *seq_hdr = picture->seq_hdr;

    if (desired_pixel_format == SAV1_PIXEL_FORMAT_YUY2 ||
        desired_pixel_format == SAV1_PIXEL_FORMAT_UYVY ||
        desired_pixel_format == SAV1_PIXEL_FORMAT_YVYU) {
        // YUV variations

        if (seq_hdr->mtrx == DAV1D_MC_IDENTITY) {
            // data is actually GBR so it needs to be converted into YUV

            // allocate twice as much memory as we'll eventually need
            output_frame->stride = 4 * output_frame->width;
            output_frame->data = (uint8_t *)malloc(
                output_frame->size * sizeof(uint8_t));  // TODO: error check this malloc

            // first pack as BGRA
            convert_yuv_to_rgba_with_identity_matrix(
                Y_data, Y_stride, U_data, V_data, UV_stride, output_frame,
                seq_hdr->layout, SAV1_PIXEL_FORMAT_BGRA);

            // convert BGRA to what we need
            if (desired_pixel_format == SAV1_PIXEL_FORMAT_YVYU) {
                ARGBToUYVY(output_frame->data, output_frame->stride, output_frame->data,
                           output_frame->stride, width, height);
            }
            else {
                ARGBToYUY2(output_frame->data, output_frame->stride, output_frame->data,
                           output_frame->stride, width, height);

                if (desired_pixel_format == SAV1_PIXEL_FORMAT_UYVY) {
                    // no conversion function exists for this so U and V must be switched
                    size_t start_x = 0;
                    for (size_t y = 0; y < output_frame->height; y++) {
                        for (int x = start_x; x < output_frame->width; x += 2) {
                            size_t i = y * output_frame->stride + x * 2;
                            uint8_t temp = output_frame->data[i];
                            size_t next_i = i + 2;
                            start_x = 0;
                            if (x == output_frame->width - 1) {
                                start_x = 1;
                                next_i = (y + 1) * output_frame->stride;
                            }
                            output_frame->data[i] = output_frame->data[i + 2];
                            output_frame->data[i + 2] = temp;
                        }
                    }
                }
            }
        }
        else {
            // allocate pixel buffer
            output_frame->stride = 2 * output_frame->width;
            output_frame->size = output_frame->stride * output_frame->height;
            output_frame->data = (uint8_t *)malloc(
                output_frame->size * sizeof(uint8_t));  // TODO: error check this malloc

            // check for cases where libYUV functions already exist (most common)
            if (desired_pixel_format == SAV1_PIXEL_FORMAT_YUY2 &&
                seq_hdr->layout == DAV1D_PIXEL_LAYOUT_I420) {
                I420ToYUY2(Y_data, Y_stride, U_data, UV_stride, V_data, UV_stride,
                           output_frame->data, output_frame->stride, width, height);
            }
            else if (desired_pixel_format == SAV1_PIXEL_FORMAT_YVYU &&
                     seq_hdr->layout == DAV1D_PIXEL_LAYOUT_I420) {
                I420ToUYVY(Y_data, Y_stride, U_data, UV_stride, V_data, UV_stride,
                           output_frame->data, output_frame->stride, width, height);
            }
            else if (desired_pixel_format == SAV1_PIXEL_FORMAT_YVYU &&
                     seq_hdr->layout == DAV1D_PIXEL_LAYOUT_I422) {
                I422ToUYVY(Y_data, Y_stride, U_data, UV_stride, V_data, UV_stride,
                           output_frame->data, output_frame->stride, width, height);
            }
            else {
                // no function already exists so use our own
                convert_yuv_to_packed(Y_data, Y_stride, U_data, V_data, UV_stride,
                                      output_frame, seq_hdr->layout);
            }
        }
    }
    else {
        // RGBA variations
        if (desired_pixel_format == SAV1_PIXEL_FORMAT_RGB ||
            desired_pixel_format == SAV1_PIXEL_FORMAT_BGR) {
            // 3 bytes per pixel (no alpha)
            output_frame->stride = 3 * output_frame->width;
        }
        else {
            // 4 bytes per pixel
            output_frame->stride = 4 * output_frame->width;
        }

        // allocate pixel buffer
        output_frame->size = output_frame->stride * output_frame->height;
        output_frame->data = (uint8_t *)malloc(
            output_frame->size * sizeof(uint8_t));  // TODO: error check this malloc
        if (seq_hdr->mtrx == DAV1D_MC_IDENTITY) {
            convert_yuv_to_rgba_with_identity_matrix(
                Y_data, Y_stride, U_data, V_data, UV_stride, output_frame,
                seq_hdr->layout, desired_pixel_format);
        }
        else {
            // get the appropriate matrix coefficients (or close enough)
            const struct YuvConstants *matrix_coefficients =
                get_matrix_coefficients(seq_hdr);

            // define libYUV type for functions that convert between RGBA variations
            typedef int (*RgbConversionFunction)(const uint8_t *, int, uint8_t *, int,
                                                 int, int);

            // map the SAV1 pixel format enum to indices manually just to be safe
            size_t output_pixel_layout_lookup[] = {
                [SAV1_PIXEL_FORMAT_RGBA] = 0, [SAV1_PIXEL_FORMAT_ARGB] = 1,
                [SAV1_PIXEL_FORMAT_BGRA] = 2, [SAV1_PIXEL_FORMAT_ABGR] = 3,
                [SAV1_PIXEL_FORMAT_RGB] = 4,  [SAV1_PIXEL_FORMAT_BGR] = 5};
            size_t output_pixel_layout_index =
                output_pixel_layout_lookup[desired_pixel_format];

            if (seq_hdr->layout == DAV1D_PIXEL_LAYOUT_I400) {
                // grayscale

                // only one libYUV function exists for this
                I400ToARGBMatrix(Y_data, Y_stride, output_frame->data,
                                 output_frame->stride, matrix_coefficients, width,
                                 height);

                RgbConversionFunction rgb_conversion_function_lookup[6] = {
                    ARGBToABGR, ARGBToBGRA,  nullptr,
                    ARGBToRGBA, ARGBToRGB24, ARGBToRGB24};

                // convert to the final byte order if necessary
                RgbConversionFunction rgb_conversion_function =
                    rgb_conversion_function_lookup[output_pixel_layout_index];
                if (rgb_conversion_function) {
                    rgb_conversion_function(output_frame->data, output_frame->stride,
                                            output_frame->data, output_frame->stride,
                                            width, height);
                }
                if (desired_pixel_format == SAV1_PIXEL_FORMAT_RGB) {
                    RGB24ToBGR24(output_frame->data, output_frame->stride,
                                 output_frame->data, output_frame->stride, width, height);
                }
            }
            else {
                // define libYUV type for functions that convert YUV to RGBA variations
                typedef int (*YuvConversionFunction)(
                    const uint8_t *, int, const uint8_t *, int, const uint8_t *, int,
                    uint8_t *, int, const struct YuvConstants *, int, int);

                // which libYUV function to use for YUV -> RGB conversion
                YuvConversionFunction yuv_conversion_function_lookup[6][3] = {
                    {
                        I420ToARGBMatrix,
                        I422ToARGBMatrix,
                        I444ToARGBMatrix,
                    },
                    {
                        I420ToARGBMatrix,
                        I422ToARGBMatrix,
                        I444ToARGBMatrix,
                    },
                    {
                        I420ToARGBMatrix,
                        I422ToARGBMatrix,
                        I444ToARGBMatrix,
                    },
                    {
                        I420ToRGBAMatrix,
                        I422ToRGBAMatrix,
                        I444ToARGBMatrix,
                    },
                    {I420ToRGB24Matrix, I422ToRGB24Matrix, I444ToRGB24Matrix},
                    {I420ToRGB24Matrix, I422ToRGB24Matrix, I444ToRGB24Matrix}};

                size_t input_pixel_layout_lookup[] = {[DAV1D_PIXEL_LAYOUT_I400] = 0,
                                                      [DAV1D_PIXEL_LAYOUT_I420] = 0,
                                                      [DAV1D_PIXEL_LAYOUT_I422] = 1,
                                                      [DAV1D_PIXEL_LAYOUT_I444] = 2};
                size_t input_pixel_layout_index =
                    input_pixel_layout_lookup[seq_hdr->layout];

                // convert from YUV to some form of RGB
                YuvConversionFunction yuv_conversion_function =
                    yuv_conversion_function_lookup[output_pixel_layout_index]
                                                  [input_pixel_layout_index];
                yuv_conversion_function(Y_data, Y_stride, U_data, UV_stride, V_data,
                                        UV_stride, output_frame->data,
                                        output_frame->stride, matrix_coefficients, width,
                                        height);

                // which libYUV function to use for RGB -> RGB conversion
                RgbConversionFunction rgb_conversion_function_lookup[6] = {
                    ARGBToABGR, ARGBToBGRA, nullptr, nullptr, RGB24ToBGR24, nullptr};
                RgbConversionFunction rgb_conversion_function =
                    rgb_conversion_function_lookup[output_pixel_layout_index];

                if (output_pixel_layout_index == 3 && input_pixel_layout_index == 2) {
                    // special case because no I444ToRGBAMatrix function exists
                    rgb_conversion_function = ARGBToRGBA;
                }
                // convert to the final byte order if necessary
                if (rgb_conversion_function) {
                    rgb_conversion_function(output_frame->data, output_frame->stride,
                                            output_frame->data, output_frame->stride,
                                            width, height);
                }
            }
        }
    }
}

void
convert_av1_init(ConvertAv1Context **context, Sav1InternalContext *ctx,
                 Sav1ThreadQueue *input_queue, Sav1ThreadQueue *output_queue)
{
    if (((*context) = (ConvertAv1Context *)malloc(sizeof(ConvertAv1Context))) == NULL) {
        sav1_set_error(ctx, "malloc() failed in convert_av1_init()");
        sav1_set_critical_error_flag(ctx);
        return;
    }

    (*context)->input_queue = input_queue;
    (*context)->output_queue = output_queue;
    (*context)->desired_pixel_format = ctx->settings->desired_pixel_format;
    (*context)->ctx = ctx;
}

void
convert_av1_destroy(ConvertAv1Context *context)
{
    free(context);
}

int
convert_av1_start(void *context)
{
    ConvertAv1Context *convert_context = (ConvertAv1Context *)context;
    thread_atomic_int_store(&(convert_context->do_convert), 1);

    while (thread_atomic_int_load(&(convert_context->do_convert))) {
        // pull a Dav1dPicture from the input queue
        Dav1dPicture *dav1d_pic =
            (Dav1dPicture *)sav1_thread_queue_pop(convert_context->input_queue);
        if (dav1d_pic == NULL) {
            sav1_thread_queue_push(convert_context->output_queue, NULL);
            break;
        }

        // make sure the picture is 8 bits per color
        if (dav1d_pic->p.bpc != 8) {
            // TODO: proper logging here
            printf("ERROR: SAV1 only currently supports 8 bits per color\n");
            sav1_thread_queue_push(convert_context->output_queue, NULL);
            break;
        }

        // setup the output frame
        Sav1VideoFrame *output_frame;
        if ((output_frame = (Sav1VideoFrame *)malloc(sizeof(Sav1VideoFrame))) == NULL) {
            sav1_set_error(convert_context->ctx,
                           "malloc() failed in convert_av1_start()");
            sav1_set_critical_error_flag(convert_context->ctx);
            return -1;
        }
        output_frame->codec = SAV1_CODEC_AV1;
        output_frame->pixel_format = convert_context->desired_pixel_format;
        output_frame->color_depth = 8;
        output_frame->timecode = dav1d_pic->m.timestamp;
        output_frame->sentinel = dav1d_pic->m.user_data.data == NULL ? 0 : 1;

        // convert the color space
        convert_dav1d_picture(dav1d_pic, output_frame);

        sav1_thread_queue_push(convert_context->output_queue, output_frame);

        dav1d_picture_unref(dav1d_pic);
    }

    return 0;
}

void
convert_av1_stop(ConvertAv1Context *context)
{
    thread_atomic_int_store(&(context->do_convert), 0);

    // add a fake entry to the input queue if necessary
    if (sav1_thread_queue_get_size(context->input_queue) == 0) {
        sav1_thread_queue_push_timeout(context->input_queue, NULL);
    }

    // drain the output queue
    convert_av1_drain_output_queue(context);
}

void
convert_av1_drain_output_queue(ConvertAv1Context *context)
{
    while (1) {
        Sav1VideoFrame *frame =
            (Sav1VideoFrame *)sav1_thread_queue_pop_timeout(context->output_queue);
        if (frame == NULL) {
            break;
        }

        sav1_video_frame_destroy(context->ctx->context, frame);
    }
}
