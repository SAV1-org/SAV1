#include "parse.h"

#define PARSE_TRACK_NUMBER_NOT_SPECIFIED 99999
#define PARSE_BUFFER_CAPACITY 10
#define INITIAL_FRAME_CAPACITY 512

using namespace webm;

void
webm_frame_init(WebMFrame *, std::size_t);

void
webm_frame_destroy(WebMFrame *);

void
webm_frame_resize(WebMFrame *, std::size_t);

class Av1Callback : public Callback {
   public:
    void
    init(ParseContext *context)
    {
        this->context = context;
        this->av1_track_number = PARSE_TRACK_NUMBER_NOT_SPECIFIED;
        this->opus_track_number = PARSE_TRACK_NUMBER_NOT_SPECIFIED;
        this->current_track_number = 0;
        this->timecode_scale = 0;
        this->cluster_timecode = 0;
        this->timecode = 0;
        this->target_video = true;
    }

    bool
    found_av1_track()
    {
        return this->av1_track_number != PARSE_TRACK_NUMBER_NOT_SPECIFIED;
    }

    void
    do_target_video(bool state)
    {
        this->target_video = state;
    }

    void
    calculate_timecode(std::int16_t relative_time)
    {
        std::uint64_t total_time = this->cluster_timecode + relative_time;
        this->timecode = (total_time * this->timecode_scale) / 1000000;
    }

    Status
    OnInfo(const ElementMetadata &, const Info &info) override
    {
        if (info.timecode_scale.is_present()) {
            this->timecode_scale = info.timecode_scale.value();
        }
        return Status(Status::kOkCompleted);
    }

    Status
    OnClusterBegin(const ElementMetadata &, const Cluster &cluster,
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
    OnTrackEntry(const ElementMetadata &, const TrackEntry &track_entry) override
    {
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
    OnSimpleBlockBegin(const ElementMetadata &, const SimpleBlock &simple_block,
                       Action *action) override
    {
        if (simple_block.track_number == this->av1_track_number ||
            simple_block.track_number == this->opus_track_number) {
            *action = Action::kRead;
            this->calculate_timecode(simple_block.timecode);
            this->current_track_number = simple_block.track_number;
        }
        else {
            *action = Action::kSkip;
        }
        return Status(Status::kOkCompleted);
    }

    Status
    OnBlockBegin(const ElementMetadata &, const Block &block, Action *action) override
    {
        if (block.track_number == this->av1_track_number ||
            block.track_number == this->opus_track_number) {
            *action = Action::kRead;
            this->calculate_timecode(block.timecode);
            this->current_track_number = block.track_number;
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
        // sanity checks
        assert(reader != nullptr);
        assert(bytes_remaining != nullptr);
        if (*bytes_remaining == 0) {
            return Status(Status::kOkCompleted);
        }

        // make sure there is space in the frame buffer
        WebMFrame *frame;
        if (this->current_track_number == this->av1_track_number) {
            if (!this->context->do_overwrite_buffer &&
                this->context->video_frames_size >=
                    this->context->frame_buffer_capacity) {
                // the video buffer is full
                return Status(Status::kWouldBlock);
            }
            std::size_t index =
                (context->video_frames_index + context->video_frames_size) %
                this->context->frame_buffer_capacity;
            frame = &this->context->video_frames[index];
            this->context->video_frames_size++;
        }
        else if (this->current_track_number == this->opus_track_number) {
            if (!this->context->do_overwrite_buffer &&
                this->context->audio_frames_size >=
                    this->context->frame_buffer_capacity) {
                // the audio buffer is full
                return Status(Status::kWouldBlock);
            }
            std::size_t index =
                (this->context->audio_frames_index + this->context->audio_frames_size) %
                this->context->frame_buffer_capacity;
            frame = &this->context->audio_frames[index];
            this->context->audio_frames_size++;
        }
        else {
            // we've somehow read a frame from a track we don't care about
            return Status(Status::kOkCompleted);
        }

        // resize the WebMFrame if it's not big enough
        if (*bytes_remaining > frame->capacity) {
            // TODO: make this a better resize
            webm_frame_resize(frame, *bytes_remaining * 1.5);
        }

        // update the frame attributes
        frame->size = *bytes_remaining;
        frame->timecode = this->timecode;

        // read frame data until there's no more to read
        std::uint8_t *buffer_location = frame->data;
        std::uint64_t num_read;
        Status status;
        do {
            status = reader->Read(*bytes_remaining, buffer_location, &num_read);
            buffer_location += num_read;
            *bytes_remaining -= num_read;
        } while (status.code == Status::kOkPartial);

        // see if we should stop or keep parsing
        if (this->current_track_number == this->av1_track_number && this->target_video) {
            // this is video and we're looking for video
            return Status(Status::kOkPartial);
        }
        else if (this->current_track_number == this->opus_track_number &&
                 !this->target_video) {
            // this is audio and we're looking for audio
            return Status(Status::kOkPartial);
        }
        // keep parsing
        return Status(Status::kOkCompleted);
    }

   private:
    ParseContext *context;
    std::uint64_t current_track_number;
    std::uint64_t av1_track_number;
    std::uint64_t opus_track_number;
    std::uint64_t timecode_scale;
    std::uint64_t cluster_timecode;
    std::uint64_t timecode;
    bool target_video;
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

    // allocate the frame buffers
    parse_context->frame_buffer_capacity = PARSE_BUFFER_CAPACITY;
    parse_context->video_frames = new WebMFrame[parse_context->frame_buffer_capacity];
    parse_context->audio_frames = new WebMFrame[parse_context->frame_buffer_capacity];

    // populate the ParseContext
    parse_context->internal_state = (void *)state;
    parse_context->video_frames_index = 0;
    parse_context->audio_frames_index = 0;
    parse_context->video_frames_size = 0;
    parse_context->audio_frames_size = 0;
    parse_context->do_overwrite_buffer = 0;

    // populate the frame buffers
    for (std::size_t i = 0; i < parse_context->frame_buffer_capacity; i++) {
        webm_frame_init(&parse_context->video_frames[i], INITIAL_FRAME_CAPACITY);
        webm_frame_init(&parse_context->audio_frames[i], INITIAL_FRAME_CAPACITY);
    }

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

    // clean up the frame buffers
    for (std::size_t i = 0; i < context->frame_buffer_capacity; i++) {
        webm_frame_destroy(&context->video_frames[i]);
        webm_frame_destroy(&context->audio_frames[i]);
    }
    delete[] context->video_frames;
    delete[] context->audio_frames;

    // clean up the context
    delete context;
}

int
parse_get_next_video_frame(ParseContext *context, WebMFrame **frame)
{
    // check if we already have a frame in the buffer
    if (context->video_frames_size == 0) {
        // we need to parse further to get more frames
        ParseInternalState *state = (ParseInternalState *)context->internal_state;
        state->callback->do_target_video(true);

        Status status = state->parser->Feed(state->callback, state->reader);
        if (status.completed_ok()) {
            if (state->callback->found_av1_track()) {
                return PARSE_END_OF_FILE;
            }
            return PARSE_NO_AV1_TRACK;
        }
        else if (status.code == Status::kWouldBlock) {
            return PARSE_BUFFER_FULL;
        }
        else if (status.code != Status::kOkPartial) {
            return PARSE_ERROR;
        }
    }

    *frame = &context->video_frames[context->video_frames_index];
    context->video_frames_size--;
    context->video_frames_index =
        (context->video_frames_index + 1) % context->frame_buffer_capacity;
    return PARSE_OK;
}

int
parse_get_next_audio_frame(ParseContext *context, WebMFrame **frame)
{
    // check if we already have a frame in the buffer
    if (context->audio_frames_size == 0) {
        // we need to parse further to get more frames
        ParseInternalState *state = (ParseInternalState *)context->internal_state;
        state->callback->do_target_video(false);

        Status status = state->parser->Feed(state->callback, state->reader);
        if (status.completed_ok()) {
            if (state->callback->found_av1_track()) {
                return PARSE_END_OF_FILE;
            }
            return PARSE_NO_AV1_TRACK;
        }
        else if (status.code == Status::kOkPartial) {
            return PARSE_BUFFER_FULL;
        }
        else if (status.code != Status::kWouldBlock) {
            return PARSE_ERROR;
        }
    }

    *frame = &context->audio_frames[context->audio_frames_index];
    context->audio_frames_size--;
    context->audio_frames_index =
        (context->audio_frames_index + 1) % context->frame_buffer_capacity;
    return PARSE_OK;
}

void
parse_clear_audio_buffer(ParseContext *context)
{
    context->audio_frames_size = 0;
    context->audio_frames_index = 0;
}

void
parse_clear_video_buffer(ParseContext *context)
{
    context->video_frames_size = 0;
    context->video_frames_index = 0;
}

void
webm_frame_init(WebMFrame *frame, std::size_t capacity)
{
    frame->size = 0;
    frame->timecode = 0;
    frame->capacity = capacity;
    frame->data = new std::uint8_t[frame->capacity];
}

void
webm_frame_destroy(WebMFrame *frame)
{
    delete[] frame->data;
}

void
webm_frame_resize(WebMFrame *frame, std::size_t capacity)
{
    webm_frame_destroy(frame);
    webm_frame_init(frame, capacity);
}
