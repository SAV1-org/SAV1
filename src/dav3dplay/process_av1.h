#ifndef PROCESS_AV1_H
#define PROCESS_AV1_H

#include "thread_queue.h"
#include "decode_av1.h"
#include "convert_av1.h"

typedef struct ProcessAv1Context {
    Sav1ThreadQueue *input_queue;
    Sav1ThreadQueue *output_queue;
    thread_atomic_int_t do_process;
    DecodeAv1Context *decode_context;
} ProcessAv1Context;

void
process_av1_init(ProcessAv1Context **context, Sav1ThreadQueue *input_queue,
                 Sav1ThreadQueue *output_queue);

void
process_av1_destroy(ProcessAv1Context *context);

int
process_av1_start(void *context);

void
process_av1_stop(ProcessAv1Context *context);

#endif
