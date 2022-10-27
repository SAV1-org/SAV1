#include <iomanip>
#include <iostream>
#include <fstream>
#include <cassert>

#include <webm/callback.h>
#include <webm/file_reader.h>
#include <webm/status.h>
#include <webm/webm_parser.h>

#include "dav1d/dav1d.h"

using namespace webm;

void
fake_dealloc(const uint8_t *data, void *cookie)
{
    return;
}

char *
str_matrix_coefficient(int constant)
{
    char* val;
    switch(constant) {
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

class Av1Callback : public Callback {
   public:
    void
    setup(Dav1dContext *context)
    {
        // this should be moved to the constructor
        this->av1_track_number = -1;
        this->context = context;
    }

    bool
    found_av1_track()
    {
        return this->av1_track_number != -1;
    }

    void
    drain_dav1d()
    {
        int status;
        int num_tries = 5;
        Dav1dPicture pic = {0};
        do {
            status = dav1d_get_picture(this->context, &pic);
            if (status == 0) {
                std::cout << "got picture from dav1d" << std::endl;
                this->handle_picture(&pic);
            }
            num_tries--;
        } while (status == DAV1D_ERR(EAGAIN) && num_tries);
    }

    void
    handle_picture(Dav1dPicture* picture)
    {
    #if 1
        Dav1dPictureParameters picparam = picture->p;
        printf("frame width=%i, height=%i bpc=%i\n", picparam.w, picparam.h, picparam.bpc);

        int height = picparam.w;
        int width = picparam.h;

        Dav1dSequenceHeader* seqhdr = picture->seq_hdr;
        if (seqhdr->layout == DAV1D_PIXEL_LAYOUT_I420) {
            printf("DAV1D_PIXEL_LAYOUT_I420\n");
        }
        if (seqhdr->layout == DAV1D_PIXEL_LAYOUT_I400) {
            printf("DAV1D_PIXEL_LAYOUT_I400\n");
        }
        if (seqhdr->layout == DAV1D_PIXEL_LAYOUT_I422) {
            printf("DAV1D_PIXEL_LAYOUT_I422\n");
        }
        if (seqhdr->layout == DAV1D_PIXEL_LAYOUT_I444) {
            printf("DAV1D_PIXEL_LAYOUT_I444\n");

            printf("Matrix coefficient=%s\n", str_matrix_coefficient(seqhdr->mtrx));

            std::uint8_t* Y = (std::uint8_t*)picture->data[0];
            std::uint8_t* U = (std::uint8_t*)picture->data[1];
            std::uint8_t* V = (std::uint8_t*)picture->data[2];

            // for one specific color matrix:
            // https://wikimedia.org/api/rest_v1/media/math/render/svg/a875fb1d6f42a721bf6c4f408ebd988d814bae58

            for (int x = 0; x < width; x++) {
                for (int y = 0; y < width; y++) {
                    int byte_loc = y*width + x;
                    int R = Y[byte_loc] + V[byte_loc] * 1.4746;
                    int G = Y[byte_loc] - 0.16 * U[byte_loc] - 0.57 * V[byte_loc];
                    int B = Y[byte_loc] + 1.88 * U[byte_loc];
                    printf("rgb=(%i, %i, %i)\n", R, G, B);
                    printf("yuv=(%i, %i, %i)\n", Y[byte_loc], U[byte_loc], V[byte_loc]);
                }
            }

        }

    #endif
    }

    void
    send_obu(std::uint8_t *buffer, std::uint64_t num_to_send)
    {
        if (num_to_send == 0) {
            return;
        }
        Dav1dData data = {0};
        Dav1dPicture pic = {0};
        int status;

        // wrap the OBU in a Dav1dData struct
        status =
            dav1d_data_wrap(&data, buffer, num_to_send, fake_dealloc, NULL);
        if (status) {
            std::cout << "dav1d data wrap failed: " << status << std::endl;
            return;
        }

        // send the OBU to dav1d
        status = dav1d_send_data(this->context, &data);
        if (status) {
            std::cout << "dav1d send data failed: " << status << std::endl;
            return;
        }

        // try to get the picture out
        this->drain_dav1d();
    }

    std::uint64_t
    parse_obu_size(std::uint8_t *buffer, std::uint8_t *size_skip)
    {
        std::uint64_t val = 0;
        std::uint8_t i = 0, more;

        do {
            uint8_t v = buffer[0];
            buffer++;
            more = v >> 7;
            val |= ((std::uint64_t)(v & 0x7F)) << i;
            i += 7;
        } while (more && i < 56);

        if (val > UINT_MAX || more) {
            std::cout << "Size bytes parsing error" << std::endl;
            *size_skip = 1;
            return 0;
        }

        *size_skip = i / 7;
        return val;
    }

    void
    parse_obu(std::uint8_t *buffer, std::uint64_t buffer_size)
    {
        if (buffer[0] >> 7 || buffer[0] & 1) {
            std::cout << "Illegal bit is set with payload ";
            for (int i = 0; i < buffer_size; i++) {
                std::cout << std::setfill('0') << std::setw(2) << std::hex
                          << (unsigned int)buffer[i] << " ";
            }
            std::cout << std::endl;
            return;
        }

        // get the OBU type
        int obu_type = buffer[0] >> 3;
        int size_start_index = 1;

        // check for extension flag
        if ((buffer[0] >> 2) & 1) {
            // extension flag is set
            std::cout << "Extension flag is set" << std::endl;
            // move start of size bytes to account for extension byte
            size_start_index = 2;
        }

        std::uint64_t obu_size = buffer_size;
        if ((buffer[0] >> 1) & 1) {
            // size flag is set
            std::uint8_t size_skip;
            // set obu_size to the payload size
            obu_size =
                this->parse_obu_size(buffer + size_start_index, &size_skip);
            // add the bytes for the extension, header, and size
            obu_size += size_skip + size_start_index;
            std::cout << "Found type " << obu_type << " OBU of size "
                      << obu_size << std::endl;
        }
        else {
            // size flag is not set
            std::cout << "Found type " << obu_type
                      << " OBU with size flag not set" << std::endl;
        }

        // send the OBU to dav1d
        this->send_obu(buffer, obu_size);

        // keep going if there's more OBUs in the buffer
        if (obu_size < buffer_size) {
            this->parse_obu(buffer + obu_size, buffer_size - obu_size);
        }
    }

    Status
    OnTrackEntry(const ElementMetadata &metadata,
                 const TrackEntry &track_entry) override
    {
        // make sure the track uses the AV1 codec
        if (track_entry.codec_id.is_present() &&
            track_entry.codec_id.value() == "V_AV1") {
            this->av1_track_number = track_entry.track_number.value();
        }
        return Status(Status::kOkCompleted);
    }

    Status
    OnSimpleBlockBegin(const ElementMetadata &metadata,
                       const SimpleBlock &simple_block,
                       Action *action) override
    {
        if (simple_block.track_number == this->av1_track_number) {
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

        // parse the OBU
        this->parse_obu(buffer, buffer_size);

        // clean up memory
        delete[] buffer;

        return status;
    }

   private:
    uint64_t av1_track_number;
    Dav1dContext *context;
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

    // setup the callback class
    callback.setup(context);

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
