#include "units.h"
#include "worker_action_atlas_config.h"
#include "worker_complete_atlas_config.h"
#include <cmath>
#include <vector>
#include <queue>
#include <algorithm>

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

UnitResources loadUnitResources(
    SDL_Renderer* renderer
)
{
    UnitResources resources = {};

    resources.workerIdle = loadColorKeyTexture(
        renderer,
        "app0:assets/units/worker/idle_s.bmp"
    );

    resources.workerWalkAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/units/worker/complete/actions/walk_atlas.bmp"
    );

    resources.workerWorkAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/units/worker/complete/actions/work_atlas.bmp"
    );

    resources.workerDigAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/units/worker/complete/actions/work1_atlas.bmp"
    );

    resources.workerPaveAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/units/worker/complete/actions/work2_atlas.bmp"
    );

    resources.serfWalkAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/units/serf/walk_atlas.bmp"
    );

    resources.serfCarryStoneAtlas =
        loadColorKeyTexture(
            renderer,
            "app0:assets/units/serf/carry_stone_atlas.bmp"
        );

    resources.serfCarryGoldAtlas =
        loadColorKeyTexture(
            renderer,
            "app0:assets/units/serf/carry_gold_atlas.bmp"
        );

    resources.serfCarryFlourAtlas =
        loadColorKeyTexture(
            renderer,
            "app0:assets/units/serf/carry_flour_atlas.bmp"
        );

    return resources;
}

void destroyUnitResources(
    UnitResources* resources
)
{
    if (!resources)
        return;

    SDL_DestroyTexture(resources->workerIdle);
    SDL_DestroyTexture(resources->workerWalkAtlas);
    SDL_DestroyTexture(resources->workerWorkAtlas);
    SDL_DestroyTexture(resources->workerDigAtlas);
    SDL_DestroyTexture(resources->workerPaveAtlas);
    SDL_DestroyTexture(resources->serfWalkAtlas);
    SDL_DestroyTexture(resources->serfCarryStoneAtlas);
    SDL_DestroyTexture(resources->serfCarryGoldAtlas);
    SDL_DestroyTexture(resources->serfCarryFlourAtlas);

    resources->workerIdle = nullptr;
    resources->workerWalkAtlas = nullptr;
    resources->workerWorkAtlas = nullptr;
    resources->workerDigAtlas = nullptr;
    resources->workerPaveAtlas = nullptr;
    resources->serfWalkAtlas = nullptr;
    resources->serfCarryStoneAtlas = nullptr;
    resources->serfCarryGoldAtlas = nullptr;
    resources->serfCarryFlourAtlas = nullptr;
}

void renderUnits(
    SDL_Renderer* renderer,
    const UnitResources& resources,
    const Unit* units,
    int unitCount,
    float cameraX,
    float cameraY
)
{
    if (!resources.workerWalkAtlas)
        return;

    const int cellWidth =
        WORKER_WALK_CELL_WIDTH;

    const int cellHeight =
        WORKER_WALK_CELL_HEIGHT;

    const int anchorX =
        WORKER_WALK_ANCHOR_X;

    const int anchorY =
        WORKER_WALK_ANCHOR_Y;

    // Originalni UnitStillFrames pro:
    // N, NE, E, SE, S, SW, W, NW
    static const int stillFrames[8] = {
        3, 2, 2, 1, 6, 7, 6, 6
    };

    for (int i = 0; i < unitCount; ++i) {
        if (!units[i].active)
            continue;

        if (!units[i].visible)
            continue;

        int direction = units[i].direction;

        if (direction < 0)
            direction = 0;

        if (direction > 7)
            direction = 7;

        if (
            units[i].type == UnitType::Worker &&
            units[i].state == UnitState::Working
        ) {
            SDL_Texture* actionAtlas = nullptr;

            int actionCellWidth = 0;
            int actionCellHeight = 0;
            int actionFrameCount = 0;
            int actionAnchorX = 0;
            int actionAnchorY = 0;

            if (
                units[i].workAction ==
                    UnitWorkAction::DigRoad ||
                units[i].workAction ==
                    UnitWorkAction::BuildHouse ||
                units[i].workAction ==
                    UnitWorkAction::BuildField ||
                units[i].workAction ==
                    UnitWorkAction::BuildWine
            ) {
                actionAtlas =
                    resources.workerDigAtlas;

                actionCellWidth =
                    WORKER_WORK1_CELL_WIDTH;

                actionCellHeight =
                    WORKER_WORK1_CELL_HEIGHT;

                actionFrameCount =
                    WORKER_WORK1_FRAME_COUNT;

                actionAnchorX =
                    WORKER_WORK1_ANCHOR_X;

                actionAnchorY =
                    WORKER_WORK1_ANCHOR_Y;
            } else if (
                units[i].workAction ==
                    UnitWorkAction::PaveRoad ||
                units[i].workAction ==
                    UnitWorkAction::BuildWineWood
            ) {
                actionAtlas =
                    resources.workerPaveAtlas;

                actionCellWidth =
                    WORKER_WORK2_CELL_WIDTH;

                actionCellHeight =
                    WORKER_WORK2_CELL_HEIGHT;

                actionFrameCount =
                    WORKER_WORK2_FRAME_COUNT;

                actionAnchorX =
                    WORKER_WORK2_ANCHOR_X;

                actionAnchorY =
                    WORKER_WORK2_ANCHOR_Y;
            } else {
                // Stavba budov zustava na kladivu.
                actionAtlas =
                    resources.workerWorkAtlas;

                actionCellWidth =
                    WORKER_WORK_CELL_WIDTH;

                actionCellHeight =
                    WORKER_WORK_CELL_HEIGHT;

                actionFrameCount =
                    WORKER_WORK_FRAME_COUNT;

                actionAnchorX =
                    WORKER_WORK_ANCHOR_X;

                actionAnchorY =
                    WORKER_WORK_ANCHOR_Y;
            }

            if (actionAtlas &&
                actionFrameCount > 0) {

                int actionFrame =
                    units[i].animationFrame %
                    actionFrameCount;

                SDL_Rect actionSource = {
                    actionFrame * actionCellWidth,
                    direction * actionCellHeight,
                    actionCellWidth,
                    actionCellHeight
                };

                // Working muze mit samostatny pouze
                // renderovaci offset.
                //
                // BuildHouse jej pouziva pro Work1 uvnitr
                // footprintu, zatimco logicka pozice nohou
                // zustava na pruchozi entrance dlazdici.
                const bool isHousePreparation =
                    units[i].workAction ==
                        UnitWorkAction::BuildHouse;

                const float workingProgress =
                    isHousePreparation
                        ? units[i].doorwayProgress
                        : 0.0f;

                const int workingOffsetX =
                    isHousePreparation
                        ? static_cast<int>(
                            units[i].doorwayOffsetX *
                            workingProgress
                        )
                        : 0;

                const int workingOffsetY =
                    isHousePreparation
                        ? static_cast<int>(
                            units[i].doorwayOffsetY *
                            workingProgress
                        )
                        : 0;

                SDL_Rect actionDestination = {
                    (int)units[i].worldX -
                        (int)cameraX +
                        actionAnchorX +
                        workingOffsetX,

                    (int)units[i].worldY -
                        (int)cameraY +
                        actionAnchorY +
                        workingOffsetY,

                    actionCellWidth,
                    actionCellHeight
                };

                SDL_RenderCopy(
                    renderer,
                    actionAtlas,
                    &actionSource,
                    &actionDestination
                );

                continue;
            }
        }

        bool isSerf =
            units[i].type == UnitType::Serf;

        SDL_Texture* walkAtlas =
            isSerf
                ? resources.serfWalkAtlas
                : resources.workerWalkAtlas;

        if (!walkAtlas)
            continue;

        int walkCellWidth =
            isSerf ? 43 : 56;

        int walkCellHeight =
            isSerf ? 47 : 50;

        int walkAnchorX =
            isSerf ? -18 : -21;

        int walkAnchorY =
            isSerf ? -38 : -42;

        int frame = 0;

        if (units[i].state == UnitState::Walking) {
            frame =
                units[i].animationFrame % 8;

            if (frame < 0)
                frame += 8;
        } else if (!isSerf) {
            frame = stillFrames[direction];
        }

        SDL_Rect source = {
            frame * walkCellWidth,
            direction * walkCellHeight,
            walkCellWidth,
            walkCellHeight
        };

        // Doorway pixel alignment is now represented by
        // the actual world-space approach and door points.
        // No additional render-only shift is needed.
        int doorwaySlideX = 0;
        int doorwaySlideY = 0;

        SDL_Rect destination = {
            (int)units[i].worldX -
                (int)cameraX +
                walkAnchorX +
                doorwaySlideX,

            (int)units[i].worldY -
                (int)cameraY +
                walkAnchorY +
                doorwaySlideY,

            walkCellWidth,
            walkCellHeight
        };

        SDL_RenderCopy(
            renderer,
            walkAtlas,
            &source,
            &destination
        );

        // =================================================
        // ORIGINAL KaM SERF GOLD CARRY LAYER
        //
        // Remake:
        // zakladni Serf animace + SerfCarry[Ware,Direction].
        //
        // Gold atlas byl vytvoren primo z:
        // unit.dat WareTypeToIndex[wt_Gold] = 7
        // a originalniho units.rx.
        //
        // Atlas:
        // cell 44x42
        // anchor -22,-45
        // 8 smeru x 8 kroku.
        // =================================================

        if (
            isSerf &&
            units[i].carriedWare ==
                WareType::Gold &&
            resources.serfCarryGoldAtlas
        ) {
            constexpr int goldCellWidth = 44;
            constexpr int goldCellHeight = 42;

            constexpr int goldAnchorX = -22;
            constexpr int goldAnchorY = -45;

            int goldFrame =
                units[i].animationFrame % 8;

            if (goldFrame < 0)
                goldFrame += 8;

            SDL_Rect goldSource = {
                goldFrame * goldCellWidth,
                direction * goldCellHeight,
                goldCellWidth,
                goldCellHeight
            };

            SDL_Rect goldDestination = {
                (int)units[i].worldX -
                    (int)cameraX +
                    goldAnchorX +
                    doorwaySlideX,

                (int)units[i].worldY -
                    (int)cameraY +
                    goldAnchorY +
                    doorwaySlideY,

                goldCellWidth,
                goldCellHeight
            };

            SDL_RenderCopy(
                renderer,
                resources.serfCarryGoldAtlas,
                &goldSource,
                &goldDestination
            );
        }

        // =================================================
        // ORIGINAL KaM SERF FLOUR CARRY LAYER
        //
        // unit.dat WareTypeToIndex[wt_Flour] = 11
        //
        // Atlas:
        // cell 40x28
        // anchor -20,-39
        // 8 smeru x 8 kroku.
        // =================================================
        if (
            isSerf &&
            units[i].carriedWare ==
                WareType::Flour &&
            resources.serfCarryFlourAtlas
        ) {
            constexpr int flourCellWidth = 40;
            constexpr int flourCellHeight = 28;

            constexpr int flourAnchorX = -20;
            constexpr int flourAnchorY = -39;

            int flourFrame =
                units[i].animationFrame % 8;

            if (flourFrame < 0)
                flourFrame += 8;

            SDL_Rect flourSource = {
                flourFrame * flourCellWidth,
                direction * flourCellHeight,
                flourCellWidth,
                flourCellHeight
            };

            SDL_Rect flourDestination = {
                (int)units[i].worldX -
                    (int)cameraX +
                    flourAnchorX +
                    doorwaySlideX,

                (int)units[i].worldY -
                    (int)cameraY +
                    flourAnchorY +
                    doorwaySlideY,

                flourCellWidth,
                flourCellHeight
            };

            SDL_RenderCopy(
                renderer,
                resources.serfCarryFlourAtlas,
                &flourSource,
                &flourDestination
            );
        }
    }
}


static bool isBuildingTileBlocked(
    int tileX,
    int tileY,
    const Building* buildings,
    int buildingCount
)
{
    for (int i = 0; i < buildingCount; ++i) {
        if (!buildings[i].active)
            continue;

        const Building& building =
            buildings[i];

        int localX =
            tileX - building.tileX;

        int localY =
            tileY - building.tileY;

        if (localX < 0 || localY < 0)
            continue;

        int width =
            getBuildingWidth(building.type);

        int height =
            getBuildingHeight(building.type);

        if (localX >= width ||
            localY >= height) {
            continue;
        }

        if (getBuildingPlanValue(
                building.type,
                localX,
                localY
            ) != 0) {
            return true;
        }
    }

    return false;
}

// ============================================================
// SERF TRANSPORT ASSIGNMENT API
// ============================================================

// ============================================================
// DESTINATION WARE DEMAND
//
// Prvni funkcni rez:
// - Store prijima ware pres existujici Store Ware API
// - School prijima Gold do fyzicke zasoby schoolGold
//
// SCHOOL_GOLD_CAPACITY odpovida soucasne sestimistne
// School training fronte.
// ============================================================

static constexpr int SCHOOL_GOLD_CAPACITY = 6;


static bool canBuildingAcceptDeliveredWare(
    const Building& destination,
    WareType ware
)
{
    if (!destination.active)
        return false;

    if (!isBuildingComplete(destination))
        return false;

    if (!destination.wareDeliveryEnabled)
        return false;

    if (!isValidWareType(ware))
        return false;

    if (destination.type == BuildingType::Store) {
        return canStoreAcceptWare(
            destination,
            ware,
            1
        );
    }

    if (
        destination.type == BuildingType::School &&
        ware == WareType::Gold
    ) {
        return
            destination.schoolGold >= 0 &&
            destination.schoolGold <
                SCHOOL_GOLD_CAPACITY;
    }

    if (
        destination.type == BuildingType::Bakery &&
        ware == WareType::Flour
    ) {
        return canHouseAcceptWare(
            destination,
            ware,
            1
        );
    }

    return false;
}


static bool deliverWareToBuilding(
    Building* destination,
    WareType ware
)
{
    if (!destination)
        return false;

    // Bakery Flour ma kapacitu rezervovanou od okamziku
    // dispatch assignmentu. Delivery tuto rezervaci
    // autoritativne prevede na resourceIn[0].
    if (
        destination->type == BuildingType::Bakery &&
        ware == WareType::Flour
    ) {
        return deliverHouseWare(
            destination,
            ware,
            1
        );
    }

    if (
        !canBuildingAcceptDeliveredWare(
            *destination,
            ware
        )
    ) {
        return false;
    }

    if (destination->type == BuildingType::Store) {
        return addStoreWare(
            destination,
            ware,
            1
        );
    }

    if (
        destination->type == BuildingType::School &&
        ware == WareType::Gold
    ) {
        ++destination->schoolGold;
        return true;
    }

    return false;
}


bool canAssignSerfTransport(
    const Unit& serf,
    WareType ware,
    int sourceBuilding,
    int destinationBuilding,
    const Building* buildings,
    int buildingCount
)
{
    if (!serf.active)
        return false;

    if (serf.type != UnitType::Serf)
        return false;

    // Serf musi byt fyzicky volny.
    if (serf.state != UnitState::Idle)
        return false;

    // A zaroven bez aktivni logisticke ulohy.
    if (
        serf.logisticsState !=
            SerfLogisticsState::Idle
    ) {
        return false;
    }

    // Nesmime prepsat fyzicky neseny naklad.
    if (serf.carriedWare != WareType::Count)
        return false;

    if (!isValidWareType(ware))
        return false;

    if (!buildings || buildingCount <= 0)
        return false;

    if (
        sourceBuilding < 0 ||
        sourceBuilding >= buildingCount ||
        destinationBuilding < 0 ||
        destinationBuilding >= buildingCount
    ) {
        return false;
    }

    if (sourceBuilding == destinationBuilding)
        return false;

    const Building& source =
        buildings[sourceBuilding];

    const Building& destination =
        buildings[destinationBuilding];

    // Zdroj transportni ulohy musi byt funkcni Store.
    if (
        !source.active ||
        source.type != BuildingType::Store ||
        !isBuildingComplete(source)
    ) {
        return false;
    }

    // Ware se zatim pouze overi.
    // Pri assignmentu se ze Store NIC neodebira.
    if (
        !canStoreTakeWare(
            source,
            ware,
            1
        )
    ) {
        return false;
    }

    // Assignment smi vzniknout pouze pro cil,
    // ktery tento konkretni ware prave prijima.
    //
    // Jde o prvni demand kontrolu. Stejna kontrola se
    // zopakuje pred cestou a autoritativne pri delivery.
    if (
        !canBuildingAcceptDeliveredWare(
            destination,
            ware
        )
    ) {
        return false;
    }

    return true;
}


bool assignSerfTransport(
    Unit* serf,
    WareType ware,
    int sourceBuilding,
    int destinationBuilding,
    const Building* buildings,
    int buildingCount
)
{
    if (!serf)
        return false;

    if (
        !canAssignSerfTransport(
            *serf,
            ware,
            sourceBuilding,
            destinationBuilding,
            buildings,
            buildingCount
        )
    ) {
        return false;
    }

    serf->logisticsState =
        SerfLogisticsState::Assigned;

    serf->assignedWare = ware;

    // Assignment neni pickup.
    serf->carriedWare =
        WareType::Count;

    serf->sourceBuilding =
        sourceBuilding;

    serf->destinationBuilding =
        destinationBuilding;

    return true;
}


void cancelSerfTransport(
    Unit* serf
)
{
    if (!serf)
        return;

    // Tato funkce je zatim urcena jen pro ulohu,
    // ktera jeste nema fyzicky vyzvednuty ware.
    // Nesenou surovinu nesmime potichu zahodit.
    if (serf->carriedWare != WareType::Count)
        return;

    serf->logisticsState =
        SerfLogisticsState::Idle;

    serf->assignedWare =
        WareType::Count;

    serf->sourceBuilding = -1;
    serf->destinationBuilding = -1;
}


bool startSerfTransportToSource(
    Unit* serf,
    const Building* buildings,
    int buildingCount,
    int mapWidth,
    int mapHeight,
    int tileSize
)
{
    if (!serf || !serf->active)
        return false;

    if (serf->type != UnitType::Serf)
        return false;

    if (
        serf->logisticsState !=
            SerfLogisticsState::Assigned
    ) {
        return false;
    }

    // Assignment musi obsahovat ware,
    // ale Serf jej jeste fyzicky nenese.
    if (!isValidWareType(serf->assignedWare))
        return false;

    if (serf->carriedWare != WareType::Count)
        return false;

    if (!buildings || buildingCount <= 0)
        return false;

    if (
        serf->sourceBuilding < 0 ||
        serf->sourceBuilding >= buildingCount
    ) {
        return false;
    }

    const Building& source =
        buildings[serf->sourceBuilding];

    if (
        !source.active ||
        source.type != BuildingType::Store ||
        !isBuildingComplete(source)
    ) {
        return false;
    }

    // Ware se mohl mezi assignmentem a startem cesty
    // mezitim spotrebovat jinou ulohou.
    if (
        !canStoreTakeWare(
            source,
            serf->assignedWare,
            1
        )
    ) {
        return false;
    }

    BuildingEntrance entrance =
        getBuildingEntrance(source.type);

    // BuildingEntrance ukazuje na vstupni tile uvnitr
    // footprintu. Carrier musi stat na pruchozi dlazdici
    // bezprostredne pred vstupem.
    //
    // Soucasne KaMVita budovy maji vstup ve spodni hrane,
    // proto je pristupovy tile o 1 radek jizne.
    const int entranceTileX =
        source.tileX +
        entrance.tileOffsetX;

    const int entranceTileY =
        source.tileY +
        entrance.tileOffsetY;

    const int approachTileX =
        entranceTileX;

    const int approachTileY =
        entranceTileY + 1;

    if (
        approachTileX < 0 ||
        approachTileX >= mapWidth ||
        approachTileY < 0 ||
        approachTileY >= mapHeight ||
        tileSize <= 0
    ) {
        return false;
    }

    const float targetWorldX =
        (
            approachTileX +
            0.5f
        ) * tileSize;

    const float targetWorldY =
        (
            approachTileY +
            0.5f
        ) * tileSize;

    // Nejdříve musi uspet pathfinding.
    // Teprve potom zmenime logisticsState.
    if (
        !setUnitMoveTarget(
            serf,
            targetWorldX,
            targetWorldY,
            buildings,
            buildingCount,
            mapWidth,
            mapHeight,
            tileSize
        )
    ) {
        return false;
    }

    serf->logisticsState =
        SerfLogisticsState::ToSource;

    return true;
}


bool startSerfTransportToDestination(
    Unit* serf,
    const Building* buildings,
    int buildingCount,
    int mapWidth,
    int mapHeight,
    int tileSize
)
{
    if (!serf || !serf->active)
        return false;

    if (serf->type != UnitType::Serf)
        return false;

    // Tento krok smi nasledovat pouze po uspesnem
    // fyzickem pickup ze zdrojoveho Store.
    if (
        serf->logisticsState !=
            SerfLogisticsState::Carrying
    ) {
        return false;
    }

    if (!isValidWareType(serf->assignedWare))
        return false;

    if (!isValidWareType(serf->carriedWare))
        return false;

    // Assignment a fyzicky naklad musi odpovidat.
    if (serf->assignedWare != serf->carriedWare)
        return false;

    if (!buildings || buildingCount <= 0)
        return false;

    if (
        serf->destinationBuilding < 0 ||
        serf->destinationBuilding >=
            buildingCount
    ) {
        return false;
    }

    const Building& destination =
        buildings[
            serf->destinationBuilding
        ];

    if (
        !canBuildingAcceptDeliveredWare(
            destination,
            serf->carriedWare
        )
    ) {
        return false;
    }

    BuildingEntrance entrance =
        getBuildingEntrance(
            destination.type
        );

    const int entranceTileX =
        destination.tileX +
        entrance.tileOffsetX;

    const int entranceTileY =
        destination.tileY +
        entrance.tileOffsetY;

    // Soucasne implementovane KaMVita House entrance
    // jsou na spodni hrane footprintu.
    // Carrier proto cili na volny tile primo pod vstupem.
    const int approachTileX =
        entranceTileX;

    const int approachTileY =
        entranceTileY + 1;

    if (
        approachTileX < 0 ||
        approachTileX >= mapWidth ||
        approachTileY < 0 ||
        approachTileY >= mapHeight ||
        tileSize <= 0
    ) {
        return false;
    }

    const float targetWorldX =
        (
            approachTileX +
            0.5f
        ) * tileSize;

    const float targetWorldY =
        (
            approachTileY +
            0.5f
        ) * tileSize;

    // Kriticke pravidlo:
    // logisticsState prepneme az PO uspesnem
    // pathfindingu. Pri neuspechu zustava Carrying
    // a Serf stale fyzicky vlastni ware.
    if (
        !setUnitMoveTarget(
            serf,
            targetWorldX,
            targetWorldY,
            buildings,
            buildingCount,
            mapWidth,
            mapHeight,
            tileSize
        )
    ) {
        return false;
    }

    serf->logisticsState =
        SerfLogisticsState::ToDestination;

    return true;
}


bool setUnitMoveTarget(
    Unit* unit,
    float targetWorldX,
    float targetWorldY,
    const Building* buildings,
    int buildingCount,
    int mapWidth,
    int mapHeight,
    int tileSize
)
{
    if (!unit || !unit->active)
        return false;

    if (mapWidth <= 0 ||
        mapHeight <= 0 ||
        tileSize <= 0) {
        return false;
    }

    int startX =
        (int)(unit->worldX / tileSize);

    int startY =
        (int)(unit->worldY / tileSize);

    int goalX =
        (int)(targetWorldX / tileSize);

    int goalY =
        (int)(targetWorldY / tileSize);

    if (startX < 0 || startX >= mapWidth ||
        startY < 0 || startY >= mapHeight ||
        goalX < 0 || goalX >= mapWidth ||
        goalY < 0 || goalY >= mapHeight) {
        return false;
    }

    if (isBuildingTileBlocked(
            goalX,
            goalY,
            buildings,
            buildingCount
        )) {
        return false;
    }

    // =====================================================
    // DIRECT STRAIGHT ROUTE
    //
    // Pokud jsou start a cil na stejne rade a vsechny
    // mezilehle dlazdice jsou pruchozi, vytvorime primo
    // vodorovnou cestu.
    //
    // To odpovida soucasnemu development layoutu:
    // Store PointBelowEntrance  -> School PointBelowEntrance
    // a zpet.
    //
    // BFS zustava fallbackem pro obecne trasy.
    // =====================================================

    if (
        startY == goalY &&
        startX != goalX
    ) {
        const int stepX =
            goalX > startX
                ? 1
                : -1;

        bool straightRouteClear = true;
        int straightLength = 0;

        for (
            int tileX = startX + stepX;
            ;
            tileX += stepX
        ) {
            if (
                isBuildingTileBlocked(
                    tileX,
                    startY,
                    buildings,
                    buildingCount
                )
            ) {
                straightRouteClear = false;
                break;
            }

            ++straightLength;

            if (tileX == goalX)
                break;
        }

        if (
            straightRouteClear &&
            straightLength > 0 &&
            straightLength <= MAX_UNIT_PATH
        ) {
            unit->pathLength =
                straightLength;

            unit->pathIndex = 0;

            for (
                int pathIndex = 0;
                pathIndex < straightLength;
                ++pathIndex
            ) {
                unit->pathX[pathIndex] =
                    startX +
                    stepX *
                        (pathIndex + 1);

                unit->pathY[pathIndex] =
                    startY;
            }

            unit->targetWorldX =
                (
                    unit->pathX[0] +
                    0.5f
                ) * tileSize;

            unit->targetWorldY =
                (
                    unit->pathY[0] +
                    0.5f
                ) * tileSize;

            unit->state =
                UnitState::Walking;

            unit->animationFrame = 0;
            unit->animationTimer = 0.0f;

            return true;
        }
    }

    const int nodeCount =
        mapWidth * mapHeight;

    std::vector<int> parent(
        nodeCount,
        -1
    );

    std::vector<unsigned char> visited(
        nodeCount,
        0
    );

    std::queue<int> open;

    int startNode =
        startY * mapWidth + startX;

    int goalNode =
        goalY * mapWidth + goalX;

    visited[startNode] = 1;
    open.push(startNode);

    static const int offsets[8][2] = {
        { 0, -1},
        { 1, -1},
        { 1,  0},
        { 1,  1},
        { 0,  1},
        {-1,  1},
        {-1,  0},
        {-1, -1}
    };

    while (!open.empty() &&
           !visited[goalNode]) {

        int current = open.front();
        open.pop();

        int currentX =
            current % mapWidth;

        int currentY =
            current / mapWidth;

        // Preferujeme sousedy smerujici primo k cili.
        //
        // BFS stale garantuje nejmensi pocet dlazdic,
        // ale mezi vice stejne dlouhymi cestami uz
        // nepreferuje sever jen kvuli poradi offsets[].
        //
        // To odstranuje zbytecna kolecka mezi body
        // PointBelowEntrance sousednich budov.
        int directionOrder[8] = {
            0, 1, 2, 3, 4, 5, 6, 7
        };

        for (
            int orderIndex = 0;
            orderIndex < 7;
            ++orderIndex
        ) {
            int bestIndex = orderIndex;

            int bestDirection =
                directionOrder[bestIndex];

            int bestX =
                currentX +
                offsets[bestDirection][0];

            int bestY =
                currentY +
                offsets[bestDirection][1];

            int bestDx = goalX - bestX;
            int bestDy = goalY - bestY;

            int bestDistance =
                std::max(
                    std::abs(bestDx),
                    std::abs(bestDy)
                );

            for (
                int candidateIndex =
                    orderIndex + 1;
                candidateIndex < 8;
                ++candidateIndex
            ) {
                int candidateDirection =
                    directionOrder[
                        candidateIndex
                    ];

                int candidateX =
                    currentX +
                    offsets[
                        candidateDirection
                    ][0];

                int candidateY =
                    currentY +
                    offsets[
                        candidateDirection
                    ][1];

                int candidateDx =
                    goalX - candidateX;

                int candidateDy =
                    goalY - candidateY;

                int candidateDistance =
                    std::max(
                        std::abs(candidateDx),
                        std::abs(candidateDy)
                    );

                if (
                    candidateDistance <
                        bestDistance
                ) {
                    bestIndex =
                        candidateIndex;

                    bestDistance =
                        candidateDistance;
                }
            }

            std::swap(
                directionOrder[orderIndex],
                directionOrder[bestIndex]
            );
        }

        for (
            int orderIndex = 0;
            orderIndex < 8;
            ++orderIndex
        ) {
            int direction =
                directionOrder[orderIndex];

            int nextX =
                currentX +
                offsets[direction][0];

            int nextY =
                currentY +
                offsets[direction][1];

            if (nextX < 0 ||
                nextX >= mapWidth ||
                nextY < 0 ||
                nextY >= mapHeight) {
                continue;
            }

            if (isBuildingTileBlocked(
                    nextX,
                    nextY,
                    buildings,
                    buildingCount
                )) {
                continue;
            }

            bool diagonal =
                offsets[direction][0] != 0 &&
                offsets[direction][1] != 0;

            if (diagonal) {
                int sideAX = nextX;
                int sideAY = currentY;

                int sideBX = currentX;
                int sideBY = nextY;

                if (isBuildingTileBlocked(
                        sideAX,
                        sideAY,
                        buildings,
                        buildingCount
                    ) ||
                    isBuildingTileBlocked(
                        sideBX,
                        sideBY,
                        buildings,
                        buildingCount
                    )) {
                    continue;
                }
            }

            int nextNode =
                nextY * mapWidth + nextX;

            if (visited[nextNode])
                continue;

            visited[nextNode] = 1;
            parent[nextNode] = current;
            open.push(nextNode);
        }
    }

    if (!visited[goalNode])
        return false;

    std::vector<int> reversedPath;

    int node = goalNode;

    while (node != startNode &&
           node != -1) {

        reversedPath.push_back(node);
        node = parent[node];
    }

    std::reverse(
        reversedPath.begin(),
        reversedPath.end()
    );

    if (reversedPath.empty()) {
        unit->state = UnitState::Idle;
        unit->pathLength = 0;
        unit->pathIndex = 0;
        return true;
    }

    if ((int)reversedPath.size() >
        MAX_UNIT_PATH) {
        return false;
    }

    unit->pathLength =
        (int)reversedPath.size();

    unit->pathIndex = 0;

    for (int i = 0;
         i < unit->pathLength;
         ++i) {

        int pathNode = reversedPath[i];

        unit->pathX[i] =
            pathNode % mapWidth;

        unit->pathY[i] =
            pathNode / mapWidth;
    }

    unit->targetWorldX =
        (unit->pathX[0] + 0.5f) *
        tileSize;

    unit->targetWorldY =
        (unit->pathY[0] + 0.5f) *
        tileSize;

    unit->state = UnitState::Walking;
    unit->animationFrame = 0;
    unit->animationTimer = 0.0f;

    return true;
}


void updateUnits(
    Unit* units,
    int unitCount,
    float deltaSeconds,
    int tileSize,
    RoadState* roads,
    LandPlanState* landPlans,
    Building* buildings,
    int buildingCount,
    int mapWidth,
    int mapHeight
)
{
    if (!units)
        return;

    if (deltaSeconds < 0.0f)
        deltaSeconds = 0.0f;

    if (deltaSeconds > 0.05f)
        deltaSeconds = 0.05f;

    const float movementSpeed = 62.0f;
    const float animationFrameDuration = 0.11f;

    for (int i = 0; i < unitCount; ++i) {
        Unit& unit = units[i];

        if (!unit.active)
            continue;

        // =================================================
        // SERF LOGISTICS STATE PROGRESSION
        //
        // Dva dulezite pripady:
        //
        // 1. Serf uz stoji na source approach tile.
        //    setUnitMoveTarget() pak vrati uspech s prazdnou
        //    cestou a state zustane Idle. Pickup proto
        //    provedeme primo zde.
        //
        // 2. Po pickup je logisticky stav Carrying.
        //    Automaticky zahajime cestu k destination.
        // =================================================

        if (
            unit.type == UnitType::Serf &&
            unit.state == UnitState::Idle &&
            unit.logisticsState ==
                SerfLogisticsState::ToSource
        ) {
            const bool validSource =
                buildings &&
                unit.sourceBuilding >= 0 &&
                unit.sourceBuilding < buildingCount &&
                buildings[
                    unit.sourceBuilding
                ].active &&
                buildings[
                    unit.sourceBuilding
                ].type == BuildingType::Store &&
                isBuildingComplete(
                    buildings[
                        unit.sourceBuilding
                    ]
                );

            const bool validWare =
                isValidWareType(
                    unit.assignedWare
                );

            bool pickedUp = false;

            if (
                validSource &&
                validWare &&
                unit.carriedWare ==
                    WareType::Count
            ) {
                pickedUp =
                    takeStoreWare(
                        &buildings[
                            unit.sourceBuilding
                        ],
                        unit.assignedWare,
                        1
                    );
            }

            if (pickedUp) {
                unit.carriedWare =
                    unit.assignedWare;

                unit.logisticsState =
                    SerfLogisticsState::Carrying;

                unit.animationFrame = 0;
                unit.animationTimer = 0.0f;
            } else {
                // Flour rezervace byla vytvorena jeste
                // pred cestou ke Store. Pokud pickup selze,
                // musi se kapacita Bakery znovu uvolnit.
                if (
                    buildings &&
                    unit.assignedWare ==
                        WareType::Flour &&
                    unit.destinationBuilding >= 0 &&
                    unit.destinationBuilding <
                        buildingCount
                ) {
                    cancelHouseWareDelivery(
                        &buildings[
                            unit.destinationBuilding
                        ],
                        WareType::Flour,
                        1
                    );
                }

                cancelSerfTransport(
                    &unit
                );
            }
        }

        if (
            unit.type == UnitType::Serf &&
            unit.state == UnitState::Idle &&
            unit.logisticsState ==
                SerfLogisticsState::Carrying
        ) {
            // Serf fyzicky vlastni ware.
            // Pokud cil ware stale prijima a existuje cesta,
            // okamzite zahajime transport k destination.
            //
            // Pri neuspechu zustava Carrying a ware se
            // nikdy potichu neztrati.
            startSerfTransportToDestination(
                &unit,
                buildings,
                buildingCount,
                mapWidth,
                mapHeight,
                tileSize
            );

            if (
                unit.state ==
                    UnitState::Walking
            ) {
                continue;
            }
        }

        // =================================================
        // SERF AUTOMATIC TRANSPORT DISPATCH
        //
        // Aktualne podporovane fyzicke transporty:
        //
        // 1. Store Gold  -> School
        // 2. Store Flour -> Bakery
        //
        // Flour kapacita je rezervovana pred zahajenim
        // cesty. Gold zachovava dosavadni School runtime.
        // =================================================
        if (
            unit.type == UnitType::Serf &&
            unit.state == UnitState::Idle &&
            unit.logisticsState ==
                SerfLogisticsState::Idle &&
            unit.carriedWare == WareType::Count &&
            buildings &&
            buildingCount > 0
        ) {
            bool dispatched = false;

            static const WareType transportWares[] = {
                WareType::Gold,
                WareType::Flour
            };

            for (
                WareType transportWare :
                    transportWares
            ) {
                if (dispatched)
                    break;

                for (
                    int sourceIndex = 0;
                    sourceIndex < buildingCount &&
                        !dispatched;
                    ++sourceIndex
                ) {
                    if (
                        !buildings[sourceIndex].active ||
                        buildings[sourceIndex].type !=
                            BuildingType::Store ||
                        !isBuildingComplete(
                            buildings[sourceIndex]
                        )
                    ) {
                        continue;
                    }

                    if (
                        !canStoreTakeWare(
                            buildings[sourceIndex],
                            transportWare,
                            1
                        )
                    ) {
                        continue;
                    }

                    for (
                        int destinationIndex = 0;
                        destinationIndex <
                            buildingCount;
                        ++destinationIndex
                    ) {
                        if (
                            destinationIndex ==
                                sourceIndex
                        ) {
                            continue;
                        }

                        if (
                            !canAssignSerfTransport(
                                unit,
                                transportWare,
                                sourceIndex,
                                destinationIndex,
                                buildings,
                                buildingCount
                            )
                        ) {
                            continue;
                        }

                        bool reservationCreated =
                            false;

                        if (
                            transportWare ==
                                WareType::Flour
                        ) {
                            reservationCreated =
                                reserveHouseWareDelivery(
                                    &buildings[
                                        destinationIndex
                                    ],
                                    transportWare,
                                    1
                                );

                            if (!reservationCreated)
                                continue;
                        }

                        if (
                            !assignSerfTransport(
                                &unit,
                                transportWare,
                                sourceIndex,
                                destinationIndex,
                                buildings,
                                buildingCount
                            )
                        ) {
                            if (reservationCreated) {
                                cancelHouseWareDelivery(
                                    &buildings[
                                        destinationIndex
                                    ],
                                    transportWare,
                                    1
                                );
                            }

                            continue;
                        }

                        if (
                            startSerfTransportToSource(
                                &unit,
                                buildings,
                                buildingCount,
                                mapWidth,
                                mapHeight,
                                tileSize
                            )
                        ) {
                            dispatched = true;
                            break;
                        }

                        // Assignment probehl, ale cesta
                        // ke Store nevznikla.
                        if (reservationCreated) {
                            cancelHouseWareDelivery(
                                &buildings[
                                    destinationIndex
                                ],
                                transportWare,
                                1
                            );
                        }

                        cancelSerfTransport(
                            &unit
                        );
                    }
                }
            }

            if (dispatched)
                continue;
        }

        // =================================================
        // SERF EXACT PRE-DOOR APPROACH
        //
        // Pathfinding konci ve stredu volne dlazdice.
        // Tato kratka faze Serfa nejdrive zarovna po ulici
        // presne pod viditelne dvere.
        //
        // Teprve po dosazeni tohoto bodu se Serf otoci
        // na sever a zahaji EnteringBuilding.
        // =================================================
        if (
            unit.state ==
                UnitState::ApproachingBuilding
        ) {
            const float dx =
                unit.targetWorldX -
                unit.worldX;

            const float dy =
                unit.targetWorldY -
                unit.worldY;

            const float distance =
                std::sqrt(
                    dx * dx +
                    dy * dy
                );

            if (distance <= 1.0f) {
                unit.worldX =
                    unit.targetWorldX;

                unit.worldY =
                    unit.targetWorldY;

                // Vstup zacina az z presneho bodu
                // nachazejiciho se pod dvermi.
                unit.doorwayStartX =
                    unit.worldX;

                unit.doorwayStartY =
                    unit.worldY;

                unit.doorwayProgress = 0.0f;

                // Vstup do budovy je od tohoto okamziku
                // ciste severnim pohybem.
                unit.direction = 0;

                unit.animationFrame = 0;
                unit.animationTimer = 0.0f;

                unit.state =
                    UnitState::EnteringBuilding;

                continue;
            }

            // Behem kratkeho zarovnani pod dvere
            // zachovame smer z posledniho normalniho
            // walking kroku.
            //
            // Serf se tedy neotoci podle nekolikapixeloveho
            // bocniho posunu. Na sever se otoci az presne
            // pod dvermi v prechodu na EnteringBuilding.

            float movement =
                movementSpeed *
                deltaSeconds;

            if (movement >= distance) {
                unit.worldX =
                    unit.targetWorldX;

                unit.worldY =
                    unit.targetWorldY;
            } else {
                unit.worldX +=
                    dx / distance *
                    movement;

                unit.worldY +=
                    dy / distance *
                    movement;
            }

            unit.animationTimer +=
                deltaSeconds;

            while (
                unit.animationTimer >=
                    animationFrameDuration
            ) {
                unit.animationTimer -=
                    animationFrameDuration;

                unit.animationFrame =
                    (
                        unit.animationFrame + 1
                    ) % 8;
            }

            continue;
        }

        // =================================================
        // SERF PHYSICAL BUILDING ENTRY
        //
        // Remake order:
        // PointBelowEntrance -> GoInside -> transfer
        // -> GoOutside.
        //
        // doorwayStart = street / PointBelowEntrance
        // doorwayTarget = entrance tile inside footprint
        // =================================================
        if (
            unit.state ==
                UnitState::EnteringBuilding
        ) {
            const float streetX =
                unit.doorwayStartX;

            const float streetY =
                unit.doorwayStartY;

            const float doorX =
                unit.doorwayTargetX;

            const float doorY =
                unit.doorwayTargetY;

            const float dx = doorX - streetX;
            const float dy = doorY - streetY;

            const float distance =
                std::sqrt(dx * dx + dy * dy);

            if (distance <= 0.0f) {
                unit.doorwayProgress = 1.0f;
            } else {
                unit.doorwayProgress +=
                    (
                        movementSpeed *
                        deltaSeconds
                    ) / distance;
            }

            if (unit.doorwayProgress > 1.0f)
                unit.doorwayProgress = 1.0f;

            unit.worldX =
                streetX +
                dx * unit.doorwayProgress;

            unit.worldY =
                streetY +
                dy * unit.doorwayProgress;

            unit.direction = 0;

            unit.animationTimer += deltaSeconds;

            while (
                unit.animationTimer >=
                    animationFrameDuration
            ) {
                unit.animationTimer -=
                    animationFrameDuration;

                unit.animationFrame =
                    (
                        unit.animationFrame + 1
                    ) % 8;
            }

            // Serf zmizi v okamziku, kdy projde viditelnou
            // hranou dveri. Zbytek logickeho pohybu do
            // entrance tile probiha uz uvnitr budovy.
            unit.visible =
                unit.doorwayProgress < 0.48f;

            if (unit.doorwayProgress < 1.0f)
                continue;

            bool transferSucceeded = false;

            // ---------------------------------------------
            // PICKUP INSIDE SOURCE STORE
            // ---------------------------------------------
            if (
                unit.type == UnitType::Serf &&
                unit.logisticsState ==
                    SerfLogisticsState::ToSource
            ) {
                const bool validSource =
                    buildings &&
                    unit.sourceBuilding >= 0 &&
                    unit.sourceBuilding <
                        buildingCount &&
                    buildings[
                        unit.sourceBuilding
                    ].active &&
                    buildings[
                        unit.sourceBuilding
                    ].type ==
                        BuildingType::Store &&
                    isBuildingComplete(
                        buildings[
                            unit.sourceBuilding
                        ]
                    );

                const bool validWare =
                    isValidWareType(
                        unit.assignedWare
                    );

                if (
                    validSource &&
                    validWare &&
                    unit.carriedWare ==
                        WareType::Count
                ) {
                    transferSucceeded =
                        takeStoreWare(
                            &buildings[
                                unit.sourceBuilding
                            ],
                            unit.assignedWare,
                            1
                        );
                }

                if (transferSucceeded) {
                    unit.carriedWare =
                        unit.assignedWare;

                    unit.logisticsState =
                        SerfLogisticsState::Carrying;
                } else {
                    // Pickup uvnitr Store selhal.
                    // U Flour musime vratit rezervovanou
                    // kapacitu cilove Bakery.
                    if (
                        buildings &&
                        unit.assignedWare ==
                            WareType::Flour &&
                        unit.destinationBuilding >= 0 &&
                        unit.destinationBuilding <
                            buildingCount
                    ) {
                        cancelHouseWareDelivery(
                            &buildings[
                                unit.destinationBuilding
                            ],
                            WareType::Flour,
                            1
                        );
                    }

                    unit.carriedWare =
                        WareType::Count;

                    unit.assignedWare =
                        WareType::Count;

                    unit.sourceBuilding = -1;
                    unit.destinationBuilding = -1;

                    unit.logisticsState =
                        SerfLogisticsState::Idle;
                }
            }

            // ---------------------------------------------
            // DELIVERY INSIDE DESTINATION HOUSE
            // ---------------------------------------------
            else if (
                unit.type == UnitType::Serf &&
                unit.logisticsState ==
                    SerfLogisticsState::ToDestination
            ) {
                const bool validDestination =
                    buildings &&
                    unit.destinationBuilding >= 0 &&
                    unit.destinationBuilding <
                        buildingCount;

                const bool validCargo =
                    isValidWareType(
                        unit.assignedWare
                    ) &&
                    isValidWareType(
                        unit.carriedWare
                    ) &&
                    unit.assignedWare ==
                        unit.carriedWare;

                if (
                    validDestination &&
                    validCargo
                ) {
                    transferSucceeded =
                        deliverWareToBuilding(
                            &buildings[
                                unit.destinationBuilding
                            ],
                            unit.carriedWare
                        );
                }

                if (transferSucceeded) {
                    unit.carriedWare =
                        WareType::Count;

                    unit.assignedWare =
                        WareType::Count;

                    unit.sourceBuilding = -1;
                    unit.destinationBuilding = -1;

                    unit.logisticsState =
                        SerfLogisticsState::Idle;
                } else {
                    // Delivery nebyla prijata.
                    // Serf si naklad ponecha.
                    unit.logisticsState =
                        SerfLogisticsState::Carrying;
                }
            }

            // ---------------------------------------------
            // LEAVE BUILDING
            //
            // Obratime predchozi doorway trasu:
            // entrance -> PointBelowEntrance.
            // ---------------------------------------------
            unit.doorwayStartX = doorX;
            unit.doorwayStartY = doorY;

            unit.doorwayTargetX = streetX;
            unit.doorwayTargetY = streetY;

            unit.doorwayProgress = 0.0f;

            unit.worldX = doorX;
            unit.worldY = doorY;

            unit.direction = 4;
            unit.visible = false;

            unit.animationFrame = 0;
            unit.animationTimer = 0.0f;

            unit.state =
                UnitState::LeavingBuilding;

            continue;
        }

        if (
            unit.state ==
                UnitState::LeavingBuilding
        ) {
            float dx =
                unit.doorwayTargetX -
                unit.doorwayStartX;

            float dy =
                unit.doorwayTargetY -
                unit.doorwayStartY;

            float distance =
                std::sqrt(
                    dx * dx +
                    dy * dy
                );

            if (distance <= 0.0f) {
                unit.doorwayProgress = 1.0f;
            } else {
                unit.doorwayProgress +=
                    (
                        movementSpeed *
                        deltaSeconds
                    ) / distance;
            }

            if (unit.doorwayProgress > 1.0f)
                unit.doorwayProgress = 1.0f;

            unit.worldX =
                unit.doorwayStartX +
                dx * unit.doorwayProgress;

            unit.worldY =
                unit.doorwayStartY +
                dy * unit.doorwayProgress;

            // Uvnitr budovy zustava jednotka skryta.
            // Zobrazi se az pri pruchodu viditelnou hranou
            // dveri, ne hluboko pod texturou budovy.
            unit.visible =
                unit.doorwayProgress >= 0.52f;

            // Vystup ze School smeruje na jih.
            unit.direction = 4;

            unit.animationTimer +=
                deltaSeconds;

            while (
                unit.animationTimer >=
                    animationFrameDuration
            ) {
                unit.animationTimer -=
                    animationFrameDuration;

                unit.animationFrame =
                    (
                        unit.animationFrame + 1
                    ) % 8;
            }

            if (
                unit.doorwayProgress >= 1.0f
            ) {
                unit.worldX =
                    unit.doorwayTargetX;

                unit.worldY =
                    unit.doorwayTargetY;

                unit.targetWorldX =
                    unit.worldX;

                unit.targetWorldY =
                    unit.worldY;

                unit.doorwayProgress = 0.0f;

                unit.doorwayOffsetX = 0;
                unit.doorwayOffsetY = 0;

                unit.visible = true;
                unit.state = UnitState::Idle;

                unit.animationFrame = 0;
                unit.animationTimer = 0.0f;
            }

            continue;
        }

        // -------------------------------------------------
        // House terrain preparation.
        //
        // Worker stoji na volne dlazdici pred entrance.
        // Prvni Work1 faze pouze pripravi podklad.
        //
        // constructionProgress:
        // 0 = rope plan + tablet
        // 1 = prepared terrain, wooden WIP fence
        //
        // Wood/Stone construction je samostatna dalsi faze.
        // -------------------------------------------------
        if (
            unit.state == UnitState::Working &&
            unit.workAction ==
                UnitWorkAction::BuildHouse
        ) {
            const bool validTarget =
                buildings &&
                unit.targetBuilding >= 0 &&
                unit.targetBuilding < buildingCount &&
                buildings[
                    unit.targetBuilding
                ].active;

            if (!validTarget) {
                unit.state = UnitState::Idle;
                unit.targetBuilding = -1;

                unit.doorwayOffsetX = 0;
                unit.doorwayOffsetY = 0;
                unit.doorwayProgress = 0.0f;

                unit.workAction =
                    UnitWorkAction::None;

                unit.workTimer = 0.0f;
                unit.animationFrame = 0;
                unit.animationTimer = 0.0f;

                continue;
            }

            Building& building =
                buildings[
                    unit.targetBuilding
                ];

            // Plan mohl byt mezitim zrusen nebo
            // uz pripraven jinym Workerem.
            if (
                building.constructionProgress != 0 ||
                building.woodStep != 0 ||
                building.stoneStep != 0
            ) {
                unit.state = UnitState::Idle;
                unit.targetBuilding = -1;

                unit.doorwayOffsetX = 0;
                unit.doorwayOffsetY = 0;
                unit.doorwayProgress = 0.0f;

                unit.workAction =
                    UnitWorkAction::None;

                unit.workTimer = 0.0f;
                unit.animationFrame = 0;
                unit.animationTimer = 0.0f;

                continue;
            }

            // Logická pozice nohou zůstává mimo
            // blokovaný footprint. Vizuální Work1 sprite
            // během krátké fáze plynule přejde dovnitř.
            if (unit.doorwayProgress < 1.0f) {
                unit.doorwayProgress +=
                    deltaSeconds * 4.0f;

                if (unit.doorwayProgress > 1.0f)
                    unit.doorwayProgress = 1.0f;
            } else {
                // Čas přípravy podkladu běží až poté,
                // co Worker vizuálně dorazí na místo.
                unit.workTimer += deltaSeconds;
            }

            unit.animationTimer += deltaSeconds;

            while (
                unit.animationTimer >=
                    animationFrameDuration
            ) {
                unit.animationTimer -=
                    animationFrameDuration;

                unit.animationFrame =
                    (
                        unit.animationFrame + 1
                    ) %
                    WORKER_WORK1_FRAME_COUNT;
            }

            // Prvni House construction checkpoint:
            // Worker dokoncil pripravu / srovnani podkladu.
            if (unit.workTimer >= 1.30f) {
                building.constructionProgress = 1;

                unit.state = UnitState::Idle;
                unit.targetBuilding = -1;

                unit.doorwayOffsetX = 0;
                unit.doorwayOffsetY = 0;
                unit.doorwayProgress = 0.0f;

                unit.workAction =
                    UnitWorkAction::None;

                unit.workTimer = 0.0f;
                unit.animationFrame = 0;
                unit.animationTimer = 0.0f;
            }

            continue;
        }


        if (
            unit.state == UnitState::Working &&
            (
                unit.workAction ==
                    UnitWorkAction::BuildField ||
                unit.workAction ==
                    UnitWorkAction::BuildWine ||
                unit.workAction ==
                    UnitWorkAction::BuildWineWood
            )
        ) {
            if (
                !landPlans ||
                unit.targetRoadX < 0 ||
                unit.targetRoadX >= mapWidth ||
                unit.targetRoadY < 0 ||
                unit.targetRoadY >= mapHeight
            ) {
                unit.state = UnitState::Idle;

                unit.targetRoadX = -1;
                unit.targetRoadY = -1;

                unit.workAction =
                    UnitWorkAction::None;

                unit.workTimer = 0.0f;
                unit.animationFrame = 0;
                unit.animationTimer = 0.0f;

                continue;
            }

            LandPlanState& land =
                landPlans[
                    unit.targetRoadY *
                        mapWidth +
                    unit.targetRoadX
                ];

            const bool fieldWork =
                unit.workAction ==
                    UnitWorkAction::BuildField;

            const bool wineDigWork =
                unit.workAction ==
                    UnitWorkAction::BuildWine;

            const bool wineWoodWork =
                unit.workAction ==
                    UnitWorkAction::BuildWineWood;

            LandPlanState expectedState =
                LandPlanState::None;

            if (fieldWork) {
                expectedState =
                    LandPlanState::Field;
            } else if (wineDigWork) {
                expectedState =
                    LandPlanState::Wine;
            } else if (wineWoodWork) {
                expectedState =
                    LandPlanState::WineInit;
            }

            if (land != expectedState) {
                // Remove mohl plan nebo mezistav
                // behem prace zrusit.
                unit.state = UnitState::Idle;

                unit.targetRoadX = -1;
                unit.targetRoadY = -1;

                unit.workAction =
                    UnitWorkAction::None;

                unit.workTimer = 0.0f;
                unit.animationFrame = 0;
                unit.animationTimer = 0.0f;

                continue;
            }

            unit.workTimer += deltaSeconds;
            unit.animationTimer += deltaSeconds;

            const int workFrameCount =
                wineWoodWork
                    ? WORKER_WORK2_FRAME_COUNT
                    : WORKER_WORK1_FRAME_COUNT;

            while (
                unit.animationTimer >=
                    animationFrameDuration
            ) {
                unit.animationTimer -=
                    animationFrameDuration;

                unit.animationFrame =
                    (
                        unit.animationFrame + 1
                    ) %
                    workFrameCount;
            }

            // Field zustava jedna Work1 faze.
            //
            // Wine:
            // Work1 -> WineInit
            // Work2 -> WineComplete
            //
            // Budouci Serf/Store 1 Wood delivery se vlozi
            // mezi WineInit a BuildWineWood.
            if (unit.workTimer >= 1.30f) {
                if (fieldWork) {
                    land =
                        LandPlanState::FieldComplete;

                    unit.state = UnitState::Idle;
                    unit.targetRoadX = -1;
                    unit.targetRoadY = -1;

                    unit.workAction =
                        UnitWorkAction::None;
                } else if (wineDigWork) {
                    // Ekvivalent prvniho Wine construction
                    // checkpointu: Terrain 55 uz muze byt
                    // vykreslen, Obj 54 jeste ne.
                    land =
                        LandPlanState::WineInit;

                    // Ve finalni ekonomice zde Worker
                    // prejde do cekani na 1 Wood.
                    //
                    // Dokud nema KaMVita obecny Ware/Store
                    // delivery system, navazeme rovnou
                    // druhou autentickou Work2 fazi.
                    unit.workAction =
                        UnitWorkAction::BuildWineWood;

                    unit.state =
                        UnitState::Working;
                } else {
                    land =
                        LandPlanState::WineComplete;

                    unit.state = UnitState::Idle;
                    unit.targetRoadX = -1;
                    unit.targetRoadY = -1;

                    unit.workAction =
                        UnitWorkAction::None;
                }

                unit.workTimer = 0.0f;
                unit.animationFrame = 0;
                unit.animationTimer = 0.0f;
            }

            continue;
        }

        if (unit.state == UnitState::Working) {
            if (!roads ||
                unit.targetRoadX < 0 ||
                unit.targetRoadX >= mapWidth ||
                unit.targetRoadY < 0 ||
                unit.targetRoadY >= mapHeight) {

                unit.state = UnitState::Idle;
                unit.targetRoadX = -1;
                unit.targetRoadY = -1;
                continue;
            }

            RoadState& road =
                roads[
                    unit.targetRoadY * mapWidth +
                    unit.targetRoadX
                ];

            if (road == RoadState::None ||
                road == RoadState::Complete) {

                unit.state = UnitState::Idle;
                unit.targetRoadX = -1;
                unit.targetRoadY = -1;
                continue;
            }

            if (
                road == RoadState::Plan ||
                road == RoadState::Dig1 ||
                road == RoadState::Dig2
            ) {
                unit.workAction =
                    UnitWorkAction::DigRoad;
            } else {
                unit.workAction =
                    UnitWorkAction::PaveRoad;
            }

            unit.workTimer += deltaSeconds;
            unit.animationTimer += deltaSeconds;

            while (
                unit.animationTimer >=
                animationFrameDuration
            ) {
                unit.animationTimer -=
                    animationFrameDuration;

                int workFrameCount =
                unit.workAction ==
                    UnitWorkAction::DigRoad
                    ? WORKER_WORK1_FRAME_COUNT
                    : WORKER_WORK2_FRAME_COUNT;

            unit.animationFrame =
                (unit.animationFrame + 1) %
                workFrameCount;
            }

            // Jeden stavebni krok cesty.
            if (unit.workTimer >= 0.65f) {
                unit.workTimer = 0.0f;

                int nextState =
                    (int)road + 1;

                if (nextState >
                    (int)RoadState::Complete) {

                    nextState =
                        (int)RoadState::Complete;
                }

                road =
                    (RoadState)nextState;

                if (road == RoadState::Complete) {
                    unit.state = UnitState::Idle;
                    unit.targetRoadX = -1;
                    unit.targetRoadY = -1;
                    unit.animationFrame = 0;
                    unit.animationTimer = 0.0f;
                }
            }

            continue;
        }

        if (unit.state != UnitState::Walking)
            continue;

        float dx =
            unit.targetWorldX - unit.worldX;

        float dy =
            unit.targetWorldY - unit.worldY;

        float distance =
            std::sqrt(dx * dx + dy * dy);

        if (distance <= 1.0f) {
            unit.worldX = unit.targetWorldX;
            unit.worldY = unit.targetWorldY;

            unit.pathIndex++;

            if (unit.pathIndex >=
                unit.pathLength) {

                unit.animationFrame = 0;
                unit.animationTimer = 0.0f;
                unit.pathLength = 0;
                unit.pathIndex = 0;

                // =========================================
                // SERF ENTER DESTINATION
                //
                // Transfer uz neprobiha pred budovou.
                // Serf nejdrive fyzicky vstoupi entrance tile.
                // Delivery se provede az po dokonceni vstupu.
                // =========================================
                if (
                    unit.type == UnitType::Serf &&
                    unit.logisticsState ==
                        SerfLogisticsState::ToDestination
                ) {
                    const bool validDestination =
                        buildings &&
                        unit.destinationBuilding >= 0 &&
                        unit.destinationBuilding <
                            buildingCount &&
                        buildings[
                            unit.destinationBuilding
                        ].active &&
                        isBuildingComplete(
                            buildings[
                                unit.destinationBuilding
                            ]
                        );

                    if (!validDestination) {
                        // Naklad zustava fyzicky u Serfa.
                        unit.logisticsState =
                            SerfLogisticsState::Carrying;

                        unit.state = UnitState::Idle;
                        continue;
                    }

                    const Building& destination =
                        buildings[
                            unit.destinationBuilding
                        ];

                    const BuildingEntrance entrance =
                        getBuildingEntrance(
                            destination.type
                        );

                    // Nejdrixe spocitame doorway offset
                    // aktualni cilove budovy. Teprve potom
                    // muzeme odvodit skutecny bod dveri.
                    constexpr int BUILDING_DOOR_ANCHOR_X =
                        -18;

                    unit.doorwayOffsetX =
                        entrance.pixelOffsetX -
                        tileSize / 2 +
                        BUILDING_DOOR_ANCHOR_X;

                    unit.doorwayOffsetY =
                        entrance.pixelOffsetY;

                    const float logicalDoorX =
                        (
                            destination.tileX +
                            entrance.tileOffsetX +
                            0.5f
                        ) * tileSize;

                    const float logicalDoorY =
                        (
                            destination.tileY +
                            entrance.tileOffsetY +
                            0.5f
                        ) * tileSize;

                    unit.doorwayTargetX =
                        logicalDoorX +
                        unit.doorwayOffsetX;

                    unit.doorwayTargetY =
                        logicalDoorY +
                        unit.doorwayOffsetY;

                    // Kratke zarovnani probiha po ulici.
                    // Serf se dostane pod stejne X jako dvere,
                    // ale zustane na pristupove pozici Y.
                    unit.targetWorldX =
                        unit.doorwayTargetX;

                    unit.targetWorldY =
                        unit.worldY;

                    unit.doorwayProgress = 0.0f;
                    unit.visible = true;

                    // Neotacime Serfa na sever ve stredu
                    // dlazdice. Nejdrive jde presne pod dvere.
                    unit.state =
                        UnitState::ApproachingBuilding;

                    continue;
                }

                // =========================================
                // SERF ENTER SOURCE STORE
                //
                // Ware zustava ve Store po celou dobu vstupu.
                // Pickup se provede az uvnitr budovy.
                // =========================================
                if (
                    unit.type == UnitType::Serf &&
                    unit.logisticsState ==
                        SerfLogisticsState::ToSource
                ) {
                    const bool validSource =
                        buildings &&
                        unit.sourceBuilding >= 0 &&
                        unit.sourceBuilding <
                            buildingCount &&
                        buildings[
                            unit.sourceBuilding
                        ].active &&
                        buildings[
                            unit.sourceBuilding
                        ].type ==
                            BuildingType::Store &&
                        isBuildingComplete(
                            buildings[
                                unit.sourceBuilding
                            ]
                        );

                    if (!validSource) {
                        cancelSerfTransport(&unit);
                        unit.state = UnitState::Idle;
                        continue;
                    }

                    const Building& sourceBuilding =
                        buildings[
                            unit.sourceBuilding
                        ];

                    const BuildingEntrance entrance =
                        getBuildingEntrance(
                            sourceBuilding.type
                        );

                    // Nejdrixe spocitame doorway offset
                    // aktualniho Store. Teprve potom
                    // muzeme odvodit skutecny bod dveri.
                    constexpr int BUILDING_DOOR_ANCHOR_X =
                        -18;

                    unit.doorwayOffsetX =
                        entrance.pixelOffsetX -
                        tileSize / 2 +
                        BUILDING_DOOR_ANCHOR_X;

                    unit.doorwayOffsetY =
                        entrance.pixelOffsetY;

                    const float logicalDoorX =
                        (
                            sourceBuilding.tileX +
                            entrance.tileOffsetX +
                            0.5f
                        ) * tileSize;

                    const float logicalDoorY =
                        (
                            sourceBuilding.tileY +
                            entrance.tileOffsetY +
                            0.5f
                        ) * tileSize;

                    unit.doorwayTargetX =
                        logicalDoorX +
                        unit.doorwayOffsetX;

                    unit.doorwayTargetY =
                        logicalDoorY +
                        unit.doorwayOffsetY;

                    // Presny bod pod dvermi, stale na ulici.
                    unit.targetWorldX =
                        unit.doorwayTargetX;

                    unit.targetWorldY =
                        unit.worldY;

                    unit.doorwayProgress = 0.0f;
                    unit.visible = true;

                    // Neotacime Serfa na sever ve stredu
                    // dlazdice. Nejdrive jde presne pod dvere.
                    unit.state =
                        UnitState::ApproachingBuilding;

                    continue;
                }

                const bool hasRoadWork =
                    unit.targetRoadX >= 0 &&
                    unit.targetRoadY >= 0;

                const bool hasBuildingWork =
                    unit.targetBuilding >= 0 &&
                    unit.workAction ==
                        UnitWorkAction::BuildHouse;

                if (
                    hasRoadWork ||
                    hasBuildingWork
                ) {
                    unit.state =
                        UnitState::Working;

                    unit.workTimer = 0.0f;

                    if (hasBuildingWork) {
                        // Worker zustava logicky na volne
                        // dlazdici pred entrance, ale Work1
                        // animace se vykresli uvnitr planu.
                        unit.doorwayOffsetX = 0;
                        unit.doorwayOffsetY =
                            -tileSize;

                        // Plynulý vizuální přesun z volné
                        // entrance dlaždice do footprintu.
                        unit.doorwayProgress = 0.0f;
                    } else {
                        unit.doorwayOffsetX = 0;
                        unit.doorwayOffsetY = 0;
                    }
                } else {
                    unit.state =
                        UnitState::Idle;
                }

                continue;
            }

            unit.targetWorldX =
                (unit.pathX[unit.pathIndex] + 0.5f) *
                tileSize;

            unit.targetWorldY =
                (unit.pathY[unit.pathIndex] + 0.5f) *
                tileSize;

            // =============================================
            // SERF FINAL WAYPOINT DOOR ALIGNMENT
            //
            // Posledni waypoint transportni cesty nemusi
            // byt stred PointBelowEntrance tile.
            //
            // Uz posledni walking segment zamirime na X
            // skutecnych dveri. Serf se tak zarovna pri
            // prichodu a nemusi se po zastaveni vracet
            // bokem nebo pozadu.
            // =============================================
            if (
                unit.type == UnitType::Serf &&
                unit.pathIndex ==
                    unit.pathLength - 1 &&
                (
                    unit.logisticsState ==
                        SerfLogisticsState::ToSource ||
                    unit.logisticsState ==
                        SerfLogisticsState::ToDestination
                ) &&
                buildings
            ) {
                int houseIndex = -1;

                if (
                    unit.logisticsState ==
                        SerfLogisticsState::ToSource
                ) {
                    houseIndex =
                        unit.sourceBuilding;
                } else {
                    houseIndex =
                        unit.destinationBuilding;
                }

                if (
                    houseIndex >= 0 &&
                    houseIndex < buildingCount &&
                    buildings[houseIndex].active
                ) {
                    const Building& house =
                        buildings[houseIndex];

                    const BuildingEntrance entrance =
                        getBuildingEntrance(
                            house.type
                        );

                    constexpr int
                        BUILDING_DOOR_ANCHOR_X = -18;

                    const int doorOffsetX =
                        entrance.pixelOffsetX -
                        tileSize / 2 +
                        BUILDING_DOOR_ANCHOR_X;

                    const float logicalDoorX =
                        (
                            house.tileX +
                            entrance.tileOffsetX +
                            0.5f
                        ) * tileSize;

                    // Y zustava na volne pristupove
                    // dlazdici. Menime pouze X posledniho
                    // walking waypointu.
                    unit.targetWorldX =
                        logicalDoorX +
                        doorOffsetX;
                }
            }

            continue;
        }

        float angle = std::atan2(dy, dx);

        const float PI = 3.14159265358979323846f;

        // 0=N, 1=NE, 2=E, 3=SE,
        // 4=S, 5=SW, 6=W, 7=NW.
        if (angle >= -PI / 8.0f &&
            angle < PI / 8.0f) {
            unit.direction = 2;
        } else if (
            angle >= PI / 8.0f &&
            angle < 3.0f * PI / 8.0f
        ) {
            unit.direction = 3;
        } else if (
            angle >= 3.0f * PI / 8.0f &&
            angle < 5.0f * PI / 8.0f
        ) {
            unit.direction = 4;
        } else if (
            angle >= 5.0f * PI / 8.0f &&
            angle < 7.0f * PI / 8.0f
        ) {
            unit.direction = 5;
        } else if (
            angle >= 7.0f * PI / 8.0f ||
            angle < -7.0f * PI / 8.0f
        ) {
            unit.direction = 6;
        } else if (
            angle >= -7.0f * PI / 8.0f &&
            angle < -5.0f * PI / 8.0f
        ) {
            unit.direction = 7;
        } else if (
            angle >= -5.0f * PI / 8.0f &&
            angle < -3.0f * PI / 8.0f
        ) {
            unit.direction = 0;
        } else {
            unit.direction = 1;
        }

        float movement =
            movementSpeed * deltaSeconds;

        if (movement >= distance) {
            unit.worldX = unit.targetWorldX;
            unit.worldY = unit.targetWorldY;
        } else {
            unit.worldX += dx / distance * movement;
            unit.worldY += dy / distance * movement;
        }

        unit.animationTimer += deltaSeconds;

        while (
            unit.animationTimer >=
            animationFrameDuration
        ) {
            unit.animationTimer -=
                animationFrameDuration;

            unit.animationFrame =
                (unit.animationFrame + 1) % 8;
        }
    }
}

UnitMovementType getUnitMovementType(
    UnitType type
)
{
    if (type == UnitType::Serf)
        return UnitMovementType::Carrier;

    return UnitMovementType::Worker;
}

