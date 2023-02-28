#define THREAD_IMPLEMENTATION
#include "thread.h"
#include "thread_queue.h"
#include "sav1_internal.h"

void
sav1_thread_queue_init(Sav1ThreadQueue **sav1_queue, Sav1InternalContext *ctx,
                       size_t capacity)
{
    if (((*sav1_queue) = (Sav1ThreadQueue *)malloc(sizeof(Sav1ThreadQueue))) == NULL) {
        sav1_set_error(ctx, "malloc() failed in sav1_thread_queue_init()");
        sav1_set_critical_error_flag(ctx);
        return;
    }

    if (((*sav1_queue)->data = (void **)malloc(capacity * sizeof(void *))) == NULL) {
        free(*sav1_queue);
        sav1_set_error(ctx, "malloc() failed in sav1_thread_queue_init()");
        sav1_set_critical_error_flag(ctx);
        return;
    }
    (*sav1_queue)->capacity = capacity;
    if (((*sav1_queue)->queue = (thread_queue_t *)malloc(sizeof(thread_queue_t))) ==
        NULL) {
        free((*sav1_queue)->data);
        free(*sav1_queue);
        sav1_set_error(ctx, "malloc() failed in sav1_thread_queue_init()");
        sav1_set_critical_error_flag(ctx);
        return;
    }
    if (((*sav1_queue)->push_lock = (thread_mutex_t *)malloc(sizeof(thread_mutex_t))) ==
        NULL) {
        free((*sav1_queue)->queue);
        free((*sav1_queue)->data);
        free((*sav1_queue));
        sav1_set_error(ctx, "malloc() failed in sav1_thread_queue_init()");
        sav1_set_critical_error_flag(ctx);
        return;
    }
    if (((*sav1_queue)->pop_lock = (thread_mutex_t *)malloc(sizeof(thread_mutex_t))) ==
        NULL) {
        free((*sav1_queue)->push_lock);
        free((*sav1_queue)->queue);
        free((*sav1_queue)->data);
        free((*sav1_queue));
        sav1_set_error(ctx, "malloc() failed in sav1_thread_queue_init()");
        sav1_set_critical_error_flag(ctx);
        return;
    }
    (*sav1_queue)->ctx = ctx;
    thread_queue_init((*sav1_queue)->queue, capacity, (*sav1_queue)->data, 0);
    thread_mutex_init((*sav1_queue)->push_lock);
    thread_mutex_init((*sav1_queue)->pop_lock);
}

void
sav1_thread_queue_destroy(Sav1ThreadQueue *sav1_queue)
{
    thread_queue_term(sav1_queue->queue);
    thread_mutex_term(sav1_queue->push_lock);
    thread_mutex_term(sav1_queue->pop_lock);
    free(sav1_queue->queue);
    free(sav1_queue->push_lock);
    free(sav1_queue->pop_lock);
    free(sav1_queue->data);
    free(sav1_queue);
}

void *
sav1_thread_queue_pop(Sav1ThreadQueue *sav1_queue)
{
    thread_mutex_lock(sav1_queue->pop_lock);
    void *item = thread_queue_consume(sav1_queue->queue, THREAD_QUEUE_WAIT_INFINITE);
    thread_mutex_unlock(sav1_queue->pop_lock);
    return item;
}

void
sav1_thread_queue_push(Sav1ThreadQueue *sav1_queue, void *item)
{
    thread_mutex_lock(sav1_queue->push_lock);
    thread_queue_produce(sav1_queue->queue, item, THREAD_QUEUE_WAIT_INFINITE);
    thread_mutex_unlock(sav1_queue->push_lock);
}

void
sav1_thread_queue_lock(Sav1ThreadQueue *sav1_queue)
{
    if (sav1_queue != NULL) {
        thread_mutex_lock(sav1_queue->push_lock);
        thread_mutex_lock(sav1_queue->pop_lock);
    }
}

void
sav1_thread_queue_unlock(Sav1ThreadQueue *sav1_queue)
{
    if (sav1_queue != NULL) {
        thread_mutex_unlock(sav1_queue->push_lock);
        thread_mutex_unlock(sav1_queue->pop_lock);
    }
}

int
sav1_thread_queue_get_size(Sav1ThreadQueue *sav1_queue)
{
    if (sav1_queue != NULL) {
        return thread_queue_count(sav1_queue->queue);
    }

    return -1;
}

void *
sav1_thread_queue_pop_timeout(Sav1ThreadQueue *sav1_queue)
{
    if (sav1_thread_queue_get_size(sav1_queue) == 0) {
        return NULL;
    }
    thread_mutex_lock(sav1_queue->pop_lock);
    void *item = thread_queue_consume(sav1_queue->queue, 5);
    thread_mutex_unlock(sav1_queue->pop_lock);
    return item;
}

void
sav1_thread_queue_push_timeout(Sav1ThreadQueue *sav1_queue, void *item)
{
    thread_mutex_lock(sav1_queue->push_lock);
    thread_queue_produce(sav1_queue->queue, item, 5);
    thread_mutex_unlock(sav1_queue->push_lock);
}
