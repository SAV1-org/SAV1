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
    void open_file(char* path) {
        this->out_file.open(path, std::ios::out | std::ios::trunc | std::ios::binary);
    }

    void close_file() {
        this->out_file.close();
    }

    Status OnFrame(
        const FrameMetadata&, 
        Reader* reader,                 
        std::uint64_t* bytes_remaining
    ) override {
        assert(reader != nullptr);
        assert(bytes_remaining != nullptr);
        
        std::uint8_t* buffer = new std::uint8_t[*bytes_remaining];
        std::uint64_t num_read;

        Status status;
        do {
            status = reader->Read(*bytes_remaining, buffer, &num_read);
            for (int i = 0; i < num_read; i++) {
                if (!this->out_file.is_open()) {
                    std::cout << "Output binary file is not open" << std::endl;
                    exit(1);
                }
                this->out_file << buffer[i];
            }
            *bytes_remaining -= num_read;
        } while (status.code == Status::kOkPartial);

        delete buffer;
        return status;
    }

private:
    std::ofstream out_file;
};


int main(int argc, char* argv[]) {

    // make sure enough arguments have been provided
    if (argc < 3) {
        std::cout << "Error: Not enough arguments provided. Expects <input_file.webm> <output_binary_file>" << std::endl;
        exit(1);
    } 

    // open the input file
    FILE* file = std::fopen(argv[1], "rb");

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
    } else {
        std::cout << "Parsing failed with status code: " << status.code << std::endl;
    }

    // close the files
    std::fclose(file);
    callback.close_file();
    return 0;
}