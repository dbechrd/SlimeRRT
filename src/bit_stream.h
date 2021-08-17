#pragma once
#include <cstdint>

// https://gafferongames.com/post/reading_and_writing_packets/
// https://gafferongames.com/post/serialization_strategies/

struct BitStream {
    // For read & write
    uint64_t    m_scratch     = 0;  // temporary scratch buffer to hold bits until we fill a 32-bit word
    size_t      m_scratchBits = 0;  // number of bits in scratch that are in use (i.e. not yet written / read)
    size_t      m_wordIndex   = 0;  // index of next word in `buffer`
    uint32_t *  m_buffer      = 0;  // buffer we're writing to / reading from
    size_t      m_bufferBits  = 0;  // size of packet in bytes * 8

    BitStream(uint32_t *buffer, size_t bufferLength);
};

struct BitStreamReader : public BitStream {
    BitStreamReader(uint32_t *buffer, size_t bufferLength) : BitStream(buffer, bufferLength) {}

    // Read # of bits from scratch into word
    uint32_t Read(uint8_t bits);

    // Align read cursor to nearest byte boundary
    void Align();

    // Return # of bytes read
    size_t BytesRead() const;

    // Return pointer to currently location in buffer (for in-place string references)
    const char *BufferPtr() const;

private:
    size_t m_numBitsRead = 0; // number of bits we've read so far
};

struct BitStreamWriter : public BitStream {
    BitStreamWriter(uint32_t *buffer, size_t bufferLength) : BitStream(buffer, bufferLength) {}

    // Write # of bits from word into scratch
    void Write(uint32_t word, uint8_t bits);

    // Align write cursor to nearest byte boundary
    void Align();

    // Flush word from scratch to buffer
    void Flush();

    // Return # of bytes written
    size_t BytesWritten() const;

private:
    size_t m_numBitsWritten = 0; // number of bits we've written so far
};