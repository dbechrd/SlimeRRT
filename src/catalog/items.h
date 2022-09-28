#pragma once

#include "csv.h"
#include "imgui/imgui.h"
#include <dlb_murmur3.h>
#include <dlb_rand.h>
#include <raylib/raylib.h>
#include <array>
#include <unordered_map>

typedef uint8_t  ItemSlot;
typedef uint8_t  ItemClass;
typedef uint16_t ItemType;
typedef uint8_t  ItemAffixType;
typedef uint32_t ItemUID;

#define ITEM_AFFIX_MAX_COUNT 16
#define ITEM_NAME_MAX_LENGTH 64
#define ITEMCLASS_COUNT      256

// slot:       chest, weapon, shield, feet, head, neck, magicOrb1, magicOrb2, potion
// class:      weapon, armor, necklace, ring, consumable, scroll, misc
// id:
//   weapon:     sword, stave, key,
//   armor:      combat boots, high heels, sneakers, motorcycle gloves, mittens, rubber gloves
//   amulet:     velvet charm, tattoo choker, gold chain, iced out
//   ring:       engagement ring, gold band, snake, brass knuckles
//   consumable: avocado, pulled pork, cheddar cheese, onion, fried chicken, blood vile, magic shrooms, moonshine
//   misc:       key

enum : ItemSlot {
    ItemSlot_Hat,         // baseball cap, top hat, stetson
    ItemSlot_Head,        // motorcycle helmet, hard hat, barbuta
    ItemSlot_Eyes,        // sunglasses

    ItemSlot_Chest,       // sports bra, shoulder holster, suspenders, bodysuit
    ItemSlot_Torso,       // chainmail, kevlar vest
    ItemSlot_Back,        // t-shirt, wings
    ItemSlot_Arms,        // elbow pads, arm cast, compression sleeve
    ItemSlot_Hands,       // mittens, rubber gloves, baseball mitt, oven mitts
    ItemSlot_Fingers,     // rings (1 finger ea), brass knuckles (3 fingers ea)
    ItemSlot_Fingernails, // nail polish (green = +1 psn dmg, red = bleeding, orange = fire dmg, etc)

    ItemSlot_Underwear,   // whitey tighties, boxer briefs, panties
    ItemSlot_Legs,        // jeans, leggings, tights, firefighter pants, motocross riding pants
    ItemSlot_Feet,        // sneakers, combat boots, high heels, flip flops, boat shoes
    ItemSlot_Toes,        // tiny rings with small stat bonuses
    ItemSlot_Toenails,    // nail polish (same as fingers, mix-and-match bonuses, same color = 2.5x)
};

enum : ItemClass {
    ItemClass_Empty,
    ItemClass_System,
    ItemClass_Currency,
    ItemClass_Gem,
    ItemClass_Ring,
    ItemClass_Amulet,
    ItemClass_Crown,
    ItemClass_Key,
    ItemClass_Coin,
    ItemClass_Ore,
    ItemClass_Item,
    ItemClass_Potion,
    ItemClass_Ingot,
    ItemClass_Nugget,
    ItemClass_Tool,
    ItemClass_Weapon,
    ItemClass_Armor,
    ItemClass_Shield,
    ItemClass_Plant,
    ItemClass_Book,
    ItemClass_Count
};

enum : ItemAffixType {
    ItemAffix_Empty,
    ItemAffix_DamageFlat,            // flat damage to deal
    ItemAffix_DamageSourcePctHealth, // damage to deal as a percentage of source health
    ItemAffix_DamageTargetPctHealth, // damage to deal as a percentage of target health
    ItemAffix_KnockbackFlat,         // flat knockback to deal
    ItemAffix_KnockbackPctDamage,    // percent of damage to deal as knockback
    ItemAffix_MoveSpeedFlat,         // flat movement speed bonus
    ItemAffix_Value,                 // calculated on drop, based on other affixes, where it spawned, value bonus effects, etc.
    ItemAffix_Count
};

struct FloatRange {
    float min {};
    float max {};

    bool operator==(const FloatRange &other) const {
        return this == &other || (min == other.min && max == other.max);
    }

    bool IsScalar(void) const {
        return min == max;
    }
};

struct ItemAffix {
    ItemAffixType type{};
    FloatRange    value{}; // assert max.min > min.max

    static ItemAffix make(ItemAffixType type, float value)
    {
        ItemAffix affix{};
        affix.type = type;
        affix.value.min = value;
        affix.value.max = value;
        return affix;
    }

    static ItemAffix make(ItemAffixType type, float min, float max)
    {
        ItemAffix affix{};
        affix.type = type;
        affix.value.min = min;
        affix.value.max = max;
        return affix;
    }

    float rollValue() const {
        const float roll = dlb_rand32f_range(value.min, value.max);
        return roll;
    }
};

// e.g. ItemAffix_DamageFlat (82-99) to (139-412)
struct ItemAffixProto {
    ItemAffixType type     {};
    FloatRange    minRange {};
    FloatRange    maxRange {}; // assert max.min > min.max

    ItemAffixProto() = default;

    ItemAffixProto(ItemAffixType type, FloatRange minRange, FloatRange maxRange) :
        type(type), minRange(minRange), maxRange(maxRange) {};

    ItemAffixProto(ItemAffixType type, FloatRange range) :
        type(type), minRange(range), maxRange(range) {};

    // Returns true if all affixes have scalar values, i.e. every roll would produce an identical item
    bool IsScalar(void) const {
        const bool isScalar = type == ItemAffix_Empty || (minRange.IsScalar() && minRange == maxRange);
        return isScalar;
    }

    ItemAffix Roll() const {
        float min = dlb_rand32f_range(minRange.min, minRange.max);
        float max = dlb_rand32f_range(maxRange.min, maxRange.max);
        return ItemAffix::make(type, min, max);
    }
};

enum : ItemType {
    ItemType_Empty,
    ItemType_Unknown,
    ItemType_Currency_Copper,
    ItemType_Currency_Silver,
    ItemType_Currency_Gilded,
    ItemType_Orig_Gem_Ruby,
    ItemType_Orig_Gem_Emerald,
    ItemType_Orig_Gem_Saphire,
    ItemType_Orig_Gem_Diamond,
    ItemType_Orig_Gem_GoldenChest,
    ItemType_Orig_Unused_10,
    ItemType_Orig_Unused_11,
    ItemType_Orig_Unused_12,

    // TODO: Better names: https://5.imimg.com/data5/TX/SL/MY-2530321/gemstones-1000x1000.jpg
    ItemType_Ring_Stone,
    ItemType_Ring_Bone,
    ItemType_Ring_Obsidian,
    ItemType_Ring_Zebra,
    ItemType_Ring_Rust,
    ItemType_Ring_BlueSteel,
    ItemType_Ring_Jade,
    ItemType_Amulet_Silver_Topaz,
    ItemType_Amulet_Silver_Saphire,
    ItemType_Amulet_Silver_Emerald,
    ItemType_Amulet_Silver_Ruby,
    ItemType_Amulet_Silver_Amethyst,
    ItemType_Amulet_Silver_Diamond,

    ItemType_Ring_Silver_Topaz,
    ItemType_Ring_Silver_Saphire,
    ItemType_Ring_Silver_Emerald,
    ItemType_Ring_Silver_Ruby,
    ItemType_Ring_Silver_Amethyst,
    ItemType_Ring_Silver_Diamond,
    ItemType_Ring_Silver_Plain,
    ItemType_Amulet_Gold_Topaz,
    ItemType_Amulet_Gold_Saphire,
    ItemType_Amulet_Gold_Emerald,
    ItemType_Amulet_Gold_Ruby,
    ItemType_Amulet_Gold_Amethyst,
    ItemType_Amulet_Gold_Diamond,

    ItemType_Ring_Gold_Topaz,
    ItemType_Ring_Gold_Saphire,
    ItemType_Ring_Gold_Emerald,
    ItemType_Ring_Gold_Ruby,
    ItemType_Ring_Gold_Amethyst,
    ItemType_Ring_Gold_Diamond,
    ItemType_Ring_Gold_Plain,
    ItemType_Crown_Gold_Spitz,
    ItemType_Crown_Gold_Spitz_Ruby,
    ItemType_Crown_Gold_Spitz_Velvet,
    ItemType_Crown_Silver_Circlet,
    ItemType_Crown_Gold_Circlet,
    ItemType_Key_Horrendously_Awful,

    ItemType_Coin_Copper,
    ItemType_Gem_Saphire,
    ItemType_Gem_Emerald,  // Olivine? search other "minerals"
    ItemType_Gem_Ruby,
    ItemType_Gem_Amethyst,
    ItemType_Gem_Diamond,
    ItemType_Gem_Pink_Himalayan_Salt,
    ItemType_Coin_Gold_Stack,
    ItemType_Ore_Gold,
    ItemType_Item_Scroll,
    ItemType_Item_Red_Cape,
    ItemType_Key_Gold,
    ItemType_Key_Less_Awful,

    ItemType_Ore_Limonite,
    ItemType_Ore_Copper,
    ItemType_Ore_Silver,
    ItemType_Ore_Virilium,
    ItemType_Ore_Steel,
    ItemType_Ore_Cerulean,
    ItemType_Ore_Tungsten,
    ItemType_Potion_White,
    ItemType_Potion_Black,
    ItemType_Potion_Purple,
    ItemType_Potion_Green,
    ItemType_Potion_Red,
    ItemType_Potion_Blue,

    ItemType_Ingot_Limonite,
    ItemType_Ingot_Copper,
    ItemType_Ingot_Silver,
    ItemType_Ingot_Virilium,
    ItemType_Ingot_Steel,
    ItemType_Ingot_Cerulean,
    ItemType_Ingot_Tungsten,
    ItemType_Potion_Brown,
    ItemType_Potion_Turquoise,
    ItemType_Potion_Yellow,
    ItemType_Potion_Orange,
    ItemType_Potion_Pink,
    ItemType_Potion_Ornament, // or change color into mystery potion? idk.. has no outline

    ItemType_Nugget_Silver,
    ItemType_Nugget_Gold,
    ItemType_Item_Logs,
    ItemType_Item_Rope,
    ItemType_Tool_Shovel,
    ItemType_Tool_Pickaxe,
    ItemType_Tool_Mallet,
    ItemType_Item_Log,
    ItemType_Weapon_Great_Axe,
    ItemType_Tool_Hatchet,
    ItemType_Weapon_Winged_Axe,
    ItemType_Weapon_Labrys,
    ItemType_Tool_Scythe,

    ItemType_Weapon_Dagger,
    ItemType_Weapon_Long_Sword,
    ItemType_Weapon_Jagged_Long_Sword,
    ItemType_Weapon_Short_Sword,
    ItemType_Weapon_Double_Axe,
    ItemType_Weapon_Spear,
    ItemType_Weapon_Mace,
    ItemType_Weapon_Machete,
    ItemType_Weapon_Slicer,
    ItemType_Weapon_Great_Blade,
    ItemType_Weapon_Great_Slicer,
    ItemType_Weapon_Great_Tungsten_Sword,
    ItemType_Weapon_Excalibur,

    ItemType_Weapon_Lightsabre,
    ItemType_Weapon_Serated_Edge,
    ItemType_Weapon_Noble_Serated_Edge,
    ItemType_Weapon_Gemmed_Long_Sword,
    ItemType_Weapon_Gemmed_Double_Blade,
    ItemType_Weapon_Tungsten_Spear,
    ItemType_Weapon_Bludgeoner,
    ItemType_Weapon_Hammer,
    ItemType_Weapon_Round_Hammer,
    ItemType_Weapon_Conic_Hammer,
    ItemType_Weapon_Short_Slicer,
    ItemType_Weapon_Tungsten_Sword,
    ItemType_Weapon_Tungsten_Dagger,

    ItemType_Armor_Helmet_Plain,
    ItemType_Armor_Helmet_Neck,
    ItemType_Armor_Helmet_Short,
    ItemType_Armor_Helmet_Grill,
    ItemType_Armor_Helmet_Galea,
    ItemType_Armor_Helmet_Cerulean,
    ItemType_Armor_Helmet_Winged,
    ItemType_Armor_Helmet_Galea_Angled,
    ItemType_Armor_Helmet_Leather,
    ItemType_Armor_Shirt,
    ItemType_Armor_Glove,
    ItemType_Weapon_Bow,
    ItemType_Item_Skull_And_Crossbones,

    ItemType_Armor_Tungsten_Helmet_Winged,
    ItemType_Armor_Tungsten_Chestplate,
    ItemType_Armor_Tungsten_Boots,
    ItemType_Armor_Tungsten_Helmet_Spiked,
    ItemType_Armor_Tungsten_Helmet_BigBoy,
    ItemType_Armor_Leather_Boots,
    ItemType_Armor_Leather_Booties,
    ItemType_Armor_Leather_Sandals,
    ItemType_Armor_Black_Leather_Boots,
    ItemType_Armor_Black_Leather_Booties,
    ItemType_Armor_Tungsten_Gloves,
    ItemType_Item_Rock,
    ItemType_Item_Paper,

    ItemType_Weapon_Staff_Wooden,
    ItemType_Weapon_Staff_Scepter,
    ItemType_Weapon_Staff_Orb,
    ItemType_Weapon_Staff_Noble,
    ItemType_Weapon_Staff_Enchanted,
    ItemType_Shield,
    ItemType_Shield_Round_Reinforced,
    ItemType_Shield_Round,
    ItemType_Shield_Royal,
    ItemType_Shield_Holy,
    ItemType_Item_Empty_Space,
    ItemType_Item_Worm,
    ItemType_Item_Dice,

    ItemType_Plant_TreeBush,
    ItemType_Plant_LeafLarge,
    ItemType_Plant_Lily,
    ItemType_Plant_Seaweed,
    ItemType_Plant_Mushroom,
    ItemType_Plant_Seagrass,
    ItemType_Plant_Berries,
    ItemType_Book_Green,
    ItemType_Book_Red,
    ItemType_Book_Blue,
    ItemType_Book_White,
    ItemType_Book_Brown,
    ItemType_Book_RedHeart,

    ItemType_Plant_FishEggs,
    ItemType_Plant_LeafSmall,
    ItemType_Plant_Leaves,
    ItemType_Plant_BlueFlower,
    ItemType_Plant_SpottedMushroom,
    ItemType_Plant_Rose,
    ItemType_Plant_Geranium,
    ItemType_Book_Purple,
    ItemType_Book_Yellow,
    ItemType_Book_Turquoise,
    ItemType_Book_Black,
    ItemType_Book_BlackSkull,
    ItemType_Book_BlueShield,

    ItemType_Count
};

Color ItemClassToColor(ItemClass itemClass)
{
    switch (itemClass) {
        case ItemClass_Empty    : return DARKGRAY;
        case ItemClass_System   : return PINK    ;
        case ItemClass_Currency : return GOLD    ;
        case ItemClass_Gem      : return BLUE    ;
        case ItemClass_Ring     : return BLUE    ;
        case ItemClass_Amulet   : return BLUE    ;
        case ItemClass_Crown    : return BLUE    ;
        case ItemClass_Key      : return YELLOW  ;
        case ItemClass_Coin     : return GOLD    ;
        case ItemClass_Ore      : return ORANGE  ;
        case ItemClass_Item     : return WHITE   ;
        case ItemClass_Potion   : return PURPLE  ;
        case ItemClass_Ingot    : return ORANGE  ;
        case ItemClass_Nugget   : return ORANGE  ;
        case ItemClass_Tool     : return GRAY    ;
        case ItemClass_Weapon   : return { 190, 190, 190, 255 };  // silver
        case ItemClass_Armor    : return GRAY    ;
        case ItemClass_Shield   : return GRAY    ;
        case ItemClass_Plant    : return GREEN   ;
        case ItemClass_Book     : return { 145, 104, 36, 255 };  // leather brown
        default                 : return MAGENTA ;
    }
}

ImVec4 RayToImColor(Color color)
{
    ImColor imColor{ color.r, color.g, color.b, color.a };
    return imColor.Value;
}

const char *ItemClassToString(ItemClass itemClass)
{
    switch (itemClass) {
        case ItemClass_Empty    : return "Empty"     ;
        case ItemClass_System   : return "System"    ;
        case ItemClass_Currency : return "Currency"  ;
        case ItemClass_Gem      : return "Gem"       ;
        case ItemClass_Ring     : return "Ring"      ;
        case ItemClass_Amulet   : return "Amulet"    ;
        case ItemClass_Crown    : return "Crown"     ;
        case ItemClass_Key      : return "Key"       ;
        case ItemClass_Coin     : return "Coin"      ;
        case ItemClass_Ore      : return "Ore"       ;
        case ItemClass_Item     : return "Item"      ;
        case ItemClass_Potion   : return "Potion"    ;
        case ItemClass_Ingot    : return "Ingot"     ;
        case ItemClass_Nugget   : return "Nugget"    ;
        case ItemClass_Tool     : return "Tool"      ;
        case ItemClass_Weapon   : return "Weapon"    ;
        case ItemClass_Armor    : return "Armor"     ;
        case ItemClass_Shield   : return "Shield"    ;
        case ItemClass_Plant    : return "Plant"     ;
        case ItemClass_Book     : return "Book"      ;
        default                 : return "<ItemClass>";
    }
}

ItemClass ItemClassFromString(const char *str, size_t length)
{
    static std::unordered_map<u32, ItemClass> itemClassByName{};
    if (!itemClassByName.size()) {
        itemClassByName[dlb_murmur3(CSTR("Empty"   ))] = ItemClass_Empty;
        itemClassByName[dlb_murmur3(CSTR("System"  ))] = ItemClass_System;
        itemClassByName[dlb_murmur3(CSTR("Currency"))] = ItemClass_Currency;
        itemClassByName[dlb_murmur3(CSTR("Gem"     ))] = ItemClass_Gem;
        itemClassByName[dlb_murmur3(CSTR("Ring"    ))] = ItemClass_Ring;
        itemClassByName[dlb_murmur3(CSTR("Amulet"  ))] = ItemClass_Amulet;
        itemClassByName[dlb_murmur3(CSTR("Crown"   ))] = ItemClass_Crown;
        itemClassByName[dlb_murmur3(CSTR("Key"     ))] = ItemClass_Key;
        itemClassByName[dlb_murmur3(CSTR("Coin"    ))] = ItemClass_Coin;
        itemClassByName[dlb_murmur3(CSTR("Ore"     ))] = ItemClass_Ore;
        itemClassByName[dlb_murmur3(CSTR("Item"    ))] = ItemClass_Item;
        itemClassByName[dlb_murmur3(CSTR("Potion"  ))] = ItemClass_Potion;
        itemClassByName[dlb_murmur3(CSTR("Ingot"   ))] = ItemClass_Ingot;
        itemClassByName[dlb_murmur3(CSTR("Nugget"  ))] = ItemClass_Nugget;
        itemClassByName[dlb_murmur3(CSTR("Tool"    ))] = ItemClass_Tool;
        itemClassByName[dlb_murmur3(CSTR("Weapon"  ))] = ItemClass_Weapon;
        itemClassByName[dlb_murmur3(CSTR("Armor"   ))] = ItemClass_Armor;
        itemClassByName[dlb_murmur3(CSTR("Shield"  ))] = ItemClass_Shield;
        itemClassByName[dlb_murmur3(CSTR("Plant"   ))] = ItemClass_Plant;
        itemClassByName[dlb_murmur3(CSTR("Book"    ))] = ItemClass_Book;
    }

    ItemClass itemClass = ItemClass_Empty;
    u32 hashCode = dlb_murmur3(str, length);
    auto kv = itemClassByName.find(hashCode);
    if (kv != itemClassByName.end()) {
        itemClass = kv->second;
    }
    return itemClass;
}

struct ItemProto {
    ItemType  itemType     {};
    ItemClass itemClass    {};
    uint8_t   stackLimit   {};
    char      nameSingular [ITEM_NAME_MAX_LENGTH]{};
    char      namePlural   [ITEM_NAME_MAX_LENGTH]{};
    // TODO: ENSURE NO DUPES (but without hash table or O(n) storage, i.e. use addAffix method w/ check and
    //       removeAffix to keep tightly packed)
    ItemAffixProto affixProtos[ITEM_AFFIX_MAX_COUNT]{};

    ItemAffixProto FindAffixProto(ItemAffixType type) const {
        for (size_t i = 0; i < ITEM_AFFIX_MAX_COUNT; i++) {
            if (affixProtos[i].type == type) {
                return affixProtos[i];
            }
        }
        return ItemAffixProto{};
    }

    bool SetAffixProto(ItemAffixType type, FloatRange min, FloatRange max) {
        ItemAffixProto *proto = 0;
        for (size_t i = 0; i < ITEM_AFFIX_MAX_COUNT; i++) {
            if (affixProtos[i].type == ItemAffix_Empty || affixProtos[i].type == type) {
                proto = &affixProtos[i];
                break;
            }
        }
        if (proto) {
            *proto = ItemAffixProto(type, min, max);
        }
        return proto;
    }

    bool SetAffixProto(ItemAffixType type, FloatRange value) {
        return SetAffixProto(type, value, value);
    }

    // Returns true if all affixes have scalar values, i.e. every roll would produce an identical item
    bool IsScalar(void) const {
        bool isScalar = true;
        for (size_t i = 0; i < ITEM_AFFIX_MAX_COUNT; i++) {
            isScalar &= affixProtos[i].IsScalar();
        }
        return isScalar;
    }
};

struct ItemCatalog {
    void LoadTextures(void);
    void LoadData(void);

    ItemProto &FindProto(ItemType type) {
        DLB_ASSERT(type < ItemType_Count);
        return protos[type];
    }

    Texture Tex() const {
        return tex;
    }

private:
    Texture   tex{};
    CSV       csv{};
    ItemProto protos[ItemType_Count]{};
};

thread_local ItemCatalog g_item_catalog{};

struct Item {
    ItemUID  uid        {};
    ItemType type       {};
    // TODO: ENSURE NO DUPES (but without hash table or O(n) storage, i.e. use addAffix method w/ check and
    //       removeAffix to keep tightly packed)
    ItemAffix affixes[ITEM_AFFIX_MAX_COUNT]{};

    Item(void) {}

    Item(ItemUID uid, ItemType type) : uid(uid), type(type) {
        Roll();
    }

    const ItemProto &Proto(void) const {
        const ItemProto &proto = g_item_catalog.FindProto(type);
        return proto;
    }

    void Roll() {
        const ItemProto &proto = g_item_catalog.FindProto(type);
        for (int i = 0; i < ARRAY_SIZE(affixes) && i < ARRAY_SIZE(proto.affixProtos); i++) {
            affixes[i] = proto.affixProtos[i].Roll();
        }
    }

    ItemAffix FindAffix(ItemAffixType type) const {
        for (size_t i = 0; i < ITEM_AFFIX_MAX_COUNT; i++) {
            if (affixes[i].type == type) {
                return affixes[i];
            }
        }
        return ItemAffix{};
    }

    bool SetAffix(ItemAffixType type, float min, float max) {
        ItemAffix *affix = 0;
        for (size_t i = 0; i < ITEM_AFFIX_MAX_COUNT; i++) {
            if (affixes[i].type == type || affixes[i].type == ItemAffix_Empty) {
                affix = &affixes[i];
                break;
            }
        }
        if (affix) {
            *affix = ItemAffix::make(type, min, max);
        }
        return affix;
    }

    bool SetAffix(ItemAffixType type, float value) {
        return SetAffix(type, value, value);
    }

    const char *Name(bool plural = false) const {
        const char *name = plural ? Proto().namePlural : Proto().nameSingular;
        return name;
    }

private:
    // TODO(dlb): Maybe cache name here if it's complicated (e.g. based on affixes)?
    //char name[64]{};
};

struct ItemDatabase {
    ItemDatabase(void) {
        // Reserve static default items for each item type
        items.resize(ItemType_Count);
        for (ItemType itemType = 0; itemType < ItemType_Count; itemType++) {
            Item &item = items[itemType];
            item.uid = itemType;
            item.type = itemType;
            byUid[item.uid] = itemType;
        }
    }

    ItemUID Spawn(ItemType type) {
        DLB_ASSERT(g_clock.server);

        if (!nextUid && g_clock.server) {
            nextUid = 9999;
        }
        nextUid = MAX(ItemType_Count, nextUid + 1); // Prevent reserved IDs from being used on overflow

        const ItemProto &proto = g_item_catalog.FindProto(type);
        if (proto.IsScalar()) {
            //printf("Reusing memoized scalar item for type: %d\n", type);
            DLB_ASSERT(items[type].type == type);
            DLB_ASSERT(items[type].uid == type);
            return type;
        } else {
            //printf("Rolling new item for type: %d\n", type);
            const Item &item = items.emplace_back(nextUid, type);
            byUid[item.uid] = (uint32_t)items.size() - 1;
            return item.uid;
        }
    }

    const Item& Find(ItemUID uid) {
        const auto &iter = byUid.find(uid);
        if (iter != byUid.end()) {
            return items[iter->second];
        }
        return items[0];
    }
private:
    uint32_t nextUid = 0;
    std::vector<Item> items{};
    std::unordered_map<ItemUID, uint32_t> byUid{};  // map of item.uid -> items[] index
};

thread_local ItemDatabase g_item_db{};

// TODO: Rename to ItemStack after done refactoring
struct ItemStack {
    ItemUID  uid   {};
    uint32_t count {};
};