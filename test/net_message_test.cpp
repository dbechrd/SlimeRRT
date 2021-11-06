#include "tests.h"
#include "../src/net_message.h"
#include "../src/packet.h"
#include <cassert>
#include <cstring>

void net_message_test()
{
    NetMessage msgWritten{};
    msgWritten.type = NetMessage::Type::ChatMessage;
    msgWritten.data.chatMsg.username = "test username";
    msgWritten.data.chatMsg.usernameLength = (uint32_t)strlen(msgWritten.data.chatMsg.username);
    msgWritten.data.chatMsg.message = "This is a test message";
    msgWritten.data.chatMsg.messageLength = (uint32_t)strlen(msgWritten.data.chatMsg.message);

    char *rawPacket = (char *)calloc(PACKET_SIZE_MAX, sizeof(*rawPacket));
    size_t rawBytes = msgWritten.Serialize((uint32_t *)rawPacket, PACKET_SIZE_MAX);
    NetMessage baseMsgRead{};
    baseMsgRead.Deserialize((uint32_t *)rawPacket, rawBytes);

    assert(baseMsgRead.type == NetMessage::Type::ChatMessage);
    NetMessage_ChatMessage &msgRead = baseMsgRead.data.chatMsg;

    assert(baseMsgRead.type == msgWritten.type);
    assert(msgRead.usernameLength == msgWritten.data.chatMsg.usernameLength);
    assert(!strncmp(msgRead.username, msgWritten.data.chatMsg.username, msgRead.usernameLength));
    assert(msgRead.messageLength == msgWritten.data.chatMsg.messageLength);
    assert(!strncmp(msgRead.message, msgWritten.data.chatMsg.message, msgRead.messageLength));
    free(rawPacket);
}