#include "bit_stream.h"
#include "helpers.h"
#include "net_message.h"
#include "slime.h"
#include "tilemap.h"
#include <cassert>

ENetBuffer NetMessage::tempBuffer{};

const char *NetMessage::TypeString(void)
{
    switch (type) {
        case Type::Unknown       : return "Unknown";
        case Type::Identify      : return "Identify";
        case Type::ChatMessage   : return "ChatMessage";
        case Type::Welcome       : return "Welcome";
        case Type::WorldChunk    : return "WorldChunk";
        case Type::WorldSnapshot : return "WorldSnapshot";
        default                  : return "[NetMessage::Type]";
    }
}

void NetMessage::Process(BitStream::Mode mode, ENetBuffer &buffer, World &world)
{
    switch (mode) {
        case BitStream::Mode::Reader: {
            assert(buffer.data);
            assert(buffer.dataLength);
            break;
        } case BitStream::Mode::Writer: {
            if (!tempBuffer.dataLength) {
                tempBuffer.dataLength = PACKET_SIZE_MAX;
                tempBuffer.data = calloc(tempBuffer.dataLength, sizeof(uint8_t));
            } else {
                memset(tempBuffer.data, 0, tempBuffer.dataLength);
            }
            buffer.data = tempBuffer.data;
            buffer.dataLength = tempBuffer.dataLength;
            break;
        }
    }

    BitStream stream(mode, buffer.data, buffer.dataLength);

    stream.Process(connectionToken, 32, 0, UINT32_MAX);

    uint32_t typeInt = (uint32_t)type;
    stream.Process(typeInt, 3, (uint32_t)NetMessage::Type::Unknown + 1, (uint32_t)NetMessage::Type::Count - 1);
    type = (Type)typeInt;
    stream.Align();

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

            assert(chatMsg.messageLength <= CHATMSG_LENGTH_MAX);
            stream.Process((uint32_t)chatMsg.usernameLength, 6, USERNAME_LENGTH_MIN, USERNAME_LENGTH_MAX);
            stream.Align();
            for (size_t i = 0; i < chatMsg.usernameLength; i++) {
                stream.ProcessChar(chatMsg.username[i]);
            }

            stream.Process((uint32_t)chatMsg.messageLength, 9, CHATMSG_LENGTH_MIN, CHATMSG_LENGTH_MAX);
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

            stream.Process(welcome.width, 9, WORLD_WIDTH_MIN, WORLD_WIDTH_MAX);
            stream.Process(welcome.height, 9, WORLD_HEIGHT_MIN, WORLD_HEIGHT_MAX);
            stream.Process(welcome.playerId, 4, 1, UINT32_MAX);
            stream.Align();

            break;
        } case NetMessage::Type::Input: {
            NetMessage_Input &input = data.input;

            stream.Process(input.sampleCount, 6, 0, CL_INPUT_SAMPLES_MAX);
            for (size_t i = 0; i < input.sampleCount; i++) {
                InputSample &sample = input.samples[i];
                bool same = i && sample.Equals(input.samples[i - 1]) && (sample.seq == (input.samples[i - 1].seq + 1));
                stream.ProcessBool(same);
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
                    if (i && mode == BitStream::Mode::Writer) {
                        printf("Diff sample!\n");
                    }
                    stream.Process(sample.seq, 32, 0, UINT32_MAX);
                    stream.Process(sample.ownerId, 32, 0, UINT32_MAX);
                    stream.ProcessBool(sample.walkNorth);
                    stream.ProcessBool(sample.walkEast);
                    stream.ProcessBool(sample.walkSouth);
                    stream.ProcessBool(sample.walkWest);
                    stream.ProcessBool(sample.run);
                    stream.ProcessBool(sample.attack);
                    stream.Process(sample.selectSlot, 4, (uint32_t)PlayerInventorySlot::None, (uint32_t)PlayerInventorySlot::Slot_6);
                }
            }
            stream.Align();

            break;
        } case NetMessage::Type::WorldChunk: {
            NetMessage_WorldChunk &worldChunk = data.worldChunk;

            stream.Process(worldChunk.offsetX, 8, 0, WORLD_WIDTH_MAX - WORLD_CHUNK_WIDTH);
            stream.Process(worldChunk.offsetY, 8, 0, WORLD_HEIGHT_MAX - WORLD_CHUNK_HEIGHT);
            stream.Process(worldChunk.tilesLength, 16, 0, WORLD_CHUNK_TILES_MAX);

            assert(worldChunk.tilesLength < world.map->width * world.map->width);
            for (size_t i = 0; i < worldChunk.tilesLength; i++) {
                Tile &tile = world.map->tiles[i];
                uint32_t tileType = (uint32_t)tile.tileType;
                stream.Process(tileType, 3, 0, (uint32_t)TileType::Count - 1);
                tile.tileType = (TileType)tileType;
            }
            stream.Align();

            if (mode == BitStream::Mode::Reader) {
                world.map->GenerateMinimap();
            }

            break;
        } case NetMessage::Type::WorldSnapshot: {
            WorldSnapshot &worldSnapshot = data.worldSnapshot;
            
            stream.Process(worldSnapshot.tick, 32, 1, UINT32_MAX);
            // TODO: Make this relative to tick to save bandwidth?
            stream.Process(worldSnapshot.lastInputAck, 32, 0, UINT32_MAX);
            stream.Process(worldSnapshot.playerCount, 4, 0, SNAPSHOT_MAX_PLAYERS);
            //stream.Align();
            stream.Process(worldSnapshot.slimeCount, 9, 0, SNAPSHOT_MAX_ENTITIES);
            //stream.Align();

#if _DEBUG
            if (!worldSnapshot.playerCount) {
                FILE *recvLog = fopen("recv.txt", "w");
                fprintf(recvLog, "Snapshot #%u: ", data.worldSnapshot.tick);
                for (size_t i = 0; i < buffer.dataLength / sizeof(uint32_t); i++) {
                    uint32_t word = ((uint32_t *)buffer.data)[i];
                    fprintf(recvLog, "%02hhx %02hhx %02hhx %02hhx ",
                        (uint8_t)((word >> 24) & 0xff),
                        (uint8_t)((word >> 16) & 0xff),
                        (uint8_t)((word >> 8) & 0xff),
                        (uint8_t)((word >> 0) & 0xff)
                    );
                }
                putc('\n', recvLog);
                fclose(recvLog);
                TraceLog(LOG_FATAL, "INVALID SNAPSHOT NOOOOO!!!! see recv.txt");
            }
#endif

            for (size_t i = 0; i < worldSnapshot.playerCount; i++) {
                PlayerSnapshot &player = worldSnapshot.players[i];

                stream.Process(player.id, 32, 0, UINT32_MAX);

                Player *existingPlayer = world.FindPlayer(player.id);
                if (player.id) {
                    if (!existingPlayer) {
                        assert(mode == BitStream::Mode::Reader);
                        world.SpawnPlayer(player.id);
                    }
                    // TODO: Don't sync name unless it has changed
                    stream.Process((uint32_t)player.nameLength, 6, USERNAME_LENGTH_MIN, USERNAME_LENGTH_MAX);
                    stream.Align();
                    for (size_t i = 0; i < player.nameLength; i++) {
                        stream.ProcessChar(player.name[i]);
                    }

                    // TODO: range validation on floats (why? malicious server?)
                    stream.ProcessFloat(player.position.x);
                    stream.ProcessFloat(player.position.y);
                    stream.ProcessFloat(player.position.z);
                    stream.ProcessFloat(player.hitPoints);
                    stream.ProcessFloat(player.hitPointsMax);
                    uint32_t facing = (uint32_t)player.direction;
                    stream.Process(facing, 3, (uint32_t)Direction::North, (uint32_t)Direction::NorthWest);
                    player.direction = (Direction)facing;
                } else if (!player.id && existingPlayer) {
                    assert(mode == BitStream::Mode::Reader);
                    world.DespawnPlayer(player.id);
                }
            }

            for (size_t i = 0; i < worldSnapshot.slimeCount; i++) {
                SlimeSnapshot &slime = worldSnapshot.slimes[i];

                bool alive = slime.id != 0;
                stream.ProcessBool(alive);
                if (!alive) {
                    continue;
                }

                uint32_t prevId = slime.id;
                stream.Process(slime.id, 32, 0, UINT32_MAX);

                Slime *existingSlime = world.FindSlime(slime.id);
                if (slime.id && !existingSlime) {
                    assert(mode == BitStream::Mode::Reader);
                    world.SpawnSlime(slime.id);
                } else if (!slime.id && existingSlime) {
                    assert(mode == BitStream::Mode::Reader);
                    world.DespawnSlime(slime.id);
                    continue; // Don't serialize other fields for inactive slimes
                }

                if (slime.id) {
                    // TODO: range validation on floats
                    stream.ProcessFloat(slime.position.x);
                    stream.ProcessFloat(slime.position.y);
                    stream.ProcessFloat(slime.position.z);
                    uint32_t facing = (uint32_t)slime.direction;
                    stream.Process(facing, 3, (uint32_t)Direction::North, (uint32_t)Direction::NorthWest);
                    slime.direction = (Direction)facing;
                    stream.ProcessFloat(slime.hitPoints);
                    stream.ProcessFloat(slime.hitPointsMax);
                    stream.ProcessFloat(slime.scale);
                }
            }

            break;
        } default: {
            assert(!"Unrecognized NetMessageType");
        }
    }

    stream.Flush();
    buffer.dataLength = stream.BytesProcessed();

    if (type == NetMessage::Type::WorldSnapshot) {
        static FILE *sendLog = fopen("send.txt", "w");
        fprintf(sendLog, "Snapshot #%u: ", data.worldSnapshot.tick);
        for (size_t i = 0; i < buffer.dataLength; i++) {
            fprintf(sendLog, "%02hhx", *((uint8_t *)buffer.data + i));
        }
        putc('\n', sendLog);
        //fclose(sendLog);
    }
}

ENetBuffer NetMessage::Serialize(World &world)
{
    ENetBuffer buffer{};
    Process(BitStream::Mode::Writer, buffer, world);
    assert(buffer.data);
    assert(buffer.dataLength);
    return buffer;
}

void NetMessage::Deserialize(ENetBuffer buffer, World &world)
{
    assert(buffer.data);
    assert(buffer.dataLength);
    Process(BitStream::Mode::Reader, buffer, world);
}