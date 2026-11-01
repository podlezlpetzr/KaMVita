#pragma once

#include <cstdint>
#include <cstdio>

#include "units.h"

struct UnitSaveV11;

bool writeUnitsV11(
    FILE* file,
    const Unit* units,
    int unitCount
);

bool readUnitsV11(
    FILE* file,
    Unit* units,
    int* unitCount,
    int buildingCount
);
