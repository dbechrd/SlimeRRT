#include "csv.h"
#include "items.h"
#include <stdlib.h>

#define CSV_DEBUG_PRINT 0

namespace Catalog {
    void Items::Load(void)
    {
        if (IsWindowReady()) {
            tex = LoadTexture("resources/joecreates.png");
        }

        const char *csvFilename = "data/items.csv";
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

            item.id = (ItemID)vId.toInt();
            item.type = ItemTypeFromString((char *)vCategory.data, vCategory.length);
            item.stackLimit = vStackLimit.toInt();
            item.value = vValue.toFloat();
            item.damage = vDamage.toFloat();
            item.nameSingular = (char *)vName.data;
            item.namePlural = (char *)vPlural.data;

            DLB_ASSERT(vName.data[vName.length] == 0);
            DLB_ASSERT(vPlural.data[vPlural.length] == 0);
#if CSV_DEBUG_PRINT
            printf("%d, %d, %u, %f, %f, %s, %s\n",
                item.id          ,
                item.type        ,
                item.stackLimit  ,
                item.value       ,
                item.damage      ,
                item.nameSingular,
                item.namePlural
            );
#endif
            byId[(size_t)item.id] = item;
        }
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