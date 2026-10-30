#include "house_plan_catalog.h"

namespace {

static const HousePlanCatalogEntry
HOUSE_PLAN_CATALOG[HOUSE_PLAN_CATALOG_COUNT] = {
    {
        "Armor smithy",
        {
            {0, 0, 0, 0},
            {0, 1, 1, 0},
            {1, 1, 1, 1},
            {1, 2, 1, 1},
        },
        1, 3,
        261,
        35, 29,
        -17, -14,
        "app0:assets/buildings/tablets/original/armor_smithy_261.bmp"
    },
    {
        "Armor workshop",
        {
            {0, 0, 0, 0},
            {0, 1, 1, 0},
            {0, 1, 1, 1},
            {0, 2, 1, 1},
        },
        1, 3,
        271,
        35, 29,
        -17, -14,
        "app0:assets/buildings/tablets/original/armor_workshop_271.bmp"
    },
    {
        "Bakery",
        {
            {0, 0, 0, 0},
            {0, 1, 1, 1},
            {0, 1, 1, 1},
            {0, 1, 1, 2},
        },
        3, 3,
        258,
        35, 28,
        -17, -14,
        "app0:assets/buildings/tablets/original/bakery_258.bmp"
    },
    {
        "Barracks",
        {
            {1, 1, 1, 1},
            {1, 1, 1, 1},
            {1, 1, 1, 1},
            {1, 2, 1, 1},
        },
        1, 3,
        272,
        35, 29,
        -17, -14,
        "app0:assets/buildings/tablets/original/barracks_272.bmp"
    },
    {
        "Butchers",
        {
            {0, 0, 0, 0},
            {0, 1, 1, 0},
            {0, 1, 1, 1},
            {0, 1, 1, 2},
        },
        3, 3,
        275,
        35, 30,
        -17, -15,
        "app0:assets/buildings/tablets/original/butchers_275.bmp"
    },
    {
        "Coal mine",
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {1, 1, 1, 0},
            {1, 2, 1, 0},
        },
        1, 3,
        254,
        35, 28,
        -17, -14,
        "app0:assets/buildings/tablets/original/coal_mine_254.bmp"
    },
    {
        "Farm",
        {
            {0, 0, 0, 0},
            {1, 1, 1, 1},
            {1, 1, 1, 1},
            {1, 2, 1, 1},
        },
        1, 3,
        259,
        35, 30,
        -17, -15,
        "app0:assets/buildings/tablets/original/farm_259.bmp"
    },
    {
        "Fisher hut",
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 1, 1, 0},
            {0, 2, 1, 1},
        },
        1, 3,
        257,
        35, 30,
        -17, -15,
        "app0:assets/buildings/tablets/original/fisher_hut_257.bmp"
    },
    {
        "Gold mine",
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 1, 2, 0},
        },
        2, 3,
        256,
        35, 28,
        -17, -14,
        "app0:assets/buildings/tablets/original/gold_mine_256.bmp"
    },
    {
        "Inn",
        {
            {0, 0, 0, 0},
            {0, 1, 1, 1},
            {1, 1, 1, 1},
            {1, 2, 1, 1},
        },
        1, 3,
        278,
        35, 30,
        -17, -15,
        "app0:assets/buildings/tablets/original/inn_278.bmp"
    },
    {
        "Iron mine",
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 1, 2, 1},
        },
        2, 3,
        255,
        35, 28,
        -17, -14,
        "app0:assets/buildings/tablets/original/iron_mine_255.bmp"
    },
    {
        "Iron smithy",
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {1, 1, 1, 1},
            {1, 1, 2, 1},
        },
        2, 3,
        252,
        35, 30,
        -17, -15,
        "app0:assets/buildings/tablets/original/iron_smithy_252.bmp"
    },
    {
        "Marketplace",
        {
            {0, 0, 0, 0},
            {0, 1, 1, 1},
            {1, 1, 1, 1},
            {1, 1, 1, 2},
        },
        3, 3,
        277,
        16, 15,
        -8, -7,
        "app0:assets/buildings/tablets/original/marketplace_277.bmp"
    },
    {
        "Metallurgist",
        {
            {0, 0, 0, 0},
            {1, 1, 1, 0},
            {1, 1, 1, 0},
            {1, 2, 1, 0},
        },
        1, 3,
        266,
        34, 29,
        -17, -14,
        "app0:assets/buildings/tablets/original/metallurgist_266.bmp"
    },
    {
        "Mill",
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 1, 1, 1},
            {0, 1, 2, 1},
        },
        2, 3,
        273,
        35, 29,
        -17, -14,
        "app0:assets/buildings/tablets/original/mill_273.bmp"
    },
    {
        "Quarry",
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 1, 1, 1},
            {0, 1, 2, 1},
        },
        2, 3,
        265,
        35, 29,
        -17, -14,
        "app0:assets/buildings/tablets/original/quarry_265.bmp"
    },
    {
        "Sawmill",
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {1, 1, 1, 1},
            {1, 2, 1, 1},
        },
        1, 3,
        251,
        35, 27,
        -17, -13,
        "app0:assets/buildings/tablets/original/sawmill_251.bmp"
    },
    {
        "School",
        {
            {0, 0, 0, 0},
            {1, 1, 1, 0},
            {1, 1, 1, 0},
            {1, 2, 1, 0},
        },
        1, 3,
        264,
        35, 30,
        -17, -15,
        "app0:assets/buildings/tablets/original/school_264.bmp"
    },
    {
        "Siege workshop",
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 1, 1, 1},
            {0, 2, 1, 1},
        },
        1, 3,
        274,
        35, 29,
        -17, -14,
        "app0:assets/buildings/tablets/original/siege_workshop_274.bmp"
    },
    {
        "Stables",
        {
            {0, 0, 0, 0},
            {1, 1, 1, 1},
            {1, 1, 1, 1},
            {1, 1, 2, 1},
        },
        2, 3,
        263,
        35, 30,
        -17, -15,
        "app0:assets/buildings/tablets/original/stables_263.bmp"
    },
    {
        "Store",
        {
            {0, 0, 0, 0},
            {1, 1, 1, 0},
            {1, 1, 1, 0},
            {1, 2, 1, 0},
        },
        1, 3,
        262,
        35, 29,
        -17, -14,
        "app0:assets/buildings/tablets/original/store_262.bmp"
    },
    {
        "Swine",
        {
            {0, 0, 0, 0},
            {0, 1, 1, 1},
            {1, 1, 1, 1},
            {1, 1, 1, 2},
        },
        3, 3,
        267,
        35, 30,
        -17, -15,
        "app0:assets/buildings/tablets/original/swine_267.bmp"
    },
    {
        "Tannery",
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 1, 1, 1},
            {0, 1, 2, 1},
        },
        2, 3,
        276,
        35, 30,
        -17, -15,
        "app0:assets/buildings/tablets/original/tannery_276.bmp"
    },
    {
        "Town hall",
        {
            {0, 0, 0, 0},
            {1, 1, 1, 1},
            {1, 1, 1, 1},
            {1, 2, 1, 1},
        },
        1, 3,
        269,
        35, 30,
        -17, -15,
        "app0:assets/buildings/tablets/original/town_hall_269.bmp"
    },
    {
        "Watch tower",
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 1, 1, 0},
            {0, 1, 2, 0},
        },
        2, 3,
        268,
        35, 29,
        -17, -14,
        "app0:assets/buildings/tablets/original/watch_tower_268.bmp"
    },
    {
        "Weapon smithy",
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {1, 1, 1, 1},
            {1, 2, 1, 1},
        },
        1, 3,
        253,
        35, 29,
        -17, -14,
        "app0:assets/buildings/tablets/original/weapon_smithy_253.bmp"
    },
    {
        "Weapon workshop",
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {1, 1, 1, 1},
            {1, 2, 1, 1},
        },
        1, 3,
        270,
        35, 30,
        -17, -15,
        "app0:assets/buildings/tablets/original/weapon_workshop_270.bmp"
    },
    {
        "Wineyard",
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 1, 1, 1},
            {0, 1, 1, 2},
        },
        3, 3,
        279,
        35, 29,
        -17, -14,
        "app0:assets/buildings/tablets/original/wineyard_279.bmp"
    },
    {
        "Woodcutter",
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {1, 1, 1, 0},
            {1, 1, 2, 0},
        },
        2, 3,
        260,
        35, 30,
        -17, -15,
        "app0:assets/buildings/tablets/original/woodcutter_260.bmp"
    },
};

}

const HousePlanCatalogEntry&
getHousePlanCatalogEntry(int index)
{
    if (index < 0)
        index = 0;

    if (index >= HOUSE_PLAN_CATALOG_COUNT)
        index = HOUSE_PLAN_CATALOG_COUNT - 1;

    return HOUSE_PLAN_CATALOG[index];
}
