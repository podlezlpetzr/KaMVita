#pragma once

#include <SDL2/SDL.h>
#include "buildings.h"

enum class UnitType {
    Worker = 0,
    Serf = 1,

    // Originalni KaM ut_Baker, unit.dat index 6.
    Baker = 2,

    // Originalni KaM ut_Farmer, unit.dat index 4.
    Farmer = 3
};

enum class UnitState {
    Idle,
    Walking,
    Working,

    // Kratke pixelove zarovnani ze stredu pristupove
    // dlazdice presne pod viditelne dvere.
    ApproachingBuilding,

    // Kratky pohyb z bodu pod dvermi dovnitr budovy.
    EnteringBuilding,

    // Kratky pohyb ze dveri zpet do bodu pod nimi.
    LeavingBuilding,
    // Originalni ua_Eat / ua_Die lifecycle.
    Eating,
    Dying

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
    BuildWineWood,

    // Farmer jde z Farm na prazdne hotove pole.
    FarmerToField,

    // Originalni Farmer ua_Work1 seti.
    FarmerSow,

    // Farmer se po praci vraci do sve Farm.
    FarmerReturnFarm,

    // Farmer smeruje ke zralemu FieldRipe.
    //
    // Nova hodnota je pridana na konec enumu,
    // aby zustaly zachovane existujici Save v11 hodnoty.
    FarmerToHarvest,

    // Originalni Farmer ua_Work:
    // 16 framu x 6 pracovnich cyklu.
    FarmerHarvest
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
    WineComplete,

    // Originalni KaM zasiate kukuricne pole:
    // Terrain 61 / cornSown.
    //
    // Stav je pridan az na konec enumu, aby zustaly
    // zachovane existujici hodnoty Save v11.
    FieldSown,

    // Originalni KaM Terrain 59:
    // mladé sazenice.
    FieldYoung,

    // Originalni KaM Terrain 60:
    // rostouci zelena kukurice.
    FieldGrowing,

    // Originalni KaM Terrain 60 + full-grown Obj 59.
    // Pole je pripravene pro budouci sklizen.
    FieldRipe,

    // Originalni KaM Terrain 60 + Obj 58:
    // mensi zelena kukurice.
    //
    // Stav je pridan na konec enumu, aby zustaly
    // zachovane vsechny existujici hodnoty Save v11.
    FieldGreen,

    // Originalni CutCorn vysledek:
    //
    // FieldAge = 0
    // Terrain  = 63
    // Obj      = 255
    //
    // Pole zustava jako corn field a pozdeji muze byt
    // Farmerem znovu zaseto.
    FieldCut
};

// KaM Remake condition maximum: 45 * 60.
static constexpr int UNIT_MAX_CONDITION =
    45 * 60;

// Originalni KaM/Remake hunger threshold.
static constexpr int UNIT_MIN_CONDITION =
    6 * 60;

// Originalni th_Death threshold.
static constexpr int UNIT_DEATH_THOUGHT_CONDITION =
    UNIT_MIN_CONDITION / 3;

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

    // Originalni Worker ua_Eat / ua_Die.
    SDL_Texture* workerEatAtlas;
    SDL_Texture* workerDieAtlas;

    SDL_Texture* serfWalkAtlas;

    // Originalni Serf ua_Eat / ua_Die.
    SDL_Texture* serfEatAtlas;
    SDL_Texture* serfDieAtlas;
    SDL_Texture* serfCarryStoneAtlas;

    // Originalni KaM SerfCarry[wt_Gold].
    // 8 smeru x 8 animacnich kroku.
    SDL_Texture* serfCarryGoldAtlas;

    // Originalni KaM SerfCarry[wt_Flour].
    // 8 smeru x 8 animacnich kroku.
    SDL_Texture* serfCarryFlourAtlas;

    // Originalni KaM SerfCarry[wt_Bread].
    // 8 smeru x 8 animacnich kroku.
    SDL_Texture* serfCarryBreadAtlas;

    // ========================================================
    // ORIGINAL KaM BAKER UNIT
    //
    // unit.dat index 6.
    //
    // Walk: 8 smeru x 8 framu.
    // Work: 8 smeru x 1 frame.
    // Eat:  8 smeru x 30 framu.
    // Die:  promenlivy pocet framu podle smeru.
    // ========================================================

    SDL_Texture* bakerWalkAtlas;
    SDL_Texture* bakerWorkAtlas;
    SDL_Texture* bakerEatAtlas;
    SDL_Texture* bakerDieAtlas;

    // ========================================================
    // ORIGINAL KaM FARMER UNIT
    //
    // unit.dat index 4.
    //
    // Walk:       8 smeru x 8 framu.
    // Work:       8 smeru x 16 framu.
    // Work1:      8 smeru x 10 framu.
    // WalkTool:   8 smeru x 8 framu.
    // WalkBooty:  8 smeru x 8 framu.
    // Eat:        8 smeru x 30 framu.
    // Die:        promenlivy pocet framu podle smeru.
    // ========================================================

    SDL_Texture* farmerWalkAtlas;
    SDL_Texture* farmerWorkAtlas;
    SDL_Texture* farmerWork1Atlas;
    SDL_Texture* farmerWalkToolAtlas;
    SDL_Texture* farmerWalkBootyAtlas;
    SDL_Texture* farmerEatAtlas;
    SDL_Texture* farmerDieAtlas;

    // Originalni KaM thought bubbles z units.rx.
    SDL_Texture* thoughtEatAtlas;
    SDL_Texture* thoughtDeathAtlas;
    SDL_Texture* thoughtQuestAtlas;
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


// Spusti fyzickou cestu jiz prirazeneho Serfa
// na volnou dlazdici pred entrance zdrojoveho Store.
//
// Ware ze Store se zde NEODEBIRA.
// Pickup prijde az po skutecnem dokonceni cesty.
bool startSerfTransportToSource(
    Unit* serf,
    const Building* buildings,
    int buildingCount,
    int mapWidth,
    int mapHeight,
    int tileSize
);


// Spusti cestu Serfa, ktery uz fyzicky nese ware,
// na volnou dlazdici pred entrance cilove budovy.
//
// Ware se zde NEODEVZDAVA.
// Pokud pathfinding selze, Serf zustane Carrying
// a fyzicky naklad mu zustane.
bool startSerfTransportToDestination(
    Unit* serf,
    const Building* buildings,
    int buildingCount,
    int mapWidth,
    int mapHeight,
    int tileSize
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
