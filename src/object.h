#pragma once
#include <cstdint>

typedef uint8_t ObjectType;
typedef uint16_t ObjectFlags;

enum : ObjectType {
    ObjectType_None,

    ObjectType_MossyStone,
    ObjectType_Rock01,
    ObjectType_Rock01_Overturned,
    ObjectType_Unused03,
    ObjectType_Unused04,
    ObjectType_Unused05,
    ObjectType_Unused06,
    ObjectType_Unused07,

    ObjectType_Unused10,
    ObjectType_Unused11,
    ObjectType_Unused12,
    ObjectType_Unused13,
    ObjectType_Unused14,
    ObjectType_Unused15,
    ObjectType_Unused16,
    ObjectType_Unused17,

    ObjectType_Unused20,
    ObjectType_Unused21,
    ObjectType_Unused22,
    ObjectType_Unused23,
    ObjectType_Unused24,
    ObjectType_Unused25,
    ObjectType_Unused26,
    ObjectType_Unused27,

    ObjectType_Unused30,
    ObjectType_Unused31,
    ObjectType_Unused32,
    ObjectType_Unused33,
    ObjectType_Unused34,
    ObjectType_Unused35,
    ObjectType_Unused36,
    ObjectType_Unused37,

    ObjectType_Count
};

enum : ObjectFlags {
    ObjectFlag_None                     = 0,

    // per-type flags (last 8 bits)
    ObjectFlag_Stone_Overturned         = 0x01,

    ObjectFlag_Door_Opened              = 0x01,
    ObjectFlag_Door_Locked              = 0x02,

    // shared flags (first 8 bits)
    ObjectFlag_Collide                  = 0x0100,
    ObjectFlag_Interact                 = 0x0200,
    ObjectFlag_Unused2                  = 0x0400,
    ObjectFlag_Unused3                  = 0x0800,
    ObjectFlag_Unused4                  = 0x1000,
    ObjectFlag_Unused5                  = 0x2000,
    ObjectFlag_Unused6                  = 0x4000,
    ObjectFlag_Unused7                  = 0x8000,
};

struct Object {
    ObjectType  type      {};  // type of object
    ObjectFlags flags     {};  // [00000000|00000000] -> [global flags|type flags]

    const char *ToString(void) const {
        switch (type) {
            case ObjectType_MossyStone : return "MossyStone";
            case ObjectType_Rock01     : return "Rock01";
            default                    : return "<ObjectType>";
        }
    }

    inline bool HasFlag(ObjectFlags flag) const {
        return flags & flag;
    }

    inline void SetFlag(ObjectFlags flag) {
        flags |= flag;
    }

    inline void ClearFlag(ObjectFlags flag) {
        flags &= ~flag;
    }

    inline bool IsIteractable(void) const { return HasFlag(ObjectFlag_Interact); }
    inline bool IsCollidable(void) const { return HasFlag(ObjectFlag_Collide); }
    inline bool IsWalkable(void) const { return !HasFlag(ObjectFlag_Collide); }
    inline bool IsSpawnable(void) const { return !HasFlag(ObjectFlag_Collide); }

    ObjectType EffectiveType(void) const {
        ObjectType effectiveType = type;
        switch (type) {
            case ObjectType_Rock01: {
                if (HasFlag(ObjectFlag_Stone_Overturned)) {
                    effectiveType = ObjectType_Rock01_Overturned;
                }
                break;
            }
        }
        return effectiveType;
    }
};
