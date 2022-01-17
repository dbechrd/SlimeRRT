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
    msgWritten.data.worldSnapshot.enemyCount = 1;
    PlayerSnapshot &player = msgWritten.data.worldSnapshot.players[0];
    player.id = 69;
    player.flags = PlayerSnapshot::Flags::Health | PlayerSnapshot::Flags::HealthMax;
    player.hitPoints = 70;
    player.hitPointsMax = 100;

    EnemySnapshot &enemy = msgWritten.data.worldSnapshot.enemies[0];
    enemy.id = 70;
    enemy.flags = EnemySnapshot::Flags::Health | EnemySnapshot::Flags::HealthMax;
    enemy.hitPoints = 140.0f;
    enemy.hitPointsMax = 150.0f;

    World *world = new World;
    world->AddPlayer(player.id);
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
    assert(msgRead.enemyCount == msgWritten.data.worldSnapshot.enemyCount);

    PlayerSnapshot &playerRead = msgRead.players[0];
    assert(playerRead.id == player.id);
    assert(playerRead.hitPointsMax == player.hitPointsMax);
    assert(playerRead.hitPoints == player.hitPoints);

    EnemySnapshot &enemyRead = msgRead.enemies[0];
    assert(enemyRead.id == enemy.id);
    assert(enemyRead.hitPoints == enemy.hitPoints);
    assert(enemyRead.hitPointsMax == enemy.hitPointsMax);

    delete &msgWritten;
    free(rawPacket.data);
}

void net_message_test_chat()
{
    NetMessage &msgWritten = *(new NetMessage{});
    msgWritten.type = NetMessage::Type::ChatMessage;
    msgWritten.data.chatMsg.source = NetMessage_ChatMessage::Source::Client;
    msgWritten.data.chatMsg.id = 69;
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
    assert(msgRead.source == msgWritten.data.chatMsg.source);
    assert(msgRead.id == msgWritten.data.chatMsg.id);
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