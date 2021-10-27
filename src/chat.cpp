#include "chat.h"
#include "error.h"
#include "net_message.h"
#include "packet.h"
#include "raylib/raylib.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <new>

const char *ChatHistory::LOG_SRC = "Chat";

void ChatHistory::PushNetMessage(const NetMessage_ChatMessage &netChat)
{
    assert(netChat.username);
    assert(netChat.usernameLength);
    assert(netChat.message);
    assert(netChat.messageLength);

    ChatMessage &chat = Alloc();

    assert(sizeof(chat.timestampStr) == sizeof(netChat.timestampStr));
    memcpy(chat.timestampStr, netChat.timestampStr, sizeof(netChat.timestampStr));

    assert(netChat.usernameLength <= USERNAME_LENGTH_MAX);
    chat.usernameLength = MIN(netChat.usernameLength, USERNAME_LENGTH_MAX);
    memcpy(chat.username, netChat.username, chat.usernameLength);

    assert(netChat.messageLength <= CHAT_MESSAGE_LENGTH_MAX);
    chat.messageLength = MIN(netChat.messageLength, CHAT_MESSAGE_LENGTH_MAX);
    memcpy(chat.message, netChat.message, chat.messageLength);
}

void ChatHistory::PushMessage(const char *username, size_t usernameLength, const char *message, size_t messageLength)
{
    assert(username);
    assert(usernameLength);
    assert(message);
    assert(messageLength);

    ChatMessage &chat = Alloc();

    char timestampStr[12];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    int len = snprintf(timestampStr, sizeof(timestampStr), "%02d:%02d:%02d", tm.tm_hour,
        tm.tm_min, tm.tm_sec);
    assert(len < sizeof(timestampStr));

    assert(sizeof(chat.timestampStr) == sizeof(timestampStr));
    memcpy(chat.timestampStr, timestampStr, sizeof(timestampStr));

    assert(usernameLength <= USERNAME_LENGTH_MAX);
    chat.usernameLength = MIN(usernameLength, USERNAME_LENGTH_MAX);
    memcpy(chat.username, username, chat.usernameLength);

    assert(messageLength <= CHAT_MESSAGE_LENGTH_MAX);
    chat.messageLength = MIN(messageLength, CHAT_MESSAGE_LENGTH_MAX);
    memcpy(chat.message, message, chat.messageLength);
}