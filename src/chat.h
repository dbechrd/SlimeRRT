#pragma once
#include "net_message.h"
#include "ring_buffer.h"
#include "raylib/raylib.h"

struct ChatHistory {
    size_t MessageCount() { return buffer.Count(); }
    void PushNetMessage(const NetMessage_ChatMessage &netChat);
    void PushMessage(const char *username, size_t usernameLength, const char *message, size_t messageLength);
    void Render(const Font &font, float left, float bottom, float chatWidth, bool chatActive);

private:
    static const char *LOG_SRC;
    RingBuffer<NetMessage_ChatMessage, CL_CHAT_HISTORY> buffer {};
};