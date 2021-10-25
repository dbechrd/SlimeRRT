#pragma once
#include "error.h"
#include "helpers.h"
#include "net_message.h"
#include "ring_buffer.h"
#include <cstdint>
#include <vector>

struct ChatMessage {
    char   timestampStr[12]                 {};  // hh:MM:SS AM
    size_t usernameLength                   {};
    char   username[USERNAME_LENGTH_MAX]    {};
    size_t messageLength                    {};
    char   message[CHAT_MESSAGE_BUFFER_LEN] {};
};

struct ChatHistory : RingBuffer<ChatMessage> {
    ChatHistory() : RingBuffer(CHAT_MESSAGE_HISTORY) {}
    void PushNetMessage(const NetMessage_ChatMessage &netChat);
    void PushMessage(const char *username, size_t usernameLength, const char *message, size_t messageLength);

private:
    static const char *LOG_SRC;
};