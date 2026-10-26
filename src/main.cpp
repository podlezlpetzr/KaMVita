#include "render/terrain.h"
#include "game/buildings.h"
#include "game/units.h"
#include "ui/game_ui.h"
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




static bool saveGame(const Building* buildings, int buildingCount)
{
    sceIoMkdir("ux0:data/kamvita", 0777);

    FILE* file = fopen("ux0:data/kamvita/save.dat", "wb");
    if (!file)
        return false;

    SaveHeader header;
    header.magic = 0x4B414D56;
    header.version = 5;
    header.buildingCount = buildingCount;

    if (fwrite(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return false;
    }

    if (fwrite(buildings, sizeof(Building), MAX_BUILDINGS, file) != MAX_BUILDINGS) {
        fclose(file);
        return false;
    }

    fclose(file);
    return true;
}

static bool loadGame(Building* buildings, int* buildingCount)
{
    FILE* file = fopen("ux0:data/kamvita/save.dat", "rb");
    if (!file)
        return false;

    SaveHeader header = {};

    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return false;
    }

    if (header.magic != 0x4B414D56 ||
        (header.version != 4 &&
         header.version != 5) ||
        header.buildingCount < 0 ||
        header.buildingCount > MAX_BUILDINGS) {
        fclose(file);
        return false;
    }

    Building loadedBuildings[MAX_BUILDINGS] = {};

    if (fread(loadedBuildings, sizeof(Building), MAX_BUILDINGS, file)
        != MAX_BUILDINGS) {
        fclose(file);
        return false;
    }

    fclose(file);

    // Save v4 obsahoval School pole, ale fronta jeste
    // nebyla aktivne pouzivana a prazdne sloty byly nuly.
    // Od v5 je prazdny slot vzdy -1.
    if (header.version == 4) {
        for (int i = 0; i < MAX_BUILDINGS; ++i) {
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

                loadedBuildings[i].schoolTrainingStep = 0;
                loadedBuildings[i].schoolIdleStep = 0;
                loadedBuildings[i].schoolTrainingActive = false;
                loadedBuildings[i].schoolFinishingTraining = false;
            }
        }
    }

    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        if (loadedBuildings[i].active) {
            bool validPosition =
                loadedBuildings[i].tileX >= 0 &&
                loadedBuildings[i].tileY >= 0 &&
                loadedBuildings[i].tileX + 2 < MAP_WIDTH &&
                loadedBuildings[i].tileY + 2 < MAP_HEIGHT;

            if (!validPosition)
                loadedBuildings[i].active = false;
        }

        buildings[i] = loadedBuildings[i];
    }

    *buildingCount = header.buildingCount;
    return true;
}


enum class BuildTool {
    Road,
    Building
};

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
    BuildingResources buildingResources = loadBuildingResources(renderer);
    UnitResources unitResources = loadUnitResources(renderer);

    SchoolUIResources schoolUIResources =
        loadSchoolUIResources(renderer);

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

    Unit units[MAX_UNITS] = {};
    int unitCount = 0;

    // Zlomkova cast School training kroku.
    // Cely progress je ulozen v Building::schoolTrainingStep
    // a Building::schoolIdleStep.
    float schoolStepTimers[MAX_BUILDINGS] = {};

    bool gameLoaded =
        loadGame(buildings, &buildingCount);

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

        // TRAIN ve School GUI.
        // Tento rez pouze radi profesi do fronty.
        // Samotny casovy vycvik a spawn po completion
        // budou implementovany v dalsim rezu.
        if (
            gameUI.section ==
                GameUISection::School &&
            crossPressed
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

                    float targetX =
                        ((int)cursorX +
                         (int)cameraX) /
                        TILE_SIZE;

                    float targetY =
                        ((int)cursorY +
                         (int)cameraY) /
                        TILE_SIZE;

                    targetX =
                        (targetX + 0.5f) * TILE_SIZE;

                    targetY =
                        (targetY + 0.5f) * TILE_SIZE;

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

                    if (pathFound &&
                        cursorTileX >= 0 &&
                        cursorTileX < MAP_WIDTH &&
                        cursorTileY >= 0 &&
                        cursorTileY < MAP_HEIGHT &&
                        roads[cursorTileY][cursorTileX] ==
                            RoadState::Plan) {

                        Unit& worker =
                            units[unitIndex];

                        worker.targetRoadX =
                            cursorTileX;

                        worker.targetRoadY =
                            cursorTileY;

                        worker.workTimer = 0.0f;

                        int workerTileX =
                            (int)(worker.worldX /
                                  TILE_SIZE);

                        int workerTileY =
                            (int)(worker.worldY /
                                  TILE_SIZE);

                        // Pokud uz Worker stoji na planu,
                        // path je prazdna. Praci proto
                        // spustime okamzite.
                        if (workerTileX == cursorTileX &&
                            workerTileY == cursorTileY) {

                            worker.state =
                                UnitState::Working;

                            // Zachovej smer, ze ktereho
                            // Worker k praci prisel.
                            worker.animationFrame = 0;
                            worker.animationTimer = 0.0f;
                        }
                    } else {
                        units[unitIndex].targetRoadX = -1;
                        units[unitIndex].targetRoadY = -1;
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

        if (startPressed)
            saveGame(buildings, buildingCount);
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
        // Pevny KaM BUILD panel
        // -------------------------------------------------
        if (gameUI.section == GameUISection::Build) {
            const int uiSlotSize = 58;
            const int uiSlotGap = 6;
            const int uiStartX = 12;
            const int uiStartY = 295;
            const int uiColumns = 3;

            int hoveredBuildItem = -1;

            for (int i = 0; i < 5; ++i) {
                int column = i % uiColumns;
                int row = i / uiColumns;

                int slotX =
                    uiStartX +
                    column *
                    (uiSlotSize + uiSlotGap);

                int slotY =
                    uiStartY +
                    row *
                    (uiSlotSize + uiSlotGap);

                if (cursorX >= slotX &&
                    cursorX < slotX + uiSlotSize &&
                    cursorY >= slotY &&
                    cursorY < slotY + uiSlotSize) {

                    hoveredBuildItem = i;
                    break;
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
            gameUI.section == GameUISection::School
        ) {
            gameUI.section =
                GameUISection::Build;

            gameUI.selectedSchoolBuilding = -1;
            gameUI.buildSectionActive = true;

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

        if (!buildMenuOpen &&
            crossPressed &&
            hoverTileX >= 0 &&
            hoverTileX < MAP_WIDTH &&
            hoverTileY >= 0 &&
            hoverTileY < MAP_HEIGHT) {

            bool openedSchool = false;

            // Nejdrive zkusime vybrat dokoncenou School.
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

                gameUI.buildSectionActive = false;

                openedSchool = true;

                selectedTileX = -1;
                selectedTileY = -1;

                break;
            }

            if (!openedSchool &&
                gameUI.section ==
                    GameUISection::Build) {

                selectedTileX = hoverTileX;
                selectedTileY = hoverTileY;
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

            if (!blockedByBuilding) {
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

        if (!buildMenuOpen &&
            activeBuildTool == BuildTool::Building &&
            squarePressed &&
            canPlaceBuilding(
                buildings,
                buildingCount,
                selectedBuildingType,
                selectedTileX,
                selectedTileY,
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

                buildings[freeSlot].tileX = selectedTileX;
                buildings[freeSlot].tileY = selectedTileY;
                buildings[freeSlot].type = selectedBuildingType;
                buildings[freeSlot].constructionProgress = 0;
                buildings[freeSlot].woodDelivered = 0;
                buildings[freeSlot].stoneDelivered = 0;
                buildings[freeSlot].woodStep = 0;
                buildings[freeSlot].stoneStep = 0;
                buildings[freeSlot].active = true;

                // Po polozeni planu uz nechceme
                // automaticky pokladat dalsi stejnou budovu.
                activeBuildTool = BuildTool::Road;

                selectedTileX = -1;
                selectedTileY = -1;
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
                    TILE_SIZE
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
            gameUI,
            buildings,
            buildingCount,
            cursorX,
            cursorY
        );

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


    destroyUnitResources(&unitResources);
    destroyBuildingResources(&buildingResources);
    destroyTerrainResources(&terrain);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
