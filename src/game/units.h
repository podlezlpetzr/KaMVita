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

    // Wine phase 1:
    // kopani / priprava terenu.
    BuildWine,

    // Wine phase 2:
    // stavba drevene konstrukce.
    //
    // Pozdeji bude tato faze povolena az po
    // doruceni 1 Wood.
    BuildWineWood
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

    // Plany cekajici na Workera.
    Field,
    Wine,

    // Field je po Work1 hotovy.
    FieldComplete,

    // Wine po prvni Worker fazi:
    // Terrain 55 uz existuje, ale jeste chybi
    // finalni drevena konstrukce / Obj 54.
    //
    // Budouci 1 Wood delivery bude cekat zde.
    WineInit,

    // Wine po druhe Worker Work2 fazi.
    WineComplete
};

// KaM Remake condition maximum: 45 * 60.
static constexpr int UNIT_MAX_CONDITION =
    45 * 60;

// New KaMVita units start at the Remake base 60 percent.
static constexpr int UNIT_START_CONDITION =
    UNIT_MAX_CONDITION * 6 / 10;

static constexpr int MAX_UNIT_PATH = 256;

// ============================================================
// SERF LOGISTICS
//
// Samostatny logisticky stav je oddeleny od UnitState.
// UnitState zustava zodpovedny za fyzicky pohyb / akci,
// SerfLogisticsState popisuje logistickou ulohu.
//
// Zatim pouze datovy zaklad. Automaticke taskovani se
// prida az v samostatnem kroku.
// ============================================================

enum class SerfLogisticsState {
    Idle = 0,

    // Uloha byla Serfovi pridelena.
    Assigned,

    // Serf smeruje ke zdrojove budove.
    ToSource,

    // Ware byl vyzvednut.
    Carrying,

    // Serf smeruje k cilove budove.
    ToDestination,

    // Ware byl dorucen a Serf se vraci.
    Returning
};


struct Unit {
    UnitType type;
    UnitState state;

    // KaM Remake unit condition:
    // 0..UNIT_MAX_CONDITION.
    int condition;

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

    // ========================================================
    // Serf logistics runtime
    //
    // carriedWare == WareType::Count znamena, ze Serf
    // momentalne zadny ware nenese.
    //
    // sourceBuilding / destinationBuilding jsou indexy do
    // hlavniho Building pole. -1 znamena zadnou budovu.
    //
    // Tyto hodnoty zatim nepouziva automaticke taskovani.
    // ========================================================

    SerfLogisticsState logisticsState;

    // Ware prirazeny logistickou ulohou.
    // Nemusi jeste byt fyzicky u Serfa.
    WareType assignedWare;

    // Ware skutecne neseny Serfem.
    // WareType::Count = zadny naklad.
    WareType carriedWare;

    int sourceBuilding;
    int destinationBuilding;

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
    Building* buildings,
    int buildingCount,
    int mapWidth,
    int mapHeight
);

// ============================================================
// SERF TRANSPORT ASSIGNMENT API
//
// Pouze rezervuje logistickou ulohu v Serf runtime stavu.
// Neodebira ware ze Store a nespousti pohyb.
//
// Pickup se provede az po skutecnem prichodu Serfa ke
// zdrojovemu Store.
// ============================================================

bool canAssignSerfTransport(
    const Unit& serf,
    WareType ware,
    int sourceBuilding,
    int destinationBuilding,
    const Building* buildings,
    int buildingCount
);

bool assignSerfTransport(
    Unit* serf,
    WareType ware,
    int sourceBuilding,
    int destinationBuilding,
    const Building* buildings,
    int buildingCount
);

void cancelSerfTransport(
    Unit* serf
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
