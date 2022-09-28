#include "csv.h"
#include "items.h"
#include <stdlib.h>

#define CSV_DEBUG_PRINT 0

void ItemCatalog::LoadTextures(void) {
    tex = LoadTexture("data/texture/item/joecreates.png");
}

void ItemCatalog::LoadData(void)
{
    const char *csvFilename = "data/entity/item/items.csv";
    CSV::Error err = csv.ReadFromFile(csvFilename);
    if (err != CSV::SUCCESS) {
        return;
    }

    const int colCount = 8;
    if (csv.columns != colCount) {
        printf("%s: Wrong # of columns. Expected %d\n", csvFilename, colCount);
        return;
    }

    DLB_ASSERT(csv.values.size() % csv.columns == 0);
#if CSV_DEBUG_PRINT
    printf("%s, %s, %s, %s, %s, %s, %s, %s\n",
        csv.values[0].data,
        csv.values[1].data,
        csv.values[2].data,
        csv.values[3].data,
        csv.values[4].data,
        csv.values[5].data,
        csv.values[6].data,
        csv.values[7].data
    );
#endif
    for (u32 i = 1; i < csv.rows; i += 1) {
        int c = 0;
        CSV::Value vId            = csv.values[i * csv.columns + c++];
        CSV::Value vCategory      = csv.values[i * csv.columns + c++];
        CSV::Value vStackLimit    = csv.values[i * csv.columns + c++];
        CSV::Value vValue         = csv.values[i * csv.columns + c++];
        CSV::Value vDamageFlatMin = csv.values[i * csv.columns + c++];
        CSV::Value vDamageFlatMax = csv.values[i * csv.columns + c++];
        CSV::Value vName          = csv.values[i * csv.columns + c++];
        CSV::Value vPlural        = csv.values[i * csv.columns + c++];

        ItemType itemType = vId.toUint();
        DLB_ASSERT(itemType < ItemType_Count);

        ItemProto &proto = g_item_catalog.protos[itemType];
        proto.itemType = itemType;
        proto.itemClass = ItemClassFromString((char *)vCategory.data, vCategory.length);
        DLB_ASSERT(proto.itemClass < ItemClass_Count);
        proto.stackLimit = vStackLimit.toUint();

        float value = vValue.toFloat();
        proto.SetAffixProto(ItemAffix_Value, { value, value });

        // HACK(dlb): Remove this and put the data in the data file, and/or roll it on item spawn
        // This isn't going to work over the network as-is, both sides will have different random numbers
        if (proto.itemClass == ItemClass_Weapon) {
            float dmgFlatMin = vDamageFlatMin.toFloat();
            float dmgFlatMax = vDamageFlatMax.toFloat();
            // HACK: Just put all 4 numbers in the file you idiot.. :D
            proto.SetAffixProto(ItemAffix_DamageFlat, { dmgFlatMin, dmgFlatMin + 2.0f }, { 2.0f + dmgFlatMax, 2.0f + dmgFlatMax + 4.0f });
            proto.SetAffixProto(ItemAffix_KnockbackFlat, { 1.0f, 5.0f }); //(float)dlb_rand32u_range(0, 2) * 2);
        } else if (proto.itemClass == ItemClass_Shield) {
            proto.SetAffixProto(ItemAffix_MoveSpeedFlat, { 1.0f, 3.0f });
        }

        DLB_ASSERT(vName.data[vName.length] == 0);
        DLB_ASSERT(vPlural.data[vPlural.length] == 0);
        strncpy(proto.nameSingular, (char *)vName.data, ARRAY_SIZE(proto.nameSingular) - 1);
        strncpy(proto.namePlural, (char *)vPlural.data, ARRAY_SIZE(proto.namePlural) - 1);

#if CSV_DEBUG_PRINT
        const ItemAffixProto &valueProto = proto.FindAffixProto(ItemAffix_Value);
        const ItemAffixProto &damageFlatProto = proto.FindAffixProto(ItemAffix_DamageFlat);
        printf("%3d, %2d, %3u, %3.0f, %3.0f - %3.0f, %3.0f - %3.0f, %32s, %32s\n",
            proto.itemType,
            proto.itemClass,
            proto.stackLimit,
            valueProto.minRange.min,
            damageFlatProto.minRange.min,
            damageFlatProto.minRange.max,
            damageFlatProto.maxRange.min,
            damageFlatProto.maxRange.max,
            proto.nameSingular,
            proto.namePlural
        );
#endif
    }
}