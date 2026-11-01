#include "save_units_v11.h"

#include <cmath>

#pragma pack(push, 1)

struct UnitSaveV11 {
    std::int32_t type;
    std::int32_t state;
    std::int32_t condition;

    float worldX;
    float worldY;
    float targetWorldX;
    float targetWorldY;

    std::int32_t pathX[MAX_UNIT_PATH];
    std::int32_t pathY[MAX_UNIT_PATH];

    std::int32_t pathLength;
    std::int32_t pathIndex;

    std::int32_t direction;
    std::int32_t animationFrame;
    float animationTimer;

    std::int32_t targetBuilding;

    std::int32_t logisticsState;
    std::int32_t assignedWare;
    std::int32_t carriedWare;

    std::int32_t sourceBuilding;
    std::int32_t destinationBuilding;

    std::int32_t targetRoadX;
    std::int32_t targetRoadY;

    float workTimer;
    std::int32_t workAction;

    float doorwayProgress;
    float doorwayStartX;
    float doorwayStartY;
    float doorwayTargetX;
    float doorwayTargetY;

    std::int32_t doorwayOffsetX;
    std::int32_t doorwayOffsetY;

    std::uint8_t visible;
    std::uint8_t active;
};

#pragma pack(pop)

static_assert(
    sizeof(UnitSaveV11) == 2166,
    "UnitSaveV11 size changed"
);

static UnitSaveV11 encodeUnit(
    const Unit& unit
)
{
    UnitSaveV11 out = {};

    out.type = static_cast<std::int32_t>(unit.type);
    out.state = static_cast<std::int32_t>(unit.state);
    out.condition = unit.condition;

    out.worldX = unit.worldX;
    out.worldY = unit.worldY;
    out.targetWorldX = unit.targetWorldX;
    out.targetWorldY = unit.targetWorldY;

    for (int i = 0; i < MAX_UNIT_PATH; ++i) {
        out.pathX[i] = unit.pathX[i];
        out.pathY[i] = unit.pathY[i];
    }

    out.pathLength = unit.pathLength;
    out.pathIndex = unit.pathIndex;

    out.direction = unit.direction;
    out.animationFrame = unit.animationFrame;
    out.animationTimer = unit.animationTimer;

    out.targetBuilding = unit.targetBuilding;

    out.logisticsState =
        static_cast<std::int32_t>(unit.logisticsState);

    out.assignedWare =
        static_cast<std::int32_t>(unit.assignedWare);

    out.carriedWare =
        static_cast<std::int32_t>(unit.carriedWare);

    out.sourceBuilding = unit.sourceBuilding;
    out.destinationBuilding = unit.destinationBuilding;

    out.targetRoadX = unit.targetRoadX;
    out.targetRoadY = unit.targetRoadY;

    out.workTimer = unit.workTimer;

    out.workAction =
        static_cast<std::int32_t>(unit.workAction);

    out.doorwayProgress = unit.doorwayProgress;
    out.doorwayStartX = unit.doorwayStartX;
    out.doorwayStartY = unit.doorwayStartY;
    out.doorwayTargetX = unit.doorwayTargetX;
    out.doorwayTargetY = unit.doorwayTargetY;

    out.doorwayOffsetX = unit.doorwayOffsetX;
    out.doorwayOffsetY = unit.doorwayOffsetY;

    out.visible = unit.visible ? 1 : 0;
    out.active = unit.active ? 1 : 0;

    return out;
}

static bool validBuildingIndex(
    std::int32_t value,
    int buildingCount
)
{
    return value == -1 ||
        (value >= 0 && value < buildingCount);
}

static bool decodeUnit(
    const UnitSaveV11& in,
    Unit* unit,
    int buildingCount
)
{
    if (!unit)
        return false;

    if (
        in.type < static_cast<int>(UnitType::Worker) ||
        in.type > static_cast<int>(UnitType::Farmer)
    )
        return false;

    if (
        in.state < static_cast<int>(UnitState::Idle) ||
        in.state > static_cast<int>(UnitState::Dying)
    )
        return false;

    if (
        in.logisticsState <
            static_cast<int>(SerfLogisticsState::Idle) ||
        in.logisticsState >
            static_cast<int>(SerfLogisticsState::Returning)
    )
        return false;

    if (
        in.assignedWare < 0 ||
        in.assignedWare > static_cast<int>(WareType::Count) ||
        in.carriedWare < 0 ||
        in.carriedWare > static_cast<int>(WareType::Count)
    )
        return false;

    if (
        in.workAction <
            static_cast<int>(UnitWorkAction::None) ||
        in.workAction >
            static_cast<int>(UnitWorkAction::BuildWineWood)
    )
        return false;

    if (
        in.condition < 0 ||
        in.condition > UNIT_MAX_CONDITION
    )
        return false;

    if (
        in.pathLength < 0 ||
        in.pathLength > MAX_UNIT_PATH ||
        in.pathIndex < 0 ||
        in.pathIndex > in.pathLength
    )
        return false;

    if (
        in.direction < 0 ||
        in.direction > 7 ||
        in.animationFrame < 0
    )
        return false;

    if (
        in.visible > 1 ||
        in.active > 1
    )
        return false;

    if (
        !validBuildingIndex(
            in.targetBuilding,
            buildingCount
        ) ||
        !validBuildingIndex(
            in.sourceBuilding,
            buildingCount
        ) ||
        !validBuildingIndex(
            in.destinationBuilding,
            buildingCount
        )
    )
        return false;

    const float floats[] = {
        in.worldX,
        in.worldY,
        in.targetWorldX,
        in.targetWorldY,
        in.animationTimer,
        in.workTimer,
        in.doorwayProgress,
        in.doorwayStartX,
        in.doorwayStartY,
        in.doorwayTargetX,
        in.doorwayTargetY
    };

    for (float value : floats)
        if (!std::isfinite(value))
            return false;

    if (
        in.animationTimer < 0.0f ||
        in.workTimer < 0.0f ||
        in.doorwayProgress < 0.0f ||
        in.doorwayProgress > 1.0f
    )
        return false;

    Unit out = {};

    out.type = static_cast<UnitType>(in.type);
    out.state = static_cast<UnitState>(in.state);
    out.condition = in.condition;

    out.worldX = in.worldX;
    out.worldY = in.worldY;
    out.targetWorldX = in.targetWorldX;
    out.targetWorldY = in.targetWorldY;

    for (int i = 0; i < MAX_UNIT_PATH; ++i) {
        out.pathX[i] = in.pathX[i];
        out.pathY[i] = in.pathY[i];
    }

    out.pathLength = in.pathLength;
    out.pathIndex = in.pathIndex;

    out.direction = in.direction;
    out.animationFrame = in.animationFrame;
    out.animationTimer = in.animationTimer;

    out.targetBuilding = in.targetBuilding;

    out.logisticsState =
        static_cast<SerfLogisticsState>(
            in.logisticsState
        );

    out.assignedWare =
        static_cast<WareType>(in.assignedWare);

    out.carriedWare =
        static_cast<WareType>(in.carriedWare);

    out.sourceBuilding = in.sourceBuilding;
    out.destinationBuilding = in.destinationBuilding;

    out.targetRoadX = in.targetRoadX;
    out.targetRoadY = in.targetRoadY;

    out.workTimer = in.workTimer;

    out.workAction =
        static_cast<UnitWorkAction>(
            in.workAction
        );

    out.doorwayProgress = in.doorwayProgress;
    out.doorwayStartX = in.doorwayStartX;
    out.doorwayStartY = in.doorwayStartY;
    out.doorwayTargetX = in.doorwayTargetX;
    out.doorwayTargetY = in.doorwayTargetY;

    out.doorwayOffsetX = in.doorwayOffsetX;
    out.doorwayOffsetY = in.doorwayOffsetY;

    out.visible = in.visible != 0;
    out.active = in.active != 0;

    *unit = out;
    return true;

}

bool writeUnitsV11(
    FILE* file,
    const Unit* units,
    int unitCount
)
{
    if (
        !file ||
        !units ||
        unitCount < 0 ||
        unitCount > MAX_UNITS
    )
        return false;

    const std::int32_t count = unitCount;

    if (
        fwrite(
            &count,
            sizeof(count),
            1,
            file
        ) != 1
    )
        return false;

    for (int i = 0; i < MAX_UNITS; ++i) {
        UnitSaveV11 saved = {};

        if (i < unitCount)
            saved = encodeUnit(units[i]);

        if (
            fwrite(
                &saved,
                sizeof(saved),
                1,
                file
            ) != 1
        )
            return false;
    }

    return true;
}

bool readUnitsV11(
    FILE* file,
    Unit* units,
    int* unitCount,
    int buildingCount
)
{
    if (
        !file ||
        !units ||
        !unitCount ||
        buildingCount < 0
    )
        return false;

    std::int32_t count = 0;

    if (
        fread(
            &count,
            sizeof(count),
            1,
            file
        ) != 1
    ) {
        return false;
    }

    if (
        count < 0 ||
        count > MAX_UNITS
    ) {
        return false;
    }

    for (int i = 0; i < MAX_UNITS; ++i) {
        UnitSaveV11 saved = {};

        if (
            fread(
                &saved,
                sizeof(saved),
                1,
                file
            ) != 1
        ) {
            return false;
        }

        if (i < count) {
            if (
                !decodeUnit(
                    saved,
                    &units[i],
                    buildingCount
                )
            ) {
                return false;
            }
        } else {
            units[i] = {};
        }
    }

    *unitCount = count;

    return true;
}
