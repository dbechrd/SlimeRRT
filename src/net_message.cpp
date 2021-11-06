#include "bit_stream.h"
#include "helpers.h"
#include "net_message.h"
#include "slime.h"
#include "tilemap.h"
#include <cassert>

size_t NetMessage::Process(bool reader, uint32_t *buffer, size_t bufferLength)
{
    BitStream::Mode mode = reader ? BitStream::Mode::Reader : BitStream::Mode::Writer;
    BitStream stream(mode, buffer, bufferLength);

    uint32_t typeInt = (uint32_t)type;
    stream.Process(typeInt, 3, (uint32_t)NetMessage::Type::Unknown + 1, (uint32_t)NetMessage::Type::Count - 1);
    type = (Type)typeInt;
    stream.Align();

    switch (type) {
        case NetMessage::Type::Identify: {
            NetMessage_Identify &ident = data.identify;

            stream.Process(ident.usernameLength, 5, USERNAME_LENGTH_MIN, USERNAME_LENGTH_MAX);
            stream.Align();

            if (stream.Reading()) {
                ident.username = stream.BufferPtr();
            }
            for (size_t i = 0; i < ident.usernameLength; i++) {
                uint32_t userChr = ident.username[i];
                stream.Process(userChr, 8, STRING_ASCII_MIN, STRING_ASCII_MAX);
            }

            stream.Process(ident.passwordLength, 5, PASSWORD_LENGTH_MIN, PASSWORD_LENGTH_MAX);
            stream.Align();

            if (stream.Reading()) {
                ident.password = stream.BufferPtr();
            }
            for (size_t i = 0; i < ident.passwordLength; i++) {
                uint32_t passChr = ident.password[i];
                stream.Process(passChr, 8, STRING_ASCII_MIN, STRING_ASCII_MAX);
            }

            break;
        } case NetMessage::Type::ChatMessage: {
            NetMessage_ChatMessage &chatMsg = data.chatMsg;

            assert(chatMsg.messageLength <= CHAT_MESSAGE_LENGTH_MAX);
            stream.Process((uint32_t)chatMsg.usernameLength, 5, USERNAME_LENGTH_MIN, USERNAME_LENGTH_MAX);
            stream.Align();

            if (stream.Reading()) {
                chatMsg.username = stream.BufferPtr();
            }
            for (size_t i = 0; i < chatMsg.usernameLength; i++) {
                uint32_t usernameChr = chatMsg.username[i];
                stream.Process(usernameChr, 8, STRING_ASCII_MIN, STRING_ASCII_MAX);
            }

            stream.Process((uint32_t)chatMsg.messageLength, 9, CHAT_MESSAGE_LENGTH_MIN, CHAT_MESSAGE_LENGTH_MAX);
            stream.Align();

            if (stream.Reading()) {
                chatMsg.message = stream.BufferPtr();
            }
            for (size_t i = 0; i < chatMsg.messageLength; i++) {
                uint32_t messageChr = chatMsg.message[i];
                stream.Process(messageChr, 8, STRING_ASCII_MIN, STRING_ASCII_MAX);
            }

            break;
        } case NetMessage::Type::Welcome: {
            NetMessage_Welcome &welcome = data.welcome;

            stream.Process(welcome.motdLength, 6, MOTD_LENGTH_MIN, MOTD_LENGTH_MAX);
            stream.Align();

            if (stream.Reading()) {
                welcome.motd = stream.BufferPtr();
            }
            for (size_t i = 0; i < welcome.motdLength; i++) {
                uint32_t motdChr = welcome.motd[i];
                stream.Process(motdChr, 8, STRING_ASCII_MIN, STRING_ASCII_MAX);
            }

            stream.Process(welcome.width, 8, WORLD_WIDTH_MIN, WORLD_WIDTH_MAX);
            stream.Process(welcome.height, 8, WORLD_HEIGHT_MIN, WORLD_HEIGHT_MAX);

            break;
        } case NetMessage::Type::WorldChunk: {
            NetMessage_WorldChunk &worldChunk = data.worldChunk;

            stream.Process(worldChunk.offsetX, 8, 0, WORLD_WIDTH_MAX - WORLD_CHUNK_WIDTH);
            stream.Process(worldChunk.offsetY, 8, 0, WORLD_HEIGHT_MAX - WORLD_CHUNK_HEIGHT);
            stream.Process(worldChunk.tilesLength, 16, 0, WORLD_MAX_TILES);

            if (stream.Reading()) {
                worldChunk.tiles = (Tile *)calloc(worldChunk.tilesLength, sizeof(*worldChunk.tiles));
            }
            for (size_t i = 0; i < worldChunk.tilesLength; i++) {
                Tile &tile = worldChunk.tiles[i];
                uint32_t tileType = (uint32_t)tile.tileType;
                stream.Process(tileType, 3, 0, (uint32_t)TileType::Count - 1);
                tile.tileType = (TileType)tileType;
            }
            stream.Align();

            break;
        } case NetMessage::Type::WorldEntities: {
            NetMessage_WorldEntities &worldEntities = data.worldEntities;

            stream.Process((uint32_t)worldEntities.entitiesLength, 32, 0, WORLD_MAX_ENTITIES);
            stream.Align();

            if (stream.Reading()) {
                worldEntities.entities = (Slime *)calloc(worldEntities.entitiesLength, sizeof(*worldEntities.entities));
            }
            for (size_t i = 0; i < worldEntities.entitiesLength; i++) {
                Slime &slime = worldEntities.entities[i];
                uint32_t position_x = *(uint32_t *)&slime.body.position.x;
                uint32_t position_y = *(uint32_t *)&slime.body.position.y;
                stream.Process(position_x, 32, ENTITY_POSITION_X_MIN, ENTITY_POSITION_X_MAX);
                stream.Process(position_y, 32, ENTITY_POSITION_Y_MIN, ENTITY_POSITION_Y_MAX);
                slime.body.position.x = *(float *)&position_x;
                slime.body.position.y = *(float *)&position_y;
            }

            break;
        } default: {
            assert(!"Unrecognized NetMessageType");
        }
    }

    stream.Flush();
    const size_t bytesProcessed = stream.BytesProcessed();
    return bytesProcessed;
}

size_t NetMessage::Serialize(uint32_t *buffer, size_t bufferLength)
{
    const size_t bytesProcessed = Process(false, buffer, bufferLength);
    return bytesProcessed;
}

size_t NetMessage::Deserialize(uint32_t *buffer, size_t bufferLength)
{
    const size_t bytesProcessed = Process(true, buffer, bufferLength);
    return bytesProcessed;
}