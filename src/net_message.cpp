#include "bit_stream.h"
#include "net_message.h"
#include "helpers.h"
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
    NetMessage::Type type = (NetMessage::Type)reader.Read(2);
    reader.Align();

    NetMessage *msg = nullptr;
    switch (type) {
        case NetMessage::Type::Identify: {
            msg = new NetMessage_Identify();
            break;
        } case NetMessage::Type::Welcome: {
            msg = new NetMessage_Welcome();
            break;
        } case NetMessage::Type::ChatMessage: {
            msg = new NetMessage_ChatMessage();
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
    writer.Write((int)type, 2);
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
    //writer.Flush();

    assert(passwordLength <= PASSWORD_LENGTH_MAX);
    writer.Write((uint32_t)passwordLength, 5);
    writer.Align();

    for (size_t i = 0; i < passwordLength; i++) {
        writer.Write(password[i], 8);
    }
    writer.Flush();
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
    //writer.Flush();

    assert(tilesLength <= MOTD_MAX_TILES);
    writer.Write((uint32_t)tilesLength, 7);
    writer.Align();

    for (size_t i = 0; i < tilesLength; i++) {
        writer.Write(tiles[i], 8);
    }

    writer.Flush();
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

void NetMessage_Welcome::Deserialize(BitStreamReader &reader)
{
    motdLength = reader.Read(6);
    assert(motdLength <= MOTD_LENGTH_MAX);
    reader.Align();

    motd = reader.BufferPtr();
    for (size_t i = 0; i < motdLength; i++) {
        reader.Read(8);
    }

    tilesLength = reader.Read(7);
    assert(tilesLength <= MOTD_MAX_TILES);
    reader.Align();

    tiles = (uint8_t *)reader.BufferPtr();
    for (size_t i = 0; i < tilesLength; i++) {
        reader.Read(8);
    }
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