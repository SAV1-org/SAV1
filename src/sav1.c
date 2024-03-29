#include <string.h>
#include <time.h>
#include <stdio.h>

#include "sav1.h"
#include "sav1_internal.h"

#define CHECK_CTX_VALID(ctx) \
    if (ctx == NULL) {       \
        return -1;           \
    }

#define CHECK_CTX_INITIALIZED(ctx, context)                                            \
    if (context->is_initialized != 1) {                                                \
        sav1_set_error(                                                                \
            ctx, "Uninitialized context: sav1_create_context() not called or failed"); \
        return -1;                                                                     \
    }

#define CHECK_CTX_CRITICAL_ERROR(ctx) \
    if (ctx->critical_error_flag) {   \
        return -1;                    \
    }

#define CHECK_CONTEXT_VALID(context) \
    if (context == NULL) {           \
        return -1;                   \
    }

#define RAISE(ctx, error)       \
    sav1_set_error(ctx, error); \
    return -1;

#define RAISE_CRITICAL(ctx, error)     \
    sav1_set_critical_error_flag(ctx); \
    RAISE(ctx, error)

int
sav1_create_context(Sav1Context *context, Sav1Settings *settings)
{
    CHECK_CONTEXT_VALID(context)

    if (context->is_initialized == 1) {
        Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
        RAISE(ctx, "Context already created: sav1_create_context() failed")
    }

    Sav1InternalContext *ctx;
    if ((ctx = (Sav1InternalContext *)malloc(sizeof(Sav1InternalContext))) == NULL) {
        return -1;  // Internal Context Malloc Failed
    }

    // initialize context values
    ctx->context = context;
    ctx->critical_error_flag = 0;
    ctx->is_playing = 0;
    if ((ctx->start_time = (struct timespec *)malloc(sizeof(struct timespec))) == NULL) {
        RAISE_CRITICAL(ctx, "malloc() failed in sav1_create_context()");
    }
    ctx->pause_time = NULL;
    ctx->curr_video_frame = NULL;
    ctx->next_video_frame = NULL;
    ctx->video_frame_ready = 0;
    ctx->curr_audio_frame = NULL;
    ctx->next_audio_frame = NULL;
    ctx->audio_frame_ready = 0;
    ctx->end_of_file = 0;
    ctx->do_seek = 0;
    if ((ctx->seek_lock = (thread_mutex_t *)malloc(sizeof(thread_mutex_t))) == NULL) {
        RAISE_CRITICAL(ctx, "malloc() failed in sav1_create_context()");
    }
    thread_mutex_init(ctx->seek_lock);

    if ((ctx->settings = (Sav1Settings *)malloc(sizeof(Sav1Settings))) == NULL) {
        thread_mutex_term(ctx->seek_lock);
        RAISE_CRITICAL(ctx, "malloc() failed in sav1_create_context()");
    }

    // copy over settings to prevent future modifications (except to file_path)
    memcpy(ctx->settings, settings, sizeof(Sav1Settings));

    // clear error string
    memset(ctx->error_message, 0, SAV1_ERROR_MESSAGE_SIZE);

    thread_manager_init(&(ctx->thread_manager), ctx);
    thread_manager_start_pipeline(ctx->thread_manager);

    context->internal_state = (void *)ctx;
    context->is_initialized = 1;

    CHECK_CTX_CRITICAL_ERROR(ctx)

    return 0;
}

int
sav1_destroy_context(Sav1Context *context)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)

    thread_manager_kill_pipeline(ctx->thread_manager);
    thread_manager_destroy(ctx->thread_manager);

    // free time structs
    if (ctx->start_time != NULL) {
        free(ctx->start_time);
    }
    if (ctx->pause_time != NULL) {
        free(ctx->pause_time);
    }

    // free seek lock mutex
    if (ctx->seek_lock != NULL) {
        thread_mutex_term(ctx->seek_lock);
        free(ctx->seek_lock);
    }

    // free any remaining frames
    if (ctx->curr_video_frame != NULL && ctx->curr_video_frame->sav1_has_ownership) {
        sav1_video_frame_destroy(context, ctx->curr_video_frame);
    }
    if (ctx->next_video_frame != NULL && ctx->next_video_frame->sav1_has_ownership) {
        sav1_video_frame_destroy(context, ctx->next_video_frame);
    }
    if (ctx->curr_audio_frame != NULL && ctx->curr_audio_frame->sav1_has_ownership) {
        sav1_audio_frame_destroy(context, ctx->curr_audio_frame);
    }
    if (ctx->next_audio_frame != NULL && ctx->next_audio_frame->sav1_has_ownership) {
        sav1_audio_frame_destroy(context, ctx->next_audio_frame);
    }

    // free the settings
    if (ctx->settings != NULL) {
        free(ctx->settings);
    }

    free(ctx);
    context->is_initialized = 0;
    context->internal_state = NULL;

    return 0;
}

char *
sav1_get_error(Sav1Context *context)
{
    if (context == NULL) {
        return (char *)"Sav1Context context passed as NULL";
    }
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;

    if (ctx) {
        return ctx->error_message;
    }

    return (char *)"Uninitialized context: sav1_create_context() not called or failed";
}

void
seek_update_start_time(Sav1InternalContext *ctx)
{
    thread_mutex_lock(ctx->seek_lock);
    if (ctx->do_seek != ctx->settings->codec_target) {
        thread_mutex_unlock(ctx->seek_lock);
        return;
    }
    thread_mutex_unlock(ctx->seek_lock);

    struct timespec curr_time;
    clock_gettime(CLOCK_MONOTONIC, &curr_time);
    uint64_t adjusted_seek_timecode = ctx->seek_timecode / ctx->settings->playback_speed;
    ctx->start_time->tv_sec = curr_time.tv_sec - adjusted_seek_timecode / 1000;
    uint64_t timecode_ns = (adjusted_seek_timecode % 1000) * 1000000;
    if (timecode_ns > (uint64_t)curr_time.tv_nsec) {
        ctx->start_time->tv_sec--;
        ctx->start_time->tv_nsec = curr_time.tv_nsec + 1000000000 - timecode_ns;
    }
    else {
        ctx->start_time->tv_nsec = curr_time.tv_nsec - timecode_ns;
    }

    if (ctx->pause_time != NULL) {
        ctx->pause_time->tv_sec = curr_time.tv_sec;
        ctx->pause_time->tv_nsec = curr_time.tv_nsec;
    }
}

void
file_end_update_start_time(Sav1InternalContext *ctx)
{
    // only update if all the frames have reached the end of the file
    if (ctx->end_of_file != ctx->settings->codec_target) {
        return;
    }

    // only update if they are on loop mode
    if (ctx->settings->on_file_end != SAV1_FILE_END_LOOP) {
        return;
    }

    // update the start time
    int status = clock_gettime(CLOCK_MONOTONIC, ctx->start_time);
    if (status) {
        sav1_set_error_with_code(ctx, "clock_gettime() in file end handler returned %d",
                                 status);
        return;
    }

    // update the pause time
    if (ctx->pause_time != NULL) {
        status = clock_gettime(CLOCK_MONOTONIC, ctx->pause_time);
        if (status) {
            sav1_set_error_with_code(
                ctx, "clock_gettime() in file end handler returned %d", status);
            return;
        }
    }

    // ensure that the old frames get deleted
    if (ctx->curr_video_frame != NULL) {
        ctx->curr_video_frame->timecode = 0;
    }
    if (ctx->curr_audio_frame != NULL) {
        ctx->curr_audio_frame->timecode = 0;
    }

    ctx->end_of_file = 0;
}

void
pump_video_frames(Sav1InternalContext *ctx, uint64_t curr_ms)
{
    // if we have no next frame, try to get one
    if (ctx->next_video_frame == NULL &&
        sav1_thread_queue_get_size(ctx->thread_manager->video_output_queue) != 0) {
        ctx->next_video_frame = (Sav1VideoFrame *)sav1_thread_queue_pop(
            ctx->thread_manager->video_output_queue);
    }

    thread_mutex_lock(ctx->seek_lock);
    if (ctx->end_of_file & SAV1_CODEC_AV1) {
        file_end_update_start_time(ctx);
        thread_mutex_unlock(ctx->seek_lock);
        return;
    }

    // if we are seeking, throw out frames until we get to a sentinel frame
    while (ctx->next_video_frame != NULL && ctx->do_seek & SAV1_CODEC_AV1) {
        if (ctx->next_video_frame->sentinel) {
            // we no longer need to seek for AV1 frames
            seek_update_start_time(ctx);
            ctx->do_seek ^= SAV1_CODEC_AV1;
            ctx->next_video_frame->timecode = ctx->seek_timecode;
        }
        else {
            // throw out this frame and try another one if we can
            if (ctx->next_video_frame->sav1_has_ownership) {
                sav1_video_frame_destroy(ctx->context, ctx->next_video_frame);
            }
            if (sav1_thread_queue_get_size(ctx->thread_manager->video_output_queue) ==
                0) {
                ctx->next_video_frame = NULL;
                thread_mutex_unlock(ctx->seek_lock);
                return;
            }
            ctx->next_video_frame = (Sav1VideoFrame *)sav1_thread_queue_pop(
                ctx->thread_manager->video_output_queue);
        }
    }

    // while we have a next frame, and the next frame is ahead of current time
    // alternatively, in FAST mode- when we have a next frame and the current frame
    // has not yet been pulled.
    while (ctx->next_video_frame != NULL &&
           (ctx->settings->playback_mode == SAV1_PLAYBACK_FAST
                ? !ctx->video_frame_ready
                : ctx->next_video_frame->timecode <= curr_ms)) {
        if (ctx->curr_video_frame != NULL) {
            // if the new video frame is from before the current frame, then we've
            // looped back to the start of the file
            if (!ctx->do_seek &&
                ctx->curr_video_frame->timecode > ctx->next_video_frame->timecode) {
                ctx->end_of_file |= SAV1_CODEC_AV1;
                file_end_update_start_time(ctx);
                thread_mutex_unlock(ctx->seek_lock);
                return;
            }

            // clean up current frame before replacing it
            if (ctx->curr_video_frame->sav1_has_ownership) {
                sav1_video_frame_destroy(ctx->context, ctx->curr_video_frame);
            }
        }

        // mark ready, there is new content
        ctx->curr_video_frame = ctx->next_video_frame;
        ctx->video_frame_ready = 1;
        ctx->next_video_frame = NULL;

        // try to get new next frame from queue
        if (sav1_thread_queue_get_size(ctx->thread_manager->video_output_queue) != 0) {
            ctx->next_video_frame = (Sav1VideoFrame *)sav1_thread_queue_pop(
                ctx->thread_manager->video_output_queue);
        }

        /* In fast mode, you only want to go through this loop once, since you're
         * not going to skip any frames */
        if (ctx->settings->playback_mode == SAV1_PLAYBACK_FAST) {
            break;
        }
    }
    thread_mutex_unlock(ctx->seek_lock);
}

void
pump_audio_frames(Sav1InternalContext *ctx, uint64_t curr_ms)
{
    // if we have no next frame, try to get one
    if (ctx->next_audio_frame == NULL &&
        sav1_thread_queue_get_size(ctx->thread_manager->audio_output_queue) != 0) {
        ctx->next_audio_frame = (Sav1AudioFrame *)sav1_thread_queue_pop(
            ctx->thread_manager->audio_output_queue);
    }

    thread_mutex_lock(ctx->seek_lock);
    if (ctx->end_of_file & SAV1_CODEC_OPUS) {
        file_end_update_start_time(ctx);
        thread_mutex_unlock(ctx->seek_lock);
        return;
    }

    // if we are seeking, throw out frames until we get to a sentinel frame
    while (ctx->next_audio_frame != NULL && ctx->do_seek & SAV1_CODEC_OPUS) {
        if (ctx->next_audio_frame->sentinel) {
            // we no longer need to seek for Opus frames
            seek_update_start_time(ctx);
            ctx->do_seek ^= SAV1_CODEC_OPUS;
        }
        else {
            // throw out this frame and try another one if we can
            if (ctx->next_audio_frame->sav1_has_ownership) {
                sav1_audio_frame_destroy(ctx->context, ctx->next_audio_frame);
            }
            if (sav1_thread_queue_get_size(ctx->thread_manager->audio_output_queue) ==
                0) {
                ctx->next_audio_frame = NULL;
                thread_mutex_unlock(ctx->seek_lock);
                return;
            }
            ctx->next_audio_frame = (Sav1AudioFrame *)sav1_thread_queue_pop(
                ctx->thread_manager->audio_output_queue);
        }
    }

    // while we have a next frame, and the next frame is ahead of current time
    while (ctx->next_audio_frame != NULL &&
           (ctx->settings->playback_mode == SAV1_PLAYBACK_FAST
                ? !ctx->audio_frame_ready
                : ctx->next_audio_frame->timecode <= curr_ms)) {
        if (ctx->curr_audio_frame != NULL) {
            // if the new audio frame is from before the current frame, then we've
            // looped back to the start of the file
            if (!ctx->do_seek &&
                ctx->curr_audio_frame->timecode > ctx->next_audio_frame->timecode) {
                ctx->end_of_file |= SAV1_CODEC_OPUS;
                file_end_update_start_time(ctx);
                thread_mutex_unlock(ctx->seek_lock);
                return;
            }

            // clean up current frame before replacing it
            if (ctx->curr_audio_frame->sav1_has_ownership) {
                sav1_audio_frame_destroy(ctx->context, ctx->curr_audio_frame);
            }
        }

        // mark ready, there is new content
        ctx->curr_audio_frame = ctx->next_audio_frame;
        ctx->audio_frame_ready = 1;
        ctx->next_audio_frame = NULL;

        // try to get new next frame from queue
        if (sav1_thread_queue_get_size(ctx->thread_manager->audio_output_queue) != 0) {
            ctx->next_audio_frame = (Sav1AudioFrame *)sav1_thread_queue_pop(
                ctx->thread_manager->audio_output_queue);
        }

        /* In fast mode, you only want to go through this loop once, since you're
         * not going to skip any frames */
        if (ctx->settings->playback_mode == SAV1_PLAYBACK_FAST) {
            break;
        }
    }
    thread_mutex_unlock(ctx->seek_lock);
}

int
sav1_get_video_frame(Sav1Context *context, Sav1VideoFrame **frame)
{
    *frame = NULL;  // on error, frame should default to 0

    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    if ((ctx->settings->codec_target & SAV1_CODEC_AV1) == 0) {
        RAISE(ctx, "Can't get video when not targeting video in settings")
    }

    uint64_t curr_ms;
    sav1_get_playback_time(context, &curr_ms);
    pump_video_frames(ctx, curr_ms);

    *frame = ctx->curr_video_frame;
    ctx->video_frame_ready = 0;
    return 0;
}

int
sav1_get_audio_frame(Sav1Context *context, Sav1AudioFrame **frame)
{
    *frame = NULL;  // on error, frame should default to 0

    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    if ((ctx->settings->codec_target & SAV1_CODEC_OPUS) == 0) {
        RAISE(ctx, "Can't get audio when not targeting audio in settings")
    }

    uint64_t curr_ms;
    sav1_get_playback_time(context, &curr_ms);
    pump_audio_frames(ctx, curr_ms);

    *frame = ctx->curr_audio_frame;
    ctx->audio_frame_ready = 0;
    return 0;
}

int
sav1_get_video_frame_ready(Sav1Context *context, int *is_ready)
{
    *is_ready = 0;  // on error, is_ready should default to 0

    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    if ((ctx->settings->codec_target & SAV1_CODEC_AV1) == 0) {
        RAISE(ctx, "Can't get video when not targeting video in settings")
    }

    uint64_t curr_ms;
    sav1_get_playback_time(context, &curr_ms);
    pump_video_frames(ctx, curr_ms);

    *is_ready = ctx->video_frame_ready;

    return 0;
}

int
sav1_get_audio_frame_ready(Sav1Context *context, int *is_ready)
{
    *is_ready = 0;  // on error, is_ready should default to 0

    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    if ((ctx->settings->codec_target & SAV1_CODEC_OPUS) == 0) {
        RAISE(ctx, "Can't get audio when not targeting audio in settings")
    }

    uint64_t curr_ms;
    sav1_get_playback_time(context, &curr_ms);
    pump_audio_frames(ctx, curr_ms);

    *is_ready = ctx->audio_frame_ready;

    return 0;
}

int
sav1_start_playback(Sav1Context *context)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    // make sure the video isn't already playing
    if (ctx->is_playing) {
        RAISE(ctx, "sav1_start_playback() called when already playing")
    }

    thread_mutex_lock(ctx->seek_lock);
    if (ctx->do_seek == ctx->settings->codec_target) {
        thread_mutex_unlock(ctx->seek_lock);
        RAISE(ctx, "sav1_stop_playback() called while in the middle of seeking")
    }
    thread_mutex_unlock(ctx->seek_lock);

    // update the start time value
    int status;
    if (ctx->pause_time == NULL) {
        // video is not paused so make the beginning now
        status = clock_gettime(CLOCK_MONOTONIC, ctx->start_time);
        if (status) {
            sav1_set_error_with_code(
                ctx, "clock_gettime() in sav1_start_playback() returned %d", status);
            return -1;
        }
    }
    else {
        // video is paused so set the start relative to that
        struct timespec curr_time;
        status = clock_gettime(CLOCK_MONOTONIC, &curr_time);
        if (status) {
            sav1_set_error_with_code(
                ctx, "clock_gettime() in sav1_start_playback() returned %d", status);
            return -1;
        }
        ctx->start_time->tv_sec += curr_time.tv_sec - ctx->pause_time->tv_sec;
        ctx->start_time->tv_sec +=
            (curr_time.tv_nsec - ctx->pause_time->tv_nsec) / 1000000000;
        ctx->start_time->tv_nsec +=
            (curr_time.tv_nsec - ctx->pause_time->tv_nsec) % 1000000000;

        // free the pause time
        free(ctx->pause_time);
        ctx->pause_time = NULL;
    }

    // set the playback status
    ctx->is_playing = 1;

    return 0;
}

int
sav1_stop_playback(Sav1Context *context)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    // make sure the video is already playing
    if (!ctx->is_playing) {
        RAISE(ctx, "sav1_stop_playback() called when already stopped")
    }

    thread_mutex_lock(ctx->seek_lock);
    if (ctx->do_seek == ctx->settings->codec_target) {
        thread_mutex_unlock(ctx->seek_lock);
        RAISE(ctx, "sav1_stop_playback() called while in the middle of seeking")
    }
    thread_mutex_unlock(ctx->seek_lock);

    if ((ctx->pause_time = (struct timespec *)malloc(sizeof(struct timespec))) == NULL) {
        RAISE_CRITICAL(ctx, "malloc() failed in sav1_stop_playback()")
    }
    int status = clock_gettime(CLOCK_MONOTONIC, ctx->pause_time);
    if (status) {
        sav1_set_error_with_code(
            ctx, "clock_gettime() in sav1_stop_playback() returned %d", status);
        return -1;
    }

    // set the playback status
    ctx->is_playing = 0;

    return 0;
}

int
sav1_is_playback_paused(Sav1Context *context, int *is_paused)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    *is_paused = !ctx->is_playing;
    return 0;
}

int
sav1_is_playback_at_file_end(Sav1Context *context, int *is_at_file_end)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    // in loop mode the file will never be over
    if (ctx->settings->on_file_end == SAV1_FILE_END_LOOP) {
        *is_at_file_end = 0;
    }
    else {
        *is_at_file_end = parse_get_status(ctx->thread_manager->parse_context) ==
                              PARSE_STATUS_END_OF_FILE &&
                          ctx->next_video_frame == NULL && ctx->next_audio_frame == NULL;
    }

    return 0;
}

int
sav1_get_playback_time(Sav1Context *context, uint64_t *timecode_ms)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    // handle when we're in the middle of seeking
    thread_mutex_lock(ctx->seek_lock);
    if (ctx->do_seek == ctx->settings->codec_target) {
        *timecode_ms = ctx->seek_timecode;
        thread_mutex_unlock(ctx->seek_lock);
        return 0;
    }
    thread_mutex_unlock(ctx->seek_lock);

    // get the video duration if it's available
    uint64_t duration = thread_manager_get_duration(ctx->thread_manager);

    // handle cases when video isn't playing
    if (!ctx->is_playing) {
        // check if video is paused or just hasn't been started
        if (ctx->pause_time == NULL) {
            *timecode_ms = 0;
        }
        else {
            // calculate the offset from the pause time to the start time
            *timecode_ms =
                (((ctx->pause_time->tv_sec - ctx->start_time->tv_sec) * 1000) +
                 ((ctx->pause_time->tv_nsec - ctx->start_time->tv_nsec) / 1000000)) *
                ctx->settings->playback_speed;
            if (duration && duration < *timecode_ms) {
                *timecode_ms = duration;
            }
        }

        return 0;
    }

    // get the current time
    struct timespec curr_time;
    int status = clock_gettime(CLOCK_MONOTONIC, &curr_time);
    if (status) {
        sav1_set_error_with_code(
            ctx, "clock_gettime() in sav1_get_playback_time() returned %d", status);
        return -1;
    }

    // calculate the offset from the current time to the start time
    *timecode_ms = (((curr_time.tv_sec - ctx->start_time->tv_sec) * 1000) +
                    ((curr_time.tv_nsec - ctx->start_time->tv_nsec) / 1000000)) *
                   ctx->settings->playback_speed;
    if (duration && duration < *timecode_ms) {
        *timecode_ms = duration;
    }

    return 0;
}

int
sav1_get_playback_duration(Sav1Context *context, uint64_t *duration_ms)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    uint64_t duration = thread_manager_get_duration(ctx->thread_manager);
    if (duration == 0) {
        RAISE(ctx, "Duration is unknown in call to sav1_get_playback_duration()")
    }
    *duration_ms = duration;

    return 0;
}

int
sav1_set_playback_speed(Sav1Context *context, double playback_speed)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    if (ctx->settings->playback_mode == SAV1_PLAYBACK_FAST) {
        RAISE(ctx, "sav1_set_playback_speed() called while in SAV1_PLAYBACK_FAST mode")
    }

    thread_mutex_lock(ctx->seek_lock);
    if (ctx->do_seek == ctx->settings->codec_target) {
        thread_mutex_unlock(ctx->seek_lock);
        RAISE(ctx, "sav1_set_playback_speed() called while in the middle of seeking")
    }
    thread_mutex_unlock(ctx->seek_lock);

    // check with a value slightly above 0 to avoid floating point precision issues
    if (playback_speed < 0.0000001) {
        RAISE(ctx, "sav1_set_playback_speed() must be called with playback_speed > 0")
    }

    // calculate adjusted playback_time
    uint64_t prev_playback_time;
    sav1_get_playback_time(context, &prev_playback_time);
    prev_playback_time /= playback_speed;

    // update playback speed
    ctx->settings->playback_speed = playback_speed;

    // update start time and pause time based on prev_playback_time
    struct timespec curr_time;
    clock_gettime(CLOCK_MONOTONIC, &curr_time);

    if (!ctx->is_playing && ctx->pause_time != NULL) {
        ctx->pause_time->tv_sec = curr_time.tv_sec;
        ctx->pause_time->tv_nsec = curr_time.tv_nsec;
    }

    ctx->start_time->tv_sec = curr_time.tv_sec - prev_playback_time / 1000;
    uint64_t timecode_ns = (prev_playback_time % 1000) * 1000000;
    if (timecode_ns > (uint64_t)curr_time.tv_nsec) {
        ctx->start_time->tv_sec--;
        ctx->start_time->tv_nsec = curr_time.tv_nsec + 1000000000 - timecode_ns;
    }
    else {
        ctx->start_time->tv_nsec = curr_time.tv_nsec - timecode_ns;
    }

    return 0;
}

int
sav1_get_playback_speed(Sav1Context *context, double *playback_speed)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    if (ctx->settings->playback_mode == SAV1_PLAYBACK_FAST) {
        RAISE(ctx, "sav1_get_playback_speed() called while in SAV1_PLAYBACK_FAST mode")
    }

    *playback_speed = ctx->settings->playback_speed;
    return 0;
}

int
sav1_seek_playback(Sav1Context *context, uint64_t timecode_ms, int seek_mode)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    // don't seek if we're already doing it
    thread_mutex_lock(ctx->seek_lock);
    if (ctx->do_seek) {
        if (parse_get_status(ctx->thread_manager->parse_context) ==
                PARSE_STATUS_END_OF_FILE &&
            thread_atomic_int_load(&(ctx->thread_manager->parse_context->do_seek)) == 0) {
            seek_update_start_time(ctx);
            ctx->do_seek = 0;
        }
        else {
            thread_mutex_unlock(ctx->seek_lock);
            RAISE(ctx, "sav1_seek_playback() called while already seeking")
        }
    }
    thread_mutex_unlock(ctx->seek_lock);

    // update the atomic seek mode variable
    thread_atomic_int_store(&(ctx->seek_mode), seek_mode);

    // make the thread manager do all the hard work
    thread_manager_seek_to_time(ctx->thread_manager, timecode_ms);

    // save the time that we want to seek to
    ctx->seek_timecode = timecode_ms;

    // remove all the currently queued frames
    if (ctx->curr_video_frame != NULL && ctx->curr_video_frame->sav1_has_ownership) {
        sav1_video_frame_destroy(context, ctx->curr_video_frame);
        ctx->curr_video_frame = NULL;
    }
    if (ctx->next_video_frame != NULL && ctx->next_video_frame->sav1_has_ownership) {
        sav1_video_frame_destroy(context, ctx->next_video_frame);
        ctx->next_video_frame = NULL;
    }
    if (ctx->curr_audio_frame != NULL && ctx->curr_audio_frame->sav1_has_ownership) {
        sav1_audio_frame_destroy(context, ctx->curr_audio_frame);
        ctx->curr_audio_frame = NULL;
    }
    if (ctx->next_audio_frame != NULL && ctx->next_audio_frame->sav1_has_ownership) {
        sav1_audio_frame_destroy(context, ctx->next_audio_frame);
        ctx->next_audio_frame = NULL;
    }

    ctx->video_frame_ready = 0;
    ctx->audio_frame_ready = 0;

    thread_mutex_lock(ctx->seek_lock);
    ctx->do_seek = ctx->settings->codec_target;
    ctx->end_of_file = 0;
    thread_mutex_unlock(ctx->seek_lock);

    return 0;
}

void
sav1_get_version(int *major, int *minor, int *patch)
{
    *major = SAV1_MAJOR_VERSION;
    *minor = SAV1_MINOR_VERSION;
    *patch = SAV1_PATCH_VERSION;
}

const char *
sav1_get_dav1d_version()
{
    return dav1d_version();
}

const char *
sav1_get_opus_version()
{
    return opus_get_version_string();
}
