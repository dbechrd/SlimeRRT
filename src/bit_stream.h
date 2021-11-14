#pragma once
#include <cstdint>

// https://gafferongames.com/post/reading_and_writing_packets/
// https://gafferongames.com/post/serialization_strategies/

struct BitStream {
    enum class Mode {
        Reader,
        Writer
    };

    BitStream(Mode mode, void *buffer, size_t bufferLength);

    bool Reading() const { return mode == Mode::Reader; };
    bool Writing() const { return mode == Mode::Writer; };

    // Align read/write cursor to nearest byte boundary
    void Align();

    // Return # of bytes read/written
    size_t BytesProcessed() const;

    // Read bits from scratch into word / Write bits form word to scratch
    void Process(uint32_t &word, uint8_t bits, uint32_t min, uint32_t max);

    // Flush word from scratch to buffer
    void Flush();

private:
    Mode mode;

    // For read & write
    uint64_t  scratch       {};  // temporary scratch buffer to hold bits until we fill a 32-bit word
    size_t    scratchBits   {};  // number of bits in scratch that are in use (i.e. not yet written / read)
    size_t    wordIndex     {};  // index of next word in `buffer`
    void     *buffer        {};  // buffer we're writing to / reading from
    size_t    bufferBits    {};  // size of packet in bytes * 8
    size_t    bitsProcessed {};  // number of bits we've read/written so far
};