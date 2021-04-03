#include "chat.h"
#include "packet.h"
#include "raylib.h"
#include <cassert>
#include <cstdlib>
#include <cstring>

int chat_history_init(ChatHistory *chatHistory)
{
    size_t capacity = CHAT_MESSAGE_HISTORY;
    chatHistory->messages = (ChatMessage *)calloc(capacity, sizeof(*chatHistory->messages));
    if (!chatHistory) {
        TraceLog(LOG_FATAL, "[Chat] Failed to chat history buffer.\n");
        return 0;
    }
    chatHistory->capacity = capacity;
    return 1;
}

static ChatMessage *chat_history_message_alloc(ChatHistory *chatHistory)
{
    size_t messageIdx = (chatHistory->first + chatHistory->count) % chatHistory->capacity;
    assert(messageIdx < chatHistory->capacity);
    ChatMessage *message = &chatHistory->messages[messageIdx];
    if (chatHistory->count < chatHistory->capacity) {
        chatHistory->count++;
    } else {
        chatHistory->first = (chatHistory->first + 1) % chatHistory->capacity;
        memset(message, 0, sizeof(*message));
    }
    return message;
}

void chat_history_push_net_message(ChatHistory *chatHistory, const NetMessage_ChatMessage *netChat)
{
    ChatMessage *chat = chat_history_message_alloc(chatHistory);

    assert(netChat->usernameLength <= USERNAME_LENGTH_MAX);
    chat->usernameLength = MIN(netChat->usernameLength, USERNAME_LENGTH_MAX);
    memcpy(chat->username, netChat->username, chat->usernameLength);

    assert(netChat->messageLength <= CHAT_MESSAGE_LENGTH_MAX);
    chat->messageLength = MIN(netChat->messageLength, CHAT_MESSAGE_LENGTH_MAX);
    memcpy(chat->message, netChat->message, chat->messageLength);
}

void chat_history_free(ChatHistory *chatHistory)
{
    free(chatHistory->messages);
    memset(chatHistory, 0, sizeof(*chatHistory));
}