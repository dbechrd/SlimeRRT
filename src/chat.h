#pragma once
#include "helpers.h"
#include <cstdint>

struct ChatMessage {
    // TODO: Send consistent chat timestamp from server
    //double timestamp;
    size_t usernameLength;
    char username[USERNAME_LENGTH_MAX];
    size_t messageLength;
    char message[CHAT_MESSAGE_BUFFER_LEN];
};

struct ChatHistory {
    size_t first;           // index of first message (ring buffer)
    size_t count;           // current # of message in buffer
    size_t capacity;        // maximum # of message in buffer (MUST BE POWER OF 2)
    ChatMessage *messages;  // array of messages
};

int  chat_history_init             (ChatHistory *chatHistory);
void chat_history_push_net_message (ChatHistory *chatHistory, const struct NetMessage_ChatMessage &netChat);
void chat_history_free             (ChatHistory *chatHistory);
