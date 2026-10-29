#include "ware_distribution.h"


namespace
{

static const std::uint8_t DISTRIBUTION_DEFAULTS[
    WARE_RATIO_TAB_COUNT
][
    WARE_RATIO_TARGET_COUNT
] = {
    {5, 5, 0, 0},
    {5, 3, 4, 4},
    {3, 4, 0, 0},
    {4, 5, 3, 0}
};


int clampRatioValue(int value)
{
    if (value < WARE_RATIO_MIN)
        return WARE_RATIO_MIN;

    if (value > WARE_RATIO_MAX)
        return WARE_RATIO_MAX;

    return value;
}


bool isValidWareIndex(int wareIndex)
{
    return
        wareIndex >= 0 &&
        wareIndex < WARE_RATIO_TAB_COUNT;
}


bool isValidTargetIndex(int targetIndex)
{
    return
        targetIndex >= 0 &&
        targetIndex < WARE_RATIO_TARGET_COUNT;
}

}


void initializeWareDistribution(
    WareDistribution* distribution
)
{
    if (!distribution)
        return;

    for (
        int wareIndex = 0;
        wareIndex < WARE_RATIO_TAB_COUNT;
        ++wareIndex
    ) {
        for (
            int targetIndex = 0;
            targetIndex < WARE_RATIO_TARGET_COUNT;
            ++targetIndex
        ) {
            distribution
                ->values[wareIndex][targetIndex] =
                    DISTRIBUTION_DEFAULTS[
                        wareIndex
                    ][
                        targetIndex
                    ];
        }
    }
}


int getWareDistributionValue(
    const WareDistribution& distribution,
    WareRatioType ware,
    int targetIndex
)
{
    const int wareIndex =
        static_cast<int>(ware);

    if (
        !isValidWareIndex(wareIndex) ||
        !isValidTargetIndex(targetIndex)
    ) {
        return WARE_RATIO_MAX;
    }

    return distribution.values[
        wareIndex
    ][
        targetIndex
    ];
}


void setWareDistributionValue(
    WareDistribution* distribution,
    WareRatioType ware,
    int targetIndex,
    int value
)
{
    if (!distribution)
        return;

    const int wareIndex =
        static_cast<int>(ware);

    if (
        !isValidWareIndex(wareIndex) ||
        !isValidTargetIndex(targetIndex)
    ) {
        return;
    }

    distribution->values[
        wareIndex
    ][
        targetIndex
    ] = static_cast<std::uint8_t>(
        clampRatioValue(value)
    );
}


int getWareRatioTargetCount(
    WareRatioType ware
)
{
    switch (ware) {
        case WareRatioType::Steel:
            return 2;

        case WareRatioType::Coal:
            return 4;

        case WareRatioType::Wood:
            return 2;

        case WareRatioType::Corn:
            return 3;
    }

    return 0;
}
