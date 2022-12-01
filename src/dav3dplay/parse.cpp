#include "parse.h"
#include "sav1_settings.h"
#include <cassert>
#include <webm/callback.h>
#include <webm/file_reader.h>
#include <webm/status.h>
#include <webm/webm_parser.h>

#define PARSE_TRACK_NUMBER_NOT_SPECIFIED 99999

using namespace webm;

class Sav1Callback : public Callback {
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
        this->av1_codec_delay = 0;
        this->opus_codec_delay = 0;
    }

    bool
    found_av1_track()
    {
        return this->av1_track_number != PARSE_TRACK_NUMBER_NOT_SPECIFIED;
    }

    bool
    found_opus_track()
    {
        return this->opus_track_number != PARSE_TRACK_NUMBER_NOT_SPECIFIED;
    }

    void
    calculate_timecode(std::int16_t relative_time)
    {
        std::uint64_t total_time = this->cluster_timecode + relative_time;
        if (this->current_track_number == this->av1_track_number) {
            if (this->av1_codec_delay >= total_time) {
                total_time = 0;
            }
            else {
                total_time -= this->av1_codec_delay;
            }
        }
        else if (this->current_track_number == this->opus_track_number) {
            if (this->opus_codec_delay >= total_time) {
                total_time = 0;
            }
            else {
                total_time -= this->opus_codec_delay;
            }
        }
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
                if (track_entry.track_number.is_present()) {
                    this->av1_track_number = track_entry.track_number.value();
                }
                if (track_entry.codec_delay.is_present()) {
                    this->av1_codec_delay = track_entry.codec_delay.value();
                }
            }
            else if (track_entry.codec_id.value() == "A_OPUS") {
                if (track_entry.track_number.is_present()) {
                    this->opus_track_number = track_entry.track_number.value();
                }
                if (track_entry.codec_delay.is_present()) {
                    this->opus_codec_delay = track_entry.codec_delay.value();
                }
                if (track_entry.audio.is_present()) {
                    if (track_entry.audio.value().channels.is_present()) {
                        this->opus_num_channels =
                            track_entry.audio.value().channels.value();
                    }
                    if (track_entry.audio.value().sampling_frequency.is_present()) {
                        this->opus_sampling_frequency =
                            track_entry.audio.value().sampling_frequency.value();
                    }
                }
            }
        }
        return Status(Status::kOkCompleted);
    }

    Status
    OnSimpleBlockBegin(const ElementMetadata &, const SimpleBlock &simple_block,
                       Action *action) override
    {
        assert(action != nullptr);
        if ((simple_block.track_number == this->av1_track_number &&
             this->context->codec_target & SAV1_CODEC_TARGET_AV1) ||
            (simple_block.track_number == this->opus_track_number &&
             this->context->codec_target & SAV1_CODEC_TARGET_OPUS)) {
            *action = Action::kRead;
            this->current_track_number = simple_block.track_number;
            this->calculate_timecode(simple_block.timecode);
        }
        else {
            *action = Action::kSkip;
        }
        return Status(Status::kOkCompleted);
    }

    Status
    OnBlockBegin(const ElementMetadata &, const Block &block, Action *action) override
    {
        assert(action != nullptr);
        if ((block.track_number == this->av1_track_number &&
             this->context->codec_target & SAV1_CODEC_TARGET_AV1) ||
            (block.track_number == this->opus_track_number &&
             this->context->codec_target & SAV1_CODEC_TARGET_OPUS)) {
            *action = Action::kRead;
            this->current_track_number = block.track_number;
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
        // check if we should stop parsing
        if (thread_atomic_int_load(&(this->context->do_parse)) == 0) {
            return Status(Status::kWouldBlock);
        }

        // sanity checks
        assert(reader != nullptr);
        assert(bytes_remaining != nullptr);
        if (*bytes_remaining == 0) {
            return Status(Status::kOkCompleted);
        }

        // create the WebMFrame
        WebMFrame *frame;
        webm_frame_init(&frame, *bytes_remaining);
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

        // fill in type-specific information
        if (this->current_track_number == this->av1_track_number &&
            this->context->codec_target & SAV1_CODEC_TARGET_AV1 &&
            this->context->video_output_queue != NULL) {
            frame->codec = PARSE_FRAME_TYPE_AV1;
            sav1_thread_queue_push(this->context->video_output_queue, frame);
        }
        else if (this->current_track_number == this->opus_track_number &&
                 this->context->codec_target & SAV1_CODEC_TARGET_OPUS &&
                 this->context->audio_output_queue != NULL) {
            frame->opus_sampling_frequency = this->opus_sampling_frequency;
            frame->opus_num_channels = this->opus_num_channels;
            frame->codec = PARSE_FRAME_TYPE_OPUS;
            sav1_thread_queue_push(this->context->audio_output_queue, frame);
        }
        else {
            // this is actually not a frame we want
            webm_frame_destroy(frame);
            return Status(Status::kOkCompleted);
        }

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
    std::uint64_t opus_codec_delay;
    std::uint64_t av1_codec_delay;
    std::uint64_t opus_num_channels;
    double opus_sampling_frequency;
};

typedef struct ParseInternalState {
    FileReader *reader;
    WebmParser *parser;
    Sav1Callback *callback;
    FILE *file;
} ParseInternalState;

void
parse_init(ParseContext **context, char *file_name, int codec_target,
           Sav1ThreadQueue *video_output_queue, Sav1ThreadQueue *audio_output_queue)
{
    // create the internal state
    ParseInternalState *state = new ParseInternalState;

    // open the input file
    state->file = std::fopen(file_name, "rb");

    // create the webmparser objects
    state->callback = new Sav1Callback();
    state->reader = new FileReader(state->file);
    state->parser = new WebmParser();

    // allocate the ParseContext
    ParseContext *parse_context = new ParseContext;
    *context = parse_context;

    // populate the ParseContext
    parse_context->internal_state = (void *)state;
    parse_context->codec_target = codec_target;
    parse_context->video_output_queue = video_output_queue;
    parse_context->audio_output_queue = audio_output_queue;
    thread_atomic_int_store(&(parse_context->status), PARSE_STATUS_OK);

    // initialize the callback class
    state->callback->init(parse_context);
}

void
parse_destroy(ParseContext *context)
{
    // clean up internal state
    ParseInternalState *state = (ParseInternalState *)context->internal_state;
    assert(state != nullptr);

    fclose(state->file);
    delete state->callback;
    delete state->reader;
    delete state->parser;
    delete state;

    // clean up the context
    delete context;
}

int
parse_start(void *context)
{
    ParseContext *parse_context = (ParseContext *)context;
    thread_atomic_int_store(&(parse_context->do_parse), 1);
    ParseInternalState *state = (ParseInternalState *)parse_context->internal_state;
    Status status = state->parser->Feed(state->callback, state->reader);
    if (status.completed_ok()) {
        thread_atomic_int_store(&(parse_context->status), PARSE_STATUS_END_OF_FILE);
    }
    else {
        thread_atomic_int_store(&(parse_context->status), PARSE_STATUS_ERROR);
    }
    return 0;
}

void
parse_stop(ParseContext *context)
{
    thread_atomic_int_store(&(context->do_parse), 0);
    if (context->video_output_queue != NULL) {
        while (1) {
            WebMFrame *video_frame =
                (WebMFrame *)sav1_thread_queue_pop_timeout(context->video_output_queue);
            if (video_frame == NULL) {
                break;
            }
            else {
                webm_frame_destroy(video_frame);
            }
        }
    }
    if (context->audio_output_queue != NULL) {
        while (1) {
            WebMFrame *audio_frame =
                (WebMFrame *)sav1_thread_queue_pop_timeout(context->audio_output_queue);
            if (audio_frame == NULL) {
                break;
            }
            else {
                webm_frame_destroy(audio_frame);
            }
        }
    }
}

int
parse_get_status(ParseContext *context)
{
    return thread_atomic_int_load(&(context->status));
}

int
parse_found_av1_track(ParseContext *context)
{
    ParseInternalState *state = (ParseInternalState *)context->internal_state;
    return state->callback->found_av1_track() ? 1 : 0;
}

int
parse_found_opus_track(ParseContext *context)
{
    ParseInternalState *state = (ParseInternalState *)context->internal_state;
    return state->callback->found_opus_track() ? 1 : 0;
}

void
webm_frame_init(WebMFrame **frame, std::size_t size)
{
    WebMFrame *webm_frame = new WebMFrame;
    *frame = webm_frame;
    webm_frame->data = new std::uint8_t[size];
    webm_frame->size = size;
    webm_frame->timecode = 0;
    webm_frame->opus_sampling_frequency = 0;
    webm_frame->opus_num_channels = 0;
    webm_frame->codec = 0;
}

void
webm_frame_destroy(WebMFrame *frame)
{
    assert(frame != nullptr);
    assert(frame->data != nullptr);
    delete[] frame->data;
    delete frame;
}
