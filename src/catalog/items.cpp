#include "csv.h"
#include "items.h"
#include <stdlib.h>

#define CSV_DEBUG_PRINT 0

namespace Catalog {
    void Items::LoadTextures(void) {
        tex = LoadTexture("data/texture/item/joecreates.png");
    }

    void Items::LoadData(void)
    {
        const char *csvFilename = "data/entity/item/items.csv";
        CSV::Error err = csv.ReadFromFile(csvFilename);
        if (err != CSV::SUCCESS) {
            return;
        }

        if (csv.columns != 7) {
            printf("%s: Wrong # of columns.\n", csvFilename);
            return;
        }

        DLB_ASSERT(csv.rows % csv.columns == 0);
#if CSV_DEBUG_PRINT
        printf("%s, %s, %s, %s, %s, %s, %s\n",
            csv.values[0].data,
            csv.values[1].data,
            csv.values[2].data,
            csv.values[3].data,
            csv.values[4].data,
            csv.values[5].data,
            csv.values[6].data
        );
#endif
        for (u32 i = 1; i < csv.rows; i += 1) {
            Catalog::Item item{};
            CSV::Value vId         = csv.values[i * csv.columns];
            CSV::Value vCategory   = csv.values[i * csv.columns + 1];
            CSV::Value vStackLimit = csv.values[i * csv.columns + 2];
            CSV::Value vValue      = csv.values[i * csv.columns + 3];
            CSV::Value vDamage     = csv.values[i * csv.columns + 4];
            CSV::Value vName       = csv.values[i * csv.columns + 5];
            CSV::Value vPlural     = csv.values[i * csv.columns + 6];

            item.itemType = vId.toUint();
            DLB_ASSERT(item.itemType < ITEMTYPE_COUNT);
            item.itemClass = ItemClassFromString((char *)vCategory.data, vCategory.length);
            DLB_ASSERT(item.itemClass < ITEMCLASS_COUNT);
            item.stackLimit = vStackLimit.toUint();
            item.value = vValue.toUint();
            item.damage = vDamage.toUint();

            DLB_ASSERT(vName.data[vName.length] == 0);
            DLB_ASSERT(vPlural.data[vPlural.length] == 0);
            strncpy(item.nameSingular, (char *)vName.data, ARRAY_SIZE(item.nameSingular) - 1);
            strncpy(item.namePlural, (char *)vPlural.data, ARRAY_SIZE(item.namePlural) - 1);

#if CSV_DEBUG_PRINT
            printf("%d, %d, %u, %f, %f, %s, %s\n",
                item.type          ,
                item.itemClass        ,
                item.stackLimit  ,
                item.value       ,
                item.damage      ,
                item.nameSingular,
                item.namePlural
            );
#endif
            byId[item.itemType] = item;
        }
    }

    Item &Items::Find(ItemType itemType)
    {
        return byId[itemType];
    }

    Texture Items::Tex(void) const
    {
        return tex;
    }

    const Item *Items::Data(void) const
    {
        return byId;
    }
}