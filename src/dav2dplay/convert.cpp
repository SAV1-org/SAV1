#include "convert.h"

using namespace libyuv;

void
convert_yuv_to_rgb_with_identity_matrix(uint8_t *Y_data, ptrdiff_t Y_stride,
                                        uint8_t *U_data, uint8_t *V_data,
                                        ptrdiff_t UV_stride, uint8_t *bgra_data,
                                        ptrdiff_t bgra_stride, Dav1dPixelLayout layout,
                                        int width, int height)
{
    int chroma_sampling_horizontal = 1;
    int chroma_sampling_vertical = 1;
    if (layout != DAV1D_PIXEL_LAYOUT_I444) {
        chroma_sampling_horizontal = 2;
        if (layout == DAV1D_PIXEL_LAYOUT_I420) {
            chroma_sampling_vertical = 2;
        }
    }

    for (int y = 0; y < height; y++) {
        int dest_index = y * bgra_stride;
        for (int x = 0; x < width; x++) {
            int Y_index = y * Y_stride + x;
            int UV_index =
                y / chroma_sampling_vertical * UV_stride + x / chroma_sampling_horizontal;

            if (layout == DAV1D_PIXEL_LAYOUT_I400) {
                // grayscale
                bgra_data[dest_index++] = Y_data[Y_index];
                bgra_data[dest_index++] = Y_data[Y_index];
                bgra_data[dest_index++] = Y_data[Y_index];
                bgra_data[dest_index++] = 255;
            }
            else {
                // color
                bgra_data[dest_index++] = U_data[UV_index];
                bgra_data[dest_index++] = Y_data[Y_index];
                bgra_data[dest_index++] = V_data[UV_index];
                bgra_data[dest_index++] = 255;
            }
        }
    }
}

const struct YuvConstants *
get_matrix_coefficients(Dav1dSequenceHeader *seqhdr)
{
    if (seqhdr->color_range) {
        switch (seqhdr->mtrx) {
            case DAV1D_MC_BT709:
                return &kYuvF709Constants;
            case DAV1D_MC_BT470BG:
            case DAV1D_MC_BT601:
            case DAV1D_MC_UNKNOWN:
                return &kYuvJPEGConstants;
            case DAV1D_MC_BT2020_NCL:
                return &kYuvV2020Constants;
            case DAV1D_MC_CHROMAT_NCL:
                switch (seqhdr->pri) {
                    case DAV1D_COLOR_PRI_BT709:
                    case DAV1D_COLOR_PRI_UNKNOWN:
                        return &kYuvF709Constants;
                    case DAV1D_COLOR_PRI_BT470BG:
                    case DAV1D_COLOR_PRI_BT601:
                        return &kYuvJPEGConstants;
                    case DAV1D_COLOR_PRI_BT2020:
                        return &kYuvV2020Constants;
                    default:
                        return NULL;
                }
            default:
                return NULL;
        }
    }
    else {
        switch (seqhdr->mtrx) {
            case DAV1D_MC_BT709:
                return &kYuvH709Constants;
            case DAV1D_MC_BT470BG:
            case DAV1D_MC_BT601:
            case DAV1D_MC_UNKNOWN:
                return &kYuvI601Constants;
            case DAV1D_MC_BT2020_NCL:
                return &kYuv2020Constants;
            case DAV1D_MC_CHROMAT_NCL:
                switch (seqhdr->pri) {
                    case DAV1D_COLOR_PRI_BT709:
                    case DAV1D_COLOR_PRI_UNKNOWN:
                        return &kYuvH709Constants;
                    case DAV1D_COLOR_PRI_BT470BG:
                    case DAV1D_COLOR_PRI_BT601:
                        return &kYuvI601Constants;
                    case DAV1D_COLOR_PRI_BT2020:
                        return &kYuv2020Constants;
                    default:
                        return NULL;
                }
            default:
                return NULL;
        }
    }
    return NULL;
}

void
convert(Dav1dPicture *picture, uint8_t *bgra_data, ptrdiff_t bgra_stride)
{
    Dav1dPictureParameters picparam = picture->p;

    int width = picparam.w;
    int height = picparam.h;

    Dav1dSequenceHeader *seqhdr = picture->seq_hdr;

    uint8_t *Y_data = (uint8_t *)picture->data[0];
    uint8_t *U_data = (uint8_t *)picture->data[1];
    uint8_t *V_data = (uint8_t *)picture->data[2];
    ptrdiff_t Y_stride = picture->stride[0];
    ptrdiff_t UV_stride = picture->stride[1];

    if (seqhdr->mtrx == DAV1D_MC_IDENTITY) {
        // this function takes way too many arguments
        convert_yuv_to_rgb_with_identity_matrix(Y_data, Y_stride, U_data, V_data,
                                                UV_stride, bgra_data, bgra_stride,
                                                seqhdr->layout, width, height);
    }
    else {
        const struct YuvConstants *matrixYUV = get_matrix_coefficients(seqhdr);
        assert(matrixYUV != NULL);

        if (seqhdr->layout == DAV1D_PIXEL_LAYOUT_I420) {
            I420ToARGBMatrix(Y_data, Y_stride, U_data, UV_stride, V_data, UV_stride,
                             bgra_data, bgra_stride, matrixYUV, width, height);
        }
        if (seqhdr->layout == DAV1D_PIXEL_LAYOUT_I400) {
            I400ToARGBMatrix(Y_data, Y_stride, bgra_data, bgra_stride, matrixYUV, width,
                             height);
        }
        if (seqhdr->layout == DAV1D_PIXEL_LAYOUT_I422) {
            I422ToARGBMatrix(Y_data, Y_stride, U_data, UV_stride, V_data, UV_stride,
                             bgra_data, bgra_stride, matrixYUV, width, height);
        }
        if (seqhdr->layout == DAV1D_PIXEL_LAYOUT_I444) {
            I444ToARGBMatrix(Y_data, Y_stride, U_data, UV_stride, V_data, UV_stride,
                             bgra_data, bgra_stride, matrixYUV, width, height);
        }
    }
    dav1d_picture_unref(picture);
}
