#include <cassert>
#include <vector>
#include <webm/callback.h>
#include <webm/status.h>
#include <webm/webm_parser.h>
#include <webm/file_reader.h>

extern "C" {
#include "parse.h"
#include "sav1_settings.h"
#include "sav1_internal.h"
#include "webm_frame.h"
}

#define PARSE_TRACK_NUMBER_NOT_SPECIFIED 99999
#define PARSE_SEEK_STATUS 5

using namespace webm;

typedef struct Sav1CuePoint {
    std::uint64_t timecode;
    std::uint64_t cluster_location;
} Sav1CuePoint;

class Sav1Callback : public Callback {
   public:
    void
    init(ParseContext *context, FileReader *reader)
    {
        this->context = context;
        this->reader = reader;
        this->av1_track_number = PARSE_TRACK_NUMBER_NOT_SPECIFIED;
        this->opus_track_number = PARSE_TRACK_NUMBER_NOT_SPECIFIED;
        this->current_track_number = 0;
        this->timecode_scale = 1;
        this->cluster_timecode = 0;
        this->timecode = 0;
        this->av1_codec_delay = 0;
        this->opus_codec_delay = 0;
        this->last_cluster_location = 0;
        Sav1CuePoint cue = {0, 0};
        this->cue_points.push_back(cue);
        this->all_cue_points = false;
        this->skip_clusters = false;
        this->is_key_frame = false;
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

    bool
    has_all_cue_points()
    {
        return this->all_cue_points;
    }

    void
    mark_has_all_cue_points()
    {
        this->all_cue_points = true;
    }

    void
    set_skip_clusters(bool do_skip_clusters)
    {
        this->skip_clusters = do_skip_clusters;
    }

    const std::vector<Sav1CuePoint> &
    get_cue_points() const
    {
        return this->cue_points;
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
        if (info.duration.is_present()) {
            thread_mutex_lock(this->context->duration_lock);
            this->context->duration = (std::uint64_t)(
                (info.duration.value() * this->timecode_scale) / 1000000.0);
            ;
            thread_mutex_unlock(this->context->duration_lock);
        }
        return Status(Status::kOkCompleted);
    }

    Status
    OnClusterBegin(const ElementMetadata &, const Cluster &cluster,
                   Action *action) override
    {
        if (cluster.timecode.is_present()) {
            this->cluster_timecode = cluster.timecode.value();
        }

        uint64_t timecode_ms = (this->cluster_timecode * this->timecode_scale) / 1000000;
        if (!this->all_cue_points && timecode_ms > this->cue_points.back().timecode) {
            Sav1CuePoint cue;
            cue.timecode = timecode_ms;
            cue.cluster_location = this->last_cluster_location;
            this->cue_points.push_back(cue);
        }

        if (this->skip_clusters) {
            if (timecode_ms > this->context->seek_timecode) {
                this->skip_clusters = false;
                return Status(PARSE_SEEK_STATUS);
            }
            *action = Action::kSkip;
        }
        else {
            *action = Action::kRead;
        }
        return Status(Status::kOkCompleted);
    }

    Status
    OnClusterEnd(const ElementMetadata &, const Cluster &)
    {
        this->last_cluster_location = this->reader->Position();
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
                    this->av1_codec_delay =
                        (track_entry.codec_delay.value() * 1) / 1000000;
                }
            }
            else if (track_entry.codec_id.value() == "A_OPUS") {
                if (track_entry.track_number.is_present()) {
                    this->opus_track_number = track_entry.track_number.value();
                }
                if (track_entry.codec_delay.is_present()) {
                    this->opus_codec_delay =
                        (track_entry.codec_delay.value() * 1) / 1000000;
                }
            }
        }
        return Status(Status::kOkCompleted);
    }

    Status
    OnSimpleBlockBegin(const ElementMetadata &, const SimpleBlock &simple_block,
                       Action *action) override
    {
        if ((simple_block.track_number == this->av1_track_number &&
             this->context->codec_target & SAV1_CODEC_AV1) ||
            (simple_block.track_number == this->opus_track_number &&
             this->context->codec_target & SAV1_CODEC_OPUS)) {
            this->current_track_number = simple_block.track_number;
            this->calculate_timecode(simple_block.timecode);
            this->is_key_frame = simple_block.is_key_frame;

            // skip opus frames before seek point
            if (this->current_track_number == this->opus_track_number &&
                thread_atomic_int_load(&(this->context->do_seek)) & SAV1_CODEC_OPUS &&
                this->timecode < this->context->seek_timecode) {
                *action = Action::kSkip;
            }
            else {
                *action = Action::kRead;
            }
        }
        else {
            *action = Action::kSkip;
        }
        return Status(Status::kOkCompleted);
    }

    Status
    OnBlockBegin(const ElementMetadata &, const Block &block, Action *action) override
    {
        if ((block.track_number == this->av1_track_number &&
             this->context->codec_target & SAV1_CODEC_AV1) ||
            (block.track_number == this->opus_track_number &&
             this->context->codec_target & SAV1_CODEC_OPUS)) {
            this->current_track_number = block.track_number;
            this->calculate_timecode(block.timecode);
            this->is_key_frame = false;

            // skip opus frames before seek point
            if (this->current_track_number == this->opus_track_number &&
                thread_atomic_int_load(&(this->context->do_seek)) & SAV1_CODEC_OPUS &&
                this->timecode < this->context->seek_timecode) {
                *action = Action::kSkip;
            }
            else {
                *action = Action::kRead;
            }
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
            return Status(PARSE_SEEK_STATUS);
        }

        // sanity checks
        if (reader == nullptr || bytes_remaining == nullptr || *bytes_remaining == 0) {
            return Status(Status::kOkCompleted);
        }

        // create the WebMFrame
        WebMFrame *frame;
        if (webm_frame_init(&frame, *bytes_remaining)) {
            sav1_set_error(this->context->ctx, "malloc() failed in webm_frame_init()");
            sav1_set_critical_error_flag(this->context->ctx);
            return Status(Status::kNotEnoughMemory);
        }
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
        int do_seek = thread_atomic_int_load(&(this->context->do_seek));
        if (this->current_track_number == this->av1_track_number) {
            if (do_seek & SAV1_CODEC_AV1) {
                // give dav1d a five second head start when seeking
                if (this->timecode + 5000 >= this->context->seek_timecode) {
                    frame->sentinel = 1;
                }
                // discard any frames before the seek point
                if (this->timecode < this->context->seek_timecode) {
                    frame->do_discard = 1;
                }
                else {
                    // we've found what we're looking for
                    thread_atomic_int_store(&(this->context->do_seek),
                                            do_seek ^ SAV1_CODEC_AV1);
                }
            }
            frame->is_key_frame = this->is_key_frame;
            frame->codec = SAV1_CODEC_AV1;
            sav1_thread_queue_push(this->context->video_output_queue, frame);
        }
        else if (this->current_track_number == this->opus_track_number) {
            if (do_seek & SAV1_CODEC_OPUS) {
                // we've found what we're looking for
                thread_atomic_int_store(&(this->context->do_seek),
                                        do_seek ^ SAV1_CODEC_OPUS);
                frame->sentinel = 1;
            }
            frame->codec = SAV1_CODEC_OPUS;
            sav1_thread_queue_push(this->context->audio_output_queue, frame);
        }
        else {
            // this is actually not a frame we want
            webm_frame_destroy(frame);
        }

        return Status(Status::kOkCompleted);
    }

   private:
    ParseContext *context;
    FileReader *reader;
    std::uint64_t current_track_number;
    std::uint64_t av1_track_number;
    std::uint64_t opus_track_number;
    std::uint64_t timecode_scale;
    std::uint64_t cluster_timecode;
    std::uint64_t timecode;
    std::uint64_t opus_codec_delay;
    std::uint64_t av1_codec_delay;
    std::vector<Sav1CuePoint> cue_points;
    std::uint64_t last_cluster_location;
    bool all_cue_points;
    bool skip_clusters;
    bool is_key_frame;
};

typedef struct ParseInternalState {
    FileReader *reader;
    WebmParser *parser;
    Sav1Callback *callback;
    FILE *file;
} ParseInternalState;

void
parse_init(ParseContext **context, Sav1InternalContext *ctx,
           Sav1ThreadQueue *video_output_queue, Sav1ThreadQueue *audio_output_queue)
{
    // create the internal state
    ParseInternalState *state = new ParseInternalState;

    // open the input file
    state->file = std::fopen(ctx->settings->file_path, "rb");
    if (state->file == NULL) {
        sav1_set_error(ctx, "parse_init failed: input file does not exist");
        sav1_set_critical_error_flag(ctx);
        state->reader = nullptr;
    }
    else {
        state->reader = new FileReader(state->file);
    }

    // create the webmparser objects
    state->callback = new Sav1Callback();
    state->parser = new WebmParser();

    // allocate the ParseContext
    ParseContext *parse_context = new ParseContext;
    *context = parse_context;

    // populate the ParseContext
    parse_context->internal_state = (void *)state;
    parse_context->ctx = ctx;
    parse_context->codec_target = ctx->settings->codec_target;
    parse_context->on_file_end = ctx->settings->on_file_end;
    parse_context->video_output_queue = video_output_queue;
    parse_context->audio_output_queue = audio_output_queue;
    parse_context->duration_lock = new thread_mutex_t;
    parse_context->wait_before_seek = new thread_mutex_t;
    parse_context->wait_after_parse = new thread_mutex_t;
    parse_context->wait_to_acquire = new thread_mutex_t;
    parse_context->running = new thread_mutex_t;
    parse_context->duration = 0;
    parse_context->seek_timecode = 0;
    thread_atomic_int_store(&(parse_context->status), PARSE_STATUS_OK);
    thread_mutex_init(parse_context->duration_lock);
    thread_mutex_init(parse_context->wait_before_seek);
    thread_mutex_init(parse_context->wait_after_parse);
    thread_mutex_init(parse_context->wait_to_acquire);
    thread_mutex_init(parse_context->running);

    // initialize the callback class
    state->callback->init(parse_context, state->reader);
}

void
parse_destroy(ParseContext *context)
{
    // clean up internal state
    ParseInternalState *state = (ParseInternalState *)context->internal_state;

    delete state->callback;
    if (state->reader != nullptr) {
        delete state->reader;
    }
    delete state->parser;
    delete state;

    thread_mutex_term(context->duration_lock);
    thread_mutex_term(context->wait_before_seek);
    thread_mutex_term(context->wait_after_parse);
    thread_mutex_term(context->wait_to_acquire);
    thread_mutex_term(context->running);
    delete context->duration_lock;
    delete context->wait_before_seek;
    delete context->wait_after_parse;
    delete context->wait_to_acquire;
    delete context->running;

    // clean up the context
    delete context;
}

int
parse_start(void *context)
{
    ParseContext *parse_context = (ParseContext *)context;
    ParseInternalState *state = (ParseInternalState *)parse_context->internal_state;

    if (state->reader == nullptr) {
        return -1;
    }

    thread_atomic_int_store(&(parse_context->do_seek), 0);
    thread_mutex_lock(parse_context->running);

    Status status;
    while (1) {
        thread_atomic_int_store(&(parse_context->do_parse), 1);
        thread_atomic_int_store(&(parse_context->status), PARSE_STATUS_OK);
        status = state->parser->Feed(state->callback, state->reader);

        // see if we should end the loop
        if (status.completed_ok()) {
            thread_atomic_int_store(&(parse_context->status), PARSE_STATUS_END_OF_FILE);

            // it's possible that we didn't find what we were looking for when seeking
            if (thread_atomic_int_load(&(parse_context->do_parse)) &&
                thread_atomic_int_load(&(parse_context->do_seek))) {
                // give up on seeking
                thread_atomic_int_store(&(parse_context->do_seek), 0);

                // loop the front end back to the beginning
                if (parse_context->on_file_end == SAV1_FILE_END_LOOP) {
                    thread_mutex_lock(parse_context->ctx->seek_lock);
                    parse_context->ctx->do_seek = 0;
                    parse_context->ctx->end_of_file = parse_context->codec_target;
                    thread_mutex_unlock(parse_context->ctx->seek_lock);
                }
            }

            state->callback->mark_has_all_cue_points();

            if (parse_context->on_file_end == SAV1_FILE_END_WAIT) {
                // wait until ThreadManager tells us to resume
                thread_mutex_lock(parse_context->wait_to_acquire);
                thread_mutex_lock(parse_context->wait_after_parse);
                thread_mutex_unlock(parse_context->wait_after_parse);
                thread_mutex_unlock(parse_context->wait_to_acquire);
            }
            else {
                // seek back to the start
                state->reader->Seek(0);
                state->parser->DidSeek();
                state->callback->set_skip_clusters(false);
                continue;
            }
        }

        if (status.code < 0) {
            thread_atomic_int_store(&(parse_context->status), PARSE_STATUS_ERROR);
            break;
        }
        else if (thread_atomic_int_load(&(parse_context->do_seek)) == 0) {
            break;
        }

        // wait here until this mutex is unlocked
        thread_mutex_lock(parse_context->wait_before_seek);
        thread_mutex_unlock(parse_context->wait_before_seek);

        // find the cue point to seek to
        std::vector<Sav1CuePoint> cue_points = state->callback->get_cue_points();
        for (auto cue = cue_points.rbegin(); cue != cue_points.rend(); ++cue) {
            if (cue->timecode <= parse_context->seek_timecode) {
                state->reader->Seek(cue->cluster_location);
                state->parser->DidSeek();
                bool skip_clusters =
                    !state->callback->has_all_cue_points() && cue == cue_points.rbegin();
                state->callback->set_skip_clusters(skip_clusters);
                break;
            }
        }
    }

    thread_mutex_unlock(parse_context->running);
    parse_stop(parse_context);

    return 0;
}

void
parse_stop(ParseContext *context)
{
    thread_atomic_int_store(&(context->do_parse), 0);

    // pop an entry from the output queues so it doesn't hang on a push
    if (context->video_output_queue != NULL) {
        WebMFrame *video_frame =
            (WebMFrame *)sav1_thread_queue_pop_timeout(context->video_output_queue);
        if (video_frame != NULL) {
            webm_frame_destroy(video_frame);
        }
    }
    if (context->audio_output_queue != NULL) {
        WebMFrame *audio_frame =
            (WebMFrame *)sav1_thread_queue_pop_timeout(context->audio_output_queue);
        if (audio_frame != NULL) {
            webm_frame_destroy(audio_frame);
        }
    }

    if (thread_atomic_int_load(&(context->do_seek)) == 0) {
        // wait for parsing to officially stop
        thread_mutex_lock(context->running);
        thread_mutex_unlock(context->running);
    }

    // drain output queues
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
parse_seek_to_time(ParseContext *context, uint64_t timecode)
{
    thread_atomic_int_store(&(context->do_seek), context->codec_target);
    context->seek_timecode = timecode;
    parse_stop(context);
}
