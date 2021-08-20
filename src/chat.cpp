#include "chat.h"
#include "error.h"
#include "net_message.h"
#include "packet.h"
#include "raylib.h"
#include <cassert>
#include <cstdlib>
#include <cstring>

static const char *LOG_SRC = "Chat";

ErrorType ChatHistory::Init()
{
E_START
    messages = (ChatMessage *)calloc(CHAT_MESSAGE_HISTORY, sizeof(*messages));
    E_CHECK_ALLOC(messages, "Failed to allocate chat message history buffer");
    capacity = CHAT_MESSAGE_HISTORY;
E_CLEAN_END
}

ChatHistory::~ChatHistory()
{
    free(messages);
}

ChatMessage *ChatHistory::Alloc()
{
    size_t messageIdx = (first + count) % capacity;
    assert(messageIdx < capacity);
    ChatMessage *message = &messages[messageIdx];
    if (count < capacity) {
        count++;
    } else {
        first = (first + 1) % capacity;
        memset(message, 0, sizeof(*message));
    }
    return message;
}

void ChatHistory::PushNetMessage(const NetMessage_ChatMessage &netChat)
{
    ChatMessage *chat = Alloc();

    assert(netChat.m_usernameLength <= USERNAME_LENGTH_MAX);
    chat->usernameLength = MIN(netChat.m_usernameLength, USERNAME_LENGTH_MAX);
    memcpy(chat->username, netChat.m_username, chat->usernameLength);

    assert(netChat.m_messageLength <= CHAT_MESSAGE_LENGTH_MAX);
    chat->messageLength = MIN(netChat.m_messageLength, CHAT_MESSAGE_LENGTH_MAX);
    memcpy(chat->message, netChat.m_message, chat->messageLength);
}