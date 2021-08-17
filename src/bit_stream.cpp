#include "bit_stream.h"
#include "helpers.h"
#include <cassert>

BitStream::BitStream(uint32_t *buffer, size_t bufferSize) : m_buffer(buffer), m_bufferBits(bufferSize * 8)
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
    assert(m_numBitsRead + bits <= m_bufferBits);

    // Read next word into scratch if scratch needs more bits to service the read
    if (bits > m_scratchBits) {
        m_scratch |= (uint64_t)m_buffer[m_wordIndex] << m_scratchBits;
        m_wordIndex++;
        m_scratchBits += 32;
    }

    uint32_t word = (((uint64_t)1 << bits) - 1) & m_scratch;
    m_scratch >>= bits;
    m_scratchBits -= bits;
    m_numBitsRead += bits;
    return word;
}

void BitStreamReader::Align()
{
    const uint8_t remainderBits = m_numBitsRead % 8;
    if (remainderBits) {
        uint32_t value = Read(8 - remainderBits);
        assert(m_scratchBits % 8 == 0);
        if (value != 0) {
            assert(!"Alignment padding corruption detected; expected zero padding");
            //return false;
        }
    }
    assert(m_numBitsRead % 8 == 0);
}

// Return # of bytes read
size_t BitStreamReader::BytesRead() const
{
    return m_numBitsRead / 8;
}

// Return pointer to currently location in buffer (for in-place string references)
const char *BitStreamReader::BufferPtr() const
{
    return (char *)m_buffer + BytesRead();
}

// Write # of bits from word into scratch
void BitStreamWriter::Write(uint32_t word, uint8_t bits)
{
    assert(m_buffer);
    assert(bits);

    uint64_t maskedWord = (((uint64_t)1 << bits) - 1) & word;
    m_scratch |= maskedWord << m_scratchBits;
    m_scratchBits += bits;
    m_numBitsWritten += bits;

    if (m_scratchBits > 32) {
        Flush();
    }
}

// https://gafferongames.com/post/serialization_strategies/
void BitStreamWriter::Align()
{
    const uint8_t remainderBits = m_numBitsWritten % 8;
    if (remainderBits) {
        uint32_t zero = 0;
        Write(zero, 8 - remainderBits);
        assert(m_scratchBits % 8 == 0);
    }
    assert(m_numBitsWritten % 8 == 0);
}

// Flush word from scratch to buffer
void BitStreamWriter::Flush()
{
    if (m_scratchBits) {
        assert(m_wordIndex * 8 < m_bufferBits);
        m_buffer[m_wordIndex] = m_scratch & 0xFFFFFFFF;
        m_wordIndex++;
        m_scratch >>= 32;
        m_scratchBits -= MIN(m_scratchBits, 32);
    }
}

size_t BitStreamWriter::BytesWritten() const
{
    assert(m_numBitsWritten % 8 == 0);
    const size_t bytesWritten = m_numBitsWritten / 8;
    return bytesWritten;
}