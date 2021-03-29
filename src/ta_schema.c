#include "ta_schema.h"
#include "packet.h"
#include "raygui.h"
#include "dlb_types.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

ta_schema tg_schemas[TYP_COUNT];

void ta_schema_field_type_str(ta_schema_field_type type, const char **str)
{
    switch (type) {
        // Compound types
        case TYP_NULL:                   *str = "TYP_NULL";                     break;
        case TYP_VECTOR2:                *str = "TYP_VECTOR2";                  break;
        case TYP_VECTOR3:                *str = "TYP_VECTOR3";                  break;
        case TYP_VECTOR4:                *str = "TYP_VECTOR4";                  break;
        case TYP_NETMESSAGE_IDENTIFY:    *str = "TYP_NETMESSAGE_IDENTIFY";      break;
        case TYP_NETMESSAGE_WELCOME:     *str = "TYP_NETMESSAGE_WELCOME";       break;
        case TYP_NETMESSAGE_CHATMESSAGE: *str = "TYP_NETMESSAGE_CHATMESSAGE";   break;

        // Atomic types
        case ATOM_BOOL:     *str = "ATOM_BOOL";   break;
        case ATOM_UINT8:    *str = "ATOM_UINT8";  break;
        case ATOM_INT:      *str = "ATOM_INT";    break;
        case ATOM_UINT:     *str = "ATOM_UINT";   break;
        case ATOM_FLOAT:    *str = "ATOM_FLOAT";  break;
        case ATOM_STRING:   *str = "ATOM_STRING"; break;
        case ATOM_ENUM:     *str = "ATOM_ENUM";   break;
        default: assert(0); *str = "TYP_???";     break;
    }
}

#define TYPE_START(_type, field_type, _init, _free) \
    schema = &tg_schemas[field_type]; \
    schema->type = field_type; \
    schema->name = STRING(_type); \
    schema->size = sizeof(_type); \
    schema->init = _init; \
    schema->free = _free;

#define TYPE_FIELD(type, field, field_type) \
    type_field_add(schema, field_type, STRING(field), \
    OFFSETOF(type, field), FIELD_SIZEOF(type, field), 0, \
    0, 0, false, false, 0)

#define TYPE_FIELD_NET(type, field, field_type) \
    type_field_add(schema, field_type, STRING(field), \
    OFFSETOF(type, field), FIELD_SIZEOF(type, field), FIELD_SIZEOF(type, field) * CHAR_BIT, \
    0, 0, false, false, 0)

#define TYPE_FIELD_NET_BITS(type, field, field_type, net_bits) \
    type_field_add(schema, field_type, STRING(field), \
    OFFSETOF(type, field), FIELD_SIZEOF(type, field), net_bits, \
    0, 0, false, false, 0)

#define TYPE_ENUM(type, field, field_type, converter) \
    type_field_add(schema, field_type, STRING(field), \
    OFFSETOF(type, field), FIELD_SIZEOF(type, field), 0, \
    0, converter, false, false, 0)

#define TYPE_ENUM_NET(type, field, field_type, converter) \
    type_field_add(schema, field_type, STRING(field), \
    OFFSETOF(type, field), FIELD_SIZEOF(type, field), FIELD_SIZEOF(type, field) * CHAR_BIT, \
    0, converter, false, false, 0)

#define TYPE_ENUM_NET_BITS(type, field, field_type, converter, net_bits) \
    type_field_add(schema, field_type, STRING(field), \
    OFFSETOF(type, field), FIELD_SIZEOF(type, field), net_bits, \
    0, converter, false, false, 0)

#define TYPE_ARRAY(type, field, field_type, size) \
    type_field_add(schema, field_type, STRING(field), \
    OFFSETOF(type, field), FIELD_SIZEOF_ARRAY(type, field), 0, \
    size, 0, false, false, 0)

#define TYPE_VECTOR(type, field, field_type) \
    type_field_add(schema, field_type, STRING(field), \
    OFFSETOF(type, field), FIELD_SIZEOF_ARRAY(type, field), 0, \
    1, 0, false, false, 0)

#define TYPE_UNION_TYPE(type, field, field_type, converter) \
    type_field_add(schema, field_type, STRING(field), \
    OFFSETOF(type, field), FIELD_SIZEOF(type, field), 0, \
    0, converter, true, false, 0)

#define TYPE_UNION_TYPE_NET(type, field, field_type, converter) \
    type_field_add(schema, field_type, STRING(field), \
    OFFSETOF(type, field), FIELD_SIZEOF(type, field), FIELD_SIZEOF(type, field) * CHAR_BIT, \
    0, converter, true, false, 0)

#define TYPE_UNION_TYPE_NET_BITS(type, field, field_type, converter, net_bits) \
    type_field_add(schema, field_type, STRING(field), \
    OFFSETOF(type, field), FIELD_SIZEOF(type, field), net_bits, \
    0, converter, true, false, 0)

#define TYPE_UNION_FIELD(type, field, field_type, union_name, union_type) \
    type_field_add(schema, field_type, STRING(field), \
    OFFSETOF(type, union_name.field), FIELD_SIZEOF(type, union_name.field), 0, \
    0, 0, false, true, union_type)

#define TYPE_UNION_FIELD_NET(type, field, field_type, union_name, union_type) \
    type_field_add(schema, field_type, STRING(field), \
    OFFSETOF(type, union_name.field), FIELD_SIZEOF(type, union_name.field), FIELD_SIZEOF(type, union_name.field) * CHAR_BIT, \
    0, 0, false, true, union_type)

#define TYPE_UNION_FIELD_NET_BITS(type, field, field_type, union_name, union_type, net_bits) \
    type_field_add(schema, field_type, STRING(field), \
    OFFSETOF(type, union_name.field), FIELD_SIZEOF(type, union_name.field), net_bits, \
    0, 0, false, true, union_type)

#define TYPE_UNION_ARRAY(type, field, field_type, size, union_name, union_type) \
    type_field_add(schema, field_type, STRING(field), \
    OFFSETOF(type, union_name.field), FIELD_SIZEOF_ARRAY(type, union_name.field), 0, \
    size, 0, false, true, union_type)

#define TYPE_UNION_VECTOR(type, field, field_type, union_name, union_type) \
    type_field_add(schema, field_type, STRING(field), \
    OFFSETOF(type, union_name.field), FIELD_SIZEOF_ARRAY(type, union_name.field), 0, \
    1, 0, false, true, union_type)

static void type_field_add(
    ta_schema *schema, ta_schema_field_type type, const char *name,
    size_t offset, size_t size, size_t netBits,
    size_t array_len, enum_to_str *enum_converter, bool is_union_type, bool in_union, int union_type)
{
    assert(size);
    // Note: We could pad things to be *bigger* on the network.. but that seems stupid.
    // Currently, `schema_atom_print_bytes` would not handle that correctly.
    assert(netBits <= size * CHAR_BIT);

    assert(schema->fieldCount < SCHEMA_MAX_FIELDS);
    ta_schema_field *field = &schema->fields[schema->fieldCount];
    schema->fieldCount++;

    field->type = type;
    field->name = name;
    field->offset = offset;
    field->size = size;
    field->netBits = netBits;
    field->array_len = array_len;
    field->enum_converter = enum_converter;
    field->is_union_type = is_union_type;
    field->in_union = in_union;
    field->union_type = union_type;
}

void ta_schema_register(void)
{
    ta_schema *schema = 0;

    TYPE_START(Vector2, TYP_VECTOR2, 0, 0);
    TYPE_FIELD(Vector2, x, ATOM_FLOAT);
    TYPE_FIELD(Vector2, y, ATOM_FLOAT);

    TYPE_START(Vector3, TYP_VECTOR3, 0, 0);
    TYPE_FIELD(Vector3, x, ATOM_FLOAT);
    TYPE_FIELD(Vector3, y, ATOM_FLOAT);
    TYPE_FIELD(Vector3, z, ATOM_FLOAT);

    TYPE_START(Vector4, TYP_VECTOR4, 0, 0);
    TYPE_FIELD(Vector4, x, ATOM_FLOAT);
    TYPE_FIELD(Vector4, y, ATOM_FLOAT);
    TYPE_FIELD(Vector4, z, ATOM_FLOAT);
    TYPE_FIELD(Vector4, w, ATOM_FLOAT);

    TYPE_START              (NetMessage, TYP_NETMESSAGE, 0, 0);
    TYPE_UNION_TYPE_NET_BITS(NetMessage, type, ATOM_UINT, 0, 2);
    TYPE_UNION_FIELD        (NetMessage, identify,    TYP_NETMESSAGE_IDENTIFY,    data, NetMessageType_Identify);
    TYPE_UNION_FIELD        (NetMessage, welcome,     TYP_NETMESSAGE_WELCOME,     data, NetMessageType_Welcome);
    TYPE_UNION_FIELD        (NetMessage, chatMessage, TYP_NETMESSAGE_CHATMESSAGE, data, NetMessageType_ChatMessage);

    TYPE_START          (NetMessage_Identify, TYP_NETMESSAGE_IDENTIFY, 0, 0);
    TYPE_FIELD_NET_BITS (NetMessage_Identify, usernameLength, ATOM_UINT, 5);
    TYPE_FIELD_NET      (NetMessage_Identify, username,       ATOM_STRING);

    TYPE_START          (NetMessage_Welcome, TYP_NETMESSAGE_WELCOME, 0, 0);

    TYPE_START          (NetMessage_ChatMessage, TYP_NETMESSAGE_CHATMESSAGE, 0, 0);
    TYPE_FIELD_NET_BITS (NetMessage_ChatMessage, usernameLength, ATOM_UINT, 5);
#if 1
    TYPE_FIELD_NET      (NetMessage_ChatMessage, username,       ATOM_STRING);
    TYPE_FIELD_NET_BITS (NetMessage_ChatMessage, messageLength,  ATOM_UINT, 9);
    TYPE_FIELD_NET      (NetMessage_ChatMessage, message,        ATOM_STRING);
#else
    TYPE_FIELD          (NetMessage_ChatMessage, username,       ATOM_STRING);
    TYPE_FIELD          (NetMessage_ChatMessage, messageLength,  ATOM_SIZE);
    TYPE_FIELD          (NetMessage_ChatMessage, message,        ATOM_STRING);
#endif
}

#if 0
static bool schema_atom_present(const ta_schema_field *field, void *ptr)
{
    bool present = false;
    switch (field->type) {
        case ATOM_BOOL: {
            bool *val = ptr;
            if (*val) present = true;
            break;
        } case ATOM_UINT8: {
            u8 *val = ptr;
            if (*val) present = true;
            break;
        } case ATOM_INT: {
            s32 *val = ptr;
            if (*val) present = true;
            break;
        } case ATOM_UINT: {
            u32 *val = ptr;
            if (*val) present = true;
            break;
        } case ATOM_SIZE: {
            size_t *val = ptr;
            if (*val) present = true;
            break;
        } case ATOM_FLOAT: {
            float *val = ptr;
            if (*val) present = true;
            break;
        } case ATOM_STRING: {
            const char **val = ptr;
            if (*val) present = true;
            break;
        } case ATOM_ENUM: {
            //int *val = ptr;
            present = true;
            break;
        } default: {
            assert(!"Unexpected field type, don't know how to print");
        }
    }
    return present;
}
#endif

static void schema_atom_print(FILE *f, const ta_schema_field *field, void *ptr)
{
    switch (field->type) {
        case ATOM_BOOL: {
            bool *val = ptr;
            fprintf(f, "%s", *val ? "true" : "false");
            break;
        } case ATOM_UINT8: {
            u8 *val = ptr;
            fprintf(f, "%u", *val);
            break;
        } case ATOM_INT: {
            s32 *val = ptr;
            fprintf(f, "%d", *val);
            break;
        } case ATOM_UINT: {
            u32 *val = ptr;
            fprintf(f, "%u", *val);
            break;
        } case ATOM_FLOAT: {
            float *val = ptr;
            fprintf(f, "0x%08X (%f)", *(u32 *)val, *val);
            break;
        } case ATOM_STRING: {
            const char **val = ptr;
            //fprintf(f, "\"%s\"  # %08X\n", IFNULL(*val, ""), (u32)*val);
            fprintf(f, "\"%s\"", IFNULL(*val, ""));
            break;
        } case ATOM_ENUM: {
            int *val = ptr;
            fprintf(f, "%d", *val);
            break;
        } default: {
            assert(!"Unexpected field type, don't know how to print");
        }
    }
}

static inline void indent(FILE *f, int count)
{
    for (int i = 0; i < count; i++) {
        fprintf(f, "  ");
    }
}

void ta_schema_print(FILE *f, ta_schema_field_type type, u8 *ptr, int level, int in_array)
{
    ta_schema *schema = &tg_schemas[type];
    if (!in_array) {
        if (level == 0) {
            fprintf(f, "%s:\n", schema->name);
        }
    }

    // NOTE: Defaults to a value unlikely to be used in any legitimate enum to
    //       detect errors more easily.
    int union_type = 0xFFFF;

    for (size_t fieldIdx = 0; fieldIdx < schema->fieldCount; fieldIdx++) {
        const ta_schema_field *field = &schema->fields[fieldIdx];
        if (field->in_union && field->union_type != union_type) {
            continue;
        }

        if (field->array_len) {
#if 0
            //DLB_ASSERT(!in_array && "Don't know how to print nested arrays");

            u8 *arr = (ptr + field->offset);
            size_t arr_len = field->array_len;
            if (arr_len == 1) {
                arr = *(void **)(ptr + field->offset);
                arr_len = dlb_vec_len(arr);
            }

            if (arr_len) {
                indent(f, level + 1);
                u8 *arr_end = arr + (arr_len * field->size);
                fprintf(f, "%s: [", field->name);
                if (field->type < TYP_COUNT) {
                    fprintf(f, "\n");
                    for (u8 *p = arr; p != arr_end; p += field->size) {
                        indent(f, level + 2);
                        fprintf(f, "{\n");
                        ta_schema_print(f, field->type, p, level + 2, in_array + 1);
                        indent(f, level + 2);
                        fprintf(f, "},\n");
                    }
                    indent(f, level + 1);
                } else {
                    for (u8 *p = arr; p != arr_end; p += field->size) {
                        schema_atom_print(f, field, p);
                        fprintf(f, ", ");
                    }
                }
                fprintf(f, "]\n");
            } else {
                // Don't print empty arrays?
                //indent(f, level + 1);
                //fprintf(f, "%s: []\n", field->name);
            }
#else
            assert(!"I don't think nested arrays are even relevant for binary serialization..");
#endif
        } else {
            if (field->type < TYP_COUNT) {
                indent(f, level + 1);
                fprintf(f, "%s:\n", field->name);
                ta_schema_print(f, field->type, ptr + field->offset, level + 1, 0);
                if (in_array) {
                    fprintf(f, ",");
                }
            } else {
                if (field->is_union_type) {
                    union_type = *(int *)(ptr + field->offset);
                }
                // if (level || in_array || schema_atom_present(field, ptr + field->offset)) {
                    indent(f, level + 1);
                    fprintf(f, "%s: ", field->name);
                    schema_atom_print(f, field, ptr + field->offset);
                    if (in_array) {
                        fprintf(f, ",");
                    }
                    if (field->type == ATOM_ENUM && field->enum_converter) {
                        int enum_type = *(int *)(ptr + field->offset);
                        const char *enum_str = field->enum_converter(enum_type);
                        fprintf(f, "  # %s", enum_str);
                    }
                    fprintf(f, "\n");
                //}
            }
        }
    }
}

static void schema_atom_print_bytes(FILE *f, const ta_schema_field *field, void *ptr)
{
    switch (field->type) {
        case ATOM_BOOL: {
            bool *val = ptr;
            fputs(*val ? "1" : "0", f);
            break;
        } case ATOM_UINT8: {
            u8 *val = ptr;
            for (int i = (int)field->netBits - 1; i >= 0; i--) {
                fputs((*val >> i) & 1 ? "1" : "0", f);
            }
            break;
        } case ATOM_INT: {
        } case ATOM_ENUM: {
            s32 *val = ptr;
            for (int i = (int)field->netBits - 1; i >= 0; i--) {
                fputs((*val >> i) & 1 ? "1" : "0", f);
            }
            break;
        } case ATOM_UINT: {
        } case ATOM_FLOAT: {
            u32 *val = ptr;
            for (int i = (int)field->netBits - 1; i >= 0; i--) {
                fputs((*val >> i) & 1 ? "1" : "0", f);
            }
            break;
        } case ATOM_STRING: {
            const char *val = *(const char **)ptr;
            if (val) {
                while (*val) {
                    fprintf(f, "%x ", *val);
                    val++;
                }
            } else {
                fputs("<null>", f);
            }
            break;
        } default: {
            assert(!"Unexpected field type, don't know how to print");
        }
    }
}

void ta_schema_print_bytes(FILE *f, ta_schema_field_type type, u8 *ptr, int level, int in_array)
{
    ta_schema *schema = &tg_schemas[type];
    if (!in_array) {
        if (level == 0) {
            fprintf(f, "%s:\n", schema->name);
        }
    }

    // NOTE: Defaults to a value unlikely to be used in any legitimate enum to
    //       detect errors more easily.
    int union_type = 0xFFFF;

    for (size_t fieldIdx = 0; fieldIdx < schema->fieldCount; fieldIdx++) {
        const ta_schema_field *field = &schema->fields[fieldIdx];
        if (field->in_union && field->union_type != union_type) {
            continue;
        }

        if (field->array_len) {
#if 0
            //DLB_ASSERT(!in_array && "Don't know how to print nested arrays");

            u8 *arr = (ptr + field->offset);
            size_t arr_len = field->array_len;
            if (arr_len == 1) {
                arr = *(void **)(ptr + field->offset);
                arr_len = dlb_vec_len(arr);
            }

            if (arr_len) {
                indent(f, level + 1);
                u8 *arr_end = arr + (arr_len * field->size);
                fprintf(f, "%s: [", field->name);
                if (field->type < TYP_COUNT) {
                    fprintf(f, "\n");
                    for (u8 *p = arr; p != arr_end; p += field->size) {
                        indent(f, level + 2);
                        fprintf(f, "{\n");
                        ta_schema_print(f, field->type, p, level + 2, in_array + 1);
                        indent(f, level + 2);
                        fprintf(f, "},\n");
                    }
                    indent(f, level + 1);
                } else {
                    for (u8 *p = arr; p != arr_end; p += field->size) {
                        schema_atom_print(f, field, p);
                        fprintf(f, ", ");
                    }
                }
                fprintf(f, "]\n");
            } else {
                // Don't print empty arrays?
                //indent(f, level + 1);
                //fprintf(f, "%s: []\n", field->name);
            }
#else
            assert(!"I don't think nested arrays are even relevant for binary serialization..");
#endif
        } else {
            if (field->type < TYP_COUNT) {
                indent(f, level + 1);
                fprintf(f, "%-*s\n", 20 - (level + 1) * 2, field->name);
                ta_schema_print_bytes(f, field->type, ptr + field->offset, level + 1, 0);
                if (in_array) {
                    fprintf(f, ",");
                }
            } else {
                if (field->is_union_type) {
                    union_type = *(int *)(ptr + field->offset);
                }
                if (field->netBits) {
                    indent(f, level + 1);
                    fprintf(f, "%-*s: ", 20 - (level + 1) * 2, field->name);
                    schema_atom_print_bytes(f, field, ptr + field->offset);
                    if (in_array) {
                        fprintf(f, ",");
                    }
                    if (field->type == ATOM_ENUM && field->enum_converter) {
                        int enum_type = *(int *)(ptr + field->offset);
                        const char *enum_str = field->enum_converter(enum_type);
                        fprintf(f, "  # %s", enum_str);
                    }
                    fprintf(f, "\n");
                }
            }
        }
    }
}

static void bit_stream_flush(BitStream *stream)
{
    stream->buffer[stream->word_index] = stream->scratch & 0xFFFFFFFF;
    stream->word_index++;
    stream->scratch >>= 32;
    stream->scratch_bits -= MIN(stream->scratch_bits, 32);
}

static void bit_stream_write(BitStream *stream, uint32_t word, uint8_t bits)
{
    assert(stream);
    assert(stream->buffer);
    assert(bits);

    uint64_t maskedWord = ((1 << bits) - 1) & word;
    stream->scratch |= maskedWord << stream->scratch_bits;
    stream->scratch_bits += bits;
    stream->total_bits += bits;

    if (stream->scratch_bits > 32) {
        bit_stream_flush(stream);
    }
}

static uint32_t bit_stream_read(BitStream *stream, uint8_t bits)
{
    assert(stream);
    assert(stream->buffer);
    assert(bits);

    assert(stream->num_bits_read + bits <= stream->total_bits);

    if (bits > stream->scratch_bits) {
        stream->scratch |= stream->buffer[stream->word_index] << stream->scratch_bits;
        stream->word_index++;
        stream->scratch_bits += 32;
    }

    uint32_t word = ((1 << bits) - 1) & stream->scratch;
    stream->scratch >>= bits;
    stream->scratch_bits -= bits;
    stream->num_bits_read += bits;
    return word;
}

// https://gafferongames.com/post/serialization_strategies/
static void bit_stream_write_align(BitStream *stream)
{
    const int remainderBits = stream->total_bits % 8;
    if (remainderBits) {
        uint32_t zero = 0;
        bit_stream_write(stream, zero, 8 - remainderBits);
        assert(stream->scratch_bits % 8 == 0);
    }
}

static void bit_stream_read_align(BitStream *stream)
{
    const int remainderBits = stream->num_bits_read % 8;
    if (remainderBits) {
        uint32_t value = bit_stream_read(stream, 8 - remainderBits);
        assert(stream->scratch_bits % 8 == 0);
        if (value != 0) {
            assert(!"Alignment padding corruption detected; expected zero padding");
            //return false;
        }
    }
}

static void schema_atom_write_bit_stream(BitStream *stream, const ta_schema_field *field, void *ptr)
{
    switch (field->type) {
        case ATOM_BOOL: {
        } case ATOM_UINT8: {
            uint32_t word = *(uint8_t *)ptr;
            bit_stream_write(stream, word, (uint8_t)field->netBits);
            break;
        } case ATOM_INT: {
        } case ATOM_ENUM: {
        } case ATOM_UINT: {
        } case ATOM_FLOAT: {
            uint32_t word = *(uint32_t *)ptr;
            bit_stream_write(stream, word, (uint8_t)field->netBits);
            break;
        } case ATOM_STRING: {
            // Align bit stream to byte boundary
            bit_stream_write_align(stream);

            // TODO(perf): Write enough of string to align to word boundary, then copy words at a time until tail.
            // That probably isn't even worth it unless we know the string's length ahead of time.
            const char *val = *(const char **)ptr;
            if (val) {
                while (*val) {
                    const u8 byte = *val;
                    bit_stream_write(stream, (uint32_t)byte, 8);
                    val++;
                }
            }

            // Write nil terminator
            bit_stream_write(stream, 0, 8);
            break;
        } default: {
            assert(!"Unexpected field type, don't know how to write");
        }
    }
}

static void schema_atom_read_bit_stream(BitStream *stream, const ta_schema_field *field, void *ptr)
{
    switch (field->type) {
        case ATOM_BOOL: {
        } case ATOM_UINT8: {
            uint8_t *val = ptr;
            uint32_t word = bit_stream_read(stream, (uint8_t)field->netBits);
            *val = (uint8_t)word;
            break;
        } case ATOM_INT: {
        } case ATOM_ENUM: {
        } case ATOM_UINT: {
        } case ATOM_FLOAT: {
            uint32_t *val = ptr;
            uint32_t word = bit_stream_read(stream, (uint8_t)field->netBits);
            *val = (uint32_t)word;
            break;
        } case ATOM_STRING: {
            // Align bit stream to byte boundary
            bit_stream_read_align(stream);

            // TODO(perf): Read enough of string to align to word boundary, then copy words at a time until tail.
            // That probably isn't even worth it unless we know the string's length ahead of time.
            char **val = (char **)ptr;
            assert(stream->num_bits_read % 8 == 0);
            size_t byteOffset = stream->num_bits_read / 8;
            *val = (char *)stream->buffer + byteOffset;

            // Discard characters that we're pointing to with the string
            char *c = *val;
            if (c) {
                while (*c) {
                    uint8_t byte = (uint8_t)bit_stream_read(stream, 8);
                    //*val = byte;
                    c++;
                }
            }

            // Discard nil terminator
            bit_stream_read(stream, 8);
            break;
        } default: {
            assert(!"Unexpected field type, don't know how to read");
        }
    }
}

void ta_schema_write_bit_stream(BitStream *stream, ta_schema_field_type type, u8 *ptr, int level, int in_array)
{
#if 1
    ta_schema *schema = &tg_schemas[type];

    // NOTE: Defaults to a value unlikely to be used in any legitimate enum to
    //       detect errors more easily.
    int union_type = 0xFFFF;

    for (size_t fieldIdx = 0; fieldIdx < schema->fieldCount; fieldIdx++) {
        const ta_schema_field *field = &schema->fields[fieldIdx];

        // Skip union fields for other types
        if (field->in_union && field->union_type != union_type) {
            continue;
        }

        assert(!field->array_len && "Not currently handling arrays for binary serialization..");

        if (field->type < TYP_COUNT) {
            ta_schema_write_bit_stream(stream, field->type, ptr + field->offset, level + 1, 0);
        } else {
            if (field->is_union_type) {
                union_type = *(int *)(ptr + field->offset);
            }
            if (field->netBits) {
                schema_atom_write_bit_stream(stream, field, ptr + field->offset);
            }
        }
    }
#else
    // 11000000100010000001111111101110
    // 00001000100000011111111011101111 11000000000000000000000000000000
    bit_stream_write(stream, 0b0, 1);
    bit_stream_write(stream, 0b11, 2);
    bit_stream_write(stream, 0b000, 3);
    bit_stream_write(stream, 0b1111, 4);
    bit_stream_write(stream, 0b00000, 5);
    bit_stream_write(stream, 0b111111, 6);
    bit_stream_write(stream, 0b0000000, 7);
    bit_stream_write(stream, 0b11111111, 8);
#endif

    // Flush any remaining bits on last recursion
    if (level == 0 && stream->scratch_bits) {
        bit_stream_flush(stream);
        //stream->buffer[stream->word_index] = stream->scratch & 0xFFFFFFFF;
        //stream->word_index++;
        //stream->scratch >>= 32;
        //stream->scratch_bits -= 32;
    }
}

void ta_schema_read_bit_stream(BitStream *stream, ta_schema_field_type type, u8 *ptr, int level, int in_array)
{
#if 1
    ta_schema *schema = &tg_schemas[type];

    // NOTE: Defaults to a value unlikely to be used in any legitimate enum to
    //       detect errors more easily.
    int union_type = 0xFFFF;

    for (size_t fieldIdx = 0; fieldIdx < schema->fieldCount; fieldIdx++) {
        const ta_schema_field *field = &schema->fields[fieldIdx];

        // Skip union fields for other types
        if (field->in_union && field->union_type != union_type) {
            continue;
        }

        assert(!field->array_len && "Not currently handling arrays for binary serialization..");

        if (field->type < TYP_COUNT) {
            ta_schema_read_bit_stream(stream, field->type, ptr + field->offset, level + 1, 0);
        } else {
            if (field->netBits) {
                schema_atom_read_bit_stream(stream, field, ptr + field->offset);
            }
            if (field->is_union_type) {
                union_type = *(int *)(ptr + field->offset);
            }
        }
    }
#else
    // 11000000100010000001111111101110
    // 00001000100000011111111011101111 11000000000000000000000000000000
    assert(bit_stream_read(stream, 1) == 0b0       );
    assert(bit_stream_read(stream, 2) == 0b11      );
    assert(bit_stream_read(stream, 3) == 0b000     );
    assert(bit_stream_read(stream, 4) == 0b1111    );
    assert(bit_stream_read(stream, 5) == 0b00000   );
    assert(bit_stream_read(stream, 6) == 0b111111  );
    assert(bit_stream_read(stream, 7) == 0b0000000 );
    assert(bit_stream_read(stream, 8) == 0b11111111);
#endif
}

static void write_net_message_identify(BitStream *stream, const NetMessage_Identify *identify)
{
    assert(stream);
    assert(identify);
    assert(identify->usernameLength <= USERNAME_LENGTH_MAX);

    bit_stream_write(stream, (uint32_t)identify->usernameLength, 5);
    bit_stream_write_align(stream);
    for (int i = 0; i < identify->usernameLength; i++) {
        bit_stream_write(stream, identify->username[i], 8);
    }
}

static void write_net_message_welcome(BitStream *stream, const NetMessage_Welcome *welcome)
{
    UNUSED(stream);
    UNUSED(welcome);
}

static void write_net_message_chat_message(BitStream *stream, const NetMessage_ChatMessage *chatMessage)
{
    assert(stream);
    assert(chatMessage);
    assert(chatMessage->usernameLength <= USERNAME_LENGTH_MAX);
    assert(chatMessage->messageLength <= CHAT_MESSAGE_LENGTH_MAX);

    bit_stream_write(stream, (uint32_t)chatMessage->usernameLength, 5);
    bit_stream_write_align(stream);
    for (int i = 0; i < chatMessage->usernameLength; i++) {
        bit_stream_write(stream, chatMessage->username[i], 8);
    }

    bit_stream_write(stream, (uint32_t)chatMessage->messageLength, 9);
    bit_stream_write_align(stream);
    for (int i = 0; i < chatMessage->messageLength; i++) {
        bit_stream_write(stream, chatMessage->message[i], 8);
    }
}

void ta_schema_write_net_message(BitStream *stream, const NetMessage *message)
{
    bit_stream_write(stream, message->type, 2);
    switch (message->type) {
        case NetMessageType_Identify: {
            write_net_message_identify(stream, &message->data.identify);
            break;
        } case NetMessageType_Welcome: {
            write_net_message_welcome(stream, &message->data.welcome);
            break;
        } case NetMessageType_ChatMessage: {
            write_net_message_chat_message(stream, &message->data.chatMessage);
            break;
        }
    }
    bit_stream_flush(stream);
}

static void read_net_message_identify(BitStream *stream, NetMessage_Identify *identify)
{
    assert(stream);
    assert(identify);

    identify->usernameLength = bit_stream_read(stream, 5);
    assert(identify->usernameLength <= USERNAME_LENGTH_MAX);
    bit_stream_read_align(stream);
    assert(stream->num_bits_read % 8 == 0);
    identify->username = (char *)stream->buffer + stream->num_bits_read / 8;
    for (int i = 0; i < identify->usernameLength; i++) {
        bit_stream_read(stream, 8);
    }
}

static void read_net_message_welcome(BitStream *stream, NetMessage_Welcome *welcome)
{
    UNUSED(stream);
    UNUSED(welcome);
}

static void read_net_message_chat_message(BitStream *stream, NetMessage_ChatMessage *chatMessage)
{
    assert(stream);
    assert(chatMessage);

    chatMessage->usernameLength = bit_stream_read(stream, 5);
    assert(chatMessage->usernameLength <= USERNAME_LENGTH_MAX);
    bit_stream_read_align(stream);
    assert(stream->num_bits_read % 8 == 0);
    chatMessage->username = (char *)stream->buffer + stream->num_bits_read / 8;
    for (int i = 0; i < chatMessage->usernameLength; i++) {
        bit_stream_read(stream, 8);
    }

    chatMessage->messageLength = bit_stream_read(stream, 9);
    assert(chatMessage->messageLength <= CHAT_MESSAGE_LENGTH_MAX);
    bit_stream_read_align(stream);
    assert(stream->num_bits_read % 8 == 0);
    chatMessage->message = (char *)stream->buffer + stream->num_bits_read / 8;
    for (int i = 0; i < chatMessage->messageLength; i++) {
        bit_stream_read(stream, 8);
    }
}

void ta_schema_read_net_message(BitStream *stream, NetMessage *message)
{
    message->type = bit_stream_read(stream, 2);
    switch (message->type) {
        case NetMessageType_Identify: {
            read_net_message_identify(stream, &message->data.identify);
            break;
        } case NetMessageType_Welcome: {
            read_net_message_welcome(stream, &message->data.welcome);
            break;
        } case NetMessageType_ChatMessage: {
            read_net_message_chat_message(stream, &message->data.chatMessage);
            break;
        }
    }
}