#pragma once
#include "chat.h"
#include "packet.h"
#include "controller.h"
#include "world.h"
#include "dlb_types.h"

struct NetClient {
    const char     *serverHost      {};
    unsigned short  serverPort      {};
    ENetHost       *client          {};
    ENetPeer       *server          {};
    uint32_t        connectionToken {};
    size_t          usernameLength  {};
    char            username        [USERNAME_LENGTH_MAX] {};
    size_t          passwordLength  {};
    char            password        [PASSWORD_LENGTH_MAX] {};

    World          *serverWorld     {};
    ChatHistory     chatHistory     {};
    RingBuffer<NetMessage_Input, CLIENT_INPUT_HISTORY> inputHistory {};
    RingBuffer<NetMessage_WorldPlayers, CLIENT_WORLD_HISTORY> worldHistory {};

    NetClient::~NetClient();
    ErrorType OpenSocket();
    ErrorType Connect(const char *serverHost, unsigned short serverPort, const char *user, const char *password);
    ErrorType SendChatMessage(const char *message, size_t messageLength);
    ErrorType SendPlayerInput(const NetMessage_Input &input);
    ErrorType Receive();
    void Disconnect();
    void CloseSocket();

private:
    static const char *LOG_SRC;

    ErrorType   SendRaw    (const void *data, size_t size);
    ErrorType   SendMsg    (NetMessage &message);
    ErrorType   Auth       ();
    void        ProcessMsg (ENetPacket &packet);
    const char *ServerStateString();
};