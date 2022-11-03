#include "parse.h"

using namespace webm;

class Av1Callback : public Callback {
   public:
    void
    init(ParseContext *context)
    {
        this->context = context;
        this->av1_track_number = -1;
        this->timecode_scale = 0;
        this->cluster_timecode = 0;
    }

    bool
    found_av1_track()
    {
        return this->av1_track_number != -1;
    }

    void
    calculate_timecode(std::int16_t relative_time)
    {
        std::uint64_t total_time = this->cluster_timecode + relative_time;
        this->context->timecode = (total_time * this->timecode_scale) / 1000000;
    }

    Status
    OnInfo(const ElementMetadata &metadata, const Info &info) override
    {
        if (info.timecode_scale.is_present()) {
            this->timecode_scale = info.timecode_scale.value();
        }
        return Status(Status::kOkCompleted);
    }

    Status
    OnClusterBegin(const ElementMetadata &metadata, const Cluster &cluster,
                   Action *action) override
    {
        assert(action != nullptr);
        *action = Action::kRead;
        if (cluster.timecode.is_present()) {
            this->cluster_timecode = cluster.timecode.value();
        }
        return Status(Status::kOkCompleted);
    }

    Status
    OnTrackEntry(const ElementMetadata &metadata, const TrackEntry &track_entry) override
    {
        // this is assuming at most one video track
        if (track_entry.codec_id.is_present() &&
            track_entry.codec_id.value() == "V_AV1") {
            this->av1_track_number = track_entry.track_number.value();
        }
        return Status(Status::kOkCompleted);
    }

    Status
    OnSimpleBlockBegin(const ElementMetadata &metadata, const SimpleBlock &simple_block,
                       Action *action) override
    {
        if (simple_block.track_number == this->av1_track_number) {
            *action = Action::kRead;
            this->calculate_timecode(simple_block.timecode);
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
        if (block.track_number == this->av1_track_number) {
            *action = Action::kRead;
            this->calculate_timecode(block.timecode);
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
        if (*bytes_remaining == 0) {
            return Status(Status::kOkCompleted);
        }

        // resize the buffer if it's not big enough
        context->size = *bytes_remaining;
        while (context->size > context->capacity) {
            delete[] context->data;
            context->capacity *= 2;
            context->data = new std::uint8_t[context->capacity];
        }

        // read frame data until there's no more to read
        std::uint8_t *buffer_location = context->data;
        std::uint64_t num_read;
        Status status;
        do {
            status = reader->Read(*bytes_remaining, buffer_location, &num_read);
            buffer_location += num_read;
            *bytes_remaining -= num_read;
        } while (status.code == Status::kOkPartial);

        // stop parsing for now
        return Status(Status::kWouldBlock);
    }

   private:
    std::uint64_t av1_track_number;
    ParseContext *context;
    std::uint64_t timecode_scale;
    std::uint64_t cluster_timecode;
};

typedef struct ParseInternalState {
    FileReader *reader;
    WebmParser *parser;
    Av1Callback *callback;
    FILE *file;
} ParseInternalState;

void
parse_init(ParseContext **context, char *file_name)
{
    // create the internal state
    ParseInternalState *state = new ParseInternalState;

    // open the input file
    state->file = std::fopen(file_name, "rb");

    // create the webmparser objects
    state->callback = new Av1Callback();
    state->reader = new FileReader(state->file);
    state->parser = new WebmParser();

    // allocate the ParseContext
    ParseContext *parse_context = new ParseContext;
    *context = parse_context;

    // populate the ParseContext
    parse_context->internal_state = (void *)state;
    parse_context->size = 0;
    parse_context->timecode = 0;

    // create the data buffer
    parse_context->capacity = 512;
    parse_context->data = new std::uint8_t[parse_context->capacity];

    // initialize the callback class
    state->callback->init(parse_context);
}

void
parse_destroy(ParseContext *context)
{
    // clean up internal state
    ParseInternalState *state = (ParseInternalState *)context->internal_state;
    fclose(state->file);
    delete state->callback;
    delete state->reader;
    delete state->parser;
    delete state;

    // clean up context
    delete[] context->data;
    delete context;
}

int
parse_get_next_frame(ParseContext *context)
{
    ParseInternalState *state = (ParseInternalState *)context->internal_state;

    Status status = state->parser->Feed(state->callback, state->reader);
    if (status.completed_ok()) {
        if (state->callback->found_av1_track()) {
            return PARSE_END_OF_FILE;
        }
        return PARSE_NO_AV1_TRACK;
    }
    else if (status.code == Status::kWouldBlock) {
        return PARSE_OK;
    }
    return PARSE_ERROR;
}
