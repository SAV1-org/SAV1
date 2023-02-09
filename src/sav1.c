#include "sav1.h"
#include "sav1_internal.h"

#include <string.h>
#include <time.h>
#include <stdio.h>

#define CHECK_CTX_VALID(ctx) \
    if (ctx == NULL) {       \
        return -1;           \
    }

#define CHECK_CTX_INITIALIZED(ctx, context)                                             \
    if (context->is_initialized != 1) {                                                 \
        sav1_set_error(ctx, "Uninitialized context: sav1_create_context() not called"); \
        return -1;                                                                      \
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

    ctx->critical_error_flag = 0;
    ctx->is_playing = 0;
    ctx->start_time = (struct timespec *)malloc(sizeof(struct timespec));
    ctx->pause_time = NULL;
    ctx->curr_video_frame = NULL;
    ctx->next_video_frame = NULL;
    ctx->video_frame_ready = 0;
    ctx->curr_audio_frame = NULL;
    ctx->next_audio_frame = NULL;
    ctx->audio_frame_ready = 0;

    memcpy(&ctx->settings, &settings, sizeof(settings));
    memset(ctx->error_message, 0, SAV1_ERROR_MESSAGE_SIZE);

    // TODO: error check these eventually
    thread_manager_init(&(ctx->thread_manager), ctx);
    thread_manager_start_pipeline(ctx->thread_manager);

    context->internal_state = (void *)ctx;
    context->is_initialized = 1;

    return 0;
}

int
sav1_destroy_context(Sav1Context *context)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)

    // TODO: error check these eventually
    thread_manager_kill_pipeline(ctx->thread_manager);
    thread_manager_destroy(ctx->thread_manager);

    free(ctx->start_time);
    if (ctx->pause_time != NULL) {
        free(ctx->pause_time);
    }

    if (ctx->curr_video_frame != NULL) {
        sav1_video_frame_destroy(context, ctx->curr_video_frame);
    }

    if (ctx->next_video_frame != NULL) {
        sav1_video_frame_destroy(context, ctx->next_video_frame);
    }

    if (ctx->curr_audio_frame != NULL) {
        sav1_audio_frame_destroy(context, ctx->curr_audio_frame);
    }

    if (ctx->next_audio_frame != NULL) {
        sav1_audio_frame_destroy(context, ctx->next_audio_frame);
    }

    free(ctx);
    context->is_initialized = 0;

    return 0;
}

char *
sav1_get_error(Sav1Context *context)
{
    if (context == NULL) {
        return (char *)"Sav1Context context passed as NULL";
    }
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;

    if (ctx == NULL) {
        return (char *)"Uninitialized context: sav1_create_context() not called";
    }
    else {
        return ctx->error_message;
    }
}

void
sav1_pump_video_frames(Sav1Context *context, uint64_t curr_ms)
{
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;

    // If we have no next frame, try to get one
    if (ctx->next_video_frame == NULL) {
        if (sav1_thread_queue_get_size(ctx->thread_manager->video_output_queue) != 0) {
            ctx->next_video_frame = (Sav1VideoFrame *)sav1_thread_queue_pop(
                ctx->thread_manager->video_output_queue);
        }
    }

    // While we have a next frame, and the next frame is ahead of current time
    while (ctx->next_video_frame != NULL && ctx->next_video_frame->timecode < curr_ms) {
        // Clean up current frame before replacing it
        if (ctx->curr_video_frame != NULL) {
            sav1_video_frame_destroy(context, ctx->curr_video_frame);
        }

        ctx->curr_video_frame = ctx->next_video_frame;
        ctx->video_frame_ready = 1;  // mark ready, there is new content
        ctx->next_video_frame = NULL;

        // Try to get new next frame from queue
        if (sav1_thread_queue_get_size(ctx->thread_manager->video_output_queue) != 0) {
            ctx->next_video_frame = (Sav1VideoFrame *)sav1_thread_queue_pop(
                ctx->thread_manager->video_output_queue);
        }
    }
}

void
sav1_pump_audio_frames(Sav1Context *context, uint64_t curr_ms)
{
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;

    // If we have no next frame, try to get one
    if (ctx->next_audio_frame == NULL) {
        if (sav1_thread_queue_get_size(ctx->thread_manager->audio_output_queue) != 0) {
            ctx->next_audio_frame = (Sav1AudioFrame *)sav1_thread_queue_pop(
                ctx->thread_manager->audio_output_queue);
        }
    }

    // While we have a next frame, and the next frame is ahead of current time
    while (ctx->next_audio_frame != NULL && ctx->next_audio_frame->timecode < curr_ms) {
        // Clean up current frame before replacing it
        if (ctx->curr_audio_frame != NULL) {
            sav1_audio_frame_destroy(context, ctx->curr_audio_frame);
        }

        ctx->curr_audio_frame = ctx->next_audio_frame;
        ctx->audio_frame_ready = 1;  // mark ready, there is new content
        ctx->next_audio_frame = NULL;

        // Try to get new next frame from queue
        if (sav1_thread_queue_get_size(ctx->thread_manager->audio_output_queue) != 0) {
            ctx->next_audio_frame = (Sav1AudioFrame *)sav1_thread_queue_pop(
                ctx->thread_manager->audio_output_queue);
        }
    }
}

int
sav1_get_video_frame(Sav1Context *context, Sav1VideoFrame **frame)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    if (ctx->settings->codec_target & SAV1_CODEC_AV1 == 0) {
        RAISE(ctx, "Can't get video when not targeting video in settings")
    }

    uint64_t curr_ms;
    sav1_get_playback_time(context, &curr_ms);
    sav1_pump_video_frames(context, curr_ms);

    *frame = ctx->curr_video_frame;
    ctx->video_frame_ready = 0;
    return 0;
}

int
sav1_get_audio_frame(Sav1Context *context, Sav1AudioFrame **frame)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    if (ctx->settings->codec_target & SAV1_CODEC_OPUS == 0) {
        RAISE(ctx, "Can't get audio when not targeting audio in settings")
    }

    uint64_t curr_ms;
    sav1_get_playback_time(context, &curr_ms);
    sav1_pump_audio_frames(context, curr_ms);

    *frame = ctx->curr_audio_frame;
    ctx->audio_frame_ready = 0;
    return 0;
}

int
sav1_get_video_frame_ready(Sav1Context *context, int *is_ready)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    if (ctx->settings->codec_target & SAV1_CODEC_AV1 == 0) {
        RAISE(ctx, "Can't get video when not targeting video in settings")
    }

    uint64_t curr_ms;
    sav1_get_playback_time(context, &curr_ms);
    sav1_pump_video_frames(context, curr_ms);

    *is_ready = ctx->video_frame_ready;

    return 0;
}

int
sav1_get_audio_frame_ready(Sav1Context *context, int *is_ready)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    if (ctx->settings->codec_target & SAV1_CODEC_OPUS == 0) {
        RAISE(ctx, "Can't get audio when not targeting audio in settings")
    }

    uint64_t curr_ms;
    sav1_get_playback_time(context, &curr_ms);
    sav1_pump_audio_frames(context, curr_ms);

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
sav1_get_playback_time(Sav1Context *context, uint64_t *timecode_ms)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    // handle cases when video isn't playing
    if (!ctx->is_playing) {
        // check if video is paused or just hasn't been started
        if (ctx->pause_time == NULL) {
            RAISE(ctx,
                  "sav1_get_playback_time() called without calling sav1_start_playback()")
        }

        // calculate the offset from the pause time to the start time
        *timecode_ms = ((ctx->pause_time->tv_sec - ctx->start_time->tv_sec) * 1000) +
                       ((ctx->pause_time->tv_nsec - ctx->start_time->tv_nsec) / 1000000);
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
    *timecode_ms = ((curr_time.tv_sec - ctx->start_time->tv_sec) * 1000) +
                   ((curr_time.tv_nsec - ctx->start_time->tv_nsec) / 1000000);
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
sav1_seek_playback(Sav1Context *context, uint64_t timecode_ms)
{
    CHECK_CONTEXT_VALID(context)
    Sav1InternalContext *ctx = (Sav1InternalContext *)context->internal_state;
    CHECK_CTX_VALID(ctx)
    CHECK_CTX_INITIALIZED(ctx, context)
    CHECK_CTX_CRITICAL_ERROR(ctx)

    thread_manager_seek_to_time(ctx->thread_manager, timecode_ms);

    struct timespec curr_time;
    clock_gettime(CLOCK_MONOTONIC, &curr_time);
    ctx->start_time->tv_sec = curr_time.tv_sec - timecode_ms / 1000;
    uint64_t timecode_ns = (timecode_ms % 1000) * 1000000;
    if (timecode_ns > curr_time.tv_nsec) {
        ctx->start_time->tv_sec--;
        ctx->start_time->tv_nsec = curr_time.tv_nsec + 1000000000 - timecode_ns;
    }
    else {
        ctx->start_time->tv_nsec = curr_time.tv_nsec - timecode_ns;
    }

    // remove all our currently queued frames
    if (ctx->curr_video_frame != NULL) {
        sav1_video_frame_destroy(context, ctx->curr_video_frame);
        ctx->curr_video_frame = NULL;
    }
    if (ctx->next_video_frame != NULL) {
        sav1_video_frame_destroy(context, ctx->next_video_frame);
        ctx->next_video_frame = NULL;
    }
    if (ctx->curr_audio_frame != NULL) {
        sav1_audio_frame_destroy(context, ctx->curr_audio_frame);
        ctx->curr_audio_frame = NULL;
    }
    if (ctx->next_audio_frame != NULL) {
        sav1_audio_frame_destroy(context, ctx->next_audio_frame);
        ctx->next_audio_frame = NULL;
    }

    printf("A\n");

    int frame_needs = ctx->settings->codec_target;
    while (frame_needs) {
        if (frame_needs & SAV1_CODEC_AV1 &&
            sav1_thread_queue_get_size(ctx->thread_manager->video_output_queue) > 0) {
            ctx->curr_video_frame = (Sav1VideoFrame *)sav1_thread_queue_pop(
                ctx->thread_manager->video_output_queue);
            if (ctx->curr_video_frame->sentinel) {
                sav1_video_frame_destroy(context, ctx->curr_video_frame);
            }
            else {
                frame_needs ^= SAV1_CODEC_AV1;
            }
        }

        if (frame_needs & SAV1_CODEC_OPUS &&
            sav1_thread_queue_get_size(ctx->thread_manager->audio_output_queue) > 0) {
            ctx->curr_audio_frame = (Sav1AudioFrame *)sav1_thread_queue_pop(
                ctx->thread_manager->audio_output_queue);
            if (ctx->curr_audio_frame->sentinel) {
                sav1_audio_frame_destroy(context, ctx->curr_audio_frame);
            }
            else {
                frame_needs ^= SAV1_CODEC_OPUS;
            }
        }

        printf("C\n");
    }

    printf("B\n");

    return 0;
}
