#include "bit_stream.h"
#include "helpers.h"
#include <cassert>

BitStream::BitStream(Mode mode, uint32_t *buffer, size_t bufferSize) : mode(mode), buffer(buffer), bufferBits(bufferSize * 8)
{
    assert(buffer);
    assert(bufferSize);
    //assert(bufferSize % 4 == 0);  // Must be aligned to word boundary.. why?
};

// Align read/write cursor to nearest byte boundary
void BitStream::Align()
{
    const uint8_t remainderBits = bitsProcessed % 8;
    if (remainderBits) {
        // NOTE: If assert fails in Reader mode, alignment padding corruption was detected; expected zero padding
        uint32_t value = 0;
        Process(value, 8 - remainderBits, 0, 0);
    }
    assert(scratchBits % 8 == 0);
    assert(bitsProcessed % 8 == 0);
}

// Return # of bytes read/written
size_t BitStream::BytesProcessed() const
{
    assert(bitsProcessed % 8 == 0);
    const size_t bytesProcessed = bitsProcessed / 8;
    return bytesProcessed;
}

// Read bits from scratch into word / Write bits form word to scratch
void BitStream::Process(uint32_t &word, uint8_t bits, uint32_t min, uint32_t max)
{
    assert(buffer);
    assert(bits);
    assert(bits <= 32);

    switch (mode) {
        case Mode::Reader: {
            // TODO: Handle malformed incoming packets in a way that doesn't crash the server
            assert(bitsProcessed + bits <= bufferBits);

            // Read next word into scratch if scratch needs more bits to service the read
            if (bits > scratchBits) {
                scratch |= (uint64_t)buffer[wordIndex] << scratchBits;
                wordIndex++;
                scratchBits += 32;
            }
            word = (((uint64_t)1 << bits) - 1) & scratch;
            scratch >>= bits;
            scratchBits -= bits;

            // TODO: Handle malformed incoming packets in a way that doesn't crash the server
            assert(word >= min);
            assert(word <= max);
            break;
        } case Mode::Writer: {
            assert(word >= min);
            assert(word <= max);

            assert((uint64_t)word < ((uint64_t)1 << bits));  // Ensure word doesn't get truncated
            uint64_t maskedWord = (((uint64_t)1 << bits) - 1) & word;
            scratch |= maskedWord << scratchBits;
            scratchBits += bits;

            if (scratchBits > 32) {
                Flush();
            }
            break;
        }
    }

    bitsProcessed += bits;
}

// Return pointer to currently location in buffer (for in-place string references)
const char *BitStream::BufferPtr() const
{
    assert(mode == Mode::Reader);  // NOTE: This would work in writer mode too.. but why would you use it?
    return (char *)buffer + BytesProcessed();
}

// Flush word from scratch to buffer
void BitStream::Flush()
{
    if (mode == Mode::Writer && scratchBits) {
        assert(wordIndex * 8 < bufferBits);
        buffer[wordIndex] = scratch & 0xFFFFFFFF;
        wordIndex++;
        scratch >>= 32;
        scratchBits -= MIN(scratchBits, 32);
    }
}

