#pragma once
#include "packet.h"
#include "chat.h"
#include "zed_net.h"
#include "dlb_types.h"

// must be power of 2 (shift modulus ring buffer)
#define NETWORK_CLIENT_PACKET_HISTORY_MAX 256

struct NetworkClient {
    const char *       serverHostname {};
    zed_net_address_t  server         {};
    zed_net_socket_t   socket         {};
    size_t             usernameLength {};
    char               username[USERNAME_LENGTH_MAX]{};
    PacketBuffer       packetHistory  {};
    ChatHistory        chatHistory    {};
};

// TODO: This should be abstracted out into a messaging system the serializes everything and sends the raw packet
// to network_client_send which handles the actual sending
int  network_client_send_chat_message(const NetworkClient *client, const char *message, size_t messageLength);

int  network_client_init         (NetworkClient *client);
int  network_client_open_socket  (NetworkClient *client);
int  network_client_connect      (NetworkClient *client, const char *hostname, unsigned short port);
int  network_client_send         (const NetworkClient *client, const char *data, size_t len);
int  network_client_receive      (NetworkClient *client);
void network_client_disconnect   (const NetworkClient *client);
void network_client_close_socket (const NetworkClient *client);
void network_client_free         (NetworkClient *client);
