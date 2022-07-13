#include "bit_stream.h"
#include "helpers.h"
#include <cassert>

BitStream::BitStream(Mode mode, void *buffer, size_t bufferSize) : mode(mode), buffer(buffer), bufferBits(bufferSize * 8)
{
    assert(buffer);
    assert(bufferSize);
    //assert(bufferSize % 4 == 0);  // Must be aligned to word boundary
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
    //assert(bitsProcessed % 8 == 0);
    const size_t bytesProcessed = bitsProcessed / 8 + (bitsProcessed % 8 > 0);
    return bytesProcessed;
}

// Read bits from scratch into word / Write bits form word to scratch
void BitStream::ProcessInternal(uint32_t &word, uint8_t bits)
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
                scratch |= (uint64_t)((uint32_t *)buffer)[wordIndex] << scratchBits;
                wordIndex++;
                scratchBits += 32;
            }
            word = (((uint64_t)1 << bits) - 1) & scratch;
            scratch >>= bits;
            scratchBits -= bits;
            break;
        } case Mode::Writer: {
            assert((uint64_t)word < ((uint64_t)1 << bits));  // Ensure there's enough bits to fit the whole value
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

void BitStream::Process(bool &value)
{
    uint32_t word = (uint32_t)value;
    ProcessInternal(word, 1);
    value = (bool)word;
}

void BitStream::Process(uint8_t &value, uint8_t bits, uint8_t min, uint8_t max)
{
    assert(bits <= sizeof(value) * 8);
    if (mode == Mode::Writer) {
        assert(value >= min);
        assert(value <= max);
    }

    uint32_t word = (uint32_t)value;
    ProcessInternal(word, bits);
    value = (uint8_t)word;

    if (mode == Mode::Reader) {
        assert(value >= min);
        assert(value <= max);
    }
}

void BitStream::Process(uint16_t &value, uint8_t bits, uint16_t min, uint16_t max)
{
    assert(bits <= sizeof(value) * 8);
    if (mode == Mode::Writer) {
        assert(value >= min);
        assert(value <= max);
    }

    uint32_t word = (uint32_t)value;
    ProcessInternal(word, bits);
    value = (uint16_t)word;

    if (mode == Mode::Reader) {
        assert(value >= min);
        assert(value <= max);
    }
}

void BitStream::Process(uint32_t &value, uint8_t bits, uint32_t min, uint32_t max)
{
    assert(bits <= sizeof(value) * 8);
    if (mode == Mode::Writer) {
        assert(value >= min);
        assert(value <= max);
    }

    ProcessInternal(value, bits);

    if (mode == Mode::Reader) {
        assert(value >= min);
        assert(value <= max);
    }
}

void BitStream::Process(int8_t &value, uint8_t bits, int8_t min, int8_t max)
{
    assert(bits <= sizeof(value) * 8);
    if (mode == Mode::Writer) {
        assert(value >= min);
        assert(value <= max);
    }

    uint32_t word = (uint8_t)value;
    ProcessInternal(word, bits);
    value = word;

    if (mode == Mode::Reader) {
        assert(value >= min);
        assert(value <= max);
    }
}

void BitStream::Process(int16_t &value, uint8_t bits, int16_t min, int16_t max)
{
    assert(bits <= sizeof(value) * 8);
    if (mode == Mode::Writer) {
        assert(value >= min);
        assert(value <= max);
    }

    uint32_t word = (uint16_t)value;
    ProcessInternal(word, bits);
    value = word;

    if (mode == Mode::Reader) {
        assert(value >= min);
        assert(value <= max);
    }
}

void BitStream::Process(int32_t &value, uint8_t bits, int32_t min, int32_t max)
{
    assert(bits <= sizeof(value) * 8);
    if (mode == Mode::Writer) {
        assert(value >= min);
        assert(value <= max);
    }

    uint32_t word = value;
    ProcessInternal(word, bits);
    value = word;

    if (mode == Mode::Reader) {
        assert(value >= min);
        assert(value <= max);
    }
}

void BitStream::Process(float &value, uint8_t bits, float min, float max)
{
    assert(bits <= sizeof(value) * 8);
    if (mode == Mode::Writer) {
        assert(value >= min);
        assert(value <= max);
    }

    uint32_t word = *(uint32_t *)&value;
    ProcessInternal(word, bits);
    value = *(float *)&word;

    if (mode == Mode::Reader) {
        assert(value >= min);
        assert(value <= max);
    }
}

void BitStream::Process(double &value, uint8_t bits, double min, double max)
{
    //assert(bits <= sizeof(value) * 8);
    assert(bits == 64);
    if (mode == Mode::Writer) {
        assert(value >= min);
        assert(value <= max);
    }

    assert(sizeof(uint32_t) * 2 == sizeof(double));
    uint32_t word0 = ((uint32_t *)&value)[0];
    uint32_t word1 = ((uint32_t *)&value)[1];
    ProcessInternal(word0, 32);
    ProcessInternal(word1, 32);
    ((uint32_t *)&value)[0] = word0;
    ((uint32_t *)&value)[1] = word1;

    if (mode == Mode::Reader) {
        assert(value >= min);
        assert(value <= max);
    }
}

void BitStream::ProcessChar(char &value)
{
    if (mode == Mode::Writer) {
        assert(value >= STRING_ASCII_MIN);
        assert(value <= STRING_ASCII_MAX);
    }

    uint32_t word = (uint32_t)value;
    ProcessInternal(word, 8);
    value = (char)word;

    if (mode == Mode::Reader) {
        assert(value >= STRING_ASCII_MIN);
        assert(value <= STRING_ASCII_MAX);
    }
}

// Flush word from scratch to buffer
void BitStream::Flush()
{
    if (mode == Mode::Writer && scratchBits) {
        assert((wordIndex + 1) * 32 < bufferBits);
        ((uint32_t *)buffer)[wordIndex] = scratch & 0xFFFFFFFF;

#if _DEBUG && 0
        if (debugPrint) {
            FILE *log = fopen(mode == Mode::Reader ? "recv.log" : "send.log", "w");
#define BIT(x, n) (x >> (31 - (n)) & 1)
            int bit = 0;
            uint32_t word = *((uint32_t *)buffer + wordIndex);
            fprintf(log,
                "%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d ",
                BIT(word, bit++), BIT(word, bit++), BIT(word, bit++), BIT(word, bit++),
                BIT(word, bit++), BIT(word, bit++), BIT(word, bit++), BIT(word, bit++),
                BIT(word, bit++), BIT(word, bit++), BIT(word, bit++), BIT(word, bit++),
                BIT(word, bit++), BIT(word, bit++), BIT(word, bit++), BIT(word, bit++),
                BIT(word, bit++), BIT(word, bit++), BIT(word, bit++), BIT(word, bit++),
                BIT(word, bit++), BIT(word, bit++), BIT(word, bit++), BIT(word, bit++),
                BIT(word, bit++), BIT(word, bit++), BIT(word, bit++), BIT(word, bit++),
                BIT(word, bit++), BIT(word, bit++), BIT(word, bit++), BIT(word, bit++)
            );
            fclose(log);
#undef BIT
        }
#endif

        wordIndex++;
        scratch >>= 32;
        scratchBits -= MIN(scratchBits, 32);
    }
}

