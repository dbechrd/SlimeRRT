#pragma once

#include "csv.h"
#include "imgui/imgui.h"
#include <dlb_murmur3.h>
#include <raylib/raylib.h>
#include <array>
#include <unordered_map>

typedef uint16_t ItemType;
typedef uint8_t  ItemClass;
typedef uint8_t  ItemAffixType;

#define ITEM_AFFIX_MAX_COUNT 32
#define ITEM_NAME_MAX_LENGTH 32
#define ITEMTYPE_EMPTY       0
#define ITEMTYPE_COUNT       256
#define ITEMCLASS_COUNT      256

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

struct ItemAffix {
    ItemAffixType type {};
    float         min  {};
    float         max  {};

    static ItemAffix make(ItemAffixType type, float value) {
        ItemAffix affix{};
        affix.type = type;
        affix.min = value;
        affix.max = value;
        return affix;
    }

    static ItemAffix make(ItemAffixType type, float min, float max) {
        ItemAffix affix{};
        affix.type = type;
        affix.min = min;
        affix.max = max;
        return affix;
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

namespace Catalog {
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
            case ItemClass_Weapon   : return GRAY    ;
            case ItemClass_Armor    : return GRAY    ;
            case ItemClass_Shield   : return GRAY    ;
            case ItemClass_Plant    : return GREEN   ;
            case ItemClass_Book     : return BROWN   ;
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

    // TODO: Separate this out into "ItemPrototype" and "RolledItem" (figure out better names)
    struct Item {
        ItemType  itemType     {};
        ItemClass itemClass    {};
        uint8_t   stackLimit   {};
        // TODO: ENSURE NO DUPES (but without hash table or O(n) storage, i.e. use addAffix method w/ check and
        //       removeAffix to keep tightly packed)
        ItemAffix affixes      [ITEM_AFFIX_MAX_COUNT]{};
        char      nameSingular [ITEM_NAME_MAX_LENGTH]{};
        char      namePlural   [ITEM_NAME_MAX_LENGTH]{};

        ItemAffix findAffix(ItemAffixType type) const {
            for (size_t i = 0; i < ITEM_AFFIX_MAX_COUNT; i++) {
                if (affixes[i].type == type) {
                    return affixes[i];
                }
            }
            return ItemAffix{};
        }

        bool setAffix(ItemAffixType type, float min, float max) {
            ItemAffix *itemAffix = 0;
            for (size_t i = 0; i < ITEM_AFFIX_MAX_COUNT; i++) {
                if (affixes[i].type == type || affixes[i].type == ItemAffix_Empty) {
                    itemAffix = &affixes[i];
                    break;
                }
            }
            if (itemAffix) {
                *itemAffix = ItemAffix::make(type, min, max);
            }
            return itemAffix;
        }

        bool setAffix(ItemAffixType type, float value) {
            return setAffix(type, value, value);
        }
    };

    struct Items {
        void        LoadTextures (void);
        void        LoadData     (void);
        Item &      Find         (ItemType id);
        Texture     Tex          (void) const;
        const Item *Data         (void) const;

    private:
        Texture tex  {};
        CSV     csv  {};
        Item    byId [ITEMTYPE_COUNT]{};
    } g_items;
}