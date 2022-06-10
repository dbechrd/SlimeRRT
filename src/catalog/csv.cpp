#include "csv.h"
#include "raylib/raylib.h"

int CSV::Value::toInt()
{
    int i = strtol((char *)data, 0, 10);
    return i;
}

unsigned int CSV::Value::toUint()
{
    unsigned int u = strtoul((char *)data, 0, 10);
    return u;
}

float CSV::Value::toFloat()
{
    float f = strtof((char *)data, 0);
    return f;
}

CSV::Error CSV::ReadFromFile(const char *filename)
{
    u32 fileBytes = 0;
    u8 *fileData = LoadFileData(filename, &fileBytes);

    CSV::Reader reader{};
    reader.ReadFromMemory(*this, fileData, fileBytes);
    printf("Reading CSV [%s]: %s\n", filename, reader.StatusMsg());
    if (reader.StatusCode() != CSV::SUCCESS) {
        printf(reader.StatusMsg());
    }
    return reader.StatusCode();
}

CSV::~CSV()
{
    if (fileData) {
        UnloadFileData(fileData);
    }
    free(data);
}

void CSV::Reader::ReadFromMemory(CSV &csv, u8 *data, u32 length)
{
    // Make copy of data that we can modify (we nil teriminate all the values for easier parsing)
    u8 *dataWithNil = (u8 *)malloc(length + 1);
    if (!dataWithNil) {
        return;
    }
    memcpy(dataWithNil, data, length);
    dataWithNil[length] = 0;

    Parse(csv, dataWithNil, length);
}

CSV::Error CSV::Reader::StatusCode()
{
    return err;
}

const char *CSV::Reader::StatusMsg()
{
    if (!msg[0]) {
        switch (err) {
            case Error::SUCCESS: {
                snprintf(msg, sizeof(msg), "SUCCESS: Successfully parsed %u rows with %u columns each.\n", row, columns);
                break;
            }
            case Error::ERR_COLUMN_MISTMATCH: {
                snprintf(msg, sizeof(msg), "ERR_COLUMN_MISMATCH: Row %u has %u columns, expected %u.\n", row, column, columns);
                break;
            }
            default: {
                snprintf(msg, sizeof(msg), "ERR_UNKNOWN: Unexpected error code %d.\n", err);
                break;
            }
        }
    }
    return msg;
}

void CSV::Reader::ParseCell(CSV &csv)
{
    CSV::Value &value = csv.values.emplace_back();
    value.data = csv.data + cursor;
    while (cursor < csv.length) {
        switch (csv.data[cursor]) {
            case ',': case '\r': case '\n': {
                return;
            }
            default: {
                value.length++;
                cursor++;
            }
        }
    }
}

void CSV::Reader::ParseRow(CSV &csv)
{
    column = 0;
    while (cursor < csv.length) {
        switch (csv.data[cursor]) {
            case '\r': case '\n': {
                return;
            }
            case ',': {
                csv.data[cursor] = 0;  // nil terminate cell
                cursor++;
                break;
            }
            default: {
                ParseCell(csv);
                column++;
                break;
            }
        }
    }
    if (!columns) {
        columns = column;
    } else if (column != columns) {
        err = ERR_COLUMN_MISTMATCH;
    }
}

void CSV::Reader::Parse(CSV &csv, u8 *data, u32 length)
{
    csv.data = data;
    csv.length = length;
    cursor = 0;
    row = 0;
    column = 0;
    while (cursor < length) {
        switch (data[cursor]) {
            case '\r': case '\n': {
                data[cursor] = 0;  // nil terminate cell
                cursor++;
                break;
            }
            default: {
                ParseRow(csv);
                if (err) return;
                row++;
                break;
            }
        }
    }
    csv.rows = row;
    csv.columns = columns;
}
