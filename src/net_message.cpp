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
    NetMessageType type = (NetMessageType)reader.Read(2);
    reader.Align();

    NetMessage *msg = nullptr;
    switch (type) {
        case NetMessageType_Identify: {
            msg = new NetMessage_Identify();
            break;
        } case NetMessageType_Welcome: {
            msg = new NetMessage_Welcome();
            break;
        } case NetMessageType_ChatMessage: {
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
    writer.Write(m_type, 2);
    writer.Align();
}

void NetMessage_Identify::Serialize(BitStreamWriter &writer) const
{
    NetMessage::Serialize(writer);

    assert(m_usernameLength <= USERNAME_LENGTH_MAX);
    writer.Write((uint32_t)m_usernameLength, 5);
    writer.Align();

    for (size_t i = 0; i < m_usernameLength; i++) {
        writer.Write(m_username[i], 8);
    }
    writer.Flush();
}

void NetMessage_Welcome::Serialize(BitStreamWriter &writer) const
{
    NetMessage::Serialize(writer);
    writer.Flush();
}

void NetMessage_ChatMessage::Serialize(BitStreamWriter &writer) const
{
    NetMessage::Serialize(writer);

    assert(m_usernameLength <= USERNAME_LENGTH_MAX);
    assert(m_messageLength <= CHAT_MESSAGE_LENGTH_MAX);
    writer.Write((uint32_t)m_usernameLength, 5);
    writer.Align();

    for (size_t i = 0; i < m_usernameLength; i++) {
        writer.Write(m_username[i], 8);
    }

    writer.Write((uint32_t)m_messageLength, 9);
    writer.Align();

    for (size_t i = 0; i < m_messageLength; i++) {
        writer.Write(m_message[i], 8);
    }
    writer.Flush();
}

void NetMessage_Identify::Deserialize(BitStreamReader &reader)
{
    m_usernameLength = reader.Read(5);
    assert(m_usernameLength <= USERNAME_LENGTH_MAX);
    reader.Align();

    m_username = reader.BufferPtr();
    for (size_t i = 0; i < m_usernameLength; i++) {
        reader.Read(8);
    }
}

void NetMessage_Welcome::Deserialize(BitStreamReader &reader)
{
    UNUSED(reader);
}

void NetMessage_ChatMessage::Deserialize(BitStreamReader &reader)
{
    m_usernameLength = reader.Read(5);
    assert(m_usernameLength <= USERNAME_LENGTH_MAX);
    reader.Align();

    m_username = reader.BufferPtr();
    for (size_t i = 0; i < m_usernameLength; i++) {
        reader.Read(8);
    }

    m_messageLength = reader.Read(9);
    assert(m_messageLength <= CHAT_MESSAGE_LENGTH_MAX);
    reader.Align();

    m_message = reader.BufferPtr();
    for (size_t i = 0; i < m_messageLength; i++) {
        reader.Read(8);
    }
}