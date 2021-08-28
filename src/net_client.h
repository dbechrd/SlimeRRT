#pragma once
#include "packet.h"
#include "chat.h"
#include "zed_net.h"
#include "dlb_types.h"

// must be power of 2 (shift modulus ring buffer)
#define NET_CLIENT_PACKET_HISTORY_MAX 256

struct NetClient {
    const char *       serverHostname {};
    zed_net_address_t  server         {};
    zed_net_socket_t   socket         {};
    size_t             usernameLength {};
    char               username[USERNAME_LENGTH_MAX]{};
    PacketBuffer       packetHistory  {};
    ChatHistory        chatHistory    {};
};

// TODO: This should be abstracted out into a messaging system the serializes everything and sends the raw packet
// to net_client_send which handles the actual sending
int  net_client_send_chat_message(const NetClient *client, const char *message, size_t messageLength);

int  net_client_init         (NetClient *client);
int  net_client_open_socket  (NetClient *client);
int  net_client_connect      (NetClient *client, const char *hostname, unsigned short port);
int  net_client_send         (const NetClient *client, const char *data, size_t len);
int  net_client_receive      (NetClient *client);
void net_client_disconnect   (const NetClient *client);
void net_client_close_socket (NetClient *client);
void net_client_free         (NetClient *client);
