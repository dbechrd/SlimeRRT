#pragma once
#include "error.h"
#include "helpers.h"
#include "net_message.h"
#include <cstdint>

struct ChatMessage {
    // TODO: Send consistent chat timestamp from server
    //double timestamp;
    size_t usernameLength                   {};
    char   username[USERNAME_LENGTH_MAX]    {};
    size_t messageLength                    {};
    char   message[CHAT_MESSAGE_BUFFER_LEN] {};
};

struct ChatHistory {
    size_t       first    {};  // index of first message (ring buffer)
    size_t       count    {};  // current # of message in buffer
    size_t       capacity {};  // maximum # of message in buffer
    ChatMessage *messages {};  // array of messages

    ~ChatHistory();
    ErrorType Init();
    void PushNetMessage(const NetMessage_ChatMessage &netChat);

private:
    ChatMessage *Alloc();
};