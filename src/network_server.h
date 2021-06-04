#pragma once
#include "packet.h"
#include "chat.h"
#include "zed_net.h"
#include <cstdint>

#define NETWORK_SERVER_CLIENTS_MAX 4
// must be power of 2 (shift modulus ring buffer)
#define NETWORK_SERVER_PACKET_HISTORY_MAX 256

struct NetworkServerClient {
    zed_net_address_t address;
    double last_packet_received_at;
    size_t usernameLength;
    char username[USERNAME_LENGTH_MAX];
};

struct NetworkServer {
    unsigned short port;
    zed_net_socket_t socket;
    size_t clientsConnected;
    NetworkServerClient clients[NETWORK_SERVER_CLIENTS_MAX];
    // TODO: Could have a packet history by message type? This would allow us to only store history of important
    // messages, and/or have different buffer sizes for different types of message.
    //PacketBuffer packetHistory[NetMessageType_Count];
    PacketBuffer packetHistory;
    ChatHistory chatHistory;
};

int  network_server_init         (NetworkServer *server);
int  network_server_open_socket  (NetworkServer *server, unsigned short port);
int  network_server_receive      (NetworkServer *server);
void network_server_close_socket (NetworkServer *server);
void network_server_free         (NetworkServer *server);

#if 0
int  network_packet_history_count    (const NetworkServer *server);
int  network_packet_history_next     (const NetworkServer *server, int index);
int  network_packet_history_prev     (const NetworkServer *server, int index);
int  network_packet_history_newest   (const NetworkServer *server);
int  network_packet_history_oldest   (const NetworkServer *server);
void network_packet_history_at       (const NetworkServer *server, int index, const Packet **packet);
#endif