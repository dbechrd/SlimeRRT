#include "tests.h"
#include "../src/net_message.h"
#include "../src/packet.h"
#include <cassert>
#include <cstring>

void net_message_test()
{
    NetMessage_ChatMessage msgWritten{};
    msgWritten.username = "test username";
    msgWritten.usernameLength = strlen(msgWritten.username);
    msgWritten.message = "This is a test message";
    msgWritten.messageLength = strlen(msgWritten.message);

    char *rawPacket = (char *)calloc(PACKET_SIZE_MAX, sizeof(*rawPacket));
    size_t rawBytes = msgWritten.Serialize((uint32_t *)rawPacket, PACKET_SIZE_MAX);
    NetMessage &baseMsgRead = NetMessage::Deserialize((uint32_t *)rawPacket, rawBytes);

    assert(baseMsgRead.type == NetMessage::Type::ChatMessage);
    NetMessage_ChatMessage &msgRead = static_cast<NetMessage_ChatMessage &>(baseMsgRead);

    assert(msgRead.type == msgWritten.type);
    assert(msgRead.usernameLength == msgWritten.usernameLength);
    assert(!strncmp(msgRead.username, msgWritten.username, msgRead.usernameLength));
    assert(msgRead.messageLength == msgWritten.messageLength);
    assert(!strncmp(msgRead.message, msgWritten.message, msgRead.messageLength));
    free(rawPacket);
}