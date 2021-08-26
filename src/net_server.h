#pragma once
#include "chat.h"
#include "error.h"
#include "packet.h"
#include "zed_net.h"
#include <cstdint>

#define NET_SERVER_CLIENTS_MAX 4
// must be power of 2 (shift modulus ring buffer)
#define NET_SERVER_PACKET_HISTORY_MAX 256

struct NetworkServerClient {
    zed_net_address_t address                       {};
    double            last_packet_received_at       {};
    size_t            usernameLength                {};
    char              username[USERNAME_LENGTH_MAX] {};
};

struct NetServer {
    unsigned short      port             {};
    zed_net_socket_t    socket           {};
    size_t              clientsConnected {};
    NetworkServerClient clients[NET_SERVER_CLIENTS_MAX]{};
    // TODO: Could have a packet history by message type? This would allow us to only store history of important
    // messages, and/or have different buffer sizes for different types of message.
    //PacketBuffer packetHistory[Count];
    PacketBuffer        packetHistory    {};
    ChatHistory         chatHistory      {};
};

extern NetServer g_net_server;

ErrorType net_server_run                  ();
ErrorType net_server_init                    (NetServer *server);
ErrorType net_server_open_socket             (NetServer *server, unsigned short port);
void      net_server_broadcast_chat_message  (const NetServer *server, const char *msg, size_t msgLength);
ErrorType net_server_listen                  (NetServer *server);
void      net_server_close_socket            (NetServer *server);
void      net_server_free                    (NetServer *server);

#if 0
int  network_packet_history_count    (const NetServer *server);
int  network_packet_history_next     (const NetServer *server, int index);
int  network_packet_history_prev     (const NetServer *server, int index);
int  network_packet_history_newest   (const NetServer *server);
int  network_packet_history_oldest   (const NetServer *server);
void network_packet_history_at       (const NetServer *server, int index, const Packet **packet);
#endif