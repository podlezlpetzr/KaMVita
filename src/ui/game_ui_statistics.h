#pragma once

#include "../game/buildings.h"
#include "../game/units.h"


struct GameUIStatistics {
    // ---------------------------------------------
    // Buildings
    // ---------------------------------------------
    int storeCount;
    int bakeryCount;
    int farmCount;
    int schoolCount;

    // Rozestavene budovy podle modelu KaMVita.
    int storeWip;
    int bakeryWip;
    int farmWip;
    int schoolWip;

    // ---------------------------------------------
    // Units
    // ---------------------------------------------
    int workerCount;
    int serfCount;

    // ---------------------------------------------
    // School souhrn
    // ---------------------------------------------
    int schoolGold;
    int schoolTrainingCount;
    int schoolQueueCount;
};


// Vypocita Statistics pouze ze skutecneho runtime stavu.
// Zadna synteticka nebo odhadnuta data.
GameUIStatistics calculateGameUIStatistics(
    const Building* buildings,
    int buildingCount,
    const Unit* units,
    int unitCount
);


struct BuildingResources;
struct SchoolUIResources;


// Vykresli skutecny Remake-style Statistics panel.
// Pouziva pouze hodnoty z calculateGameUIStatistics().
void renderGameUIStatistics(
    SDL_Renderer* renderer,
    const BuildingResources& buildingResources,
    const SchoolUIResources& uiResources,
    const GameUIStatistics& stats
);
