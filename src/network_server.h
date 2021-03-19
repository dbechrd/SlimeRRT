#pragma once
#include "zed_net.h"

#define NETWORK_SERVER_MAX_CLIENTS 4

#define MAX_PACKET_SIZE_BYTES 1024
#define MAX_PACKET_HISTORY 10
//#define MAX_CHAT_HISTORY 10

typedef struct {
    char timestampStr[9];  // hh:mm:ss
    char data[MAX_PACKET_SIZE_BYTES];
} NetworkPacket;

//typedef struct {
//    const char *message;
//    size_t len;
//    double timestamp;
//} chat_message;
//
//static chat_message chat_history[MAX_CHAT_HISTORY];

typedef struct {
    zed_net_address_t address;
    // todo: double last_packet_received_at
} NetworkServerClient;

typedef struct {
    unsigned short port;
    zed_net_socket_t socket;
    NetworkServerClient clients[NETWORK_SERVER_MAX_CLIENTS];
} NetworkServer;

int                  network_init                    ();
int                  network_server_start            (NetworkServer *server, unsigned short port);
int                  network_server_process_incoming (NetworkServer *server);
int                  network_packet_history_count    (NetworkServer *server);
int                  network_packet_history_next     (NetworkServer *server, int index);
int                  network_packet_history_prev     (NetworkServer *server, int index);
int                  network_packet_history_newest   (NetworkServer *server);
int                  network_packet_history_oldest   (NetworkServer *server);
const NetworkPacket *network_packet_history_at       (NetworkServer *server, int index);
void                 network_server_stop             (NetworkServer *server);
void                 network_shutdown                ();

