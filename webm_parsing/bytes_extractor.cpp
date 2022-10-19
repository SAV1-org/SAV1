#include <iomanip>
#include <iostream>
#include <cassert>

#include <webm/callback.h>
#include <webm/file_reader.h>
#include <webm/status.h>
#include <webm/webm_parser.h>

using namespace webm;

class FrameCallback : public Callback {
public:
    Status OnFrame(
        const FrameMetadata&, 
        Reader* reader,                 
        std::uint64_t* bytes_remaining
    ) {
        assert(reader != nullptr);
        assert(bytes_remaining != nullptr);
        
        std::uint8_t* buffer = new std::uint8_t[*bytes_remaining];
        std::uint64_t num_read;

        reader->Read(*bytes_remaining, buffer, &num_read);
        
        for (int i = 0; i < *bytes_remaining; i++) {
            
            std::cout << (int) buffer[i] << ' ';
        }
        std::cout << std::endl;

        return Status(Status::kOkCompleted);
    }
};


int main(int argc, char* argv[]) {

    // make sure enough arguments have been provided
    if (argc < 2) {
        std::cout << "Error: file name expected" << std::endl;
        exit(1);
    } 

    // open the file
    FILE* file = std::fopen(argv[1], "rb");

    // create the libwebm objects
    FrameCallback callback;
    FileReader reader(file);
    WebmParser parser;

    // parse the file
    Status status = parser.Feed(&callback, &reader);
    if (status.completed_ok()) {
        std::cout << "Parsing successfully completed" << std::endl;
    } else {
        std::cout << "Parsing failed with status code: " << status.code << std::endl;
    }

    // close the file
    std::fclose(file);
    return 0;
}