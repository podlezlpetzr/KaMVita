#pragma once

#include <SDL2/SDL.h>
#include "buildings.h"

enum class UnitType {
    Worker = 0,
    Serf = 1
};

enum class UnitState {
    Idle,
    Walking,
    Working,
    LeavingBuilding
};

enum class UnitWorkAction {
    None,
    BuildHouse,
    DigRoad,
    WaitForStone,
    PaveRoad,

    // Worker construction pro Field/Wine.
    BuildField,
    BuildWine
};

enum class RoadState : unsigned char {
    None = 0,
    Plan,
    Dig1,
    Dig2,
    Dig3,
    Dig4,
    Complete
};


// Field/Wine workflow.
//
// Field/Wine jsou plany cekajici na Workera.
// Complete varianty jsou hotovy teren.
//
// Farmer a rust budou samostatna dalsi vrstva.
enum class LandPlanState : unsigned char {
    None = 0,
    Field,
    Wine,
    FieldComplete,
    WineComplete
};

static constexpr int MAX_UNIT_PATH = 256;

struct Unit {
    UnitType type;
    UnitState state;

    float worldX;
    float worldY;

    float targetWorldX;
    float targetWorldY;

    int pathX[MAX_UNIT_PATH];
    int pathY[MAX_UNIT_PATH];

    int pathLength;
    int pathIndex;

    int direction;
    int animationFrame;
    float animationTimer;

    int targetBuilding;

    int targetRoadX;
    int targetRoadY;
    float workTimer;

    UnitWorkAction workAction;

    // Pruchod z budovy na ulici.
    float doorwayProgress;

    float doorwayStartX;
    float doorwayStartY;

    float doorwayTargetX;
    float doorwayTargetY;

    // Pouze renderovaci pixelovy posun.
    int doorwayOffsetX;
    int doorwayOffsetY;

    bool visible;
    bool active;
};

struct UnitResources {
    SDL_Texture* workerIdle;
    SDL_Texture* workerWalkAtlas;
    SDL_Texture* workerWorkAtlas;
    SDL_Texture* workerDigAtlas;
    SDL_Texture* workerPaveAtlas;

    SDL_Texture* serfWalkAtlas;
    SDL_Texture* serfCarryStoneAtlas;
};

static constexpr int MAX_UNITS = 32;

UnitResources loadUnitResources(
    SDL_Renderer* renderer
);

void destroyUnitResources(
    UnitResources* resources
);

void renderUnits(
    SDL_Renderer* renderer,
    const UnitResources& resources,
    const Unit* units,
    int unitCount,
    float cameraX,
    float cameraY
);

void updateUnits(
    Unit* units,
    int unitCount,
    float deltaSeconds,
    int tileSize,
    RoadState* roads,
    LandPlanState* landPlans,
    int mapWidth,
    int mapHeight
);

bool setUnitMoveTarget(
    Unit* unit,
    float targetWorldX,
    float targetWorldY,
    const Building* buildings,
    int buildingCount,
    int mapWidth,
    int mapHeight,
    int tileSize
);

enum class UnitMovementType {
    Worker,
    Carrier,
    Army
};

UnitMovementType getUnitMovementType(
    UnitType type
);
