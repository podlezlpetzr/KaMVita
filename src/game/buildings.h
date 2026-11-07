#pragma once

#include <SDL2/SDL.h>
#include "../render/kam_plan_renderer.h"

struct Unit;

enum class BuildingType {
    Store = 0,
    Bakery = 1,
    Farm = 2,
    School = 3,
    Quarry = 4,
    Mill = 5
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

    // =====================================================
    // GENERAL HOUSE WARE INVENTORY
    //
    // Remake TKMHouse:
    //   fResourceIn[1..4]
    //   fResourceDeliveryCount[1..4]
    //   fResourceOut[1..4]
    //
    // resourceIn:
    // skutecne dorucene vstupni suroviny.
    //
    // resourceDeliveryCount:
    // pocet kusu rezervovanych aktivni dopravou.
    // Zabranuje preplneni domu vice Serfy.
    //
    // resourceOut:
    // skutecne vyrobene suroviny cekajici na odvoz.
    // =====================================================

    int resourceIn[4];
    int resourceDeliveryCount[4];
    int resourceOut[4];

    // =====================================================
    // BAKERY PRODUCTION RUNTIME
    //
    // Originalni Bakery workplan:
    // 1 Flour -> 2 Bread
    // 7x Work2(24) + Work3(30)
    // celkem 378 House animacnich kroku.
    //
    // Flour se odebere pri startu cyklu.
    // Bread se prida az po dokonceni celeho cyklu.
    // =====================================================

    bool bakeryProductionActive;
    int bakeryProductionTick;

    bool schoolTrainingActive;
    bool schoolFinishingTraining;

    bool active;
};

static constexpr int MAX_BUILDINGS = 32;

// KaMVita Store capacity for one ware type.
//
// Store GUI represents one ware stack up to 32 pieces.
// Delivery must stop before exceeding that visible stock.
static constexpr int STORE_WARE_CAPACITY = 32;


// ============================================================
// STORE WARE API
//
// Herni systemy vcetne Serf logistiky nesmi menit
// Building::storeWares primo.
//
// Vsechny funkce overuji:
// - platny Store
// - dokoncenou budovu
// - platny WareType
// - kladne mnozstvi
//
// canStoreAcceptWare respektuje take:
// - globalni House ware delivery toggle
// - Store NotAcceptFlag daneho ware
// ============================================================

bool isValidWareType(
    WareType ware
);

int getStoreWareAmount(
    const Building& store,
    WareType ware
);

bool canStoreAcceptWare(
    const Building& store,
    WareType ware,
    int amount = 1
);

bool addStoreWare(
    Building* store,
    WareType ware,
    int amount = 1
);

bool canStoreTakeWare(
    const Building& store,
    WareType ware,
    int amount = 1
);

bool takeStoreWare(
    Building* store,
    WareType ware,
    int amount = 1
);


// ============================================================
// GENERAL HOUSE INPUT WARE API
//
// Bakery:
//   resourceIn[0] = Flour
//
// Remake normal House input capacity = 5.
// resourceDeliveryCount rezervuje ware na ceste.
// ============================================================

static constexpr int HOUSE_INPUT_CAPACITY = 5;
static constexpr int HOUSE_RESOURCE_SLOT_COUNT = 4;

int getHouseInputSlot(
    BuildingType type,
    WareType ware
);

int getHouseInputAmount(
    const Building& building,
    WareType ware
);

int getHouseIncomingAmount(
    const Building& building,
    WareType ware
);

bool canHouseAcceptWare(
    const Building& building,
    WareType ware,
    int amount = 1
);

bool reserveHouseWareDelivery(
    Building* building,
    WareType ware,
    int amount = 1
);

bool cancelHouseWareDelivery(
    Building* building,
    WareType ware,
    int amount = 1
);

bool deliverHouseWare(
    Building* building,
    WareType ware,
    int amount = 1
);


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
    const KaMPlanResources* planResources,
    bool bakeryHasInsideBaker,
    bool farmHasInsideFarmer,
    const Unit* units,
    int unitCount,
    int buildingIndexBase
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

    // Originalni Store ha_Flag1 / ha_Flag2.
    SDL_Texture* storeFlag1Atlas;
    SDL_Texture* storeFlag2Atlas;

    SDL_Texture* bakeryWoodAtlas;
    SDL_Texture* bakeryStoneAtlas;

    // Originalni Bakery House work overlaye z houses.rx.
    SDL_Texture* bakeryWork2Atlas;
    SDL_Texture* bakeryWork3Atlas;

    // Originalni Bakery ha_Idle, RX 1077..1082.
    SDL_Texture* bakeryIdleAtlas;

    // Originalni Bakery ha_Flagpole / ha_Flag1.
    SDL_Texture* bakeryFlagpole;
    SDL_Texture* bakeryFlag1Atlas;

    // Originalni Bakery ha_Smoke, RX 889..892.
    SDL_Texture* bakerySmokeAtlas;

    // Originalni Bakery SupplyOut[1], RX 105..109.
    SDL_Texture* bakeryBreadSupplyAtlas;

    SDL_Texture* farmWoodAtlas;
    SDL_Texture* farmStoneAtlas;

    // Originalni Farm ha_Idle z houses.rx.
    //
    // House index 8:
    // count  = 30
    // cell   = 18x17
    // anchor = (8,-6)
    // Move   = (-16,-11)
    SDL_Texture* farmIdleAtlas;

    // Originalni Farm ha_Flag1.
    //
    // Farm nema ha_Flagpole, ha_Flag2 ani ha_Flag3.
    //
    // Count  = 10
    // cell   = 44x15
    // anchor = (-46,-18)
    // Move   = (-1,-69)
    SDL_Texture* farmFlag1Atlas;

    // Originalni Farm SupplyOut[1] pro wt_Corn.
    //
    // houses.dat:
    // DAT 386..390
    //
    // houses.rx:
    // RX 387..391
    //
    // 5 quantity states:
    // resourceOut[0] = 1..5+
    //
    // Atlas:
    // cell   = 45x34
    // anchor = (20,-3)
    SDL_Texture* farmCornSupplyAtlas;

    SDL_Texture* schoolWoodAtlas;
    SDL_Texture* schoolStoneAtlas;

    // Original Quarry construction from houses.rx.
    SDL_Texture* quarryWoodAtlas;
    SDL_Texture* quarryStoneAtlas;

    // Original Mill construction from houses.rx:
    // Wood  RX 359 + mask 361, 21 steps.
    // Stone RX 358 + mask 360, 31 steps.
    SDL_Texture* millWoodAtlas;
    SDL_Texture* millStoneAtlas;

    // Optional completed Mill WoodPic loaded directly
    // from external original houses.rx, sprite selected
    // by the Mill houses.dat record, currently RX 359.
    //
    // If unavailable, millWoodAtlas remains the packaged
    // construction fallback.
    SDL_Texture* millRuntimeWoodTexture;

    // Runtime Mill wooden construction generated directly
    // from original WoodPic RX359 + WoodMask RX361.
    //
    // Mask rule from the original construction generator:
    // mask != 0 && mask < construction step.
    //
    // Index 0 is unused by rendering. Steps 1..21 map
    // directly to the original DAT WoodSteps progression.
    SDL_Texture* millRuntimeWoodSteps[22];

    // Optional completed Mill StonePic loaded directly
    // from external original houses.rx, sprite RX 358.
    //
    // If unavailable, the final frame of millStoneAtlas
    // remains the complete packaged fallback.
    SDL_Texture* millRuntimeStoneTexture;

    // Original Mill ha_Flagpole turbine:
    // RX 348..357, 10 frames.
    // cell   = 106x147
    // grid   = 5x2
    // anchor = (-35,-34)
    // Move   = (-17,-80)
    SDL_Texture* millTurbineAtlas;

    // Original Mill ha_Idle:
    // RX 1095..1100 in original 30-frame sequence.
    // cell   = 17x16
    // anchor = (4,-52)
    // Move   = (0,-1)
    SDL_Texture* millIdleAtlas;
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

// Optional WoodCost / StoneCost values loaded from
// external original houses.dat.
//
// If external data is unavailable, existing values in
// getBuildingWoodCost/getBuildingStoneCost remain fallback.
void clearBuildingRuntimeCosts();

bool setBuildingRuntimeCosts(
    BuildingType type,
    int woodCost,
    int stoneCost
);

bool hasBuildingRuntimeCosts(
    BuildingType type
);

int getBuildingWoodCost(BuildingType type);
int getBuildingStoneCost(BuildingType type);

// Optional WoodSteps / StoneSteps loaded from external
// original houses.dat.
//
// Existing hardcoded values remain the fallback.
void clearBuildingRuntimeSteps();

bool setBuildingRuntimeSteps(
    BuildingType type,
    int woodSteps,
    int stoneSteps
);

bool hasBuildingRuntimeSteps(
    BuildingType type
);

int getBuildingWoodSteps(BuildingType type);
int getBuildingStoneSteps(BuildingType type);

bool isBuildingComplete(const Building& building);

struct BuildingEntrance {
    int tileOffsetX;
    int tileOffsetY;
    int pixelOffsetX;
    int pixelOffsetY;
};

// Optional EntranceOffsetPx loaded from external
// original houses.dat.
//
// Only pixel offsets are overridden. The logical
// KaMVita entrance tile remains unchanged.
void clearBuildingRuntimeEntrancePixels();

bool setBuildingRuntimeEntrancePixels(
    BuildingType type,
    int pixelOffsetX,
    int pixelOffsetY
);

bool hasBuildingRuntimeEntrancePixels(
    BuildingType type
);

BuildingEntrance getBuildingEntrance(
    BuildingType type
);
