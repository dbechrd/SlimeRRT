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
        case Type::WorldPlayers  : return "WorldPlayers";
        case Type::WorldEntities : return "WorldEntities";
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

    uint32_t typeInt = (uint32_t)type;
    stream.Process(typeInt, 3, (uint32_t)NetMessage::Type::Unknown + 1, (uint32_t)NetMessage::Type::Count - 1);
    type = (Type)typeInt;
    stream.Align();

    switch (type) {
        case NetMessage::Type::Identify: {
            NetMessage_Identify &ident = data.identify;

            stream.Process(ident.usernameLength, 5, USERNAME_LENGTH_MIN, USERNAME_LENGTH_MAX);
            stream.Align();

            for (size_t i = 0; i < ident.usernameLength; i++) {
                stream.ProcessChar(ident.username[i]);
            }

            stream.Process(ident.passwordLength, 5, PASSWORD_LENGTH_MIN, PASSWORD_LENGTH_MAX);
            stream.Align();

            for (size_t i = 0; i < ident.passwordLength; i++) {
                stream.ProcessChar(ident.password[i]);
            }

            break;
        } case NetMessage::Type::ChatMessage: {
            NetMessage_ChatMessage &chatMsg = data.chatMsg;

            assert(chatMsg.messageLength <= CHAT_MESSAGE_LENGTH_MAX);
            stream.Process((uint32_t)chatMsg.usernameLength, 5, USERNAME_LENGTH_MIN, USERNAME_LENGTH_MAX);
            stream.Align();

            for (size_t i = 0; i < chatMsg.usernameLength; i++) {
                stream.ProcessChar(chatMsg.username[i]);
            }

            stream.Process((uint32_t)chatMsg.messageLength, 9, CHAT_MESSAGE_LENGTH_MIN, CHAT_MESSAGE_LENGTH_MAX);
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
            stream.Process(welcome.playerIdx, 4, 0, SERVER_MAX_PLAYERS - 1);
            stream.Align();

            break;
        } case NetMessage::Type::Input: {
            NetMessage_Input &input = data.input;

            stream.ProcessBool(input.walkNorth);
            stream.ProcessBool(input.walkEast);
            stream.ProcessBool(input.walkSouth);
            stream.ProcessBool(input.walkWest);
            stream.ProcessBool(input.run);
            stream.ProcessBool(input.attack);
            stream.Align();

            uint32_t slot = (uint32_t)input.selectSlot;
            stream.Process(slot, 4, (uint32_t)PlayerInventorySlot::None, (uint32_t)PlayerInventorySlot::Slot_6);
            input.selectSlot = (PlayerInventorySlot)slot;
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
        } case NetMessage::Type::WorldPlayers: {
            NetMessage_WorldPlayers &worldPlayers = data.worldPlayers;

            stream.Process(world.playerCount, 5, 0, SERVER_MAX_PLAYERS);
            stream.Align();

            for (size_t i = 0; i < world.playerCount; i++) {
                Player &player = world.players[i];
                // TODO: range validation on floats
                world.InitPlayer(player, player.name, player.nameLength);
                stream.ProcessFloat(player.body.position.x);
                stream.ProcessFloat(player.body.position.y);
                stream.ProcessFloat(player.combat.hitPoints);
                stream.ProcessFloat(player.combat.maxHitPoints);
            }

            break;
        } case NetMessage::Type::WorldEntities: {
            NetMessage_WorldEntities &worldEntities = data.worldEntities;

            stream.Process(world.slimeCount, 32, 0, WORLD_ENTITIES_MAX);
            stream.Align();

            for (size_t i = 0; i < world.slimeCount; i++) {
                Slime &slime = world.slimes[i];
                world.InitSlime(slime);
                // TODO: range validation on floats
                stream.ProcessFloat(slime.body.position.x);
                stream.ProcessFloat(slime.body.position.y);
                stream.ProcessFloat(slime.body.position.z);
                stream.ProcessFloat(slime.combat.hitPoints);
                stream.ProcessFloat(slime.combat.maxHitPoints);
            }

            break;
        } default: {
            assert(!"Unrecognized NetMessageType");
        }
    }

    stream.Flush();
    buffer.dataLength = stream.BytesProcessed();
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