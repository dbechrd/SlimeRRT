#pragma once
#include "dlb_types.h"
#include <stdio.h>

typedef enum ta_schema_field_type {
    // Compound types
    TYP_NULL,   // must be 0
    TYP_VECTOR2,
    TYP_VECTOR3,
    TYP_VECTOR4,
    TYP_NETMESSAGE,
    TYP_NETMESSAGE_IDENTIFY,
    TYP_NETMESSAGE_WELCOME,
    TYP_NETMESSAGE_CHATMESSAGE,
    TYP_COUNT,  // must be last

    // Atomic types
    ATOM_BOOL,
    ATOM_UINT8,
    ATOM_INT,
    ATOM_UINT,
    ATOM_FLOAT,
    ATOM_STRING,
    ATOM_ENUM,
} ta_schema_field_type;

typedef const char *(enum_to_str)(int);

typedef struct ta_schema_field {
    ta_schema_field_type type;      // field type
    const char   *name;             // field name
    size_t       offset;            // field offset in the struct the parent schema represents
    size_t       size;              // field size (bytes) in the struct
    size_t       netBits;           // field size (bits) when serialized for network transport
    size_t       array_len;         // 0 = not array, 1 = vector, >1 = fixed array size
    enum_to_str  *enum_converter;   // enum to string converter (for enum fields)
    bool         is_union_type;     // true if this field is a union
    bool         in_union;          // true if this field is in a union
    int          union_type;        // enum representing the type for which this union field is associated (used to
                                    // validate union fields are only present when they match the field type)
} ta_schema_field;

typedef void (schema_init)(void *ptr);
typedef void (schema_free)(void *ptr);

#define SCHEMA_MAX_FIELDS 8

typedef struct ta_schema {
    ta_schema_field_type type;  // field type of the top-most node representing this schema
    const char      *name;      // field name of the top-most node (i.e. "schema name")
    size_t          size;       // size in bytes of the schema type and all of its children
    schema_init     *init;      // constructor
    schema_free     *free;      // destructor
    size_t          fieldCount; // number of fields in this type (fields array)
    ta_schema_field fields[SCHEMA_MAX_FIELDS];    // array of fields in this schema type
} ta_schema;

extern ta_schema tg_schemas[TYP_COUNT];

void ta_schema_register         (void);
void ta_schema_field_type_str   (ta_schema_field_type type, const char **str);
//void ta_schema_field_find       (ta_schema_field_type type, const char *name, ta_schema_field **field);
void ta_schema_print            (FILE *f, ta_schema_field_type type, u8 *ptr, int level, int in_array);
void ta_schema_print_bytes      (FILE *f, ta_schema_field_type type, u8 *ptr, int level, int in_array);


//----------------------------------------------------------------------------------------------------------------
// TODO: Move to bit_stream.h
//----------------------------------------------------------------------------------------------------------------
// https://gafferongames.com/post/reading_and_writing_packets/
typedef struct BitStream {
    // For read & write
    uint64_t scratch;   // temporary scratch buffer to hold bits until we fill a 32-bit word
    int scratch_bits;   // number of bits in scratch that are in use (i.e. not yet written / read)
    int word_index;     // index of next word in `buffer`
    uint32_t *buffer;   // buffer we're writing to / reading from

    // For read only
    int total_bits;     // size of packet in bytes * 8
    int num_bits_read;  // number of bits we've read so far
} BitStream;

void ta_schema_write_bit_stream(BitStream *stream, ta_schema_field_type type, u8 *ptr, int level, int in_array);
void ta_schema_read_bit_stream (BitStream *stream, ta_schema_field_type type, u8 *ptr, int level, int in_array);

void ta_schema_write_net_message(BitStream *stream, const struct NetMessage *message);
void ta_schema_read_net_message(BitStream *stream, struct NetMessage *message);
//----------------------------------------------------------------------------------------------------------------