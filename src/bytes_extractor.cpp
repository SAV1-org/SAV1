#include <iomanip>
#include <iostream>
#include <fstream>
#include <cassert>

#include <webm/callback.h>
#include <webm/file_reader.h>
#include <webm/status.h>
#include <webm/webm_parser.h>

using namespace webm;

class FrameCallback : public Callback {
   public:
    void
    open_file(char *path)
    {
        // open the specified file to write frame data to
        this->out_file.open(
            path, std::ios::out | std::ios::trunc | std::ios::binary);
        this->av1_track_number = -1;
    }

    void
    close_file()
    {
        // close the output file
        this->out_file.close();
    }

    bool
    found_av1_track()
    {
        return this->av1_track_number != -1;
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
        std::uint8_t *buffer = new std::uint8_t[*bytes_remaining];
        std::uint64_t num_read;

        // make sure the output file is open
        if (!this->out_file.is_open()) {
            std::cout << "Output binary file "
                         "is not open"
                      << std::endl;
            exit(1);
        }

        // read frame data until there's no more to read
        Status status;
        do {
            status = reader->Read(*bytes_remaining, buffer, &num_read);
            // write bytes to output file
            for (int i = 0; i < num_read; i++) {
                this->out_file << buffer[i];
            }
            *bytes_remaining -= num_read;
        } while (status.code == Status::kOkPartial);

        // clean up memory
        delete[] buffer;

        return status;
    }

   private:
    std::ofstream out_file;
    uint64_t av1_track_number;
};

int
main(int argc, char *argv[])
{
    // make sure enough arguments have been provided
    if (argc < 3) {
        std::cout << "Error: Not enough arguments provided. Expects "
                     "<input_file.webm> <output_binary_file>"
                  << std::endl;
        exit(1);
    }

    // open the input file
    FILE *file = std::fopen(argv[1], "rb");

    // create the libwebm objects
    FrameCallback callback;
    FileReader reader(file);
    WebmParser parser;

    // open the output file
    callback.open_file(argv[2]);

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

    // close the files
    std::fclose(file);
    callback.close_file();
    return 0;
}
