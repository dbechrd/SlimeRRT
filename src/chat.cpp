#include "chat.h"
#include "error.h"
#include "net_message.h"
#include "raylib/raylib.h"
#include <cassert>
#include <cstring>

const char *ChatHistory::LOG_SRC = "Chat";

// TODO: Refactor this out into net_client / net_server, just memcpy the net message into the buffer
void ChatHistory::PushNetMessage(const NetMessage_ChatMessage &netChat)
{
    NetMessage_ChatMessage &chat = buffer.Alloc();
    memcpy(&chat, &netChat, sizeof(netChat));
    assert(!chat.timestampStr[0]); // If this triggers, FYI, your timestamp will be overwritten
    const char *timestampStr = TextFormatTimestamp();
    memcpy(chat.timestampStr, timestampStr, sizeof(timestampStr));
}

void ChatHistory::PushMessage(const char *username, size_t usernameLength, const char *message, size_t messageLength)
{
    assert(username);
    assert(usernameLength);
    assert(usernameLength <= USERNAME_LENGTH_MAX);
    assert(message);
    assert(messageLength);
    assert(messageLength <= CHAT_MESSAGE_LENGTH_MAX);

    NetMessage_ChatMessage &chat = buffer.Alloc();
    const char *timestampStr = TextFormatTimestamp();
    memcpy(chat.timestampStr, timestampStr, sizeof(timestampStr));
    chat.usernameLength = (uint32_t)MIN(usernameLength, USERNAME_LENGTH_MAX);
    memcpy(chat.username, username, chat.usernameLength);
    chat.messageLength = (uint32_t)MIN(messageLength, CHAT_MESSAGE_LENGTH_MAX);
    memcpy(chat.message, message, chat.messageLength);
}

void ChatHistory::Render(Font &font, Rectangle rect)
{
    size_t chatMsgCount = buffer.Count();
    if (!chatMsgCount) {
        return;
    }

    const float margin = 6.0f;   // left/bottom margin
    const float pad = 4.0f;      // left/bottom pad

    // NOTE: The chat history renders from the bottom up (most recent message first)
    float cursorY = (rect.y + rect.height) - pad - font.baseSize;
    const char *chatText = 0;
    Color chatColor = WHITE;

    DrawRectangleRec(rect, Fade(DARKGRAY, 0.8f));
    DrawRectangleLinesEx(rect, 1, Fade(BLACK, 0.8f));
    //DrawRectangle((int)chatX, (int)chatY, (int)chatWidth, (int)chatHeight, Fade(DARKGRAY, 0.8f));
    //DrawRectangleLines((int)chatX, (int)chatY, (int)chatWidth, (int)chatHeight, Fade(BLACK, 0.8f));

    for (int i = (int)chatMsgCount - 1; i >= 0; i--) {
        const NetMessage_ChatMessage &chatMsg = buffer.At(i);
        assert(chatMsg.messageLength);

#define CHECK_USERNAME(value) chatMsg.usernameLength == (sizeof(value) - 1) && !strncmp(chatMsg.username, (value), chatMsg.usernameLength);
        bool fromServer = CHECK_USERNAME("SERVER");
        bool fromSam = CHECK_USERNAME("Sam");
        bool fromDebug = CHECK_USERNAME("Debug");
        bool fromMotd = CHECK_USERNAME("Message of the day");
#undef CHECK_USERNAME

        if (fromServer || fromSam || fromDebug || fromMotd) {
            chatText = TextFormat("[%s]<%.*s>: %.*s", chatMsg.timestampStr, chatMsg.usernameLength, chatMsg.username,
                chatMsg.messageLength, chatMsg.message);
            if (fromServer) {
                chatColor = RED;
            } else if (fromSam) {
                chatColor = GREEN;
            } else if (fromDebug) {
                chatColor = LIGHTGRAY;
            } else if (fromMotd) {
                chatColor = YELLOW;
            }
        } else {
            chatText = TextFormat("[%s][%.*s]: %.*s", chatMsg.timestampStr, chatMsg.usernameLength, chatMsg.username,
                chatMsg.messageLength, chatMsg.message);
            chatColor = WHITE;
        }
        DrawTextFont(font, chatText, margin + pad, cursorY, font.baseSize, chatColor);
        cursorY -= font.baseSize + pad;
    }
}