#include "tests.h"
#include "../src/bit_stream.h"
#include <cassert>

void bit_stream_write(BitStream &stream, uint32_t value, uint8_t bits)
{
    stream.Process(value, bits, 0, UINT32_MAX);
}

uint32_t bit_stream_read(BitStream &stream, uint8_t bits)
{
    uint32_t value = 0;
    stream.Process(value, bits, 0, UINT32_MAX);
    return value;
}

void bit_stream_test()
{
    uint8_t buffer[20]{};

    BitStream writer{ BitStream::Mode::Writer, buffer, sizeof(buffer) };
    bit_stream_write(writer, 0b0, 1);
    bit_stream_write(writer, 0b11, 2);
    bit_stream_write(writer, 0b000, 3);
    bit_stream_write(writer, 0b1111, 4);
    bit_stream_write(writer, 0b00000, 5);
    bit_stream_write(writer, 0b111111, 6);
    bit_stream_write(writer, 0b0000000, 7);
    bit_stream_write(writer, 0b11111111, 8);
    uint64_t seedIn = 12345678900987654321;
    writer.Process(seedIn);
    writer.Flush();

    BitStream reader{ BitStream::Mode::Reader, buffer, sizeof(buffer) };
    assert(bit_stream_read(reader, 1) == 0b0);
    assert(bit_stream_read(reader, 2) == 0b11);
    assert(bit_stream_read(reader, 3) == 0b000);
    assert(bit_stream_read(reader, 4) == 0b1111);
    assert(bit_stream_read(reader, 5) == 0b00000);
    assert(bit_stream_read(reader, 6) == 0b111111);
    assert(bit_stream_read(reader, 7) == 0b0000000);
    assert(bit_stream_read(reader, 8) == 0b11111111);
    uint64_t seedOut = 0;
    reader.Process(seedOut);
    assert(seedOut == seedIn);
}