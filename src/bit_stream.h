#pragma once
#include <cstdint>
#include <cfloat>

// https://gafferongames.com/post/reading_and_writing_packets/
// https://gafferongames.com/post/serialization_strategies/

struct BitStream {
    enum class Mode {
        Reader,
        Writer
    };

    BitStream(Mode mode, void *buffer, size_t bufferLength);

    bool Reading() const { return mode == Mode::Reader; }
    bool Writing() const { return mode == Mode::Writer; }

    // Align read/write cursor to nearest byte boundary
    void Align();

    // Return # of bytes read/written
    size_t BytesProcessed() const;

    // Read bits from scratch into word / Write bits form word to scratch
    void Process (bool     &value);
    void Process (uint8_t  &value, uint8_t bits =  8, uint8_t  min = 0        , uint8_t  max = UINT8_MAX );
    void Process (uint16_t &value, uint8_t bits = 16, uint16_t min = 0        , uint16_t max = UINT16_MAX);
    void Process (uint32_t &value, uint8_t bits = 32, uint32_t min = 0        , uint32_t max = UINT32_MAX);
    void Process (int8_t   &value, uint8_t bits =  8, int8_t   min = INT8_MIN , int8_t   max = INT8_MAX  );
    void Process (int16_t  &value, uint8_t bits = 16, int16_t  min = INT16_MIN, int16_t  max = INT16_MAX );
    void Process (int32_t  &value, uint8_t bits = 32, int32_t  min = INT32_MIN, int32_t  max = INT32_MAX );
    void Process (float    &value, uint8_t bits = 32, float    min = -FLT_MAX , float    max = FLT_MAX   );
    void Process (double   &value, uint8_t bits = 64, double   min = -DBL_MAX , double   max = DBL_MAX   );

    void ProcessChar   (char &chr);

    // Flush word from scratch to buffer
    void Flush();

    bool debugPrint {};

private:
    Mode mode;

    // For read & write
    uint64_t  scratch       {};  // temporary scratch buffer to hold bits until we fill a 32-bit word
    size_t    scratchBits   {};  // number of bits in scratch that are in use (i.e. not yet written / read)
    size_t    wordIndex     {};  // index of next word in `buffer`
    void     *buffer        {};  // buffer we're writing to / reading from
    size_t    bufferBits    {};  // size of packet in bytes * 8
    size_t    bitsProcessed {};  // number of bits we've read/written so far

    void ProcessInternal(uint32_t &word, uint8_t bits);
};
