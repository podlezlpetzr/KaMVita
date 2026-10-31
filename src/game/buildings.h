#pragma once

#include <SDL2/SDL.h>
#include "../render/kam_plan_renderer.h"

enum class BuildingType {
    Store = 0,
    Bakery = 1,
    Farm = 2,
    School = 3
};

// Originalni KaM WareType poradi pouzivane Store GUI.
// Poradi odpovida STORE_RES_COUNT a GUI ikonam 351..378.
enum class WareType {
    Trunk = 0,
    Stone,
    Wood,
    IronOre,
    GoldOre,
    Coal,
    Steel,
    Gold,
    Wine,
    Corn,
    Bread,
    Flour,
    Leather,
    Sausage,
    Pig,
    Skin,
    WoodenShield,
    IronShield,
    LeatherArmor,
    IronArmor,
    Axe,
    Sword,
    Lance,
    Pike,
    Bow,
    Crossbow,
    Horse,
    Fish,

    Count
};

static constexpr int STORE_WARE_COUNT =
    static_cast<int>(WareType::Count);

static_assert(
    STORE_WARE_COUNT == 28,
    "KaM Store musi obsahovat presne 28 ware typu"
);

static constexpr int STORE_GUI_FIRST_WARE_SPRITE = 351;
static constexpr int STORE_GUI_ACCEPT_SPRITE = 49;

inline int wareTypeToIndex(WareType type)
{
    return static_cast<int>(type);
}

inline int getWareGUIIcon(WareType type)
{
    return
        STORE_GUI_FIRST_WARE_SPRITE +
        wareTypeToIndex(type);
}

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

    // House runtime stav podle puvodniho KaM.
    // MaxHealth = (WoodCost + StoneCost) * 50.
    int health;
    int maxHealth;

    // Originalni vychozi stavy:
    // repair OFF, ware delivery ON.
    bool repairEnabled;
    bool wareDeliveryEnabled;

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

    // Store inventory podle puvodniho KaM.
    //
    // Index odpovida WareType a originalnimu poradi
    // StoreResType. Hodnota je skutecny pocet kusu ware
    // uvnitr skladu.
    int storeWares[STORE_WARE_COUNT];

    // false = Store ware prijima
    // true  = prijem daneho ware je zakazan
    //
    // Nazev a vyznam odpovida Remake NotAcceptFlag.
    bool storeNotAccept[STORE_WARE_COUNT];

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
    int tileSize,
    const KaMPlanResources* planResources
);


void getBuildingTopLeftFromEntrance(
    BuildingType type,
    int entranceTileX,
    int entranceTileY,
    int* topLeftTileX,
    int* topLeftTileY
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
    SDL_Texture* schoolWorkAtlas;

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
