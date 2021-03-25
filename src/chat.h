#pragma once
#include <stdint.h>

#define MAX_CHAT_MESSAGE_LEN 512

typedef struct {
    double timestamp;
    size_t messageLength;
    const char message[MAX_CHAT_MESSAGE_LEN];
} chat_message;

int  chat_init(void);
void chat_free(void);