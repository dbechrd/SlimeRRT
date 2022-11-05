#include "tests.h"
#include "../src/net_message.h"
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

    DLB_ASSERT(baseMsgRead.connectionToken = msgWritten.connectionToken);
    DLB_ASSERT(baseMsgRead.type == msgWritten.type);

    DLB_ASSERT(baseMsgRead.type == NetMessage::Type::WorldSnapshot);
    WorldSnapshot &msgRead = baseMsgRead.data.worldSnapshot;
    DLB_ASSERT(msgRead.tick == msgWritten.data.worldSnapshot.tick);
    DLB_ASSERT(msgRead.lastInputAck == msgWritten.data.worldSnapshot.lastInputAck);
    DLB_ASSERT(msgRead.inputOverflow == msgWritten.data.worldSnapshot.inputOverflow);
    DLB_ASSERT(msgRead.playerCount == msgWritten.data.worldSnapshot.playerCount);
    DLB_ASSERT(msgRead.npcCount == msgWritten.data.worldSnapshot.npcCount);

    PlayerSnapshot &playerRead = msgRead.players[0];
    DLB_ASSERT(playerRead.id == player.id);
    DLB_ASSERT(playerRead.hitPointsMax == player.hitPointsMax);
    DLB_ASSERT(playerRead.hitPoints == player.hitPoints);

    NpcSnapshot &npcRead = msgRead.npcs[0];
    DLB_ASSERT(npcRead.id == npc.id);
    DLB_ASSERT(npcRead.type == npc.type);
    DLB_ASSERT(npcRead.hitPoints == npc.hitPoints);
    DLB_ASSERT(npcRead.hitPointsMax == npc.hitPointsMax);

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

    DLB_ASSERT(baseMsgRead.type == NetMessage::Type::ChatMessage);
    NetMessage_ChatMessage &msgRead = baseMsgRead.data.chatMsg;

    DLB_ASSERT(baseMsgRead.type == msgWritten.type);
    DLB_ASSERT(msgRead.source == msgWritten.data.chatMsg.source);
    DLB_ASSERT(msgRead.id == msgWritten.data.chatMsg.id);
    DLB_ASSERT(msgRead.messageLength == msgWritten.data.chatMsg.messageLength);
    DLB_ASSERT(!strncmp(msgRead.message, msgWritten.data.chatMsg.message, msgRead.messageLength));
    delete &msgWritten;
    free(buf);
}

void net_message_test()
{
    net_message_test_snapshot();
    net_message_test_chat();
}