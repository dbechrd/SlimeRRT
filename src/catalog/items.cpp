#include "items.h"

namespace Catalog {
    void Items::Load(void)
    {
        if (IsWindowReady()) {
            tex = LoadTexture("resources/joecreates.png");
        }

        // TODO: Load from file (.csv?)
        // Row 1
        byId[(size_t)ItemID::Empty                        ] = { ItemID::Empty                        , ItemType::Empty    ,  1, 0.00f, 0.0f, "Empty"                     , "Empty"                      };
        byId[(size_t)ItemID::Unknown                      ] = { ItemID::Unknown                      , ItemType::System   ,  1, 0.00f, 0.0f, "_Sys_Unknown_"             , "_Sys_Unknown_"              };
        byId[(size_t)ItemID::Currency_Copper              ] = { ItemID::Currency_Copper              , ItemType::Currency ,  9, 0.01f, 0.0f, "Alloyed Piece"             , "Alloyed Pieces"             };
        byId[(size_t)ItemID::Currency_Silver              ] = { ItemID::Currency_Silver              , ItemType::Currency ,  9, 0.10f, 0.0f, "Plated Piece"              , "Plated Pieces"              };
        byId[(size_t)ItemID::Currency_Gilded              ] = { ItemID::Currency_Gilded              , ItemType::Currency ,  9, 1.00f, 0.0f, "Gilded Piece"              , "Gilded Pieces"              };
        byId[(size_t)ItemID::Orig_Gem_Ruby                ] = { ItemID::Orig_Gem_Ruby                , ItemType::Gem      , 50, 0.00f, 0.0f, "Ruby"                      , "Rubies"                     };
        byId[(size_t)ItemID::Orig_Gem_Emerald             ] = { ItemID::Orig_Gem_Emerald             , ItemType::Gem      , 50, 0.00f, 0.0f, "Emerald"                   , "Emeralds"                   };
        byId[(size_t)ItemID::Orig_Gem_Saphire             ] = { ItemID::Orig_Gem_Saphire             , ItemType::Gem      , 50, 0.00f, 0.0f, "Saphire"                   , "Saphires"                   };
        byId[(size_t)ItemID::Orig_Gem_Diamond             ] = { ItemID::Orig_Gem_Diamond             , ItemType::Gem      , 50, 0.00f, 0.0f, "Diamond"                   , "Diamonds"                   };
        byId[(size_t)ItemID::Orig_Gem_GoldenChest         ] = { ItemID::Orig_Gem_GoldenChest         , ItemType::Gem      , 50, 0.00f, 0.0f, "Golden Chest"              , "Golden Chests"              };
        byId[(size_t)ItemID::Orig_Unused_10               ] = { ItemID::Orig_Unused_10               , ItemType::System   ,  1, 0.00f, 0.0f, "_unused_10_"               , "_unused_10_"                };
        byId[(size_t)ItemID::Orig_Unused_11               ] = { ItemID::Orig_Unused_11               , ItemType::System   ,  1, 0.00f, 0.0f, "_unused_11_"               , "_unused_11_"                };
        byId[(size_t)ItemID::Orig_Unused_12               ] = { ItemID::Orig_Unused_12               , ItemType::System   ,  1, 0.00f, 0.0f, "_unused_12_"               , "_unused_12_"                };
        // Row 2
        byId[(size_t)ItemID::Ring_Stone                   ] = { ItemID::Ring_Stone                   , ItemType::Ring     ,  1, 0.00f, 0.0f, "Tungsten Ring"             , "Tungsten Rings"             };
        byId[(size_t)ItemID::Ring_Bone                    ] = { ItemID::Ring_Bone                    , ItemType::Ring     ,  1, 0.00f, 0.0f, "Bone Ring"                 , "Bone Rings"                 };
        byId[(size_t)ItemID::Ring_Obsidian                ] = { ItemID::Ring_Obsidian                , ItemType::Ring     ,  1, 0.00f, 0.0f, "Obsidian Ring"             , "Obsidian Rings"             };
        byId[(size_t)ItemID::Ring_Zebra                   ] = { ItemID::Ring_Zebra                   , ItemType::Ring     ,  1, 0.00f, 0.0f, "Zebra Ring"                , "Zebra Rings"                };
        byId[(size_t)ItemID::Ring_Rust                    ] = { ItemID::Ring_Rust                    , ItemType::Ring     ,  1, 0.00f, 0.0f, "Rusty Ring"                , "Rusty Rings"                };
        byId[(size_t)ItemID::Ring_BlueSteel               ] = { ItemID::Ring_BlueSteel               , ItemType::Ring     ,  1, 0.00f, 0.0f, "Steel Ring"                , "Steel Rings"                };
        byId[(size_t)ItemID::Ring_Jade                    ] = { ItemID::Ring_Jade                    , ItemType::Ring     ,  1, 0.00f, 0.0f, "Jade Ring"                 , "Jade Rings"                 };
        byId[(size_t)ItemID::Amulet_Silver_Topaz          ] = { ItemID::Amulet_Silver_Topaz          , ItemType::Amulet   ,  1, 0.00f, 0.0f, "Silver Amulet of Topaz"    , "Silver Amulets of Topaz"    };
        byId[(size_t)ItemID::Amulet_Silver_Saphire        ] = { ItemID::Amulet_Silver_Saphire        , ItemType::Amulet   ,  1, 0.00f, 0.0f, "Silver Amulet of Saphire"  , "Silver Amulets of Saphire"  };
        byId[(size_t)ItemID::Amulet_Silver_Emerald        ] = { ItemID::Amulet_Silver_Emerald        , ItemType::Amulet   ,  1, 0.00f, 0.0f, "Silver Amulet of Emerald"  , "Silver Amulets of Emerald"  };
        byId[(size_t)ItemID::Amulet_Silver_Ruby           ] = { ItemID::Amulet_Silver_Ruby           , ItemType::Amulet   ,  1, 0.00f, 0.0f, "Silver Amulet of Ruby"     , "Silver Amulets of Ruby"     };
        byId[(size_t)ItemID::Amulet_Silver_Amethyst       ] = { ItemID::Amulet_Silver_Amethyst       , ItemType::Amulet   ,  1, 0.00f, 0.0f, "Silver Amulet of Amethyst" , "Silver Amulets of Amethyst" };
        byId[(size_t)ItemID::Amulet_Silver_Diamond        ] = { ItemID::Amulet_Silver_Diamond        , ItemType::Amulet   ,  1, 0.00f, 0.0f, "Silver Amulet of Diamond"  , "Silver Amulets of Diamond"  };
        // Row 3
        byId[(size_t)ItemID::Ring_Silver_Topaz            ] = { ItemID::Ring_Silver_Topaz            , ItemType::Ring     ,  1, 0.00f, 0.0f, "Silver Ring of Topaz"      , "Silver Rings of Topaz"      };
        byId[(size_t)ItemID::Ring_Silver_Saphire          ] = { ItemID::Ring_Silver_Saphire          , ItemType::Ring     ,  1, 0.00f, 0.0f, "Silver Ring of Saphire"    , "Silver Rings of Saphire"    };
        byId[(size_t)ItemID::Ring_Silver_Emerald          ] = { ItemID::Ring_Silver_Emerald          , ItemType::Ring     ,  1, 0.00f, 0.0f, "Silver Ring of Emerald"    , "Silver Rings of Emerald"    };
        byId[(size_t)ItemID::Ring_Silver_Ruby             ] = { ItemID::Ring_Silver_Ruby             , ItemType::Ring     ,  1, 0.00f, 0.0f, "Silver Ring of Ruby"       , "Silver Rings of Ruby"       };
        byId[(size_t)ItemID::Ring_Silver_Amethyst         ] = { ItemID::Ring_Silver_Amethyst         , ItemType::Ring     ,  1, 0.00f, 0.0f, "Silver Ring of Amethyst"   , "Silver Rings of Amethyst"   };
        byId[(size_t)ItemID::Ring_Silver_Diamond          ] = { ItemID::Ring_Silver_Diamond          , ItemType::Ring     ,  1, 0.00f, 0.0f, "Silver Ring of Diamond"    , "Silver Rings of Diamond"    };
        byId[(size_t)ItemID::Ring_Silver_Plain            ] = { ItemID::Ring_Silver_Plain            , ItemType::Ring     ,  1, 0.00f, 0.0f, "Silver Ring"               , "Silver Rings"               };
        byId[(size_t)ItemID::Amulet_Gold_Topaz            ] = { ItemID::Amulet_Gold_Topaz            , ItemType::Amulet   ,  1, 0.00f, 0.0f, "Gold Amulet of Topaz"      , "Gold Amulets of Topaz"      };
        byId[(size_t)ItemID::Amulet_Gold_Saphire          ] = { ItemID::Amulet_Gold_Saphire          , ItemType::Amulet   ,  1, 0.00f, 0.0f, "Gold Amulet of Saphire"    , "Gold Amulets of Saphire"    };
        byId[(size_t)ItemID::Amulet_Gold_Emerald          ] = { ItemID::Amulet_Gold_Emerald          , ItemType::Amulet   ,  1, 0.00f, 0.0f, "Gold Amulet of Emerald"    , "Gold Amulets of Emerald"    };
        byId[(size_t)ItemID::Amulet_Gold_Ruby             ] = { ItemID::Amulet_Gold_Ruby             , ItemType::Amulet   ,  1, 0.00f, 0.0f, "Gold Amulet of Ruby"       , "Gold Amulets of Ruby"       };
        byId[(size_t)ItemID::Amulet_Gold_Amethyst         ] = { ItemID::Amulet_Gold_Amethyst         , ItemType::Amulet   ,  1, 0.00f, 0.0f, "Gold Amulet of Amethyst"   , "Gold Amulets of Amethyst"   };
        byId[(size_t)ItemID::Amulet_Gold_Diamond          ] = { ItemID::Amulet_Gold_Diamond          , ItemType::Amulet   ,  1, 0.00f, 0.0f, "Gold Amulet of Diamond"    , "Gold Amulets of Diamond"    };
        // Row 4
        byId[(size_t)ItemID::Ring_Gold_Topaz              ] = { ItemID::Ring_Gold_Topaz              , ItemType::Ring     ,  1, 0.00f, 0.0f, "Gold Ring of Topaz"        , "Gold Rings of Topaz"        };
        byId[(size_t)ItemID::Ring_Gold_Saphire            ] = { ItemID::Ring_Gold_Saphire            , ItemType::Ring     ,  1, 0.00f, 0.0f, "Gold Ring of Saphire"      , "Gold Rings of Saphire"      };
        byId[(size_t)ItemID::Ring_Gold_Emerald            ] = { ItemID::Ring_Gold_Emerald            , ItemType::Ring     ,  1, 0.00f, 0.0f, "Gold Ring of Emerald"      , "Gold Rings of Emerald"      };
        byId[(size_t)ItemID::Ring_Gold_Ruby               ] = { ItemID::Ring_Gold_Ruby               , ItemType::Ring     ,  1, 0.00f, 0.0f, "Gold Ring of Ruby"         , "Gold Rings of Ruby"         };
        byId[(size_t)ItemID::Ring_Gold_Amethyst           ] = { ItemID::Ring_Gold_Amethyst           , ItemType::Ring     ,  1, 0.00f, 0.0f, "Gold Ring of Amethyst"     , "Gold Rings of Amethyst"     };
        byId[(size_t)ItemID::Ring_Gold_Diamond            ] = { ItemID::Ring_Gold_Diamond            , ItemType::Ring     ,  1, 0.00f, 0.0f, "Gold Ring of Diamond"      , "Gold Rings of Diamond"      };
        byId[(size_t)ItemID::Ring_Gold_Plain              ] = { ItemID::Ring_Gold_Plain              , ItemType::Ring     ,  1, 0.00f, 0.0f, "Gold Ring"                 , "Gold Rings"                 };
        byId[(size_t)ItemID::Crown_Gold_Spitz             ] = { ItemID::Crown_Gold_Spitz             , ItemType::Crown    ,  1, 0.00f, 0.0f, "Gold Spiked Crown"         , "Gold Spiked Crowns"         };
        byId[(size_t)ItemID::Crown_Gold_Spitz_Ruby        ] = { ItemID::Crown_Gold_Spitz_Ruby        , ItemType::Crown    ,  1, 0.00f, 0.0f, "Gold Spiked Crown of Ruby" , "Gold Spiked Crowns of Ruby" };
        byId[(size_t)ItemID::Crown_Gold_Spitz_Velvet      ] = { ItemID::Crown_Gold_Spitz_Velvet      , ItemType::Crown    ,  1, 0.00f, 0.0f, "Gold Velvet Crown"         , "Gold Velvet Crowns"         };
        byId[(size_t)ItemID::Crown_Silver_Circlet         ] = { ItemID::Crown_Silver_Circlet         , ItemType::Crown    ,  1, 0.00f, 0.0f, "Silver Circlet"            , "Silver Circlets"            };
        byId[(size_t)ItemID::Crown_Gold_Circlet           ] = { ItemID::Crown_Gold_Circlet           , ItemType::Crown    ,  1, 0.00f, 0.0f, "Gold Circlet"              , "Gold Circlets"              };
        byId[(size_t)ItemID::Key_Horrendously_Awful       ] = { ItemID::Key_Horrendously_Awful       , ItemType::Key      ,  9, 0.00f, 0.0f, "Horrendously Awful Key"    , "Horrendously Awful Keys"    };
        // Row 5
        byId[(size_t)ItemID::Coin_Copper                  ] = { ItemID::Coin_Copper                  , ItemType::Coin     , 99, 0.00f, 0.0f, "Copper Coin"               , "Copper Coins"               };
        byId[(size_t)ItemID::Gem_Saphire                  ] = { ItemID::Gem_Saphire                  , ItemType::Gem      , 50, 0.00f, 0.0f, "Saphire"                   , "Saphires"                   };
        byId[(size_t)ItemID::Gem_Emerald                  ] = { ItemID::Gem_Emerald                  , ItemType::Gem      , 50, 0.00f, 0.0f, "Emerald"                   , "Emeralds"                   };
        byId[(size_t)ItemID::Gem_Ruby                     ] = { ItemID::Gem_Ruby                     , ItemType::Gem      , 50, 0.00f, 0.0f, "Ruby"                      , "Rubies"                     };
        byId[(size_t)ItemID::Gem_Amethyst                 ] = { ItemID::Gem_Amethyst                 , ItemType::Gem      , 50, 0.00f, 0.0f, "Amethyst"                  , "Amethysts"                  };
        byId[(size_t)ItemID::Gem_Diamond                  ] = { ItemID::Gem_Diamond                  , ItemType::Gem      , 50, 0.00f, 0.0f, "Diamond"                   , "Diamonds"                   };
        byId[(size_t)ItemID::Gem_Pink_Himalayan_Salt      ] = { ItemID::Gem_Pink_Himalayan_Salt      , ItemType::Gem      , 50, 0.00f, 0.0f, "Pink Himalayan Salt"       , "Pink Himalayan Salts"       };
        byId[(size_t)ItemID::Coin_Gold_Stack              ] = { ItemID::Coin_Gold_Stack              , ItemType::Coin     ,100, 0.00f, 0.0f, "Stack of Coins"            , "Stacks of Coins"            };
        byId[(size_t)ItemID::Ore_Gold                     ] = { ItemID::Ore_Gold                     , ItemType::Ore      ,100, 0.00f, 0.0f, "Gold Ore"                  , "Gold Ores"                  };
        byId[(size_t)ItemID::Item_Scroll                  ] = { ItemID::Item_Scroll                  , ItemType::Item     ,  1, 0.00f, 0.0f, "Scroll of Wisdom"          , "Scrolls of Wisdom"          };
        byId[(size_t)ItemID::Item_Red_Cape                ] = { ItemID::Item_Red_Cape                , ItemType::Item     ,  1, 0.00f, 0.0f, "Red Cape"                  , "Red Capes"                  };
        byId[(size_t)ItemID::Key_Gold                     ] = { ItemID::Key_Gold                     , ItemType::Key      ,  9, 0.00f, 0.0f, "Key"                       , "Keys"                       };
        byId[(size_t)ItemID::Key_Less_Awful               ] = { ItemID::Key_Less_Awful               , ItemType::Key      ,  9, 0.00f, 0.0f, "Mangled Key"               , "Mangled Keys"               };
        // Row 6
        byId[(size_t)ItemID::Ore_Limonite                 ] = { ItemID::Ore_Limonite                 , ItemType::Ore      ,100, 0.00f, 0.0f, "Limonite Ore"              , "Limonite Ores"              };
        byId[(size_t)ItemID::Ore_Copper                   ] = { ItemID::Ore_Copper                   , ItemType::Ore      ,100, 0.00f, 0.0f, "Copper Ore"                , "Copper Ores"                };
        byId[(size_t)ItemID::Ore_Silver                   ] = { ItemID::Ore_Silver                   , ItemType::Ore      ,100, 0.00f, 0.0f, "Silver Ore"                , "Silver Ores"                };
        byId[(size_t)ItemID::Ore_Virilium                 ] = { ItemID::Ore_Virilium                 , ItemType::Ore      ,100, 0.00f, 0.0f, "Virilium Ore"              , "Virilium Ores"              };
        byId[(size_t)ItemID::Ore_Steel                    ] = { ItemID::Ore_Steel                    , ItemType::Ore      ,100, 0.00f, 0.0f, "Steel Ore"                 , "Steel Ores"                 };
        byId[(size_t)ItemID::Ore_Cerulean                 ] = { ItemID::Ore_Cerulean                 , ItemType::Ore      ,100, 0.00f, 0.0f, "Cerulean Ore"              , "Cerulean Ores"              };
        byId[(size_t)ItemID::Ore_Tungsten                 ] = { ItemID::Ore_Tungsten                 , ItemType::Ore      ,100, 0.00f, 0.0f, "Tungsten Ore"              , "Tungsten Ores"              };
        byId[(size_t)ItemID::Potion_White                 ] = { ItemID::Potion_White                 , ItemType::Potion   ,  5, 0.00f, 0.0f, "White Potion"              , "White Potions"              };
        byId[(size_t)ItemID::Potion_Black                 ] = { ItemID::Potion_Black                 , ItemType::Potion   ,  5, 0.00f, 0.0f, "Black Potion"              , "Black Potions"              };
        byId[(size_t)ItemID::Potion_Purple                ] = { ItemID::Potion_Purple                , ItemType::Potion   ,  5, 0.00f, 0.0f, "Purple Potion"             , "Purple Potions"             };
        byId[(size_t)ItemID::Potion_Green                 ] = { ItemID::Potion_Green                 , ItemType::Potion   ,  5, 0.00f, 0.0f, "Green Potion"              , "Green Potions"              };
        byId[(size_t)ItemID::Potion_Red                   ] = { ItemID::Potion_Red                   , ItemType::Potion   ,  5, 0.00f, 0.0f, "Red Potion"                , "Red Potions"                };
        byId[(size_t)ItemID::Potion_Blue                  ] = { ItemID::Potion_Blue                  , ItemType::Potion   ,  5, 0.00f, 0.0f, "Blue Potion"               , "Blue Potions"               };
        // Row 7
        byId[(size_t)ItemID::Ingot_Limonite               ] = { ItemID::Ingot_Limonite               , ItemType::Ingot    ,100, 0.00f, 0.0f, "Limonite Ingot"            , "Limonite Ingots"            };
        byId[(size_t)ItemID::Ingot_Copper                 ] = { ItemID::Ingot_Copper                 , ItemType::Ingot    ,100, 0.00f, 0.0f, "Copper Ingot"              , "Copper Ingots"              };
        byId[(size_t)ItemID::Ingot_Silver                 ] = { ItemID::Ingot_Silver                 , ItemType::Ingot    ,100, 0.00f, 0.0f, "Silver Ingot"              , "Silver Ingots"              };
        byId[(size_t)ItemID::Ingot_Virilium               ] = { ItemID::Ingot_Virilium               , ItemType::Ingot    ,100, 0.00f, 0.0f, "Virilium Ingot"            , "Virilium Ingots"            };
        byId[(size_t)ItemID::Ingot_Steel                  ] = { ItemID::Ingot_Steel                  , ItemType::Ingot    ,100, 0.00f, 0.0f, "Steel Ingot"               , "Steel Ingots"               };
        byId[(size_t)ItemID::Ingot_Cerulean               ] = { ItemID::Ingot_Cerulean               , ItemType::Ingot    ,100, 0.00f, 0.0f, "Cerulean Ingot"            , "Cerulean Ingots"            };
        byId[(size_t)ItemID::Ingot_Tungsten               ] = { ItemID::Ingot_Tungsten               , ItemType::Ingot    ,100, 0.00f, 0.0f, "Tungsten Ingot"            , "Tungsten Ingots"            };
        byId[(size_t)ItemID::Potion_Brown                 ] = { ItemID::Potion_Brown                 , ItemType::Potion   ,  5, 0.00f, 0.0f, "Brown Potion"              , "Brown Potions"              };
        byId[(size_t)ItemID::Potion_Turquoise             ] = { ItemID::Potion_Turquoise             , ItemType::Potion   ,  5, 0.00f, 0.0f, "Turquoise Potion"          , "Turquoise Potions"          };
        byId[(size_t)ItemID::Potion_Yellow                ] = { ItemID::Potion_Yellow                , ItemType::Potion   ,  5, 0.00f, 0.0f, "Yellow Potion"             , "Yellow Potions"             };
        byId[(size_t)ItemID::Potion_Orange                ] = { ItemID::Potion_Orange                , ItemType::Potion   ,  5, 0.00f, 0.0f, "Orange Potion"             , "Orange Potions"             };
        byId[(size_t)ItemID::Potion_Pink                  ] = { ItemID::Potion_Pink                  , ItemType::Potion   ,  5, 0.00f, 0.0f, "Pink Potion"               , "Pink Potions"               };
        byId[(size_t)ItemID::Potion_Ornament              ] = { ItemID::Potion_Ornament              , ItemType::Potion   ,  5, 0.00f, 0.0f, "Ornamental Potion"         , "Ornamental Potions"         };
        // Row 8
        byId[(size_t)ItemID::Nugget_Silver                ] = { ItemID::Nugget_Silver                , ItemType::Nugget   ,100, 0.00f, 0.0f, "Silver Nugget"             , "Silver Nuggets"             };
        byId[(size_t)ItemID::Nugget_Gold                  ] = { ItemID::Nugget_Gold                  , ItemType::Nugget   ,100, 0.00f, 0.0f, "Gold Nugget"               , "Gold Nuggets"               };
        byId[(size_t)ItemID::Item_Logs                    ] = { ItemID::Item_Logs                    , ItemType::Item     , 50, 0.00f, 0.0f, "Stack of Logs"             , "Stacks of Logs"             };
        byId[(size_t)ItemID::Item_Rope                    ] = { ItemID::Item_Rope                    , ItemType::Item     , 50, 0.00f, 0.0f, "Coil of Rope"              , "Coils of Rope"              };
        byId[(size_t)ItemID::Tool_Shovel                  ] = { ItemID::Tool_Shovel                  , ItemType::Tool     ,  1, 0.00f, 0.0f, "Shovel"                    , "Shovels"                    };
        byId[(size_t)ItemID::Tool_Pickaxe                 ] = { ItemID::Tool_Pickaxe                 , ItemType::Tool     ,  1, 0.00f, 0.0f, "Pickaxe"                   , "Pickaxes"                   };
        byId[(size_t)ItemID::Tool_Mallet                  ] = { ItemID::Tool_Mallet                  , ItemType::Tool     ,  1, 0.00f, 0.0f, "Mallet"                    , "Mallets"                    };
        byId[(size_t)ItemID::Item_Log                     ] = { ItemID::Item_Log                     , ItemType::Item     ,  2, 0.00f, 0.0f, "Log"                       , "Logs"                       };
        byId[(size_t)ItemID::Weapon_Great_Axe             ] = { ItemID::Weapon_Great_Axe             , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Great Axe"                 , "Great Axes"                 };
        byId[(size_t)ItemID::Tool_Hatchet                 ] = { ItemID::Tool_Hatchet                 , ItemType::Tool     ,  1, 0.00f, 0.0f, "Hatchet"                   , "Hatchets"                   };
        byId[(size_t)ItemID::Weapon_Winged_Axe            ] = { ItemID::Weapon_Winged_Axe            , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Winged Axe"                , "Winged Axes"                };
        byId[(size_t)ItemID::Weapon_Labrys                ] = { ItemID::Weapon_Labrys                , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Labrys"                    , "labryes"                    };
        byId[(size_t)ItemID::Tool_Scythe                  ] = { ItemID::Tool_Scythe                  , ItemType::Tool     ,  1, 0.00f, 0.0f, "Scythe"                    , "Scythes"                    };
        // Row 9
        byId[(size_t)ItemID::Weapon_Dagger                ] = { ItemID::Weapon_Dagger                , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Dagger"                    , "Daggers"                    };
        byId[(size_t)ItemID::Weapon_Long_Sword            ] = { ItemID::Weapon_Long_Sword            , ItemType::Weapon   ,  1, 0.00f, 5.0f, "Long Sword"                , "Long Swords"                };
        byId[(size_t)ItemID::Weapon_Jagged_Long_Sword     ] = { ItemID::Weapon_Jagged_Long_Sword     , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Jagged Long Sword"         , "Jagged Long Swords"         };
        byId[(size_t)ItemID::Weapon_Short_Sword           ] = { ItemID::Weapon_Short_Sword           , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Short Sword"               , "Short Sword"                };
        byId[(size_t)ItemID::Weapon_Double_Axe            ] = { ItemID::Weapon_Double_Axe            , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Double Axe"                , "Double Axes"                };
        byId[(size_t)ItemID::Weapon_Spear                 ] = { ItemID::Weapon_Spear                 , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Spear"                     , "Spears"                     };
        byId[(size_t)ItemID::Weapon_Mace                  ] = { ItemID::Weapon_Mace                  , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Mace"                      , "Maces"                      };
        byId[(size_t)ItemID::Weapon_Machete               ] = { ItemID::Weapon_Machete               , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Machete"                   , "Machetes"                   };
        byId[(size_t)ItemID::Weapon_Slicer                ] = { ItemID::Weapon_Slicer                , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Slicer"                    , "Slicers"                    };
        byId[(size_t)ItemID::Weapon_Great_Blade           ] = { ItemID::Weapon_Great_Blade           , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Great Blade"               , "Great Blades"               };
        byId[(size_t)ItemID::Weapon_Great_Slicer          ] = { ItemID::Weapon_Great_Slicer          , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Great Slicer"              , "Great Slicers"              };
        byId[(size_t)ItemID::Weapon_Great_Tungsten_Sword  ] = { ItemID::Weapon_Great_Tungsten_Sword  , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Great Tungsten Sword"      , "Great Tungsten Swords"      };
        byId[(size_t)ItemID::Weapon_Excalibur             ] = { ItemID::Weapon_Excalibur             , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Excalibur"                 , "Excaliburs"                 };
        // Row 10
        byId[(size_t)ItemID::Weapon_Lightsabre            ] = { ItemID::Weapon_Lightsabre            , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Lightsabre"                , "Lightsabres"                };
        byId[(size_t)ItemID::Weapon_Serated_Edge          ] = { ItemID::Weapon_Serated_Edge          , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Serated Edge"              , "Serated Edges"              };
        byId[(size_t)ItemID::Weapon_Noble_Serated_Edge    ] = { ItemID::Weapon_Noble_Serated_Edge    , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Noble Serated Edge"        , "Noble Serated Edges"        };
        byId[(size_t)ItemID::Weapon_Gemmed_Long_Sword     ] = { ItemID::Weapon_Gemmed_Long_Sword     , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Gemmed Long Sword"         , "Gemmed Long Swords"         };
        byId[(size_t)ItemID::Weapon_Gemmed_Double_Blade   ] = { ItemID::Weapon_Gemmed_Double_Blade   , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Gemmed Double Blade"       , "Gemmed Double Blades"       };
        byId[(size_t)ItemID::Weapon_Tungsten_Spear        ] = { ItemID::Weapon_Tungsten_Spear        , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Tungsten Spear"            , "Tungsten Spears"            };
        byId[(size_t)ItemID::Weapon_Bludgeoner            ] = { ItemID::Weapon_Bludgeoner            , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Bludgeoner"                , "Bludgeoners"                };
        byId[(size_t)ItemID::Weapon_Hammer                ] = { ItemID::Weapon_Hammer                , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Hammer"                    , "Hammers"                    };
        byId[(size_t)ItemID::Weapon_Round_Hammer          ] = { ItemID::Weapon_Round_Hammer          , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Round Hammer"              , "Round Hammers"              };
        byId[(size_t)ItemID::Weapon_Conic_Hammer          ] = { ItemID::Weapon_Conic_Hammer          , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Conic Hammer"              , "Conic Hammers"              };
        byId[(size_t)ItemID::Weapon_Short_Slicer          ] = { ItemID::Weapon_Short_Slicer          , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Short Slicer"              , "Short Slicers"              };
        byId[(size_t)ItemID::Weapon_Tungsten_Sword        ] = { ItemID::Weapon_Tungsten_Sword        , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Tungsten Sword"            , "Tungsten Swords"            };
        byId[(size_t)ItemID::Weapon_Tungsten_Dagger       ] = { ItemID::Weapon_Tungsten_Dagger       , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Tungsten Dagger"           , "Tungsten Daggers"           };
        // Row 11
        byId[(size_t)ItemID::Armor_Helmet_Plain           ] = { ItemID::Armor_Helmet_Plain           , ItemType::Armor    ,  1, 0.00f, 0.0f, "Helmet"                    , "Helmets"                    };
        byId[(size_t)ItemID::Armor_Helmet_Neck            ] = { ItemID::Armor_Helmet_Neck            , ItemType::Armor    ,  1, 0.00f, 0.0f, "Longneck Helmet"           , "Longneck Helmets"           };
        byId[(size_t)ItemID::Armor_Helmet_Short           ] = { ItemID::Armor_Helmet_Short           , ItemType::Armor    ,  1, 0.00f, 0.0f, "Short Helmet"              , "Short Helmets"              };
        byId[(size_t)ItemID::Armor_Helmet_Grill           ] = { ItemID::Armor_Helmet_Grill           , ItemType::Armor    ,  1, 0.00f, 0.0f, "Shielded Helmet"           , "Shielded Helmets"           };
        byId[(size_t)ItemID::Armor_Helmet_Galea           ] = { ItemID::Armor_Helmet_Galea           , ItemType::Armor    ,  1, 0.00f, 0.0f, "Galea"                     , "Galeae"                     };
        byId[(size_t)ItemID::Armor_Helmet_Cerulean        ] = { ItemID::Armor_Helmet_Cerulean        , ItemType::Armor    ,  1, 0.00f, 0.0f, "Cerulean Helmet"           , "Cerulean Helmets"           };
        byId[(size_t)ItemID::Armor_Helmet_Winged          ] = { ItemID::Armor_Helmet_Winged          , ItemType::Armor    ,  1, 0.00f, 0.0f, "Winged Helmet"             , "Winged Helmets"             };
        byId[(size_t)ItemID::Armor_Helmet_Galea_Angled    ] = { ItemID::Armor_Helmet_Galea_Angled    , ItemType::Armor    ,  1, 0.00f, 0.0f, "Lesser Galea"              , "Lesser Galeae"              };
        byId[(size_t)ItemID::Armor_Helmet_Leather         ] = { ItemID::Armor_Helmet_Leather         , ItemType::Armor    ,  1, 0.00f, 0.0f, "Leather Helmet"            , "Leather Helmets"            };
        byId[(size_t)ItemID::Armor_Shirt                  ] = { ItemID::Armor_Shirt                  , ItemType::Armor    ,  1, 0.00f, 0.0f, "Shirt"                     , "Shirts"                     };
        byId[(size_t)ItemID::Armor_Glove                  ] = { ItemID::Armor_Glove                  , ItemType::Armor    ,  1, 0.00f, 0.0f, "Glove"                     , "Gloves"                     };
        byId[(size_t)ItemID::Weapon_Bow                   ] = { ItemID::Weapon_Bow                   , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Bow"                       , "Bows"                       };
        byId[(size_t)ItemID::Item_Skull_And_Crossbones    ] = { ItemID::Item_Skull_And_Crossbones    , ItemType::Item     ,  9, 0.00f, 0.0f, "Skull & Crossbones"        , "Skulls & Crossbones"        };
        // Row 12
        byId[(size_t)ItemID::Armor_Tungsten_Helmet_Winged ] = { ItemID::Armor_Tungsten_Helmet_Winged , ItemType::Armor    ,  1, 0.00f, 0.0f, "Winged Tungsten Helmet"    , "Winged Tungsten Helmets"    };
        byId[(size_t)ItemID::Armor_Tungsten_Chestplate    ] = { ItemID::Armor_Tungsten_Chestplate    , ItemType::Armor    ,  1, 0.00f, 0.0f, "Tungsten Breastplate"      , "Tungsten Breastplates"      };
        byId[(size_t)ItemID::Armor_Tungsten_Boots         ] = { ItemID::Armor_Tungsten_Boots         , ItemType::Armor    ,  1, 0.00f, 0.0f, "Tungsten Boots"            , "Tungsten Boots"             };
        byId[(size_t)ItemID::Armor_Tungsten_Helmet_Spiked ] = { ItemID::Armor_Tungsten_Helmet_Spiked , ItemType::Armor    ,  1, 0.00f, 0.0f, "Spiked Tungsten Helmet"    , "Spiked Tungsten Helmets"    };
        byId[(size_t)ItemID::Armor_Tungsten_Helmet_BigBoy ] = { ItemID::Armor_Tungsten_Helmet_BigBoy , ItemType::Armor    ,  1, 0.00f, 0.0f, "Giant Tungsten Helmet"     , "Giant Tungsten Helmets"     };
        byId[(size_t)ItemID::Armor_Leather_Boots          ] = { ItemID::Armor_Leather_Boots          , ItemType::Armor    ,  1, 0.00f, 0.0f, "Leather Boots"             , "Leather Boots"              };
        byId[(size_t)ItemID::Armor_Leather_Booties        ] = { ItemID::Armor_Leather_Booties        , ItemType::Armor    ,  1, 0.00f, 0.0f, "Leather Booties"           , "Leather Booties"            };
        byId[(size_t)ItemID::Armor_Leather_Sandals        ] = { ItemID::Armor_Leather_Sandals        , ItemType::Armor    ,  1, 0.00f, 0.0f, "Leather Sandals"           , "Leather Sandals"            };
        byId[(size_t)ItemID::Armor_Black_Leather_Boots    ] = { ItemID::Armor_Black_Leather_Boots    , ItemType::Armor    ,  1, 0.00f, 0.0f, "Black Leather Boots"       , "Black Leather Boots"        };
        byId[(size_t)ItemID::Armor_Black_Leather_Booties  ] = { ItemID::Armor_Black_Leather_Booties  , ItemType::Armor    ,  1, 0.00f, 0.0f, "Black Leather Booties"     , "Black Leather Booties"      };
        byId[(size_t)ItemID::Armor_Tungsten_Gloves        ] = { ItemID::Armor_Tungsten_Gloves        , ItemType::Armor    ,  1, 0.00f, 0.0f, "Tungsten Gloves"           , "Tungsten Gloves"            };
        byId[(size_t)ItemID::Item_Rock                    ] = { ItemID::Item_Rock                    , ItemType::Item     , 50, 0.00f, 0.0f, "Rock"                      , "Rocks"                      };
        byId[(size_t)ItemID::Item_Paper                   ] = { ItemID::Item_Paper                   , ItemType::Item     , 50, 0.00f, 0.0f, "Piece of Paper"            , "Pieces of Paper"            };
        // Row 13
        byId[(size_t)ItemID::Weapon_Staff_Wooden          ] = { ItemID::Weapon_Staff_Wooden          , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Wooden Staff"              , "Wooden Staves"              };
        byId[(size_t)ItemID::Weapon_Staff_Scepter         ] = { ItemID::Weapon_Staff_Scepter         , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Scepter"                   , "Scepters"                   };
        byId[(size_t)ItemID::Weapon_Staff_Orb             ] = { ItemID::Weapon_Staff_Orb             , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Orb"                       , "Orbs"                       };
        byId[(size_t)ItemID::Weapon_Staff_Noble           ] = { ItemID::Weapon_Staff_Noble           , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Noble Staff"               , "Noble Staves"               };
        byId[(size_t)ItemID::Weapon_Staff_Enchanted       ] = { ItemID::Weapon_Staff_Enchanted       , ItemType::Weapon   ,  1, 0.00f, 0.0f, "Enchanted Staff"           , "Enchanted Staves"           };
        byId[(size_t)ItemID::Shield                       ] = { ItemID::Shield                       , ItemType::Shield   ,  1, 0.00f, 0.0f, "Shield"                    , "Shields"                    };
        byId[(size_t)ItemID::Shield_Round_Reinforced      ] = { ItemID::Shield_Round_Reinforced      , ItemType::Shield   ,  1, 0.00f, 0.0f, "Reinforced Round Shield"   , "Reinforced Round Shields"   };
        byId[(size_t)ItemID::Shield_Round                 ] = { ItemID::Shield_Round                 , ItemType::Shield   ,  1, 0.00f, 0.0f, "Round Shield"              , "Round Shields"              };
        byId[(size_t)ItemID::Shield_Royal                 ] = { ItemID::Shield_Royal                 , ItemType::Shield   ,  1, 0.00f, 0.0f, "Royal Shield"              , "Royal Shields"              };
        byId[(size_t)ItemID::Shield_Holy                  ] = { ItemID::Shield_Holy                  , ItemType::Shield   ,  1, 0.00f, 0.0f, "Holy Shield"               , "Holy Shields"               };
        byId[(size_t)ItemID::Item_Empty_Space             ] = { ItemID::Item_Empty_Space             , ItemType::Item     ,  1, 0.00f, 0.0f, "_empty_space_"             , "_empty_space_"              };
        byId[(size_t)ItemID::Item_Worm                    ] = { ItemID::Item_Worm                    , ItemType::Item     ,  9, 0.00f, 0.0f, "Worm"                      , "Worms"                      };
        byId[(size_t)ItemID::Item_Dice                    ] = { ItemID::Item_Dice                    , ItemType::Item     ,  9, 0.00f, 0.0f, "Pair of Dice"              , "Pairs of Dice"              };
        // Row 14
        byId[(size_t)ItemID::Plant_TreeBush               ] = { ItemID::Plant_TreeBush               , ItemType::Plant    , 50, 0.00f, 0.0f, "Bush"                      , "Bushes"                     };
        byId[(size_t)ItemID::Plant_LeafLarge              ] = { ItemID::Plant_LeafLarge              , ItemType::Plant    , 50, 0.00f, 0.0f, "Large Leaf"                , "Large Leaves"               };
        byId[(size_t)ItemID::Plant_Lily                   ] = { ItemID::Plant_Lily                   , ItemType::Plant    , 50, 0.00f, 0.0f, "Lily"                      , "Lilies"                     };
        byId[(size_t)ItemID::Plant_Seaweed                ] = { ItemID::Plant_Seaweed                , ItemType::Plant    , 50, 0.00f, 0.0f, "Seaweed"                   , "Seaweed"                    };
        byId[(size_t)ItemID::Plant_Mushroom               ] = { ItemID::Plant_Mushroom               , ItemType::Plant    , 50, 0.00f, 0.0f, "Mushroom"                  , "Mushrooms"                  };
        byId[(size_t)ItemID::Plant_Seagrass               ] = { ItemID::Plant_Seagrass               , ItemType::Plant    , 50, 0.00f, 0.0f, "Seagrass"                  , "Seagrass"                   };
        byId[(size_t)ItemID::Plant_Berries                ] = { ItemID::Plant_Berries                , ItemType::Plant    , 50, 0.00f, 0.0f, "Berries"                   , "Berries"                    };
        byId[(size_t)ItemID::Book_Green                   ] = { ItemID::Book_Green                   , ItemType::Book     ,  5, 0.00f, 0.0f, "Green Book"                , "Green Books"                };
        byId[(size_t)ItemID::Book_Red                     ] = { ItemID::Book_Red                     , ItemType::Book     ,  5, 0.00f, 0.0f, "Red Book"                  , "Red Books"                  };
        byId[(size_t)ItemID::Book_Blue                    ] = { ItemID::Book_Blue                    , ItemType::Book     ,  5, 0.00f, 0.0f, "Blue Book"                 , "Blue Books"                 };
        byId[(size_t)ItemID::Book_White                   ] = { ItemID::Book_White                   , ItemType::Book     ,  5, 0.00f, 0.0f, "White Book"                , "White Books"                };
        byId[(size_t)ItemID::Book_Brown                   ] = { ItemID::Book_Brown                   , ItemType::Book     ,  5, 0.00f, 0.0f, "Brown Book"                , "Brown Books"                };
        byId[(size_t)ItemID::Book_RedHeart                ] = { ItemID::Book_RedHeart                , ItemType::Book     ,  5, 0.00f, 0.0f, "Red Book of Hearts"        , "Red Books of Hearts"        };
        // Row 15
        byId[(size_t)ItemID::Plant_FishEggs               ] = { ItemID::Plant_FishEggs               , ItemType::Plant    , 50, 0.00f, 0.0f, "Fish Eggs"                 , "Fish Eggs"                  };
        byId[(size_t)ItemID::Plant_LeafSmall              ] = { ItemID::Plant_LeafSmall              , ItemType::Plant    , 50, 0.00f, 0.0f, "Small Leaf"                , "Small Leaves"               };
        byId[(size_t)ItemID::Plant_Leaves                 ] = { ItemID::Plant_Leaves                 , ItemType::Plant    , 50, 0.00f, 0.0f, "Leaf"                      , "Leaves"                     };
        byId[(size_t)ItemID::Plant_BlueFlower             ] = { ItemID::Plant_BlueFlower             , ItemType::Plant    , 50, 0.00f, 0.0f, "Blue Orchid"               , "Blue Orchids"               };
        byId[(size_t)ItemID::Plant_SpottedMushroom        ] = { ItemID::Plant_SpottedMushroom        , ItemType::Plant    , 50, 0.00f, 0.0f, "Spotted Mushroom"          , "Spotted Mushrooms"          };
        byId[(size_t)ItemID::Plant_Rose                   ] = { ItemID::Plant_Rose                   , ItemType::Plant    , 50, 0.00f, 0.0f, "Rose"                      , "Roses"                      };
        byId[(size_t)ItemID::Plant_Geranium               ] = { ItemID::Plant_Geranium               , ItemType::Plant    , 50, 0.00f, 0.0f, "Geranium"                  , "Geraniums"                  };
        byId[(size_t)ItemID::Book_Purple                  ] = { ItemID::Book_Purple                  , ItemType::Book     ,  5, 0.00f, 0.0f, "Purple Book"               , "Purple Books"               };
        byId[(size_t)ItemID::Book_Yellow                  ] = { ItemID::Book_Yellow                  , ItemType::Book     ,  5, 0.00f, 0.0f, "Yellow Book"               , "Yellow Books"               };
        byId[(size_t)ItemID::Book_Turquoise               ] = { ItemID::Book_Turquoise               , ItemType::Book     ,  5, 0.00f, 0.0f, "Turquoise Book"            , "Turquoise Books"            };
        byId[(size_t)ItemID::Book_Black                   ] = { ItemID::Book_Black                   , ItemType::Book     ,  5, 0.00f, 0.0f, "Black Book"                , "Black Books"                };
        byId[(size_t)ItemID::Book_BlackSkull              ] = { ItemID::Book_BlackSkull              , ItemType::Book     ,  5, 0.00f, 0.0f, "Black Book of Death"       , "Black Books of Death"       };
        byId[(size_t)ItemID::Book_BlueShield              ] = { ItemID::Book_BlueShield              , ItemType::Book     ,  5, 0.00f, 0.0f, "Blue Book of Defense"      , "Blue Books of Defense"      };
    }

    const Item &Items::FindById(ItemID id) const
    {
        return byId[(size_t)id];
    }

    const Texture Items::Tex(void) const
    {
        return tex;
    }
}