// Taken wholesale from webm_parser's FileReader class with the addition of the
// SetPosition() function

#include "sav1_file_reader.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <memory>
#include <webm/status.h>

namespace webm {

Sav1FileReader::Sav1FileReader(FILE *file) : file_(file) { assert(file); }

Sav1FileReader::Sav1FileReader(Sav1FileReader &&other)
    : file_(std::move(other.file_)), position_(other.position_)
{
    other.position_ = 0;
}

Sav1FileReader &
Sav1FileReader::operator=(Sav1FileReader &&other)
{
    if (this != &other) {
        file_ = std::move(other.file_);
        position_ = other.position_;
        other.position_ = 0;
    }
    return *this;
}

Status
Sav1FileReader::Read(std::size_t num_to_read, std::uint8_t *buffer,
                     std::uint64_t *num_actually_read)
{
    assert(num_to_read > 0);
    assert(buffer != nullptr);
    assert(num_actually_read != nullptr);

    if (file_ == nullptr) {
        *num_actually_read = 0;
        return Status(Status::kEndOfFile);
    }

    std::size_t actual =
        std::fread(static_cast<void *>(buffer), 1, num_to_read, file_.get());
    *num_actually_read = static_cast<std::uint64_t>(actual);
    position_ += *num_actually_read;

    if (actual == 0) {
        return Status(Status::kEndOfFile);
    }

    if (actual == num_to_read) {
        return Status(Status::kOkCompleted);
    }
    else {
        return Status(Status::kOkPartial);
    }
}

Status
Sav1FileReader::Skip(std::uint64_t num_to_skip, std::uint64_t *num_actually_skipped)
{
    assert(num_to_skip > 0);
    assert(num_actually_skipped != nullptr);

    *num_actually_skipped = 0;

    if (file_ == nullptr) {
        return Status(Status::kEndOfFile);
    }

    // Try seeking forward first.
    long seek_offset = std::numeric_limits<long>::max();
    if (num_to_skip < static_cast<unsigned long>(seek_offset)) {
        seek_offset = static_cast<long>(num_to_skip);
    }

    if (!std::fseek(file_.get(), seek_offset, SEEK_CUR)) {
        *num_actually_skipped = static_cast<std::uint64_t>(seek_offset);
        position_ += static_cast<std::uint64_t>(seek_offset);
        if (static_cast<unsigned long>(seek_offset) == num_to_skip) {
            return Status(Status::kOkCompleted);
        }
        else {
            return Status(Status::kOkPartial);
        }
    }
    std::clearerr(file_.get());

    // Seeking doesn't work on things like pipes, so if seeking failed then fall
    // back to reading the data into a junk buffer.
    std::size_t actual = 0;
    do {
        std::uint8_t junk[1024];
        std::size_t num_to_read = sizeof(junk);
        if (num_to_skip < num_to_read) {
            num_to_read = static_cast<std::size_t>(num_to_skip);
        }

        std::size_t actual =
            std::fread(static_cast<void *>(junk), 1, num_to_read, file_.get());
        *num_actually_skipped += static_cast<std::uint64_t>(actual);
        position_ += static_cast<std::uint64_t>(actual);
        num_to_skip -= static_cast<std::uint64_t>(actual);
    } while (actual > 0 && num_to_skip > 0);

    if (*num_actually_skipped == 0) {
        return Status(Status::kEndOfFile);
    }

    if (num_to_skip == 0) {
        return Status(Status::kOkCompleted);
    }
    else {
        return Status(Status::kOkPartial);
    }
}

std::uint64_t
Sav1FileReader::Position() const
{
    return position_;
}

void
Sav1FileReader::SetPosition(std::uint64_t pos)
{
    position_ = pos;
}

}  // namespace webm
