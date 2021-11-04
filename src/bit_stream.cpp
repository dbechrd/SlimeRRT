#include "bit_stream.h"
#include "helpers.h"
#include <cassert>

BitStream::BitStream(uint32_t *buffer, size_t bufferSize) : buffer(buffer), bufferBits(bufferSize * 8)
{
    assert(buffer);
    assert(bufferSize);
    //assert(bufferSize % 4 == 0);  // Must be aligned to word boundary.. why?
};

// Read # of bits from scratch into word
uint32_t BitStreamReader::Read(uint8_t bits)
{
    assert(bits);
    assert(bits <= 32);
    assert(numBitsRead + bits <= bufferBits);

    // Read next word into scratch if scratch needs more bits to service the read
    if (bits > scratchBits) {
        scratch |= (uint64_t)buffer[wordIndex] << scratchBits;
        wordIndex++;
        scratchBits += 32;
    }

    uint32_t word = (((uint64_t)1 << bits) - 1) & scratch;
    scratch >>= bits;
    scratchBits -= bits;
    numBitsRead += bits;
    return word;
}

void BitStreamReader::Align()
{
    const uint8_t remainderBits = numBitsRead % 8;
    if (remainderBits) {
        uint32_t value = Read(8 - remainderBits);
        assert(scratchBits % 8 == 0);
        if (value != 0) {
            assert(!"Alignment padding corruption detected; expected zero padding");
            //return false;
        }
    }
    assert(numBitsRead % 8 == 0);
}

// Return # of bytes read
size_t BitStreamReader::BytesRead() const
{
    return numBitsRead / 8;
}

// Return pointer to currently location in buffer (for in-place string references)
const char *BitStreamReader::BufferPtr() const
{
    return (char *)buffer + BytesRead();
}

// Write # of bits from word into scratch
void BitStreamWriter::Write(uint32_t word, uint8_t bits)
{
    assert(buffer);
    assert(bits);
    assert((uint64_t)word < ((uint64_t)1 << bits));  // Ensure word doesn't get truncated

    uint64_t maskedWord = (((uint64_t)1 << bits) - 1) & word;
    scratch |= maskedWord << scratchBits;
    scratchBits += bits;
    numBitsWritten += bits;

    if (scratchBits > 32) {
        Flush();
    }
}

// https://gafferongames.com/post/serialization_strategies/
void BitStreamWriter::Align()
{
    const uint8_t remainderBits = numBitsWritten % 8;
    if (remainderBits) {
        uint32_t zero = 0;
        Write(zero, 8 - remainderBits);
        assert(scratchBits % 8 == 0);
    }
    assert(numBitsWritten % 8 == 0);
}

// Flush word from scratch to buffer
void BitStreamWriter::Flush()
{
    if (scratchBits) {
        assert(wordIndex * 8 < bufferBits);
        buffer[wordIndex] = scratch & 0xFFFFFFFF;
        wordIndex++;
        scratch >>= 32;
        scratchBits -= MIN(scratchBits, 32);
    }
}

size_t BitStreamWriter::BytesWritten() const
{
    assert(numBitsWritten % 8 == 0);
    const size_t bytesWritten = numBitsWritten / 8;
    return bytesWritten;
}