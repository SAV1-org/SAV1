#include "sav1.h"
#include "sav1_internal.h"

#include <string.h>
#include <cstdio>
#include <cassert>

void
sav1_set_error(Sav1InternalContext *ctx, const char *message)
{   
    int message_len = strlen(message);
    assert(message_len < SAV1_ERROR_MESSAGE_SIZE);
    memcpy(ctx->error_message, message, message_len);
    ctx->error_message[message_len] = '\0';
}

void
sav1_set_error_with_code(Sav1InternalContext *ctx, const char *message, int code)
{
    snprintf(ctx->error_message, SAV1_ERROR_MESSAGE_SIZE, message, code);
}

void
sav1_set_critical_error_flag(Sav1InternalContext *ctx)
{
}
