#include "sav1.h"
#include "sav1_internal.h"

int
sav1_start_playback(Sav1Context *context)
{
    Sav1InternalContext *ctx = (Sav1InternalContext *)context;

    if (ctx->playing) {
        set_error(ctx, "sav1_start_playback() called when already playing");
        return -1;
    }
}