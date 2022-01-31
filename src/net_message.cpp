#include "bit_stream.h"
#include "helpers.h"
#include "net_message.h"
#include "slime.h"
#include "tilemap.h"
#include <cassert>

void NetMessage::Process(BitStream::Mode mode, ENetBuffer &buffer, World &world)
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
        } case NetMessage::Type::Input: {
            NetMessage_Input &input = data.input;

            stream.Process(input.sampleCount, 6, 0, CL_INPUT_SAMPLES_MAX);
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
                } else {
                    // TODO: Don't send every seq number, it's implicit based on the first seq number and count
                    stream.Process(sample.seq, 32, 0, UINT32_MAX);
                    // TODO: Don't send ownerId more than once.. move this up outside of the loop
                    stream.Process(sample.ownerId, 32, 0, UINT32_MAX);
                    stream.Process(sample.walkNorth);
                    stream.Process(sample.walkEast);
                    stream.Process(sample.walkSouth);
                    stream.Process(sample.walkWest);
                    stream.Process(sample.run);
                    stream.Process(sample.attack);
                    stream.Process(sample.selectSlot, 4, (uint32_t)PlayerInventorySlot::None, (uint32_t)PlayerInventorySlot::Slot_6);
                }
            }
            stream.Align();

            break;
        } case NetMessage::Type::WorldChunk: {
            NetMessage_WorldChunk &worldChunk = data.worldChunk;

            stream.Process(worldChunk.chunk.x, 16, 0, WORLD_WIDTH_MAX);
            stream.Process(worldChunk.chunk.y, 16, 0, WORLD_HEIGHT_MAX);

            // TODO(perf): RLE compression
            // https://moddingwiki.shikadi.net/wiki/RLE_Compression#Code
            for (size_t i = 0; i < ARRAY_SIZE(worldChunk.chunk.tiles); i++) {
                stream.Process((uint8_t &)worldChunk.chunk.tiles[i].type, 4, (uint8_t)Tile::Type::Grass, (uint8_t)Tile::Type::Count);
            }
            stream.Align();

            // TODO(cleanup): Move this out to net_client.cpp
            if (mode == BitStream::Mode::Reader) {
                world.map->GenerateMinimap();
            }

            break;
        } case NetMessage::Type::WorldSnapshot: {
            WorldSnapshot &worldSnapshot = data.worldSnapshot;

            stream.Process(worldSnapshot.tick, 32, 1, UINT32_MAX);
            stream.Process(worldSnapshot.lastInputAck);
            stream.Process(worldSnapshot.playerCount, 4, 0, SNAPSHOT_MAX_PLAYERS);
            stream.Process(worldSnapshot.enemyCount, 9, 0, SNAPSHOT_MAX_SLIMES);
            stream.Process(worldSnapshot.itemCount, 9, 0, SNAPSHOT_MAX_ITEMS);
            stream.Align();

            for (size_t i = 0; i < worldSnapshot.playerCount; i++) {
                PlayerSnapshot &player = worldSnapshot.players[i];
                stream.Process(player.id, 32, 1, UINT32_MAX);
                stream.Process((uint32_t &)player.flags, 8, 0, UINT8_MAX);
                if (player.flags & PlayerSnapshot::Flags::Position) {
                    stream.Process(player.position.x);
                    stream.Process(player.position.y);
                    stream.Process(player.position.z);
                }
                if (player.flags & PlayerSnapshot::Flags::Direction) {
                    stream.Process((uint8_t &)player.direction, 3, (uint8_t)Direction::North, (uint8_t)Direction::NorthWest);
                    stream.Align();
                }
                if (player.flags & PlayerSnapshot::Flags::Health) {
                    stream.Process(player.hitPoints);
                }
                if (player.flags & PlayerSnapshot::Flags::HealthMax) {
                    stream.Process(player.hitPointsMax);
                }
            }

            for (size_t i = 0; i < worldSnapshot.enemyCount; i++) {
                EnemySnapshot &enemy = worldSnapshot.enemies[i];
                stream.Process(enemy.id, 32, 1, UINT32_MAX);
                assert((uint32_t)enemy.flags <= UINT8_MAX);
                stream.Process((uint8_t &)enemy.flags);
                if (enemy.flags & EnemySnapshot::Flags::Position) {
                    stream.Process(enemy.position.x);
                    stream.Process(enemy.position.y);
                    stream.Process(enemy.position.z);
                }
                if (enemy.flags & EnemySnapshot::Flags::Direction) {
                    stream.Process((uint8_t &)enemy.direction, 3, (uint8_t)Direction::North, (uint8_t)Direction::NorthWest);
                    stream.Align();
                }
                if (enemy.flags & EnemySnapshot::Flags::Scale) {
                    stream.Process(enemy.scale);
                }
                if (enemy.flags & EnemySnapshot::Flags::Health) {
                    stream.Process(enemy.hitPoints);
                }
                if (enemy.flags & EnemySnapshot::Flags::HealthMax) {
                    stream.Process(enemy.hitPointsMax);
                }
            }

            for (size_t i = 0; i < worldSnapshot.itemCount; i++) {
                ItemSnapshot &item = worldSnapshot.items[i];
                stream.Process(item.id, 32, 1, UINT32_MAX);
                assert((uint32_t)item.flags <= UINT8_MAX);
                stream.Process((uint8_t &)item.flags);
                if (item.flags & ItemSnapshot::Flags::Position) {
                    stream.Process(item.position.x);
                    stream.Process(item.position.y);
                    stream.Process(item.position.z);
                }
                if (item.flags & ItemSnapshot::Flags::CatalogId) {
                    stream.Process((uint32_t &)item.catalogId, 3, (uint32_t)Catalog::ItemID::Empty + 1, (uint32_t)Catalog::ItemID::Count - 1);
                }
                if (item.flags & ItemSnapshot::Flags::StackCount) {
                    stream.Process(item.stackCount, 32, 1, UINT_MAX);
                }
                if (item.flags & ItemSnapshot::Flags::PickedUp) {
                    stream.Process(item.pickedUp);
                    stream.Align();
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

                    TraceLog(LOG_DEBUG, "PlayerJoin %u", playerJoin.playerId);

                    break;
                } case NetMessage_GlobalEvent::Type::PlayerLeave: {
                    NetMessage_GlobalEvent::PlayerLeave &playerLeave = globalEvent.data.playerLeave;

                    stream.Process(playerLeave.playerId, 32, 1, UINT32_MAX);
                    stream.Align();
                    break;
                } default: {
                    TraceLog(LOG_FATAL, "Unexpected message");
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
                    stream.Process(state.id, 32, 1, UINT32_MAX);
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
                    TraceLog(LOG_FATAL, "Unexpected message");
#endif
                    break;
                } case NetMessage_NearbyEvent::Type::PlayerEquip: {
                    TraceLog(LOG_FATAL, "Unexpected message");
                    break;
                } case NetMessage_NearbyEvent::Type::EnemyState: {
#if 1
                    NetMessage_NearbyEvent::EnemyState &state = nearbyEvent.data.enemyState;
                    stream.Process(state.id, 32, 1, UINT32_MAX);
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
                    TraceLog(LOG_FATAL, "Unexpected message");
#endif
                    break;
                } case NetMessage_NearbyEvent::Type::ItemState: {
#if 0
                    NetMessage_NearbyEvent::ItemState &state = nearbyEvent.data.itemState;
                    stream.Process(state.id, 32, 1, UINT32_MAX);
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
                    TraceLog(LOG_FATAL, "Unexpected message");
#endif
                    break;
                } default: {
                    TraceLog(LOG_FATAL, "Unexpected message");
                }
            }

            break;
        } default: {
            assert(!"Unrecognized NetMessageType");
        }
    }

    stream.Flush();
    buffer.dataLength = stream.BytesProcessed();

#if _DEBUG && 0
    if (type == NetMessage::Type::WorldSnapshot) {
        static FILE *sendLog = fopen("send.log", "w");
        fprintf(sendLog, "Snapshot #%u: ", data.worldSnapshot.tick);
        for (size_t i = 0; i < buffer.dataLength; i++) {
            fprintf(sendLog, "%02hhx", *((uint8_t *)buffer.data + i));
        }
        putc('\n', sendLog);
        //fclose(sendLog);
    }
#endif
}

void NetMessage::Serialize(const World &world, ENetBuffer &buffer)
{
    Process(BitStream::Mode::Writer, buffer, (World &)world);
    assert(buffer.data);
    assert(buffer.dataLength);
}

void NetMessage::Deserialize(World &world, const ENetBuffer &buffer)
{
    assert(buffer.data);
    assert(buffer.dataLength);
    Process(BitStream::Mode::Reader, (ENetBuffer &)buffer, world);
}