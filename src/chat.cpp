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
    chat.recvAt = glfwGetTime();
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
    assert(messageLength <= CHATMSG_LENGTH_MAX);

    NetMessage_ChatMessage &chat = buffer.Alloc();
    chat.recvAt = glfwGetTime();
    const char *timestampStr = TextFormatTimestamp();
    memcpy(chat.timestampStr, timestampStr, sizeof(timestampStr));
    chat.usernameLength = (uint32_t)MIN(usernameLength, USERNAME_LENGTH_MAX);
    memcpy(chat.username, username, chat.usernameLength);
    chat.messageLength = (uint32_t)MIN(messageLength, CHATMSG_LENGTH_MAX);
    memcpy(chat.message, message, chat.messageLength);
}

void ChatHistory::Render(Font &font, float left, float bottom, float chatWidth, bool chatActive)
{
    double now = glfwGetTime();
    const float fadeBeginAt = 9.0f;
    const float visibleFor = 10.0f;

    int chatMsgCount = (int)buffer.Count();
    if (!chatMsgCount) {
        return;
    }

    const float pad = 4.0f;
    const float chatHeight = (int)chatMsgCount * (font.baseSize + pad) + pad;

    // NOTE: The chat history renders from the bottom up (most recent message first)
    float cursorY = bottom - pad - font.baseSize;
    const char *chatText = 0;
    Color chatColor = WHITE;
    Color chatBgColor = Fade({ 50, 50, 50, 255 }, 0.6f);
    bool bottomLine = true;

    if (chatActive) {
        //DrawRectangleRec(rect, Fade(DARKGRAY, 0.8f));
        //DrawRectangleLinesEx(rect, 1, Fade(BLACK, 0.8f));
        DrawRectangle((int)left, (int)(bottom - chatHeight), (int)chatWidth, (int)chatHeight, chatBgColor);
        DrawRectangleLines((int)left, (int)(bottom - chatHeight), (int)chatWidth, (int)chatHeight, Fade(BLACK, 0.8f));
    }

    for (int i = (int)chatMsgCount - 1; i >= 0; i--) {
        const NetMessage_ChatMessage &chatMsg = buffer.At(i);
        assert(chatMsg.messageLength);

        // Show messages for 10 seconds when chat window not active
        float fadeAlpha = 1.0;
        float timeVisible = (float)(now - chatMsg.recvAt);
        if (!chatActive) {
            if (timeVisible > visibleFor) {
                continue;
            }

            // Fade during last second
            fadeAlpha = 1.0f - MAX(0, timeVisible - fadeBeginAt) / (visibleFor - fadeBeginAt);
        }

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
        if (!chatActive) {
            if (bottomLine) {
                DrawRectangle((int)left, (int)(bottom - pad), (int)chatWidth, (int)pad, Fade(chatBgColor, 0.6f * fadeAlpha));
                bottomLine = false;
            }
            DrawRectangleRec({ left, cursorY - pad, chatWidth, font.baseSize + pad }, Fade(chatBgColor, 0.6f * fadeAlpha));
        }
        DrawTextFont(font, chatText, left + pad, cursorY, font.baseSize, Fade(chatColor, fadeAlpha));
        cursorY -= font.baseSize + pad;
    }
}