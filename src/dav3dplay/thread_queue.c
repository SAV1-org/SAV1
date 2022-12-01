#define THREAD_IMPLEMENTATION
#include "thread.h"
#include "thread_queue.h"

void
sav1_thread_queue_init(Sav1ThreadQueue **sav1_queue, size_t capacity)
{
    *sav1_queue = (Sav1ThreadQueue *)malloc(sizeof(Sav1ThreadQueue));
    (*sav1_queue)->data = (void **)malloc(capacity * sizeof(void *));
    (*sav1_queue)->capacity = capacity;
    (*sav1_queue)->queue = (thread_queue_t *)malloc(sizeof(thread_queue_t));
    (*sav1_queue)->queue_lock = (thread_mutex_t *)malloc(sizeof(thread_mutex_t));
    thread_queue_init((*sav1_queue)->queue, capacity, (*sav1_queue)->data, 0);
    thread_mutex_init((*sav1_queue)->queue_lock);
}

void
sav1_thread_queue_destroy(Sav1ThreadQueue *sav1_queue)
{
    thread_queue_term(sav1_queue->queue);
    thread_mutex_term(sav1_queue->queue_lock);
    free(sav1_queue->queue);
    free(sav1_queue->queue_lock);
    free(sav1_queue->data);
    free(sav1_queue);
}

void *
sav1_thread_queue_pop(Sav1ThreadQueue *sav1_queue)
{
    thread_mutex_lock(sav1_queue->queue_lock);
    thread_mutex_unlock(sav1_queue->queue_lock);

    return thread_queue_consume(sav1_queue->queue, THREAD_QUEUE_WAIT_INFINITE);
}

void
sav1_thread_queue_push(Sav1ThreadQueue *sav1_queue, void *item)
{
    thread_mutex_lock(sav1_queue->queue_lock);
    thread_mutex_unlock(sav1_queue->queue_lock);

    thread_queue_produce(sav1_queue->queue, item, THREAD_QUEUE_WAIT_INFINITE);
}

void
sav1_thread_queue_lock(Sav1ThreadQueue *sav1_queue)
{
    thread_mutex_lock(sav1_queue->queue_lock);
}

void
sav1_thread_queue_unlock(Sav1ThreadQueue *sav1_queue)
{
    thread_mutex_unlock(sav1_queue->queue_lock);
}

int
sav1_thread_queue_get_size(Sav1ThreadQueue *sav1_queue)
{
    return thread_queue_count(sav1_queue->queue);
}

void *
sav1_thread_queue_pop_timeout(Sav1ThreadQueue *sav1_queue)
{
    thread_mutex_lock(sav1_queue->queue_lock);
    thread_mutex_unlock(sav1_queue->queue_lock);

    return thread_queue_consume(sav1_queue->queue, 10);
}

void
sav1_thread_queue_push_timeout(Sav1ThreadQueue *sav1_queue, void *item)
{
    thread_mutex_lock(sav1_queue->queue_lock);
    thread_mutex_unlock(sav1_queue->queue_lock);

    thread_queue_produce(sav1_queue->queue, item, 10);
}
