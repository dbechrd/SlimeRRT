#include "tests.h"
#include "../src/bit_stream.h"
#include <cassert>

void bit_stream_test()
{
    const size_t size = 64;
    uint32_t buffer[size]{};

    BitStreamWriter writer{ buffer, size };
    writer.Write(0b0, 1);
    writer.Write(0b11, 2);
    writer.Write(0b000, 3);
    writer.Write(0b1111, 4);
    writer.Write(0b00000, 5);
    writer.Write(0b111111, 6);
    writer.Write(0b0000000, 7);
    writer.Write(0b11111111, 8);
    writer.Flush();

    BitStreamReader reader{ buffer, size };
    assert(reader.Read(1) == 0b0       );
    assert(reader.Read(2) == 0b11      );
    assert(reader.Read(3) == 0b000     );
    assert(reader.Read(4) == 0b1111    );
    assert(reader.Read(5) == 0b00000   );
    assert(reader.Read(6) == 0b111111  );
    assert(reader.Read(7) == 0b0000000 );
    assert(reader.Read(8) == 0b11111111);
}