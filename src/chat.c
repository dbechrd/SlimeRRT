#include "chat.h"
#include "raylib.h"
#include <stdlib.h>

// must be power of 2 (shift modulus ring buffer)
#define MAX_CHAT_MESSAGES 64

static chat_message *chatHistory;

int chat_init(void)
{
    chatHistory = calloc(MAX_CHAT_MESSAGES, sizeof(*chatHistory));
    if (!chatHistory) {
        TraceLog(LOG_FATAL, "[Chat] Failed to chat history buffer.\n");
        return 0;
    }
    return 1;
}

void chat_free(void)
{
    free(chatHistory);
}