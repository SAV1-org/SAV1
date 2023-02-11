#ifndef THREAD_QUEUE_H
#define THREAD_QUEUE_H

#include <cstddef>

#include "thread.h"

typedef struct Sav1InternalContext Sav1InternalContext;

typedef struct Sav1ThreadQueue {
    void **data;
    size_t capacity;
    thread_queue_t *queue;
    thread_mutex_t *pop_lock;
    thread_mutex_t *push_lock;
    Sav1InternalContext *ctx;
} Sav1ThreadQueue;

void
sav1_thread_queue_init(Sav1ThreadQueue **sav1_queue, Sav1InternalContext *ctx,
                       size_t capacity);

void
sav1_thread_queue_destroy(Sav1ThreadQueue *sav1_queue);

void *
sav1_thread_queue_pop(Sav1ThreadQueue *sav1_queue);

void
sav1_thread_queue_push(Sav1ThreadQueue *sav1_queue, void *item);

void
sav1_thread_queue_lock(Sav1ThreadQueue *sav1_queue);

void
sav1_thread_queue_unlock(Sav1ThreadQueue *sav1_queue);

int
sav1_thread_queue_get_size(Sav1ThreadQueue *sav1_queue);

void *
sav1_thread_queue_pop_timeout(Sav1ThreadQueue *sav1_queue);

void
sav1_thread_queue_push_timeout(Sav1ThreadQueue *sav1_queue, void *item);

#endif
