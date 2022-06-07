#pragma once

#include "csv.h"
#include "imgui/imgui.h"
#include <dlb_murmur3.h>
#include <raylib/raylib.h>
#include <array>
#include <unordered_map>

namespace Catalog {
    enum class ItemID {
        Empty,
        Unknown,
        Currency_Copper,
        Currency_Silver,
        Currency_Gilded,
        Orig_Gem_Ruby,
        Orig_Gem_Emerald,
        Orig_Gem_Saphire,
        Orig_Gem_Diamond,
        Orig_Gem_GoldenChest,
        Orig_Unused_10,
        Orig_Unused_11,
        Orig_Unused_12,

        // TODO: Better names: https://5.imimg.com/data5/TX/SL/MY-2530321/gemstones-1000x1000.jpg
        Ring_Stone,
        Ring_Bone,
        Ring_Obsidian,
        Ring_Zebra,
        Ring_Rust,
        Ring_BlueSteel,
        Ring_Jade,
        Amulet_Silver_Topaz,
        Amulet_Silver_Saphire,
        Amulet_Silver_Emerald,
        Amulet_Silver_Ruby,
        Amulet_Silver_Amethyst,
        Amulet_Silver_Diamond,

        Ring_Silver_Topaz,
        Ring_Silver_Saphire,
        Ring_Silver_Emerald,
        Ring_Silver_Ruby,
        Ring_Silver_Amethyst,
        Ring_Silver_Diamond,
        Ring_Silver_Plain,
        Amulet_Gold_Topaz,
        Amulet_Gold_Saphire,
        Amulet_Gold_Emerald,
        Amulet_Gold_Ruby,
        Amulet_Gold_Amethyst,
        Amulet_Gold_Diamond,

        Ring_Gold_Topaz,
        Ring_Gold_Saphire,
        Ring_Gold_Emerald,
        Ring_Gold_Ruby,
        Ring_Gold_Amethyst,
        Ring_Gold_Diamond,
        Ring_Gold_Plain,
        Crown_Gold_Spitz,
        Crown_Gold_Spitz_Ruby,
        Crown_Gold_Spitz_Velvet,
        Crown_Silver_Circlet,
        Crown_Gold_Circlet,
        Key_Horrendously_Awful,

        Coin_Copper,
        Gem_Saphire,
        Gem_Emerald,  // Olivine? search other "minerals"
        Gem_Ruby,
        Gem_Amethyst,
        Gem_Diamond,
        Gem_Pink_Himalayan_Salt,
        Coin_Gold_Stack,
        Ore_Gold,
        Item_Scroll,
        Item_Red_Cape,
        Key_Gold,
        Key_Less_Awful,

        Ore_Limonite,
        Ore_Copper,
        Ore_Silver,
        Ore_Virilium,
        Ore_Steel,
        Ore_Cerulean,
        Ore_Tungsten,
        Potion_White,
        Potion_Black,
        Potion_Purple,
        Potion_Green,
        Potion_Red,
        Potion_Blue,

        Ingot_Limonite,
        Ingot_Copper,
        Ingot_Silver,
        Ingot_Virilium,
        Ingot_Steel,
        Ingot_Cerulean,
        Ingot_Tungsten,
        Potion_Brown,
        Potion_Turquoise,
        Potion_Yellow,
        Potion_Orange,
        Potion_Pink,
        Potion_Ornament, // or change color into mystery potion? idk.. has no outline

        Nugget_Silver,
        Nugget_Gold,
        Item_Logs,
        Item_Rope,
        Tool_Shovel,
        Tool_Pickaxe,
        Tool_Mallet,
        Item_Log,
        Weapon_Great_Axe,
        Tool_Hatchet,
        Weapon_Winged_Axe,
        Weapon_Labrys,
        Tool_Scythe,

        Weapon_Dagger,
        Weapon_Long_Sword,
        Weapon_Jagged_Long_Sword,
        Weapon_Short_Sword,
        Weapon_Double_Axe,
        Weapon_Spear,
        Weapon_Mace,
        Weapon_Machete,
        Weapon_Slicer,
        Weapon_Great_Blade,
        Weapon_Great_Slicer,
        Weapon_Great_Tungsten_Sword,
        Weapon_Excalibur,

        Weapon_Lightsabre,
        Weapon_Serated_Edge,
        Weapon_Noble_Serated_Edge,
        Weapon_Gemmed_Long_Sword,
        Weapon_Gemmed_Double_Blade,
        Weapon_Tungsten_Spear,
        Weapon_Bludgeoner,
        Weapon_Hammer,
        Weapon_Round_Hammer,
        Weapon_Conic_Hammer,
        Weapon_Short_Slicer,
        Weapon_Tungsten_Sword,
        Weapon_Tungsten_Dagger,

        Armor_Helmet_Plain,
        Armor_Helmet_Neck,
        Armor_Helmet_Short,
        Armor_Helmet_Grill,
        Armor_Helmet_Galea,
        Armor_Helmet_Cerulean,
        Armor_Helmet_Winged,
        Armor_Helmet_Galea_Angled,
        Armor_Helmet_Leather,
        Armor_Shirt,
        Armor_Glove,
        Weapon_Bow,
        Item_Skull_And_Crossbones,

        Armor_Tungsten_Helmet_Winged,
        Armor_Tungsten_Chestplate,
        Armor_Tungsten_Boots,
        Armor_Tungsten_Helmet_Spiked,
        Armor_Tungsten_Helmet_BigBoy,
        Armor_Leather_Boots,
        Armor_Leather_Booties,
        Armor_Leather_Sandals,
        Armor_Black_Leather_Boots,
        Armor_Black_Leather_Booties,
        Armor_Tungsten_Gloves,
        Item_Rock,
        Item_Paper,

        Weapon_Staff_Wooden,
        Weapon_Staff_Scepter,
        Weapon_Staff_Orb,
        Weapon_Staff_Noble,
        Weapon_Staff_Enchanted,
        Shield,
        Shield_Round_Reinforced,
        Shield_Round,
        Shield_Royal,
        Shield_Holy,
        Item_Empty_Space,
        Item_Worm,
        Item_Dice,

        Plant_TreeBush,
        Plant_LeafLarge,
        Plant_Lily,
        Plant_Seaweed,
        Plant_Mushroom,
        Plant_Seagrass,
        Plant_Berries,
        Book_Green,
        Book_Red,
        Book_Blue,
        Book_White,
        Book_Brown,
        Book_RedHeart,

        Plant_FishEggs,
        Plant_LeafSmall,
        Plant_Leaves,
        Plant_BlueFlower,
        Plant_SpottedMushroom,
        Plant_Rose,
        Plant_Geranium,
        Book_Purple,
        Book_Yellow,
        Book_Turquoise,
        Book_Black,
        Book_BlackSkull,
        Book_BlueShield,

        Count
    };

    enum class ItemType {
        Empty,
        System,
        Currency,
        Gem,
        Ring,
        Amulet,
        Crown,
        Key,
        Coin,
        Ore,
        Item,
        Potion,
        Ingot,
        Nugget,
        Tool,
        Weapon,
        Armor,
        Shield,
        Plant,
        Book,
        Count
    };

    Color ItemTypeToColor(ItemType itemType)
    {
        switch (itemType) {
            case ItemType::Empty    : return DARKGRAY;
            case ItemType::System   : return PINK    ;
            case ItemType::Currency : return GOLD    ;
            case ItemType::Gem      : return BLUE    ;
            case ItemType::Ring     : return BLUE    ;
            case ItemType::Amulet   : return BLUE    ;
            case ItemType::Crown    : return BLUE    ;
            case ItemType::Key      : return YELLOW  ;
            case ItemType::Coin     : return GOLD    ;
            case ItemType::Ore      : return ORANGE  ;
            case ItemType::Item     : return WHITE   ;
            case ItemType::Potion   : return PURPLE  ;
            case ItemType::Ingot    : return ORANGE  ;
            case ItemType::Nugget   : return ORANGE  ;
            case ItemType::Tool     : return GRAY    ;
            case ItemType::Weapon   : return GRAY    ;
            case ItemType::Armor    : return GRAY    ;
            case ItemType::Shield   : return GRAY    ;
            case ItemType::Plant    : return GREEN   ;
            case ItemType::Book     : return BROWN   ;
            default                 : return MAGENTA ;
        }
    }

    ImVec4 RayToImColor(Color color)
    {
        ImColor imColor{ color.r, color.g, color.b, color.a };
        return imColor.Value;
    }

    const char *ItemTypeToString(ItemType itemType)
    {
        switch (itemType) {
            case ItemType::Empty    : return "Empty"     ;
            case ItemType::System   : return "System"    ;
            case ItemType::Currency : return "Currency"  ;
            case ItemType::Gem      : return "Gem"       ;
            case ItemType::Ring     : return "Ring"      ;
            case ItemType::Amulet   : return "Amulet"    ;
            case ItemType::Crown    : return "Crown"     ;
            case ItemType::Key      : return "Key"       ;
            case ItemType::Coin     : return "Coin"      ;
            case ItemType::Ore      : return "Ore"       ;
            case ItemType::Item     : return "Item"      ;
            case ItemType::Potion   : return "Potion"    ;
            case ItemType::Ingot    : return "Ingot"     ;
            case ItemType::Nugget   : return "Nugget"    ;
            case ItemType::Tool     : return "Tool"      ;
            case ItemType::Weapon   : return "Weapon"    ;
            case ItemType::Armor    : return "Armor"     ;
            case ItemType::Shield   : return "Shield"    ;
            case ItemType::Plant    : return "Plant"     ;
            case ItemType::Book     : return "Book"      ;
            default                 : return "<ItemType>";
        }
    }

    ItemType ItemTypeFromString(const char *str, size_t length)
    {
        thread_local std::unordered_map<u32, ItemType> itemTypeByName{};
        if (!itemTypeByName.size()) {
            itemTypeByName[dlb_murmur3(CSTR("Empty"   ))] = ItemType::Empty;
            itemTypeByName[dlb_murmur3(CSTR("System"  ))] = ItemType::System;
            itemTypeByName[dlb_murmur3(CSTR("Currency"))] = ItemType::Currency;
            itemTypeByName[dlb_murmur3(CSTR("Gem"     ))] = ItemType::Gem;
            itemTypeByName[dlb_murmur3(CSTR("Ring"    ))] = ItemType::Ring;
            itemTypeByName[dlb_murmur3(CSTR("Amulet"  ))] = ItemType::Amulet;
            itemTypeByName[dlb_murmur3(CSTR("Crown"   ))] = ItemType::Crown;
            itemTypeByName[dlb_murmur3(CSTR("Key"     ))] = ItemType::Key;
            itemTypeByName[dlb_murmur3(CSTR("Coin"    ))] = ItemType::Coin;
            itemTypeByName[dlb_murmur3(CSTR("Ore"     ))] = ItemType::Ore;
            itemTypeByName[dlb_murmur3(CSTR("Item"    ))] = ItemType::Item;
            itemTypeByName[dlb_murmur3(CSTR("Potion"  ))] = ItemType::Potion;
            itemTypeByName[dlb_murmur3(CSTR("Ingot"   ))] = ItemType::Ingot;
            itemTypeByName[dlb_murmur3(CSTR("Nugget"  ))] = ItemType::Nugget;
            itemTypeByName[dlb_murmur3(CSTR("Tool"    ))] = ItemType::Tool;
            itemTypeByName[dlb_murmur3(CSTR("Weapon"  ))] = ItemType::Weapon;
            itemTypeByName[dlb_murmur3(CSTR("Armor"   ))] = ItemType::Armor;
            itemTypeByName[dlb_murmur3(CSTR("Shield"  ))] = ItemType::Shield;
            itemTypeByName[dlb_murmur3(CSTR("Plant"   ))] = ItemType::Plant;
            itemTypeByName[dlb_murmur3(CSTR("Book"    ))] = ItemType::Book;
        }

        Catalog::ItemType type = ItemType::Empty;
        u32 hashCode = dlb_murmur3(str, length);
        auto kv = itemTypeByName.find(hashCode);
        if (kv != itemTypeByName.end()) {
            type = kv->second;
        }
        return type;
    }

    struct Item {
        ItemID      id           {};
        ItemType    type         {};
        uint32_t    stackLimit   {};
        float       value        {};
        float       damage       {};
        const char *nameSingular {};
        const char *namePlural   {};
    };

    struct Items {
        void  LoadTextures(void);
        void  LoadData(void);
        const Item &FindById(ItemID id) const;
        const Texture Tex(void) const;

    private:
        Texture tex {};
        CSV csv {};
        std::array<Item, (size_t)ItemID::Count> byId {};
    } g_items;
}