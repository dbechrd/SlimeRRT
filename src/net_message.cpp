#include "bit_stream.h"
#include "entities/entities.h"
#include "helpers.h"
#include "net_message.h"
#include "tilemap.h"
#include <cassert>

size_t NetMessage::Process(BitStream::Mode mode, ENetBuffer &buffer)
{
    assert(buffer.data);
    assert(buffer.dataLength);

    BitStream stream(mode, buffer.data, buffer.dataLength);

    stream.Process(connectionToken);

    stream.Process((uint32_t &)type, 4, (uint32_t)NetMessage::Type::Unknown + 1, (uint32_t)NetMessage::Type::Count - 1);
    stream.Align();

    if (type == NetMessage::Type::WorldSnapshot) {
        stream.debugPrint = true;
        //puts("WorldSnapshot:");
    }

    switch (type) {
        case NetMessage::Type::Identify: {
            NetMessage_Identify &ident = data.identify;

            stream.Process(ident.usernameLength, 6, USERNAME_LENGTH_MIN, USERNAME_LENGTH_MAX);
            stream.Align();

            for (size_t i = 0; i < ident.usernameLength; i++) {
                stream.ProcessChar(ident.username[i]);
            }

            stream.Process(ident.passwordLength, 7, PASSWORD_LENGTH_MIN, PASSWORD_LENGTH_MAX);
            stream.Align();

            for (size_t i = 0; i < ident.passwordLength; i++) {
                stream.ProcessChar(ident.password[i]);
            }

            break;
        } case NetMessage::Type::Welcome: {
            NetMessage_Welcome &welcome = data.welcome;

            stream.Process(welcome.motdLength, 6, MOTD_LENGTH_MIN, MOTD_LENGTH_MAX);
            stream.Align();

            for (size_t i = 0; i < welcome.motdLength; i++) {
                stream.ProcessChar(welcome.motd[i]);
            }

            stream.Process(welcome.playerId, 32, 1, UINT32_MAX);
            stream.Align();

            stream.Process(welcome.playerCount, 4, 0, SV_MAX_PLAYERS);

            for (size_t i = 0; i < welcome.playerCount; i++) {
                NetMessage_Welcome::NetMessage_Welcome_Player &player = welcome.players[i];

                stream.Process(player.id, 32, 0, UINT32_MAX);

                if (player.id) {
                    // TODO: Don't sync name unless it has changed
                    stream.Process(player.nameLength, 6, USERNAME_LENGTH_MIN, USERNAME_LENGTH_MAX);
                    stream.Align();
                    for (size_t i = 0; i < player.nameLength; i++) {
                        stream.ProcessChar(player.name[i]);
                    }
                }
            }

            break;
        } case NetMessage::Type::ChatMessage: {
            NetMessage_ChatMessage &chatMsg = data.chatMsg;

            stream.Process((uint32_t &)chatMsg.source, 6, (uint32_t)NetMessage_ChatMessage::Source::Unknown + 1, (uint32_t)NetMessage_ChatMessage::Source::Count - 1);
            switch (chatMsg.source) {
                case NetMessage_ChatMessage::Source::Client: {
                    stream.Process(chatMsg.id, 32, 0, UINT32_MAX);
                    break;
                }
            }
            stream.Process(chatMsg.messageLength, 9, CHATMSG_LENGTH_MIN, CHATMSG_LENGTH_MAX);
            stream.Align();
            for (size_t i = 0; i < chatMsg.messageLength; i++) {
                stream.ProcessChar(chatMsg.message[i]);
            }

            break;
        } case NetMessage::Type::Input: {
            NetMessage_Input &input = data.input;

            stream.Process(input.sampleCount, 10, 0, CL_INPUT_SAMPLES_MAX);
            for (size_t i = 0; i < input.sampleCount; i++) {
                InputSample &sample = input.samples[i];
                bool same = i && sample.Equals(input.samples[i - 1]) && (sample.seq == (input.samples[i - 1].seq + 1));
                stream.Process(same);
                if (same) {
                    if (mode == BitStream::Mode::Reader) {
#pragma warning(push)
#pragma warning(disable: 6385)
                        // conversion from 'int' to 'float'
                        sample = input.samples[i - 1];
#pragma warning(pop)
                        sample.seq = input.samples[i - 1].seq + 1;
                    }
                    stream.Process(sample.dt);
                } else {
                    // TODO: Don't send every seq number, it's implicit based on the first seq number and count
                    stream.Process(sample.seq, 32, 0, UINT32_MAX);
                    // TODO: Don't send ownerId more than once.. move this up outside of the loop
                    stream.Process(sample.ownerId, 32, 0, UINT32_MAX);
                    stream.Process(sample.dt);
                    stream.Process(sample.walkNorth);
                    stream.Process(sample.walkEast);
                    stream.Process(sample.walkSouth);
                    stream.Process(sample.walkWest);
                    stream.Process(sample.run);
                    stream.Process(sample.primary);
                    assert(PlayerInvSlot_Count > 0);
                    stream.Process(sample.selectSlot, 8, 0, PlayerInvSlot_Count - 1);
                }
                //E_DEBUG("%s sample: %u %f", mode == BitStream::Mode::Reader ? "READ" : "WRITE", sample.seq, sample.dt);
            }
            stream.Align();

            break;
        } case NetMessage::Type::WorldChunk: {
            NetMessage_WorldChunk &worldChunk = data.worldChunk;

            stream.Process(worldChunk.chunk.x, 16, WORLD_CHUNK_MIN, WORLD_CHUNK_MAX);
            stream.Process(worldChunk.chunk.y, 16, WORLD_CHUNK_MIN, WORLD_CHUNK_MAX);

            // TODO(perf): RLE compression
            // https://moddingwiki.shikadi.net/wiki/RLE_Compression#Code
            for (size_t i = 0; i < ARRAY_SIZE(worldChunk.chunk.tiles); i++) {
                stream.Process(worldChunk.chunk.tiles[i].type, 4, 0, TileType_Count - 1);
                stream.Process(worldChunk.chunk.tiles[i].object.type, 4, 0, ObjectType_Count - 1);
                stream.Process(worldChunk.chunk.tiles[i].object.flags);
            }
            stream.Align();

            break;
        } case NetMessage::Type::TileUpdate: {
            NetMessage_TileUpdate &tileUpdate = data.tileUpdate;

            stream.Process(tileUpdate.worldX);
            stream.Process(tileUpdate.worldY);

            stream.Process(tileUpdate.tile.type, 4, 0, TileType_Count - 1);
            stream.Process(tileUpdate.tile.object.type, 4, 0, ObjectType_Count - 1);
            stream.Process(tileUpdate.tile.object.flags);

            break;
        } case NetMessage::Type::WorldSnapshot: {
            WorldSnapshot &worldSnapshot = data.worldSnapshot;

            stream.Process(worldSnapshot.tick, 32, 1, UINT32_MAX);
            stream.Process(worldSnapshot.clock);
            stream.Process(worldSnapshot.lastInputAck);
            stream.Process(worldSnapshot.inputOverflow);
            stream.Process(worldSnapshot.playerCount, 4, 0, SNAPSHOT_MAX_PLAYERS);
            stream.Process(worldSnapshot.npcCount, 9, 0, SNAPSHOT_MAX_NPCS);
            stream.Process(worldSnapshot.itemCount, 9, 0, SNAPSHOT_MAX_ITEMS);
            stream.Align();

            for (size_t i = 0; i < worldSnapshot.playerCount; i++) {
                PlayerSnapshot &playerSnap = worldSnapshot.players[i];
                stream.Process(playerSnap.id, 32, 1, UINT32_MAX);
                stream.Process((uint32_t &)playerSnap.flags);
                if (playerSnap.flags & PlayerSnapshot::Flags_Position) {
                    stream.Process(playerSnap.position.x);
                    stream.Process(playerSnap.position.y);
                    stream.Process(playerSnap.position.z);
                }
                if (playerSnap.flags & PlayerSnapshot::Flags_Direction) {
                    stream.Process((uint8_t &)playerSnap.direction, 3, (uint8_t)Direction::North, (uint8_t)Direction::NorthWest);
                    stream.Align();
                }
                if (playerSnap.flags & PlayerSnapshot::Flags_Speed) {
                    stream.Process(playerSnap.speed);
                }
                if (playerSnap.flags & PlayerSnapshot::Flags_Health) {
                    stream.Process(playerSnap.hitPoints);
                }
                if (playerSnap.flags & PlayerSnapshot::Flags_HealthMax) {
                    stream.Process(playerSnap.hitPointsMax);
                }
                if (playerSnap.flags & PlayerSnapshot::Flags_Level) {
                    stream.Process(playerSnap.level);
                }
                if (playerSnap.flags & PlayerSnapshot::Flags_XP) {
                    stream.Process(playerSnap.xp);
                }
                if (playerSnap.flags & PlayerSnapshot::Flags_Inventory) {
                    //if (stream.Writing()) {
                    //    E_DEBUG("Sending player inventory update for player %u\n", playerSnap.id);
                    //}

                    stream.Process((uint8_t &)playerSnap.inventory.selectedSlot, 8, 0, PlayerInvSlot_Count - 1);

                    const size_t slotCount = ARRAY_SIZE(playerSnap.inventory.slots);
                    bool slotMap[slotCount]{};
                    for (size_t slot = 0; slot < slotCount; slot++) {
                        ItemStack &invStack = playerSnap.inventory.slots[slot].stack;
                        slotMap[slot] = invStack.count > 0;
                        stream.Process(slotMap[slot]);
                    }
                    stream.Align();

                    for (size_t slot = 0; slot < slotCount; slot++) {
                        if (slotMap[slot]) {
                            ItemStack &invStack = playerSnap.inventory.slots[slot].stack;
                            stream.Process(invStack.uid);
                            stream.Process(invStack.count);
                            DLB_ASSERT(invStack.uid);  // ensure stack with count > 0 has valid item ID

                            Item &item = g_item_db.FindOrCreate(invStack.uid);
                            DLB_ASSERT(item.uid);
                            if (stream.Writing()) {
                                DLB_ASSERT(item.type);
                            }
                            stream.Process(item.type);

                            // Default items have no rolled affixes that need to be sync'd
                            if (item.uid < ItemType_Count) {
                                continue;
                            }

                            const size_t affixCount = ARRAY_SIZE(item.affixes);
                            bool affixMap[affixCount]{};
                            for (int affix = 0; affix < affixCount; affix++) {
                                affixMap[affix] = item.affixes[affix].type != ItemAffix_Empty;
                                stream.Process(affixMap[affix]);
                            }
                            stream.Align();

                            for (int affix = 0; affix < affixCount; affix++) {
                                if (affixMap[affix]) {
                                    stream.Process(item.affixes[affix].type, 4, 0, ItemAffix_Count);
                                    stream.Process(item.affixes[affix].value.min);
                                    stream.Process(item.affixes[affix].value.max);
                                } else {
                                    item.affixes[affix] = {};
                                }
                            }
                        }
                    }
                }
            }

            for (size_t i = 0; i < worldSnapshot.npcCount; i++) {
                NpcSnapshot &npcSnap = worldSnapshot.npcs[i];
                stream.Process(npcSnap.id, 32, 1, UINT32_MAX);
                stream.Process((uint32_t &)npcSnap.flags);
                stream.Process((uint32_t &)npcSnap.type, 4, NPC::Type_None + 1, NPC::Type_Count - 1);
                if (npcSnap.flags & NpcSnapshot::Flags_Name) {
                    stream.Process(npcSnap.nameLength, 7, 0, ENTITY_NAME_LENGTH_MAX);
                    stream.Align();
                    for (size_t i = 0; i < npcSnap.nameLength; i++) {
                        stream.ProcessChar(npcSnap.name[i]);
                    }
                }
                if (npcSnap.flags & NpcSnapshot::Flags_Position) {
                    stream.Process(npcSnap.position.x);
                    stream.Process(npcSnap.position.y);
                    stream.Process(npcSnap.position.z);
                }
                if (npcSnap.flags & NpcSnapshot::Flags_Direction) {
                    stream.Process((uint8_t &)npcSnap.direction, 3, (uint8_t)Direction::North, (uint8_t)Direction::NorthWest);
                    stream.Align();
                }
                if (npcSnap.flags & NpcSnapshot::Flags_Scale) {
                    stream.Process(npcSnap.scale);
                }
                if (npcSnap.flags & NpcSnapshot::Flags_Health) {
                    stream.Process(npcSnap.hitPoints);
                }
                if (npcSnap.flags & NpcSnapshot::Flags_HealthMax) {
                    stream.Process(npcSnap.hitPointsMax);
                }
                if (npcSnap.flags & NpcSnapshot::Flags_Level) {
                    stream.Process(npcSnap.level);
                }
            }

            for (size_t i = 0; i < worldSnapshot.itemCount; i++) {
                ItemSnapshot &itemSnap = worldSnapshot.items[i];
                stream.Process(itemSnap.id, 32, 1, UINT32_MAX);
                stream.Process((uint32_t &)itemSnap.flags);
                if (itemSnap.flags & ItemSnapshot::Flags_Position) {
                    stream.Process(itemSnap.position.x);
                    stream.Process(itemSnap.position.y);
                    stream.Process(itemSnap.position.z);
                }
                if (itemSnap.flags & ItemSnapshot::Flags_ItemUid) {
                    stream.Process(itemSnap.itemUid);

                    Item &item = g_item_db.FindOrCreate(itemSnap.itemUid);
                    DLB_ASSERT(item.uid);
                    if (stream.Writing()) {
                        DLB_ASSERT(item.type);
                    }
                    stream.Process(item.type);
                }
                if (itemSnap.flags & ItemSnapshot::Flags_StackCount) {
                    stream.Process(itemSnap.stackCount);
                }
            }

            break;
        } case NetMessage::Type::GlobalEvent: {
            NetMessage_GlobalEvent &globalEvent = data.globalEvent;

            stream.Process((uint32_t &)globalEvent.type, 3, (uint32_t)NetMessage_GlobalEvent::Type::Unknown + 1, (uint32_t)NetMessage_GlobalEvent::Type::Count - 1);
            stream.Align();

            switch (globalEvent.type) {
                case NetMessage_GlobalEvent::Type::PlayerJoin: {
                    NetMessage_GlobalEvent::PlayerJoin &playerJoin = globalEvent.data.playerJoin;

                    stream.Process(playerJoin.playerId, 32, 1, UINT32_MAX);
                    stream.Align();

                    stream.Process(playerJoin.nameLength, 6, USERNAME_LENGTH_MIN, USERNAME_LENGTH_MAX);
                    stream.Align();
                    for (size_t i = 0; i < playerJoin.nameLength; i++) {
                        stream.ProcessChar(playerJoin.name[i]);
                    }
                    break;
                } case NetMessage_GlobalEvent::Type::PlayerLeave: {
                    NetMessage_GlobalEvent::PlayerLeave &playerLeave = globalEvent.data.playerLeave;

                    stream.Process(playerLeave.playerId, 32, 1, UINT32_MAX);
                    stream.Align();
                    break;
                } default: {
                    TraceLog(LOG_ERROR, "Unexpected message");
                }
            }

            break;
        } case NetMessage::Type::NearbyEvent: {
            NetMessage_NearbyEvent &nearbyEvent = data.nearbyEvent;

            stream.Process((uint32_t &)nearbyEvent.type, 4, (uint32_t)NetMessage_NearbyEvent::Type::Unknown + 1, (uint32_t)NetMessage_NearbyEvent::Type::Count - 1);
            stream.Align();

            switch (nearbyEvent.type) {
                case NetMessage_NearbyEvent::Type::PlayerState: {
#if 0
                    NetMessage_NearbyEvent::PlayerState &state = nearbyEvent.data.playerState;
                    stream.Process(state.type, 32, 1, UINT32_MAX);
                    stream.ProcessBool(state.nearby);
                    if (state.nearby) {
                        stream.ProcessBool(state.spawned);
                        stream.ProcessBool(state.attacked);
                        stream.ProcessBool(state.moved);
                        stream.ProcessBool(state.tookDamage);
                        stream.ProcessBool(state.healed);
                        stream.Align();
                        if (state.moved) {
                            stream.ProcessFloat(state.position.x);
                            stream.ProcessFloat(state.position.y);
                            stream.ProcessFloat(state.position.z);
                            stream.Process((uint32_t &)state.direction, 3, (uint32_t)Direction::North, (uint32_t)Direction::NorthWest);
                            stream.Align();
                        }
                        if (state.tookDamage || state.healed) {
                            stream.ProcessFloat(state.hitPoints);
                            stream.ProcessFloat(state.hitPointsMax);
                        }
                    }
#else
                    TraceLog(LOG_ERROR, "Unexpected message");
#endif
                    break;
                } case NetMessage_NearbyEvent::Type::PlayerEquip: {
                    TraceLog(LOG_ERROR, "Unexpected message");
                    break;
                } case NetMessage_NearbyEvent::Type::EnemyState: {
#if 0
                    NetMessage_NearbyEvent::EnemyState &state = nearbyEvent.data.enemyState;
                    stream.Process(state.type, 32, 1, UINT32_MAX);
                    stream.Process(state.nearby);
                    if (state.nearby) {
                        stream.Process(state.spawned);
                        stream.Process(state.attacked);
                        stream.Process(state.moved);
                        stream.Process(state.tookDamage);
                        stream.Process(state.healed);
                        stream.Align();
                        if (state.moved) {
                            stream.Process(state.position.x);
                            stream.Process(state.position.y);
                            stream.Process(state.position.z);
                            stream.Process((uint8_t &)state.direction, 3, (uint8_t)Direction::North, (uint8_t)Direction::NorthWest);
                            stream.Align();
                        }
                        if (state.tookDamage || state.healed) {
                            stream.Process(state.hitPoints);
                            stream.Process(state.hitPointsMax);
                        }
                    }
#else
                    TraceLog(LOG_ERROR, "Unexpected message");
#endif
                    break;
                } case NetMessage_NearbyEvent::Type::ItemState: {
#if 0
                    NetMessage_NearbyEvent::ItemState &state = nearbyEvent.data.itemState;
                    stream.Process(state.type, 32, 1, UINT32_MAX);
                    stream.ProcessBool(state.nearby);
                    if (state.nearby) {
                        stream.ProcessBool(state.spawned);
                        stream.ProcessBool(state.moved);
                        stream.Align();
                        if (state.moved) {
                            stream.ProcessFloat(state.position.x);
                            stream.ProcessFloat(state.position.y);
                            stream.ProcessFloat(state.position.z);
                            stream.Align();
                        }
                    }
#else
                    TraceLog(LOG_ERROR, "Unexpected message");
#endif
                    break;
                } default: {
                    TraceLog(LOG_ERROR, "Unexpected message");
                }
            }

            break;
        } case NetMessage::Type::SlotClick: {
            NetMessage_SlotClick &slotClick = data.slotClick;
            stream.Process((uint8_t &)slotClick.slotId, 8, 0, PlayerInvSlot_Count - 1);
            stream.Align();
            stream.Process(slotClick.doubleClick);
            break;
        } case NetMessage::Type::SlotScroll: {
            NetMessage_SlotScroll &slotScroll = data.slotScroll;
            stream.Process((uint8_t &)slotScroll.slotId, 8, 0, PlayerInvSlot_Count - 1);
            stream.Align();
            stream.Process(slotScroll.scrollY);
            break;
        } case NetMessage::Type::SlotDrop: {
            NetMessage_SlotDrop &slotDrop = data.slotDrop;
            stream.Process((uint8_t &)slotDrop.slotId, 8, 0, PlayerInvSlot_Count - 1);
            stream.Align();
            stream.Process(slotDrop.count);
            break;
        } case NetMessage::Type::TileInteract: {
            NetMessage_TileInteract &tileInteract = data.tileInteract;
            stream.Process(tileInteract.tileX);
            stream.Process(tileInteract.tileY);
            stream.Align();
            break;
        } default: {
            assert(!"Unrecognized NetMessageType");
        }
    }

    stream.Flush();
    size_t bytesProcessed = stream.BytesProcessed();

#if _DEBUG && 0
    if (itemClass == NetMessage::Type::WorldSnapshot) {
        thread_local FILE *sendLog = fopen("send.log", "w");
        fprintf(sendLog, "Snapshot #%u: ", data.worldSnapshot.tick);
        for (size_t i = 0; i < bytesProcessed; i++) {
            fprintf(sendLog, "%02hhx", *((uint8_t *)buffer.data + i));
        }
        putc('\n', sendLog);
        //fclose(sendLog);
    }
#endif

    return bytesProcessed;
}

size_t NetMessage::Serialize(ENetBuffer &buffer)
{
    assert(buffer.data);
    assert(buffer.dataLength);
    size_t bytesProcessed = Process(BitStream::Mode::Writer, buffer);
    assert(bytesProcessed);
    return bytesProcessed;
}

size_t NetMessage::Deserialize(const ENetBuffer &buffer)
{
    assert(buffer.data);
    assert(buffer.dataLength);
    size_t bytesProcessed = Process(BitStream::Mode::Reader, (ENetBuffer &)buffer);
    assert(bytesProcessed);
    return bytesProcessed;
}