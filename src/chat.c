#include "chat.h"
#include "raylib.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

int chat_history_init(ChatHistory *chatHistory)
{
    chatHistory->messages = calloc(CHAT_MESSAGE_HISTORY, sizeof(*chatHistory));
    if (!chatHistory) {
        TraceLog(LOG_FATAL, "[Chat] Failed to chat history buffer.\n");
        return 0;
    }
    chatHistory->capacity = CHAT_MESSAGE_HISTORY;
    return 1;
}

ChatMessage *chat_history_message_alloc(ChatHistory *chatHistory)
{
    size_t messageIdx = (chatHistory->first + chatHistory->count) % CHAT_MESSAGE_HISTORY;
    assert(messageIdx < CHAT_MESSAGE_HISTORY);
    ChatMessage *message = &chatHistory->messages[messageIdx];
    if (chatHistory->count < chatHistory->capacity) {
        chatHistory->count++;
    } else {
        memset(message, 0, sizeof(*message));
    }
    return message;
}

void chat_history_free(ChatHistory *chatHistory)
{
    free(chatHistory->messages);
    memset(chatHistory, 0, sizeof(*chatHistory));
}