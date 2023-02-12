#ifndef SAV1_FILE_READER
#define SAV1_FILE_READER

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <webm/reader.h>
#include <webm/status.h>

/**
 \file
 A `Reader` implementation that reads from a `FILE*`. This is exactly like the FileReader
 class from webmparser, except it has an additional method to set the private position_
 variable.
 */

namespace webm {

/**
 A `Reader` implementation that can read from `FILE*` resources.
 */
class Sav1FileReader : public Reader {
   public:
    /**
     Constructs a new, empty reader.
     */
    Sav1FileReader() = default;

    /**
     Constructs a new reader, using the provided file as the data source.

     Ownership of the file is taken, and `std::fclose()` will be called when the
     object is destroyed.

     \param file The file to use as a data source. Must not be null. The file will
     be closed (via `std::fclose()`) when the `Sav1FileReader`'s destructor runs.
     */
    explicit Sav1FileReader(FILE *file);

    /**
     Constructs a new reader by moving the provided reader into the new reader.

     \param other The source reader to move. After moving, it will be reset to an
     empty stream.
     */
    Sav1FileReader(Sav1FileReader &&other);

    /**
     Moves the provided reader into this reader.

     \param other The source reader to move. After moving, it will be reset to an
     empty stream. May be equal to `*this`, in which case this is a no-op.
     \return `*this`.
     */
    Sav1FileReader &
    operator=(Sav1FileReader &&other);

    Status
    Read(std::size_t num_to_read, std::uint8_t *buffer,
         std::uint64_t *num_actually_read) override;

    Status
    Skip(std::uint64_t num_to_skip, std::uint64_t *num_actually_skipped) override;

    std::uint64_t
    Position() const override;

    void
    SetPosition(std::uint64_t pos);

   private:
    struct FileCloseFunctor {
        void
        operator()(FILE *file) const
        {
            if (file)
                std::fclose(file);
        }
    };

    std::unique_ptr<FILE, FileCloseFunctor> file_;

    // We can't rely on ftell() for the position (since it only returns long, and
    // doesn't work on things like pipes); we need to manually track the reading
    // position.
    std::uint64_t position_ = 0;
};

}  // namespace webm

#endif
