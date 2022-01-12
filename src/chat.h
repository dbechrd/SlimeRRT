#pragma once
#include "net_message.h"
#include "ring_buffer.h"
#include "raylib/raylib.h"

struct ChatHistory {
    size_t MessageCount() { return buffer.Count(); }
    void PushNetMessage(const NetMessage_ChatMessage &netChat);

    void PushSystem(const char *message, size_t messageLength);
    void PushDebug(const char *message, size_t messageLength);
    void PushServer(const char *message, size_t messageLength);
    void PushPlayer(uint32_t playerId, const char *message, size_t messageLength);
    void PushSam(const char *message, size_t messageLength);

    void Render(const Font &font, World &world, float left, float bottom, float chatWidth, bool chatActive);

private:
    static const char *LOG_SRC;
    RingBuffer<NetMessage_ChatMessage, CL_CHAT_HISTORY> buffer {};

    void PushMessage(NetMessage_ChatMessage::Source source, uint32_t id, const char *message, size_t messageLength);
};