#pragma once
#include <cstdint>

// https://gafferongames.com/post/reading_and_writing_packets/
struct BitStream {
    // For read & write
    uint64_t scratch;   // temporary scratch buffer to hold bits until we fill a 32-bit word
    size_t scratch_bits;   // number of bits in scratch that are in use (i.e. not yet written / read)
    size_t word_index;     // index of next word in `buffer`
    uint32_t *buffer;   // buffer we're writing to / reading from
    // TODO: Use this for reading too?
    size_t bufferSize;  // size of buffer writing to

    // For read only
    size_t total_bits;     // size of packet in bytes * 8
    size_t num_bits_read;  // number of bits we've read so far
};

void bit_stream_writer_init(BitStream *stream, uint32_t *buffer, size_t bufferSize);
void bit_stream_reader_init(BitStream *stream, uint32_t *buffer, size_t bytes);
size_t serialize_net_message(BitStream *stream, const struct NetMessage *message);
void deserialize_net_message(BitStream *stream, struct NetMessage *message);
