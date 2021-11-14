#include "tests.h"
#include "../src/net_message.h"
#include "../src/packet.h"
#include <cassert>
#include <cstring>

void net_message_test()
{
    NetMessage msgWritten{};
    msgWritten.type = NetMessage::Type::ChatMessage;
    memcpy(msgWritten.data.chatMsg.username, CSTR("test username"));
    msgWritten.data.chatMsg.usernameLength = (uint32_t)strlen(msgWritten.data.chatMsg.username);
    memcpy(msgWritten.data.chatMsg.message, CSTR("This is a test message"));
    msgWritten.data.chatMsg.messageLength = (uint32_t)strlen(msgWritten.data.chatMsg.message);

    World world{};
    ENetBuffer rawPacket = msgWritten.Serialize(world);
    NetMessage baseMsgRead{};
    baseMsgRead.Deserialize(rawPacket, world);

    assert(baseMsgRead.type == NetMessage::Type::ChatMessage);
    NetMessage_ChatMessage &msgRead = baseMsgRead.data.chatMsg;

    assert(baseMsgRead.type == msgWritten.type);
    assert(msgRead.usernameLength == msgWritten.data.chatMsg.usernameLength);
    assert(!strncmp(msgRead.username, msgWritten.data.chatMsg.username, msgRead.usernameLength));
    assert(msgRead.messageLength == msgWritten.data.chatMsg.messageLength);
    assert(!strncmp(msgRead.message, msgWritten.data.chatMsg.message, msgRead.messageLength));
}