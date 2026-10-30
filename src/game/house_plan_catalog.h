#pragma once

#include <cstddef>

struct HousePlanCatalogEntry {
    const char* name;

    unsigned char plan[4][4];

    int entranceX;
    int entranceY;

    int tabletSpriteId;

    int tabletWidth;
    int tabletHeight;

    int tabletPivotX;
    int tabletPivotY;

    const char* tabletAssetPath;
};

static constexpr int HOUSE_PLAN_CATALOG_COUNT = 29;

const HousePlanCatalogEntry&
getHousePlanCatalogEntry(int index);
