#include "tests.h"
#include "../src/net_message.h"
#include <cassert>
#include <cstring>

void net_message_test_snapshot()
{
    NetMessage &msgWritten = *(new NetMessage{});
    msgWritten.connectionToken = 42;
    msgWritten.type = NetMessage::Type::WorldSnapshot;
    msgWritten.data.worldSnapshot.tick = 123456789;
    msgWritten.data.worldSnapshot.lastInputAck = 1900;
    msgWritten.data.worldSnapshot.playerCount = 1;
    msgWritten.data.worldSnapshot.slimeCount = 35;
    PlayerSnapshot &player = msgWritten.data.worldSnapshot.players[0];
    memcpy(player.name, CSTR("test username"));
    player.id = 4;
    player.nameLength = (uint32_t)strlen(player.name);
    player.hitPointsMax = 100;
    player.hitPoints = 50;

    World *world = new World;
    world->SpawnPlayer(player.id);
    ENetBuffer rawPacket{ PACKET_SIZE_MAX, calloc(PACKET_SIZE_MAX, sizeof(uint8_t)) };
    msgWritten.Serialize(*world, rawPacket);
    NetMessage baseMsgRead{};
    baseMsgRead.Deserialize(*world, rawPacket);
    delete world;

    assert(baseMsgRead.connectionToken = msgWritten.connectionToken);
    assert(baseMsgRead.type == msgWritten.type);
    
    assert(baseMsgRead.type == NetMessage::Type::WorldSnapshot);
    WorldSnapshot &msgRead = baseMsgRead.data.worldSnapshot;
    assert(msgRead.tick == msgWritten.data.worldSnapshot.tick);
    assert(msgRead.lastInputAck == msgWritten.data.worldSnapshot.lastInputAck);
    assert(msgRead.playerCount == msgWritten.data.worldSnapshot.playerCount);
    assert(msgRead.slimeCount == msgWritten.data.worldSnapshot.slimeCount);

    PlayerSnapshot &playerRead = msgRead.players[0];
    assert(playerRead.id == player.id);
    assert(playerRead.nameLength == player.nameLength);
    assert(!strncmp(playerRead.name, player.name, playerRead.nameLength));
    assert(playerRead.hitPointsMax == player.hitPointsMax);
    assert(playerRead.hitPoints == player.hitPoints);
    delete &msgWritten;
    free(rawPacket.data);
}

void net_message_test_chat()
{
    NetMessage &msgWritten = *(new NetMessage{});
    msgWritten.type = NetMessage::Type::ChatMessage;
    memcpy(msgWritten.data.chatMsg.username, CSTR("test username"));
    msgWritten.data.chatMsg.usernameLength = (uint32_t)strlen(msgWritten.data.chatMsg.username);
    memcpy(msgWritten.data.chatMsg.message, CSTR("This is a test message"));
    msgWritten.data.chatMsg.messageLength = (uint32_t)strlen(msgWritten.data.chatMsg.message);

    World *world = new World;
    ENetBuffer rawPacket{ PACKET_SIZE_MAX, calloc(PACKET_SIZE_MAX, sizeof(uint8_t)) };
    msgWritten.Serialize(*world, rawPacket);
    NetMessage baseMsgRead{};
    baseMsgRead.Deserialize(*world, rawPacket);
    delete world;

    assert(baseMsgRead.type == NetMessage::Type::ChatMessage);
    NetMessage_ChatMessage &msgRead = baseMsgRead.data.chatMsg;

    assert(baseMsgRead.type == msgWritten.type);
    assert(msgRead.usernameLength == msgWritten.data.chatMsg.usernameLength);
    assert(!strncmp(msgRead.username, msgWritten.data.chatMsg.username, msgRead.usernameLength));
    assert(msgRead.messageLength == msgWritten.data.chatMsg.messageLength);
    assert(!strncmp(msgRead.message, msgWritten.data.chatMsg.message, msgRead.messageLength));
    delete &msgWritten;
    free(rawPacket.data);
}

void net_message_test()
{
    net_message_test_snapshot();
    net_message_test_chat();
}