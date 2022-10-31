#include <iomanip>
#include <iostream>
#include <fstream>
#include <cassert>
#include <string.h>

#include <webm/callback.h>
#include <webm/file_reader.h>
#include <webm/status.h>
#include <webm/webm_parser.h>

#include <libyuv.h>

#include <dav1d/dav1d.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

using namespace webm;

void
dealloc_buffer(const std::uint8_t *data, void *cookie)
{
    delete[] data;
}

const char *
str_matrix_coefficient(int constant)
{
    const char *val;
    switch (constant) {
        case DAV1D_MC_IDENTITY:
            val = "DAV1D_MC_IDENTITY";
            break;
        case DAV1D_MC_BT709:
            val = "DAV1D_MC_BT709";
            break;
        case DAV1D_MC_UNKNOWN:
            val = "DAV1D_MC_UNKNOWN";
            break;
        case DAV1D_MC_FCC:
            val = "DAV1D_MC_FCC";
            break;
        case DAV1D_MC_BT470BG:
            val = "DAV1D_MC_BT470BG";
            break;
        case DAV1D_MC_BT601:
            val = "DAV1D_MC_BT601";
            break;
        case DAV1D_MC_SMPTE240:
            val = "DAV1D_MC_SMPTE240";
            break;
        case DAV1D_MC_SMPTE_YCGCO:
            val = "DAV1D_MC_SMPTE_YCGCO";
            break;
        case DAV1D_MC_BT2020_NCL:
            val = "DAV1D_MC_BT2020_NCL";
            break;
        case DAV1D_MC_BT2020_CL:
            val = "DAV1D_MC_BT2020_CL";
            break;
        case DAV1D_MC_SMPTE2085:
            val = "DAV1D_MC_SMPTE2085";
            break;
        case DAV1D_MC_CHROMAT_NCL:
            val = "DAV1D_MC_CHROMAT_NCL";
            break;
        case DAV1D_MC_CHROMAT_CL:
            val = "DAV1D_MC_CHROMAT_CL";
            break;
        case DAV1D_MC_ICTCP:
            val = "DAV1D_MC_ICTCP";
            break;
    }
    return val;
}

const char *
str_pixel_layout(int constant)
{
    const char *val;
    switch (constant) {
        case DAV1D_PIXEL_LAYOUT_I420:
            val = "DAV1D_PIXEL_LAYOUT_I420";
            break;
        case DAV1D_PIXEL_LAYOUT_I400:
            val = "DAV1D_PIXEL_LAYOUT_I400";
            break;
        case DAV1D_PIXEL_LAYOUT_I422:
            val = "DAV1D_PIXEL_LAYOUT_I422";
            break;
        case DAV1D_PIXEL_LAYOUT_I444:
            val = "DAV1D_PIXEL_LAYOUT_I444";
            break;
    }
    return val;
}

void
poll_events()
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        continue;
    }
}

class Av1Callback : public Callback {
   public:
    void
    setup(Dav1dContext *context, SDL_Window *window)
    {
        // this should be moved to the constructor
        this->av1_track_number = -1;
        this->opus_track_number = -1;
        this->context = context;
        this->window = window;
    }

    bool
    found_av1_track()
    {
        return this->av1_track_number != -1;
    }

    bool
    found_opus_track()
    {
        return this->opus_track_number != -1;
    }

    void
    decode_opus(std::uint8_t *buffer, std::uint64_t num_bytes)
    {
        // DR_OPUS who?
        delete[] buffer;
    }

    void
    drain_dav1d()
    {
        int status;
        Dav1dPicture pic = {0};

        status = dav1d_get_picture(this->context, &pic);
        // try one more time if dav1d tells us to (we usually have to)
        if (status == DAV1D_ERR(EAGAIN)) {
            status = dav1d_get_picture(this->context, &pic);
        }
        if (status == 0) {
            std::cout << "got picture from dav1d" << std::endl;
            this->handle_picture(&pic);
        }
        else {
            std::cout << "failed to get picture: " << strerror(-status)
                      << std::endl;
        }
    }

    const struct libyuv::YuvConstants *
    get_matrix_coefficients(Dav1dSequenceHeader *seqhdr)
    {
        if (seqhdr->color_range) {
            switch (seqhdr->mtrx) {
                case DAV1D_MC_BT709:
                    return &libyuv::kYuvF709Constants;
                case DAV1D_MC_BT470BG:
                case DAV1D_MC_BT601:
                case DAV1D_MC_UNKNOWN:
                    return &libyuv::kYuvJPEGConstants;
                case DAV1D_MC_BT2020_NCL:
                    return &libyuv::kYuvV2020Constants;
                case DAV1D_MC_CHROMAT_NCL:
                    switch (seqhdr->pri) {
                        case DAV1D_COLOR_PRI_BT709:
                        case DAV1D_COLOR_PRI_UNKNOWN:
                            return &libyuv::kYuvF709Constants;
                        case DAV1D_COLOR_PRI_BT470BG:
                        case DAV1D_COLOR_PRI_BT601:
                            return &libyuv::kYuvJPEGConstants;
                        case DAV1D_COLOR_PRI_BT2020:
                            return &libyuv::kYuvV2020Constants;
                        default:
                            return nullptr;
                    }
                default:
                    return nullptr;
            }
        }
        else {
            switch (seqhdr->mtrx) {
                case DAV1D_MC_BT709:
                    return &libyuv::kYuvH709Constants;
                case DAV1D_MC_BT470BG:
                case DAV1D_MC_BT601:
                case DAV1D_MC_UNKNOWN:
                    return &libyuv::kYuvI601Constants;
                case DAV1D_MC_BT2020_NCL:
                    return &libyuv::kYuv2020Constants;
                case DAV1D_MC_CHROMAT_NCL:
                    switch (seqhdr->pri) {
                        case DAV1D_COLOR_PRI_BT709:
                        case DAV1D_COLOR_PRI_UNKNOWN:
                            return &libyuv::kYuvH709Constants;
                        case DAV1D_COLOR_PRI_BT470BG:
                        case DAV1D_COLOR_PRI_BT601:
                            return &libyuv::kYuvI601Constants;
                        case DAV1D_COLOR_PRI_BT2020:
                            return &libyuv::kYuv2020Constants;
                        default:
                            return nullptr;
                    }
                default:
                    return nullptr;
            }
        }
        return nullptr;
    }

    void
    handle_picture(Dav1dPicture *picture)
    {
        Dav1dPictureParameters picparam = picture->p;

        int width = picparam.w;
        int height = picparam.h;

        Dav1dSequenceHeader *seqhdr = picture->seq_hdr;

        std::uint8_t *Y_data = (std::uint8_t *)picture->data[0];
        std::uint8_t *U_data = (std::uint8_t *)picture->data[1];
        std::uint8_t *V_data = (std::uint8_t *)picture->data[2];
        ptrdiff_t Y_stride = picture->stride[0];
        ptrdiff_t UV_stride = picture->stride[1];

        int bgra_size = width * height * 4;
        std::uint8_t *bgra_data = new std::uint8_t[bgra_size];
        ptrdiff_t bgra_stride = width * 4;
        assert(bgra_data != NULL);

        if (seqhdr->mtrx == DAV1D_MC_IDENTITY &&
            seqhdr->layout == DAV1D_PIXEL_LAYOUT_I444) {
            // manually copy the data (look for a better way to do this)
            int dest_index = 0;
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    int source_index = y * Y_stride + x;
                    bgra_data[dest_index++] = U_data[source_index];
                    bgra_data[dest_index++] = Y_data[source_index];
                    bgra_data[dest_index++] = V_data[source_index];
                    bgra_data[dest_index++] = 255;
                }
            }
        }
        else {
            const struct libyuv::YuvConstants *matrixYUV =
                this->get_matrix_coefficients(seqhdr);
            assert(matrixYUV != nullptr);

            if (seqhdr->layout == DAV1D_PIXEL_LAYOUT_I420) {
                libyuv::I420ToARGBMatrix(
                    Y_data, Y_stride, U_data, UV_stride, V_data, UV_stride,
                    bgra_data, bgra_stride, matrixYUV, width, height);
            }
            if (seqhdr->layout == DAV1D_PIXEL_LAYOUT_I400) {
                libyuv::I400ToARGBMatrix(Y_data, Y_stride, bgra_data,
                                         bgra_stride, matrixYUV, width,
                                         height);
            }
            if (seqhdr->layout == DAV1D_PIXEL_LAYOUT_I422) {
                libyuv::I422ToARGBMatrix(
                    Y_data, Y_stride, U_data, UV_stride, V_data, UV_stride,
                    bgra_data, bgra_stride, matrixYUV, width, height);
            }
            if (seqhdr->layout == DAV1D_PIXEL_LAYOUT_I444) {
                libyuv::I444ToARGBMatrix(
                    Y_data, Y_stride, U_data, UV_stride, V_data, UV_stride,
                    bgra_data, bgra_stride, matrixYUV, width, height);
            }
        }

        SDL_Surface *frame = SDL_CreateRGBSurfaceWithFormatFrom(
            (void *)bgra_data, width, height, 32, bgra_stride,
            SDL_PIXELFORMAT_BGRA32);

        SDL_SetWindowSize(this->window, width, height);
        SDL_Surface *screen = SDL_GetWindowSurface(this->window);
        // SDL_SaveBMP(frame, "output.bmp");

        SDL_BlitSurface(frame, NULL, screen, NULL);
        SDL_UpdateWindowSurface(this->window);
        SDL_FreeSurface(frame);

        poll_events();

        /* frame by frame input delay */
        // char delay[10];
        // std::cin >> delay;

        delete[] bgra_data;
        dav1d_picture_unref(picture);
    }

    void
    decode_av1(std::uint8_t *buffer, std::uint64_t num_bytes)
    {
        Dav1dData data = {0};
        Dav1dPicture pic = {0};
        int status;

        // wrap the OBUs in a Dav1dData struct
        status =
            dav1d_data_wrap(&data, buffer, num_bytes, dealloc_buffer, NULL);
        if (status) {
            std::cout << "dav1d data wrap failed: " << status << std::endl;
            return;
        }

        do {
            // send the OBUs to dav1d
            status = dav1d_send_data(this->context, &data);
            if (status && status != DAV1D_ERR(EAGAIN)) {
                std::cout << "dav1d send data failed: " << status << std::endl;
                // manually unref the data
                dav1d_data_unref(&data);
                return;
            }

            // try to get the picture out
            this->drain_dav1d();
        } while (status == DAV1D_ERR(EAGAIN) || data.sz > 0);
    }

    Status
    OnTrackEntry(const ElementMetadata &metadata,
                 const TrackEntry &track_entry) override
    {
        // this is assuming at most one audio and one video track
        if (track_entry.codec_id.is_present()) {
            if (track_entry.codec_id.value() == "V_AV1") {
                this->av1_track_number = track_entry.track_number.value();
            }
            else if (track_entry.codec_id.value() == "A_OPUS") {
                this->opus_track_number = track_entry.track_number.value();
            }
        }
        return Status(Status::kOkCompleted);
    }

    Status
    OnSimpleBlockBegin(const ElementMetadata &metadata,
                       const SimpleBlock &simple_block,
                       Action *action) override
    {
        this->current_track_number = simple_block.track_number;
        if (simple_block.track_number == this->av1_track_number ||
            simple_block.track_number == this->opus_track_number) {
            *action = Action::kRead;
        }
        else {
            *action = Action::kSkip;
        }
        return Status(Status::kOkCompleted);
    }

    Status
    OnBlockBegin(const ElementMetadata &metadata, const Block &block,
                 Action *action) override
    {
        this->current_track_number = block.track_number;
        if (block.track_number == this->av1_track_number ||
            block.track_number == this->opus_track_number) {
            *action = Action::kRead;
        }
        else {
            *action = Action::kSkip;
        }
        return Status(Status::kOkCompleted);
    }

    Status
    OnFrame(const FrameMetadata &, Reader *reader,
            std::uint64_t *bytes_remaining) override
    {
        //  sanity checks
        assert(reader != nullptr);
        assert(bytes_remaining != nullptr);

        // create a buffer to store the frame data to
        std::uint64_t buffer_size = *bytes_remaining;
        if (buffer_size == 0) {
            return Status(Status::kOkCompleted);
        }

        std::uint8_t *buffer = new std::uint8_t[buffer_size];
        std::uint8_t *buffer_location = buffer;
        std::uint64_t num_read;

        // read frame data until there's no more to read
        Status status;
        do {
            status =
                reader->Read(*bytes_remaining, buffer_location, &num_read);
            buffer_location += num_read;
            *bytes_remaining -= num_read;
        } while (status.code == Status::kOkPartial);

        // decode the data
        if (this->current_track_number == this->av1_track_number) {
            std::cout << "received av1 frame from webm_parser" << std::endl;
            this->decode_av1(buffer, buffer_size);
        }
        else if (this->current_track_number == this->opus_track_number) {
            std::cout << "received opus frame from webm_parser" << std::endl;
            this->decode_opus(buffer, buffer_size);
        }

        return status;
    }

   private:
    uint64_t av1_track_number;
    uint64_t opus_track_number;
    uint64_t current_track_number;
    Dav1dContext *context;
    SDL_Window *window;
};

int
main(int argc, char *argv[])
{
    // make sure enough arguments have been provided
    if (argc < 2) {
        std::cout << "Error: No input file specified" << std::endl;
        exit(1);
    }

    // open the input file
    FILE *file = std::fopen(argv[1], "rb");

    // create the libwebm objects
    Av1Callback callback;
    FileReader reader(file);
    WebmParser parser;

    // create the dav1d objects
    Dav1dSettings settings = {0};
    dav1d_default_settings(&settings);
    Dav1dContext *context;
    dav1d_open(&context, &settings);

    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window =
        SDL_CreateWindow("!!AV1 != !!False", SDL_WINDOWPOS_UNDEFINED,
                         SDL_WINDOWPOS_UNDEFINED, 500, 500, 0);

    // setup the callback class
    callback.setup(context, window);

    // parse the input file
    Status status = parser.Feed(&callback, &reader);
    if (status.completed_ok()) {
        std::cout << "Parsing successfully completed" << std::endl;
    }
    else {
        std::cout << "Parsing failed with status code: " << status.code
                  << std::endl;
    }

    if (!callback.found_av1_track()) {
        std::cout << "The requested file does not contain an av1 track"
                  << std::endl;
    }

    // close the file
    std::fclose(file);

    return 0;
}
