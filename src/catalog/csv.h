#pragma once

#include "dlb_types.h"
#include <vector>

struct CSV {
    enum Error {
        SUCCESS = 0,
        ERR_COLUMN_MISTMATCH = -1,
    };

    struct Value {
        u8  *data  {};
        u32 length {};

        int          toInt   ();
        unsigned int toUint  ();
        float        toFloat ();
    };

    u8  *data     {};  // Raw bytes of CSV data
    u8  *fileData {};  // Same as data, but only if read in from a file via Raylib
    u32 length    {};  // Length of data in bytes
    u32 columns   {};  // Number of columns in CSV
    u32 rows      {};  // Number of columns expected per row (based on first row's column count)
    std::vector<Value> values{};  // Contiguous list of cells

    Error ReadFromFile(const char *filename);
    ~CSV();

    class Reader {
    public:
        void ReadFromMemory(CSV &csv, u8 *data, u32 length);

        Error StatusCode();
        const char *StatusMsg();
    private:
        const char *LOG_SRC = "CSV::Reader";

        u32   cursor    {};  // Current byte being read (as offset into data array)
        u32   columns   {};  // Number of columns expected per row (based on first row's column count)
        u32   row       {};  // Index of row being parsed (for generating error messages)
        u32   column    {};  // Index of column being parsed (unused for now)
        Error err       {};  // Error code (SUCCESS if no error)
        char  msg[128]  {};  // Status message (error msg if fails, otherwise a success msg w/ statistics)

        void ParseCell(CSV &csv);
        void ParseRow(CSV &csv);
        void Parse(CSV &csv, u8 *data, u32 length);
    };

private:
    const char *LOG_SRC = "CSV";
};
