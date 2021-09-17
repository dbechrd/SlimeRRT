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
    ChatMessage &chat = Alloc();

    assert(netChat.usernameLength <= USERNAME_LENGTH_MAX);
    chat.usernameLength = MIN(netChat.usernameLength, USERNAME_LENGTH_MAX);
    memcpy(chat.username, netChat.username, chat.usernameLength);

    assert(netChat.messageLength <= CHAT_MESSAGE_LENGTH_MAX);
    chat.messageLength = MIN(netChat.messageLength, CHAT_MESSAGE_LENGTH_MAX);
    memcpy(chat.message, netChat.message, chat.messageLength);
}