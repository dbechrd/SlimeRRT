#include "chat.h"
#include "error.h"
#include "net_message.h"
#include "packet.h"
#include "raylib.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <new>

static const char *LOG_SRC = "Chat";

ErrorType ChatHistory::Init()
{
E_START
    messages = (ChatMessage *)calloc(CHAT_MESSAGE_HISTORY, sizeof(*messages));
    E_CHECK_ALLOC(messages, "Failed to allocate chat message history buffer");
    for (size_t i = 0; i < CHAT_MESSAGE_HISTORY; i++) {
        new(messages + i) ChatMessage{};
    }
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
        *message = {};
        //*message = ChatMessage();
    }
    return message;
}

void ChatHistory::PushNetMessage(const NetMessage_ChatMessage &netChat)
{
    ChatMessage *chat = Alloc();

    assert(netChat.usernameLength <= USERNAME_LENGTH_MAX);
    chat->usernameLength = MIN(netChat.usernameLength, USERNAME_LENGTH_MAX);
    memcpy(chat->username, netChat.username, chat->usernameLength);

    assert(netChat.messageLength <= CHAT_MESSAGE_LENGTH_MAX);
    chat->messageLength = MIN(netChat.messageLength, CHAT_MESSAGE_LENGTH_MAX);
    memcpy(chat->message, netChat.message, chat->messageLength);
}