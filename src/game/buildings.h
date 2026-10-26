#pragma once

#include <SDL2/SDL.h>

enum class BuildingType {
    Store = 0,
    Bakery = 1,
    Farm = 2,
    School = 3
};

struct Building {
    int tileX;
    int tileY;
    BuildingType type;

    // Docasne ponechano, dokud neprepojime renderer.
    int constructionProgress;

    // Skutecny stavebni stav ve stylu puvodniho KaM.
    int woodDelivered;
    int stoneDelivered;
    int woodStep;
    int stoneStep;

    // School: originalni fronta KaM.
    // [0] = prave trenovana jednotka
    // [1..5] = cekajici jednotky
    // -1 = prazdny slot
    int schoolQueue[6];

    // 0..149 = Work1..Work5
    int schoolTrainingStep;

    // Po 150 treninkovych krocich
    // nasleduje 10 kroku idle.
    int schoolIdleStep;

    // Zasoba zlata uvnitr School.
    int schoolGold;

    bool schoolTrainingActive;
    bool schoolFinishingTraining;

    bool active;
};

static constexpr int MAX_BUILDINGS = 32;

bool buildingsOverlap(
    const Building& a,
    BuildingType type,
    int tileX,
    int tileY
);

struct BuildingResources;

void renderBuildings(
    SDL_Renderer* renderer,
    const BuildingResources& resources,
    const Building* buildings,
    int buildingCount,
    float cameraX,
    float cameraY,
    int tileSize
);

bool canPlaceBuilding(
    const Building* buildings,
    int buildingCount,
    BuildingType type,
    int tileX,
    int tileY,
    int mapWidth,
    int mapHeight
);


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
);

struct BuildingResources {
    SDL_Texture* store;
    SDL_Texture* bakery;
    SDL_Texture* farm;
    SDL_Texture* school;
    SDL_Texture* schoolFlagpole;
    SDL_Texture* schoolFlagAtlas;

    SDL_Texture* roadIcon;
    SDL_Texture* storeIcon;
    SDL_Texture* bakeryIcon;
    SDL_Texture* farmIcon;
    SDL_Texture* schoolIcon;

    SDL_Texture* storeWoodAtlas;
    SDL_Texture* storeStoneAtlas;

    SDL_Texture* bakeryWoodAtlas;
    SDL_Texture* bakeryStoneAtlas;

    SDL_Texture* farmWoodAtlas;
    SDL_Texture* farmStoneAtlas;

    SDL_Texture* schoolWoodAtlas;
    SDL_Texture* schoolStoneAtlas;
};

BuildingResources loadBuildingResources(
    SDL_Renderer* renderer
);

void destroyBuildingResources(
    BuildingResources* resources
);

int getBuildingWidth(BuildingType type);
int getBuildingHeight(BuildingType type);

int getBuildingPlanValue(
    BuildingType type,
    int localX,
    int localY
);

int getBuildingWoodCost(BuildingType type);
int getBuildingStoneCost(BuildingType type);
int getBuildingWoodSteps(BuildingType type);
int getBuildingStoneSteps(BuildingType type);

bool isBuildingComplete(const Building& building);

struct BuildingEntrance {
    int tileOffsetX;
    int tileOffsetY;
    int pixelOffsetX;
    int pixelOffsetY;
};

BuildingEntrance getBuildingEntrance(
    BuildingType type
);
