#include <string.h>

#include "sav1.h"
#include "sav1_internal.h"

void
sav1_set_error(Sav1InternalContext *ctx, char *message)
{
    int message_len = strlen(message);
    assert(message_len < 128);
    memcpy(ctx->error_message, message, message_len);
    ctx->error_message[message_len] = '\0';
}

void
sav1_set_error_with_code(Sav1InternalContext *ctx, char *message, int code)
{
    snprintf(ctx->error_message, 128, message, code);
}
