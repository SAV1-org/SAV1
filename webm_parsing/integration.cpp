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
    send_dav1d_data(std::uint8_t *buffer, std::uint64_t num_to_send)
    {
        if (num_to_send == 0) {
            return;
        }
        Dav1dData data = {0};
        Dav1dPicture pic = {0};
        int status;

        status =
            dav1d_data_wrap(&data, buffer, num_to_send, fake_dealloc, NULL);
        if (status) {
            std::cout << "dav1d data wrap failed: " << status << std::endl;
            return;
        }

        status = dav1d_send_data(this->context, &data);
        if (status) {
            std::cout << "dav1d send data failed: " << status << std::endl;
            return;
        }

        status = dav1d_get_picture(context, &pic);
        if (status == 0) {
            std::cout << "got picture from dav1d" << std::endl;
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
        if (buffer[0] >> 7 || buffer[0] & 1) {
            std::cout << "Illegal bit is set with payload ";
            for (int i = 0; i < buffer_size; i++) {
                std::cout << std::setfill('0') << std::setw(2) << std::hex
                          << (unsigned int)buffer[i] << " ";
            }
            std::cout << std::endl;
            delete[] buffer;
            return Status(Status::kOkCompleted);
        }

        int obu_type = buffer[0] >> 3;
        if (obu_type != 1) {
            // frame OBU
            this->send_dav1d_data(buffer, buffer_size);
        }
        else {
            std::cout << "Found non-frame OBU of type " << obu_type
                      << std::endl;
            if ((buffer[0] >> 1) & 1) {
                // size flag is set
                std::uint8_t obu_payload_size = buffer[1] + 2;
                if (buffer_size != obu_payload_size) {
                    std::cout << "Found frame with multiple OBUs" << std::endl;
                }
                this->send_dav1d_data(buffer, obu_payload_size);
                this->send_dav1d_data(buffer + obu_payload_size,
                                      buffer_size - obu_payload_size);
            }
            else {
                // size flag is not set
                std::cout << "OBU size flag is not set" << std::endl;
            }
        }

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
