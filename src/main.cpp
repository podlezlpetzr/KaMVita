#include "render/terrain.h"
#include "original_kam/original_kam_resources.h"
#include "render/kam_plan_renderer.h"
#include "game/buildings.h"
#include "game/units.h"
#include "game/save_units_v11.h"
#include "game/save_slots.h"
#include "ui/game_ui.h"
#include "ui/game_ui_unit.h"
#include "ui/game_ui_ratios.h"
#include "ui/kam_runtime_font.h"
#include "ui/house_icons.h"
#include "game/ware_distribution.h"
#include <algorithm>
#include <vector>
#include <SDL2/SDL.h>
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <cstring>


static constexpr int SCREEN_WIDTH  = 960;
static constexpr int SCREEN_HEIGHT = 544;
static constexpr int TILE_SIZE = 32;
static constexpr int MAP_WIDTH = 64;
static constexpr int MAP_HEIGHT = 64;
static constexpr int WORLD_WIDTH = MAP_WIDTH * TILE_SIZE;
static constexpr int WORLD_HEIGHT = MAP_HEIGHT * TILE_SIZE;

// ============================================================
// ORIGINAL KaM BAKERY PRODUCTION
//
// houses.dat / Remake workplan:
// 1 Flour -> 2 Bread
// 7x Work2(24) + Work3(30) = 378 animation ticks.
// House output capacity = 5.
// ============================================================

static constexpr int BAKERY_FLOUR_PER_CYCLE = 1;
static constexpr int BAKERY_BREAD_PER_CYCLE = 2;
static constexpr int BAKERY_PRODUCTION_TICKS = 378;
static constexpr int BAKERY_OUTPUT_CAPACITY = 5;

// Jeden originalni House animation tick.
// Zpracovani pouziva accumulator, takze nepropadaji tick kroky
// ani pri delsim frame delta.
static constexpr float BAKERY_TICK_SECONDS = 0.1f;


struct SaveHeader {
    uint32_t magic;
    uint32_t version;
    int32_t buildingCount;
};


struct BuildingSaveV5 {
    int tileX;
    int tileY;
    BuildingType type;
    int constructionProgress;
    int woodDelivered;
    int stoneDelivered;
    int woodStep;
    int stoneStep;
    int schoolQueue[6];
    int schoolTrainingStep;
    int schoolIdleStep;
    int schoolGold;
    bool schoolTrainingActive;
    bool schoolFinishingTraining;
    bool active;
};

static_assert(
    sizeof(BuildingSaveV5) == 72,
    "BuildingSaveV5 musi odpovidat puvodnimu save formatu"
);


// Save v6 a puvodni save v7 vznikly po pridani
// House runtime stavu, ale pred Store inventory.
//
// Tento layout musi zustat pevny. Nesmime pro legacy
// save pouzivat sizeof(Building), protoze Building se
// dale rozsiruje.
struct BuildingSaveV7 {
    int tileX;
    int tileY;
    BuildingType type;
    int constructionProgress;

    int woodDelivered;
    int stoneDelivered;
    int woodStep;
    int stoneStep;

    int health;
    int maxHealth;

    bool repairEnabled;
    bool wareDeliveryEnabled;

    int schoolQueue[6];
    int schoolTrainingStep;
    int schoolIdleStep;
    int schoolGold;

    bool schoolTrainingActive;
    bool schoolFinishingTraining;
    bool active;
};

static_assert(
    sizeof(BuildingSaveV7) == 84,
    "BuildingSaveV7 musi mit presne 84 bajtu"
);


// ===========================================================
// SAVE V8 FIXED BUILDING LAYOUT
//
// Save v8 zapisoval tehdejsi Building primo jako 224 bajtu.
// Po rozsireni runtime Building nesmime v8 cist pres novy
// sizeof(Building).
// ===========================================================

struct BuildingSaveV8 {
    int tileX;
    int tileY;
    BuildingType type;

    int constructionProgress;

    int woodDelivered;
    int stoneDelivered;
    int woodStep;
    int stoneStep;

    int health;
    int maxHealth;

    bool repairEnabled;
    bool wareDeliveryEnabled;

    int schoolQueue[6];

    int schoolTrainingStep;
    int schoolIdleStep;
    int schoolGold;

    int storeWares[STORE_WARE_COUNT];
    bool storeNotAccept[STORE_WARE_COUNT];

    bool schoolTrainingActive;
    bool schoolFinishingTraining;
    bool active;
};

static_assert(
    sizeof(BuildingSaveV8) == 224,
    "BuildingSaveV8 musi mit presne 224 bajtu"
);

// ===========================================================
// SAVE V9 FIXED BUILDING LAYOUT
//
// Save v9 zapisoval tehdejsi Building primo jako 272 bajtu.
// Novy Building ma Bakery production runtime na konci,
// proto v9 musime nacitat pres pevny 272bajtovy zaznam.
// ===========================================================

struct BuildingSaveV9 {
    unsigned char bytes[272];
};

static_assert(
    sizeof(BuildingSaveV9) == 272,
    "BuildingSaveV9 musi mit presne 272 bajtu"
);

static_assert(
    sizeof(Building) == 280,
    "Save v10 ocekava Building o velikosti 280 bajtu"
);


// Save v11 unit serialization is implemented in
// src/game/save_units_v11.cpp.

static int getDefaultBuildingMaxHealth(
    BuildingType type
)
{
    return (
        getBuildingWoodCost(type) +
        getBuildingStoneCost(type)
    ) * 50;
}


static void initializeHouseRuntimeState(
    Building* building
)
{
    if (!building)
        return;

    building->maxHealth =
        getDefaultBuildingMaxHealth(
            building->type
        );

    if (building->maxHealth < 1)
        building->maxHealth = 1;

    building->health = building->maxHealth;
    building->repairEnabled = false;
    building->wareDeliveryEnabled = true;

    building->bakeryProductionActive = false;
    building->bakeryProductionTick = 0;
}


static Building migrateBuildingSaveV5(
    const BuildingSaveV5& old
)
{
    Building building = {};

    building.tileX = old.tileX;
    building.tileY = old.tileY;
    building.type = old.type;
    building.constructionProgress =
        old.constructionProgress;
    building.woodDelivered = old.woodDelivered;
    building.stoneDelivered = old.stoneDelivered;
    building.woodStep = old.woodStep;
    building.stoneStep = old.stoneStep;

    for (
        int i = 0;
        i < SCHOOL_QUEUE_SIZE;
        ++i
    ) {
        building.schoolQueue[i] =
            old.schoolQueue[i];
    }

    building.schoolTrainingStep =
        old.schoolTrainingStep;
    building.schoolIdleStep =
        old.schoolIdleStep;
    building.schoolGold =
        old.schoolGold;
    building.schoolTrainingActive =
        old.schoolTrainingActive;
    building.schoolFinishingTraining =
        old.schoolFinishingTraining;
    building.active = old.active;

    initializeHouseRuntimeState(
        &building
    );

    return building;
}




static Building migrateBuildingSaveV7(
    const BuildingSaveV7& old
)
{
    Building building = {};

    building.tileX = old.tileX;
    building.tileY = old.tileY;
    building.type = old.type;

    building.constructionProgress =
        old.constructionProgress;

    building.woodDelivered =
        old.woodDelivered;

    building.stoneDelivered =
        old.stoneDelivered;

    building.woodStep = old.woodStep;
    building.stoneStep = old.stoneStep;

    building.health = old.health;
    building.maxHealth = old.maxHealth;

    building.repairEnabled =
        old.repairEnabled;

    building.wareDeliveryEnabled =
        old.wareDeliveryEnabled;

    for (
        int queueIndex = 0;
        queueIndex < SCHOOL_QUEUE_SIZE;
        ++queueIndex
    ) {
        building.schoolQueue[queueIndex] =
            old.schoolQueue[queueIndex];
    }

    building.schoolTrainingStep =
        old.schoolTrainingStep;

    building.schoolIdleStep =
        old.schoolIdleStep;

    building.schoolGold =
        old.schoolGold;

    // Store pole zustanou z value initialization
    // nastavena na 0 a false.
    for (
        int wareIndex = 0;
        wareIndex < STORE_WARE_COUNT;
        ++wareIndex
    ) {
        building.storeWares[wareIndex] = 0;
        building.storeNotAccept[wareIndex] = false;
    }

    building.schoolTrainingActive =
        old.schoolTrainingActive;

    building.schoolFinishingTraining =
        old.schoolFinishingTraining;

    building.active = old.active;

    return building;
}


static Building migrateBuildingSaveV8(
    const BuildingSaveV8& old
)
{
    Building building = {};

    building.tileX = old.tileX;
    building.tileY = old.tileY;
    building.type = old.type;

    building.constructionProgress =
        old.constructionProgress;

    building.woodDelivered =
        old.woodDelivered;

    building.stoneDelivered =
        old.stoneDelivered;

    building.woodStep = old.woodStep;
    building.stoneStep = old.stoneStep;

    building.health = old.health;
    building.maxHealth = old.maxHealth;

    building.repairEnabled =
        old.repairEnabled;

    building.wareDeliveryEnabled =
        old.wareDeliveryEnabled;

    for (
        int queueIndex = 0;
        queueIndex < SCHOOL_QUEUE_SIZE;
        ++queueIndex
    ) {
        building.schoolQueue[queueIndex] =
            old.schoolQueue[queueIndex];
    }

    building.schoolTrainingStep =
        old.schoolTrainingStep;

    building.schoolIdleStep =
        old.schoolIdleStep;

    building.schoolGold =
        old.schoolGold;

    for (
        int wareIndex = 0;
        wareIndex < STORE_WARE_COUNT;
        ++wareIndex
    ) {
        building.storeWares[wareIndex] =
            old.storeWares[wareIndex];

        building.storeNotAccept[wareIndex] =
            old.storeNotAccept[wareIndex];
    }

    // Nova obecna House inventory pole zustanou po
    // value initialization nastavena na nulu.
    for (
        int slot = 0;
        slot < 4;
        ++slot
    ) {
        building.resourceIn[slot] = 0;
        building.resourceDeliveryCount[slot] = 0;
        building.resourceOut[slot] = 0;
    }

    building.schoolTrainingActive =
        old.schoolTrainingActive;

    building.schoolFinishingTraining =
        old.schoolFinishingTraining;

    building.active = old.active;

    return building;
}


// ===========================================================
// SAVE GAME PATHS
//
// KaM Remake principle:
//
// Save/Load operate on a selected named save instead of
// owning one hard-coded filename internally.
//
// During this transition the existing Vita save.dat remains
// the selected compatibility save. SAVE SLOTS will later
// provide paths from ux0:data/kamvita/Saves/*.sav.
// ===========================================================

static constexpr const char* LEGACY_SAVE_PATH =
    "ux0:data/kamvita/save.dat";


static bool saveGame(
    const char* savePath,
    const Building* buildings,
    int buildingCount,
    const WareDistribution& wareDistribution,
    const Unit* units,
    int unitCount,
    const RoadState* roads,
    const LandPlanState* landPlans,
    const float* cornGrowthTimers
)
{
    sceIoMkdir(
        "ux0:data/kamvita",
        0777
    );

    FILE* file = fopen(
        savePath,
        "wb"
    );

    if (!file)
        return false;

    if (
        !savePath ||
        savePath[0] == '\0' ||
        !units ||
        !roads ||
        !landPlans ||
        !cornGrowthTimers ||
        unitCount < 0 ||
        unitCount > MAX_UNITS
    ) {
        fclose(file);
        return false;
    }

    SaveHeader header = {};

    header.magic = 0x4B414D56;

    // Save v12:
    //
    // - kompletni Buildings
    // - WareDistribution
    // - stabilni UnitSaveV11 blok
    // - RoadState pro kazdy map tile
    // - LandPlanState pro kazdy map tile
    // - cornGrowthTimers pro kazdy map tile
    //
    // Unit payload zustava ve stabilnim V11 formatu.
    header.version = 12;
    header.buildingCount = buildingCount;

    if (
        fwrite(
            &header,
            sizeof(header),
            1,
            file
        ) != 1
    ) {
        fclose(file);
        return false;
    }

    if (
        fwrite(
            buildings,
            sizeof(Building),
            MAX_BUILDINGS,
            file
        ) != MAX_BUILDINGS
    ) {
        fclose(file);
        return false;
    }

    static_assert(
        sizeof(WareDistribution) == 16,
        "WareDistribution save format musi mit 16 bajtu"
    );

    if (
        fwrite(
            &wareDistribution,
            sizeof(WareDistribution),
            1,
            file
        ) != 1
    ) {
        fclose(file);
        return false;
    }

    // =====================================================
    // SAVE V11 UNIT WRITE
    //
    // Stable Unit serialization lives in save_units_v11.cpp.
    // =====================================================

    if (
        !writeUnitsV11(
            file,
            units,
            unitCount
        )
    ) {
        fclose(file);
        return false;
    }


    // =====================================================
    // SAVE V12 MAP STATE
    //
    // Road:
    // None, Plan, Dig1..Dig4, Complete.
    //
    // Land:
    // Field/Wine construction and complete Corn lifecycle.
    // =====================================================

    constexpr int MAP_TILE_COUNT =
        MAP_WIDTH * MAP_HEIGHT;

    for (
        int tileIndex = 0;
        tileIndex < MAP_TILE_COUNT;
        ++tileIndex
    ) {
        const unsigned char value =
            static_cast<unsigned char>(
                roads[tileIndex]
            );

        if (
            fwrite(
                &value,
                sizeof(value),
                1,
                file
            ) != 1
        ) {
            fclose(file);
            return false;
        }
    }

    for (
        int tileIndex = 0;
        tileIndex < MAP_TILE_COUNT;
        ++tileIndex
    ) {
        const unsigned char value =
            static_cast<unsigned char>(
                landPlans[tileIndex]
            );

        if (
            fwrite(
                &value,
                sizeof(value),
                1,
                file
            ) != 1
        ) {
            fclose(file);
            return false;
        }
    }

    if (
        fwrite(
            cornGrowthTimers,
            sizeof(float),
            MAP_TILE_COUNT,
            file
        ) != MAP_TILE_COUNT
    ) {
        fclose(file);
        return false;
    }


    fclose(file);

    return true;
}

static bool loadGame(
    const char* savePath,
    Building* buildings,
    int* buildingCount,
    WareDistribution* wareDistribution,
    Unit* units,
    int* unitCount,
    RoadState* roads,
    LandPlanState* landPlans,
    float* cornGrowthTimers
)
{
    if (
        !savePath ||
        savePath[0] == '\0'
    ) {
        return false;
    }




    // Remake defaults plati pro novou hru,
    // chybejici save a starsi save v4-v6.
    initializeWareDistribution(
        wareDistribution
    );

    FILE* file =
        fopen(
            savePath,
            "rb"
        );

    if (!file)
        return false;

    SaveHeader header = {};

    if (
        fread(
            &header,
            sizeof(header),
            1,
            file
        ) != 1
    ) {
        fclose(file);
        return false;
    }

    if (
        header.magic != 0x4B414D56 ||
        (
            header.version != 4 &&
            header.version != 5 &&
            header.version != 6 &&
            header.version != 7 &&
            header.version != 8 &&
            header.version != 9 &&
            header.version != 10 &&
            header.version != 11 &&
            header.version != 12
        ) ||
        header.buildingCount < 0 ||
        header.buildingCount > MAX_BUILDINGS
    ) {
        fclose(file);
        return false;
    }

    Building loadedBuildings[MAX_BUILDINGS] = {};

    // Behem Store implementace mohl kratce vzniknout
    // save oznaceny jako v7, ale uz s novym 224bajtovym
    // Building layoutem. Rozlisime jej podle velikosti
    // souboru, aby fungoval puvodni i prechodny v7.
    long saveFileSize = 0;

    if (fseek(file, 0, SEEK_END) == 0) {
        saveFileSize = ftell(file);

        if (
            fseek(
                file,
                static_cast<long>(
                    sizeof(SaveHeader)
                ),
                SEEK_SET
            ) != 0
        ) {
            fclose(file);
            return false;
        }
    } else {
        fclose(file);
        return false;
    }

    const long legacyV7FileSize =
        static_cast<long>(sizeof(SaveHeader)) +
        static_cast<long>(
            sizeof(BuildingSaveV7) *
            MAX_BUILDINGS
        ) +
        static_cast<long>(
            sizeof(WareDistribution)
        );

    const long currentV7FileSize =
        static_cast<long>(sizeof(SaveHeader)) +
        static_cast<long>(
            sizeof(BuildingSaveV8) *
            MAX_BUILDINGS
        ) +
        static_cast<long>(
            sizeof(WareDistribution)
        );

    const bool v7UsesCurrentLayout =
        header.version == 7 &&
        saveFileSize == currentV7FileSize;

    const bool v7UsesLegacyLayout =
        header.version == 7 &&
        saveFileSize == legacyV7FileSize;

    if (
        header.version == 7 &&
        !v7UsesCurrentLayout &&
        !v7UsesLegacyLayout
    ) {
        fclose(file);
        return false;
    }

    if (
        header.version == 10 ||
        header.version == 11 ||
        header.version == 12
    ) {
        if (
            fread(
                loadedBuildings,
                sizeof(Building),
                MAX_BUILDINGS,
                file
            ) != MAX_BUILDINGS
        ) {
            fclose(file);
            return false;
        }
    } else if (header.version == 9) {
        BuildingSaveV9 legacyBuildings[
            MAX_BUILDINGS
        ] = {};

        if (
            fread(
                legacyBuildings,
                sizeof(BuildingSaveV9),
                MAX_BUILDINGS,
                file
            ) != MAX_BUILDINGS
        ) {
            fclose(file);
            return false;
        }

        for (
            int buildingIndex = 0;
            buildingIndex < MAX_BUILDINGS;
            ++buildingIndex
        ) {
            loadedBuildings[buildingIndex] = {};

            static_assert(
                sizeof(BuildingSaveV9) <
                    sizeof(Building),
                "v9 layout musi byt mensi nez v10"
            );

            std::memcpy(
                &loadedBuildings[buildingIndex],
                legacyBuildings[buildingIndex].bytes,
                sizeof(BuildingSaveV9)
            );

            loadedBuildings[buildingIndex]
                .bakeryProductionActive = false;

            loadedBuildings[buildingIndex]
                .bakeryProductionTick = 0;
        }
    } else if (
        header.version == 8 ||
        v7UsesCurrentLayout
    ) {
        BuildingSaveV8 legacyBuildings[
            MAX_BUILDINGS
        ] = {};

        if (
            fread(
                legacyBuildings,
                sizeof(BuildingSaveV8),
                MAX_BUILDINGS,
                file
            ) != MAX_BUILDINGS
        ) {
            fclose(file);
            return false;
        }

        for (
            int buildingIndex = 0;
            buildingIndex < MAX_BUILDINGS;
            ++buildingIndex
        ) {
            loadedBuildings[buildingIndex] =
                migrateBuildingSaveV8(
                    legacyBuildings[buildingIndex]
                );
        }
    } else if (
        header.version == 6 ||
        v7UsesLegacyLayout
    ) {
        BuildingSaveV7 legacyBuildings[
            MAX_BUILDINGS
        ] = {};

        if (
            fread(
                legacyBuildings,
                sizeof(BuildingSaveV7),
                MAX_BUILDINGS,
                file
            ) != MAX_BUILDINGS
        ) {
            fclose(file);
            return false;
        }

        for (
            int buildingIndex = 0;
            buildingIndex < MAX_BUILDINGS;
            ++buildingIndex
        ) {
            loadedBuildings[buildingIndex] =
                migrateBuildingSaveV7(
                    legacyBuildings[buildingIndex]
                );
        }
    } else {
        BuildingSaveV5 legacyBuildings[
            MAX_BUILDINGS
        ] = {};

        if (
            fread(
                legacyBuildings,
                sizeof(BuildingSaveV5),
                MAX_BUILDINGS,
                file
            ) != MAX_BUILDINGS
        ) {
            fclose(file);
            return false;
        }

        for (
            int buildingIndex = 0;
            buildingIndex < MAX_BUILDINGS;
            ++buildingIndex
        ) {
            loadedBuildings[buildingIndex] =
                migrateBuildingSaveV5(
                    legacyBuildings[buildingIndex]
                );
        }
    }

    if (
        header.version == 7 ||
        header.version == 8 ||
        header.version == 9 ||
        header.version == 10 ||
        header.version == 11 ||
        header.version == 12
    ) {
        WareDistribution loadedDistribution = {};

        if (
            fread(
                &loadedDistribution,
                sizeof(WareDistribution),
                1,
                file
            ) != 1
        ) {
            fclose(file);
            return false;
        }

        if (wareDistribution) {
            for (
                int wareIndex = 0;
                wareIndex < WARE_RATIO_TAB_COUNT;
                ++wareIndex
            ) {
                for (
                    int targetIndex = 0;
                    targetIndex <
                        WARE_RATIO_TARGET_COUNT;
                    ++targetIndex
                ) {
                    // Setter zaroven omezi poskozene
                    // hodnoty do rozsahu 0..5.
                    setWareDistributionValue(
                        wareDistribution,
                        static_cast<WareRatioType>(
                            wareIndex
                        ),
                        targetIndex,
                        loadedDistribution.values[
                            wareIndex
                        ][
                            targetIndex
                        ]
                    );
                }
            }
        }
    }


    // Save v4 mel School frontu pred zavedenim
    // aktivniho queue systemu.
    if (header.version == 4) {
        for (
            int i = 0;
            i < MAX_BUILDINGS;
            ++i
        ) {
            if (
                loadedBuildings[i].active &&
                loadedBuildings[i].type ==
                    BuildingType::School
            ) {
                for (
                    int queueIndex = 0;
                    queueIndex < SCHOOL_QUEUE_SIZE;
                    ++queueIndex
                ) {
                    loadedBuildings[i]
                        .schoolQueue[queueIndex] = -1;
                }

                loadedBuildings[i]
                    .schoolTrainingStep = 0;

                loadedBuildings[i]
                    .schoolIdleStep = 0;

                loadedBuildings[i]
                    .schoolTrainingActive = false;

                loadedBuildings[i]
                    .schoolFinishingTraining = false;
            }
        }
    }

    for (
        int i = 0;
        i < MAX_BUILDINGS;
        ++i
    ) {
        if (loadedBuildings[i].active) {
            bool validPosition =
                loadedBuildings[i].tileX >= 0 &&
                loadedBuildings[i].tileY >= 0 &&
                loadedBuildings[i].tileX + 2 <
                    MAP_WIDTH &&
                loadedBuildings[i].tileY + 2 <
                    MAP_HEIGHT;

            if (!validPosition) {
                loadedBuildings[i].active = false;
            }

            // Ochrana poskozeneho save v6.
            if (
                loadedBuildings[i].maxHealth < 1
            ) {
                initializeHouseRuntimeState(
                    &loadedBuildings[i]
                );
            }

            if (
                loadedBuildings[i].health < 0
            ) {
                loadedBuildings[i].health = 0;
            }

            if (
                loadedBuildings[i].health >
                    loadedBuildings[i].maxHealth
            ) {
                loadedBuildings[i].health =
                    loadedBuildings[i].maxHealth;
            }

            // Obecna House inventory save ochrana.
            //
            // Zaporne mnozstvi ani zaporna rezervace
            // dopravy nejsou platne.
            for (
                int slot = 0;
                slot < 4;
                ++slot
            ) {
                if (
                    loadedBuildings[i]
                        .resourceIn[slot] < 0
                ) {
                    loadedBuildings[i]
                        .resourceIn[slot] = 0;
                }

                if (
                    loadedBuildings[i]
                        .resourceDeliveryCount[slot] < 0
                ) {
                    loadedBuildings[i]
                        .resourceDeliveryCount[slot] = 0;
                }

                if (
                    loadedBuildings[i]
                        .resourceOut[slot] < 0
                ) {
                    loadedBuildings[i]
                        .resourceOut[slot] = 0;
                }

                // Bez aktivni delivery queue nelze po loadu
                // bezpecne obnovit rozpracovanou rezervaci.
                // Serf runtime se zatim neuklada.
                loadedBuildings[i]
                    .resourceDeliveryCount[slot] = 0;
            }

            // Bakery production save ochrana.
            //
            // Platny rozpracovany cyklus ma tick 0..377.
            // Tick 378 znamena dokonceni a nesmi zustat
            // ulozen jako aktivni stav.
            if (
                loadedBuildings[i].type ==
                    BuildingType::Bakery
            ) {
                if (
                    loadedBuildings[i]
                        .bakeryProductionTick < 0 ||
                    loadedBuildings[i]
                        .bakeryProductionTick >= 378
                ) {
                    loadedBuildings[i]
                        .bakeryProductionActive = false;

                    loadedBuildings[i]
                        .bakeryProductionTick = 0;
                }

                if (
                    !loadedBuildings[i]
                        .bakeryProductionActive
                ) {
                    loadedBuildings[i]
                        .bakeryProductionTick = 0;
                }
            } else {
                loadedBuildings[i]
                    .bakeryProductionActive = false;

                loadedBuildings[i]
                    .bakeryProductionTick = 0;
            }

            // Store save ochrana:
            // zaporne mnozstvi ware neni platne.
            if (
                loadedBuildings[i].type ==
                    BuildingType::Store
            ) {
                for (
                    int wareIndex = 0;
                    wareIndex < STORE_WARE_COUNT;
                    ++wareIndex
                ) {
                    if (
                        loadedBuildings[i]
                            .storeWares[wareIndex] < 0
                    ) {
                        loadedBuildings[i]
                            .storeWares[wareIndex] = 0;
                    }
                }
            }
        }

        buildings[i] = loadedBuildings[i];
    }

    *buildingCount = header.buildingCount;

    // =====================================================
    // SAVE V11 UNIT READ
    //
    // v4-v10 nemaji Unit blok a zachovaji bootstrap units.
    // v11 obnovi unitCount a runtime jednotek z pevneho
    // UnitSaveV11 formatu v samostatnem save modulu.
    // =====================================================

    if (
        header.version >= 11 &&
        !readUnitsV11(
            file,
            units,
            unitCount,
            header.buildingCount
        )
    ) {
        fclose(file);
        return false;
    }


    // =====================================================
    // SAVE V12 MAP STATE READ
    //
    // v4-v11 mapove vrstvy neobsahovaly.
    // U starsich save zustanou caller arrays ve vychozim
    // new-game stavu.
    //
    // V12 nejdrive nacte a overi docasna pole.
    // Live mapu zmeni az po uspesnem overeni celeho bloku.
    // =====================================================

    if (header.version >= 12) {
        if (
            !roads ||
            !landPlans ||
            !cornGrowthTimers
        ) {
            fclose(file);
            return false;
        }

        constexpr int MAP_TILE_COUNT =
            MAP_WIDTH * MAP_HEIGHT;

        RoadState loadedRoads[
            MAP_TILE_COUNT
        ] = {};

        LandPlanState loadedLandPlans[
            MAP_TILE_COUNT
        ] = {};

        float loadedCornGrowthTimers[
            MAP_TILE_COUNT
        ] = {};


        for (
            int tileIndex = 0;
            tileIndex < MAP_TILE_COUNT;
            ++tileIndex
        ) {
            unsigned char value = 0;

            if (
                fread(
                    &value,
                    sizeof(value),
                    1,
                    file
                ) != 1
            ) {
                fclose(file);
                return false;
            }

            if (
                value >
                    static_cast<unsigned char>(
                        RoadState::Complete
                    )
            ) {
                fclose(file);
                return false;
            }

            loadedRoads[tileIndex] =
                static_cast<RoadState>(
                    value
                );
        }


        for (
            int tileIndex = 0;
            tileIndex < MAP_TILE_COUNT;
            ++tileIndex
        ) {
            unsigned char value = 0;

            if (
                fread(
                    &value,
                    sizeof(value),
                    1,
                    file
                ) != 1
            ) {
                fclose(file);
                return false;
            }

            if (
                value >
                    static_cast<unsigned char>(
                        LandPlanState::FieldCut
                    )
            ) {
                fclose(file);
                return false;
            }

            loadedLandPlans[tileIndex] =
                static_cast<LandPlanState>(
                    value
                );
        }


        if (
            fread(
                loadedCornGrowthTimers,
                sizeof(float),
                MAP_TILE_COUNT,
                file
            ) != MAP_TILE_COUNT
        ) {
            fclose(file);
            return false;
        }


        for (
            int tileIndex = 0;
            tileIndex < MAP_TILE_COUNT;
            ++tileIndex
        ) {
            const float timer =
                loadedCornGrowthTimers[
                    tileIndex
                ];

            if (
                !std::isfinite(timer) ||
                timer < 0.0f
            ) {
                fclose(file);
                return false;
            }
        }


        for (
            int tileIndex = 0;
            tileIndex < MAP_TILE_COUNT;
            ++tileIndex
        ) {
            roads[tileIndex] =
                loadedRoads[tileIndex];

            landPlans[tileIndex] =
                loadedLandPlans[tileIndex];

            cornGrowthTimers[tileIndex] =
                loadedCornGrowthTimers[
                    tileIndex
                ];
        }
    }


    fclose(file);

    return true;
}


// DEV UNIT MOVE BEGIN
//
// Vypnutim teto jedine konstanty zustane zachovan:
// - vyber jednotky
// - Unit GUI
// - selection marker
//
// Zrusi se pouze rucni testovaci pohyb.
static constexpr bool ENABLE_DEV_UNIT_MOVE = true;

// DEV UNIT MOVE END


enum class BuildTool {
    Road,
    Field,
    Wine,
    Building,
    Remove
};


// LandPlanState je sdileny s Worker state machine
// a je deklarovany v game/units.h.

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "KaM Vita",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        0
    );

    if (!window) {
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED
    );

    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    // =====================================================
    // ORIGINAL KAM DATA RUNTIME DIAGNOSTIC
    //
    // Experimental KAMV00002 only.
    //
    // Original game data stays external at:
    //
    // ux0:data/kamvita_runtime/original/data
    //
    // This checkpoint is read-only.
    // Current BMP assets remain authoritative.
    // Failure to load original data is non-fatal.
    // =====================================================

    OriginalKaM::Resources originalKaMResources;

    const char* originalKaMDataRoot =
        "ux0:data/kamvita_runtime/original/data";

    const bool originalKaMReady =
        originalKaMResources.initialize(
            originalKaMDataRoot
        );

    FILE* originalDataDiagnostic =
        fopen(
            "ux0:data/kamvita_runtime/"
            "original_data_runtime.txt",
            "w"
        );

    if (originalDataDiagnostic) {
        fprintf(
            originalDataDiagnostic,
            "DATARUNTIME_TITLE_ID=KAMV00002\n"
        );

        fprintf(
            originalDataDiagnostic,
            "DATA_ROOT=%s\n",
            originalKaMDataRoot
        );

        fprintf(
            originalDataDiagnostic,
            "INITIALIZE=%s\n",
            originalKaMReady
                ? "OK"
                : "FAILED"
        );

        if (originalKaMReady) {
            OriginalKaM::HouseDatRecord mill;

            const bool millRecordOk =
                originalKaMResources.readMillRecord(
                    &mill
                );

            fprintf(
                originalDataDiagnostic,
                "MILL_RECORD=%s\n",
                millRecordOk
                    ? "OK"
                    : "FAILED"
            );

            if (millRecordOk) {
                fprintf(
                    originalDataDiagnostic,
                    "MILL_DAT_INDEX=%d\n",
                    mill.datIndex
                );

                fprintf(
                    originalDataDiagnostic,
                    "MILL_STONE_RX=%d\n",
                    mill.stonePicRx
                );

                fprintf(
                    originalDataDiagnostic,
                    "MILL_WOOD_RX=%d\n",
                    mill.woodPicRx
                );

                fprintf(
                    originalDataDiagnostic,
                    "MILL_WOOD_STEPS=%d\n",
                    mill.woodSteps
                );

                fprintf(
                    originalDataDiagnostic,
                    "MILL_STONE_STEPS=%d\n",
                    mill.stoneSteps
                );

                fprintf(
                    originalDataDiagnostic,
                    "MILL_WOOD_COST=%d\n",
                    mill.woodCost
                );

                fprintf(
                    originalDataDiagnostic,
                    "MILL_STONE_COST=%d\n",
                    mill.stoneCost
                );

                fprintf(
                    originalDataDiagnostic,
                    "MILL_ENTRANCE_PX=%d,%d\n",
                    mill.entrancePixelX,
                    mill.entrancePixelY
                );
            }

            OriginalKaM::RxSprite millStone;

            const bool millStoneOk =
                millRecordOk &&
                originalKaMResources.loadRxSprite(
                    mill.stonePicRx,
                    &millStone
                );

            fprintf(
                originalDataDiagnostic,
                "MILL_STONE_SPRITE=%s\n",
                millStoneOk
                    ? "OK"
                    : "FAILED"
            );

            if (millStoneOk) {
                fprintf(
                    originalDataDiagnostic,
                    "MILL_STONE_SIZE=%d,%d\n",
                    millStone.width,
                    millStone.height
                );

                fprintf(
                    originalDataDiagnostic,
                    "MILL_STONE_PIVOT=%d,%d\n",
                    millStone.pivotX,
                    millStone.pivotY
                );

                fprintf(
                    originalDataDiagnostic,
                    "MILL_STONE_RGBA_PIXELS=%lu\n",
                    static_cast<unsigned long>(
                        millStone.rgbaPixels.size()
                    )
                );
            }
        } else {
            fprintf(
                originalDataDiagnostic,
                "ERROR=%s\n",
                originalKaMResources
                    .getLastError()
                    .c_str()
            );

            fprintf(
                originalDataDiagnostic,
                "BMP_FALLBACK=ACTIVE\n"
            );
        }

        fprintf(
            originalDataDiagnostic,
            "BMP_RENDERER=UNCHANGED\n"
        );

        fclose(
            originalDataDiagnostic
        );

        originalDataDiagnostic = nullptr;
    }

    // =====================================================
    // REMAKE-STYLE SAVE DIRECTORY ENUMERATION
    //
    // KaM Remake builds its Load/Save list from named save
    // files in the Saves directory.
    //
    // This checkpoint only verifies the Vita directory layer.
    //
    // No .sav is automatically selected, loaded or created.
    // Existing save.dat bootstrap behaviour stays unchanged.
    // =====================================================

    SaveFileEntry startupSaveFiles[
        MAX_SAVE_FILES
    ] = {};

    const int startupSaveFileCount =
        enumerateSaveFiles(
            startupSaveFiles,
            MAX_SAVE_FILES
        );

    FILE* saveSlotsScan =
        fopen(
            "ux0:data/kamvita/save_slots_scan.txt",
            "w"
        );

    if (saveSlotsScan) {
        fprintf(
            saveSlotsScan,
            "SAVE_DIRECTORY=ux0:data/kamvita/Saves\n"
        );

        fprintf(
            saveSlotsScan,
            "SAVE_COUNT=%d\n",
            startupSaveFileCount
        );

        for (
            int saveIndex = 0;
            saveIndex < startupSaveFileCount;
            ++saveIndex
        ) {
            fprintf(
                saveSlotsScan,
                "SAVE[%d].NAME=%s\n",
                saveIndex,
                startupSaveFiles[
                    saveIndex
                ].name
            );

            fprintf(
                saveSlotsScan,
                "SAVE[%d].PATH=%s\n",
                saveIndex,
                startupSaveFiles[
                    saveIndex
                ].path
            );
        }

        fclose(
            saveSlotsScan
        );

        saveSlotsScan = nullptr;
    }


    // =====================================================
    // DAT-DRIVEN HOUSE COSTS ONLY
    //
    // Vita-verified DataRuntime 014 baseline plus one
    // isolated metadata migration:
    //
    // WoodCost + StoneCost <- external houses.dat
    //
    // Construction steps, entrances and render geometry
    // remain unchanged.
    // =====================================================

    clearBuildingRuntimeCosts();
    clearBuildingRuntimeSteps();
    clearBuildingRuntimeEntrancePixels();

    int runtimeCostHouseCount = 0;
    int runtimeStepHouseCount = 0;
    int runtimeEntranceHouseCount = 0;

    if (originalKaMReady) {
        struct RuntimeCostEntry {
            OriginalKaM::HouseType source;
            BuildingType target;
        };

        const RuntimeCostEntry entries[] = {
            {
                OriginalKaM::HouseType::Store,
                BuildingType::Store
            },
            {
                OriginalKaM::HouseType::Bakery,
                BuildingType::Bakery
            },
            {
                OriginalKaM::HouseType::Farm,
                BuildingType::Farm
            },
            {
                OriginalKaM::HouseType::School,
                BuildingType::School
            },
            {
                OriginalKaM::HouseType::Quarry,
                BuildingType::Quarry
            },
            {
                OriginalKaM::HouseType::Mill,
                BuildingType::Mill
            }
        };

        const int entryCount =
            static_cast<int>(
                sizeof(entries) /
                sizeof(entries[0])
            );

        for (
            int index = 0;
            index < entryCount;
            ++index
        ) {
            OriginalKaM::HouseDatRecord record;

            if (
                !originalKaMResources
                    .readHouseRecord(
                        entries[index].source,
                        &record
                    )
            ) {
                continue;
            }

            if (
                setBuildingRuntimeCosts(
                    entries[index].target,
                    record.woodCost,
                    record.stoneCost
                )
            ) {
                ++runtimeCostHouseCount;
            }

            if (
                setBuildingRuntimeSteps(
                    entries[index].target,
                    record.woodSteps,
                    record.stoneSteps
                )
            ) {
                ++runtimeStepHouseCount;
            }

            if (
                setBuildingRuntimeEntrancePixels(
                    entries[index].target,
                    record.entrancePixelX,
                    record.entrancePixelY
                )
            ) {
                ++runtimeEntranceHouseCount;
            }
        }
    }

    FILE* costDiagnostic =
        fopen(
            "ux0:data/kamvita_runtime/"
            "original_cost_runtime.txt",
            "w"
        );

    if (costDiagnostic) {
        fprintf(
            costDiagnostic,
            "TITLE_ID=KAMV00002\n"
        );

        fprintf(
            costDiagnostic,
            "ORIGINAL_DATA_READY=%s\n",
            originalKaMReady
                ? "YES"
                : "NO"
        );

        fprintf(
            costDiagnostic,
            "RUNTIME_COST_HOUSE_COUNT=%d\n",
            runtimeCostHouseCount
        );

        fprintf(
            costDiagnostic,
            "EXPECTED_COST_HOUSE_COUNT=6\n"
        );

        fprintf(
            costDiagnostic,
            "COST_SOURCE=%s\n",
            runtimeCostHouseCount == 6
                ? "EXTERNAL_HOUSES_DAT"
                : (
                    runtimeCostHouseCount > 0
                        ? "HYBRID"
                        : "HARDCODED_FALLBACK"
                )
        );

        fprintf(
            costDiagnostic,
            "RUNTIME_STEP_HOUSE_COUNT=%d\n",
            runtimeStepHouseCount
        );

        fprintf(
            costDiagnostic,
            "EXPECTED_STEP_HOUSE_COUNT=6\n"
        );

        fprintf(
            costDiagnostic,
            "CONSTRUCTION_STEPS_SOURCE=%s\n",
            runtimeStepHouseCount == 6
                ? "EXTERNAL_HOUSES_DAT"
                : (
                    runtimeStepHouseCount > 0
                        ? "HYBRID"
                        : "HARDCODED_FALLBACK"
                )
        );

        fprintf(
            costDiagnostic,
            "RUNTIME_ENTRANCE_HOUSE_COUNT=%d\n",
            runtimeEntranceHouseCount
        );

        fprintf(
            costDiagnostic,
            "EXPECTED_ENTRANCE_HOUSE_COUNT=6\n"
        );

        fprintf(
            costDiagnostic,
            "ENTRANCE_PIXEL_SOURCE=%s\n",
            runtimeEntranceHouseCount == 6
                ? "EXTERNAL_HOUSES_DAT"
                : (
                    runtimeEntranceHouseCount > 0
                        ? "HYBRID"
                        : "HARDCODED_FALLBACK"
                )
        );

        fprintf(
            costDiagnostic,
            "ENTRANCE_TILE_SOURCE="
            "KAMVITA_EXISTING\n"
        );

        fprintf(
            costDiagnostic,
            "HOUSE_EDGE_BOUNDS_FIX="
            "ACTIVE\n"
        );

        fclose(costDiagnostic);
        costDiagnostic = nullptr;
    }

    TerrainResources terrain = loadTerrainResources(renderer);

    KaMPlanResources kamPlanResources =
        loadKaMPlanResources(renderer);
    BuildingResources buildingResources = loadBuildingResources(renderer);

    // =====================================================
    // MILL STONEPIC SDL RUNTIME TEXTURE
    //
    // First visual proof that KAMV00002 renders the
    // completed Mill base directly from the StonePic
    // selected by the original Mill houses.dat record.
    //
    // The construction atlases remain packaged fallbacks.
    // =====================================================

    bool runtimeMillStoneTextureReady = false;

    int runtimeMillDatIndex = -1;
    int runtimeMillStoneRx = 0;

    int runtimeMillStoneWidth = 0;
    int runtimeMillStoneHeight = 0;

    int runtimeMillStonePivotX = 0;
    int runtimeMillStonePivotY = 0;

    if (originalKaMReady) {
        OriginalKaM::HouseDatRecord runtimeMillRecord;
        OriginalKaM::RxSprite runtimeMillStone;

        const bool runtimeMillRecordReady =
            originalKaMResources.readHouseRecord(
                OriginalKaM::HouseType::Mill,
                &runtimeMillRecord
            );

        bool runtimeMillStoneDecoded = false;

        if (runtimeMillRecordReady) {
            runtimeMillDatIndex =
                runtimeMillRecord.datIndex;

            runtimeMillStoneRx =
                runtimeMillRecord.stonePicRx;

            runtimeMillStoneDecoded =
                originalKaMResources.loadRxSprite(
                    runtimeMillStoneRx,
                    &runtimeMillStone
                );
        }

        if (runtimeMillStoneDecoded) {
            runtimeMillStoneWidth =
                runtimeMillStone.width;

            runtimeMillStoneHeight =
                runtimeMillStone.height;

            runtimeMillStonePivotX =
                runtimeMillStone.pivotX;

            runtimeMillStonePivotY =
                runtimeMillStone.pivotY;
        }

        const std::size_t expectedPixelCount =
            runtimeMillStoneDecoded
                ? static_cast<std::size_t>(
                    runtimeMillStone.width
                ) *
                    static_cast<std::size_t>(
                        runtimeMillStone.height
                    )
                : 0;

        if (
            runtimeMillStoneDecoded &&
            runtimeMillStone.width > 0 &&
            runtimeMillStone.height > 0 &&
            runtimeMillStone.rgbaPixels.size() ==
                expectedPixelCount &&
            !runtimeMillStone.rgbaPixels.empty()
        ) {
            SDL_Texture* runtimeTexture =
                SDL_CreateTexture(
                    renderer,
                    SDL_PIXELFORMAT_ABGR8888,
                    SDL_TEXTUREACCESS_STATIC,
                    runtimeMillStone.width,
                    runtimeMillStone.height
                );

            if (runtimeTexture) {
                const int texturePitch =
                    runtimeMillStone.width *
                    static_cast<int>(
                        sizeof(std::uint32_t)
                    );

                const void* runtimePixelData =
                    static_cast<const void*>(
                        &runtimeMillStone
                            .rgbaPixels[0]
                    );

                const int updateResult =
                    SDL_UpdateTexture(
                        runtimeTexture,
                        nullptr,
                        runtimePixelData,
                        texturePitch
                    );

                if (updateResult == 0) {
                    SDL_SetTextureBlendMode(
                        runtimeTexture,
                        SDL_BLENDMODE_BLEND
                    );

                    buildingResources
                        .millRuntimeStoneTexture =
                            runtimeTexture;

                    buildingResources
                        .millRuntimeStoneWidth =
                            runtimeMillStone.width;

                    buildingResources
                        .millRuntimeStoneHeight =
                            runtimeMillStone.height;

                    buildingResources
                        .millRuntimeStonePivotX =
                            runtimeMillStone.pivotX;

                    buildingResources
                        .millRuntimeStonePivotY =
                            runtimeMillStone.pivotY;

                    runtimeMillStoneTextureReady =
                        true;
                } else {
                    SDL_DestroyTexture(
                        runtimeTexture
                    );

                    runtimeTexture = nullptr;
                }
            }
        }
    }

    // =====================================================
    // MILL WOODPIC SDL RUNTIME TEXTURE
    //
    // The final wooden construction phase is decoded
    // directly from the WoodPic selected by houses.dat.
    // The packaged BMP atlas remains the fallback.
    // =====================================================

    bool runtimeMillWoodTextureReady = false;

    int runtimeMillWoodRx = 0;
    int runtimeMillWoodWidth = 0;
    int runtimeMillWoodHeight = 0;
    int runtimeMillWoodPivotX = 0;
    int runtimeMillWoodPivotY = 0;

    if (originalKaMReady) {
        OriginalKaM::HouseDatRecord runtimeMillRecord;
        OriginalKaM::RxSprite runtimeMillWood;

        const bool runtimeMillRecordReady =
            originalKaMResources.readHouseRecord(
                OriginalKaM::HouseType::Mill,
                &runtimeMillRecord
            );

        bool runtimeMillWoodDecoded = false;

        if (runtimeMillRecordReady) {
            runtimeMillWoodRx =
                runtimeMillRecord.woodPicRx;

            runtimeMillWoodDecoded =
                originalKaMResources.loadRxSprite(
                    runtimeMillWoodRx,
                    &runtimeMillWood
                );
        }

        if (runtimeMillWoodDecoded) {
            runtimeMillWoodWidth =
                runtimeMillWood.width;

            runtimeMillWoodHeight =
                runtimeMillWood.height;

            runtimeMillWoodPivotX =
                runtimeMillWood.pivotX;

            runtimeMillWoodPivotY =
                runtimeMillWood.pivotY;
        }

        const std::size_t expectedWoodPixelCount =
            runtimeMillWoodDecoded
                ? static_cast<std::size_t>(
                    runtimeMillWood.width
                ) *
                    static_cast<std::size_t>(
                        runtimeMillWood.height
                    )
                : 0;

        if (
            runtimeMillWoodDecoded &&
            runtimeMillWood.width > 0 &&
            runtimeMillWood.height > 0 &&
            runtimeMillWood.rgbaPixels.size() ==
                expectedWoodPixelCount &&
            !runtimeMillWood.rgbaPixels.empty()
        ) {
            SDL_Texture* runtimeWoodTexture =
                SDL_CreateTexture(
                    renderer,
                    SDL_PIXELFORMAT_ABGR8888,
                    SDL_TEXTUREACCESS_STATIC,
                    runtimeMillWood.width,
                    runtimeMillWood.height
                );

            if (runtimeWoodTexture) {
                const int texturePitch =
                    runtimeMillWood.width *
                    static_cast<int>(
                        sizeof(std::uint32_t)
                    );

                const void* runtimePixelData =
                    static_cast<const void*>(
                        &runtimeMillWood.rgbaPixels[0]
                    );

                const int updateResult =
                    SDL_UpdateTexture(
                        runtimeWoodTexture,
                        nullptr,
                        runtimePixelData,
                        texturePitch
                    );

                if (updateResult == 0) {
                    SDL_SetTextureBlendMode(
                        runtimeWoodTexture,
                        SDL_BLENDMODE_BLEND
                    );

                    buildingResources
                        .millRuntimeWoodTexture =
                            runtimeWoodTexture;

                    buildingResources
                        .millRuntimeWoodWidth =
                            runtimeMillWood.width;

                    buildingResources
                        .millRuntimeWoodHeight =
                            runtimeMillWood.height;

                    buildingResources
                        .millRuntimeWoodPivotX =
                            runtimeMillWood.pivotX;

                    buildingResources
                        .millRuntimeWoodPivotY =
                            runtimeMillWood.pivotY;

                    runtimeMillWoodTextureReady = true;
                } else {
                    SDL_DestroyTexture(
                        runtimeWoodTexture
                    );

                    runtimeWoodTexture = nullptr;
                }
            }
        }
    }

    // =====================================================
    // MILL WOODMASK RUNTIME CONSTRUCTION
    //
    // Generate steps 1..20 completely at runtime from:
    //
    //   WoodPic  = houses.dat -> RX359
    //   WoodMask = houses.dat -> RX361
    //
    // Original mask rule:
    //
    //   mask == 0    -> hidden
    //   mask >= step -> hidden
    //
    // therefore:
    //
    //   visible = mask != 0 && mask < step
    //
    // Step 21 remains the direct full WoodPic RX359 texture
    // introduced by DataRuntime 019.
    // =====================================================

    bool runtimeMillWoodMaskReady = false;
    int runtimeMillWoodMaskRx = 0;
    int runtimeMillWoodMaskWidth = 0;
    int runtimeMillWoodMaskHeight = 0;
    int runtimeMillWoodMaskPivotX = 0;
    int runtimeMillWoodMaskPivotY = 0;
    int runtimeMillWoodMaskFramesReady = 0;

    if (originalKaMReady) {
        OriginalKaM::HouseDatRecord runtimeMaskRecord;
        OriginalKaM::RxSprite runtimeMaskWood;
        OriginalKaM::RxSprite runtimeMask;

        const bool runtimeMaskRecordReady =
            originalKaMResources.readHouseRecord(
                OriginalKaM::HouseType::Mill,
                &runtimeMaskRecord
            );

        bool runtimeMaskWoodDecoded = false;
        bool runtimeMaskDecoded = false;

        if (runtimeMaskRecordReady) {
            runtimeMillWoodMaskRx =
                runtimeMaskRecord.woodPalRx;

            runtimeMaskWoodDecoded =
                originalKaMResources.loadRxSprite(
                    runtimeMaskRecord.woodPicRx,
                    &runtimeMaskWood
                );

            runtimeMaskDecoded =
                originalKaMResources.loadRxSprite(
                    runtimeMillWoodMaskRx,
                    &runtimeMask
                );
        }

        if (runtimeMaskDecoded) {
            runtimeMillWoodMaskWidth =
                runtimeMask.width;

            runtimeMillWoodMaskHeight =
                runtimeMask.height;

            runtimeMillWoodMaskPivotX =
                runtimeMask.pivotX;

            runtimeMillWoodMaskPivotY =
                runtimeMask.pivotY;
        }

        const bool runtimeMaskGeometryReady =
            runtimeMaskWoodDecoded &&
            runtimeMaskDecoded &&
            runtimeMaskWood.width > 0 &&
            runtimeMaskWood.height > 0 &&
            runtimeMaskWood.width ==
                runtimeMask.width &&
            runtimeMaskWood.height ==
                runtimeMask.height &&
            runtimeMaskWood.pivotX ==
                runtimeMask.pivotX &&
            runtimeMaskWood.pivotY ==
                runtimeMask.pivotY &&
            runtimeMaskWood.indexedPixels.size() ==
                runtimeMask.indexedPixels.size() &&
            runtimeMaskWood.rgbaPixels.size() ==
                runtimeMask.indexedPixels.size();

        if (
            runtimeMaskGeometryReady &&
            runtimeMaskRecord.woodSteps > 0 &&
            runtimeMaskRecord.woodSteps <= 21
        ) {
            runtimeMillWoodMaskReady = true;

            const int runtimeWoodStepCount =
                runtimeMaskRecord.woodSteps;

            for (
                int constructionStep = 1;
                constructionStep <
                    runtimeWoodStepCount;
                ++constructionStep
            ) {
                std::vector<std::uint32_t>
                    runtimeStepPixels(
                        runtimeMaskWood
                            .rgbaPixels.size(),
                        static_cast<std::uint32_t>(0)
                    );

                for (
                    std::size_t pixelIndex = 0;
                    pixelIndex <
                        runtimeStepPixels.size();
                    ++pixelIndex
                ) {
                    const std::uint8_t maskValue =
                        runtimeMask.indexedPixels[
                            pixelIndex
                        ];

                    if (
                        maskValue != 0 &&
                        maskValue <
                            constructionStep
                    ) {
                        runtimeStepPixels[pixelIndex] =
                            runtimeMaskWood
                                .rgbaPixels[
                                    pixelIndex
                                ];
                    }
                }

                SDL_Texture* runtimeStepTexture =
                    SDL_CreateTexture(
                        renderer,
                        SDL_PIXELFORMAT_ABGR8888,
                        SDL_TEXTUREACCESS_STATIC,
                        runtimeMaskWood.width,
                        runtimeMaskWood.height
                    );

                if (!runtimeStepTexture)
                    continue;

                const int runtimeStepPitch =
                    runtimeMaskWood.width *
                    static_cast<int>(
                        sizeof(std::uint32_t)
                    );

                const int runtimeStepUpdate =
                    SDL_UpdateTexture(
                        runtimeStepTexture,
                        nullptr,
                        static_cast<const void*>(
                            &runtimeStepPixels[0]
                        ),
                        runtimeStepPitch
                    );

                if (runtimeStepUpdate != 0) {
                    SDL_DestroyTexture(
                        runtimeStepTexture
                    );
                    continue;
                }

                SDL_SetTextureBlendMode(
                    runtimeStepTexture,
                    SDL_BLENDMODE_BLEND
                );

                buildingResources
                    .millRuntimeWoodSteps[
                        constructionStep
                    ] =
                        runtimeStepTexture;

                ++runtimeMillWoodMaskFramesReady;
            }
        }
    }

    // =====================================================
    // MILL STONEMASK RUNTIME CONSTRUCTION
    //
    // Generate construction steps 1..30 directly from:
    //
    //   StonePic  = houses.dat -> RX358
    //   StoneMask = houses.dat -> RX360
    //
    // Original construction mask rule:
    //
    //   mask == 0    -> hidden
    //   mask >= step -> hidden
    //
    // visible = mask != 0 && mask < step
    //
    // Step 31 remains complete StonePic RX358.
    // =====================================================

    bool runtimeMillStoneMaskReady = false;

    int runtimeMillStoneMaskRx = 0;
    int runtimeMillStoneMaskWidth = 0;
    int runtimeMillStoneMaskHeight = 0;
    int runtimeMillStoneMaskPivotX = 0;
    int runtimeMillStoneMaskPivotY = 0;
    int runtimeMillStoneMaskFramesReady = 0;

    if (originalKaMReady) {
        OriginalKaM::HouseDatRecord runtimeStoneMaskRecord;
        OriginalKaM::RxSprite runtimeMaskStone;
        OriginalKaM::RxSprite runtimeStoneMask;

        const bool runtimeStoneMaskRecordReady =
            originalKaMResources.readHouseRecord(
                OriginalKaM::HouseType::Mill,
                &runtimeStoneMaskRecord
            );

        bool runtimeMaskStoneDecoded = false;
        bool runtimeStoneMaskDecoded = false;

        if (runtimeStoneMaskRecordReady) {
            runtimeMillStoneMaskRx =
                runtimeStoneMaskRecord.stonePalRx;

            runtimeMaskStoneDecoded =
                originalKaMResources.loadRxSprite(
                    runtimeStoneMaskRecord.stonePicRx,
                    &runtimeMaskStone
                );

            runtimeStoneMaskDecoded =
                originalKaMResources.loadRxSprite(
                    runtimeMillStoneMaskRx,
                    &runtimeStoneMask
                );
        }

        if (runtimeStoneMaskDecoded) {
            runtimeMillStoneMaskWidth =
                runtimeStoneMask.width;

            runtimeMillStoneMaskHeight =
                runtimeStoneMask.height;

            runtimeMillStoneMaskPivotX =
                runtimeStoneMask.pivotX;

            runtimeMillStoneMaskPivotY =
                runtimeStoneMask.pivotY;
        }

        const bool runtimeStoneMaskGeometryReady =
            runtimeMaskStoneDecoded &&
            runtimeStoneMaskDecoded &&
            runtimeMaskStone.width > 0 &&
            runtimeMaskStone.height > 0 &&
            runtimeMaskStone.width ==
                runtimeStoneMask.width &&
            runtimeMaskStone.height ==
                runtimeStoneMask.height &&
            runtimeMaskStone.pivotX ==
                runtimeStoneMask.pivotX &&
            runtimeMaskStone.pivotY ==
                runtimeStoneMask.pivotY &&
            runtimeMaskStone.indexedPixels.size() ==
                runtimeStoneMask.indexedPixels.size() &&
            runtimeMaskStone.rgbaPixels.size() ==
                runtimeStoneMask.indexedPixels.size() &&
            !runtimeMaskStone.rgbaPixels.empty();

        if (
            runtimeStoneMaskGeometryReady &&
            runtimeStoneMaskRecord.stoneSteps > 0 &&
            runtimeStoneMaskRecord.stoneSteps <= 31
        ) {
            runtimeMillStoneMaskReady = true;

            const int runtimeStoneStepCount =
                runtimeStoneMaskRecord.stoneSteps;

            for (
                int constructionStep = 1;
                constructionStep <
                    runtimeStoneStepCount;
                ++constructionStep
            ) {
                std::vector<std::uint32_t>
                    runtimeStoneStepPixels(
                        runtimeMaskStone.rgbaPixels.size(),
                        static_cast<std::uint32_t>(0)
                    );

                for (
                    std::size_t pixelIndex = 0;
                    pixelIndex <
                        runtimeStoneStepPixels.size();
                    ++pixelIndex
                ) {
                    const std::uint8_t maskValue =
                        runtimeStoneMask.indexedPixels[
                            pixelIndex
                        ];

                    if (
                        maskValue != 0 &&
                        maskValue < constructionStep
                    ) {
                        runtimeStoneStepPixels[
                            pixelIndex
                        ] =
                            runtimeMaskStone.rgbaPixels[
                                pixelIndex
                            ];
                    }
                }

                SDL_Texture* runtimeStoneStepTexture =
                    SDL_CreateTexture(
                        renderer,
                        SDL_PIXELFORMAT_ABGR8888,
                        SDL_TEXTUREACCESS_STATIC,
                        runtimeMaskStone.width,
                        runtimeMaskStone.height
                    );

                if (!runtimeStoneStepTexture)
                    continue;

                const int runtimeStoneStepPitch =
                    runtimeMaskStone.width *
                    static_cast<int>(
                        sizeof(std::uint32_t)
                    );

                const int runtimeStoneStepUpdate =
                    SDL_UpdateTexture(
                        runtimeStoneStepTexture,
                        nullptr,
                        static_cast<const void*>(
                            &runtimeStoneStepPixels[0]
                        ),
                        runtimeStoneStepPitch
                    );

                if (runtimeStoneStepUpdate != 0) {
                    SDL_DestroyTexture(
                        runtimeStoneStepTexture
                    );
                    continue;
                }

                SDL_SetTextureBlendMode(
                    runtimeStoneStepTexture,
                    SDL_BLENDMODE_BLEND
                );

                buildingResources
                    .millRuntimeStoneSteps[
                        constructionStep
                    ] =
                        runtimeStoneStepTexture;

                ++runtimeMillStoneMaskFramesReady;
            }
        }
    }

    UnitResources unitResources = loadUnitResources(renderer);

    FILE* runtimeTextureDiagnostic =
        fopen(
            "ux0:data/kamvita_runtime/"
            "original_data_texture.txt",
            "w"
        );

    if (runtimeTextureDiagnostic) {
        fprintf(
            runtimeTextureDiagnostic,
            "TITLE_ID=KAMV00002\n"
        );

        fprintf(
            runtimeTextureDiagnostic,
            "ORIGINAL_DATA_READY=%s\n",
            originalKaMReady
                ? "YES"
                : "NO"
        );

        fprintf(
            runtimeTextureDiagnostic,
            "MILL_DAT_INDEX=%d\n",
            runtimeMillDatIndex
        );

        fprintf(
            runtimeTextureDiagnostic,
            "MILL_STONE_RX=%d\n",
            runtimeMillStoneRx
        );

        fprintf(
            runtimeTextureDiagnostic,
            "MILL_STONE_TEXTURE=%s\n",
            runtimeMillStoneTextureReady
                ? "OK"
                : "FALLBACK"
        );

        fprintf(
            runtimeTextureDiagnostic,
            "MILL_STONE_SIZE=%d,%d\n",
            runtimeMillStoneWidth,
            runtimeMillStoneHeight
        );

        fprintf(
            runtimeTextureDiagnostic,
            "MILL_STONE_PIVOT=%d,%d\n",
            runtimeMillStonePivotX,
            runtimeMillStonePivotY
        );

        fprintf(
            runtimeTextureDiagnostic,
            "COMPLETED_MILL_SOURCE=%s\n",
            runtimeMillStoneTextureReady
                ? "EXTERNAL_HOUSES_RX"
                : "PACKAGED_BMP_ATLAS"
        );

        fprintf(
            runtimeTextureDiagnostic,
            "CONSTRUCTION_SOURCE="
            "PACKAGED_BMP_ATLAS\n"
        );

        fprintf(
            runtimeTextureDiagnostic,
            "TURBINE_SOURCE="
            "PACKAGED_BMP_ATLAS\n"
        );

        fprintf(
            runtimeTextureDiagnostic,
            "IDLE_SOURCE="
            "PACKAGED_BMP_ATLAS\n"
        );

        fclose(
            runtimeTextureDiagnostic
        );

        runtimeTextureDiagnostic = nullptr;
    }

    UnitUIResources unitUIResources =
        loadUnitUIResources(renderer);

    UnitUIState unitUI = {};
    initUnitUI(&unitUI);

    unitUI.previousSection =
        GameUISection::Build;

    SchoolUIResources schoolUIResources =
        loadSchoolUIResources(renderer);

    // Vsech 29 originalnich House GUI ikon
    // se nacita centralne pouze jednou.
    HouseIconResources houseIconResources =
        loadHouseIconResources(renderer);

    GameUIState gameUI = {};
    initGameUI(&gameUI);

    // Runtime list of real named .sav files used by
    // Options -> Load Game / Save Game.
    SaveFileList optionsSaveFiles = {};
    initSaveFileList(&optionsSaveFiles);
    refreshSaveFileList(&optionsSaveFiles);

    RuntimeMetalFont runtimeMetalFont = {};

    // Save-name text input state.
    //
    // SDL text input uses the platform text-input backend.
    // On Vita this is the system-supported text entry path.
    bool saveNameEditing = false;
    bool saveNameInputReceived = false;

    // Remake-style Game Settings modal.
    // Visual checkpoint only.
    bool settingsPopupOpen = false;

    if (
        !loadRuntimeMetalFont(
            renderer,
            &runtimeMetalFont
        )
    ) {
        std::printf(
            "Runtime Metal font failed to load\n"
        );
    }

    // Original KaM fnt_Outline Settings headings.
    SDL_Texture* settingsGraphicsText = nullptr;
    SDL_Texture* settingsControlsText = nullptr;

    {
        SDL_Surface* surface = SDL_LoadBMP(
            "app0:assets/ui/settings/text/graphics.bmp"
        );

        if (surface) {
            SDL_SetColorKey(
                surface,
                SDL_TRUE,
                SDL_MapRGB(
                    surface->format,
                    255,
                    0,
                    255
                )
            );

            settingsGraphicsText =
                SDL_CreateTextureFromSurface(
                    renderer,
                    surface
                );

            SDL_FreeSurface(surface);
        }
    }

    {
        SDL_Surface* surface = SDL_LoadBMP(
            "app0:assets/ui/settings/text/controls.bmp"
        );

        if (surface) {
            SDL_SetColorKey(
                surface,
                SDL_TRUE,
                SDL_MapRGB(
                    surface->format,
                    255,
                    0,
                    255
                )
            );

            settingsControlsText =
                SDL_CreateTextureFromSurface(
                    renderer,
                    surface
                );

            SDL_FreeSurface(surface);
        }
    }

    // Original KaM fnt_Outline middle headings.
    SDL_Texture* settingsSoundText = nullptr;
    SDL_Texture* settingsGameplayText = nullptr;

    {
        SDL_Surface* surface =
            SDL_LoadBMP(
                "app0:assets/ui/settings/text/sound.bmp"
            );

        if (surface) {
            SDL_SetColorKey(
                surface,
                SDL_TRUE,
                SDL_MapRGB(
                    surface->format,
                    255,
                    0,
                    255
                )
            );

            settingsSoundText =
                SDL_CreateTextureFromSurface(
                    renderer,
                    surface
                );

            SDL_FreeSurface(surface);
        }
    }

    {
        SDL_Surface* surface =
            SDL_LoadBMP(
                "app0:assets/ui/settings/text/gameplay.bmp"
            );

        if (surface) {
            SDL_SetColorKey(
                surface,
                SDL_TRUE,
                SDL_MapRGB(
                    surface->format,
                    255,
                    0,
                    255
                )
            );

            settingsGameplayText =
                SDL_CreateTextureFromSurface(
                    renderer,
                    surface
                );

            SDL_FreeSurface(surface);
        }
    }

    // Original GUIMain:15 stretched popup parchment.
    SDL_Texture* settingsPopupTexture = nullptr;

    {
        SDL_Surface* surface =
            SDL_LoadBMP(
                "app0:assets/ui/settings/original/"
                "guimain_0015.bmp"
            );

        if (surface) {
            settingsPopupTexture =
                SDL_CreateTextureFromSurface(
                    renderer,
                    surface
                );

            SDL_FreeSurface(surface);

            if (settingsPopupTexture) {
                SDL_SetTextureBlendMode(
                    settingsPopupTexture,
                    SDL_BLENDMODE_BLEND
                );
            }
        }
    }

    // Original GUIMain:9 bsMenu metal background.
    SDL_Texture* settingsMenuButtonTexture = nullptr;

    {
        SDL_Surface* surface =
            SDL_LoadBMP(
                "app0:assets/ui/settings/original/"
                "guimain_0009.bmp"
            );

        if (surface) {
            settingsMenuButtonTexture =
                SDL_CreateTextureFromSurface(
                    renderer,
                    surface
                );

            SDL_FreeSurface(surface);

            if (settingsMenuButtonTexture) {
                SDL_SetTextureBlendMode(
                    settingsMenuButtonTexture,
                    SDL_BLENDMODE_BLEND
                );
            }
        }
    }


    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    sceTouchSetSamplingState(
        SCE_TOUCH_PORT_FRONT,
        SCE_TOUCH_SAMPLING_STATE_START
    );

    SceTouchPanelInfo frontTouchInfo = {};
    sceTouchGetPanelInfo(SCE_TOUCH_PORT_FRONT, &frontTouchInfo);

    float cameraX = (WORLD_WIDTH - SCREEN_WIDTH) / 2.0f;
    float cameraY = (WORLD_HEIGHT - SCREEN_HEIGHT) / 2.0f;

    float cursorX = SCREEN_WIDTH / 2.0f;
    float cursorY = SCREEN_HEIGHT / 2.0f;

    int selectedTileX = -1;
    int selectedTileY = -1;


    Building buildings[MAX_BUILDINGS] = {};
    int buildingCount = 0;

    // Pouze podsekundovy runtime accumulator.
    // Autoritativni produkcni stav a tick jsou v Building
    // a ukladaji se v save v10.
    float bakeryProductionTimers[MAX_BUILDINGS] = {};

    // Stav vystavby cesty pro kazdou dlazdici.
    RoadState roads[MAP_HEIGHT][MAP_WIDTH] = {};

    // Field/Wine planovaci vrstva.
    // V tomto checkpointu pouzivame pouze Field.
    // Wine bude zapojeno v nasledujicim rezu.
    LandPlanState landPlans[
        MAP_HEIGHT
    ][
        MAP_WIDTH
    ] = {};

    // Per-tile runtime age of planted corn in seconds.
    //
    // Remake thresholds:
    // 1400, 2200, 4400, 6400 game ticks.
    //
    // Remake time conversion:
    // 10 game ticks = 1 second.
    float cornGrowthTimers[
        MAP_HEIGHT
    ][
        MAP_WIDTH
    ] = {};

    Unit units[MAX_UNITS] = {};
    int unitCount = 0;

    // -------------------------------------------------
    // Development bootstrap units
    //
    // Pro rychle testovani construction workflow jsou
    // Worker a Serf dostupni ihned po startu hry.
    //
    // School training zustava beze zmeny a pozdeji
    // muzeme tento bootstrap jednoduse odstranit.
    // -------------------------------------------------

    if (unitCount + 2 <= MAX_UNITS) {
        // WORKER
        Unit& worker = units[unitCount++];

        worker = {};
        worker.type = UnitType::Worker;
        worker.state = UnitState::Idle;
        worker.condition = UNIT_START_CONDITION;

        // Pred development Store.
        worker.worldX =
            (28.5f) * TILE_SIZE;

        worker.worldY =
            (32.5f) * TILE_SIZE;

        worker.targetWorldX =
            worker.worldX;

        worker.targetWorldY =
            worker.worldY;

        // Smer S:
        // N, NE, E, SE, S, SW, W, NW.
        worker.direction = 4;

        worker.animationFrame = 0;
        worker.animationTimer = 0.0f;

        worker.pathLength = 0;
        worker.pathIndex = 0;

        worker.targetBuilding = -1;
        worker.targetRoadX = -1;
        worker.targetRoadY = -1;

        worker.workTimer = 0.0f;
        worker.workAction =
            UnitWorkAction::None;

        worker.doorwayProgress = 0.0f;

        worker.doorwayStartX =
            worker.worldX;

        worker.doorwayStartY =
            worker.worldY;

        worker.doorwayTargetX =
            worker.worldX;

        worker.doorwayTargetY =
            worker.worldY;

        worker.doorwayOffsetX = 0;
        worker.doorwayOffsetY = 0;

        worker.visible = true;
        worker.active = true;


        // SERF
        Unit& serf = units[unitCount++];

        serf = {};
        serf.type = UnitType::Serf;
        serf.state = UnitState::Idle;
        serf.condition = UNIT_START_CONDITION;

        // Vedle Workera pred development Store.
        serf.worldX =
            (27.5f) * TILE_SIZE;

        serf.worldY =
            (32.5f) * TILE_SIZE;

        serf.targetWorldX =
            serf.worldX;

        serf.targetWorldY =
            serf.worldY;

        serf.direction = 4;

        serf.animationFrame = 0;
        serf.animationTimer = 0.0f;

        serf.pathLength = 0;
        serf.pathIndex = 0;

        serf.targetBuilding = -1;

        serf.logisticsState =
            SerfLogisticsState::Idle;

        serf.assignedWare =
            WareType::Count;

        serf.carriedWare =
            WareType::Count;

        serf.sourceBuilding = -1;
        serf.destinationBuilding = -1;

        serf.targetRoadX = -1;
        serf.targetRoadY = -1;

        serf.workTimer = 0.0f;
        serf.workAction =
            UnitWorkAction::None;

        serf.doorwayProgress = 0.0f;

        serf.doorwayStartX =
            serf.worldX;

        serf.doorwayStartY =
            serf.worldY;

        serf.doorwayTargetX =
            serf.worldX;

        serf.doorwayTargetY =
            serf.worldY;

        serf.doorwayOffsetX = 0;
        serf.doorwayOffsetY = 0;

        serf.visible = true;
        serf.active = true;
    }

    WareDistribution wareDistribution = {};
    initializeWareDistribution(
        &wareDistribution
    );

    // Zlomkova cast School training kroku.
    // Cely progress je ulozen v Building::schoolTrainingStep
    // a Building::schoolIdleStep.
    float schoolStepTimers[MAX_BUILDINGS] = {};

        const bool gameLoaded =
            loadGame(
                LEGACY_SAVE_PATH,
                buildings,
                &buildingCount,
                &wareDistribution,
                units,
                &unitCount,
                &roads[0][0],
                &landPlans[0][0],
                &cornGrowthTimers[0][0]
            );

    // Development start:
    // pokud neni save, zacneme s hotovym
    // Store a School.
    if (!gameLoaded) {
        buildingCount = 2;

        // STORE
        buildings[0] = {};
        buildings[0].active = true;
        buildings[0].type =
            BuildingType::Store;

        buildings[0].tileX = 27;
        buildings[0].tileY = 29;

        buildings[0].woodStep =
            getBuildingWoodSteps(
                BuildingType::Store
            );

        buildings[0].stoneStep =
            getBuildingStoneSteps(
                BuildingType::Store
            );

        buildings[0].woodDelivered =
            getBuildingWoodCost(
                BuildingType::Store
            );

        buildings[0].stoneDelivered =
            getBuildingStoneCost(
                BuildingType::Store
            );

        initializeHouseRuntimeState(
            &buildings[0]
        );

        // SCHOOL
        buildings[1] = {};
        buildings[1].active = true;
        buildings[1].type =
            BuildingType::School;

        buildings[1].tileX = 32;
        buildings[1].tileY = 29;

        buildings[1].woodStep =
            getBuildingWoodSteps(
                BuildingType::School
            );

        buildings[1].stoneStep =
            getBuildingStoneSteps(
                BuildingType::School
            );

        buildings[1].woodDelivered =
            getBuildingWoodCost(
                BuildingType::School
            );

        buildings[1].stoneDelivered =
            getBuildingStoneCost(
                BuildingType::School
            );

        initializeHouseRuntimeState(
            &buildings[1]
        );

        for (
            int queueIndex = 0;
            queueIndex < SCHOOL_QUEUE_SIZE;
            ++queueIndex
        ) {
            buildings[1].schoolQueue[queueIndex] = -1;
        }

        buildings[1].schoolTrainingStep = 0;
        buildings[1].schoolIdleStep = 0;

        // Docasny development bootstrap.
        // Ekonomika a doprava Gold zatim nejsou implementovane.
        buildings[1].schoolGold =
            SCHOOL_QUEUE_SIZE * SCHOOL_GOLD_COST;

        buildings[1].schoolTrainingActive = false;
        buildings[1].schoolFinishingTraining = false;

    }
    unsigned int previousButtons = 0;
    bool previousTouchDown = false;
    float touchStartX = 0.0f;
    float touchStartY = 0.0f;
    bool touchMoved = false;

    bool buildMenuOpen = false;
    BuildingType selectedBuildingType = BuildingType::Store;
    BuildTool activeBuildTool = BuildTool::Road;
    int buildMenuSelection = 0;

    Uint32 previousFrameTicks = SDL_GetTicks();

    bool running = true;

    while (running) {
        Uint32 currentFrameTicks = SDL_GetTicks();

        float deltaSeconds =
            (currentFrameTicks - previousFrameTicks) /
            1000.0f;

        previousFrameTicks = currentFrameTicks;

        SceCtrlData pad = {};
        sceCtrlPeekBufferPositive(0, &pad, 1);

        SceTouchData touchData = {};
        sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touchData, 1);

        bool touchDown = touchData.reportNum > 0;
        bool touchReleased = !touchDown && previousTouchDown;
        bool touchTapped = false;

        if (touchDown) {
            const SceTouchReport& touch = touchData.report[0];

            int touchWidth = frontTouchInfo.maxDispX - frontTouchInfo.minDispX;
            int touchHeight = frontTouchInfo.maxDispY - frontTouchInfo.minDispY;

            float currentTouchX = (float)(touch.x - frontTouchInfo.minDispX) * SCREEN_WIDTH / touchWidth;
            float currentTouchY = (float)(touch.y - frontTouchInfo.minDispY) * SCREEN_HEIGHT / touchHeight;

            if (!previousTouchDown) {
                touchStartX = currentTouchX;
                touchStartY = currentTouchY;
                touchMoved = false;
            }

            float dx = currentTouchX - touchStartX;
            float dy = currentTouchY - touchStartY;

            if (dx * dx + dy * dy > 400.0f)
                touchMoved = true;
        }

        if (touchReleased && !touchMoved)
            touchTapped = true;

        previousTouchDown = touchDown;

        bool crossPressed =
            (pad.buttons & SCE_CTRL_CROSS) &&
            !(previousButtons & SCE_CTRL_CROSS);

        bool squarePressed =
            (pad.buttons & SCE_CTRL_SQUARE) &&
            !(previousButtons & SCE_CTRL_SQUARE);

        bool circlePressed =
            (pad.buttons & SCE_CTRL_CIRCLE) &&
            !(previousButtons & SCE_CTRL_CIRCLE);

        bool trianglePressed =
            (pad.buttons & SCE_CTRL_TRIANGLE) &&
            !(previousButtons & SCE_CTRL_TRIANGLE);

        bool upPressed =
            (pad.buttons & SCE_CTRL_UP) &&
            !(previousButtons & SCE_CTRL_UP);

        bool downPressed =
            (pad.buttons & SCE_CTRL_DOWN) &&
            !(previousButtons & SCE_CTRL_DOWN);

        bool leftPressed =
            (pad.buttons & SCE_CTRL_LEFT) &&
            !(previousButtons & SCE_CTRL_LEFT);

        bool rightPressed =
            (pad.buttons & SCE_CTRL_RIGHT) &&
            !(previousButtons & SCE_CTRL_RIGHT);

        bool r1Pressed =
            (pad.buttons & SCE_CTRL_RTRIGGER) &&
            !(previousButtons & SCE_CTRL_RTRIGGER);

        bool l1Pressed =
            (pad.buttons & SCE_CTRL_LTRIGGER) &&
            !(previousButtons & SCE_CTRL_LTRIGGER);

        // Navigace GUI.
        if (gameUI.section == GameUISection::School) {
            // Sipka nahoru prepne z carouselu profesi
            // do WIP/queue oblasti.
            if (
                upPressed &&
                !gameUI.schoolQueueFocus
            ) {
                gameUI.schoolQueueFocus = true;
                gameUI.schoolQueueSelection = 0;
            }

            // Sipka dolu vrati focus na carousel profesi.
            if (
                downPressed &&
                gameUI.schoolQueueFocus
            ) {
                gameUI.schoolQueueFocus = false;
            }

            if (gameUI.schoolQueueFocus) {
                if (leftPressed) {
                    gameUI.schoolQueueSelection--;

                    if (
                        gameUI.schoolQueueSelection < 0
                    ) {
                        gameUI.schoolQueueSelection = 0;
                    }
                }

                if (rightPressed) {
                    gameUI.schoolQueueSelection++;

                    if (
                        gameUI.schoolQueueSelection >=
                            SCHOOL_QUEUE_SIZE
                    ) {
                        gameUI.schoolQueueSelection =
                            SCHOOL_QUEUE_SIZE - 1;
                    }
                }
            } else {
                if (leftPressed) {
                    moveSchoolUISelection(
                        &gameUI,
                        -1
                    );
                }

                if (rightPressed) {
                    moveSchoolUISelection(
                        &gameUI,
                        1
                    );
                }
            }
        } else if (
            gameUI.section ==
                GameUISection::Store
        ) {
            int selection =
                gameUI.storeWareSelection;

            if (selection < 0)
                selection = 0;

            if (selection >= STORE_WARE_COUNT)
                selection =
                    STORE_WARE_COUNT - 1;

            const int currentColumn =
                selection % 5;

            if (leftPressed) {
                if (currentColumn > 0)
                    --selection;
            }

            if (rightPressed) {
                if (
                    currentColumn < 4 &&
                    selection + 1 <
                        STORE_WARE_COUNT
                ) {
                    ++selection;
                }
            }

            if (upPressed) {
                if (selection >= 5)
                    selection -= 5;
            }

            if (downPressed) {
                if (
                    selection + 5 <
                        STORE_WARE_COUNT
                ) {
                    selection += 5;
                }
            }

            gameUI.storeWareSelection =
                selection;

            // Store spotreboval navigacni vstup.
            leftPressed = false;
            rightPressed = false;
            upPressed = false;
            downPressed = false;

        } else if (gameUI.buildSectionActive) {
            if (leftPressed) {
                moveGameUISelection(
                    &gameUI,
                    -1,
                    0
                );
            }

            if (rightPressed) {
                moveGameUISelection(
                    &gameUI,
                    1,
                    0
                );
            }

            if (upPressed) {
                moveGameUISelection(
                    &gameUI,
                    0,
                    -1
                );
            }

            if (downPressed) {
                moveGameUISelection(
                    &gameUI,
                    0,
                    1
                );
            }
        }

        if (
            gameUI.section ==
                GameUISection::Ratios
        ) {
            handleGameUIRatiosGamepad(
                &gameUI,
                &wareDistribution,
                leftPressed,
                rightPressed,
                upPressed,
                downPressed,
                crossPressed
            );

            if (touchTapped) {
                if (
                    handleGameUIRatiosTouch(
                        &gameUI,
                        &wareDistribution,
                        touchStartX,
                        touchStartY
                    )
                ) {
                    touchTapped = false;
                }
            }

            // Ratios spotrebovalo navigacni vstupy.
            leftPressed = false;
            rightPressed = false;
            upPressed = false;
            downPressed = false;
            crossPressed = false;
        }

        // -------------------------------------------------
        // STORE WARE ACCEPT / NOT-ACCEPT
        //
        // Remake:
        // kazdy Store ware ma vlastni NotAcceptFlag.
        //
        // Gamepad:
        // D-pad vybere ware, X prepina flag.
        //
        // Touch:
        // tap na konkretni 32x36 ware slot vybere
        // a okamzite prepne jeho flag.
        // -------------------------------------------------

        if (
            gameUI.section ==
                GameUISection::Store &&
            gameUI.selectedStoreBuilding >= 0 &&
            gameUI.selectedStoreBuilding <
                buildingCount
        ) {
            Building& store =
                buildings[
                    gameUI.selectedStoreBuilding
                ];

            const bool validStore =
                store.active &&
                store.type ==
                    BuildingType::Store &&
                isBuildingComplete(store);

            if (validStore) {
                if (crossPressed) {
                    int wareIndex =
                        gameUI.storeWareSelection;

                    if (
                        wareIndex >= 0 &&
                        wareIndex <
                            STORE_WARE_COUNT
                    ) {
                        store.storeNotAccept[
                            wareIndex
                        ] =
                            !store.storeNotAccept[
                                wareIndex
                            ];
                    }

                    // X patri Store GUI a nesmi
                    // propadnout do mapoveho inputu.
                    crossPressed = false;
                }

                if (touchTapped) {
                    const int storePanelX = 6;
                    const int storePanelY = 274;

                    for (
                        int wareIndex = 0;
                        wareIndex <
                            STORE_WARE_COUNT;
                        ++wareIndex
                    ) {
                        const int column =
                            wareIndex % 5;

                        const int row =
                            wareIndex / 5;

                        SDL_Rect wareArea = {
                            storePanelX +
                                2 +
                                column * 36,
                            storePanelY +
                                19 +
                                row * 42,
                            32,
                            36
                        };

                        const bool insideWare =
                            touchStartX >=
                                wareArea.x &&
                            touchStartX <
                                wareArea.x +
                                    wareArea.w &&
                            touchStartY >=
                                wareArea.y &&
                            touchStartY <
                                wareArea.y +
                                    wareArea.h;

                        if (!insideWare)
                            continue;

                        gameUI.storeWareSelection =
                            wareIndex;

                        store.storeNotAccept[
                            wareIndex
                        ] =
                            !store.storeNotAccept[
                                wareIndex
                            ];

                        touchTapped = false;
                        crossPressed = false;
                        break;
                    }
                }
            }
        }

        // -------------------------------------------------
        // STORE HOUSE HEADER INPUT
        //
        // Stejna House header geometrie jako Bakery:
        // Delivery = x 6..35,  y 249..278
        // Repair   = x 36..65, y 249..278
        //
        // Originalni GUI:
        // Delivery 37/38
        // Repair   39/40
        // -------------------------------------------------
        if (
            gameUI.section ==
                GameUISection::Store &&
            gameUI.selectedStoreBuilding >= 0 &&
            gameUI.selectedStoreBuilding <
                buildingCount
        ) {
            Building& store =
                buildings[
                    gameUI.selectedStoreBuilding
                ];

            const bool validStore =
                store.active &&
                store.type ==
                    BuildingType::Store &&
                isBuildingComplete(store);

            if (validStore && touchTapped) {
                const SDL_Rect deliveryArea = {
                    6,
                    249,
                    30,
                    30
                };

                const SDL_Rect repairArea = {
                    36,
                    249,
                    30,
                    30
                };

                const bool insideDelivery =
                    touchStartX >=
                        deliveryArea.x &&
                    touchStartX <
                        deliveryArea.x +
                            deliveryArea.w &&
                    touchStartY >=
                        deliveryArea.y &&
                    touchStartY <
                        deliveryArea.y +
                            deliveryArea.h;

                const bool insideRepair =
                    touchStartX >=
                        repairArea.x &&
                    touchStartX <
                        repairArea.x +
                            repairArea.w &&
                    touchStartY >=
                        repairArea.y &&
                    touchStartY <
                        repairArea.y +
                            repairArea.h;

                if (insideDelivery) {
                    store.wareDeliveryEnabled =
                        !store.wareDeliveryEnabled;

                    touchTapped = false;
                    crossPressed = false;
                } else if (insideRepair) {
                    store.repairEnabled =
                        !store.repairEnabled;

                    touchTapped = false;
                    crossPressed = false;
                }
            }
        }


        // -------------------------------------------------
        // BAKERY HOUSE HEADER INPUT
        //
        // Stejna geometrie jako originalni House header:
        // Delivery = x 6..35,  y 249..278
        // Repair   = x 36..65, y 249..278
        //
        // Remake:
        // Delivery 37/38
        // Repair   39/40
        // -------------------------------------------------
        if (
            gameUI.section ==
                GameUISection::Bakery &&
            gameUI.selectedBakeryBuilding >= 0 &&
            gameUI.selectedBakeryBuilding <
                buildingCount
        ) {
            Building& bakery =
                buildings[
                    gameUI.selectedBakeryBuilding
                ];

            const bool validBakery =
                bakery.active &&
                bakery.type ==
                    BuildingType::Bakery &&
                isBuildingComplete(bakery);

            if (validBakery) {
                const SDL_Rect deliveryArea = {
                    6,
                    249,
                    30,
                    30
                };

                const SDL_Rect repairArea = {
                    36,
                    249,
                    30,
                    30
                };

                if (touchTapped) {
                    const bool insideDelivery =
                        touchStartX >=
                            deliveryArea.x &&
                        touchStartX <
                            deliveryArea.x +
                                deliveryArea.w &&
                        touchStartY >=
                            deliveryArea.y &&
                        touchStartY <
                            deliveryArea.y +
                                deliveryArea.h;

                    const bool insideRepair =
                        touchStartX >=
                            repairArea.x &&
                        touchStartX <
                            repairArea.x +
                                repairArea.w &&
                        touchStartY >=
                            repairArea.y &&
                        touchStartY <
                            repairArea.y +
                                repairArea.h;

                    if (insideDelivery) {
                        bakery.wareDeliveryEnabled =
                            !bakery.wareDeliveryEnabled;

                        touchTapped = false;
                        crossPressed = false;
                    } else if (insideRepair) {
                        bakery.repairEnabled =
                            !bakery.repairEnabled;

                        touchTapped = false;
                        crossPressed = false;
                    }
                }
            }
        }


        // -------------------------------------------------
        // FARM HOUSE HEADER INPUT
        //
        // Stejna geometrie jako Store/Bakery:
        //
        // Delivery = x 6..35,  y 249..278
        // Repair   = x 36..65, y 249..278
        //
        // Renderer pouziva:
        // Delivery 37/38
        // Repair   39/40
        // -------------------------------------------------

        if (
            gameUI.section ==
                GameUISection::Farm &&
            gameUI.selectedFarmBuilding >= 0 &&
            gameUI.selectedFarmBuilding <
                buildingCount
        ) {
            Building& farm =
                buildings[
                    gameUI.selectedFarmBuilding
                ];

            const bool validFarm =
                farm.active &&
                farm.type ==
                    BuildingType::Farm &&
                isBuildingComplete(farm);

            if (validFarm && touchTapped) {
                const SDL_Rect deliveryArea = {
                    6,
                    249,
                    30,
                    30
                };

                const SDL_Rect repairArea = {
                    36,
                    249,
                    30,
                    30
                };

                const bool insideDelivery =
                    touchStartX >=
                        deliveryArea.x &&
                    touchStartX <
                        deliveryArea.x +
                            deliveryArea.w &&
                    touchStartY >=
                        deliveryArea.y &&
                    touchStartY <
                        deliveryArea.y +
                            deliveryArea.h;

                const bool insideRepair =
                    touchStartX >=
                        repairArea.x &&
                    touchStartX <
                        repairArea.x +
                            repairArea.w &&
                    touchStartY >=
                        repairArea.y &&
                    touchStartY <
                        repairArea.y +
                            repairArea.h;

                if (insideDelivery) {
                    farm.wareDeliveryEnabled =
                        !farm.wareDeliveryEnabled;

                    touchTapped = false;
                    crossPressed = false;
                } else if (insideRepair) {
                    farm.repairEnabled =
                        !farm.repairEnabled;

                    touchTapped = false;
                    crossPressed = false;
                }
            }
        }


        // Originalni hlavni KaM toolbar 439..442.
        // Kazdy glyph prepina skutecnou GameUISection.
        if (
            touchTapped &&
            !unitUI.panelVisible
        ) {
            const int toolbarX = 6;
            const int toolbarY = 194;
            const int toolbarStep = 46;
            const int toolbarButtonWidth = 42;
            const int toolbarButtonHeight = 36;

            for (
                int toolbarIndex = 0;
                toolbarIndex < 4;
                ++toolbarIndex
            ) {
                SDL_Rect toolbarArea = {
                    toolbarX +
                        toolbarIndex * toolbarStep,
                    toolbarY,
                    toolbarButtonWidth,
                    toolbarButtonHeight
                };

                bool insideToolbarButton =
                    touchStartX >= toolbarArea.x &&
                    touchStartX <
                        toolbarArea.x +
                            toolbarArea.w &&
                    touchStartY >= toolbarArea.y &&
                    touchStartY <
                        toolbarArea.y +
                            toolbarArea.h;

                if (!insideToolbarButton)
                    continue;

                gameUI.toolbarSelection =
                    toolbarIndex;

                gameUI.toolbarFocus = true;
                gameUI.schoolQueueFocus = false;
                gameUI.schoolQueueSelection = 0;

                switch (toolbarIndex) {
                    case 0:
                        gameUI.section =
                            GameUISection::Build;

                        gameUI.buildSectionActive =
                            true;
                        break;

                    case 1:
                        gameUI.section =
                            GameUISection::Ratios;

                        gameUI.buildSectionActive =
                            false;
                        break;

                    case 2:
                        gameUI.section =
                            GameUISection::Statistics;

                        gameUI.buildSectionActive =
                            false;
                        break;

                    case 3:
                        gameUI.section =
                            GameUISection::Options;

                        gameUI.buildSectionActive =
                            false;

                        gameUI.optionsSelection = 0;
                        gameUI.optionsFocus = true;
                        break;

                    default:
                        break;
                }

                touchTapped = false;
                break;
            }
        }

        // =================================================
        // OPTIONS MENU TOUCH
        //
        // The coordinates must match game_ui.cpp exactly.
        // =================================================

        if (
            gameUI.section ==
                GameUISection::Options &&
            touchTapped
        ) {
            constexpr int optionsPanelX = 9;
            constexpr int optionsPanelWidth = 180;
            constexpr int optionsPanelY = 234;
            constexpr int optionsButtonHeight = 30;

            const int optionsButtonY[4] = {
                optionsPanelY + 20,
                optionsPanelY + 60,
                optionsPanelY + 100,
                optionsPanelY + 160
            };

            for (
                int optionIndex = 0;
                optionIndex < 4;
                ++optionIndex
            ) {
                SDL_Rect optionArea = {
                    optionsPanelX,
                    optionsButtonY[optionIndex],
                    optionsPanelWidth,
                    optionsButtonHeight
                };

                const bool insideOption =
                    touchStartX >= optionArea.x &&
                    touchStartX <
                        optionArea.x +
                            optionArea.w &&
                    touchStartY >= optionArea.y &&
                    touchStartY <
                        optionArea.y +
                            optionArea.h;

                if (!insideOption)
                    continue;

                gameUI.optionsSelection =
                    optionIndex;

                gameUI.optionsFocus = true;
                gameUI.toolbarFocus = false;

                // Touch uses the exact same action path as
                // the gamepad Cross button.
                crossPressed = true;
                touchTapped = false;

                break;
            }
        }


        // Touch ovladani originalniho House headeru.
        if (
            gameUI.section == GameUISection::School &&
            touchTapped &&
            gameUI.selectedSchoolBuilding >= 0 &&
            gameUI.selectedSchoolBuilding < buildingCount
        ) {
            Building& house =
                buildings[
                    gameUI.selectedSchoolBuilding
                ];

            // Zamknuty School layout:
            // panelX=6, panelY=274, House top=198.
            const int houseHeaderY = 249;

            SDL_Rect deliveryArea = {
                6,
                houseHeaderY,
                30,
                30
            };

            SDL_Rect repairArea = {
                36,
                houseHeaderY,
                30,
                30
            };

            auto insideHouseHeader = [
                touchStartX,
                touchStartY
            ](const SDL_Rect& area) -> bool {
                return
                    touchStartX >= area.x &&
                    touchStartX < area.x + area.w &&
                    touchStartY >= area.y &&
                    touchStartY < area.y + area.h;
            };

            if (insideHouseHeader(deliveryArea)) {
                house.wareDeliveryEnabled =
                    !house.wareDeliveryEnabled;

                touchTapped = false;
            } else if (
                insideHouseHeader(repairArea)
            ) {
                house.repairEnabled =
                    !house.repairEnabled;

                touchTapped = false;
            }
        }

        // Touch ovladani spodnich School tlacitek.
        //
        // Renderer:
        // panelX = 6
        // panelY = 274
        // LEFT  = 6,   496, 54x40
        // TRAIN = 68,  496, 54x40
        // RIGHT = 130, 496, 54x40
        //
        // Touch pouziva stejne akce jako gamepad.
        if (
            gameUI.section == GameUISection::School &&
            touchTapped &&
            !gameUI.schoolQueueFocus
        ) {
            const int schoolButtonY = 274 + 222;

            SDL_Rect schoolLeftButton = {
                6,
                schoolButtonY,
                54,
                40
            };

            SDL_Rect schoolTrainButton = {
                6 + 62,
                schoolButtonY,
                54,
                40
            };

            SDL_Rect schoolRightButton = {
                6 + 124,
                schoolButtonY,
                54,
                40
            };

            auto touchInside = [
                touchStartX,
                touchStartY
            ](const SDL_Rect& area) -> bool {
                return
                    touchStartX >= area.x &&
                    touchStartX <
                        area.x + area.w &&
                    touchStartY >= area.y &&
                    touchStartY <
                        area.y + area.h;
            };

            if (touchInside(schoolLeftButton)) {
                moveSchoolUISelection(
                    &gameUI,
                    -1
                );

                touchTapped = false;
            } else if (
                touchInside(schoolRightButton)
            ) {
                moveSchoolUISelection(
                    &gameUI,
                    1
                );

                touchTapped = false;
            } else if (
                touchInside(schoolTrainButton)
            ) {
                // Pouzij existujici a overenou TRAIN cestu.
                crossPressed = true;

                touchTapped = false;
            }
        }


        // =================================================
        // OPTIONS MENU GAMEPAD
        //
        // Up/Down navigate the four Remake menu entries.
        // Circle returns focus to GUI 442 toolbar.
        // Cross is consumed until each action is wired.
        // =================================================

        if (
            gameUI.section ==
                GameUISection::Options
        ) {
            if (
                gameUI.optionsFocus &&
                upPressed
            ) {
                gameUI.optionsSelection--;

                if (
                    gameUI.optionsSelection < 0
                ) {
                    gameUI.optionsSelection = 3;
                }

                upPressed = false;
            }

            if (
                gameUI.optionsFocus &&
                downPressed
            ) {
                gameUI.optionsSelection++;

                if (
                    gameUI.optionsSelection > 3
                ) {
                    gameUI.optionsSelection = 0;
                }

                downPressed = false;
            }

            if (
                gameUI.optionsFocus &&
                circlePressed
            ) {
                gameUI.optionsFocus = false;
                gameUI.toolbarFocus = true;
                circlePressed = false;
            }

            if (
                gameUI.optionsFocus &&
                crossPressed
            ) {
                switch (gameUI.optionsSelection) {
                    case 0:
                        // LOAD GAME opens the real save list.
                        refreshSaveFileList(
                            &optionsSaveFiles
                        );

                        gameUI.section =
                            GameUISection::OptionsLoad;

                        gameUI.saveLoadFocus = 0;
                        gameUI.saveLoadTop = 0;
                        gameUI.saveLoadSelection =
                            optionsSaveFiles.selectedIndex;

                        crossPressed = false;
                        break;

                    case 1:
                    {
                        // SAVE GAME opens the Remake-style
                        // save list and editable name field.
                        refreshSaveFileList(
                            &optionsSaveFiles
                        );

                        gameUI.section =
                            GameUISection::OptionsSave;

                        gameUI.saveLoadFocus = 1;
                        gameUI.saveLoadTop = 0;
                        gameUI.saveLoadSelection =
                            optionsSaveFiles.selectedIndex;

                        const SaveFileEntry* selected =
                            getSelectedSaveFile(
                                &optionsSaveFiles
                            );

                        std::snprintf(
                            gameUI.saveName,
                            SAVE_NAME_LENGTH,
                            "%s",
                            selected
                                ? selected->name
                                : "New Save"
                        );

                        crossPressed = false;
                        break;
                    }

                    case 2:
                        // SETTINGS
                        settingsPopupOpen = true;
                        crossPressed = false;
                        break;

                    case 3:
                        // QUIT MISSION
                        running = false;
                        crossPressed = false;
                        break;

                    default:
                        crossPressed = false;
                        break;
                }
            }
        }


        // =================================================
        // REMAKE-STYLE SAVE / LOAD SUBPAGES
        // =================================================

        if (
            gameUI.section ==
                GameUISection::OptionsLoad ||
            gameUI.section ==
                GameUISection::OptionsSave
        ) {
            const bool savePage =
                gameUI.section ==
                    GameUISection::OptionsSave;

            constexpr int visibleRows = 8;

            // Circle first closes active Save-name editing.
            // Outside editing it returns to the Options menu.
            if (
                circlePressed &&
                saveNameEditing
            ) {
                SDL_StopTextInput();

                saveNameEditing = false;
                saveNameInputReceived = false;

                gameUI.saveLoadFocus = 1;

                circlePressed = false;
                crossPressed = false;
            } else if (circlePressed) {
                gameUI.section =
                    GameUISection::Options;

                gameUI.optionsFocus = true;
                gameUI.toolbarFocus = false;

                circlePressed = false;
                crossPressed = false;
            } else {
                // IME owns navigation while text editing
                // is active.
                if (saveNameEditing) {
                    upPressed = false;
                    downPressed = false;
                    leftPressed = false;
                    rightPressed = false;
                    crossPressed = false;
                }

                // Horizontal navigation changes focus area.
                if (
                    !saveNameEditing &&
                    (leftPressed || upPressed)
                ) {
                    if (savePage) {
                        gameUI.saveLoadFocus--;

                        if (gameUI.saveLoadFocus < 0)
                            gameUI.saveLoadFocus = 2;
                    } else {
                        gameUI.saveLoadFocus =
                            gameUI.saveLoadFocus == 0
                                ? 2
                                : 0;
                    }

                    leftPressed = false;
                }

                if (
                    !saveNameEditing &&
                    (rightPressed || downPressed)
                ) {
                    if (savePage) {
                        gameUI.saveLoadFocus++;

                        if (gameUI.saveLoadFocus > 2)
                            gameUI.saveLoadFocus = 0;
                    } else {
                        gameUI.saveLoadFocus =
                            gameUI.saveLoadFocus == 0
                                ? 2
                                : 0;
                    }

                    rightPressed = false;
                }

                // When list owns focus, Up/Down select real
                // SaveFileList rows instead of changing area.
                if (
                    gameUI.saveLoadFocus == 0 &&
                    optionsSaveFiles.count > 0
                ) {
                    if (upPressed) {
                        gameUI.saveLoadSelection--;

                        if (
                            gameUI.saveLoadSelection < 0
                        ) {
                            gameUI.saveLoadSelection =
                                optionsSaveFiles.count - 1;
                        }

                        upPressed = false;
                    }

                    if (downPressed) {
                        gameUI.saveLoadSelection++;

                        if (
                            gameUI.saveLoadSelection >=
                                optionsSaveFiles.count
                        ) {
                            gameUI.saveLoadSelection = 0;
                        }

                        downPressed = false;
                    }

                    selectSaveFile(
                        &optionsSaveFiles,
                        gameUI.saveLoadSelection
                    );

                    if (
                        gameUI.saveLoadSelection <
                            gameUI.saveLoadTop
                    ) {
                        gameUI.saveLoadTop =
                            gameUI.saveLoadSelection;
                    }

                    if (
                        gameUI.saveLoadSelection >=
                            gameUI.saveLoadTop +
                                visibleRows
                    ) {
                        gameUI.saveLoadTop =
                            gameUI.saveLoadSelection -
                                visibleRows + 1;
                    }

                    // Selecting a Save row copies its name
                    // into the Save page name field.
                    if (savePage) {
                        const SaveFileEntry* selected =
                            getSelectedSaveFile(
                                &optionsSaveFiles
                            );

                        if (selected) {
                            std::snprintf(
                                gameUI.saveName,
                                SAVE_NAME_LENGTH,
                                "%s",
                                selected->name
                            );
                        }
                    }
                }

                if (
                    crossPressed &&
                    !saveNameEditing
                ) {
                    // List row X selects and advances to the
                    // next relevant control.
                    if (gameUI.saveLoadFocus == 0) {
                        if (savePage) {
                            gameUI.saveLoadFocus = 1;
                        } else {
                            gameUI.saveLoadFocus = 2;
                        }
                    } else if (
                        savePage &&
                        gameUI.saveLoadFocus == 1
                    ) {
                        // Open the Vita/SDL text-input path.
                        //
                        // The first received text replaces the
                        // previous save name. Further text
                        // input is appended up to the normal
                        // SaveFileList name limit.
                        saveNameEditing = true;
                        saveNameInputReceived = false;

                        SDL_StartTextInput();
                    } else if (
                        gameUI.saveLoadFocus == 2
                    ) {
                        if (savePage) {
                            char namedSavePath[
                                SAVE_PATH_LENGTH
                            ] = {};

                            if (
                                buildNamedSavePath(
                                    gameUI.saveName,
                                    namedSavePath,
                                    SAVE_PATH_LENGTH
                                )
                            ) {
                                const bool saved =
                                    saveGame(
                                        namedSavePath,
                                        buildings,
                                        buildingCount,
                                        wareDistribution,
                                        units,
                                        unitCount,
                                        &roads[0][0],
                                        &landPlans[0][0],
                                        &cornGrowthTimers[0][0]
                                    );

                                if (saved) {
                                    refreshSaveFileList(
                                        &optionsSaveFiles
                                    );

                                    for (
                                        int index = 0;
                                        index <
                                            optionsSaveFiles.count;
                                        ++index
                                    ) {
                                        if (
                                            std::strcmp(
                                                optionsSaveFiles
                                                    .entries[index]
                                                    .path,
                                                namedSavePath
                                            ) == 0
                                        ) {
                                            selectSaveFile(
                                                &optionsSaveFiles,
                                                index
                                            );

                                            gameUI
                                                .saveLoadSelection =
                                                index;

                                            break;
                                        }
                                    }

                                    gameUI.section =
                                        GameUISection::Options;

                                    gameUI.optionsFocus = true;
                                }
                            }
                        } else {
                            const SaveFileEntry* selected =
                                getSelectedSaveFile(
                                    &optionsSaveFiles
                                );

                            if (selected) {
                                const bool loaded =
                                    loadGame(
                                        selected->path,
                                        buildings,
                                        &buildingCount,
                                        &wareDistribution,
                                        units,
                                        &unitCount,
                                        &roads[0][0],
                                        &landPlans[0][0],
                                        &cornGrowthTimers[0][0]
                                    );

                                if (loaded) {
                                    for (
                                        int index = 0;
                                        index < MAX_BUILDINGS;
                                        ++index
                                    ) {
                                        bakeryProductionTimers[
                                            index
                                        ] = 0.0f;

                                        schoolStepTimers[
                                            index
                                        ] = 0.0f;
                                    }

                                    selectedTileX = -1;
                                    selectedTileY = -1;

                                    closeUnitUI(&unitUI);

                                    gameUI
                                        .selectedSchoolBuilding =
                                        -1;

                                    gameUI
                                        .selectedStoreBuilding =
                                        -1;

                                    gameUI
                                        .selectedBakeryBuilding =
                                        -1;

                                    gameUI
                                        .selectedFarmBuilding =
                                        -1;

                                    gameUI.section =
                                        GameUISection::Options;

                                    gameUI.optionsFocus = true;
                                }
                            }
                        }
                    }

                    crossPressed = false;
                }

                // This page owns all navigation input.
                upPressed = false;
                downPressed = false;
                leftPressed = false;
                rightPressed = false;
            }
        }


        // =============================================
        // REMAKE-STYLE SETTINGS MODAL INPUT
        // =============================================

        if (settingsPopupOpen) {
            // Exact rendered Close geometry:
            //
            // popupX = 320
            // popupY = 32
            // Close:
            //   X = 320 + 16  = 336
            //   Y = 32  + 436 = 468
            //   W = 224
            //   H = 24
            const SDL_Rect settingsCloseArea = {
                336,
                468,
                224,
                24
            };

            bool closeSettingsPopup =
                circlePressed ||
                crossPressed;

            if (touchTapped) {
                const bool insideClose =
                    touchStartX >=
                        settingsCloseArea.x &&
                    touchStartX <
                        settingsCloseArea.x +
                            settingsCloseArea.w &&
                    touchStartY >=
                        settingsCloseArea.y &&
                    touchStartY <
                        settingsCloseArea.y +
                            settingsCloseArea.h;

                if (insideClose) {
                    closeSettingsPopup = true;
                    touchTapped = false;
                }
            }

            if (closeSettingsPopup) {
                settingsPopupOpen = false;

                // Return focus to Settings in the underlying
                // Options menu.
                gameUI.section =
                    GameUISection::Options;

                gameUI.optionsSelection = 2;
                gameUI.optionsFocus = true;
                gameUI.toolbarFocus = false;
            }

            // Modal consumes all underlying Options input.
            circlePressed = false;
            crossPressed = false;
            upPressed = false;
            downPressed = false;
            leftPressed = false;
            rightPressed = false;
        }


        // TRAIN ve School GUI.
        // Tento rez pouze radi profesi do fronty.
        // Samotny casovy vycvik a spawn po completion
        // budou implementovany v dalsim rezu.
        if (
            gameUI.section ==
                GameUISection::School &&
            crossPressed &&
            !gameUI.schoolQueueFocus
        ) {
            int schoolIndex =
                gameUI.selectedSchoolBuilding;

            bool validSchool =
                schoolIndex >= 0 &&
                schoolIndex < buildingCount &&
                buildings[schoolIndex].active &&
                buildings[schoolIndex].type ==
                    BuildingType::School &&
                isBuildingComplete(
                    buildings[schoolIndex]
                );

            if (validSchool) {
                Building& school =
                    buildings[schoolIndex];

                int freeQueueSlot = -1;

                for (
                    int queueIndex = 0;
                    queueIndex < SCHOOL_QUEUE_SIZE;
                    ++queueIndex
                ) {
                    if (
                        school.schoolQueue[queueIndex] < 0
                    ) {
                        freeQueueSlot = queueIndex;
                        break;
                    }
                }

                if (freeQueueSlot >= 0) {
                    int profession =
                        static_cast<int>(
                            gameUI.schoolSelection
                        );

                    if (
                        profession >= 0 &&
                        profession <
                            SCHOOL_PROFESSION_COUNT
                    ) {
                        school.schoolQueue[freeQueueSlot] =
                            profession;
                    }
                }
            }

            // X uz zpracoval TRAIN.
            // Nesmime ho dale pouzit pro BUILD/mapu.
            crossPressed = false;
        }

        // Pri focusu School queue nema X zadnou TRAIN
        // ani mapovou funkci.
        if (
            gameUI.section == GameUISection::School &&
            gameUI.schoolQueueFocus &&
            crossPressed
        ) {
            crossPressed = false;
        }

        // Dotykove CANCEL tlacitko je WIP slot queue[0].
        // Souradnice musi odpovidat renderu v game_ui.cpp.
        bool schoolCancelTapped = false;

        if (
            gameUI.section == GameUISection::School &&
            touchTapped
        ) {
            const int schoolPanelX = 6;

            SDL_Rect schoolCancelArea = {
                schoolPanelX,
                274 + 48,
                32,
                32
            };

            schoolCancelTapped =
                touchStartX >= schoolCancelArea.x &&
                touchStartX <
                    schoolCancelArea.x +
                    schoolCancelArea.w &&
                touchStartY >= schoolCancelArea.y &&
                touchStartY <
                    schoolCancelArea.y +
                    schoolCancelArea.h;
        }

        // Dotykove zruseni konkretni cekajici
        // polozky queue[1..5].
        int schoolWaitingCancelIndex = -1;

        // Gamepad CANCEL pro vybrany cekajici slot.
        // Slot 0 je WIP a obslouzi jej WIP cancel blok.
        if (
            gameUI.section == GameUISection::School &&
            gameUI.schoolQueueFocus &&
            squarePressed &&
            gameUI.schoolQueueSelection >= 1 &&
            gameUI.schoolQueueSelection <
                SCHOOL_QUEUE_SIZE
        ) {
            schoolWaitingCancelIndex =
                gameUI.schoolQueueSelection;
        }

        if (
            gameUI.section == GameUISection::School &&
            touchTapped &&
            !schoolCancelTapped
        ) {
            const int schoolPanelX = 6;

            const int schoolQueueY = 274 + 80;

            for (
                int waitingIndex = 0;
                waitingIndex < SCHOOL_WAITING_SLOTS;
                ++waitingIndex
            ) {
                SDL_Rect waitingSlotArea = {
                    schoolPanelX + waitingIndex * 36,
                    schoolQueueY,
                    32,
                    32
                };

                bool insideWaitingSlot =
                    touchStartX >= waitingSlotArea.x &&
                    touchStartX <
                        waitingSlotArea.x +
                        waitingSlotArea.w &&
                    touchStartY >= waitingSlotArea.y &&
                    touchStartY <
                        waitingSlotArea.y +
                        waitingSlotArea.h;

                if (insideWaitingSlot) {
                    // waitingIndex 0 odpovida queue[1].
                    schoolWaitingCancelIndex =
                        waitingIndex + 1;
                    break;
                }
            }
        }

        if (schoolWaitingCancelIndex >= 1) {
            int schoolIndex =
                gameUI.selectedSchoolBuilding;

            bool validSchool =
                schoolIndex >= 0 &&
                schoolIndex < buildingCount &&
                buildings[schoolIndex].active &&
                buildings[schoolIndex].type ==
                    BuildingType::School &&
                isBuildingComplete(
                    buildings[schoolIndex]
                );

            if (validSchool) {
                Building& school =
                    buildings[schoolIndex];

                if (
                    schoolWaitingCancelIndex <
                        SCHOOL_QUEUE_SIZE &&
                    school.schoolQueue[
                        schoolWaitingCancelIndex
                    ] >= 0
                ) {
                    // Odstran pouze zvolenou cekajici
                    // polozku a zbytek fronty posun.
                    for (
                        int queueIndex =
                            schoolWaitingCancelIndex;
                        queueIndex <
                            SCHOOL_QUEUE_SIZE - 1;
                        ++queueIndex
                    ) {
                        school.schoolQueue[queueIndex] =
                            school.schoolQueue[
                                queueIndex + 1
                            ];
                    }

                    school.schoolQueue[
                        SCHOOL_QUEUE_SIZE - 1
                    ] = -1;

                    // queue[0] a bezici progress se
                    // zamerne nijak nerestuji.
                }
            }

            // Tap do queue patri pouze School GUI,
            // i kdyz byl zvoleny slot prazdny.
            touchTapped = false;

            // Square uz zpracovalo zruseni vybraneho
            // cekajiciho slotu.
            if (
                gameUI.schoolQueueFocus &&
                gameUI.schoolQueueSelection >= 1
            ) {
                squarePressed = false;
            }
        }

        // CANCEL ve School GUI.
        // Square zrusi prave trenovanou jednotku v queue[0].
        // Gold se nevraci, protoze se spotrebovava az pri completion.
        if (
            gameUI.section == GameUISection::School &&
            (
                (
                    squarePressed &&
                    (
                        !gameUI.schoolQueueFocus ||
                        gameUI.schoolQueueSelection == 0
                    )
                ) ||
                schoolCancelTapped
            )
        ) {
            int schoolIndex =
                gameUI.selectedSchoolBuilding;

            bool validSchool =
                schoolIndex >= 0 &&
                schoolIndex < buildingCount &&
                buildings[schoolIndex].active &&
                buildings[schoolIndex].type ==
                    BuildingType::School &&
                isBuildingComplete(
                    buildings[schoolIndex]
                );

            if (validSchool) {
                Building& school =
                    buildings[schoolIndex];

                if (school.schoolQueue[0] >= 0) {
                    // Odstran WIP a posun vsechny cekajici
                    // polozky o jeden slot dopredu.
                    for (
                        int queueIndex = 0;
                        queueIndex < SCHOOL_QUEUE_SIZE - 1;
                        ++queueIndex
                    ) {
                        school.schoolQueue[queueIndex] =
                            school.schoolQueue[
                                queueIndex + 1
                            ];
                    }

                    school.schoolQueue[
                        SCHOOL_QUEUE_SIZE - 1
                    ] = -1;

                    // Reset celeho rozpracovaneho vycviku.
                    school.schoolTrainingStep = 0;
                    school.schoolIdleStep = 0;
                    school.schoolTrainingActive = false;
                    school.schoolFinishingTraining = false;
                }
            }

            // Vstup uz zpracovalo School GUI.
            squarePressed = false;

            if (schoolCancelTapped)
                touchTapped = false;
        }

        // -------------------------------------------------
        // Unit GUI Back.
        //
        // Remake pouziva GUI 443. Circle i touch provedou
        // stejnou akci: deselect a navrat k hlavni strance.
        // -------------------------------------------------
        if (
            unitUI.panelVisible &&
            circlePressed
        ) {
            const GameUISection returnSection =
                unitUI.previousSection;

            closeUnitUI(&unitUI);

            gameUI.section =
                returnSection;

            gameUI.buildSectionActive =
                returnSection ==
                    GameUISection::Build;

            circlePressed = false;
        }

        if (
            unitUI.panelVisible &&
            touchTapped
        ) {
            const GameUISection returnSection =
                unitUI.previousSection;

            if (
                handleUnitUIBackTouch(
                    &unitUI,
                    touchStartX,
                    touchStartY
                )
            ) {
                gameUI.section =
                    returnSection;

                gameUI.buildSectionActive =
                    returnSection ==
                        GameUISection::Build;

                touchTapped = false;
                crossPressed = false;
            }
        }


        // -------------------------------------------------
        // Unit selection.
        //
        // X nebo touch tap nad jednotkou ma prednost pred
        // mapovym prikazem. Vyber a GUI nejsou DEV funkce.
        // -------------------------------------------------
        bool unitSelectionConsumed = false;

        if (
            !buildMenuOpen &&
            (
                crossPressed ||
                touchTapped
            )
        ) {
            const float selectScreenX =
                touchTapped
                    ? touchStartX
                    : cursorX;

            const float selectScreenY =
                touchTapped
                    ? touchStartY
                    : cursorY;

            const int hitUnit =
                hitTestUnit(
                    units,
                    unitCount,
                    selectScreenX,
                    selectScreenY,
                    cameraX,
                    cameraY
                );

            if (hitUnit >= 0) {
                // Backward-compatible runtime initialization:
                // older or zero-initialized development units
                // receive the Remake base condition.
                if (units[hitUnit].condition <= 0) {
                    units[hitUnit].condition =
                        UNIT_START_CONDITION;
                }

                // Unit detail je skutecna sidebar stranka,
                // nikoliv overlay nad Build/Stats/School.
                if (
                    gameUI.section !=
                        GameUISection::Unit
                ) {
                    unitUI.previousSection =
                        gameUI.section;
                }

                unitUI.selectedUnitIndex =
                    hitUnit;

                unitUI.panelVisible = true;

                gameUI.section =
                    GameUISection::Unit;

                gameUI.buildSectionActive = false;
                gameUI.toolbarFocus = false;

                unitSelectionConsumed = true;

                crossPressed = false;

                if (touchTapped)
                    touchTapped = false;
            }
        }

        // Neviditelna nebo zrusena jednotka nemuze
        // zustat vybrana.
        if (
            unitUI.selectedUnitIndex >= 0 &&
            (
                unitUI.selectedUnitIndex >= unitCount ||
                !units[
                    unitUI.selectedUnitIndex
                ].active ||
                !units[
                    unitUI.selectedUnitIndex
                ].visible
            )
        ) {
            const GameUISection returnSection =
                unitUI.previousSection;

            unitUI.selectedUnitIndex = -1;
            unitUI.panelVisible = false;

            if (
                gameUI.section ==
                    GameUISection::Unit
            ) {
                gameUI.section =
                    returnSection;

                gameUI.buildSectionActive =
                    returnSection ==
                        GameUISection::Build;
            }
        }

        previousButtons = pad.buttons;

        if (r1Pressed && !buildMenuOpen) {
            int cursorTileX =
                ((int)cursorX + (int)cameraX) / TILE_SIZE;

            int cursorTileY =
                ((int)cursorY + (int)cameraY) / TILE_SIZE;

            for (int i = 0; i < buildingCount; ++i) {
                if (!buildings[i].active)
                    continue;

                int width =
                    getBuildingWidth(buildings[i].type);

                int height =
                    getBuildingHeight(buildings[i].type);

                bool insideBuilding =
                    cursorTileX >= buildings[i].tileX &&
                    cursorTileX < buildings[i].tileX + width &&
                    cursorTileY >= buildings[i].tileY &&
                    cursorTileY < buildings[i].tileY + height;

                if (!insideBuilding)
                    continue;

                int maxWoodStep =
                    getBuildingWoodSteps(buildings[i].type);

                int maxStoneStep =
                    getBuildingStoneSteps(buildings[i].type);

                if (buildings[i].woodStep < maxWoodStep) {
                    buildings[i].woodStep++;
                } else if (buildings[i].stoneStep < maxStoneStep) {
                    buildings[i].stoneStep++;
                }

                break;
            }
        }

        // DEV UNIT MOVE BEGIN
        if (
            ENABLE_DEV_UNIT_MOVE &&
            unitUI.selectedUnitIndex >= 0 &&
            unitUI.selectedUnitIndex < unitCount &&
            !buildMenuOpen &&
            squarePressed &&
            cursorX >= GAME_UI_PANEL_WIDTH
        ) {
            Unit& selectedUnit =
                units[
                    unitUI.selectedUnitIndex
                ];

            const float moveTargetX =
                (
                    (
                        (int)cursorX +
                        (int)cameraX
                    ) /
                    TILE_SIZE +
                    0.5f
                ) *
                TILE_SIZE;

            const float moveTargetY =
                (
                    (
                        (int)cursorY +
                        (int)cameraY
                    ) /
                    TILE_SIZE +
                    0.5f
                ) *
                TILE_SIZE;

            selectedUnit.targetBuilding = -1;
            selectedUnit.targetRoadX = -1;
            selectedUnit.targetRoadY = -1;
            selectedUnit.workAction =
                UnitWorkAction::None;

            selectedUnit.doorwayOffsetX = 0;
            selectedUnit.doorwayOffsetY = 0;

            setUnitMoveTarget(
                &selectedUnit,
                moveTargetX,
                moveTargetY,
                buildings,
                buildingCount,
                MAP_WIDTH,
                MAP_HEIGHT,
                TILE_SIZE
            );

            squarePressed = false;
        }
        // DEV UNIT MOVE END

        if (l1Pressed && !buildMenuOpen) {
            bool workerSpawned = false;

            int cursorTileX =
                ((int)cursorX + (int)cameraX) / TILE_SIZE;

            int cursorTileY =
                ((int)cursorY + (int)cameraY) / TILE_SIZE;

            for (int i = 0; i < buildingCount; ++i) {
                if (!buildings[i].active)
                    continue;

                if (buildings[i].type != BuildingType::School)
                    continue;

                int width =
                    getBuildingWidth(buildings[i].type);

                int height =
                    getBuildingHeight(buildings[i].type);

                bool cursorInsideSchool =
                    cursorTileX >= buildings[i].tileX &&
                    cursorTileX <
                        buildings[i].tileX + width &&
                    cursorTileY >= buildings[i].tileY &&
                    cursorTileY <
                        buildings[i].tileY + height;

                if (!cursorInsideSchool)
                    continue;

                if (!isBuildingComplete(buildings[i]))
                    break;

                int freeUnitSlot = -1;

                for (int unitIndex = 0;
                     unitIndex < unitCount;
                     ++unitIndex) {

                    if (!units[unitIndex].active) {
                        freeUnitSlot = unitIndex;
                        break;
                    }
                }

                if (freeUnitSlot == -1 &&
                    unitCount < MAX_UNITS) {

                    freeUnitSlot = unitCount;
                    unitCount++;
                }

                if (freeUnitSlot >= 0) {
                    Unit& worker = units[freeUnitSlot];

                    worker.type = UnitType::Worker;
                    worker.state = UnitState::Idle;
                    worker.condition = UNIT_START_CONDITION;

                    // School ma vstup ve spodni rade
                    // na prostredni dlazdici. Worker se
                    // objevi tesne pred budovou.
                    worker.worldX =
                        (buildings[i].tileX + 1.5f) *
                        TILE_SIZE;

                    worker.worldY =
                        (buildings[i].tileY + height) *
                        TILE_SIZE;

                    // Smer S podle poradi:
                    // N, NE, E, SE, S, SW, W, NW.
                    worker.direction = 4;
                    worker.animationFrame = 0;
                    worker.animationTimer = 0.0f;

                    worker.targetWorldX = worker.worldX;
                    worker.targetWorldY = worker.worldY;

                    worker.pathLength = 0;
                    worker.pathIndex = 0;

                    worker.targetBuilding = -1;

                    worker.targetRoadX = -1;
                    worker.targetRoadY = -1;
                    worker.workTimer = 0.0f;
                    worker.workAction =
                        UnitWorkAction::None;

                    worker.doorwayProgress = 0.0f;

                    worker.doorwayStartX =
                        worker.worldX;

                    worker.doorwayStartY =
                        worker.worldY;

                    worker.doorwayTargetX =
                        worker.worldX;

                    worker.doorwayTargetY =
                        worker.worldY;

                    worker.doorwayOffsetX = 0;
                    worker.doorwayOffsetY = 0;

                    worker.visible = true;
                    worker.active = true;
                    workerSpawned = true;
                }

                break;
            }

            if (!workerSpawned) {
                for (int unitIndex = unitCount - 1;
                     unitIndex >= 0;
                     --unitIndex) {

                    if (!units[unitIndex].active)
                        continue;

                    const bool validCursorTile =
                        cursorTileX >= 0 &&
                        cursorTileX < MAP_WIDTH &&
                        cursorTileY >= 0 &&
                        cursorTileY < MAP_HEIGHT;

                    UnitWorkAction requestedWorkAction =
                        UnitWorkAction::None;

                    int requestedBuilding = -1;

                    int workTargetTileX =
                        cursorTileX;

                    int workTargetTileY =
                        cursorTileY;

                    if (validCursorTile) {
                        // Nejdřív zkus House plan.
                        //
                        // Prikaz lze zadat na libovolne aktivni
                        // dlazdici planu. Worker pak jde pred
                        // entrance, nikoliv dovnitr blokovaneho
                        // footprintu.
                        for (
                            int buildingIndex = 0;
                            buildingIndex < buildingCount;
                            ++buildingIndex
                        ) {
                            Building& candidate =
                                buildings[
                                    buildingIndex
                                ];

                            if (!candidate.active)
                                continue;

                            if (
                                candidate.constructionProgress != 0 ||
                                candidate.woodStep != 0 ||
                                candidate.stoneStep != 0
                            ) {
                                continue;
                            }

                            const int localX =
                                cursorTileX -
                                candidate.tileX;

                            const int localY =
                                cursorTileY -
                                candidate.tileY;

                            const int width =
                                getBuildingWidth(
                                    candidate.type
                                );

                            const int height =
                                getBuildingHeight(
                                    candidate.type
                                );

                            if (
                                localX < 0 ||
                                localY < 0 ||
                                localX >= width ||
                                localY >= height
                            ) {
                                continue;
                            }

                            if (
                                getBuildingPlanValue(
                                    candidate.type,
                                    localX,
                                    localY
                                ) == 0
                            ) {
                                continue;
                            }

                            const BuildingEntrance entrance =
                                getBuildingEntrance(
                                    candidate.type
                                );

                            requestedWorkAction =
                                UnitWorkAction::BuildHouse;

                            requestedBuilding =
                                buildingIndex;

                            workTargetTileX =
                                candidate.tileX +
                                entrance.tileOffsetX;

                            // Jedna volna dlazdice pred
                            // jiznim entrance.
                            workTargetTileY =
                                candidate.tileY +
                                entrance.tileOffsetY +
                                1;

                            break;
                        }

                        if (
                            requestedWorkAction ==
                                UnitWorkAction::None &&
                            roads[
                                cursorTileY
                            ][
                                cursorTileX
                            ] == RoadState::Plan
                        ) {
                            requestedWorkAction =
                                UnitWorkAction::DigRoad;
                        } else if (
                            requestedWorkAction ==
                                UnitWorkAction::None &&
                            landPlans[
                                cursorTileY
                            ][
                                cursorTileX
                            ] == LandPlanState::Field
                        ) {
                            requestedWorkAction =
                                UnitWorkAction::BuildField;
                        } else if (
                            landPlans[
                                cursorTileY
                            ][
                                cursorTileX
                            ] == LandPlanState::Wine
                        ) {
                            requestedWorkAction =
                                UnitWorkAction::BuildWine;
                        }
                    }

                    const bool hasWorkTarget =
                        requestedWorkAction !=
                            UnitWorkAction::None;

                    // Field, Wine a Road construction muze
                    // vykonavat pouze skutecny Worker.
                    if (
                        hasWorkTarget &&
                        units[unitIndex].type !=
                            UnitType::Worker
                    ) {
                        continue;
                    }

                    float targetX =
                        (
                            workTargetTileX +
                            0.5f
                        ) *
                        TILE_SIZE;

                    float targetY =
                        (
                            workTargetTileY +
                            0.5f
                        ) *
                        TILE_SIZE;

                    bool pathFound =
                        setUnitMoveTarget(
                            &units[unitIndex],
                            targetX,
                            targetY,
                            buildings,
                            buildingCount,
                            MAP_WIDTH,
                            MAP_HEIGHT,
                            TILE_SIZE
                        );

                    Unit& worker =
                        units[unitIndex];

                    if (
                        pathFound &&
                        hasWorkTarget
                    ) {
                        worker.workAction =
                            requestedWorkAction;

                        worker.workTimer = 0.0f;

                        if (
                            requestedWorkAction ==
                                UnitWorkAction::BuildHouse
                        ) {
                            worker.targetBuilding =
                                requestedBuilding;

                            worker.targetRoadX = -1;
                            worker.targetRoadY = -1;
                        } else {
                            worker.targetBuilding = -1;

                            worker.targetRoadX =
                                cursorTileX;

                            worker.targetRoadY =
                                cursorTileY;
                        }
                        worker.animationFrame = 0;
                        worker.animationTimer = 0.0f;

                        const int workerTileX =
                            (int)(
                                worker.worldX /
                                TILE_SIZE
                            );

                        const int workerTileY =
                            (int)(
                                worker.worldY /
                                TILE_SIZE
                            );

                        // Pokud Worker uz stoji na cilove
                        // dlazdici, neni potreba walking faze.
                        if (
                            workerTileX ==
                                workTargetTileX &&
                            workerTileY ==
                                workTargetTileY
                        ) {
                            worker.state =
                                UnitState::Working;

                            if (
                                requestedWorkAction ==
                                    UnitWorkAction::BuildHouse
                            ) {
                                worker.doorwayOffsetX = 0;
                                worker.doorwayOffsetY =
                                    -TILE_SIZE;

                                worker.doorwayProgress =
                                    0.0f;
                            } else {
                                worker.doorwayOffsetX = 0;
                                worker.doorwayOffsetY = 0;
                            }
                        }
                    } else {
                        // Bez pracovniho planu zustava X
                        // beznym prikazem k pohybu.
                        worker.targetBuilding = -1;
                        worker.targetRoadX = -1;
                        worker.targetRoadY = -1;

                        worker.doorwayOffsetX = 0;
                        worker.doorwayOffsetY = 0;

                        worker.workAction =
                            UnitWorkAction::None;

                        if (!pathFound) {
                            worker.state =
                                UnitState::Idle;
                        }
                    }

                    break;
                }
            }
        }

        if (trianglePressed) {
            buildMenuOpen = !buildMenuOpen;

            if (buildMenuOpen) {
                buildMenuSelection = 0;
                activeBuildTool = BuildTool::Road;
                buildMenuSelection = 0;

                gameUI.buildSelection =
                    GameUIBuildSelection::Road;

                selectedTileX = -1;
                selectedTileY = -1;
            }
        }

        if (buildMenuOpen) {
            if (upPressed) {
                buildMenuSelection--;
                if (buildMenuSelection < 0)
                    buildMenuSelection = 4;
            }

            if (downPressed) {
                buildMenuSelection++;
                if (buildMenuSelection > 4)
                    buildMenuSelection = 0;
            }
        }

        // START intentionally has no Save Game action.
        // Manual saving is available only from Options.

        int analogX = (int)pad.lx - 128;
        int analogY = (int)pad.ly - 128;

        if (analogX > 18 || analogX < -18)
            cameraX += analogX * 0.03f;

        if (analogY > 18 || analogY < -18)
            cameraY += analogY * 0.03f;

        if (cameraX < 0.0f) cameraX = 0.0f;
        if (cameraY < 0.0f) cameraY = 0.0f;

        const float maxCameraX = WORLD_WIDTH - SCREEN_WIDTH;
        const float maxCameraY = WORLD_HEIGHT - SCREEN_HEIGHT;

        if (cameraX > maxCameraX) cameraX = maxCameraX;
        if (cameraY > maxCameraY) cameraY = maxCameraY;


        int cursorAnalogX = (int)pad.rx - 128;
        int cursorAnalogY = (int)pad.ry - 128;

        if (cursorAnalogX > 18 || cursorAnalogX < -18)
            cursorX += cursorAnalogX * 0.03f;

        if (cursorAnalogY > 18 || cursorAnalogY < -18)
            cursorY += cursorAnalogY * 0.03f;

        if (touchData.reportNum > 0) {
            const SceTouchReport& touch = touchData.report[0];

            int touchWidth =
                frontTouchInfo.maxDispX - frontTouchInfo.minDispX;
            int touchHeight =
                frontTouchInfo.maxDispY - frontTouchInfo.minDispY;

            if (touchWidth > 0 && touchHeight > 0) {
                cursorX =
                    (float)(touch.x - frontTouchInfo.minDispX) *
                    SCREEN_WIDTH / touchWidth;

                cursorY =
                    (float)(touch.y - frontTouchInfo.minDispY) *
                    SCREEN_HEIGHT / touchHeight;
            }
        }

        if (cursorX < 0.0f) cursorX = 0.0f;
        if (cursorY < 0.0f) cursorY = 0.0f;
        if (cursorX > SCREEN_WIDTH - 1) cursorX = SCREEN_WIDTH - 1;
        if (cursorY > SCREEN_HEIGHT - 1) cursorY = SCREEN_HEIGHT - 1;

        // -------------------------------------------------
        // Kompletni KaM BUILD 439 touch geometrie
        // -------------------------------------------------
        if (gameUI.section == GameUISection::Build) {
            const int buildButtonSize = 33;
            const int buildButtonStep = 37;
            const int buildContentX = 6;
            const int buildActionY = 282;
            const int buildHousesY = 321;

            int hoveredBuildItem = -1;

            // Index 0..26 v GUI_HOUSE_ORDER.
            // Je oddeleny od hoveredBuildItem, protoze
            // vsechny domy maji detail, ale pouze ctyri
            // maji v KaMVita funkcni placement.
            int hoveredHouseCatalogIndex = -1;

            // Originalni Build tool row:
            // 0 Road, 1 Field, 2 Wine, 3 Remove.
            const int buildToolX[4] = {
                buildContentX,
                buildContentX + 37,
                buildContentX + 74,
                buildContentX + 148
            };

            int hoveredBuildTool = -1;

            for (
                int toolIndex = 0;
                toolIndex < 4;
                ++toolIndex
            ) {
                SDL_Rect toolArea = {
                    buildToolX[toolIndex],
                    buildActionY,
                    buildButtonSize,
                    buildButtonSize
                };

                if (
                    cursorX >= toolArea.x &&
                    cursorX <
                        toolArea.x + toolArea.w &&
                    cursorY >= toolArea.y &&
                    cursorY <
                        toolArea.y + toolArea.h
                ) {
                    hoveredBuildTool = toolIndex;
                    break;
                }
            }

            if (
                hoveredBuildTool >= 0 &&
                touchTapped
            ) {
                gameUI.buildToolSelection =
                    hoveredBuildTool;

                // Road/Field/Wine/Remove maji vlastni
                // detail a opousteji House katalog.
                gameUI.buildHouseCatalogSelection = -1;

                switch (hoveredBuildTool) {
                    case 0:
                        activeBuildTool =
                            BuildTool::Road;

                        gameUI.buildSelection =
                            GameUIBuildSelection::Road;
                        break;

                    case 1:
                        activeBuildTool =
                            BuildTool::Field;
                        break;

                    case 2:
                        activeBuildTool =
                            BuildTool::Wine;
                        break;

                    case 3:
                        activeBuildTool =
                            BuildTool::Remove;
                        break;

                    default:
                        break;
                }

                selectedTileX = -1;
                selectedTileY = -1;
            }

            // Pouze implementovane houses jsou klikatelne.
            if (
                hoveredBuildItem < 0 &&
                hoveredBuildTool < 0
            ) {
                for (
                    int houseIndex = 0;
                    houseIndex < GUI_HOUSE_COUNT;
                    ++houseIndex
                ) {
                    const int column =
                        houseIndex % 5;

                    const int row =
                        houseIndex / 5;

                    SDL_Rect houseArea = {
                        buildContentX +
                            column * buildButtonStep,
                        buildHousesY +
                            row * buildButtonStep,
                        buildButtonSize,
                        buildButtonSize
                    };

                    if (
                        cursorX < houseArea.x ||
                        cursorX >=
                            houseArea.x +
                                houseArea.w ||
                        cursorY < houseArea.y ||
                        cursorY >=
                            houseArea.y +
                                houseArea.h
                    ) {
                        continue;
                    }

                    // Kazda pozice katalogu je vybiratelna
                    // pro zobrazeni ikony, nazvu a ceny.
                    hoveredHouseCatalogIndex =
                        houseIndex;

                    // Pouze implementovane typy dostanou
                    // GameUIBuildSelection a placement.
                    switch (
                        GUI_HOUSE_ORDER[houseIndex]
                    ) {
                        case HouseIconType::Store:
                            hoveredBuildItem =
                                static_cast<int>(
                                    GameUIBuildSelection::
                                        Store
                                );
                            break;

                        case HouseIconType::Bakery:
                            hoveredBuildItem =
                                static_cast<int>(
                                    GameUIBuildSelection::
                                        Bakery
                                );
                            break;

                        case HouseIconType::Farm:
                            hoveredBuildItem =
                                static_cast<int>(
                                    GameUIBuildSelection::
                                        Farm
                                );
                            break;

                        case HouseIconType::Quarry:
                            hoveredBuildItem =
                                static_cast<int>(
                                    GameUIBuildSelection::
                                        Quarry
                                );
                            break;

                        case HouseIconType::Mill:
                            hoveredBuildItem =
                                static_cast<int>(
                                    GameUIBuildSelection::
                                        Mill
                                );
                            break;

                        case HouseIconType::School:
                            hoveredBuildItem =
                                static_cast<int>(
                                    GameUIBuildSelection::
                                        School
                                );
                            break;

                        default:
                            // Neimplementovane budovy odpovidaji
                            // neaktivnimu GUI 41 v Remaku.
                            hoveredBuildItem = -1;
                            break;
                    }

                    break;
                }
            }

            // Touch nad libovolnou z 27 House ikon
            // vzdy aktualizuje katalogovy detail.
            if (
                hoveredHouseCatalogIndex >= 0 &&
                (
                    touchDown ||
                    touchTapped
                )
            ) {
                gameUI.buildHouseCatalogSelection =
                    hoveredHouseCatalogIndex;

                // Zadny tool row button nema byt selected,
                // kdyz zobrazujeme House detail.
                gameUI.buildToolSelection = -1;

                // Bezpecnost: pouhe prohlizeni zamceneho
                // domu nesmi ponechat aktivni stary
                // Building placement.
                if (hoveredBuildItem < 0) {
                    activeBuildTool =
                        BuildTool::Road;

                    selectedTileX = -1;
                    selectedTileY = -1;
                }
            }

            // Touch muze menit aktivni GUI polozku.
            // Samotna poloha herniho kurzoru nesmi
            // prepisovat vyber provedeny D-padem.
            if (hoveredBuildItem >= 0 &&
                (touchDown || touchTapped)) {

                gameUI.buildSelection =
                    static_cast<GameUIBuildSelection>(
                        hoveredBuildItem
                    );
            }

            // Touch tap nebo X nad ikonou potvrdi vyber.
            bool activateBuildItem =
                gameUI.section ==
                    GameUISection::Build &&
                (
                    (
                        crossPressed &&
                        cursorX <
                            GAME_UI_PANEL_WIDTH
                    ) ||
                    (
                        touchTapped &&
                        hoveredBuildItem >= 0
                    )
                );

            if (activateBuildItem) {
                if (unitUI.panelVisible) {
                    closeUnitUI(&unitUI);
                }

                int selection =
                    touchTapped &&
                    hoveredBuildItem >= 0
                        ? hoveredBuildItem
                        : static_cast<int>(
                            gameUI.buildSelection
                        );

                gameUI.buildSelection =
                    static_cast<GameUIBuildSelection>(
                        selection
                    );

                // Road pouziva tool detail.
                // House zachova katalogovy detail.
                gameUI.buildToolSelection =
                    selection == 0
                        ? 0
                        : -1;

                if (selection == 0) {
                    gameUI.buildHouseCatalogSelection = -1;
                }

                buildMenuSelection = selection;

                if (selection == 0) {
                    activeBuildTool =
                        BuildTool::Road;

                    selectedTileX = -1;
                    selectedTileY = -1;
                } else {
                    if (selection == 1)
                        selectedBuildingType =
                            BuildingType::Store;
                    else if (selection == 2)
                        selectedBuildingType =
                            BuildingType::Bakery;
                    else if (selection == 3)
                        selectedBuildingType =
                            BuildingType::Farm;
                    else if (selection == 4)
                        selectedBuildingType =
                            BuildingType::School;
                    else if (selection == 5)
                        selectedBuildingType =
                            BuildingType::Quarry;
                    else if (selection == 6)
                        selectedBuildingType =
                            BuildingType::Mill;

                    // Store, Bakery, Farm, School, Quarry and Mill
                    // now use the real building placement tool.
                    activeBuildTool =
                        BuildTool::Building;
                }

                // Stare popup menu uz pri pouziti
                // pevneho panelu nepotrebujeme.
                buildMenuOpen = false;
            }
        }

        if (buildMenuOpen) {
            const int menuSlotX = 10;
            const int menuSlotSize = 64;
            const int menuSlotGap = 12;
            const int menuFirstY = 40;

            if (cursorX >= menuSlotX &&
                cursorX < menuSlotX + menuSlotSize) {

                for (int i = 0; i < 5; ++i) {
                    int slotY =
                        menuFirstY +
                        i * (menuSlotSize + menuSlotGap);

                    if (cursorY >= slotY &&
                        cursorY < slotY + menuSlotSize) {

                        buildMenuSelection = i;

                        if (touchTapped) {
                            if (i == 0) {
                                activeBuildTool =
                                    BuildTool::Road;
                            } else {
                                if (i == 1)
                                    selectedBuildingType =
                                        BuildingType::Store;
                                else if (i == 2)
                                    selectedBuildingType =
                                        BuildingType::Bakery;
                                else if (i == 3)
                                    selectedBuildingType =
                                        BuildingType::Farm;
                                else
                                    selectedBuildingType =
                                        BuildingType::School;

                                activeBuildTool =
                                    BuildTool::Building;
                            }

                            buildMenuOpen = false;
                        }

                        break;
                    }
                }
            }

            if (crossPressed && buildMenuOpen) {
                if (buildMenuSelection == 0) {
                    activeBuildTool =
                        BuildTool::Road;
                } else {
                    if (buildMenuSelection == 1)
                        selectedBuildingType =
                            BuildingType::Store;
                    else if (buildMenuSelection == 2)
                        selectedBuildingType =
                            BuildingType::Bakery;
                    else if (buildMenuSelection == 3)
                        selectedBuildingType =
                            BuildingType::Farm;
                    else
                        selectedBuildingType =
                            BuildingType::School;

                    activeBuildTool =
                        BuildTool::Building;
                }

                buildMenuOpen = false;
            }
        }

        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            // =========================================
            // SAVE-NAME TEXT INPUT
            // =========================================

            if (saveNameEditing) {
                if (
                    event.type ==
                        SDL_TEXTINPUT
                ) {
                    const char* incoming =
                        event.text.text;

                    if (
                        incoming &&
                        incoming[0] != '\0'
                    ) {
                        if (!saveNameInputReceived) {
                            gameUI.saveName[0] = '\0';
                            saveNameInputReceived = true;
                        }

                        const size_t currentLength =
                            std::strlen(
                                gameUI.saveName
                            );

                        const size_t maximumLength =
                            SAVE_NAME_LENGTH - 1;

                        if (
                            currentLength <
                                maximumLength
                        ) {
                            const size_t freeLength =
                                maximumLength -
                                    currentLength;

                            std::strncat(
                                gameUI.saveName,
                                incoming,
                                freeLength
                            );

                            gameUI.saveName[
                                maximumLength
                            ] = '\0';
                        }
                    }

                    continue;
                }

                if (
                    event.type ==
                        SDL_KEYDOWN
                ) {
                    if (
                        event.key.keysym.sym ==
                            SDLK_BACKSPACE
                    ) {
                        const size_t length =
                            std::strlen(
                                gameUI.saveName
                            );

                        if (length > 0) {
                            gameUI.saveName[
                                length - 1
                            ] = '\0';
                        }

                        saveNameInputReceived = true;
                        continue;
                    }

                    if (
                        event.key.keysym.sym ==
                            SDLK_RETURN ||
                        event.key.keysym.sym ==
                            SDLK_KP_ENTER
                    ) {
                        SDL_StopTextInput();

                        saveNameEditing = false;
                        saveNameInputReceived = false;

                        if (
                            gameUI.saveName[0] ==
                                '\0'
                        ) {
                            std::snprintf(
                                gameUI.saveName,
                                SAVE_NAME_LENGTH,
                                "%s",
                                "New Save"
                            );
                        }

                        gameUI.saveLoadFocus = 2;
                        continue;
                    }

                    if (
                        event.key.keysym.sym ==
                            SDLK_ESCAPE
                    ) {
                        SDL_StopTextInput();

                        saveNameEditing = false;
                        saveNameInputReceived = false;

                        gameUI.saveLoadFocus = 1;
                        continue;
                    }
                }
            }

            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // KaM-like dark background
        SDL_SetRenderDrawColor(renderer, 35, 31, 24, 255);
        SDL_RenderClear(renderer);

        renderTerrain(
            renderer,
            terrain,
            cameraX,
            cameraY,
            SCREEN_WIDTH,
            SCREEN_HEIGHT,
            TILE_SIZE,
            MAP_WIDTH,
            MAP_HEIGHT
        );

        int worldCursorX = (int)(cameraX + cursorX);
        int worldCursorY = (int)(cameraY + cursorY);

        int hoverTileX = worldCursorX / TILE_SIZE;
        int hoverTileY = worldCursorY / TILE_SIZE;

        if (
            circlePressed &&
            (
                gameUI.section == GameUISection::School ||
                gameUI.section == GameUISection::Store ||
                gameUI.section == GameUISection::Bakery ||
                gameUI.section == GameUISection::Farm
            )
        ) {
            gameUI.section =
                GameUISection::Build;

            gameUI.selectedSchoolBuilding = -1;
            gameUI.selectedStoreBuilding = -1;
            gameUI.selectedBakeryBuilding = -1;
            gameUI.selectedFarmBuilding = -1;
            gameUI.buildSectionActive = true;

            circlePressed = false;

        } else if (circlePressed) {
            for (int i = buildingCount - 1; i >= 0; --i) {
                if (!buildings[i].active)
                    continue;

                int buildingWidth =
                    getBuildingWidth(buildings[i].type);

                int buildingHeight =
                    getBuildingHeight(buildings[i].type);

                bool cursorInsideBuilding =
                    hoverTileX >= buildings[i].tileX &&
                    hoverTileX <
                        buildings[i].tileX + buildingWidth &&
                    hoverTileY >= buildings[i].tileY &&
                    hoverTileY <
                        buildings[i].tileY + buildingHeight;

                if (cursorInsideBuilding) {
                    buildings[i].active = false;
                    break;
                }
            }
        }

        if (
            !buildMenuOpen &&
            (
                crossPressed ||
                touchTapped
            )
        ) {
            const float houseSelectScreenX =
                touchTapped
                    ? touchStartX
                    : cursorX;

            const float houseSelectScreenY =
                touchTapped
                    ? touchStartY
                    : cursorY;

            const int houseSelectTileX =
                (
                    (int)houseSelectScreenX +
                    (int)cameraX
                ) /
                TILE_SIZE;

            const int houseSelectTileY =
                (
                    (int)houseSelectScreenY +
                    (int)cameraY
                ) /
                TILE_SIZE;

            bool openedStore = false;
            bool openedSchool = false;
            bool openedBakery = false;
            bool openedFarm = false;

            if (
                houseSelectScreenX >=
                    GAME_UI_PANEL_WIDTH &&
                houseSelectTileX >= 0 &&
                houseSelectTileX < MAP_WIDTH &&
                houseSelectTileY >= 0 &&
                houseSelectTileY < MAP_HEIGHT
            ) {
                // Store ma stejnou prioritu jako ostatni
                // House detail pages. Unit selection uz
                // probehl drive a pripadny vstup spotreboval.
                for (
                    int i = buildingCount - 1;
                    i >= 0;
                    --i
                ) {
                    if (!buildings[i].active)
                        continue;

                    if (
                        buildings[i].type !=
                            BuildingType::Store
                    ) {
                        continue;
                    }

                    if (!isBuildingComplete(buildings[i]))
                        continue;

                    const int width =
                        getBuildingWidth(
                            buildings[i].type
                        );

                    const int height =
                        getBuildingHeight(
                            buildings[i].type
                        );

                    const bool insideStore =
                        houseSelectTileX >=
                            buildings[i].tileX &&
                        houseSelectTileX <
                            buildings[i].tileX +
                                width &&
                        houseSelectTileY >=
                            buildings[i].tileY &&
                        houseSelectTileY <
                            buildings[i].tileY +
                                height;

                    if (!insideStore)
                        continue;

                    gameUI.section =
                        GameUISection::Store;

                    gameUI.selectedStoreBuilding =
                        i;

                    gameUI.storeWareSelection = 0;

                    gameUI.selectedSchoolBuilding =
                        -1;

                    gameUI.selectedBakeryBuilding =
                        -1;

                    gameUI.selectedFarmBuilding =
                        -1;

                    gameUI.buildSectionActive =
                        false;

                    gameUI.toolbarFocus =
                        false;

                    openedStore = true;

                    selectedTileX = -1;
                    selectedTileY = -1;

                    crossPressed = false;

                    if (touchTapped)
                        touchTapped = false;

                    break;
                }

                // -----------------------------------------
                // Dokoncena Bakery
                //
                // Stejny House-selection model jako
                // Store a School. Zadna vlastni temporary
                // grafika se zde nevytvari.
                // -----------------------------------------

                if (!openedStore) {
                    for (
                        int i = buildingCount - 1;
                        i >= 0;
                        --i
                    ) {
                        if (!buildings[i].active)
                            continue;

                        if (
                            buildings[i].type !=
                                BuildingType::Bakery
                        ) {
                            continue;
                        }

                        if (
                            !isBuildingComplete(
                                buildings[i]
                            )
                        ) {
                            continue;
                        }

                        const int width =
                            getBuildingWidth(
                                buildings[i].type
                            );

                        const int height =
                            getBuildingHeight(
                                buildings[i].type
                            );

                        const bool insideBakery =
                            houseSelectTileX >=
                                buildings[i].tileX &&
                            houseSelectTileX <
                                buildings[i].tileX +
                                    width &&
                            houseSelectTileY >=
                                buildings[i].tileY &&
                            houseSelectTileY <
                                buildings[i].tileY +
                                    height;

                        if (!insideBakery)
                            continue;

                        gameUI.section =
                            GameUISection::Bakery;

                        gameUI.selectedBakeryBuilding =
                            i;

                        gameUI.selectedStoreBuilding =
                            -1;

                        gameUI.selectedSchoolBuilding =
                            -1;

                        gameUI.selectedFarmBuilding =
                            -1;

                        gameUI.buildSectionActive =
                            false;

                        gameUI.toolbarFocus =
                            false;

                        openedBakery = true;

                        selectedTileX = -1;
                        selectedTileY = -1;

                        crossPressed = false;

                        if (touchTapped)
                            touchTapped = false;

                        break;
                    }
                }

                // -----------------------------------------
                // Dokoncena Farm
                //
                // Stejny House-selection model jako Bakery,
                // Store a School.
                // -----------------------------------------

                if (
                    !openedStore &&
                    !openedBakery
                ) {
                    for (
                        int i = buildingCount - 1;
                        i >= 0;
                        --i
                    ) {
                        if (!buildings[i].active)
                            continue;

                        if (
                            buildings[i].type !=
                                BuildingType::Farm
                        ) {
                            continue;
                        }

                        if (
                            !isBuildingComplete(
                                buildings[i]
                            )
                        ) {
                            continue;
                        }

                        // ---------------------------------
                        // FARM VISUAL GUI HITBOX
                        //
                        // Original Farm stone sprite:
                        //   175x124 KaM pixels.
                        //
                        // Logical footprint:
                        //   4x3 tiles.
                        //
                        // KaM coordinates use 40 px tiles,
                        // KaMVita uses TILE_SIZE = 32.
                        //
                        // The visual Farm sprite is therefore
                        // wider and slightly taller than the
                        // logical footprint.
                        //
                        // Use both:
                        //   1. complete logical footprint,
                        //   2. actual rendered sprite rectangle.
                        // ---------------------------------

                        constexpr int
                            FARM_SPRITE_WIDTH = 175;

                        constexpr int
                            FARM_SPRITE_HEIGHT = 124;

                        const int farmWidth =
                            getBuildingWidth(
                                buildings[i].type
                            );

                        const int farmHeight =
                            getBuildingHeight(
                                buildings[i].type
                            );

                        const int footprintScreenX =
                            buildings[i].tileX *
                                TILE_SIZE -
                            (int)cameraX;

                        const int footprintScreenY =
                            buildings[i].tileY *
                                TILE_SIZE -
                            (int)cameraY;

                        const int footprintPixelWidth =
                            farmWidth *
                            TILE_SIZE;

                        const int footprintPixelHeight =
                            farmHeight *
                            TILE_SIZE;

                        // Same 40 -> TILE_SIZE conversion
                        // used by the House renderer.
                        const int scaledFarmWidth =
                            (
                                FARM_SPRITE_WIDTH *
                                    TILE_SIZE +
                                20
                            ) / 40;

                        const int scaledFarmHeight =
                            (
                                FARM_SPRITE_HEIGHT *
                                    TILE_SIZE +
                                20
                            ) / 40;

                        // Farm has no additional visual X
                        // correction. It is centered on its
                        // logical footprint.
                        const int farmVisualX =
                            footprintScreenX +
                            (
                                footprintPixelWidth -
                                scaledFarmWidth
                            ) / 2;

                        const int farmVisualY =
                            footprintScreenY +
                            footprintPixelHeight -
                            scaledFarmHeight;

                        const bool insideFarmVisual =
                            houseSelectScreenX >=
                                farmVisualX &&
                            houseSelectScreenX <
                                farmVisualX +
                                    scaledFarmWidth &&
                            houseSelectScreenY >=
                                farmVisualY &&
                            houseSelectScreenY <
                                farmVisualY +
                                    scaledFarmHeight;

                        const bool insideFarmFootprint =
                            houseSelectTileX >=
                                buildings[i].tileX &&
                            houseSelectTileX <
                                buildings[i].tileX +
                                    farmWidth &&
                            houseSelectTileY >=
                                buildings[i].tileY &&
                            houseSelectTileY <
                                buildings[i].tileY +
                                    farmHeight;

                        const bool insideFarm =
                            insideFarmVisual ||
                            insideFarmFootprint;

                        if (!insideFarm)
                            continue;

                        gameUI.section =
                            GameUISection::Farm;

                        gameUI.selectedFarmBuilding =
                            i;

                        gameUI.selectedStoreBuilding =
                            -1;

                        gameUI.selectedBakeryBuilding =
                            -1;

                        gameUI.selectedSchoolBuilding =
                            -1;

                        gameUI.buildSectionActive =
                            false;

                        gameUI.toolbarFocus =
                            false;

                        openedFarm = true;

                        selectedTileX = -1;
                        selectedTileY = -1;

                        crossPressed = false;

                        if (touchTapped)
                            touchTapped = false;

                        break;
                    }
                }


                // Nakonec zkusime dokoncenou School.

            if (
                !openedStore &&
                !openedBakery &&
                !openedFarm
            ) {
            for (int i = buildingCount - 1;
                 i >= 0;
                 --i) {

                if (!buildings[i].active)
                    continue;

                if (buildings[i].type !=
                    BuildingType::School) {
                    continue;
                }

                if (!isBuildingComplete(
                        buildings[i])) {
                    continue;
                }

                int width =
                    getBuildingWidth(
                        buildings[i].type
                    );

                int height =
                    getBuildingHeight(
                        buildings[i].type
                    );

                bool insideSchool =
                    hoverTileX >=
                        buildings[i].tileX &&
                    hoverTileX <
                        buildings[i].tileX + width &&
                    hoverTileY >=
                        buildings[i].tileY &&
                    hoverTileY <
                        buildings[i].tileY + height;

                if (!insideSchool)
                    continue;

                gameUI.section =
                    GameUISection::School;

                gameUI.schoolSelection =
                    GameUISchoolSelection::Serf;

                gameUI.selectedSchoolBuilding = i;
                gameUI.selectedStoreBuilding = -1;
                gameUI.selectedBakeryBuilding = -1;
                gameUI.selectedFarmBuilding = -1;

                gameUI.buildSectionActive = false;

                openedSchool = true;

                selectedTileX = -1;
                selectedTileY = -1;

                crossPressed = false;

                if (touchTapped)
                    touchTapped = false;

                break;
            }
            }

            if (
                !openedStore &&
                !openedBakery &&
                !openedFarm &&
                !openedSchool &&
                gameUI.section ==
                    GameUISection::Build &&
                crossPressed &&
                houseSelectTileX >= 0 &&
                houseSelectTileX < MAP_WIDTH &&
                houseSelectTileY >= 0 &&
                houseSelectTileY < MAP_HEIGHT
            ) {
                selectedTileX =
                    houseSelectTileX;

                selectedTileY =
                    houseSelectTileY;
            }
            }
        }
        // -------------------------------------------------
        // Field tool GUI 337
        //
        // Prvni runtime implementace Field planu.
        //
        // Square:
        // None  -> Field
        // Field -> None
        //
        // Field plan nesmi prekryvat:
        // - aktivni building footprint,
        // - zadny RoadState.
        // -------------------------------------------------

        if (
            !buildMenuOpen &&
            activeBuildTool == BuildTool::Field &&
            squarePressed &&
            hoverTileX >= 0 &&
            hoverTileX < MAP_WIDTH &&
            hoverTileY >= 0 &&
            hoverTileY < MAP_HEIGHT
        ) {
            bool blockedByBuilding = false;

            for (
                int buildingIndex = 0;
                buildingIndex < buildingCount;
                ++buildingIndex
            ) {
                const Building& building =
                    buildings[buildingIndex];

                if (!building.active)
                    continue;

                const int localX =
                    hoverTileX -
                    building.tileX;

                const int localY =
                    hoverTileY -
                    building.tileY;

                if (
                    localX < 0 ||
                    localY < 0
                ) {
                    continue;
                }

                const int width =
                    getBuildingWidth(
                        building.type
                    );

                const int height =
                    getBuildingHeight(
                        building.type
                    );

                if (
                    localX >= width ||
                    localY >= height
                ) {
                    continue;
                }

                if (
                    getBuildingPlanValue(
                        building.type,
                        localX,
                        localY
                    ) != 0
                ) {
                    blockedByBuilding = true;
                    break;
                }
            }

            const bool blockedByRoad =
                roads[
                    hoverTileY
                ][
                    hoverTileX
                ] != RoadState::None;

            if (
                !blockedByBuilding &&
                !blockedByRoad
            ) {
                LandPlanState& plan =
                    landPlans[
                        hoverTileY
                    ][
                        hoverTileX
                    ];

                if (
                    plan ==
                    LandPlanState::Field
                ) {
                    plan =
                        LandPlanState::None;
                } else if (
                    plan ==
                    LandPlanState::None
                ) {
                    plan =
                        LandPlanState::Field;
                }
            }
        }


        // -------------------------------------------------
        // Wine tool GUI 336
        //
        // Pouziva stejnou LandPlanState vrstvu jako Field.
        //
        // Square:
        // None  -> Wine
        // Wine  -> None
        // Field -> beze zmeny
        //
        // KaM Remake CanAddFieldPlan vyzaduje,
        // aby na dlazdici nebyl zadny jiny field plan.
        //
        // Wine plan nesmi prekryvat:
        // - aktivni building footprint,
        // - zadny RoadState.
        // -------------------------------------------------

        if (
            !buildMenuOpen &&
            activeBuildTool == BuildTool::Wine &&
            squarePressed &&
            hoverTileX >= 0 &&
            hoverTileX < MAP_WIDTH &&
            hoverTileY >= 0 &&
            hoverTileY < MAP_HEIGHT
        ) {
            bool blockedByBuilding = false;

            for (
                int buildingIndex = 0;
                buildingIndex < buildingCount;
                ++buildingIndex
            ) {
                const Building& building =
                    buildings[buildingIndex];

                if (!building.active)
                    continue;

                const int localX =
                    hoverTileX -
                    building.tileX;

                const int localY =
                    hoverTileY -
                    building.tileY;

                if (
                    localX < 0 ||
                    localY < 0
                ) {
                    continue;
                }

                const int width =
                    getBuildingWidth(
                        building.type
                    );

                const int height =
                    getBuildingHeight(
                        building.type
                    );

                if (
                    localX >= width ||
                    localY >= height
                ) {
                    continue;
                }

                if (
                    getBuildingPlanValue(
                        building.type,
                        localX,
                        localY
                    ) != 0
                ) {
                    blockedByBuilding = true;
                    break;
                }
            }

            const bool blockedByRoad =
                roads[
                    hoverTileY
                ][
                    hoverTileX
                ] != RoadState::None;

            if (
                !blockedByBuilding &&
                !blockedByRoad
            ) {
                LandPlanState& plan =
                    landPlans[
                        hoverTileY
                    ][
                        hoverTileX
                    ];

                if (
                    plan ==
                    LandPlanState::Wine
                ) {
                    // Stejny typ planu se druhym
                    // stiskem zrusi.
                    plan =
                        LandPlanState::None;
                } else if (
                    plan ==
                    LandPlanState::None
                ) {
                    // Jiny FieldType nesmi prepsat
                    // existujici plan na dlazdici.
                    plan =
                        LandPlanState::Wine;
                }
            }
        }


        // -------------------------------------------------
        // Remove tool GUI 340
        //
        // Bezpecny Remove rusi:
        // - nedokoncene stavebni plany,
        // - RoadState::Plan,
        // - Field plan,
        // - Wine plan.
        //
        // Hotove buildings a RoadState::Complete chrani.
        // -------------------------------------------------

        if (
            !buildMenuOpen &&
            activeBuildTool == BuildTool::Remove &&
            squarePressed &&
            hoverTileX >= 0 &&
            hoverTileX < MAP_WIDTH &&
            hoverTileY >= 0 &&
            hoverTileY < MAP_HEIGHT
        ) {
            bool removedBuildingPlan = false;

            for (
                int buildingIndex = buildingCount - 1;
                buildingIndex >= 0;
                --buildingIndex
            ) {
                Building& building =
                    buildings[buildingIndex];

                if (!building.active)
                    continue;

                if (isBuildingComplete(building))
                    continue;

                const int width =
                    getBuildingWidth(
                        building.type
                    );

                const int height =
                    getBuildingHeight(
                        building.type
                    );

                const bool cursorInsideBuilding =
                    hoverTileX >= building.tileX &&
                    hoverTileX <
                        building.tileX + width &&
                    hoverTileY >= building.tileY &&
                    hoverTileY <
                        building.tileY + height;

                if (!cursorInsideBuilding)
                    continue;

                // Zrus pripadne odkazy jednotek
                // na recyklovany Building slot.
                for (
                    int unitIndex = 0;
                    unitIndex < unitCount;
                    ++unitIndex
                ) {
                    if (
                        units[unitIndex].targetBuilding ==
                        buildingIndex
                    ) {
                        units[unitIndex].targetBuilding = -1;
                    }
                }

                building = {};
                building.active = false;

                if (
                    gameUI.selectedSchoolBuilding ==
                    buildingIndex
                ) {
                    gameUI.selectedSchoolBuilding = -1;
                }

                if (
                    gameUI.selectedStoreBuilding ==
                    buildingIndex
                ) {
                    gameUI.selectedStoreBuilding = -1;
                }

                if (
                    gameUI.selectedBakeryBuilding ==
                    buildingIndex
                ) {
                    gameUI.selectedBakeryBuilding = -1;
                }

                if (
                    gameUI.selectedFarmBuilding ==
                    buildingIndex
                ) {
                    gameUI.selectedFarmBuilding = -1;
                }

                selectedTileX = -1;
                selectedTileY = -1;

                removedBuildingPlan = true;
                break;
            }

            // Pokud pod kurzorem nebyl house plan:
            //
            // 1. zrus RoadState::Plan,
            // 2. jinak zrus Field/Wine plan.
            //
            // Hotova cesta zustava chranena.
            if (!removedBuildingPlan) {
                RoadState& road =
                    roads[hoverTileY][hoverTileX];

                if (road == RoadState::Plan) {
                    road = RoadState::None;
                } else if (
                    landPlans[
                        hoverTileY
                    ][
                        hoverTileX
                    ] != LandPlanState::None
                ) {
                    landPlans[
                        hoverTileY
                    ][
                        hoverTileX
                    ] = LandPlanState::None;
                }
            }
        }

        // Road tool: Square prepina plan cesty
        // na dlazdici pod kurzorem.
        if (!buildMenuOpen &&
            activeBuildTool == BuildTool::Road &&
            squarePressed &&
            hoverTileX >= 0 &&
            hoverTileX < MAP_WIDTH &&
            hoverTileY >= 0 &&
            hoverTileY < MAP_HEIGHT) {

            bool blockedByBuilding = false;

            for (int i = 0;
                 i < buildingCount;
                 ++i) {

                if (!buildings[i].active)
                    continue;

                int localX =
                    hoverTileX -
                    buildings[i].tileX;

                int localY =
                    hoverTileY -
                    buildings[i].tileY;

                if (localX < 0 || localY < 0)
                    continue;

                int width =
                    getBuildingWidth(
                        buildings[i].type
                    );

                int height =
                    getBuildingHeight(
                        buildings[i].type
                    );

                if (localX >= width ||
                    localY >= height) {
                    continue;
                }

                if (getBuildingPlanValue(
                        buildings[i].type,
                        localX,
                        localY
                    ) != 0) {

                    blockedByBuilding = true;
                    break;
                }
            }

            if (
                !blockedByBuilding &&
                landPlans[
                    hoverTileY
                ][
                    hoverTileX
                ] == LandPlanState::None
            ) {
                RoadState& road =
                    roads[hoverTileY][hoverTileX];

                if (road == RoadState::None) {
                    road = RoadState::Plan;
                } else if (
                    road != RoadState::Complete
                ) {
                    road = RoadState::None;
                }
            }
        }

        int buildingPlacementTileX =
            selectedTileX;

        int buildingPlacementTileY =
            selectedTileY;

        if (
            activeBuildTool ==
                BuildTool::Building &&
            selectedTileX >= 0 &&
            selectedTileY >= 0
        ) {
            getBuildingTopLeftFromEntrance(
                selectedBuildingType,
                selectedTileX,
                selectedTileY,
                &buildingPlacementTileX,
                &buildingPlacementTileY
            );
        }

        if (!buildMenuOpen &&
            activeBuildTool == BuildTool::Building &&
            squarePressed &&
            canPlaceBuilding(
                buildings,
                buildingCount,
                selectedBuildingType,
                buildingPlacementTileX,
                buildingPlacementTileY,
                MAP_WIDTH,
                MAP_HEIGHT
            )) {

            int freeSlot = -1;

            for (int i = 0; i < buildingCount; ++i) {
                if (!buildings[i].active) {
                    freeSlot = i;
                    break;
                }
            }

            if (freeSlot == -1 && buildingCount < MAX_BUILDINGS) {
                freeSlot = buildingCount;
                buildingCount++;
            }

            if (freeSlot >= 0) {
                // Recyklovany slot nesmi zdedit stav stare budovy.
                buildings[freeSlot] = {};

                for (
                    int queueIndex = 0;
                    queueIndex < SCHOOL_QUEUE_SIZE;
                    ++queueIndex
                ) {
                    buildings[freeSlot]
                        .schoolQueue[queueIndex] = -1;
                }

                // Internal save/runtime position remains
                // the top-left of the active footprint.
                buildings[freeSlot].tileX =
                    buildingPlacementTileX;

                buildings[freeSlot].tileY =
                    buildingPlacementTileY;
                buildings[freeSlot].type = selectedBuildingType;
                buildings[freeSlot].constructionProgress = 0;
                buildings[freeSlot].woodDelivered = 0;
                buildings[freeSlot].stoneDelivered = 0;
                buildings[freeSlot].woodStep = 0;
                buildings[freeSlot].stoneStep = 0;

                initializeHouseRuntimeState(
                    &buildings[freeSlot]
                );

                buildings[freeSlot].active = true;

                // Po polozeni planu uz nechceme
                // automaticky pokladat dalsi stejnou budovu.
                activeBuildTool = BuildTool::Road;

                selectedTileX = -1;
                selectedTileY = -1;
            }
        }


        // -------------------------------------------------
        // Field / Wine planning overlay
        //
        // Docasny diagnosticky visual pro Field plan.
        // Originalni KaM plan rendering prijde az po
        // overeni mapove logiky.
        // -------------------------------------------------

        SDL_SetRenderDrawBlendMode(
            renderer,
            SDL_BLENDMODE_BLEND
        );

        for (
            int fieldY = 0;
            fieldY < MAP_HEIGHT;
            ++fieldY
        ) {
            for (
                int fieldX = 0;
                fieldX < MAP_WIDTH;
                ++fieldX
            ) {
                const LandPlanState landPlan =
                    landPlans[
                        fieldY
                    ][
                        fieldX
                    ];

                if (
                    landPlan ==
                    LandPlanState::None
                ) {
                    continue;
                }

                SDL_Rect fieldPlanRect = {
                    fieldX * TILE_SIZE -
                        (int)cameraX,
                    fieldY * TILE_SIZE -
                        (int)cameraY,
                    TILE_SIZE,
                    TILE_SIZE
                };

                if (
                    landPlan ==
                        LandPlanState::FieldComplete ||
                    landPlan ==
                        LandPlanState::FieldSown ||
                    landPlan ==
                        LandPlanState::FieldYoung ||
                    landPlan ==
                        LandPlanState::FieldGrowing ||
                    landPlan ==
                        LandPlanState::FieldGreen ||
                    landPlan ==
                        LandPlanState::FieldRipe ||
                    landPlan ==
                        LandPlanState::FieldCut ||
                    landPlan ==
                        LandPlanState::WineInit ||
                    landPlan ==
                        LandPlanState::WineComplete
                ) {
                    SDL_Texture* completedTexture =
                        nullptr;

                    if (
                        landPlan ==
                            LandPlanState::FieldComplete
                    ) {
                        completedTexture =
                            terrain.cornEmpty;
                    } else if (
                        landPlan ==
                            LandPlanState::FieldSown
                    ) {
                        completedTexture =
                            terrain.cornSown;
                    } else if (
                        landPlan ==
                            LandPlanState::FieldYoung
                    ) {
                        completedTexture =
                            terrain.cornYoung;
                    } else if (
                        landPlan ==
                            LandPlanState::FieldGrowing ||
                        landPlan ==
                            LandPlanState::FieldGreen ||
                        landPlan ==
                            LandPlanState::FieldRipe
                    ) {
                        completedTexture =
                            terrain.cornGround;
                    } else if (
                        landPlan ==
                            LandPlanState::FieldCut
                    ) {
                        // Originalni CutCorn Terrain 63.
                        completedTexture =
                            terrain.cornCut;
                    } else {
                        completedTexture =
                            terrain.wineGround;
                    }

                    if (completedTexture) {
                        // Originalni KaM terrain:
                        //
                        // Field stage 0 = Terrain 62
                        // Wine ground   = Terrain 55
                        //
                        // Assety jsou nativne 32x32,
                        // stejne jako KaMVita TILE_SIZE.
                        SDL_RenderCopy(
                            renderer,
                            completedTexture,
                            nullptr,
                            &fieldPlanRect
                        );


                    }

                    // Hotovy teren uz nema barevny
                    // planovaci overlay.
                    continue;
                }

                SDL_Texture* planMarkup =
                    landPlan ==
                        LandPlanState::Field
                            ? kamPlanResources.fieldMarkup
                            : kamPlanResources.wineMarkup;

                renderKaMMarkup(
                    renderer,
                    planMarkup,
                    fieldPlanRect.x,
                    fieldPlanRect.y,
                    TILE_SIZE
                );
            }
        }

        SDL_SetRenderDrawBlendMode(
            renderer,
            SDL_BLENDMODE_NONE
        );


        // -------------------------------------------------
        // Originalni KaM Field/Wine fences
        //
        // Connectivity odpovida UpdateFences:
        // spolecna hrana stejneho field typu se nekresli.
        // -------------------------------------------------

        for (
            int fieldY = 0;
            fieldY < MAP_HEIGHT;
            ++fieldY
        ) {
            for (
                int fieldX = 0;
                fieldX < MAP_WIDTH;
                ++fieldX
            ) {
                const LandPlanState type =
                    landPlans[fieldY][fieldX];

                const bool isCorn =
                    type ==
                        LandPlanState::FieldComplete ||
                    type ==
                        LandPlanState::FieldSown ||
                    type ==
                        LandPlanState::FieldYoung ||
                    type ==
                        LandPlanState::FieldGrowing ||
                    type ==
                        LandPlanState::FieldGreen ||
                    type ==
                        LandPlanState::FieldRipe ||
                    type ==
                        LandPlanState::FieldCut;

                const bool isWine =
                    type ==
                        LandPlanState::WineInit ||
                    type ==
                        LandPlanState::WineComplete;

                if (!isCorn && !isWine)
                    continue;

                auto sameFieldType = [
                    isCorn,
                    isWine
                ](LandPlanState neighbor) -> bool {
                    if (isCorn) {
                        return
                            neighbor ==
                                LandPlanState::FieldComplete ||
                            neighbor ==
                                LandPlanState::FieldSown ||
                            neighbor ==
                                LandPlanState::FieldYoung ||
                            neighbor ==
                                LandPlanState::FieldGrowing ||
                            neighbor ==
                                LandPlanState::FieldGreen ||
                            neighbor ==
                                LandPlanState::FieldRipe ||
                            neighbor ==
                                LandPlanState::FieldCut;
                    }

                    if (isWine) {
                        return
                            neighbor ==
                                LandPlanState::WineInit ||
                            neighbor ==
                                LandPlanState::WineComplete;
                    }

                    return false;
                };

                const bool north =
                    fieldY == 0 ||
                    !sameFieldType(
                        landPlans[
                            fieldY - 1
                        ][
                            fieldX
                        ]
                    );

                const bool left =
                    fieldX == 0 ||
                    !sameFieldType(
                        landPlans[
                            fieldY
                        ][
                            fieldX - 1
                        ]
                    );

                const bool right =
                    fieldX + 1 >= MAP_WIDTH ||
                    !sameFieldType(
                        landPlans[
                            fieldY
                        ][
                            fieldX + 1
                        ]
                    );

                const bool south =
                    fieldY + 1 >= MAP_HEIGHT ||
                    !sameFieldType(
                        landPlans[
                            fieldY + 1
                        ][
                            fieldX
                        ]
                    );

                SDL_Texture* horizontal =
                    isCorn
                        ? terrain.cornFenceHorizontal
                        : terrain.wineFenceHorizontal;

                SDL_Texture* vertical =
                    isCorn
                        ? terrain.cornFenceVertical
                        : terrain.wineFenceVertical;

                const int screenX =
                    fieldX * TILE_SIZE -
                    (int)cameraX;

                const int screenY =
                    fieldY * TILE_SIZE -
                    (int)cameraY;

                // Horizontalni rxGui zdroj 10x47 je
                // predem otoceny na 47x10.
                //
                // Prevod KaM 40 px -> KaMVita 32 px:
                // 47x10 -> 37x8.
                const int horizontalWidth = 37;
                const int horizontalHeight = 8;

                // Verticalni zdroj zustava neotoceny:
                // Corn 8 px -> 6 px
                // Wine 5 px -> 4 px
                const int verticalWidth =
                    isCorn ? 6 : 4;

                // Originalni quad ma vysku 44 KaM px.
                // 44 * 32 / 40 = 35 Vita px.
                const int verticalHeight = 35;

                // Original RenderFence ma presah 3/40
                // dlazdice na kazde strane.
                const int horizontalX =
                    screenX - 2;

                // VO = -4 KaM px -> priblizne +3 Vita px.
                const int northBottom =
                    screenY + 3;

                const int southBottom =
                    screenY +
                    TILE_SIZE +
                    3;

                if (north && horizontal) {
                    SDL_Rect destination = {
                        horizontalX,
                        northBottom -
                            horizontalHeight,
                        horizontalWidth,
                        horizontalHeight
                    };

                    SDL_RenderCopy(
                        renderer,
                        horizontal,
                        nullptr,
                        &destination
                    );
                }

                if (south && horizontal) {
                    SDL_Rect destination = {
                        horizontalX,
                        southBottom -
                            horizontalHeight,
                        horizontalWidth,
                        horizontalHeight
                    };

                    SDL_RenderCopy(
                        renderer,
                        horizontal,
                        nullptr,
                        &destination
                    );
                }

                // Verticalni segment zacina na horni
                // hrane a presahuje 3 px pod dlazdici.
                const int verticalY =
                    screenY;

                if (left && vertical) {
                    SDL_Rect destination = {
                        screenX -
                            verticalWidth +
                            2,
                        verticalY,
                        verticalWidth,
                        verticalHeight
                    };

                    SDL_RenderCopy(
                        renderer,
                        vertical,
                        nullptr,
                        &destination
                    );
                }

                if (right && vertical) {
                    SDL_Rect destination = {
                        screenX +
                            TILE_SIZE -
                            2,
                        verticalY,
                        verticalWidth,
                        verticalHeight
                    };

                    SDL_RenderCopy(
                        renderer,
                        vertical,
                        nullptr,
                        &destination
                    );
                }
            }
        }


        // =================================================
        // ORIGINAL KaM ANIMATED CORN OBJECTS
        //
        // Foreground pass after Field/Wine fences.
        // Render order:
        // terrain -> fences -> Obj58 / Obj59.
        // =================================================

        for (
            int cornFieldY = 0;
            cornFieldY < MAP_HEIGHT;
            ++cornFieldY
        ) {
            for (
                int cornFieldX = 0;
                cornFieldX < MAP_WIDTH;
                ++cornFieldX
            ) {
                const LandPlanState cornLandPlan =
                    landPlans[
                        cornFieldY
                    ][
                        cornFieldX
                    ];

                SDL_Rect cornFieldRect = {
                    cornFieldX * TILE_SIZE -
                        (int)cameraX,

                    cornFieldY * TILE_SIZE -
                        (int)cameraY,

                    TILE_SIZE,
                    TILE_SIZE
                };

                if (
                    cornLandPlan ==
                        LandPlanState::FieldGreen ||
                    cornLandPlan ==
                        LandPlanState::FieldRipe
                ) {
                    static constexpr int
                        CORN_ANIMATION_STEPS = 12;

                    static constexpr int
                        CORN_ANIMATION_STEP_MS = 100;

                    static constexpr int
                        CORN_STEP_TO_FRAME[
                            CORN_ANIMATION_STEPS
                        ] = {
                            0, 0, 0,
                            1, 1, 1,
                            0, 0, 0,
                            2, 2, 2
                        };

                    const int cornAnimationStep =
                        (
                            currentFrameTicks /
                            CORN_ANIMATION_STEP_MS
                        ) %
                        CORN_ANIMATION_STEPS;

                    const int nextCornAnimationStep =
                        (
                            cornAnimationStep + 1
                        ) %
                        CORN_ANIMATION_STEPS;

                    const int primaryFrame =
                        CORN_STEP_TO_FRAME[
                            cornAnimationStep
                        ];

                    const int secondaryFrame =
                        CORN_STEP_TO_FRAME[
                            nextCornAnimationStep
                        ];

                    SDL_Texture** cornSprites =
                        cornLandPlan ==
                            LandPlanState::FieldGreen
                                ? terrain.
                                    cornObj58Sprites
                                : terrain.
                                    cornObj59Sprites;

                    static constexpr int
                        OBJ58_WIDTHS[3] = {
                            20, 21, 20
                        };

                    static constexpr int
                        OBJ58_HEIGHTS[3] = {
                            24, 24, 24
                        };

                    static constexpr int
                        OBJ58_PIVOT_X[3] = {
                            -9, -10, -9
                        };

                    static constexpr int
                        OBJ58_PIVOT_Y[3] = {
                            -16, -16, -16
                        };

                    static constexpr int
                        OBJ59_WIDTHS[3] = {
                            20, 22, 20
                        };

                    static constexpr int
                        OBJ59_HEIGHTS[3] = {
                            29, 29, 29
                        };

                    static constexpr int
                        OBJ59_PIVOT_X[3] = {
                            -9, -10, -9
                        };

                    static constexpr int
                        OBJ59_PIVOT_Y[3] = {
                            -22, -22, -22
                        };

                    const int* widths =
                        cornLandPlan ==
                            LandPlanState::FieldGreen
                                ? OBJ58_WIDTHS
                                : OBJ59_WIDTHS;

                    const int* heights =
                        cornLandPlan ==
                            LandPlanState::FieldGreen
                                ? OBJ58_HEIGHTS
                                : OBJ59_HEIGHTS;

                    const int* pivotX =
                        cornLandPlan ==
                            LandPlanState::FieldGreen
                                ? OBJ58_PIVOT_X
                                : OBJ59_PIVOT_X;

                    const int* pivotY =
                        cornLandPlan ==
                            LandPlanState::FieldGreen
                                ? OBJ58_PIVOT_Y
                                : OBJ59_PIVOT_Y;

                    const int frameIndices[4] = {
                        primaryFrame,
                        secondaryFrame,
                        secondaryFrame,
                        primaryFrame
                    };

                    // Local quarter-tile ground anchors,
                    // equivalent to RenderMapElement4.
                    const int anchorX[4] = {
                        8, 24, 8, 24
                    };

                    const int anchorY[4] = {
                        8, 8, 24, 24
                    };

                    for (
                        int cornPart = 0;
                        cornPart < 4;
                        ++cornPart
                    ) {
                        const int frameIndex =
                            frameIndices[
                                cornPart
                            ];

                        SDL_Texture* cornSprite =
                            cornSprites[
                                frameIndex
                            ];

                        if (!cornSprite)
                            continue;

                        const int scaledWidth =
                            (
                                widths[
                                    frameIndex
                                ] *
                                TILE_SIZE +
                                20
                            ) /
                            40;

                        const int scaledHeight =
                            (
                                heights[
                                    frameIndex
                                ] *
                                TILE_SIZE +
                                20
                            ) /
                            40;

                        const int scaledPivotX =
                            (
                                pivotX[
                                    frameIndex
                                ] *
                                TILE_SIZE
                            ) /
                            40;

                        const int scaledPivotY =
                            (
                                pivotY[
                                    frameIndex
                                ] *
                                TILE_SIZE
                            ) /
                            40;

                        SDL_Rect cornSpriteRect = {
                            cornFieldRect.x +
                                anchorX[
                                    cornPart
                                ] +
                                scaledPivotX,

                            cornFieldRect.y +
                                anchorY[
                                    cornPart
                                ] +
                                scaledPivotY,

                            scaledWidth,
                            scaledHeight
                        };

                        SDL_RenderCopy(
                            renderer,
                            cornSprite,
                            nullptr,
                            &cornSpriteRect
                        );
                    }
                }
            }
        }


        // =================================================
        // ORIGINAL KaM HOUSE ENTRANCE ROAD
        //
        // Remake pri dokonceni House:
        //
        //   gTerrain.SetRoad(fHouse.Entrance, Owner)
        //
        // Entrance tedy neni specialni House textura.
        // Je to normalni road tile a musi byt soucasti
        // stejne RoadsConnectivity site jako ostatni cesty.
        //
        // KaMVita nema samostatny terrain-house overlay,
        // proto zde synchronizujeme entrance vsech hotovych
        // Houses do RoadState::Complete.
        //
        // Nemenime:
        // - House footprint
        // - House render scale
        // - House visual X anchor
        // - doorway pixel geometrii
        // =================================================

        for (
            int buildingIndex = 0;
            buildingIndex < buildingCount;
            ++buildingIndex
        ) {
            const Building& building =
                buildings[buildingIndex];

            if (
                !building.active ||
                !isBuildingComplete(building)
            ) {
                continue;
            }

            const BuildingEntrance entrance =
                getBuildingEntrance(
                    building.type
                );

            const int entranceX =
                building.tileX +
                entrance.tileOffsetX;

            const int entranceY =
                building.tileY +
                entrance.tileOffsetY;

            if (
                entranceX < 0 ||
                entranceX >= MAP_WIDTH ||
                entranceY < 0 ||
                entranceY >= MAP_HEIGHT
            ) {
                continue;
            }

            roads[
                entranceY
            ][
                entranceX
            ] = RoadState::Complete;
        }


        // Vykresleni planu a fazi vystavby cest.
        SDL_SetRenderDrawBlendMode(
            renderer,
            SDL_BLENDMODE_BLEND
        );

        for (int roadY = 0;
             roadY < MAP_HEIGHT;
             ++roadY) {

            for (int roadX = 0;
                 roadX < MAP_WIDTH;
                 ++roadX) {

                RoadState roadState =
                    roads[roadY][roadX];

                if (roadState == RoadState::None)
                    continue;

                SDL_Rect destination = {
                    roadX * TILE_SIZE -
                        (int)cameraX,

                    roadY * TILE_SIZE -
                        (int)cameraY,

                    TILE_SIZE,
                    TILE_SIZE
                };

                SDL_Texture* roadTexture = nullptr;

                if (roadState == RoadState::Plan) {
                    renderKaMMarkup(
                        renderer,
                        kamPlanResources.roadMarkup,
                        destination.x,
                        destination.y,
                        TILE_SIZE
                    );

                    continue;
                }

                switch (roadState) {
                    case RoadState::Dig1:
                        roadTexture = terrain.roadDig1;
                        break;

                    case RoadState::Dig2:
                        roadTexture = terrain.roadDig2;
                        break;

                    case RoadState::Dig3:
                        roadTexture = terrain.roadDig3;
                        break;

                    case RoadState::Dig4:
                        roadTexture = terrain.roadDig4;
                        break;

                    case RoadState::Complete: {
                        // Originalni KaM RoadsConnectivity.
                        // Maska:
                        // bit 0 = sever
                        // bit 1 = vychod
                        // bit 2 = jih
                        // bit 3 = zapad
                        static const int roadTile[16] = {
                            248, 248, 248, 250,
                            248, 248, 250, 252,
                            248, 250, 248, 252,
                            250, 252, 252, 254
                        };

                        static const int roadRotation[16] = {
                            0, 0, 1, 3,
                            0, 0, 0, 0,
                            1, 2, 1, 3,
                            1, 2, 1, 0
                        };

                        int roadMask = 0;

                        if (
                            roadY > 0 &&
                            roads[roadY - 1][roadX] ==
                                RoadState::Complete
                        ) {
                            roadMask |= 1;
                        }

                        if (
                            roadX + 1 < MAP_WIDTH &&
                            roads[roadY][roadX + 1] ==
                                RoadState::Complete
                        ) {
                            roadMask |= 2;
                        }

                        if (
                            roadY + 1 < MAP_HEIGHT &&
                            roads[roadY + 1][roadX] ==
                                RoadState::Complete
                        ) {
                            roadMask |= 4;
                        }

                        if (
                            roadX > 0 &&
                            roads[roadY][roadX - 1] ==
                                RoadState::Complete
                        ) {
                            roadMask |= 8;
                        }

                        int tileId =
                            roadTile[roadMask];

                        SDL_Texture* completedRoad =
                            nullptr;

                        if (tileId == 248)
                            completedRoad =
                                terrain.road248;
                        else if (tileId == 250)
                            completedRoad =
                                terrain.road250;
                        else if (tileId == 252)
                            completedRoad =
                                terrain.road252;
                        else
                            completedRoad =
                                terrain.road254;

                        if (completedRoad) {
                            SDL_RenderCopyEx(
                                renderer,
                                completedRoad,
                                nullptr,
                                &destination,
                                roadRotation[roadMask] *
                                    90.0,
                                nullptr,
                                SDL_FLIP_NONE
                            );
                        }

                        // Hotova cesta uz byla vykreslena
                        // pres SDL_RenderCopyEx.
                        roadTexture = nullptr;
                        break;
                    }

                    case RoadState::Plan:
                    case RoadState::None:
                        break;
                }

                if (roadTexture) {
                    SDL_SetTextureBlendMode(
                        roadTexture,
                        SDL_BLENDMODE_BLEND
                    );

                    SDL_SetTextureAlphaMod(
                        roadTexture,
                        255
                    );

                    SDL_RenderCopy(
                        renderer,
                        roadTexture,
                        nullptr,
                        &destination
                    );
                } else if (
                    roadState == RoadState::Plan
                ) {
                    // Plan zustava pruhlednou znackou,
                    // dokud Worker nezacne kopat.
                    SDL_SetRenderDrawColor(
                        renderer,
                        210, 180, 95, 105
                    );

                    SDL_RenderFillRect(
                        renderer,
                        &destination
                    );

                    SDL_SetRenderDrawColor(
                        renderer,
                        245, 211, 125, 220
                    );

                    SDL_RenderDrawRect(
                        renderer,
                        &destination
                    );

                    SDL_RenderDrawLine(
                        renderer,
                        destination.x + 7,
                        destination.y +
                            destination.h / 2,
                        destination.x +
                            destination.w - 7,
                        destination.y +
                            destination.h / 2
                    );

                    SDL_RenderDrawLine(
                        renderer,
                        destination.x +
                            destination.w / 2,
                        destination.y + 7,
                        destination.x +
                            destination.w / 2,
                        destination.y +
                            destination.h - 7
                    );
                }
            }
        }

        SDL_SetRenderDrawBlendMode(
            renderer,
            SDL_BLENDMODE_NONE
        );

        // -------------------------------------------------
        // SCHOOL TRAINING UPDATE
        // -------------------------------------------------
        // Jeden logicky krok prozatim trva 0,1 sekundy.
        // Presna herni rychlost se muze pozdeji doladit
        // bez zmeny ulozeneho progressu.
        constexpr float SCHOOL_STEP_SECONDS = 0.1f;

        for (
            int schoolIndex = 0;
            schoolIndex < buildingCount;
            ++schoolIndex
        ) {
            Building& school =
                buildings[schoolIndex];

            if (
                !school.active ||
                school.type != BuildingType::School ||
                !isBuildingComplete(school)
            ) {
                schoolStepTimers[schoolIndex] = 0.0f;
                continue;
            }

            int profession = school.schoolQueue[0];

            bool validProfession =
                profession >= 0 &&
                profession < SCHOOL_PROFESSION_COUNT;

            if (!validProfession) {
                school.schoolTrainingActive = false;
                school.schoolFinishingTraining = false;
                school.schoolTrainingStep = 0;
                school.schoolIdleStep = 0;
                schoolStepTimers[schoolIndex] = 0.0f;
                continue;
            }

            // Dokonceni vycviku a odchod jednotky ze School.
            if (school.schoolFinishingTraining) {
                bool supportedProfession =
                    profession ==
                        static_cast<int>(
                            GameUISchoolSelection::Serf
                        ) ||
                    profession ==
                        static_cast<int>(
                            GameUISchoolSelection::Worker
                        ) ||
                    profession ==
                        static_cast<int>(
                            GameUISchoolSelection::Farmer
                        ) ||
                    profession ==
                        static_cast<int>(
                            GameUISchoolSelection::Baker
                        );

                // Ostatni profese zatim nemaji vlastni UnitType.
                // Nesmime je potichu vytvorit jako Workera.
                if (!supportedProfession) {
                    schoolStepTimers[schoolIndex] = 0.0f;
                    continue;
                }

                int freeUnitSlot = -1;

                for (
                    int unitIndex = 0;
                    unitIndex < unitCount;
                    ++unitIndex
                ) {
                    if (!units[unitIndex].active) {
                        freeUnitSlot = unitIndex;
                        break;
                    }
                }

                if (
                    freeUnitSlot == -1 &&
                    unitCount < MAX_UNITS
                ) {
                    freeUnitSlot = unitCount;
                    unitCount++;
                }

                // Pokud neni volne misto, hotova jednotka
                // zustane uvnitr School a zkusime to znovu.
                if (freeUnitSlot < 0) {
                    schoolStepTimers[schoolIndex] = 0.0f;
                    continue;
                }

                Unit& trainedUnit =
                    units[freeUnitSlot];

                trainedUnit = {};

                if (
                    profession ==
                    static_cast<int>(
                        GameUISchoolSelection::Serf
                    )
                ) {
                    trainedUnit.type =
                        UnitType::Serf;
                } else if (
                    profession ==
                    static_cast<int>(
                        GameUISchoolSelection::Farmer
                    )
                ) {
                    trainedUnit.type =
                        UnitType::Farmer;
                } else if (
                    profession ==
                    static_cast<int>(
                        GameUISchoolSelection::Baker
                    )
                ) {
                    trainedUnit.type =
                        UnitType::Baker;
                } else {
                    trainedUnit.type =
                        UnitType::Worker;
                }

                trainedUnit.condition =
                    UNIT_START_CONDITION;

                BuildingEntrance entrance =
                    getBuildingEntrance(
                        school.type
                    );

                // =========================================
                // SCHOOL TRAINING EXIT
                //
                // Puvodni logicka osa School entrance tile.
                //
                // Stary development/debug spawn obsahoval
                // pevny posun -18 px. Jednotka proto stale
                // vychazela z pozice odstraneneho debug
                // krizku.
                //
                // Training spawn nepouziva Serf transportni
                // doorway. Vratime jej na stred originalni
                // School entrance tile.
                // =========================================

                trainedUnit.doorwayStartX =
                    (
                        school.tileX +
                        entrance.tileOffsetX +
                        0.5f
                    ) * TILE_SIZE;

                trainedUnit.doorwayStartY =
                    (
                        school.tileY +
                        entrance.tileOffsetY +
                        0.5f
                    ) * TILE_SIZE;

                // Ulice je jednu dlazdici pod vstupem.
                trainedUnit.doorwayTargetX =
                    trainedUnit.doorwayStartX;

                trainedUnit.doorwayTargetY =
                    trainedUnit.doorwayStartY +
                    TILE_SIZE;

                trainedUnit.worldX =
                    trainedUnit.doorwayStartX;

                trainedUnit.worldY =
                    trainedUnit.doorwayStartY;

                trainedUnit.targetWorldX =
                    trainedUnit.doorwayTargetX;

                trainedUnit.targetWorldY =
                    trainedUnit.doorwayTargetY;

                // Doorway hodnoty jsou navazane na puvodni
                // KaM CELL_SIZE_PX = 40, KaMVita ma 32.
                constexpr float ORIGINAL_CELL_SIZE_PX =
                    40.0f;

                float doorwayScale =
                    TILE_SIZE /
                    ORIGINAL_CELL_SIZE_PX;

                trainedUnit.doorwayOffsetX =
                    static_cast<int>(
                        (
                            entrance.pixelOffsetX -
                            ORIGINAL_CELL_SIZE_PX /
                                2.0f
                        ) *
                        doorwayScale
                    );

                trainedUnit.doorwayOffsetY =
                    static_cast<int>(
                        entrance.pixelOffsetY *
                        doorwayScale
                    );

                trainedUnit.doorwayProgress = 0.0f;
                trainedUnit.visible = false;

                trainedUnit.state =
                    UnitState::LeavingBuilding;

                // Smer na jih:
                // N, NE, E, SE, S, SW, W, NW.
                trainedUnit.direction = 4;

                trainedUnit.animationFrame = 0;
                trainedUnit.animationTimer = 0.0f;

                trainedUnit.pathLength = 0;
                trainedUnit.pathIndex = 0;

                trainedUnit.targetBuilding = -1;

                trainedUnit.logisticsState =
                    SerfLogisticsState::Idle;

                trainedUnit.assignedWare =
                    WareType::Count;

                trainedUnit.carriedWare =
                    WareType::Count;

                trainedUnit.sourceBuilding = -1;
                trainedUnit.destinationBuilding = -1;

                trainedUnit.targetRoadX = -1;
                trainedUnit.targetRoadY = -1;

                trainedUnit.workTimer = 0.0f;
                trainedUnit.workAction =
                    UnitWorkAction::None;

                trainedUnit.active = true;

                // Posun cekajicich objednavek smerem k WIP.
                for (
                    int queueIndex = 0;
                    queueIndex <
                        SCHOOL_QUEUE_SIZE - 1;
                    ++queueIndex
                ) {
                    school.schoolQueue[queueIndex] =
                        school.schoolQueue[
                            queueIndex + 1
                        ];
                }

                school.schoolQueue[
                    SCHOOL_QUEUE_SIZE - 1
                ] = -1;

                school.schoolTrainingStep = 0;
                school.schoolIdleStep = 0;
                school.schoolTrainingActive = false;
                school.schoolFinishingTraining = false;

                schoolStepTimers[schoolIndex] = 0.0f;

                // Dalsi polozka Queue[0] se v pristim
                // pruchodu automaticky pokusi zahajit.
                continue;
            }

            // Bez Gold zustava objednavka ve fronte.
            if (
                !school.schoolTrainingActive &&
                school.schoolGold < SCHOOL_GOLD_COST
            ) {
                schoolStepTimers[schoolIndex] = 0.0f;
                continue;
            }

            if (!school.schoolTrainingActive) {
                school.schoolTrainingActive = true;
                school.schoolTrainingStep = 0;
                school.schoolIdleStep = 0;
                schoolStepTimers[schoolIndex] = 0.0f;
            }

            schoolStepTimers[schoolIndex] +=
                deltaSeconds;

            while (
                schoolStepTimers[schoolIndex] >=
                    SCHOOL_STEP_SECONDS &&
                school.schoolTrainingActive &&
                !school.schoolFinishingTraining
            ) {
                schoolStepTimers[schoolIndex] -=
                    SCHOOL_STEP_SECONDS;

                if (
                    school.schoolTrainingStep <
                        SCHOOL_TRAINING_STEPS
                ) {
                    school.schoolTrainingStep++;

                    if (
                        school.schoolTrainingStep <
                            SCHOOL_TRAINING_STEPS
                    ) {
                        continue;
                    }
                } else if (
                    school.schoolIdleStep <
                        SCHOOL_IDLE_STEPS
                ) {
                    school.schoolIdleStep++;
                }

                if (
                    school.schoolTrainingStep <
                        SCHOOL_TRAINING_STEPS ||
                    school.schoolIdleStep <
                        SCHOOL_IDLE_STEPS
                ) {
                    continue;
                }

                school.schoolTrainingActive = false;
                school.schoolFinishingTraining = true;

                if (
                    school.schoolGold >=
                        SCHOOL_GOLD_COST
                ) {
                    school.schoolGold -=
                        SCHOOL_GOLD_COST;
                }

                schoolStepTimers[schoolIndex] = 0.0f;
            }
        }

        // =================================================
        // BAKERY PRODUCTION UPDATE
        //
        // Originalni prevod:
        // 1 Flour -> 2 Bread
        //
        // Start:
        // - aktivni dokoncena Bakery
        // - alespon 1 fyzicky doruceny Flour
        // - misto pro cele 2 Bread
        //
        // Flour se odebere pri startu.
        // Bread vznikne az po ticku 378.
        // =================================================

        for (
            int bakeryIndex = 0;
            bakeryIndex < buildingCount;
            ++bakeryIndex
        ) {
            Building& bakery =
                buildings[bakeryIndex];

            if (
                !bakery.active ||
                bakery.type != BuildingType::Bakery ||
                !isBuildingComplete(bakery)
            ) {
                bakeryProductionTimers[
                    bakeryIndex
                ] = 0.0f;

                continue;
            }

            // =============================================
            // BAKERY REQUIRES PHYSICAL BAKER
            //
            // Bakery smi vyrabet pouze pokud je uvnitr
            // skutecny Baker:
            //
            // type           = Baker
            // state          = Working
            // visible        = false
            // targetBuilding = tato Bakery
            //
            // Baker na ceste ani u doorway jeste neplati.
            // =============================================

            // =============================================
            // BAKER INSIDE IDLE WORKING LIFECYCLE
            //
            // Hidden + targetBuilding identifikuje Bakera
            // fyzicky uvnitr Bakery.
            //
            // Idle    = ceka na Flour nebo volny vystup.
            // Working = Bakery ma aktivni vyrobni cyklus.
            // =============================================

            int insideBakerIndex = -1;

            for (
                int unitIndex = 0;
                unitIndex < unitCount;
                ++unitIndex
            ) {
                const Unit& candidate =
                    units[unitIndex];

                const bool validInsideState =
                    candidate.state ==
                        UnitState::Idle ||
                    candidate.state ==
                        UnitState::Working;

                if (
                    candidate.active &&
                    candidate.type ==
                        UnitType::Baker &&
                    validInsideState &&
                    !candidate.visible &&
                    candidate.targetBuilding ==
                        bakeryIndex
                ) {
                    insideBakerIndex = unitIndex;
                    break;
                }
            }

            if (insideBakerIndex < 0) {
                // Bez Bakera nezahajujeme ani neposouvame
                // vyrobni cyklus.
                //
                // Jiz rozpracovany cyklus zustava pozastaveny,
                // aby se Flour ani progress neztratily.
                bakeryProductionTimers[
                    bakeryIndex
                ] = 0.0f;

                continue;
            }

            Unit& insideBaker =
                units[insideBakerIndex];

            // Existujici aktivni cyklus znamena skutecnou
            // praci. Jinak Baker uvnitr ceka v Idle.
            insideBaker.state =
                bakery.bakeryProductionActive
                    ? UnitState::Working
                    : UnitState::Idle;

            insideBaker.visible = false;

            // Neaktivni Bakery muze zahajit novy cyklus
            // pouze pokud ma Bakera, vstup a vystupni
            // kapacitu pro cele 2 Bread.
            if (!bakery.bakeryProductionActive) {
                bakery.bakeryProductionTick = 0;

                const bool hasFlour =
                    bakery.resourceIn[0] >=
                        BAKERY_FLOUR_PER_CYCLE;

                const bool hasBreadCapacity =
                    bakery.resourceOut[0] >= 0 &&
                    bakery.resourceOut[0] <=
                        BAKERY_OUTPUT_CAPACITY -
                        BAKERY_BREAD_PER_CYCLE;

                if (
                    !hasFlour ||
                    !hasBreadCapacity
                ) {
                    bakeryProductionTimers[
                        bakeryIndex
                    ] = 0.0f;

                    continue;
                }

                // Remake odebere vstup pred prvni
                // House work action.
                bakery.resourceIn[0] -=
                    BAKERY_FLOUR_PER_CYCLE;

                bakery.bakeryProductionActive =
                    true;

                // Baker zacina skutecne pracovat az spolu
                // se zahajenim House production cyklu.
                insideBaker.state =
                    UnitState::Working;

                insideBaker.animationFrame = 0;
                insideBaker.animationTimer = 0.0f;

                bakery.bakeryProductionTick = 0;

                bakeryProductionTimers[
                    bakeryIndex
                ] = 0.0f;
            }

            bakeryProductionTimers[
                bakeryIndex
            ] += deltaSeconds;

            while (
                bakery.bakeryProductionActive &&
                bakeryProductionTimers[
                    bakeryIndex
                ] >= BAKERY_TICK_SECONDS
            ) {
                bakeryProductionTimers[
                    bakeryIndex
                ] -= BAKERY_TICK_SECONDS;

                ++bakery.bakeryProductionTick;

                if (
                    bakery.bakeryProductionTick <
                        BAKERY_PRODUCTION_TICKS
                ) {
                    continue;
                }

                // Remake prida produkt az po posledni
                // House work action.
                bakery.resourceOut[0] +=
                    BAKERY_BREAD_PER_CYCLE;

                bakery.bakeryProductionActive =
                    false;

                // Dokonceny cyklus vraci Bakera do Idle
                // uvnitr Bakery. Dalsi cyklus ho znovu
                // prepne na Working.
                insideBaker.state =
                    UnitState::Idle;

                insideBaker.animationFrame = 0;
                insideBaker.animationTimer = 0.0f;

                bakery.bakeryProductionTick = 0;

                bakeryProductionTimers[
                    bakeryIndex
                ] = 0.0f;
            }
        }

        // =================================================
        // ORIGINAL KaM CORN GROWTH
        //
        // Remake thresholds:
        //
        // CORN_AGE_1    = 1400 ticks = 140 seconds
        // CORN_AGE_2    = 2200 ticks = 220 seconds
        // CORN_AGE_3    = 4400 ticks = 440 seconds
        // CORN_AGE_FULL = 6400 ticks = 640 seconds
        //
        // Vita checkpoint uses 10x speed for practical
        // visual verification. Set to 1.0f for authentic
        // real-time growth after the test checkpoint.
        // =================================================

        constexpr float CORN_GROWTH_TEST_SPEED =
            10.0f;

        constexpr float CORN_YOUNG_SECONDS =
            140.0f /
            CORN_GROWTH_TEST_SPEED;

        constexpr float CORN_GROWING_SECONDS =
            220.0f /
            CORN_GROWTH_TEST_SPEED;

        constexpr float CORN_GREEN_SECONDS =
            440.0f /
            CORN_GROWTH_TEST_SPEED;

        constexpr float CORN_RIPE_SECONDS =
            640.0f /
            CORN_GROWTH_TEST_SPEED;

        for (
            int fieldY = 0;
            fieldY < MAP_HEIGHT;
            ++fieldY
        ) {
            for (
                int fieldX = 0;
                fieldX < MAP_WIDTH;
                ++fieldX
            ) {
                LandPlanState& field =
                    landPlans[
                        fieldY
                    ][
                        fieldX
                    ];

                float& growthTimer =
                    cornGrowthTimers[
                        fieldY
                    ][
                        fieldX
                    ];

                // =================================================
                // MONOTONIC CORN GROWTH
                //
                // A field can only advance:
                //
                // FieldSown
                // -> FieldYoung
                // -> FieldGrowing
                // -> FieldGreen
                // -> FieldRipe
                //
                // FieldRipe is terminal until future harvest.
                // =================================================

                if (
                    field ==
                        LandPlanState::FieldRipe
                ) {
                    growthTimer = 0.0f;
                    continue;
                }

                const bool growingCorn =
                    field ==
                        LandPlanState::FieldSown ||
                    field ==
                        LandPlanState::FieldYoung ||
                    field ==
                        LandPlanState::FieldGrowing ||
                    field ==
                        LandPlanState::FieldGreen;

                if (!growingCorn) {
                    growthTimer = 0.0f;
                    continue;
                }

                growthTimer += deltaSeconds;

                if (
                    field ==
                        LandPlanState::FieldSown &&
                    growthTimer >=
                        CORN_YOUNG_SECONDS
                ) {
                    field =
                        LandPlanState::FieldYoung;
                }

                if (
                    field ==
                        LandPlanState::FieldYoung &&
                    growthTimer >=
                        CORN_GROWING_SECONDS
                ) {
                    field =
                        LandPlanState::FieldGrowing;
                }

                if (
                    field ==
                        LandPlanState::FieldGrowing &&
                    growthTimer >=
                        CORN_GREEN_SECONDS
                ) {
                    // Originalni KaM:
                    // Terrain 60 + Obj 58.
                    field =
                        LandPlanState::FieldGreen;
                }

                if (
                    field ==
                        LandPlanState::FieldGreen &&
                    growthTimer >=
                        CORN_RIPE_SECONDS
                ) {
                    // Originalni KaM:
                    // Terrain 60 + Obj 59.
                    //
                    // Terminal until Farmer harvest exists.
                    field =
                        LandPlanState::FieldRipe;

                    growthTimer = 0.0f;
                }
            }
        }


        // Aktualizuj pozice jednotek pred vykreslenim.
        updateUnits(
            units,
            unitCount,
            deltaSeconds,
            TILE_SIZE,
            &roads[0][0],
            &landPlans[0][0],
            buildings,
            buildingCount,
            MAP_WIDTH,
            MAP_HEIGHT
        );

        // Spolecny seznam budov a jednotek.
        // Mensi worldY se vykresli drive, vetsi worldY pozdeji.
        struct WorldRenderEntry {
            float sortY;
            int kind;
            int index;
        };

        std::vector<WorldRenderEntry> worldRenderEntries;

        worldRenderEntries.reserve(
            buildingCount + unitCount
        );

        for (int i = 0; i < buildingCount; ++i) {
            if (!buildings[i].active)
                continue;

            WorldRenderEntry entry;

            // Budova se radi podle spodni hrany
            // sveho logickeho footprintu.
            entry.sortY =
                (buildings[i].tileY +
                 getBuildingHeight(
                     buildings[i].type
                 )) *
                TILE_SIZE;

            entry.kind = 0;
            entry.index = i;

            worldRenderEntries.push_back(entry);
        }

        for (int i = 0; i < unitCount; ++i) {
            if (!units[i].active)
                continue;

            WorldRenderEntry entry;

            // Jednotka se radi podle polohy nohou.
            entry.sortY = units[i].worldY;
            entry.kind = 1;
            entry.index = i;

            worldRenderEntries.push_back(entry);
        }

        std::sort(
            worldRenderEntries.begin(),
            worldRenderEntries.end(),
            [](
                const WorldRenderEntry& a,
                const WorldRenderEntry& b
            ) {
                if (a.sortY < b.sortY)

                    return true;

                if (a.sortY > b.sortY)
                    return false;

                // Pri stejnem Y vykresli budovu prvni
                // a jednotku pres ni.
                return a.kind < b.kind;
            }
        );

        for (
            const WorldRenderEntry& entry :
            worldRenderEntries
        ) {
            if (entry.kind == 0) {
                bool bakeryHasInsideBaker = false;
                bool farmHasInsideFarmer = false;

                const Building& renderBuilding =
                    buildings[entry.index];

                if (
                    renderBuilding.type ==
                        BuildingType::Bakery &&
                    renderBuilding.active &&
                    isBuildingComplete(renderBuilding)
                ) {
                    for (
                        int bakerIndex = 0;
                        bakerIndex < unitCount;
                        ++bakerIndex
                    ) {
                        const Unit& baker =
                            units[bakerIndex];

                        if (
                            baker.active &&
                            baker.type ==
                                UnitType::Baker &&
                            !baker.visible &&
                            baker.targetBuilding ==
                                entry.index &&
                            (
                                baker.state ==
                                    UnitState::Idle ||
                                baker.state ==
                                    UnitState::Working
                            )
                        ) {
                            bakeryHasInsideBaker = true;
                            break;
                        }
                    }
                }

                if (
                    renderBuilding.type ==
                        BuildingType::Farm &&
                    renderBuilding.active &&
                    isBuildingComplete(
                        renderBuilding
                    )
                ) {
                    for (
                        int farmerIndex = 0;
                        farmerIndex < unitCount;
                        ++farmerIndex
                    ) {
                        const Unit& farmer =
                            units[farmerIndex];

                        if (
                            farmer.active &&
                            farmer.type ==
                                UnitType::Farmer &&
                            !farmer.visible &&
                            farmer.targetBuilding ==
                                entry.index &&
                            (
                                farmer.state ==
                                    UnitState::Idle ||
                                farmer.state ==
                                    UnitState::Working
                            )
                        ) {
                            farmHasInsideFarmer = true;
                            break;
                        }
                    }
                }

                renderBuildings(
                    renderer,
                    buildingResources,
                    &buildings[entry.index],
                    1,
                    cameraX,
                    cameraY,
                    TILE_SIZE,
                    &kamPlanResources,
                    bakeryHasInsideBaker,
                    farmHasInsideFarmer,
                    units,
                    unitCount,
                    entry.index
                );
            } else {
                renderUnits(
                    renderer,
                    unitResources,
                    &units[entry.index],
                    1,
                    cameraX,
                    cameraY
                );
            }
        }

        renderSelectedUnitMarker(
            renderer,
            units,
            unitCount,
            unitUI.selectedUnitIndex,
            cameraX,
            cameraY
        );

        if (activeBuildTool == BuildTool::Building) {
            renderBuildingPreview(
                renderer,
                buildingResources,
                buildings,
                buildingCount,
                selectedBuildingType,
                selectedTileX,
                selectedTileY,
                cameraX,
                cameraY,
                TILE_SIZE,
                MAP_WIDTH,
                MAP_HEIGHT
            );
        }

        if (selectedTileX >= 0 && selectedTileX < MAP_WIDTH &&
            selectedTileY >= 0 && selectedTileY < MAP_HEIGHT) {

            SDL_Rect selectedTile = {
                selectedTileX * TILE_SIZE - (int)cameraX,
                selectedTileY * TILE_SIZE - (int)cameraY,
                TILE_SIZE,
                TILE_SIZE
            };

            SDL_SetRenderDrawColor(renderer, 255, 135, 35, 255);
            SDL_RenderDrawRect(renderer, &selectedTile);
        }

        if (hoverTileX >= 0 && hoverTileX < MAP_WIDTH &&
            hoverTileY >= 0 && hoverTileY < MAP_HEIGHT) {

            SDL_Rect hoverTile = {
                hoverTileX * TILE_SIZE - (int)cameraX,
                hoverTileY * TILE_SIZE - (int)cameraY,
                TILE_SIZE,
                TILE_SIZE
            };

            SDL_SetRenderDrawColor(renderer, 255, 235, 90, 255);
            SDL_RenderDrawRect(renderer, &hoverTile);
        }

        if (buildMenuOpen) {
            const int panelWidth = 150;
            const int panelPadding = 10;
            const int slotSize = 64;
            const int slotGap = 12;
            const int firstSlotY = 40;

            SDL_Rect buildPanel = {
                0,
                0,
                panelWidth,
                SCREEN_HEIGHT
            };

            SDL_SetRenderDrawColor(renderer, 46, 35, 23, 255);
            SDL_RenderFillRect(renderer, &buildPanel);

            SDL_SetRenderDrawColor(renderer, 185, 151, 89, 255);
            SDL_RenderDrawRect(renderer, &buildPanel);

            for (int i = 0; i < 4; ++i) {
                SDL_Rect slot = {
                    panelPadding,
                    firstSlotY + i * (slotSize + slotGap),
                    slotSize,
                    slotSize
                };

                bool hovered =
                    cursorX >= slot.x &&
                    cursorX < slot.x + slot.w &&
                    cursorY >= slot.y &&
                    cursorY < slot.y + slot.h;

                bool selected = buildMenuSelection == i;
                hovered = hovered || selected;

                SDL_SetRenderDrawColor(
                    renderer,
                    hovered ? 132 : 82,
                    hovered ? 102 : 62,
                    hovered ? 59 : 39,
                    255
                );
                SDL_RenderFillRect(renderer, &slot);

                SDL_SetRenderDrawColor(
                    renderer,
                    hovered ? 255 : 191,
                    hovered ? 218 : 160,
                    hovered ? 128 : 101,
                    255
                );
                SDL_RenderDrawRect(renderer, &slot);

                SDL_Texture* icon = nullptr;

                if (i == 0)
                    icon = buildingResources.roadIcon;
                else if (i == 1)
                    icon = buildingResources.storeIcon;
                else if (i == 2)
                    icon = buildingResources.bakeryIcon;
                else if (i == 3)
                    icon = buildingResources.farmIcon;
                else
                    icon = buildingResources.schoolIcon;

                if (icon) {
                    SDL_Rect iconDestination = {
                        slot.x + 4,
                        slot.y + 4,
                        slot.w - 8,
                        slot.h - 8
                    };

                    SDL_RenderCopy(
                        renderer,
                        icon,
                        nullptr,
                        &iconDestination
                    );
                }
            }
        }

        renderGameUI(
            renderer,
            buildingResources,
            schoolUIResources,
            houseIconResources,
            gameUI,
            buildings,
            buildingCount,
            units,
            unitCount,
            wareDistribution,
            cursorX,
            cursorY
        );

        // =================================================
        // ORIGINAL KAM REMAKE GAME SETTINGS POPUP
        //
        // Background: GUIMain:15
        // Close:      GUIMain:9 / bsMenu
        //
        // No replacement fills or temporary layout panels.
        // =================================================

        if (
            settingsPopupOpen &&
            settingsPopupTexture
        ) {
            constexpr int popupX = 320;
            constexpr int popupY = 32;
            constexpr int popupWidth = 512;
            constexpr int popupHeight = 480;

            SDL_Rect popupDestination = {
                popupX,
                popupY,
                popupWidth,
                popupHeight
            };

            // Remake ImageStretch:
            // stretch the complete GUIMain:15 sprite.
            SDL_RenderCopy(
                renderer,
                settingsPopupTexture,
                nullptr,
                &popupDestination
            );

            // Original reference title position after 0.8
            // conversion from the full-size Settings popup.
            SDL_Rect titleArea = {
                popupX,
                popupY + 17,
                popupWidth,
                24
            };

            renderRuntimeMetalTextCentered(
                renderer,
                runtimeMetalFont,
                "Game Settings",
                titleArea
            );

            // =============================================
            // SETTINGS 18: GRAPHICS / CONTROLS
            //
            // Exact upper two-column composition converted
            // uniformly from the supplied Remake reference.
            // Visual-only checkpoint.
            // =============================================

            constexpr int leftGroupX =
                popupX + 24;

            constexpr int rightGroupX =
                popupX + 251;

            constexpr int upperHeadingY =
                popupY + 80;

            constexpr int upperGroupY =
                popupY + 100;

            constexpr int upperGroupWidth =
                224;

            constexpr int upperGroupHeight =
                94;


            // ---------------------------------------------
            // Original Remake WriteBevel equivalent.
            // ---------------------------------------------

            auto drawSettingsBevel =
                [
                    renderer
                ](
                    const SDL_Rect& area,
                    Uint8 backAlpha
                )
            {
                SDL_SetRenderDrawBlendMode(
                    renderer,
                    SDL_BLENDMODE_BLEND
                );

                SDL_Rect background = {
                    area.x + 1,
                    area.y + 1,
                    area.w - 2,
                    area.h - 2
                };

                // WriteBevel background:
                // glColor4f(0,0,0,aBackAlpha).
                SDL_SetRenderDrawColor(
                    renderer,
                    0,
                    0,
                    0,
                    backAlpha
                );

                SDL_RenderFillRect(
                    renderer,
                    &background
                );

                // Dark top and left edge.
                SDL_SetRenderDrawColor(
                    renderer,
                    0,
                    0,
                    0,
                    255
                );

                SDL_RenderDrawLine(
                    renderer,
                    area.x,
                    area.y,
                    area.x + area.w - 1,
                    area.y
                );

                SDL_RenderDrawLine(
                    renderer,
                    area.x,
                    area.y,
                    area.x,
                    area.y + area.h - 1
                );

                // Bright right and bottom edge.
                SDL_SetRenderDrawColor(
                    renderer,
                    190,
                    190,
                    190,
                    190
                );

                SDL_RenderDrawLine(
                    renderer,
                    area.x + area.w - 1,
                    area.y,
                    area.x + area.w - 1,
                    area.y + area.h - 1
                );

                SDL_RenderDrawLine(
                    renderer,
                    area.x,
                    area.y + area.h - 1,
                    area.x + area.w - 1,
                    area.y + area.h - 1
                );

                SDL_SetRenderDrawBlendMode(
                    renderer,
                    SDL_BLENDMODE_NONE
                );
            };


            // ---------------------------------------------
            // Original checkbox visual.
            // ---------------------------------------------

            auto drawSettingsCheckbox =
                [
                    renderer,
                    &runtimeMetalFont
                ](
                    int x,
                    int y,
                    const char* caption,
                    bool checked
                )
            {
                SDL_Rect box = {
                    x,
                    y,
                    12,
                    12
                };

                SDL_SetRenderDrawBlendMode(
                    renderer,
                    SDL_BLENDMODE_BLEND
                );

                SDL_SetRenderDrawColor(
                    renderer,
                    0,
                    0,
                    0,
                    165
                );

                SDL_RenderFillRect(
                    renderer,
                    &box
                );

                SDL_SetRenderDrawColor(
                    renderer,
                    30,
                    24,
                    17,
                    255
                );

                SDL_RenderDrawLine(
                    renderer,
                    box.x,
                    box.y,
                    box.x + box.w - 1,
                    box.y
                );

                SDL_RenderDrawLine(
                    renderer,
                    box.x,
                    box.y,
                    box.x,
                    box.y + box.h - 1
                );

                SDL_SetRenderDrawColor(
                    renderer,
                    195,
                    178,
                    132,
                    255
                );

                SDL_RenderDrawLine(
                    renderer,
                    box.x + box.w - 1,
                    box.y,
                    box.x + box.w - 1,
                    box.y + box.h - 1
                );

                SDL_RenderDrawLine(
                    renderer,
                    box.x,
                    box.y + box.h - 1,
                    box.x + box.w - 1,
                    box.y + box.h - 1
                );

                if (checked) {
                    SDL_SetRenderDrawColor(
                        renderer,
                        245,
                        238,
                        213,
                        255
                    );

                    SDL_RenderDrawLine(
                        renderer,
                        box.x + 2,
                        box.y + 5,
                        box.x + 5,
                        box.y + 9
                    );

                    SDL_RenderDrawLine(
                        renderer,
                        box.x + 5,
                        box.y + 9,
                        box.x + 10,
                        box.y + 2
                    );

                    SDL_RenderDrawLine(
                        renderer,
                        box.x + 3,
                        box.y + 5,
                        box.x + 6,
                        box.y + 8
                    );
                }

                SDL_SetRenderDrawBlendMode(
                    renderer,
                    SDL_BLENDMODE_NONE
                );

                renderRuntimeMetalText(
                    renderer,
                    runtimeMetalFont,
                    caption,
                    x + 17,
                    y - 1
                );
            };


            // ---------------------------------------------
            // Original-style trackbar.
            // ---------------------------------------------

            auto drawSettingsTrackbar =
                [
                    renderer,
                    &runtimeMetalFont,
                    &drawSettingsBevel,
                    &schoolUIResources
                ](
                    int x,
                    int y,
                    int width,
                    const char* caption,
                    int value,
                    int maximum
                )
            {
                renderRuntimeMetalText(
                    renderer,
                    runtimeMetalFont,
                    caption,
                    x,
                    y
                );

                SDL_Rect track = {
                    x,
                    y + 17,
                    width,
                    14
                };

                drawSettingsBevel(
                    track,
                    128
                );

                constexpr int thumbWidth = 27;
                constexpr int thumbHeight = 16;

                const int travel =
                    track.w -
                    thumbWidth -
                    2;

                const int thumbOffset =
                    maximum > 0
                        ? (
                            travel *
                            value
                        ) /
                            maximum
                        : 0;

                SDL_Rect thumb = {
                    track.x + 1 + thumbOffset,
                    track.y - 1,
                    thumbWidth,
                    thumbHeight
                };

                // Original KaM trackbar thumb.
                if (
                    schoolUIResources.ratiosTrackbarThumb
                ) {
                    SDL_RenderCopy(
                        renderer,
                        schoolUIResources.ratiosTrackbarThumb,
                        nullptr,
                        &thumb
                    );
                }
            };


            // ---------------------------------------------
            // Original KaM fnt_Outline section headings.
            // ---------------------------------------------

            auto drawSettingsHeading =
                [renderer](
                    SDL_Texture* texture,
                    int x,
                    int y
                )
            {
                if (!texture)
                    return;

                int width = 0;
                int height = 0;

                SDL_QueryTexture(
                    texture,
                    nullptr,
                    nullptr,
                    &width,
                    &height
                );

                SDL_Rect destination = {
                    x,
                    y,
                    width,
                    height
                };

                SDL_RenderCopy(
                    renderer,
                    texture,
                    nullptr,
                    &destination
                );
            };

            drawSettingsHeading(
                settingsGraphicsText,
                leftGroupX + 5,
                upperHeadingY
            );

            drawSettingsHeading(
                settingsControlsText,
                rightGroupX + 5,
                upperHeadingY
            );


            // ---------------------------------------------
            // Bevel groups.
            // ---------------------------------------------

            SDL_Rect graphicsGroup = {
                leftGroupX,
                upperGroupY,
                upperGroupWidth,
                upperGroupHeight
            };

            SDL_Rect controlsGroup = {
                rightGroupX,
                upperGroupY,
                upperGroupWidth,
                upperGroupHeight
            };

            drawSettingsBevel(
                graphicsGroup,
                112
            );

            drawSettingsBevel(
                controlsGroup,
                112
            );


            // ---------------------------------------------
            // GRAPHICS
            // ---------------------------------------------

            drawSettingsCheckbox(
                graphicsGroup.x + 8,
                graphicsGroup.y + 8,
                "HD movements",
                true
            );

            drawSettingsCheckbox(
                graphicsGroup.x + 8,
                graphicsGroup.y + 25,
                "HD animations",
                true
            );

            drawSettingsTrackbar(
                graphicsGroup.x + 8,
                graphicsGroup.y + 44,
                graphicsGroup.w - 16,
                "Brightness",
                2,
                20
            );


            // ---------------------------------------------
            // CONTROLS
            // ---------------------------------------------

            drawSettingsTrackbar(
                controlsGroup.x + 8,
                controlsGroup.y + 8,
                controlsGroup.w - 16,
                "Scroll speed",
                2,
                20
            );

            SDL_Rect keyBindingsButton = {
                controlsGroup.x + 8,
                controlsGroup.y + 52,
                controlsGroup.w - 16,
                24
            };

            if (settingsMenuButtonTexture) {
                int textureWidth = 0;
                int textureHeight = 0;

                SDL_QueryTexture(
                    settingsMenuButtonTexture,
                    nullptr,
                    nullptr,
                    &textureWidth,
                    &textureHeight
                );

                if (
                    textureWidth > 0 &&
                    textureHeight > 0
                ) {
                    // Write3DButton bsMenu samples the
                    // GUIMain:9 texture from absolute control
                    // coordinates at half scale.
                    const int sourceX =
                        (
                            keyBindingsButton.x /
                            2
                        ) %
                        textureWidth;

                    const int sourceY =
                        (
                            keyBindingsButton.y /
                            2
                        ) %
                        textureHeight;

                    int sourceWidth =
                        keyBindingsButton.w /
                        2;

                    int sourceHeight =
                        keyBindingsButton.h /
                        2;

                    if (
                        sourceX + sourceWidth >
                        textureWidth
                    ) {
                        sourceWidth =
                            textureWidth -
                            sourceX;
                    }

                    if (
                        sourceY + sourceHeight >
                        textureHeight
                    ) {
                        sourceHeight =
                            textureHeight -
                            sourceY;
                    }

                    if (sourceWidth < 1)
                        sourceWidth = 1;

                    if (sourceHeight < 1)
                        sourceHeight = 1;

                    SDL_Rect source = {
                        sourceX,
                        sourceY,
                        sourceWidth,
                        sourceHeight
                    };

                    SDL_RenderCopy(
                        renderer,
                        settingsMenuButtonTexture,
                        &source,
                        &keyBindingsButton
                    );
                }
            }

            // Key-bindings bsMenu bevel.
            constexpr int keyChamfer = 2;

            SDL_SetRenderDrawBlendMode(
                renderer,
                SDL_BLENDMODE_BLEND
            );

            SDL_SetRenderDrawColor(
                renderer,
                255,
                255,
                255,
                179
            );

            SDL_Rect keyTop = {
                keyBindingsButton.x,
                keyBindingsButton.y,
                keyBindingsButton.w,
                keyChamfer
            };

            SDL_RenderFillRect(
                renderer,
                &keyTop
            );

            SDL_SetRenderDrawColor(
                renderer,
                255,
                255,
                255,
                153
            );

            SDL_Rect keyLeft = {
                keyBindingsButton.x,
                keyBindingsButton.y,
                keyChamfer,
                keyBindingsButton.h
            };

            SDL_RenderFillRect(
                renderer,
                &keyLeft
            );

            SDL_SetRenderDrawColor(
                renderer,
                0,
                0,
                0,
                128
            );

            SDL_Rect keyRight = {
                keyBindingsButton.x +
                    keyBindingsButton.w -
                    keyChamfer,
                keyBindingsButton.y,
                keyChamfer,
                keyBindingsButton.h
            };

            SDL_RenderFillRect(
                renderer,
                &keyRight
            );

            SDL_SetRenderDrawColor(
                renderer,
                0,
                0,
                0,
                102
            );

            SDL_Rect keyBottom = {
                keyBindingsButton.x,
                keyBindingsButton.y +
                    keyBindingsButton.h -
                    keyChamfer,
                keyBindingsButton.w,
                keyChamfer
            };

            SDL_RenderFillRect(
                renderer,
                &keyBottom
            );

            SDL_SetRenderDrawBlendMode(
                renderer,
                SDL_BLENDMODE_NONE
            );

            renderRuntimeMetalTextCentered(
                renderer,
                runtimeMetalFont,
                "Key-bindings",
                keyBindingsButton
            );


            // =============================================
            // SETTINGS 23: SOUND / GAMEPLAY
            //
            // Visual-only Remake layout.
            // Upper SETTINGS 19L geometry remains locked.
            // =============================================

            constexpr int middleHeadingY =
                popupY + 207;

            constexpr int middleGroupY =
                popupY + 227;

            constexpr int soundGroupHeight =
                130;

            constexpr int gameplayGroupHeight =
                194;


            // ---------------------------------------------
            // Original Outline headings.
            // ---------------------------------------------

            drawSettingsHeading(
                settingsSoundText,
                leftGroupX + 5,
                middleHeadingY
            );

            drawSettingsHeading(
                settingsGameplayText,
                rightGroupX + 5,
                middleHeadingY
            );


            // ---------------------------------------------
            // Original WriteBevel group backgrounds.
            // ---------------------------------------------

            SDL_Rect soundGroup = {
                leftGroupX,
                middleGroupY,
                upperGroupWidth,
                soundGroupHeight
            };

            SDL_Rect gameplayGroup = {
                rightGroupX,
                middleGroupY,
                upperGroupWidth,
                gameplayGroupHeight
            };

            drawSettingsBevel(
                soundGroup,
                112
            );

            drawSettingsBevel(
                gameplayGroup,
                112
            );


            // =============================================
            // SOUND
            // =============================================

            drawSettingsTrackbar(
                soundGroup.x + 8,
                soundGroup.y + 8,
                soundGroup.w - 16,
                "SFX",
                5,
                20
            );

            drawSettingsTrackbar(
                soundGroup.x + 8,
                soundGroup.y + 48,
                soundGroup.w - 16,
                "Music",
                5,
                20
            );

            drawSettingsCheckbox(
                soundGroup.x + 8,
                soundGroup.y + 91,
                "Disable music",
                false
            );

            drawSettingsCheckbox(
                soundGroup.x + 8,
                soundGroup.y + 109,
                "Shuffle tracks",
                false
            );


            // =============================================
            // GAMEPLAY
            // =============================================

            drawSettingsCheckbox(
                gameplayGroup.x + 8,
                gameplayGroup.y + 8,
                "Autosave",
                true
            );

            drawSettingsCheckbox(
                gameplayGroup.x + 8,
                gameplayGroup.y + 27,
                "Autosave at game end",
                false
            );

            drawSettingsCheckbox(
                gameplayGroup.x + 8,
                gameplayGroup.y + 46,
                "Make replay savepoints",
                true
            );

            drawSettingsCheckbox(
                gameplayGroup.x + 8,
                gameplayGroup.y + 65,
                "Save ware distribution",
                false
            );

            renderRuntimeMetalText(
                renderer,
                runtimeMetalFont,
                "between games",
                gameplayGroup.x + 25,
                gameplayGroup.y + 81
            );

            renderRuntimeMetalText(
                renderer,
                runtimeMetalFont,
                "Players colors:",
                gameplayGroup.x + 8,
                gameplayGroup.y + 106
            );

            drawSettingsCheckbox(
                gameplayGroup.x + 8,
                gameplayGroup.y + 126,
                "Default",
                true
            );

            drawSettingsCheckbox(
                gameplayGroup.x + 8,
                gameplayGroup.y + 145,
                "Ally / Enemy",
                false
            );

            drawSettingsCheckbox(
                gameplayGroup.x + 8,
                gameplayGroup.y + 164,
                "Teams",
                false
            );


            // Original Close geometry converted by 0.8.
            SDL_Rect closeButton = {
                popupX + 16,
                popupY + 436,
                224,
                24
            };

            if (settingsMenuButtonTexture) {
                int textureWidth = 0;
                int textureHeight = 0;

                SDL_QueryTexture(
                    settingsMenuButtonTexture,
                    nullptr,
                    nullptr,
                    &textureWidth,
                    &textureHeight
                );

                if (
                    textureWidth > 0 &&
                    textureHeight > 0
                ) {
                    // Exact Write3DButton bsMenu source
                    // calculation from KM_RenderUI.pas.
                    //
                    // GUIMain:9 is sampled according to the
                    // absolute control position, not stretched
                    // from the entire 300x400 texture.
                    const int down = 0;

                    const float centerBlockX =
                        static_cast<float>(
                            (
                                closeButton.x +
                                closeButton.w / 2
                            ) /
                            textureWidth
                        ) /
                        2.0f;

                    const float centerBlockY =
                        static_cast<float>(
                            (
                                closeButton.y +
                                closeButton.h / 2
                            ) /
                            textureHeight
                        ) /
                        2.0f;

                    float u1 =
                        static_cast<float>(
                            closeButton.x - down
                        ) /
                        2.0f /
                        textureWidth -
                        centerBlockX;

                    float u2 =
                        static_cast<float>(
                            closeButton.x +
                            closeButton.w -
                            down
                        ) /
                        2.0f /
                        textureWidth -
                        centerBlockX;

                    float v1 =
                        static_cast<float>(
                            closeButton.y - down
                        ) /
                        2.0f /
                        textureHeight -
                        centerBlockY;

                    float v2 =
                        static_cast<float>(
                            closeButton.y +
                            closeButton.h -
                            down
                        ) /
                        2.0f /
                        textureHeight -
                        centerBlockY;

                    if (u1 < 0.0f)
                        u1 = 0.0f;

                    if (u2 > 1.0f)
                        u2 = 1.0f;

                    if (v1 < 0.0f)
                        v1 = 0.0f;

                    if (v2 > 1.0f)
                        v2 = 1.0f;

                    SDL_Rect buttonSource = {
                        static_cast<int>(
                            u1 * textureWidth
                        ),
                        static_cast<int>(
                            v1 * textureHeight
                        ),
                        static_cast<int>(
                            (u2 - u1) *
                            textureWidth
                        ),
                        static_cast<int>(
                            (v2 - v1) *
                            textureHeight
                        )
                    };

                    if (buttonSource.w < 1)
                        buttonSource.w = 1;

                    if (buttonSource.h < 1)
                        buttonSource.h = 1;

                    SDL_RenderCopy(
                        renderer,
                        settingsMenuButtonTexture,
                        &buttonSource,
                        &closeButton
                    );
                }
            }

            // Original Write3DButton bevel.
            //
            // Scaled 24px-high Close uses a 2px chamfer.
            constexpr int chamfer = 2;

            SDL_SetRenderDrawBlendMode(
                renderer,
                SDL_BLENDMODE_BLEND
            );

            // Top edge: white, alpha 0.7.
            SDL_SetRenderDrawColor(
                renderer,
                255,
                255,
                255,
                179
            );

            SDL_Rect topEdge = {
                closeButton.x,
                closeButton.y,
                closeButton.w,
                chamfer
            };

            SDL_RenderFillRect(
                renderer,
                &topEdge
            );

            // Left edge: white, alpha 0.6.
            SDL_SetRenderDrawColor(
                renderer,
                255,
                255,
                255,
                153
            );

            SDL_Rect leftEdge = {
                closeButton.x,
                closeButton.y,
                chamfer,
                closeButton.h
            };

            SDL_RenderFillRect(
                renderer,
                &leftEdge
            );

            // Right edge: black, alpha 0.5.
            SDL_SetRenderDrawColor(
                renderer,
                0,
                0,
                0,
                128
            );

            SDL_Rect rightEdge = {
                closeButton.x +
                    closeButton.w -
                    chamfer,
                closeButton.y,
                chamfer,
                closeButton.h
            };

            SDL_RenderFillRect(
                renderer,
                &rightEdge
            );

            // Bottom edge: black, alpha 0.4.
            SDL_SetRenderDrawColor(
                renderer,
                0,
                0,
                0,
                102
            );

            SDL_Rect bottomEdge = {
                closeButton.x,
                closeButton.y +
                    closeButton.h -
                    chamfer,
                closeButton.w,
                chamfer
            };

            SDL_RenderFillRect(
                renderer,
                &bottomEdge
            );

            SDL_SetRenderDrawBlendMode(
                renderer,
                SDL_BLENDMODE_NONE
            );

            // TKMButton caption:
            // fnt_Metal, taCenter, icWhite.
            renderRuntimeMetalTextCentered(
                renderer,
                runtimeMetalFont,
                "Close",
                closeButton
            );
        }


        // =================================================
        // REMAKE-STYLE SAVE / LOAD PAGE RENDERER
        // =================================================

        if (
            gameUI.section ==
                GameUISection::OptionsLoad ||
            gameUI.section ==
                GameUISection::OptionsSave
        ) {
            const bool savePage =
                gameUI.section ==
                    GameUISection::OptionsSave;

            constexpr int panelX = 9;
            constexpr int panelY = 234;
            constexpr int panelW = 180;

            SDL_Rect listArea = {
                panelX,
                panelY + (savePage ? 4 : 2),
                panelW,
                savePage ? 220 : 260
            };

            SDL_SetRenderDrawBlendMode(
                renderer,
                SDL_BLENDMODE_BLEND
            );

            SDL_SetRenderDrawColor(
                renderer,
                30,
                24,
                18,
                150
            );

            SDL_RenderFillRect(
                renderer,
                &listArea
            );

            SDL_SetRenderDrawColor(
                renderer,
                15,
                12,
                9,
                255
            );

            SDL_RenderDrawRect(
                renderer,
                &listArea
            );

            constexpr int rowHeight = 24;
            constexpr int visibleRows = 8;

            for (
                int visibleIndex = 0;
                visibleIndex < visibleRows;
                ++visibleIndex
            ) {
                const int saveIndex =
                    gameUI.saveLoadTop +
                    visibleIndex;

                if (
                    saveIndex >=
                        optionsSaveFiles.count
                ) {
                    break;
                }

                SDL_Rect row = {
                    listArea.x + 3,
                    listArea.y +
                        3 +
                        visibleIndex * rowHeight,
                    listArea.w -
                        (
                            optionsSaveFiles.count >
                                visibleRows
                                ? 15
                                : 6
                        ),
                    rowHeight - 2
                };

                const bool selected =
                    gameUI.saveLoadFocus == 0 &&
                    saveIndex ==
                        gameUI.saveLoadSelection;

                if (selected) {
                    SDL_SetRenderDrawColor(
                        renderer,
                        210,
                        180,
                        100,
                        85
                    );

                    SDL_RenderFillRect(
                        renderer,
                        &row
                    );

                    SDL_SetRenderDrawColor(
                        renderer,
                        235,
                        207,
                        126,
                        255
                    );

                    SDL_RenderDrawRect(
                        renderer,
                        &row
                    );
                }

                const char* name =
                    optionsSaveFiles
                        .entries[saveIndex]
                        .name;

                renderRuntimeMetalText(
                    renderer,
                    runtimeMetalFont,
                    name,
                    row.x + 5,
                    row.y + 4
                );
            }

            // Visible Remake-style scrollbar for save lists.
            //
            // saveLoadTop already controls the first visible
            // row. The scrollbar only visualizes that state.
            if (optionsSaveFiles.count > visibleRows) {
                SDL_Rect scrollTrack = {
                    listArea.x + listArea.w - 8,
                    listArea.y + 4,
                    4,
                    listArea.h - 8
                };

                SDL_SetRenderDrawColor(
                    renderer,
                    24,
                    19,
                    14,
                    210
                );

                SDL_RenderFillRect(
                    renderer,
                    &scrollTrack
                );

                const int maximumTop =
                    optionsSaveFiles.count -
                        visibleRows;

                int thumbHeight =
                    (
                        scrollTrack.h *
                        visibleRows
                    ) /
                    optionsSaveFiles.count;

                if (thumbHeight < 14)
                    thumbHeight = 14;

                if (thumbHeight > scrollTrack.h)
                    thumbHeight = scrollTrack.h;

                const int thumbTravel =
                    scrollTrack.h -
                        thumbHeight;

                const int thumbOffset =
                    maximumTop > 0
                        ? (
                            gameUI.saveLoadTop *
                            thumbTravel
                        ) /
                            maximumTop
                        : 0;

                SDL_Rect scrollThumb = {
                    scrollTrack.x - 2,
                    scrollTrack.y +
                        thumbOffset,
                    8,
                    thumbHeight
                };

                SDL_SetRenderDrawColor(
                    renderer,
                    171,
                    154,
                    121,
                    255
                );

                SDL_RenderFillRect(
                    renderer,
                    &scrollThumb
                );

                SDL_SetRenderDrawColor(
                    renderer,
                    35,
                    30,
                    25,
                    255
                );

                SDL_RenderDrawRect(
                    renderer,
                    &scrollThumb
                );
            }

            if (optionsSaveFiles.count == 0) {
                SDL_Rect emptyArea = {
                    listArea.x,
                    listArea.y + 15,
                    listArea.w,
                    30
                };

                renderRuntimeMetalTextCentered(
                    renderer,
                    runtimeMetalFont,
                    "No Save Files",
                    emptyArea
                );
            }

            if (savePage) {
                SDL_Rect editArea = {
                    panelX,
                    panelY + 235,
                    panelW,
                    20
                };

                SDL_SetRenderDrawColor(
                    renderer,
                    gameUI.saveLoadFocus == 1
                        ? 94
                        : 45,
                    gameUI.saveLoadFocus == 1
                        ? 74
                        : 37,
                    gameUI.saveLoadFocus == 1
                        ? 38
                        : 28,
                    220
                );

                SDL_RenderFillRect(
                    renderer,
                    &editArea
                );

                SDL_SetRenderDrawColor(
                    renderer,
                    gameUI.saveLoadFocus == 1
                        ? 235
                        : 120,
                    gameUI.saveLoadFocus == 1
                        ? 207
                        : 100,
                    gameUI.saveLoadFocus == 1
                        ? 126
                        : 70,
                    255
                );

                SDL_RenderDrawRect(
                    renderer,
                    &editArea
                );

                renderRuntimeMetalText(
                    renderer,
                    runtimeMetalFont,
                    gameUI.saveName,
                    editArea.x + 5,
                    editArea.y + 3
                );

                // Blinking cursor while the name field owns
                // active text input.
                if (
                    saveNameEditing &&
                    (
                        (
                            SDL_GetTicks() /
                            500
                        ) %
                        2
                    ) == 0
                ) {
                    const int textWidth =
                        measureRuntimeMetalText(
                            runtimeMetalFont,
                            gameUI.saveName
                        );

                    SDL_Rect editCursor = {
                        editArea.x +
                            5 +
                            textWidth +
                            1,
                        editArea.y + 3,
                        2,
                        14
                    };

                    if (
                        editCursor.x >
                            editArea.x +
                                editArea.w - 4
                    ) {
                        editCursor.x =
                            editArea.x +
                            editArea.w - 4;
                    }

                    SDL_SetRenderDrawColor(
                        renderer,
                        255,
                        255,
                        255,
                        255
                    );

                    SDL_RenderFillRect(
                        renderer,
                        &editCursor
                    );
                }
            }

            SDL_Rect confirmButton = {
                panelX,
                panelY + 270,
                panelW,
                30
            };

            SDL_SetRenderDrawColor(
                renderer,
                70,
                65,
                55,
                255
            );

            SDL_RenderFillRect(
                renderer,
                &confirmButton
            );

            SDL_SetRenderDrawColor(
                renderer,
                gameUI.saveLoadFocus == 2
                    ? 255
                    : 170,
                gameUI.saveLoadFocus == 2
                    ? 221
                    : 150,
                gameUI.saveLoadFocus == 2
                    ? 116
                    : 90,
                255
            );

            SDL_RenderDrawRect(
                renderer,
                &confirmButton
            );

            renderRuntimeMetalTextCentered(
                renderer,
                runtimeMetalFont,
                savePage ? "Save" : "Load",
                confirmButton
            );

            SDL_SetRenderDrawBlendMode(
                renderer,
                SDL_BLENDMODE_NONE
            );
        }


        if (
            gameUI.section ==
                GameUISection::Unit
        ) {
            renderUnitUI(
                renderer,
                unitUIResources,
                unitUI,
                units,
                unitCount
            );
        }

        SDL_SetRenderDrawColor(renderer, 255, 230, 120, 255);

        SDL_RenderDrawLine(renderer,
            (int)cursorX - 8, (int)cursorY,
            (int)cursorX + 8, (int)cursorY);

        SDL_RenderDrawLine(renderer,
            (int)cursorX, (int)cursorY - 8,
            (int)cursorX, (int)cursorY + 8);

        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }


    if (saveNameEditing) {
        SDL_StopTextInput();
        saveNameEditing = false;
    }

    if (settingsPopupTexture) {
        SDL_DestroyTexture(
            settingsPopupTexture
        );

        settingsPopupTexture = nullptr;
    }

    if (settingsMenuButtonTexture) {
        SDL_DestroyTexture(
            settingsMenuButtonTexture
        );

        settingsMenuButtonTexture = nullptr;
    }

    if (settingsSoundText) {
        SDL_DestroyTexture(
            settingsSoundText
        );

        settingsSoundText = nullptr;
    }

    if (settingsGameplayText) {
        SDL_DestroyTexture(
            settingsGameplayText
        );

        settingsGameplayText = nullptr;
    }

    if (settingsGraphicsText) {
        SDL_DestroyTexture(settingsGraphicsText);
        settingsGraphicsText = nullptr;
    }

    if (settingsControlsText) {
        SDL_DestroyTexture(settingsControlsText);
        settingsControlsText = nullptr;
    }

    destroyRuntimeMetalFont(
        &runtimeMetalFont
    );

    // House textury musi byt uvolneny jeste pred
    // znicenim SDL rendereru.
    destroyHouseIconResources(
        &houseIconResources
    );

    // SchoolUIResources obsahuje sidebar, toolbar,
    // Ratios ware ikony, Statistics atlasy a School GUI.
    // Vsechny textury uvolnime pred SDL rendererem.
    destroySchoolUIResources(
        &schoolUIResources
    );

    destroyUnitResources(&unitResources);
    destroyBuildingResources(&buildingResources);
    destroyKaMPlanResources(
        &kamPlanResources
    );

    destroyTerrainResources(&terrain);

    destroyUnitUIResources(
        &unitUIResources
    );

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
