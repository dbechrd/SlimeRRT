#include "chat.h"
#include "error.h"
#include "packet.h"
#include "raylib.h"
#include <cassert>
#include <cstdlib>
#include <cstring>

static const char *LOG_SRC = "Chat";

int chat_history_init(ChatHistory *chatHistory)
{
E_START
    size_t capacity = CHAT_MESSAGE_HISTORY;
    chatHistory->messages = (ChatMessage *)calloc(capacity, sizeof(*chatHistory->messages));
    E_CHECK_ALLOC(chatHistory->messages, "Failed to allocate chat message history buffer");
    chatHistory->capacity = capacity;
E_CLEAN_END
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

void chat_history_push_net_message(ChatHistory *chatHistory, const NetMessage_ChatMessage &netChat)
{
    // TODO: Copy constructor for ChatMessage(NetMessage_ChatMessage &netChatMsg)
    ChatMessage *chat = chat_history_message_alloc(chatHistory);

    assert(netChat.m_usernameLength <= USERNAME_LENGTH_MAX);
    chat->usernameLength = MIN(netChat.m_usernameLength, USERNAME_LENGTH_MAX);
    memcpy(chat->username, netChat.m_username, chat->usernameLength);

    assert(netChat.m_messageLength <= CHAT_MESSAGE_LENGTH_MAX);
    chat->messageLength = MIN(netChat.m_messageLength, CHAT_MESSAGE_LENGTH_MAX);
    memcpy(chat->message, netChat.m_message, chat->messageLength);
}

void chat_history_free(ChatHistory *chatHistory)
{
    free(chatHistory->messages);
    memset(chatHistory, 0, sizeof(*chatHistory));
}