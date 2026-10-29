#pragma once

#include <cstdint>


enum class WareRatioType {
    Steel = 0,
    Coal = 1,
    Wood = 2,
    Corn = 3
};


static constexpr int WARE_RATIO_TAB_COUNT = 4;
static constexpr int WARE_RATIO_TARGET_COUNT = 4;
static constexpr int WARE_RATIO_MIN = 0;
static constexpr int WARE_RATIO_MAX = 5;


struct WareDistribution {
    std::uint8_t values[
        WARE_RATIO_TAB_COUNT
    ][
        WARE_RATIO_TARGET_COUNT
    ];
};


// Nastavi puvodni KaM Remake defaults:
//
// Steel: 5 5 0 0
// Coal:  5 3 4 4
// Wood:  3 4 0 0
// Corn:  4 5 3 0
void initializeWareDistribution(
    WareDistribution* distribution
);


int getWareDistributionValue(
    const WareDistribution& distribution,
    WareRatioType ware,
    int targetIndex
);


void setWareDistributionValue(
    WareDistribution* distribution,
    WareRatioType ware,
    int targetIndex,
    int value
);


int getWareRatioTargetCount(
    WareRatioType ware
);
