#include "bit_stream.h"
#include "packet.h"
#include <cassert>

void bit_stream_writer_init(BitStream *stream, uint32_t *buffer, size_t bufferSize)
{
    // Must be aligned to word boundary
    assert(bufferSize % 4 == 0);

    stream->buffer = buffer;
    stream->bufferSize = bufferSize;
}

void bit_stream_reader_init(BitStream *stream, uint32_t *buffer, size_t bytes)
{
    stream->buffer = buffer;
    stream->total_bits = bytes * 8;
}

static void bit_stream_flush(BitStream *stream)
{
    assert(stream->word_index < stream->bufferSize);
    stream->buffer[stream->word_index] = stream->scratch & 0xFFFFFFFF;
    stream->word_index++;
    stream->scratch >>= 32;
    stream->scratch_bits -= MIN(stream->scratch_bits, 32);
}

static void bit_stream_write(BitStream *stream, uint32_t word, uint8_t bits)
{
    assert(stream);
    assert(stream->buffer);
    assert(bits);

    uint64_t maskedWord = (((uint64_t)1 << bits) - 1) & word;
    stream->scratch |= maskedWord << stream->scratch_bits;
    stream->scratch_bits += bits;
    stream->total_bits += bits;

    if (stream->scratch_bits > 32) {
        bit_stream_flush(stream);
    }
}

static uint32_t bit_stream_read(BitStream *stream, uint8_t bits)
{
    assert(stream);
    assert(stream->buffer);
    assert(bits);

    assert(stream->num_bits_read + bits <= stream->total_bits);

    if (bits > stream->scratch_bits) {
        stream->scratch |= (uint64_t)stream->buffer[stream->word_index] << stream->scratch_bits;
        stream->word_index++;
        stream->scratch_bits += 32;
    }

    uint32_t word = (((uint64_t)1 << bits) - 1) & stream->scratch;
    stream->scratch >>= bits;
    stream->scratch_bits -= bits;
    stream->num_bits_read += bits;
    return word;
}

// https://gafferongames.com/post/serialization_strategies/
static void bit_stream_write_align(BitStream *stream)
{
    const uint8_t remainderBits = stream->total_bits % 8;
    if (remainderBits) {
        uint32_t zero = 0;
        bit_stream_write(stream, zero, 8 - remainderBits);
        assert(stream->scratch_bits % 8 == 0);
    }
}

static void bit_stream_read_align(BitStream *stream)
{
    const uint8_t remainderBits = stream->num_bits_read % 8;
    if (remainderBits) {
        uint32_t value = bit_stream_read(stream, 8 - remainderBits);
        assert(stream->scratch_bits % 8 == 0);
        if (value != 0) {
            assert(!"Alignment padding corruption detected; expected zero padding");
            //return false;
        }
    }
}

static void write_net_message_identify(BitStream *stream, const NetMessage_Identify *identify)
{
    assert(stream);
    assert(identify);
    assert(identify->usernameLength <= USERNAME_LENGTH_MAX);

    bit_stream_write(stream, (uint32_t)identify->usernameLength, 5);
    bit_stream_write_align(stream);
    for (size_t i = 0; i < identify->usernameLength; i++) {
        bit_stream_write(stream, identify->username[i], 8);
    }
}

static void read_net_message_identify(BitStream *stream, NetMessage_Identify *identify)
{
    assert(stream);
    assert(identify);

    identify->usernameLength = bit_stream_read(stream, 5);
    assert(identify->usernameLength <= USERNAME_LENGTH_MAX);
    bit_stream_read_align(stream);
    assert(stream->num_bits_read % 8 == 0);
    identify->username = (char *)stream->buffer + stream->num_bits_read / 8;
    for (size_t i = 0; i < identify->usernameLength; i++) {
        bit_stream_read(stream, 8);
    }
}

static void write_net_message_welcome(BitStream *stream, const NetMessage_Welcome *welcome)
{
    UNUSED(stream);
    UNUSED(welcome);
}

static void read_net_message_welcome(BitStream *stream, NetMessage_Welcome *welcome)
{
    UNUSED(stream);
    UNUSED(welcome);
}

static void write_net_message_chat_message(BitStream *stream, const NetMessage_ChatMessage *chatMessage)
{
    assert(stream);
    assert(chatMessage);
    assert(chatMessage->usernameLength <= USERNAME_LENGTH_MAX);
    assert(chatMessage->messageLength <= CHAT_MESSAGE_LENGTH_MAX);

    bit_stream_write(stream, (uint32_t)chatMessage->usernameLength, 5);
    bit_stream_write_align(stream);
    for (size_t i = 0; i < chatMessage->usernameLength; i++) {
        bit_stream_write(stream, chatMessage->username[i], 8);
    }

    bit_stream_write(stream, (uint32_t)chatMessage->messageLength, 9);
    bit_stream_write_align(stream);
    for (size_t i = 0; i < chatMessage->messageLength; i++) {
        bit_stream_write(stream, chatMessage->message[i], 8);
    }
}

static void read_net_message_chat_message(BitStream *stream, NetMessage_ChatMessage *chatMessage)
{
    assert(stream);
    assert(chatMessage);

    chatMessage->usernameLength = bit_stream_read(stream, 5);
    assert(chatMessage->usernameLength <= USERNAME_LENGTH_MAX);
    bit_stream_read_align(stream);
    assert(stream->num_bits_read % 8 == 0);
    chatMessage->username = (char *)stream->buffer + stream->num_bits_read / 8;
    for (size_t i = 0; i < chatMessage->usernameLength; i++) {
        bit_stream_read(stream, 8);
    }

    chatMessage->messageLength = bit_stream_read(stream, 9);
    assert(chatMessage->messageLength <= CHAT_MESSAGE_LENGTH_MAX);
    bit_stream_read_align(stream);
    assert(stream->num_bits_read % 8 == 0);
    chatMessage->message = (char *)stream->buffer + stream->num_bits_read / 8;
    for (size_t i = 0; i < chatMessage->messageLength; i++) {
        bit_stream_read(stream, 8);
    }
}

size_t serialize_net_message(BitStream *stream, const NetMessage *message)
{
    bit_stream_write(stream, message->type, 2);
    switch (message->type) {
        case NetMessageType_Identify: {
            write_net_message_identify(stream, &message->data.identify);
            break;
        } case NetMessageType_Welcome: {
            write_net_message_welcome(stream, &message->data.welcome);
            break;
        } case NetMessageType_ChatMessage: {
            write_net_message_chat_message(stream, &message->data.chatMessage);
            break;
        }
    }
    if (stream->scratch_bits) {
        bit_stream_flush(stream);
    }
    return stream->total_bits / 8 + (stream->total_bits % 8 > 0);
}

void deserialize_net_message(BitStream *stream, NetMessage *message)
{
    // TODO: Validate that this is a valid enum value
    message->type = (NetMessageType)bit_stream_read(stream, 2);
    switch (message->type) {
        case NetMessageType_Identify: {
            read_net_message_identify(stream, &message->data.identify);
            break;
        } case NetMessageType_Welcome: {
            read_net_message_welcome(stream, &message->data.welcome);
            break;
        } case NetMessageType_ChatMessage: {
            read_net_message_chat_message(stream, &message->data.chatMessage);
            break;
        }
    }
}