#include "buildings.h"
#include "units.h"
#include <climits>
#include "house_plan_catalog.h"

static SDL_Texture* loadColorKeyTexture(
    SDL_Renderer* renderer,
    const char* path
)
{
    SDL_Surface* surface = SDL_LoadBMP(path);

    if (!surface)
        return nullptr;

    SDL_SetColorKey(
        surface,
        SDL_TRUE,
        SDL_MapRGB(surface->format, 255, 0, 255)
    );

    SDL_Texture* texture =
        SDL_CreateTextureFromSurface(renderer, surface);

    SDL_FreeSurface(surface);
    return texture;
}


// ============================================================
// GENERAL HOUSE INPUT WARE API
// ============================================================

int getHouseInputSlot(
    BuildingType type,
    WareType ware
)
{
    switch (type) {
        case BuildingType::Bakery:
            return
                ware == WareType::Flour
                    ? 0
                    : -1;

        case BuildingType::Store:
        case BuildingType::Farm:
        case BuildingType::School:
            return -1;
    }

    return -1;
}


static bool isUsableInputHouse(
    const Building& building
)
{
    return
        building.active &&
        building.type != BuildingType::Store &&
        isBuildingComplete(building);
}


int getHouseInputAmount(
    const Building& building,
    WareType ware
)
{
    if (!isUsableInputHouse(building))
        return 0;

    const int slot =
        getHouseInputSlot(
            building.type,
            ware
        );

    if (
        slot < 0 ||
        slot >= HOUSE_RESOURCE_SLOT_COUNT
    ) {
        return 0;
    }

    const int amount =
        building.resourceIn[slot];

    return amount > 0
        ? amount
        : 0;
}


int getHouseIncomingAmount(
    const Building& building,
    WareType ware
)
{
    if (!isUsableInputHouse(building))
        return 0;

    const int slot =
        getHouseInputSlot(
            building.type,
            ware
        );

    if (
        slot < 0 ||
        slot >= HOUSE_RESOURCE_SLOT_COUNT
    ) {
        return 0;
    }

    const int amount =
        building.resourceDeliveryCount[slot];

    return amount > 0
        ? amount
        : 0;
}


bool canHouseAcceptWare(
    const Building& building,
    WareType ware,
    int amount
)
{
    if (
        !isUsableInputHouse(building) ||
        !building.wareDeliveryEnabled ||
        !isValidWareType(ware) ||
        amount <= 0
    ) {
        return false;
    }

    const int slot =
        getHouseInputSlot(
            building.type,
            ware
        );

    if (
        slot < 0 ||
        slot >= HOUSE_RESOURCE_SLOT_COUNT
    ) {
        return false;
    }

    const int stored =
        getHouseInputAmount(
            building,
            ware
        );

    const int incoming =
        getHouseIncomingAmount(
            building,
            ware
        );

    if (
        stored > HOUSE_INPUT_CAPACITY ||
        incoming > HOUSE_INPUT_CAPACITY
    ) {
        return false;
    }

    return
        amount <=
        HOUSE_INPUT_CAPACITY -
        stored -
        incoming;
}


bool reserveHouseWareDelivery(
    Building* building,
    WareType ware,
    int amount
)
{
    if (
        !building ||
        !canHouseAcceptWare(
            *building,
            ware,
            amount
        )
    ) {
        return false;
    }

    const int slot =
        getHouseInputSlot(
            building->type,
            ware
        );

    building->
        resourceDeliveryCount[slot] +=
            amount;

    return true;
}


bool cancelHouseWareDelivery(
    Building* building,
    WareType ware,
    int amount
)
{
    if (
        !building ||
        amount <= 0
    ) {
        return false;
    }

    const int slot =
        getHouseInputSlot(
            building->type,
            ware
        );

    if (
        slot < 0 ||
        slot >= HOUSE_RESOURCE_SLOT_COUNT
    ) {
        return false;
    }

    if (
        building->
            resourceDeliveryCount[slot] <
        amount
    ) {
        return false;
    }

    building->
        resourceDeliveryCount[slot] -=
            amount;

    return true;
}


bool deliverHouseWare(
    Building* building,
    WareType ware,
    int amount
)
{
    if (
        !building ||
        amount <= 0
    ) {
        return false;
    }

    const int slot =
        getHouseInputSlot(
            building->type,
            ware
        );

    if (
        slot < 0 ||
        slot >= HOUSE_RESOURCE_SLOT_COUNT
    ) {
        return false;
    }

    if (
        building->
            resourceDeliveryCount[slot] <
        amount
    ) {
        return false;
    }

    const int current =
        getHouseInputAmount(
            *building,
            ware
        );

    if (
        current >
            HOUSE_INPUT_CAPACITY -
            amount
    ) {
        return false;
    }

    building->
        resourceDeliveryCount[slot] -=
            amount;

    building->
        resourceIn[slot] +=
            amount;

    return true;
}


bool buildingsOverlap(
    const Building& a,
    BuildingType type,
    int tileX,
    int tileY
)
{
    if (!a.active)
        return false;

    int candidateWidth = getBuildingWidth(type);
    int candidateHeight = getBuildingHeight(type);

    int existingWidth = getBuildingWidth(a.type);
    int existingHeight = getBuildingHeight(a.type);

    int candidateRight = tileX + candidateWidth - 1;
    int candidateBottom = tileY + candidateHeight - 1;

    int existingRight = a.tileX + existingWidth - 1;
    int existingBottom = a.tileY + existingHeight - 1;

    bool separated =
        candidateRight < a.tileX ||
        tileX > existingRight ||
        candidateBottom < a.tileY ||
        tileY > existingBottom;

    return !separated;
}


// ============================================================
// KAM HOUSE RENDER SCALE 40 TO 32
//
// Originalni houses.rx / houses.dat pracuji v prostoru
// CELL_SIZE_PX = 40.
//
// KaMVita world pouziva TILE_SIZE = 32.
//
// Zdrojove bitmapy a source recty zustavaji originalni.
// Skaluji se pouze destination rozmery a sprite-pixelove
// pivot/move/anchor offsety.
//
// Zaokrouhleni je symetricke i pro zaporne pivoty.
// ============================================================

// ============================================================
// HOUSE VISUAL X ANCHOR CALIBRATION
//
// Scale 40 -> 32 je spolecny a zustava beze zmeny.
//
// Vita visual verification:
// Bakery = centered
// Farm   = centered
// Store  = too far left
// School = too far left
//
// Proto korigujeme pouze vizualni House anchor.
// Logical footprint ani doorway se zde nemeni.
//
// Prvni kalibracni hodnoty:
// Store  +6 px
// School +10 px
// ============================================================

static int getHouseVisualOffsetX(
    BuildingType type,
    int tileSize
)
{
    // Hodnoty jsou definovane pro Vita TILE_SIZE 32.
    // Prevod zachovava stejne chovani i pri jinem tileSize.
    int offsetAt32 = 0;

    switch (type) {
        case BuildingType::Store:
            offsetAt32 = 9;
            break;

        case BuildingType::School:
            offsetAt32 = 13;
            break;

        case BuildingType::Bakery:
        case BuildingType::Farm:
            offsetAt32 = 0;
            break;
    }

    if (offsetAt32 >= 0) {
        return (
            offsetAt32 * tileSize + 16
        ) / 32;
    }

    return -(
        (
            -offsetAt32 * tileSize + 16
        ) / 32
    );
}


static int scaleKaMHousePixel(
    int value,
    int tileSize
)
{
    constexpr int ORIGINAL_KAM_CELL_SIZE = 40;

    if (value >= 0) {
        return (
            value * tileSize +
            ORIGINAL_KAM_CELL_SIZE / 2
        ) / ORIGINAL_KAM_CELL_SIZE;
    }

    return -(
        (
            -value * tileSize +
            ORIGINAL_KAM_CELL_SIZE / 2
        ) / ORIGINAL_KAM_CELL_SIZE
    );
}


static int getHousePlanCatalogIndex(
    BuildingType type
)
{
    switch (type) {
        case BuildingType::Store:
            return 20;

        case BuildingType::Bakery:
            return 2;

        case BuildingType::Farm:
            return 6;

        case BuildingType::School:
            return 17;
    }

    return -1;
}


void renderBuildings(
    SDL_Renderer* renderer,
    const BuildingResources& resources,
    const Building* buildings,
    int buildingCount,
    float cameraX,
    float cameraY,
    int tileSize,
    const KaMPlanResources* planResources,
    bool bakeryHasInsideBaker,
    bool farmHasInsideFarmer,
    const Unit* units,
    int unitCount,
    int buildingIndexBase
)
{
    for (int i = 0; i < buildingCount; ++i) {
        if (!buildings[i].active)
            continue;

        const Building& building = buildings[i];

        const int globalBuildingIndex =
            buildingIndexBase + i;

        bool millHasInsideBaker = false;

        if (
            building.type == BuildingType::Mill &&
            units &&
            unitCount > 0
        ) {
            for (
                int unitIndex = 0;
                unitIndex < unitCount;
                ++unitIndex
            ) {
                const Unit& candidate =
                    units[unitIndex];

                if (
                    candidate.active &&
                    candidate.type == UnitType::Baker &&
                    !candidate.visible &&
                    candidate.targetBuilding ==
                        globalBuildingIndex
                ) {
                    millHasInsideBaker = true;
                    break;
                }
            }
        }

        int footprintX =
            building.tileX * tileSize - (int)cameraX;

        int footprintY =
            building.tileY * tileSize - (int)cameraY;

        int buildingWidth =
            getBuildingWidth(building.type);

        int buildingHeight =
            getBuildingHeight(building.type);

        int footprintWidth =
            buildingWidth * tileSize;

        int footprintHeight =
            buildingHeight * tileSize;

        SDL_Texture* woodAtlas = nullptr;
        SDL_Texture* stoneAtlas = nullptr;

        int woodFrameWidth = 0;
        int woodFrameHeight = 0;
        int woodColumns = 1;
        int woodMaximumStep = 0;
        int woodPivotX = 0;
        int woodPivotY = 0;

        int stoneFrameWidth = 0;
        int stoneFrameHeight = 0;
        int stoneColumns = 1;
        int stoneMaximumStep = 0;
        int stonePivotX = 0;
        int stonePivotY = 0;

        switch (building.type) {
            case BuildingType::Store:
                woodAtlas = resources.storeWoodAtlas;
                stoneAtlas = resources.storeStoneAtlas;

                woodFrameWidth = 134;
                woodFrameHeight = 117;
                woodColumns = 5;
                woodMaximumStep = 24;
                woodPivotX = -69;
                woodPivotY = -82;

                stoneFrameWidth = 144;
                stoneFrameHeight = 136;
                stoneColumns = 10;
                stoneMaximumStep = 75;
                stonePivotX = -78;
                stonePivotY = -99;
                break;

            case BuildingType::Bakery:
                woodAtlas = resources.bakeryWoodAtlas;
                stoneAtlas = resources.bakeryStoneAtlas;

                woodFrameWidth = 130;
                woodFrameHeight = 122;
                woodColumns = 6;
                woodMaximumStep = 21;
                woodPivotX = -38;
                woodPivotY = -92;

                stoneFrameWidth = 140;
                stoneFrameHeight = 131;
                stoneColumns = 11;
                stoneMaximumStep = 54;
                stonePivotX = -44;
                stonePivotY = -98;
                break;

            case BuildingType::Farm:
                woodAtlas = resources.farmWoodAtlas;
                stoneAtlas = resources.farmStoneAtlas;

                woodFrameWidth = 166;
                woodFrameHeight = 107;
                woodColumns = 9;
                woodMaximumStep = 26;
                woodPivotX = -73;
                woodPivotY = -74;

                stoneFrameWidth = 175;
                stoneFrameHeight = 124;
                stoneColumns = 11;
                stoneMaximumStep = 43;
                stonePivotX = -78;
                stonePivotY = -91;
                break;

            case BuildingType::School:
                woodAtlas = resources.schoolWoodAtlas;
                stoneAtlas = resources.schoolStoneAtlas;

                woodFrameWidth = 149;
                woodFrameHeight = 108;
                woodColumns = 6;
                woodMaximumStep = 21;
                woodPivotX = -79;
                woodPivotY = -90;

                stoneFrameWidth = 171;
                stoneFrameHeight = 142;
                stoneColumns = 11;
                stoneMaximumStep = 65;
                stonePivotX = -89;
                stonePivotY = -107;
                break;
            case BuildingType::Quarry:
                woodAtlas = resources.quarryWoodAtlas;
                stoneAtlas = resources.quarryStoneAtlas;

                // Original houses.rx 212 + 214.
                woodFrameWidth = 146;
                woodFrameHeight = 85;
                woodColumns = 6;
                woodMaximumStep = 17;
                woodPivotX = -48;
                woodPivotY = -55;

                // Original houses.rx 211 + 213.
                stoneFrameWidth = 148;
                stoneFrameHeight = 91;
                stoneColumns = 5;
                stoneMaximumStep = 18;
                stonePivotX = -52;
                stonePivotY = -61;
                break;
            case BuildingType::Mill:
                woodAtlas = resources.millWoodAtlas;
                stoneAtlas = resources.millStoneAtlas;

                // Original houses.rx 359 + 361.
                woodFrameWidth = 103;
                woodFrameHeight = 113;
                woodColumns = 6;
                woodMaximumStep = 21;
                woodPivotX = -27;
                woodPivotY = -85;

                // Original houses.rx 358 + 360.
                stoneFrameWidth = 111;
                stoneFrameHeight = 113;
                stoneColumns = 8;
                stoneMaximumStep = 31;
                stonePivotX = -35;
                stonePivotY = -85;
                break;
        }

        int woodStep = building.woodStep;
        int stoneStep = building.stoneStep;

        if (woodStep < 0)
            woodStep = 0;

        if (woodStep > woodMaximumStep)
            woodStep = woodMaximumStep;

        if (stoneStep < 0)
            stoneStep = 0;

        if (stoneStep > stoneMaximumStep)
            stoneStep = stoneMaximumStep;

        // Pokud uz zacal kamen, drevena konstrukce musi byt cela.
        int renderedWoodStep =
            stoneStep > 0
                ? woodMaximumStep
                : woodStep;

        // House pred prvnim Wood/Stone krokem:
        //
        // constructionProgress == 0:
        //   originalni provaz 105/117 + House tablet
        //
        // constructionProgress > 0:
        //   originalni drevene WIP ohraniceni 463/467
        if (renderedWoodStep == 0 && stoneStep == 0) {
            const int catalogIndex =
                getHousePlanCatalogIndex(
                    building.type
                );

            if (
                catalogIndex >= 0 &&
                planResources
            ) {
                const HousePlanCatalogEntry& plan =
                    getHousePlanCatalogEntry(
                        catalogIndex
                    );

                // Soucasne KaMVita tileX/tileY ukazuje na
                // levy horni roh aktivniho footprintu.
                //
                // HouseDatX je plny 4x4 grid, a proto
                // zde prevedeme jeho aktivni cast na
                // soucasnou lokalni kotvu budovy.
                int minX = 4;
                int minY = 4;

                for (int y = 0; y < 4; ++y) {
                    for (int x = 0; x < 4; ++x) {
                        if (plan.plan[y][x] == 0)
                            continue;

                        if (x < minX)
                            minX = x;

                        if (y < minY)
                            minY = y;
                    }
                }

                if (minX >= 4)
                    minX = 0;

                if (minY >= 4)
                    minY = 0;

                const bool preparedTerrain =
                    building.constructionProgress > 0;

                SDL_Texture* horizontal =
                    preparedTerrain
                        ? planResources->
                            houseWipHorizontal
                        : planResources->
                            housePlanHorizontal;

                SDL_Texture* vertical =
                    preparedTerrain
                        ? planResources->
                            houseWipVertical
                        : planResources->
                            housePlanVertical;

                for (int y = 0; y < 4; ++y) {
                    for (int x = 0; x < 4; ++x) {
                        if (plan.plan[y][x] == 0)
                            continue;

                        const int cellScreenX =
                            footprintX +
                            (x - minX) * tileSize;

                        const int cellScreenY =
                            footprintY +
                            (y - minY) * tileSize;

                        const bool north =
                            y == 0 ||
                            plan.plan[y - 1][x] == 0;

                        const bool east =
                            x == 3 ||
                            plan.plan[y][x + 1] == 0;

                        const bool south =
                            y == 3 ||
                            plan.plan[y + 1][x] == 0;

                        const bool west =
                            x == 0 ||
                            plan.plan[y][x - 1] == 0;

                        if (north) {
                            renderKaMHouseEdge(
                                renderer,
                                horizontal,
                                vertical,
                                cellScreenX,
                                cellScreenY,
                                tileSize,
                                0
                            );
                        }

                        if (east) {
                            renderKaMHouseEdge(
                                renderer,
                                horizontal,
                                vertical,
                                cellScreenX,
                                cellScreenY,
                                tileSize,
                                1
                            );
                        }

                        if (south) {
                            renderKaMHouseEdge(
                                renderer,
                                horizontal,
                                vertical,
                                cellScreenX,
                                cellScreenY,
                                tileSize,
                                2
                            );
                        }

                        if (west) {
                            renderKaMHouseEdge(
                                renderer,
                                horizontal,
                                vertical,
                                cellScreenX,
                                cellScreenY,
                                tileSize,
                                3
                            );
                        }
                    }
                }

                // Tablet patri pouze k provazovemu planu.
                if (
                    !preparedTerrain &&
                    (int)building.type >= 0 &&
                    (int)building.type < 6
                ) {
                    SDL_Texture* tablet =
                        planResources->houseTablets[
                            (int)building.type
                        ];

                    if (tablet) {
                        const int entranceScreenX =
                            footprintX +
                            (
                                plan.entranceX -
                                minX
                            ) * tileSize;

                        const int entranceScreenY =
                            footprintY +
                            (
                                plan.entranceY -
                                minY
                            ) * tileSize;

                        // Presny prevod Remake
                        // AddHouseTablet:
                        //
                        // CornerX =
                        //   Loc.X + PivotX / 40 - 0.25
                        //
                        // CornerY =
                        //   Loc.Y +
                        //   (PivotY + Height) / 40 -
                        //   0.55
                        //
                        // Loc.X / Loc.Y v AddHouseTablet
                        // se pouzivaji primo jako mapova
                        // entrance pozice. Nepricitame dalsi
                        // celou dlazdici.
                        const int tabletLocationX =
                            entranceScreenX;

                        const int tabletLocationY =
                            entranceScreenY;

                        const int horizontalOffset =
                            (
                                tileSize * 25 +
                                50
                            ) / 100;

                        const int verticalOffset =
                            (
                                tileSize * 55 +
                                50
                            ) / 100;

                        // KaMVita entrance anchor je proti
                        // Remake Loc.X o jednu mapovou bunku
                        // vlevo. Y uz odpovida spravne.
                        SDL_Rect destination = {
                            tabletLocationX +
                                tileSize +
                                plan.tabletPivotX -
                                horizontalOffset,

                            tabletLocationY +
                                plan.tabletPivotY +
                                plan.tabletHeight -
                                verticalOffset,

                            plan.tabletWidth,
                            plan.tabletHeight
                        };

                        SDL_RenderCopy(
                            renderer,
                            tablet,
                            nullptr,
                            &destination
                        );
                    }
                }
            }

            continue;
        }


        // Kamenna vrstva je kotva pro celou budovu.
        //
        // Source rect zustava v originalnich KaM pixelech.
        // Destination rect pouziva prevod 40 -> tileSize.
        const int scaledStoneWidth =
            scaleKaMHousePixel(
                stoneFrameWidth,
                tileSize
            );

        const int scaledStoneHeight =
            scaleKaMHousePixel(
                stoneFrameHeight,
                tileSize
            );

        const int scaledWoodWidth =
            scaleKaMHousePixel(
                woodFrameWidth,
                tileSize
            );

        const int scaledWoodHeight =
            scaleKaMHousePixel(
                woodFrameHeight,
                tileSize
            );

        const int houseVisualOffsetX =
            getHouseVisualOffsetX(
                building.type,
                tileSize
            );

        int stoneDestinationX =
            footprintX +
            (
                footprintWidth -
                scaledStoneWidth
            ) / 2 +
            houseVisualOffsetX;

        int stoneDestinationY =
            footprintY +
            footprintHeight -
            scaledStoneHeight;


        // Originalni KaM School pouziva virtualni
        // fPosition na local (2, 2). Renderer pak
        // aplikuje pivot a odecte jednu dlazdici:
        //
        // destination =
        // footprint + TILE_SIZE + originalni pivot
        //
        // Centrovani sprite posouvalo School vuci
        // logickemu Entrance doprava a dolu.

        // Drevo se zarovna vuci kameni podle rozdilu
        // originalnich pivotu z houses.rx.
        int woodDestinationX =
            stoneDestinationX +
            scaleKaMHousePixel(
                woodPivotX - stonePivotX,
                tileSize
            );

        int woodDestinationY =
            stoneDestinationY +
            scaleKaMHousePixel(
                woodPivotY - stonePivotY,
                tileSize
            );

        if (woodAtlas && renderedWoodStep > 0) {
            const bool useRuntimeMillWoodStep =
                building.type == BuildingType::Mill &&
                renderedWoodStep > 0 &&
                renderedWoodStep < woodMaximumStep &&
                renderedWoodStep < 22 &&
                resources.millRuntimeWoodSteps[
                    renderedWoodStep
                ] != nullptr;

            const bool useRuntimeMillWood =
                building.type == BuildingType::Mill &&
                renderedWoodStep >= woodMaximumStep &&
                resources.millRuntimeWoodTexture !=
                    nullptr;

            SDL_Texture* renderedWoodTexture =
                useRuntimeMillWood
                    ? resources.millRuntimeWoodTexture
                    : (
                        useRuntimeMillWoodStep
                            ? resources
                                .millRuntimeWoodSteps[
                                    renderedWoodStep
                                ]
                            : woodAtlas
                    );

            SDL_Rect source = {};

            int renderedWoodWidth = woodFrameWidth;
            int renderedWoodHeight = woodFrameHeight;

            if (
                useRuntimeMillWood ||
                useRuntimeMillWoodStep
            ) {
                // Direct external houses.rx runtime texture.
                // Final step uses WoodPic RX359 directly.
                // Intermediate steps use RX359 + RX361.
                SDL_QueryTexture(
                    renderedWoodTexture,
                    nullptr,
                    nullptr,
                    &renderedWoodWidth,
                    &renderedWoodHeight
                );

                source.x = 0;
                source.y = 0;
                source.w = renderedWoodWidth;
                source.h = renderedWoodHeight;
            } else {
                const int frameColumn =
                    renderedWoodStep % woodColumns;

                const int frameRow =
                    renderedWoodStep / woodColumns;

                source.x =
                    frameColumn * woodFrameWidth;

                source.y =
                    frameRow * woodFrameHeight;

                source.w = woodFrameWidth;
                source.h = woodFrameHeight;
            }

            SDL_Rect destination = {
                woodDestinationX,
                woodDestinationY,
                (
                    useRuntimeMillWood ||
                    useRuntimeMillWoodStep
                )
                    ? scaleKaMHousePixel(
                        renderedWoodWidth,
                        tileSize
                    )
                    : scaledWoodWidth,
                (
                    useRuntimeMillWood ||
                    useRuntimeMillWoodStep
                )
                    ? scaleKaMHousePixel(
                        renderedWoodHeight,
                        tileSize
                    )
                    : scaledWoodHeight
            };

            SDL_RenderCopy(
                renderer,
                renderedWoodTexture,
                &source,
                &destination
            );
        }

        // Kamenna vrstva se vykresluje pres hotove drevo.
        if (stoneAtlas && stoneStep > 0) {
            const bool useRuntimeMillStone =
                building.type == BuildingType::Mill &&
                stoneStep >= stoneMaximumStep &&
                resources.millRuntimeStoneTexture !=
                    nullptr;

            SDL_Texture* renderedStoneTexture =
                useRuntimeMillStone
                    ? resources.millRuntimeStoneTexture
                    : stoneAtlas;

            SDL_Rect source = {};

            if (useRuntimeMillStone) {
                // Direct external houses.rx sprite RX 358.
                source.x = 0;
                source.y = 0;
                source.w = stoneFrameWidth;
                source.h = stoneFrameHeight;
            } else {
                const int frameColumn =
                    stoneStep % stoneColumns;

                const int frameRow =
                    stoneStep / stoneColumns;

                source.x =
                    frameColumn * stoneFrameWidth;

                source.y =
                    frameRow * stoneFrameHeight;

                source.w = stoneFrameWidth;
                source.h = stoneFrameHeight;
            }

            SDL_Rect destination = {
                stoneDestinationX,
                stoneDestinationY,
                scaledStoneWidth,
                scaledStoneHeight
            };

            SDL_RenderCopy(
                renderer,
                renderedStoneTexture,
                &source,
                &destination
            );
        }

        static constexpr int BAKERY_BREAD_SUPPLY_CELL_WIDTH = 32;
        static constexpr int BAKERY_BREAD_SUPPLY_CELL_HEIGHT = 20;
        static constexpr int BAKERY_BREAD_SUPPLY_ANCHOR_X = -30;
        static constexpr int BAKERY_BREAD_SUPPLY_ANCHOR_Y = 5;

        // =================================================
        // STORE ORIGINAL FLAG1 FLAG2 OVERLAYS
        //
        // Store nema ha_Flagpole.
        //
        // Flag1:
        // RX = 1166,1167,1168,1164,1165
        // Move = (15,-74)
        //
        // Flag2:
        // RX = 1164,1165,1166,1167,1168
        // Move = (-54,-76)
        //
        // Oba overlaye pouzivaji House 40 -> 32 scale.
        // =================================================

        if (
            building.type == BuildingType::Store &&
            isBuildingComplete(building)
        ) {
            const int storeFlagFrame =
                (SDL_GetTicks() / 120) % 5;

            if (resources.storeFlag1Atlas) {
                constexpr int cellW =
                    18;

                constexpr int cellH =
                    11;

                constexpr int anchorX =
                    -1;

                constexpr int anchorY =
                    -21;

                constexpr int moveX =
                    15;

                constexpr int moveY =
                    -74;

                SDL_Rect src = {
                    storeFlagFrame * cellW,
                    0,
                    cellW,
                    cellH
                };

                SDL_Rect dst = {
                    stoneDestinationX +
                        scaleKaMHousePixel(
                            anchorX +
                                moveX -
                                stonePivotX,
                            tileSize
                        ),

                    stoneDestinationY +
                        scaleKaMHousePixel(
                            anchorY +
                                moveY -
                                stonePivotY,
                            tileSize
                        ),

                    scaleKaMHousePixel(
                        cellW,
                        tileSize
                    ),

                    scaleKaMHousePixel(
                        cellH,
                        tileSize
                    )
                };

                SDL_RenderCopy(
                    renderer,
                    resources.storeFlag1Atlas,
                    &src,
                    &dst
                );
            }

            if (resources.storeFlag2Atlas) {
                constexpr int cellW =
                    18;

                constexpr int cellH =
                    11;

                constexpr int anchorX =
                    -1;

                constexpr int anchorY =
                    -21;

                constexpr int moveX =
                    -54;

                constexpr int moveY =
                    -76;

                SDL_Rect src = {
                    storeFlagFrame * cellW,
                    0,
                    cellW,
                    cellH
                };

                SDL_Rect dst = {
                    stoneDestinationX +
                        scaleKaMHousePixel(
                            anchorX +
                                moveX -
                                stonePivotX,
                            tileSize
                        ),

                    stoneDestinationY +
                        scaleKaMHousePixel(
                            anchorY +
                                moveY -
                                stonePivotY,
                            tileSize
                        ),

                    scaleKaMHousePixel(
                        cellW,
                        tileSize
                    ),

                    scaleKaMHousePixel(
                        cellH,
                        tileSize
                    )
                };

                SDL_RenderCopy(
                    renderer,
                    resources.storeFlag2Atlas,
                    &src,
                    &dst
                );
            }
        }


        // =================================================
        // BAKERY ORIGINAL FLAGPOLE FLAG1 OVERLAY
        //
        // StonePic pivot = (-44,-98)
        //
        // Flagpole:
        // RX   = 1200
        // Move = (54,-64)
        //
        // Flag1:
        // RX =
        // 1181,1182,1183,1204,1205,
        // 1206,1207,1208,1179,1180
        // Move = (103,-56)
        //
        // Flag2 / Flag3 jsou v originalu prazdne.
        // =================================================

        if (
            building.type == BuildingType::Bakery &&
            isBuildingComplete(building)
        ) {
            if (resources.bakeryFlagpole) {
                constexpr int poleWidth =
                    11;

                constexpr int poleHeight =
                    28;

                constexpr int polePivotX =
                    -8;

                constexpr int polePivotY =
                    -29;

                constexpr int poleMoveX =
                    54;

                constexpr int poleMoveY =
                    -64;

                SDL_Rect poleDestination = {
                    stoneDestinationX +
                        scaleKaMHousePixel(
                            polePivotX +
                                poleMoveX -
                                stonePivotX,
                            tileSize
                        ),

                    stoneDestinationY +
                        scaleKaMHousePixel(
                            polePivotY +
                                poleMoveY -
                                stonePivotY,
                            tileSize
                        ),

                    scaleKaMHousePixel(
                        poleWidth,
                        tileSize
                    ),

                    scaleKaMHousePixel(
                        poleHeight,
                        tileSize
                    )
                };

                SDL_RenderCopy(
                    renderer,
                    resources.bakeryFlagpole,
                    nullptr,
                    &poleDestination
                );
            }

            if (resources.bakeryFlag1Atlas) {
                constexpr int flagCellWidth =
                    22;

                constexpr int flagCellHeight =
                    21;

                constexpr int flagFrames =
                    10;

                constexpr int flagAnchorX =
                    -49;

                constexpr int flagAnchorY =
                    -33;

                constexpr int flagMoveX =
                    103;

                constexpr int flagMoveY =
                    -56;

                const int flagFrame =
                    (SDL_GetTicks() / 120) %
                    flagFrames;

                SDL_Rect flagSource = {
                    flagFrame *
                        flagCellWidth,
                    0,
                    flagCellWidth,
                    flagCellHeight
                };

                SDL_Rect flagDestination = {
                    stoneDestinationX +
                        scaleKaMHousePixel(
                            flagAnchorX +
                                flagMoveX -
                                stonePivotX,
                            tileSize
                        ),

                    stoneDestinationY +
                        scaleKaMHousePixel(
                            flagAnchorY +
                                flagMoveY -
                                stonePivotY,
                            tileSize
                        ),

                    scaleKaMHousePixel(
                        flagCellWidth,
                        tileSize
                    ),

                    scaleKaMHousePixel(
                        flagCellHeight,
                        tileSize
                    )
                };

                SDL_RenderCopy(
                    renderer,
                    resources.bakeryFlag1Atlas,
                    &flagSource,
                    &flagDestination
                );
            }
        }


        // =================================================
        // BAKERY ORIGINAL BREAD SUPPLYOUT
        //
        // Remake AddHouseSupply:
        // SupplyOut[1, Min(resourceOut[0],5)].
        //
        // Bakery SupplyOut[1]:
        // 1 -> RX 105
        // 2 -> RX 106
        // 3 -> RX 107
        // 4 -> RX 108
        // 5 -> RX 109
        // =================================================
        if (
            building.type == BuildingType::Bakery &&
            isBuildingComplete(building) &&
            building.resourceOut[0] > 0 &&
            resources.bakeryBreadSupplyAtlas
        ) {
            const int breadCellWidth =
                BAKERY_BREAD_SUPPLY_CELL_WIDTH;

            const int breadCellHeight =
                BAKERY_BREAD_SUPPLY_CELL_HEIGHT;

            int breadCount =
                building.resourceOut[0];

            if (breadCount > 5)
                breadCount = 5;

            const int breadFrame =
                breadCount - 1;

            SDL_Rect breadSource = {
                breadFrame * breadCellWidth,
                0,
                breadCellWidth,
                breadCellHeight
            };

            SDL_Rect breadDestination = {
                stoneDestinationX +
                    scaleKaMHousePixel(
                        BAKERY_BREAD_SUPPLY_ANCHOR_X -
                            stonePivotX,
                        tileSize
                    ),

                stoneDestinationY +
                    scaleKaMHousePixel(
                        BAKERY_BREAD_SUPPLY_ANCHOR_Y -
                            stonePivotY,
                        tileSize
                    ),

                scaleKaMHousePixel(
                    breadCellWidth,
                    tileSize
                ),

                scaleKaMHousePixel(
                    breadCellHeight,
                    tileSize
                )
            };

            SDL_RenderCopy(
                renderer,
                resources.bakeryBreadSupplyAtlas,
                &breadSource,
                &breadDestination
            );
        }


        // =================================================
        // FARM ORIGINAL FLAG1 OVERLAY
        //
        // Original houses.dat Farm house index 8:
        //
        // ha_Flagpole = empty
        //
        // ha_Flag1:
        // Count  = 10
        // Move   = (-1,-69)
        //
        // RX:
        //
        // 1175,1176,1177,1178,1169,
        // 1170,1171,1172,1173,1174
        //
        // Shared atlas geometry:
        //
        // cell   = 44x15
        // anchor = (-46,-18)
        //
        // ha_Flag2 = empty
        // ha_Flag3 = empty
        // =================================================

        if (
            building.type == BuildingType::Farm &&
            isBuildingComplete(building) &&
            resources.farmFlag1Atlas
        ) {
            constexpr int flagCellWidth = 44;
            constexpr int flagCellHeight = 15;
            constexpr int flagFrameCount = 10;

            constexpr int flagAnchorX = -46;
            constexpr int flagAnchorY = -18;

            constexpr int flagMoveX = -1;
            constexpr int flagMoveY = -69;

            const int flagFrame =
                (SDL_GetTicks() / 120) %
                flagFrameCount;

            SDL_Rect flagSource = {
                flagFrame * flagCellWidth,
                0,
                flagCellWidth,
                flagCellHeight
            };

            SDL_Rect flagDestination = {
                stoneDestinationX +
                    scaleKaMHousePixel(
                        flagAnchorX +
                            flagMoveX -
                            stonePivotX,
                        tileSize
                    ),

                stoneDestinationY +
                    scaleKaMHousePixel(
                        flagAnchorY +
                            flagMoveY -
                            stonePivotY,
                        tileSize
                    ),

                scaleKaMHousePixel(
                    flagCellWidth,
                    tileSize
                ),

                scaleKaMHousePixel(
                    flagCellHeight,
                    tileSize
                )
            };

            SDL_RenderCopy(
                renderer,
                resources.farmFlag1Atlas,
                &flagSource,
                &flagDestination
            );
        }


        // =================================================
        // MILL ORIGINAL TURBINE
        //
        // Real Mill houses.dat record 22:
        //
        // ha_Flagpole
        // RX sequence = 348..357
        // count       = 10
        // cell        = 106x147
        // grid        = 5x2
        // anchor      = (-35,-34)
        // Move        = (-17,-80)
        //
        // Original House overlay alignment:
        //
        // stoneDestination +
        // (overlay anchor + House Move - StonePic pivot)
        //
        // The turbine is part of the completed Mill House
        // decoration and therefore rotates continuously.
        // =================================================

        if (
            building.type == BuildingType::Mill &&
            isBuildingComplete(building) &&
            resources.millTurbineAtlas
        ) {
            constexpr int turbineCellWidth = 106;
            constexpr int turbineCellHeight = 147;

            constexpr int turbineColumns = 5;
            constexpr int turbineFrameCount = 10;

            constexpr int turbineAnchorX = -35;
            constexpr int turbineAnchorY = -34;

            constexpr int turbineMoveX = -17;
            constexpr int turbineMoveY = -80;

            // Original House flag/decorative cadence.
            const int turbineFrame =
                (SDL_GetTicks() / 120) %
                turbineFrameCount;

            const int turbineColumn =
                turbineFrame %
                turbineColumns;

            const int turbineRow =
                turbineFrame /
                turbineColumns;

            SDL_Rect turbineSource = {
                turbineColumn *
                    turbineCellWidth,

                turbineRow *
                    turbineCellHeight,

                turbineCellWidth,
                turbineCellHeight
            };

            SDL_Rect turbineDestination = {
                stoneDestinationX +
                    scaleKaMHousePixel(
                        turbineAnchorX +
                            turbineMoveX -
                            stonePivotX,
                        tileSize
                    ),

                stoneDestinationY +
                    scaleKaMHousePixel(
                        turbineAnchorY +
                            turbineMoveY -
                            stonePivotY,
                        tileSize
                    ),

                scaleKaMHousePixel(
                    turbineCellWidth,
                    tileSize
                ),

                scaleKaMHousePixel(
                    turbineCellHeight,
                    tileSize
                )
            };

            SDL_RenderCopy(
                renderer,
                resources.millTurbineAtlas,
                &turbineSource,
                &turbineDestination
            );
        }


        // =================================================
        // MILL ORIGINAL IDLE OVERLAY
        //
        // Real Mill houses.dat record 22:
        //
        // ha_Idle
        // count  = 30
        // cell   = 17x16
        // anchor = (4,-52)
        // Move   = (0,-1)
        //
        // ha_Idle is visible only while a real Baker is
        // physically hidden inside this specific Mill.
        //
        // Alignment:
        //
        // stoneDestination +
        // (overlay anchor + House Move - StonePic pivot)
        // =================================================

        if (
            building.type == BuildingType::Mill &&
            isBuildingComplete(building) &&
            millHasInsideBaker &&
            resources.millIdleAtlas
        ) {
            constexpr int idleCellWidth = 17;
            constexpr int idleCellHeight = 16;
            constexpr int idleFrameCount = 30;

            constexpr int idleAnchorX = 4;
            constexpr int idleAnchorY = -52;

            constexpr int idleMoveX = 0;
            constexpr int idleMoveY = -1;

            const int idleFrame =
                (SDL_GetTicks() / 100) %
                idleFrameCount;

            SDL_Rect idleSource = {
                idleFrame * idleCellWidth,
                0,
                idleCellWidth,
                idleCellHeight
            };

            SDL_Rect idleDestination = {
                stoneDestinationX +
                    scaleKaMHousePixel(
                        idleAnchorX +
                            idleMoveX -
                            stonePivotX,
                        tileSize
                    ),

                stoneDestinationY +
                    scaleKaMHousePixel(
                        idleAnchorY +
                            idleMoveY -
                            stonePivotY,
                        tileSize
                    ),

                scaleKaMHousePixel(
                    idleCellWidth,
                    tileSize
                ),

                scaleKaMHousePixel(
                    idleCellHeight,
                    tileSize
                )
            };

            SDL_RenderCopy(
                renderer,
                resources.millIdleAtlas,
                &idleSource,
                &idleDestination
            );
        }


        // =================================================
        // FARM ORIGINAL CORN SUPPLYOUT
        //
        // Original Remake AddHouseSupply:
        //
        // SupplyOut[
        //     1,
        //     Min(resourceOut[0], 5)
        // ]
        //
        // Farm SupplyOut[1]:
        //
        // 1 -> RX 387
        // 2 -> RX 388
        // 3 -> RX 389
        // 4 -> RX 390
        // 5 -> RX 391
        //
        // Atlas geometry:
        //
        // cell   = 45x34
        // anchor = (20,-3)
        //
        // resourceOut[0] remains the only authoritative
        // physical Corn inventory.
        // =================================================

        if (
            building.type == BuildingType::Farm &&
            isBuildingComplete(building) &&
            building.resourceOut[0] > 0 &&
            resources.farmCornSupplyAtlas
        ) {
            constexpr int cornCellWidth = 45;
            constexpr int cornCellHeight = 34;
            constexpr int cornFrameCount = 5;

            constexpr int cornAnchorX = 20;
            constexpr int cornAnchorY = -3;

            int cornCount =
                building.resourceOut[0];

            if (cornCount > cornFrameCount)
                cornCount = cornFrameCount;

            if (cornCount < 1)
                cornCount = 1;

            const int cornFrame =
                cornCount - 1;

            SDL_Rect cornSource = {
                cornFrame * cornCellWidth,
                0,
                cornCellWidth,
                cornCellHeight
            };

            SDL_Rect cornDestination = {
                stoneDestinationX +
                    scaleKaMHousePixel(
                        cornAnchorX -
                            stonePivotX,
                        tileSize
                    ),

                stoneDestinationY +
                    scaleKaMHousePixel(
                        cornAnchorY -
                            stonePivotY,
                        tileSize
                    ),

                scaleKaMHousePixel(
                    cornCellWidth,
                    tileSize
                ),

                scaleKaMHousePixel(
                    cornCellHeight,
                    tileSize
                )
            };

            SDL_RenderCopy(
                renderer,
                resources.farmCornSupplyAtlas,
                &cornSource,
                &cornDestination
            );
        }


        // =================================================
        // FARM ORIGINAL OCCUPIED IDLE OVERLAY
        //
        // Originalni Farm ha_Idle:
        //
        // count  = 30
        // cell   = 18x17
        // anchor = (8,-6)
        // move   = (-16,-11)
        //
        // RX sequence uses 1083..1088.
        //
        // Overlay is active only while a real Farmer is
        // physically hidden inside this completed Farm.
        // =================================================

        if (
            building.type == BuildingType::Farm &&
            isBuildingComplete(building) &&
            farmHasInsideFarmer &&
            resources.farmIdleAtlas
        ) {
            constexpr int idleCellWidth = 18;
            constexpr int idleCellHeight = 17;
            constexpr int idleFrameCount = 30;

            constexpr int idleAnchorX = 8;
            constexpr int idleAnchorY = -6;

            constexpr int idleMoveX = -16;
            constexpr int idleMoveY = -11;

            // Same 100 ms House cadence as Bakery idle.
            const int idleFrame =
                (SDL_GetTicks() / 100) %
                idleFrameCount;

            SDL_Rect idleSource = {
                idleFrame * idleCellWidth,
                0,
                idleCellWidth,
                idleCellHeight
            };

            SDL_Rect idleDestination = {
                stoneDestinationX +
                    scaleKaMHousePixel(
                        idleAnchorX +
                            idleMoveX -
                            stonePivotX,
                        tileSize
                    ),

                stoneDestinationY +
                    scaleKaMHousePixel(
                        idleAnchorY +
                            idleMoveY -
                            stonePivotY,
                        tileSize
                    ),

                scaleKaMHousePixel(
                    idleCellWidth,
                    tileSize
                ),

                scaleKaMHousePixel(
                    idleCellHeight,
                    tileSize
                )
            };

            SDL_RenderCopy(
                renderer,
                resources.farmIdleAtlas,
                &idleSource,
                &idleDestination
            );
        }


        // =================================================
        // BAKERY ORIGINAL OCCUPIED IDLE OVERLAY
        //
        // Originalni Bakery ha_Idle:
        //
        // count  = 30
        // cell   = 16x37
        // anchor = (-8,-12)
        // move   = (18,5)
        //
        // RX sekvence pouziva 1077..1082.
        //
        // Vykresluje se pouze pokud je skutecny Baker
        // fyzicky uvnitr teto Bakery a House prave nevyrabi.
        //
        // Baker unit zustava uvnitr invisible.
        // =================================================

        if (
            building.type == BuildingType::Bakery &&
            isBuildingComplete(building) &&
            bakeryHasInsideBaker &&
            !building.bakeryProductionActive &&
            resources.bakeryIdleAtlas
        ) {
            constexpr int idleCellWidth = 16;
            constexpr int idleCellHeight = 37;

            constexpr int idleFrameCount = 30;

            constexpr int idleAnchorX = -8;
            constexpr int idleAnchorY = -12;

            constexpr int idleMoveX = 18;
            constexpr int idleMoveY = 5;

            // House production tick pouziva 100 ms.
            // Idle zachovava stejnou 100ms House cadence.
            const int idleFrame =
                (SDL_GetTicks() / 100) %
                idleFrameCount;

            SDL_Rect idleSource = {
                idleFrame * idleCellWidth,
                0,
                idleCellWidth,
                idleCellHeight
            };

            SDL_Rect idleDestination = {
                stoneDestinationX +
                    scaleKaMHousePixel(
                        idleAnchorX +
                            idleMoveX -
                            stonePivotX,
                        tileSize
                    ),

                stoneDestinationY +
                    scaleKaMHousePixel(
                        idleAnchorY +
                            idleMoveY -
                            stonePivotY,
                        tileSize
                    ),

                scaleKaMHousePixel(
                    idleCellWidth,
                    tileSize
                ),

                scaleKaMHousePixel(
                    idleCellHeight,
                    tileSize
                )
            };

            SDL_RenderCopy(
                renderer,
                resources.bakeryIdleAtlas,
                &idleSource,
                &idleDestination
            );
        }


        // =================================================
        // BAKERY ORIGINAL WORK2 WORK3 OVERLAY
        //
        // Originalni houses.dat workplan:
        //   7x Work2(24) + Work3(30)
        //   378 kroku celkem.
        //
        // Tick v jednom paru:
        //   0..23  = Work2 frame 0..23
        //   24..53 = Work3 frame 0..29
        //
        // House overlay se zarovnava stejnym vzorcem
        // jako School:
        //
        // stoneDestination +
        // (overlay anchor + House Move - stone pivot)
        //
        // Work2 i Work3 maji spolecny pivotovy prostor:
        //   cell   = 18x27
        //   anchor = (-10,-13)
        //   move   = (18,5)
        //
        // Zadna temporary nebo nahradni animace.
        // =================================================

        if (
            building.type == BuildingType::Bakery &&
            isBuildingComplete(building) &&
            building.bakeryProductionActive &&
            building.bakeryProductionTick >= 0 &&
            building.bakeryProductionTick < 378
        ) {
            const int workCellWidth = 18;
            const int workCellHeight = 27;

            const int work2FrameCount = 24;
            const int work3FrameCount = 30;
            const int workPairLength =
                work2FrameCount +
                work3FrameCount;

            const int workAnchorX = -10;
            const int workAnchorY = -13;
            const int workMoveX = 18;
            const int workMoveY = 5;

            const int pairTick =
                building.bakeryProductionTick %
                    workPairLength;

            SDL_Texture* workTexture = nullptr;
            int workFrame = 0;

            if (pairTick < work2FrameCount) {
                workTexture =
                    resources.bakeryWork2Atlas;

                workFrame = pairTick;
            } else {
                workTexture =
                    resources.bakeryWork3Atlas;

                workFrame =
                    pairTick -
                    work2FrameCount;
            }

            if (workTexture) {
                SDL_Rect workSource = {
                    workFrame * workCellWidth,
                    0,
                    workCellWidth,
                    workCellHeight
                };

                const int workX =
                    stoneDestinationX +
                    scaleKaMHousePixel(
                        workAnchorX +
                            workMoveX -
                            stonePivotX,
                        tileSize
                    );

                const int workY =
                    stoneDestinationY +
                    scaleKaMHousePixel(
                        workAnchorY +
                            workMoveY -
                            stonePivotY,
                        tileSize
                    );

                SDL_Rect workDestination = {
                    workX,
                    workY,

                    scaleKaMHousePixel(
                        workCellWidth,
                        tileSize
                    ),

                    scaleKaMHousePixel(
                        workCellHeight,
                        tileSize
                    )
                };

                SDL_RenderCopy(
                    renderer,
                    workTexture,
                    &workSource,
                    &workDestination
                );
            }
        }


        // =================================================
        // BAKERY ORIGINAL SMOKE OVERLAY
        //
        // Remake:
        // - adds ha_Smoke when production begins
        // - keeps it during all Work2/Work3 phases
        // - removes it when the House returns to Idle
        //
        // Original Bakery ha_Smoke:
        // RX     = 889,890,891,892
        // count  = 4
        // cell   = 34x54
        // anchor = (19,-151)
        // move   = (18,5)
        // =================================================

        if (
            building.type == BuildingType::Bakery &&
            isBuildingComplete(building) &&
            building.bakeryProductionActive &&
            resources.bakerySmokeAtlas
        ) {
            const int smokeCellWidth =
                34;

            const int smokeCellHeight =
                54;

            const int smokeAnchorX =
                19;

            const int smokeAnchorY =
                -151;

            const int smokeMoveX =
                18;

            const int smokeMoveY =
                5;

            const int smokeFrame =
                building.bakeryProductionTick % 4;

            SDL_Rect smokeSource = {
                smokeFrame * smokeCellWidth,
                0,
                smokeCellWidth,
                smokeCellHeight
            };

            SDL_Rect smokeDestination = {
                stoneDestinationX +
                    scaleKaMHousePixel(
                        smokeAnchorX +
                            smokeMoveX -
                            stonePivotX,
                        tileSize
                    ),

                stoneDestinationY +
                    scaleKaMHousePixel(
                        smokeAnchorY +
                            smokeMoveY -
                            stonePivotY,
                        tileSize
                    ),

                scaleKaMHousePixel(
                    smokeCellWidth,
                    tileSize
                ),

                scaleKaMHousePixel(
                    smokeCellHeight,
                    tileSize
                )
            };

            SDL_RenderCopy(
                renderer,
                resources.bakerySmokeAtlas,
                &smokeSource,
                &smokeDestination
            );
        }


        // SCHOOL ORIGINAL FLAG OVERLAY
        //
        // House overlaye jsou originální sprite-pixelové
        // offsety. Proto je umistujeme relativne k
        // hotovemu StonePic, ne pres TILE_SIZE.
        if (
            building.type == BuildingType::School &&
            isBuildingComplete(building)
        ) {
            // School StonePic:
            // pivot = (-89, -107)
            //
            // Flagpole:
            // pivot = (-2, -24)
            // move  = (-16, -118)
            //
            // Flag1:
            // pivot = (-20, 18)
            // move  = (7, -153)

            if (resources.schoolFlagpole) {
                const int polePivotX = -2;
                const int polePivotY = -24;
                const int poleMoveX = -16;
                const int poleMoveY = -118;

                int poleX =
                    stoneDestinationX +
                    scaleKaMHousePixel(
                        polePivotX +
                            poleMoveX -
                            stonePivotX,
                        tileSize
                    );

                int poleY =
                    stoneDestinationY +
                    scaleKaMHousePixel(
                        polePivotY +
                            poleMoveY -
                            stonePivotY,
                        tileSize
                    );

                SDL_Rect poleDestination = {
                    poleX,
                    poleY,
                    scaleKaMHousePixel(9, tileSize),
                    scaleKaMHousePixel(35, tileSize)
                };

                SDL_RenderCopy(
                    renderer,
                    resources.schoolFlagpole,
                    nullptr,
                    &poleDestination
                );
            }

            if (resources.schoolFlagAtlas) {
                const int flagWidth = 45;
                const int flagHeight = 22;
                const int flagFrames = 10;

                const int flagPivotX = -20;
                const int flagPivotY = 18;
                const int flagMoveX = 7;
                const int flagMoveY = -153;

                int flagFrame =
                    (SDL_GetTicks() / 120) %
                    flagFrames;

                SDL_Rect flagSource = {
                    flagFrame * flagWidth,
                    0,
                    flagWidth,
                    flagHeight
                };

                int flagX =
                    stoneDestinationX +
                    scaleKaMHousePixel(
                        flagPivotX +
                            flagMoveX -
                            stonePivotX,
                        tileSize
                    );

                int flagY =
                    stoneDestinationY +
                    scaleKaMHousePixel(
                        flagPivotY +
                            flagMoveY -
                            stonePivotY,
                        tileSize
                    );

                SDL_Rect flagDestination = {
                    flagX,
                    flagY,

                    scaleKaMHousePixel(
                        flagWidth,
                        tileSize
                    ),

                    scaleKaMHousePixel(
                        flagHeight,
                        tileSize
                    )
                };

                SDL_RenderCopy(
                    renderer,
                    resources.schoolFlagAtlas,
                    &flagSource,
                    &flagDestination
                );
            }

            // SCHOOL WORK1-WORK5 TRAINING OVERLAY
            //
            // Atlas ma 25 bunek 16x20:
            // Work1 0..4, Work2 5..9, ... Work5 20..24.
            // Skutecny rozmer a pivot zustava per-frame.
            if (
                resources.schoolWorkAtlas &&
                building.schoolTrainingActive &&
                building.schoolTrainingStep >= 0 &&
                building.schoolTrainingStep <
                    150
            ) {
                static const int workWidths[25] = {
                    14, 14, 14, 14, 14,
                    14, 14, 14, 14, 14,
                    14, 14, 14, 15, 15,
                    15, 15, 15, 15, 16,
                    15, 15, 15, 15, 15
                };

                static const int workHeights[25] = {
                    13, 13, 13, 13, 13,
                    13, 13, 20, 20, 20,
                    20, 20, 20, 20, 20,
                    20, 20, 20, 13, 13,
                    13, 13, 13, 13, 13
                };

                static const int workPivotX[25] = {
                    -5, -5, -5, -5, -5,
                    -5, -5, -5, -5, -5,
                    -5, -5, -5, -13, -13,
                    -13, -13, -13, -13, -13,
                    -13, -13, -13, -13, -13
                };

                static const int workPivotY[25] = {
                    -76, -76, -76, -76, -76,
                    -76, -76, -76, -76, -76,
                    -76, -76, -76, -76, -76,
                    -76, -76, -76, -76, -76,
                    -76, -76, -76, -76, -76
                };

                const int workCellWidth = 16;
                const int workCellHeight = 20;
                const int workFramesPerPhase = 5;
                const int workTrainingSteps = 150;
                const int workPhaseSteps = 30;
                const int workMoveX = -17;
                const int workMoveY = 5;

                int workPhase =
                    building.schoolTrainingStep /
                    workPhaseSteps;

                if (workPhase > 4)
                    workPhase = 4;

                // Frame je odvozen primo z training kroku.
                // Kazda Work faze ma 30 kroku a 5 RX snimku,
                // tedy jeden RX snimek pripada na 6 kroku.
                int phaseStep =
                    building.schoolTrainingStep %
                    30;

                int phaseFrame =
                    (
                        phaseStep *
                        workFramesPerPhase
                    ) /
                    30;

                if (
                    phaseFrame >=
                        workFramesPerPhase
                ) {
                    phaseFrame =
                        workFramesPerPhase - 1;
                }

                int workFrame =
                    workPhase * workFramesPerPhase +
                    phaseFrame;

                int frameWidth =
                    workWidths[workFrame];

                int frameHeight =
                    workHeights[workFrame];

                SDL_Rect workSource = {
                    workFrame * workCellWidth,
                    0,
                    frameWidth,
                    frameHeight
                };

                int workX =
                    stoneDestinationX +
                    scaleKaMHousePixel(
                        workPivotX[workFrame] +
                            workMoveX -
                            stonePivotX,
                        tileSize
                    );

                int workY =
                    stoneDestinationY +
                    scaleKaMHousePixel(
                        workPivotY[workFrame] +
                            workMoveY -
                            stonePivotY,
                        tileSize
                    );

                SDL_Rect workDestination = {
                    workX,
                    workY,

                    scaleKaMHousePixel(
                        frameWidth,
                        tileSize
                    ),

                    scaleKaMHousePixel(
                        frameHeight,
                        tileSize
                    )
                };

                SDL_RenderCopy(
                    renderer,
                    resources.schoolWorkAtlas,
                    &workSource,
                    &workDestination
                );
            }
        }


    }
}


void getBuildingTopLeftFromEntrance(
    BuildingType type,
    int entranceTileX,
    int entranceTileY,
    int* topLeftTileX,
    int* topLeftTileY
)
{
    int resultX = entranceTileX;
    int resultY = entranceTileY;

    const int catalogIndex =
        getHousePlanCatalogIndex(type);

    if (catalogIndex >= 0) {
        const HousePlanCatalogEntry& plan =
            getHousePlanCatalogEntry(
                catalogIndex
            );

        int minX = 4;
        int minY = 4;

        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                if (plan.plan[y][x] == 0)
                    continue;

                if (x < minX)
                    minX = x;

                if (y < minY)
                    minY = y;
            }
        }

        if (minX >= 4)
            minX = 0;

        if (minY >= 4)
            minY = 0;

        const int normalizedEntranceX =
            plan.entranceX - minX;

        const int normalizedEntranceY =
            plan.entranceY - minY;

        resultX =
            entranceTileX -
            normalizedEntranceX;

        resultY =
            entranceTileY -
            normalizedEntranceY;
    }

    if (topLeftTileX)
        *topLeftTileX = resultX;

    if (topLeftTileY)
        *topLeftTileY = resultY;
}


bool canPlaceBuilding(
    const Building* buildings,
    int buildingCount,
    BuildingType type,
    int tileX,
    int tileY,
    int mapWidth,
    int mapHeight
)
{
    if (tileX < 0 || tileY < 0)
        return false;

    int buildingWidth = getBuildingWidth(type);
    int buildingHeight = getBuildingHeight(type);

    bool insideMap =
        tileX + buildingWidth <= mapWidth &&
        tileY + buildingHeight <= mapHeight;

    if (!insideMap)
        return false;

    for (int localY = 0; localY < buildingHeight; ++localY) {
        for (int localX = 0; localX < buildingWidth; ++localX) {
            int planValue = getBuildingPlanValue(
                type,
                localX,
                localY
            );

            if (planValue == 0)
                continue;

            int worldTileX = tileX + localX;
            int worldTileY = tileY + localY;

            bool tileIsWater =
                worldTileX < 10 &&
                worldTileY < 12;

            if (tileIsWater)
                return false;
        }
    }

    for (int i = 0; i < buildingCount; ++i) {
        if (buildingsOverlap(
                buildings[i],
                type,
                tileX,
                tileY
            )) {
            return false;
        }
    }

    return true;
}

void renderBuildingPreview(
    SDL_Renderer* renderer,
    const BuildingResources& resources,
    const Building* buildings,
    int buildingCount,
    BuildingType selectedType,
    int selectedTileX,
    int selectedTileY,
    float cameraX,
    float cameraY,
    int tileSize,
    int mapWidth,
    int mapHeight
)
{
    if (selectedTileX < 0 || selectedTileY < 0)
        return;

    // selectedTileX/Y represents the entrance tile,
    // matching KaM Remake placement behavior.
    int placementTileX = selectedTileX;
    int placementTileY = selectedTileY;

    getBuildingTopLeftFromEntrance(
        selectedType,
        selectedTileX,
        selectedTileY,
        &placementTileX,
        &placementTileY
    );

    bool previewValid = canPlaceBuilding(
        buildings,
        buildingCount,
        selectedType,
        placementTileX,
        placementTileY,
        mapWidth,
        mapHeight
    );

    int buildingWidth = getBuildingWidth(selectedType);
    int buildingHeight = getBuildingHeight(selectedType);

    for (int localY = 0; localY < buildingHeight; ++localY) {
        for (int localX = 0; localX < buildingWidth; ++localX) {
            int planValue = getBuildingPlanValue(
                selectedType,
                localX,
                localY
            );

            if (planValue == 0)
                continue;

            SDL_Rect tile = {
                (placementTileX + localX) * tileSize -
                    (int)cameraX,
                (placementTileY + localY) * tileSize -
                    (int)cameraY,
                tileSize,
                tileSize
            };

            if (!previewValid) {
                SDL_SetRenderDrawColor(
                    renderer,
                    255, 65, 65, 255
                );
            } else if (planValue == 2) {
                SDL_SetRenderDrawColor(
                    renderer,
                    255, 185, 45, 255
                );
            } else {
                SDL_SetRenderDrawColor(
                    renderer,
                    80, 255, 100, 255
                );
            }

            SDL_RenderDrawRect(renderer, &tile);

            if (planValue == 2) {
                SDL_Rect entranceMarker = {
                    tile.x + tileSize / 4,
                    tile.y + tileSize / 4,
                    tileSize / 2,
                    tileSize / 2
                };

                SDL_RenderDrawRect(
                    renderer,
                    &entranceMarker
                );
            }
        }
    }

    SDL_Texture* previewTexture = nullptr;
    int spriteWidth = 0;
    int spriteHeight = 0;

    switch (selectedType) {
        case BuildingType::Store:
            previewTexture = resources.store;
            spriteWidth = 144;
            spriteHeight = 136;
            break;

        case BuildingType::Bakery:
            previewTexture = resources.bakery;
            spriteWidth = 140;
            spriteHeight = 131;
            break;

        case BuildingType::Farm:
            previewTexture = resources.farm;
            spriteWidth = 175;
            spriteHeight = 124;
            break;

        case BuildingType::School:
            previewTexture = resources.school;
            spriteWidth = 171;
            spriteHeight = 142;
            break;
    }

    if (previewTexture) {
        int footprintWidth =
            buildingWidth * tileSize;

        int footprintHeight =
            buildingHeight * tileSize;

        const int scaledSpriteWidth =
            scaleKaMHousePixel(
                spriteWidth,
                tileSize
            );

        const int scaledSpriteHeight =
            scaleKaMHousePixel(
                spriteHeight,
                tileSize
            );

        SDL_Rect destination = {
            placementTileX * tileSize -
                (int)cameraX +
                (
                    footprintWidth -
                    scaledSpriteWidth
                ) / 2 +
                getHouseVisualOffsetX(
                    selectedType,
                    tileSize
                ),

            placementTileY * tileSize -
                (int)cameraY +
                footprintHeight -
                scaledSpriteHeight,

            scaledSpriteWidth,
            scaledSpriteHeight
        };

        SDL_SetTextureBlendMode(
            previewTexture,
            SDL_BLENDMODE_BLEND
        );

        SDL_SetTextureAlphaMod(
            previewTexture,
            previewValid ? 145 : 95
        );

        SDL_SetTextureColorMod(
            previewTexture,
            255,
            previewValid ? 255 : 150,
            previewValid ? 255 : 150
        );

        SDL_RenderCopy(
            renderer,
            previewTexture,
            nullptr,
            &destination
        );

        SDL_SetTextureAlphaMod(
            previewTexture,
            255
        );

        SDL_SetTextureColorMod(
            previewTexture,
            255,
            255,
            255
        );
    }
}

BuildingResources loadBuildingResources(
    SDL_Renderer* renderer
)
{
    BuildingResources resources = {};

    SDL_Surface* surface =
        SDL_LoadBMP("app0:assets/buildings/store.bmp");

    if (surface) {
        SDL_SetColorKey(
            surface,
            SDL_TRUE,
            SDL_MapRGB(surface->format, 255, 0, 255)
        );
        resources.store =
            SDL_CreateTextureFromSurface(renderer, surface);

        SDL_FreeSurface(surface);
    }


    surface = SDL_LoadBMP("app0:assets/buildings/bakery.bmp");

    if (surface) {
        SDL_SetColorKey(
            surface,
            SDL_TRUE,
            SDL_MapRGB(surface->format, 255, 0, 255)
        );

        resources.bakery =
            SDL_CreateTextureFromSurface(renderer, surface);

        SDL_FreeSurface(surface);
    }

    surface = SDL_LoadBMP("app0:assets/buildings/farm.bmp");

    if (surface) {
        SDL_SetColorKey(
            surface,
            SDL_TRUE,
            SDL_MapRGB(surface->format, 255, 0, 255)
        );

        resources.farm =
            SDL_CreateTextureFromSurface(renderer, surface);

        SDL_FreeSurface(surface);
    }

    resources.roadIcon = loadColorKeyTexture(
        renderer,
        "app0:assets/ui/buildings/road_icon.bmp"
    );

    resources.storeIcon = loadColorKeyTexture(
        renderer,
        "app0:assets/ui/buildings/store_icon.bmp"
    );

    resources.bakeryIcon = loadColorKeyTexture(
        renderer,
        "app0:assets/ui/buildings/bakery_icon.bmp"
    );

    resources.farmIcon = loadColorKeyTexture(
        renderer,
        "app0:assets/ui/buildings/farm_icon.bmp"
    );
    resources.storeWoodAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/construction/store/wood_atlas.bmp"
    );

    resources.storeStoneAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/construction/store/stone_atlas.bmp"
    );

    resources.storeFlag1Atlas =
        loadColorKeyTexture(
            renderer,
            "app0:assets/buildings/store/runtime/flag1_atlas.bmp"
        );

    resources.storeFlag2Atlas =
        loadColorKeyTexture(
            renderer,
            "app0:assets/buildings/store/runtime/flag2_atlas.bmp"
        );

    resources.bakeryWoodAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/construction/bakery/wood_atlas.bmp"
    );

    resources.bakeryStoneAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/construction/bakery/stone_atlas.bmp"
    );

    resources.bakeryWork2Atlas =
        loadColorKeyTexture(
            renderer,
            "app0:assets/buildings/bakery/runtime/work2_atlas.bmp"
        );

    resources.bakeryWork3Atlas =
        loadColorKeyTexture(
            renderer,
            "app0:assets/buildings/bakery/runtime/work3_atlas.bmp"
        );

    resources.bakeryIdleAtlas =
        loadColorKeyTexture(
            renderer,
            "app0:assets/buildings/bakery/runtime/idle_atlas.bmp"
        );

    resources.bakeryFlagpole =
        loadColorKeyTexture(
            renderer,
            "app0:assets/buildings/bakery/runtime/flagpole.bmp"
        );

    resources.bakeryFlag1Atlas =
        loadColorKeyTexture(
            renderer,
            "app0:assets/buildings/bakery/runtime/flag1_atlas.bmp"
        );

    resources.bakerySmokeAtlas =
        loadColorKeyTexture(
            renderer,
            "app0:assets/buildings/bakery/runtime/smoke_atlas.bmp"
        );

    resources.bakeryBreadSupplyAtlas =
        loadColorKeyTexture(
            renderer,
            "app0:assets/buildings/bakery/runtime/bread_supply_atlas.bmp"
        );

    resources.farmWoodAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/construction/farm/wood_atlas.bmp"
    );

    resources.farmStoneAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/construction/farm/stone_atlas.bmp"
    );

    resources.farmIdleAtlas =
        loadColorKeyTexture(
            renderer,
            "app0:assets/buildings/farm/runtime/idle_atlas.bmp"
        );

    resources.farmFlag1Atlas =
        loadColorKeyTexture(
            renderer,
            "app0:assets/buildings/farm/runtime/flag1_atlas.bmp"
        );

    resources.farmCornSupplyAtlas =
        loadColorKeyTexture(
            renderer,
            "app0:assets/buildings/farm/runtime/corn_supply_atlas.bmp"
        );

    resources.school = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/school.bmp"
    );

    resources.schoolFlagpole =
        loadColorKeyTexture(
            renderer,
            "app0:assets/buildings/school/runtime/flagpole/flagpole.bmp"
        );

    resources.schoolFlagAtlas =
        loadColorKeyTexture(
            renderer,
            "app0:assets/buildings/school/runtime/flag_atlas.bmp"
        );

    resources.schoolWorkAtlas =
        loadColorKeyTexture(
            renderer,
            "app0:assets/buildings/school/runtime/work_atlas.bmp"
        );

    resources.schoolIcon = loadColorKeyTexture(
        renderer,
        "app0:assets/ui/buildings/school_icon.bmp"
    );

    resources.schoolWoodAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/construction/school/wood_atlas.bmp"
    );

    resources.schoolStoneAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/construction/school/stone_atlas.bmp"
    );

    resources.quarryWoodAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/construction/quarry/wood_atlas.bmp"
    );

    resources.quarryStoneAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/construction/quarry/stone_atlas.bmp"
    );

    resources.millWoodAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/construction/mill/wood_atlas.bmp"
    );

    resources.millStoneAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/construction/mill/stone_atlas.bmp"
    );

    // main.cpp may replace the completed Mill sources with
    // textures decoded directly from external houses.rx.
    resources.millRuntimeWoodTexture = nullptr;

    for (int step = 0; step < 22; ++step) {
        resources.millRuntimeWoodSteps[step] = nullptr;
    }

    resources.millRuntimeStoneTexture = nullptr;

    resources.millTurbineAtlas =
        loadColorKeyTexture(
            renderer,
            "app0:assets/buildings/mill/runtime/turbine_atlas.bmp"
        );

    resources.millIdleAtlas =
        loadColorKeyTexture(
            renderer,
            "app0:assets/buildings/mill/runtime/idle_atlas.bmp"
        );

    return resources;
}

void destroyBuildingResources(
    BuildingResources* resources
)
{
    if (!resources)
        return;

    SDL_DestroyTexture(resources->store);
    SDL_DestroyTexture(resources->bakery);
    SDL_DestroyTexture(resources->farm);
    SDL_DestroyTexture(resources->school);

    SDL_DestroyTexture(resources->roadIcon);
    SDL_DestroyTexture(resources->storeIcon);
    SDL_DestroyTexture(resources->bakeryIcon);
    SDL_DestroyTexture(resources->farmIcon);
    SDL_DestroyTexture(resources->schoolIcon);

    SDL_DestroyTexture(resources->storeWoodAtlas);
    SDL_DestroyTexture(resources->storeStoneAtlas);

    SDL_DestroyTexture(resources->storeFlag1Atlas);
    SDL_DestroyTexture(resources->storeFlag2Atlas);

    SDL_DestroyTexture(resources->bakeryWoodAtlas);
    SDL_DestroyTexture(resources->bakeryStoneAtlas);

    SDL_DestroyTexture(
        resources->bakeryWork2Atlas
    );

    SDL_DestroyTexture(
        resources->bakeryWork3Atlas
    );

    SDL_DestroyTexture(
        resources->bakeryIdleAtlas
    );

    SDL_DestroyTexture(
        resources->bakeryFlagpole
    );

    SDL_DestroyTexture(
        resources->bakeryFlag1Atlas
    );

    SDL_DestroyTexture(
        resources->bakerySmokeAtlas
    );

    SDL_DestroyTexture(
        resources->bakeryBreadSupplyAtlas
    );

    SDL_DestroyTexture(resources->farmWoodAtlas);
    SDL_DestroyTexture(resources->farmStoneAtlas);

    SDL_DestroyTexture(
        resources->farmIdleAtlas
    );

    SDL_DestroyTexture(
        resources->farmFlag1Atlas
    );

    SDL_DestroyTexture(
        resources->farmCornSupplyAtlas
    );

    SDL_DestroyTexture(resources->schoolWoodAtlas);
    SDL_DestroyTexture(resources->schoolStoneAtlas);

    SDL_DestroyTexture(resources->quarryWoodAtlas);
    SDL_DestroyTexture(resources->quarryStoneAtlas);

    SDL_DestroyTexture(resources->millWoodAtlas);
    SDL_DestroyTexture(resources->millStoneAtlas);

    SDL_DestroyTexture(
        resources->millRuntimeWoodTexture
    );

    for (int step = 0; step < 22; ++step) {
        SDL_DestroyTexture(
            resources->millRuntimeWoodSteps[step]
        );
    }

    SDL_DestroyTexture(
        resources->millRuntimeStoneTexture
    );

    SDL_DestroyTexture(
        resources->millTurbineAtlas
    );

    SDL_DestroyTexture(
        resources->millIdleAtlas
    );

    resources->store = nullptr;
    resources->bakery = nullptr;
    resources->farm = nullptr;
    resources->school = nullptr;

    resources->roadIcon = nullptr;
    resources->storeIcon = nullptr;
    resources->bakeryIcon = nullptr;
    resources->farmIcon = nullptr;
    SDL_DestroyTexture(
        resources->schoolFlagpole
    );

    SDL_DestroyTexture(
        resources->schoolFlagAtlas
    );

    SDL_DestroyTexture(
        resources->schoolWorkAtlas
    );

    resources->schoolFlagpole = nullptr;
    resources->schoolFlagAtlas = nullptr;
    resources->schoolWorkAtlas = nullptr;

    resources->schoolIcon = nullptr;

    resources->storeWoodAtlas = nullptr;
    resources->storeStoneAtlas = nullptr;

    resources->storeFlag1Atlas = nullptr;
    resources->storeFlag2Atlas = nullptr;

    resources->bakeryWoodAtlas = nullptr;
    resources->bakeryStoneAtlas = nullptr;
    resources->bakeryWork2Atlas = nullptr;
    resources->bakeryWork3Atlas = nullptr;
    resources->bakeryIdleAtlas = nullptr;
    resources->bakeryFlagpole = nullptr;
    resources->bakeryFlag1Atlas = nullptr;
    resources->bakerySmokeAtlas = nullptr;
    resources->bakeryBreadSupplyAtlas = nullptr;

    resources->farmWoodAtlas = nullptr;
    resources->farmStoneAtlas = nullptr;
    resources->farmIdleAtlas = nullptr;
    resources->farmFlag1Atlas = nullptr;
    resources->farmCornSupplyAtlas = nullptr;

    resources->schoolWoodAtlas = nullptr;
    resources->schoolStoneAtlas = nullptr;

    resources->quarryWoodAtlas = nullptr;
    resources->quarryStoneAtlas = nullptr;

    resources->millWoodAtlas = nullptr;
    resources->millStoneAtlas = nullptr;
    resources->millRuntimeWoodTexture = nullptr;

    for (int step = 0; step < 22; ++step) {
        resources->millRuntimeWoodSteps[step] = nullptr;
    }

    resources->millRuntimeStoneTexture = nullptr;

    resources->millTurbineAtlas = nullptr;

    resources->millIdleAtlas = nullptr;
}

int getBuildingWidth(BuildingType type)
{
    switch (type) {
        case BuildingType::Store:
            return 3;

        case BuildingType::Bakery:
            return 3;

        case BuildingType::Farm:
            return 4;

        case BuildingType::School:
            return 3;
        case BuildingType::Quarry:
            return 3;
        case BuildingType::Mill:
            return 3;
    }

    return 3;
}

int getBuildingHeight(BuildingType type)
{
    switch (type) {
        case BuildingType::Store:
            return 3;

        case BuildingType::Bakery:
            return 3;

        case BuildingType::Farm:
            return 3;

        case BuildingType::School:
            return 3;
        case BuildingType::Quarry:
            return 2;
        case BuildingType::Mill:
            return 2;
    }

    return 3;
}

int getBuildingPlanValue(
    BuildingType type,
    int localX,
    int localY
)
{
    static const int storePlan[3][3] = {
        {1, 1, 1},
        {1, 1, 1},
        {1, 2, 1}
    };

    static const int bakeryPlan[3][3] = {
        {1, 1, 1},
        {1, 1, 1},
        {1, 2, 1}
    };

    static const int farmPlan[3][4] = {
        {1, 1, 1, 1},
        {1, 1, 1, 1},
        {1, 2, 1, 1}
    };

    static const int schoolPlan[3][3] = {
        {1, 1, 1},
        {1, 1, 1},
        {1, 2, 1}
    };

    // Original Quarry PlanYX normalized from 4x4
    // to its active 3x2 footprint:
    //
    // 1 1 1
    // 1 2 1
    static const int quarryPlan[2][3] = {
        {1, 1, 1},
        {1, 2, 1}
    };

    // Original Mill PlanYX normalized from 4x4
    // to its active 3x2 footprint.
    static const int millPlan[2][3] = {
        {1, 1, 1},
        {1, 2, 1}
    };

    int width = getBuildingWidth(type);
    int height = getBuildingHeight(type);

    if (localX < 0 || localY < 0 ||
        localX >= width || localY >= height) {
        return 0;
    }

    switch (type) {
        case BuildingType::Store:
            return storePlan[localY][localX];

        case BuildingType::Bakery:
            return bakeryPlan[localY][localX];

        case BuildingType::Farm:
            return farmPlan[localY][localX];

        case BuildingType::School:
            return schoolPlan[localY][localX];
        case BuildingType::Quarry:
            return quarryPlan[localY][localX];
        case BuildingType::Mill:
            return millPlan[localY][localX];
    }

    return 0;
}

namespace {


constexpr int RuntimeCostBuildingCount = 6;


int gBuildingRuntimeWoodCosts[
    RuntimeCostBuildingCount
] = {
    0, 0, 0, 0, 0, 0
};


int gBuildingRuntimeStoneCosts[
    RuntimeCostBuildingCount
] = {
    0, 0, 0, 0, 0, 0
};


int gBuildingRuntimeWoodSteps[
    RuntimeCostBuildingCount
] = {
    0, 0, 0, 0, 0, 0
};


int gBuildingRuntimeStoneSteps[
    RuntimeCostBuildingCount
] = {
    0, 0, 0, 0, 0, 0
};


int buildingTypeToRuntimeCostIndex(
    BuildingType type
)
{
    switch (type) {
        case BuildingType::Store:
            return 0;

        case BuildingType::Bakery:
            return 1;

        case BuildingType::Farm:
            return 2;

        case BuildingType::School:
            return 3;

        case BuildingType::Quarry:
            return 4;

        case BuildingType::Mill:
            return 5;
    }

    return -1;
}


} // namespace


void clearBuildingRuntimeCosts()
{
    for (
        int index = 0;
        index < RuntimeCostBuildingCount;
        ++index
    ) {
        gBuildingRuntimeWoodCosts[index] = 0;
        gBuildingRuntimeStoneCosts[index] = 0;
    }
}


bool setBuildingRuntimeCosts(
    BuildingType type,
    int woodCost,
    int stoneCost
)
{
    const int index =
        buildingTypeToRuntimeCostIndex(
            type
        );

    if (
        index < 0 ||
        index >= RuntimeCostBuildingCount ||
        woodCost <= 0 ||
        stoneCost <= 0
    ) {
        return false;
    }

    gBuildingRuntimeWoodCosts[index] =
        woodCost;

    gBuildingRuntimeStoneCosts[index] =
        stoneCost;

    return true;
}


bool hasBuildingRuntimeCosts(
    BuildingType type
)
{
    const int index =
        buildingTypeToRuntimeCostIndex(
            type
        );

    if (
        index < 0 ||
        index >= RuntimeCostBuildingCount
    ) {
        return false;
    }

    return
        gBuildingRuntimeWoodCosts[index] > 0 &&
        gBuildingRuntimeStoneCosts[index] > 0;
}


void clearBuildingRuntimeSteps()
{
    for (
        int index = 0;
        index < RuntimeCostBuildingCount;
        ++index
    ) {
        gBuildingRuntimeWoodSteps[index] = 0;
        gBuildingRuntimeStoneSteps[index] = 0;
    }
}


bool setBuildingRuntimeSteps(
    BuildingType type,
    int woodSteps,
    int stoneSteps
)
{
    const int index =
        buildingTypeToRuntimeCostIndex(
            type
        );

    if (
        index < 0 ||
        index >= RuntimeCostBuildingCount ||
        woodSteps <= 0 ||
        stoneSteps <= 0
    ) {
        return false;
    }

    gBuildingRuntimeWoodSteps[index] =
        woodSteps;

    gBuildingRuntimeStoneSteps[index] =
        stoneSteps;

    return true;
}


bool hasBuildingRuntimeSteps(
    BuildingType type
)
{
    const int index =
        buildingTypeToRuntimeCostIndex(
            type
        );

    if (
        index < 0 ||
        index >= RuntimeCostBuildingCount
    ) {
        return false;
    }

    return
        gBuildingRuntimeWoodSteps[index] > 0 &&
        gBuildingRuntimeStoneSteps[index] > 0;
}


namespace {


int gBuildingRuntimeEntrancePixelX[6] = {
    0, 0, 0, 0, 0, 0
};


int gBuildingRuntimeEntrancePixelY[6] = {
    0, 0, 0, 0, 0, 0
};


bool gBuildingRuntimeEntranceValid[6] = {
    false, false, false,
    false, false, false
};


} // namespace


void clearBuildingRuntimeEntrancePixels()
{
    for (int index = 0; index < 6; ++index) {
        gBuildingRuntimeEntrancePixelX[index] = 0;
        gBuildingRuntimeEntrancePixelY[index] = 0;
        gBuildingRuntimeEntranceValid[index] = false;
    }
}


bool setBuildingRuntimeEntrancePixels(
    BuildingType type,
    int pixelOffsetX,
    int pixelOffsetY
)
{
    const int index =
        static_cast<int>(type);

    if (index < 0 || index >= 6)
        return false;

    gBuildingRuntimeEntrancePixelX[index] =
        pixelOffsetX;

    gBuildingRuntimeEntrancePixelY[index] =
        pixelOffsetY;

    gBuildingRuntimeEntranceValid[index] =
        true;

    return true;
}


bool hasBuildingRuntimeEntrancePixels(
    BuildingType type
)
{
    const int index =
        static_cast<int>(type);

    return
        index >= 0 &&
        index < 6 &&
        gBuildingRuntimeEntranceValid[index];
}


int getBuildingWoodCost(BuildingType type)
{
    const int runtimeIndex =
        buildingTypeToRuntimeCostIndex(
            type
        );

    if (
        runtimeIndex >= 0 &&
        runtimeIndex <
            RuntimeCostBuildingCount &&
        gBuildingRuntimeWoodCosts[
            runtimeIndex
        ] > 0
    ) {
        return
            gBuildingRuntimeWoodCosts[
                runtimeIndex
            ];
    }

    // Stable fallback used if external houses.dat
    // is missing or its record was rejected.
    switch (type) {
        case BuildingType::Store:
            return 6;

        case BuildingType::Bakery:
            return 4;

        case BuildingType::Farm:
            return 4;

        case BuildingType::School:
            return 6;
        case BuildingType::Quarry:
            return 3;
        case BuildingType::Mill:
            return 4;
    }

    return 0;
}

int getBuildingStoneCost(BuildingType type)
{
    const int runtimeIndex =
        buildingTypeToRuntimeCostIndex(
            type
        );

    if (
        runtimeIndex >= 0 &&
        runtimeIndex <
            RuntimeCostBuildingCount &&
        gBuildingRuntimeStoneCosts[
            runtimeIndex
        ] > 0
    ) {
        return
            gBuildingRuntimeStoneCosts[
                runtimeIndex
            ];
    }

    // Stable fallback used if external houses.dat
    // is missing or its record was rejected.
    switch (type) {
        case BuildingType::Store:
            return 5;

        case BuildingType::Bakery:
            return 3;

        case BuildingType::Farm:
            return 3;

        case BuildingType::School:
            return 5;
        case BuildingType::Quarry:
            return 2;
        case BuildingType::Mill:
            return 3;
    }

    return 0;
}

int getBuildingWoodSteps(BuildingType type)
{
    const int runtimeIndex =
        buildingTypeToRuntimeCostIndex(
            type
        );

    if (
        runtimeIndex >= 0 &&
        runtimeIndex <
            RuntimeCostBuildingCount &&
        gBuildingRuntimeWoodSteps[
            runtimeIndex
        ] > 0
    ) {
        return
            gBuildingRuntimeWoodSteps[
                runtimeIndex
            ];
    }

    // Stable fallback for missing external data.
    switch (type) {
        case BuildingType::Store:
            return 24;

        case BuildingType::Bakery:
            return 21;

        case BuildingType::Farm:
            return 26;

        case BuildingType::School:
            return 21;
        case BuildingType::Quarry:
            return 17;
        case BuildingType::Mill:
            return 21;
    }

    return 0;
}

int getBuildingStoneSteps(BuildingType type)
{
    const int runtimeIndex =
        buildingTypeToRuntimeCostIndex(
            type
        );

    if (
        runtimeIndex >= 0 &&
        runtimeIndex <
            RuntimeCostBuildingCount &&
        gBuildingRuntimeStoneSteps[
            runtimeIndex
        ] > 0
    ) {
        return
            gBuildingRuntimeStoneSteps[
                runtimeIndex
            ];
    }

    // Stable fallback for missing external data.
    switch (type) {
        case BuildingType::Store:
            return 75;

        case BuildingType::Bakery:
            return 54;

        case BuildingType::Farm:
            return 43;

        case BuildingType::School:
            return 65;
        case BuildingType::Quarry:
            return 18;
        case BuildingType::Mill:
            return 31;
    }

    return 0;
}

bool isBuildingComplete(const Building& building)
{
    return
        building.woodStep >=
            getBuildingWoodSteps(building.type) &&
        building.stoneStep >=
            getBuildingStoneSteps(building.type);
}

BuildingEntrance getBuildingEntrance(
    BuildingType type
)
{
    BuildingEntrance result = {};

    switch (type) {
        case BuildingType::School:
            result = {
                1,
                2,
                17,
                10
            };
            break;

        case BuildingType::Store:
            result = {
                1,
                2,
                19,
                7
            };
            break;

        case BuildingType::Bakery:
            // Originalni houses.dat:
            // EntranceOffsetPx = (17, 12)
            //
            // Aktivni KaMVita footprint entrance zustava
            // na local (2,2). Pixelovy offset nyni odpovida
            // skutecnemu Bakery vstupu v originalnim sprite.
            result = {
                2,
                2,
                17,
                12
            };
            break;

        case BuildingType::Farm:
            // Originalni houses.dat:
            // EntranceOffset = (-1, 0)
            // EntranceOffsetPx = (9, 4)
            //
            // Aktivni KaMVita footprint entrance zustava
            // na local (1,2). Pixelovy offset odpovida
            // skutecnemu Farm vstupu v originalnim sprite.
            result = {
                1,
                2,
                9,
                4
            };
            break;

        case BuildingType::Quarry:
            // Original houses.dat:
            // EntranceOffset   = (0, 0)
            // EntranceOffsetPx = (26, 10)
            //
            // Original PlanYX entrance (2,3)
            // normalized to active 3x2 footprint:
            // local entrance (1,1).
            result = {
                1,
                1,
                26,
                10
            };
            break;

        case BuildingType::Mill:
            // Original houses.dat record 22:
            // EntranceOffset   = (0, 0)
            // EntranceOffsetPx = (16, 15)
            //
            // Original PlanYX entrance (2,3)
            // normalized to active 3x2 footprint:
            // local entrance (1,1).
            result = {
                1,
                1,
                16,
                15
            };
            break;

        default:
            result = {
                0,
                0,
                0,
                0
            };
            break;
    }

    const int runtimeIndex =
        static_cast<int>(type);

    if (
        runtimeIndex >= 0 &&
        runtimeIndex < 6 &&
        gBuildingRuntimeEntranceValid[
            runtimeIndex
        ]
    ) {
        result.pixelOffsetX =
            gBuildingRuntimeEntrancePixelX[
                runtimeIndex
            ];

        result.pixelOffsetY =
            gBuildingRuntimeEntrancePixelY[
                runtimeIndex
            ];
    }

    return result;
}

// ============================================================
// STORE WARE API
// ============================================================

bool isValidWareType(
    WareType ware
)
{
    const int wareIndex =
        wareTypeToIndex(ware);

    return
        wareIndex >= 0 &&
        wareIndex < STORE_WARE_COUNT;
}


static bool isUsableStore(
    const Building& store
)
{
    return
        store.active &&
        store.type == BuildingType::Store &&
        isBuildingComplete(store);
}


int getStoreWareAmount(
    const Building& store,
    WareType ware
)
{
    if (
        !isUsableStore(store) ||
        !isValidWareType(ware)
    ) {
        return 0;
    }

    const int wareIndex =
        wareTypeToIndex(ware);

    const int amount =
        store.storeWares[wareIndex];

    return amount > 0
        ? amount
        : 0;
}


bool canStoreAcceptWare(
    const Building& store,
    WareType ware,
    int amount
)
{
    if (
        !isUsableStore(store) ||
        !isValidWareType(ware) ||
        amount <= 0
    ) {
        return false;
    }

    if (!store.wareDeliveryEnabled)
        return false;

    const int wareIndex =
        wareTypeToIndex(ware);

    if (store.storeNotAccept[wareIndex])
        return false;

    const int currentAmount =
        getStoreWareAmount(
            store,
            ware
        );

    // ---------------------------------------------
    // KaMVita Store ware capacity.
    //
    // Stop transport before this individual ware stack
    // exceeds the Store GUI capacity.
    // ---------------------------------------------

    if (
        currentAmount >=
            STORE_WARE_CAPACITY
    ) {
        return false;
    }

    return amount <=
        STORE_WARE_CAPACITY -
            currentAmount;
}


bool addStoreWare(
    Building* store,
    WareType ware,
    int amount
)
{
    if (
        !store ||
        !canStoreAcceptWare(
            *store,
            ware,
            amount
        )
    ) {
        return false;
    }

    const int wareIndex =
        wareTypeToIndex(ware);

    store->storeWares[wareIndex] +=
        amount;

    return true;
}


bool canStoreTakeWare(
    const Building& store,
    WareType ware,
    int amount
)
{
    if (
        !isUsableStore(store) ||
        !isValidWareType(ware) ||
        amount <= 0
    ) {
        return false;
    }

    return
        getStoreWareAmount(
            store,
            ware
        ) >= amount;
}


bool takeStoreWare(
    Building* store,
    WareType ware,
    int amount
)
{
    if (
        !store ||
        !canStoreTakeWare(
            *store,
            ware,
            amount
        )
    ) {
        return false;
    }

    const int wareIndex =
        wareTypeToIndex(ware);

    store->storeWares[wareIndex] -=
        amount;

    return true;
}
