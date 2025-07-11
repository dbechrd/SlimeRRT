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
    msgWritten.data.worldSnapshot.inputOverflow = 0.016731f;
    msgWritten.data.worldSnapshot.playerCount = 1;
    msgWritten.data.worldSnapshot.npcCount = 1;
    PlayerSnapshot &player = msgWritten.data.worldSnapshot.players[0];
    player.id = 69;
    player.flags = PlayerSnapshot::Flags_Health | PlayerSnapshot::Flags_HealthMax;
    player.hitPoints = 70;
    player.hitPointsMax = 100;

    NpcSnapshot &npc = msgWritten.data.worldSnapshot.npcs[0];
    npc.id = 70;
    npc.type = NPC::Type_Slime;
    npc.flags = NpcSnapshot::Flags_Health | NpcSnapshot::Flags_HealthMax;
    npc.hitPoints = 140.0f;
    npc.hitPointsMax = 150.0f;

    size_t len = PACKET_SIZE_MAX;
    uint8_t *buf = (uint8_t *)calloc(len, sizeof(*buf));
    msgWritten.Serialize(buf, len);
    NetMessage baseMsgRead{};
    baseMsgRead.Deserialize(buf, len);

    assert(baseMsgRead.connectionToken = msgWritten.connectionToken);
    assert(baseMsgRead.type == msgWritten.type);

    assert(baseMsgRead.type == NetMessage::Type::WorldSnapshot);
    WorldSnapshot &msgRead = baseMsgRead.data.worldSnapshot;
    assert(msgRead.tick == msgWritten.data.worldSnapshot.tick);
    assert(msgRead.lastInputAck == msgWritten.data.worldSnapshot.lastInputAck);
    assert(msgRead.inputOverflow == msgWritten.data.worldSnapshot.inputOverflow);
    assert(msgRead.playerCount == msgWritten.data.worldSnapshot.playerCount);
    assert(msgRead.npcCount == msgWritten.data.worldSnapshot.npcCount);

    PlayerSnapshot &playerRead = msgRead.players[0];
    assert(playerRead.id == player.id);
    assert(playerRead.hitPointsMax == player.hitPointsMax);
    assert(playerRead.hitPoints == player.hitPoints);

    NpcSnapshot &npcRead = msgRead.npcs[0];
    assert(npcRead.id == npc.id);
    assert(npcRead.type == npc.type);
    assert(npcRead.hitPoints == npc.hitPoints);
    assert(npcRead.hitPointsMax == npc.hitPointsMax);

    delete &msgWritten;
    free(buf);
}

void net_message_test_chat()
{
    NetMessage &msgWritten = *(new NetMessage{});
    msgWritten.type = NetMessage::Type::ChatMessage;
    msgWritten.data.chatMsg.source = NetMessage_ChatMessage::Source::Client;
    msgWritten.data.chatMsg.id = 69;
    memcpy(msgWritten.data.chatMsg.message, CSTR("This is a test message"));
    msgWritten.data.chatMsg.messageLength = (uint32_t)strlen(msgWritten.data.chatMsg.message);

    size_t len = PACKET_SIZE_MAX;
    uint8_t *buf = (uint8_t *)calloc(len, sizeof(*buf));
    msgWritten.Serialize(buf, len);
    NetMessage baseMsgRead{};
    baseMsgRead.Deserialize(buf, len);

    assert(baseMsgRead.type == NetMessage::Type::ChatMessage);
    NetMessage_ChatMessage &msgRead = baseMsgRead.data.chatMsg;

    assert(baseMsgRead.type == msgWritten.type);
    assert(msgRead.source == msgWritten.data.chatMsg.source);
    assert(msgRead.id == msgWritten.data.chatMsg.id);
    assert(msgRead.messageLength == msgWritten.data.chatMsg.messageLength);
    assert(!strncmp(msgRead.message, msgWritten.data.chatMsg.message, msgRead.messageLength));
    delete &msgWritten;
    free(buf);
}

void net_message_test()
{
    net_message_test_snapshot();
    net_message_test_chat();
}