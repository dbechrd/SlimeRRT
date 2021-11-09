#include "chat.h"
#include "error.h"
#include "net_message.h"
#include "packet.h"
#include "raylib/raylib.h"
#include <cassert>
#include <cstring>

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

void ChatHistory::Render(Font &font, ChatHistory &chatHistory, Rectangle rect)
{
    size_t chatHistoryMsgCount = chatHistory.Count();
    if (!chatHistoryMsgCount) {
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

    for (int i = (int)chatHistoryMsgCount - 1; i >= 0; i--) {
        const ChatMessage &chatMsg = chatHistory.At(i);
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