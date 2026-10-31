#include "render/terrain.h"
#include "render/kam_plan_renderer.h"
#include "game/buildings.h"
#include "game/units.h"
#include "ui/game_ui.h"
#include "ui/game_ui_unit.h"
#include "ui/game_ui_ratios.h"
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


static constexpr int SCREEN_WIDTH  = 960;
static constexpr int SCREEN_HEIGHT = 544;
static constexpr int TILE_SIZE = 32;
static constexpr int MAP_WIDTH = 64;
static constexpr int MAP_HEIGHT = 64;
static constexpr int WORLD_WIDTH = MAP_WIDTH * TILE_SIZE;
static constexpr int WORLD_HEIGHT = MAP_HEIGHT * TILE_SIZE;


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




static bool saveGame(
    const Building* buildings,
    int buildingCount,
    const WareDistribution& wareDistribution
)
{
    sceIoMkdir(
        "ux0:data/kamvita",
        0777
    );

    FILE* file = fopen(
        "ux0:data/kamvita/save.dat",
        "wb"
    );

    if (!file)
        return false;

    SaveHeader header = {};

    header.magic = 0x4B414D56;

    // Save v7:
    // za blokem budov je ulozenych presne
    // 16 bajtu WareDistribution.
    header.version = 7;
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

    fclose(file);
    return true;
}

static bool loadGame(
    Building* buildings,
    int* buildingCount,
    WareDistribution* wareDistribution
)
{
    // Remake defaults plati pro novou hru,
    // chybejici save a starsi save v4-v6.
    initializeWareDistribution(
        wareDistribution
    );

    FILE* file =
        fopen(
            "ux0:data/kamvita/save.dat",
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
            header.version != 7
        ) ||
        header.buildingCount < 0 ||
        header.buildingCount > MAX_BUILDINGS
    ) {
        fclose(file);
        return false;
    }

    Building loadedBuildings[MAX_BUILDINGS] = {};

    if (
        header.version == 6 ||
        header.version == 7
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
            int i = 0;
            i < MAX_BUILDINGS;
            ++i
        ) {
            loadedBuildings[i] =
                migrateBuildingSaveV5(
                    legacyBuildings[i]
                );
        }
    }

    if (header.version == 7) {
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

    fclose(file);

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
        }

        buildings[i] = loadedBuildings[i];
    }

    *buildingCount = header.buildingCount;
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
    TerrainResources terrain = loadTerrainResources(renderer);

    KaMPlanResources kamPlanResources =
        loadKaMPlanResources(renderer);
    BuildingResources buildingResources = loadBuildingResources(renderer);
    UnitResources unitResources = loadUnitResources(renderer);

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

    bool gameLoaded =
        loadGame(
            buildings,
            &buildingCount,
            &wareDistribution
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

        bool startPressed =
            (pad.buttons & SCE_CTRL_START) &&
            !(previousButtons & SCE_CTRL_START);

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
                        break;

                    default:
                        break;
                }

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

        if (startPressed) {
            saveGame(
                buildings,
                buildingCount,
                wareDistribution
            );
        }
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
                    else
                        selectedBuildingType =
                            BuildingType::School;

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
                gameUI.section == GameUISection::Store
            )
        ) {
            gameUI.section =
                GameUISection::Build;

            gameUI.selectedSchoolBuilding = -1;
            gameUI.selectedStoreBuilding = -1;
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

                // Nejdrive zkusime vybrat dokoncenou School.

            if (!openedStore) {
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
                        LandPlanState::WineInit ||
                    landPlan ==
                        LandPlanState::WineComplete
                ) {
                    SDL_Texture* completedTexture =
                        landPlan ==
                            LandPlanState::FieldComplete
                                ? terrain.cornEmpty
                                : terrain.wineGround;

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
                        LandPlanState::FieldComplete;

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
                                LandPlanState::FieldComplete;
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

                trainedUnit.type =
                    profession ==
                        static_cast<int>(
                            GameUISchoolSelection::Serf
                        )
                        ? UnitType::Serf
                        : UnitType::Worker;

                trainedUnit.condition =
                    UNIT_START_CONDITION;

                BuildingEntrance entrance =
                    getBuildingEntrance(
                        school.type
                    );

                // Logicky stred vstupni dlazdice.
                trainedUnit.doorwayStartX =
                    (
                        school.tileX +
                        entrance.tileOffsetX +
                        0.5f
                    ) * TILE_SIZE -
                    18.0f;

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
                renderBuildings(
                    renderer,
                    buildingResources,
                    &buildings[entry.index],
                    1,
                    cameraX,
                    cameraY,
                    TILE_SIZE,
                    &kamPlanResources
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
