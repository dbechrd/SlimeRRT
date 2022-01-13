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
                scratch |= (uint64_t)((uint32_t *)buffer)[wordIndex] << scratchBits;
                wordIndex++;
                scratchBits += 32;
            }
            word = (((uint64_t)1 << bits) - 1) & scratch;
            scratch >>= bits;
            scratchBits -= bits;

            assert(word >= min);
            assert(word <= max);
            break;
        } case Mode::Writer: {
            assert(word >= min);
            assert(word <= max);

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

void BitStream::ProcessBool(bool &flag)
{
    uint32_t word = (uint32_t)flag;
    Process(word, 1, 0, 1);
    flag = (bool)word;
}

void BitStream::ProcessChar(char &chr)
{
    uint32_t word = (uint32_t)chr;
    Process(word, 8, STRING_ASCII_MIN, STRING_ASCII_MAX);
    chr = (char)word;
}

void BitStream::ProcessFloat(float &flt)
{
    uint32_t word = *(uint32_t *)&flt;
    Process(word, 32, 0, UINT32_MAX);
    flt = *(float *)&word;
}

void BitStream::ProcessDouble(double &dbl)
{
    assert(sizeof(uint32_t) * 2 == sizeof(double));
    uint32_t word0 = ((uint32_t *)&dbl)[0];
    uint32_t word1 = ((uint32_t *)&dbl)[1];
    Process(word0, 32, 0, UINT32_MAX);
    Process(word1, 32, 0, UINT32_MAX);
    ((uint32_t *)&dbl)[0] = word0;
    ((uint32_t *)&dbl)[1] = word1;
}

// Flush word from scratch to buffer
void BitStream::Flush()
{
    if (mode == Mode::Writer && scratchBits) {
        assert((wordIndex + 1) * 32 < bufferBits);
        ((uint32_t *)buffer)[wordIndex] = scratch & 0xFFFFFFFF;

#if _DEBUG && 0
        if (debugPrint) {
            FILE *log = fopen(mode == Mode::Reader ? "recv.txt" : "send.txt", "w");
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

