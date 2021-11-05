#include "bit_stream.h"
#include "helpers.h"
#include "net_message.h"
#include "slime.h"
#include "tilemap.h"
#include <cassert>

size_t NetMessage::Serialize(uint32_t *buffer, size_t bufferLength) const
{
    BitStreamWriter writer(buffer, bufferLength);
    Serialize(writer);
    const size_t bytesWritten = writer.BytesWritten();
    return bytesWritten;
}

NetMessage &NetMessage::Deserialize(uint32_t *buffer, size_t bufferLength)
{
    BitStreamReader reader(buffer, bufferLength);
    NetMessage::Type type = (NetMessage::Type)reader.Read(3);
    reader.Align();

    // TODO(perf): Where do these get freed?
    NetMessage *msg = nullptr;
    switch (type) {
        case NetMessage::Type::Identify: {
            msg = new NetMessage_Identify();
            break;
        } case NetMessage::Type::ChatMessage: {
            msg = new NetMessage_ChatMessage();
            break;
        } case NetMessage::Type::Welcome: {
            msg = new NetMessage_Welcome();
            break;
        } case NetMessage::Type::WorldChunk: {
            msg = new NetMessage_WorldChunk();
            break;
        } case NetMessage::Type::WorldEntities: {
            msg = new NetMessage_WorldEntities();
            break;
        } default: {
            assert(!"Unrecognized NetMessageType");
        }
    }
    msg->Deserialize(reader);
    return *msg;
}

void NetMessage::Serialize(BitStreamWriter &writer) const
{
    writer.Write((int)type, 3);
    writer.Align();
}

void NetMessage_Identify::Serialize(BitStreamWriter &writer) const
{
    NetMessage::Serialize(writer);

    assert(usernameLength <= USERNAME_LENGTH_MAX);
    writer.Write((uint32_t)usernameLength, 5);
    writer.Align();

    for (size_t i = 0; i < usernameLength; i++) {
        writer.Write(username[i], 8);
    }

    assert(passwordLength <= PASSWORD_LENGTH_MAX);
    writer.Write((uint32_t)passwordLength, 5);
    writer.Align();

    for (size_t i = 0; i < passwordLength; i++) {
        writer.Write(password[i], 8);
    }

    writer.Flush();
}
void NetMessage_Identify::Deserialize(BitStreamReader &reader)
{
    usernameLength = reader.Read(5);
    assert(usernameLength <= USERNAME_LENGTH_MAX);
    reader.Align();

    username = reader.BufferPtr();
    for (size_t i = 0; i < usernameLength; i++) {
        reader.Read(8);
    }

    passwordLength = reader.Read(5);
    assert(passwordLength <= PASSWORD_LENGTH_MAX);
    reader.Align();

    password = reader.BufferPtr();
    for (size_t i = 0; i < passwordLength; i++) {
        reader.Read(8);
    }
}

void NetMessage_ChatMessage::Serialize(BitStreamWriter &writer) const
{
    NetMessage::Serialize(writer);

    assert(usernameLength <= USERNAME_LENGTH_MAX);
    assert(messageLength <= CHAT_MESSAGE_LENGTH_MAX);
    writer.Write((uint32_t)usernameLength, 5);
    writer.Align();

    for (size_t i = 0; i < usernameLength; i++) {
        writer.Write(username[i], 8);
    }

    writer.Write((uint32_t)messageLength, 9);
    writer.Align();

    for (size_t i = 0; i < messageLength; i++) {
        writer.Write(message[i], 8);
    }

    writer.Flush();
}
void NetMessage_ChatMessage::Deserialize(BitStreamReader &reader)
{
    usernameLength = reader.Read(5);
    assert(usernameLength <= USERNAME_LENGTH_MAX);
    reader.Align();

    username = reader.BufferPtr();
    for (size_t i = 0; i < usernameLength; i++) {
        reader.Read(8);
    }

    messageLength = reader.Read(9);
    assert(messageLength <= CHAT_MESSAGE_LENGTH_MAX);
    reader.Align();

    message = reader.BufferPtr();
    for (size_t i = 0; i < messageLength; i++) {
        reader.Read(8);
    }
}

void NetMessage_Welcome::Serialize(BitStreamWriter &writer) const
{
    NetMessage::Serialize(writer);

    assert(motdLength <= MOTD_LENGTH_MAX);
    writer.Write((uint32_t)motdLength, 6);
    writer.Align();

    for (size_t i = 0; i < motdLength; i++) {
        writer.Write(motd[i], 8);
    }

    assert(width * height <= WORLD_MAX_TILES);
    writer.Write((uint32_t)width, 8);
    writer.Align();
    writer.Write((uint32_t)height, 8);
    writer.Align();

    writer.Flush();
}
void NetMessage_Welcome::Deserialize(BitStreamReader &reader)
{
    motdLength = reader.Read(6);
    assert(motdLength <= MOTD_LENGTH_MAX);
    reader.Align();

    motd = reader.BufferPtr();
    for (size_t i = 0; i < motdLength; i++) {
        reader.Read(8);
    }

    width = reader.Read(8);
    //reader.Align();
    height = reader.Read(8);
    //reader.Align();
    assert(width * height <= WORLD_MAX_TILES);

}

void NetMessage_WorldChunk::Serialize(BitStreamWriter &writer) const
{
    NetMessage::Serialize(writer);

    // TODO: Assert offsets/rowWidth are in a sane range?
    assert(tilesLength <= WORLD_MAX_TILES);
    
    writer.Write((uint32_t)offsetX, 8);
    writer.Align();
    writer.Write((uint32_t)offsetY, 8);
    writer.Align();
    writer.Write((uint32_t)rowWidth, 8);
    writer.Align();
    writer.Write((uint32_t)tilesLength, 16);
    writer.Align();

    for (size_t i = 0; i < tilesLength; i++) {
        const Tile &tile = tiles[i];
        writer.Write((uint8_t)tile.tileType, 8);
    }

    writer.Flush();
}
void NetMessage_WorldChunk::Deserialize(BitStreamReader &reader)
{
    offsetX = reader.Read(8);
    //reader.Align();
    offsetY = reader.Read(8);
    //reader.Align();
    rowWidth = reader.Read(8);
    //reader.Align();

    tilesLength = reader.Read(16);
    //reader.Align();
    assert(tilesLength <= WORLD_MAX_TILES);

    netTiles = (NetTile *)reader.BufferPtr();
    for (size_t i = 0; i < tilesLength; i++) {
        reader.Read(8);
    }
}

void NetMessage_WorldEntities::Serialize(BitStreamWriter &writer) const
{
    NetMessage::Serialize(writer);

    // TODO: Assert offsets/rowWidth are in a sane range?
    assert(entitiesLength <= WORLD_MAX_ENTITIES);

    writer.Write((uint32_t)entitiesLength, 32);
    writer.Align();

    for (size_t i = 0; i < entitiesLength; i++) {
        const Slime &slime = entities[i];
        writer.Write((uint32_t)slime.body.position.x, 32);
        writer.Write((uint32_t)slime.body.position.y, 32);
    }

    writer.Flush();
}
void NetMessage_WorldEntities::Deserialize(BitStreamReader &reader)
{
    entitiesLength = reader.Read(32);
    //reader.Align();
    assert(entitiesLength <= WORLD_MAX_ENTITIES);

    netEntities = (NetEntity *)reader.BufferPtr();
    for (size_t i = 0; i < entitiesLength; i++) {
        reader.Read(32);
        reader.Read(32);
    }
}