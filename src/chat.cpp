#include "chat.h"
#include "error.h"
#include "net_message.h"
#include "world.h"
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

void ChatHistory::PushSystem(const char *message, size_t messageLength)
{
    PushMessage(NetMessage_ChatMessage::Source::System, 0, message, messageLength);
}

void ChatHistory::PushDebug(const char *message, size_t messageLength)
{
    PushMessage(NetMessage_ChatMessage::Source::Debug, 0, message, messageLength);
}

void ChatHistory::PushServer(const char *message, size_t messageLength)
{
    PushMessage(NetMessage_ChatMessage::Source::Server, 0, message, messageLength);
}

void ChatHistory::PushPlayer(uint32_t playerId, const char *message, size_t messageLength)
{
    PushMessage(NetMessage_ChatMessage::Source::Client, playerId, message, messageLength);
}

void ChatHistory::PushSam(const char *message, size_t messageLength)
{
    PushMessage(NetMessage_ChatMessage::Source::Sam, 0, message, messageLength);
}

void ChatHistory::PushMessage(NetMessage_ChatMessage::Source source, uint32_t id, const char *message, size_t messageLength)
{
    assert(message);
    assert(messageLength);
    assert(messageLength <= CHATMSG_LENGTH_MAX);

    NetMessage_ChatMessage &chat = buffer.Alloc();
    chat.recvAt = glfwGetTime();
    const char *timestampStr = TextFormatTimestamp();
    memcpy(chat.timestampStr, timestampStr, sizeof(timestampStr));
    chat.source = source;
    chat.id = id;
    chat.messageLength = (uint32_t)MIN(messageLength, CHATMSG_LENGTH_MAX);
    memcpy(chat.message, message, chat.messageLength);
}

void ChatHistory::Render(const Font &font, World &world, int left, int bottom, int chatWidth, bool chatActive)
{
    double now = glfwGetTime();
    const float fadeBeginAt = 9.0f;
    const float visibleFor = 10.0f;

    int chatMsgCount = (int)buffer.Count();
    if (!chatMsgCount) {
        return;
    }

    const int pad = 4;
    const int chatHeight = chatMsgCount * (font.baseSize + pad) + pad;
    const Color chatBgColor = Fade(BLACK, 0.5f);
    const float chatBgAlpha = chatBgColor.a / 255.0f;

    // NOTE: The chat history renders from the bottom up (most recent message first)
    int cursorY = bottom - pad - font.baseSize;
    bool bottomLine = true;

    if (chatActive) {
        //DrawRectangleRec(rect, Fade(DARKGRAY, 0.8f));
        //DrawRectangleLinesEx(rect, 1, Fade(BLACK, 0.8f));
        DrawRectangle(left, (bottom - chatHeight), chatWidth, chatHeight, chatBgColor);
        DrawRectangleLines(left, (bottom - chatHeight), chatWidth, chatHeight, Fade(BLACK, 0.5f));
    }

    for (int i = chatMsgCount - 1; i >= 0; i--) {
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

        const char *displayName = 0;
        Color chatColor = WHITE;
        switch (chatMsg.source) {
            case NetMessage_ChatMessage::Source::System: displayName = "<System>"; chatColor = YELLOW;    break;
            case NetMessage_ChatMessage::Source::Debug:  displayName = "<Debug>";  chatColor = LIGHTGRAY; break;
            case NetMessage_ChatMessage::Source::Server: displayName = "<Server>"; chatColor = RED;       break;
            case NetMessage_ChatMessage::Source::Sam:    displayName = "[System]"; chatColor = GREEN;     break;
            case NetMessage_ChatMessage::Source::Client: {
                Player *player = world.FindPlayer(chatMsg.id);
                displayName = TextFormat("[%.*s]", player->nameLength, player ? player->name : "someone");
                break;
            }
            default: TraceLog(LOG_FATAL, "Unhandled chat source");
        }

        const char *chatText = TextFormat("[%s]%s: %.*s", chatMsg.timestampStr, displayName,
            chatMsg.messageLength, chatMsg.message);

        if (!chatActive) {
            if (bottomLine) {
                DrawRectangle(left, (bottom - pad), chatWidth, pad, Fade(chatBgColor, chatBgAlpha * fadeAlpha));
                bottomLine = false;
            }
            DrawRectangle(left, cursorY - pad, chatWidth, font.baseSize + pad, Fade(chatBgColor, chatBgAlpha * fadeAlpha));
        }
        DrawTextFont(font, chatText, (float)left + pad, (float)cursorY, 0, 0, font.baseSize, Fade(chatColor, fadeAlpha));
        cursorY -= font.baseSize + pad;
    }
}