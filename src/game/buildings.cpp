#include "buildings.h"

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


bool buildingsOverlap(
    const Building& a,
    BuildingType type,
    int tileX,
    int tileY
)
{
    if (!a.active)
        return false;

    int candidateWidth = getBuildingWidth(type);
    int candidateHeight = getBuildingHeight(type);

    int existingWidth = getBuildingWidth(a.type);
    int existingHeight = getBuildingHeight(a.type);

    int candidateRight = tileX + candidateWidth - 1;
    int candidateBottom = tileY + candidateHeight - 1;

    int existingRight = a.tileX + existingWidth - 1;
    int existingBottom = a.tileY + existingHeight - 1;

    bool separated =
        candidateRight < a.tileX ||
        tileX > existingRight ||
        candidateBottom < a.tileY ||
        tileY > existingBottom;

    return !separated;
}

void renderBuildings(
    SDL_Renderer* renderer,
    const BuildingResources& resources,
    const Building* buildings,
    int buildingCount,
    float cameraX,
    float cameraY,
    int tileSize
)
{
    for (int i = 0; i < buildingCount; ++i) {
        if (!buildings[i].active)
            continue;

        const Building& building = buildings[i];

        int footprintX =
            building.tileX * tileSize - (int)cameraX;

        int footprintY =
            building.tileY * tileSize - (int)cameraY;

        int buildingWidth =
            getBuildingWidth(building.type);

        int buildingHeight =
            getBuildingHeight(building.type);

        int footprintWidth =
            buildingWidth * tileSize;

        int footprintHeight =
            buildingHeight * tileSize;

        SDL_Texture* woodAtlas = nullptr;
        SDL_Texture* stoneAtlas = nullptr;

        int woodFrameWidth = 0;
        int woodFrameHeight = 0;
        int woodColumns = 1;
        int woodMaximumStep = 0;
        int woodPivotX = 0;
        int woodPivotY = 0;

        int stoneFrameWidth = 0;
        int stoneFrameHeight = 0;
        int stoneColumns = 1;
        int stoneMaximumStep = 0;
        int stonePivotX = 0;
        int stonePivotY = 0;

        switch (building.type) {
            case BuildingType::Store:
                woodAtlas = resources.storeWoodAtlas;
                stoneAtlas = resources.storeStoneAtlas;

                woodFrameWidth = 134;
                woodFrameHeight = 117;
                woodColumns = 5;
                woodMaximumStep = 24;
                woodPivotX = -69;
                woodPivotY = -82;

                stoneFrameWidth = 144;
                stoneFrameHeight = 136;
                stoneColumns = 10;
                stoneMaximumStep = 75;
                stonePivotX = -78;
                stonePivotY = -99;
                break;

            case BuildingType::Bakery:
                woodAtlas = resources.bakeryWoodAtlas;
                stoneAtlas = resources.bakeryStoneAtlas;

                woodFrameWidth = 130;
                woodFrameHeight = 122;
                woodColumns = 6;
                woodMaximumStep = 21;
                woodPivotX = -38;
                woodPivotY = -92;

                stoneFrameWidth = 140;
                stoneFrameHeight = 131;
                stoneColumns = 11;
                stoneMaximumStep = 54;
                stonePivotX = -44;
                stonePivotY = -98;
                break;

            case BuildingType::Farm:
                woodAtlas = resources.farmWoodAtlas;
                stoneAtlas = resources.farmStoneAtlas;

                woodFrameWidth = 166;
                woodFrameHeight = 107;
                woodColumns = 9;
                woodMaximumStep = 26;
                woodPivotX = -73;
                woodPivotY = -74;

                stoneFrameWidth = 175;
                stoneFrameHeight = 124;
                stoneColumns = 11;
                stoneMaximumStep = 43;
                stonePivotX = -78;
                stonePivotY = -91;
                break;

            case BuildingType::School:
                woodAtlas = resources.schoolWoodAtlas;
                stoneAtlas = resources.schoolStoneAtlas;

                woodFrameWidth = 149;
                woodFrameHeight = 108;
                woodColumns = 6;
                woodMaximumStep = 21;
                woodPivotX = -79;
                woodPivotY = -90;

                stoneFrameWidth = 171;
                stoneFrameHeight = 142;
                stoneColumns = 11;
                stoneMaximumStep = 65;
                stonePivotX = -89;
                stonePivotY = -107;
                break;
        }

        int woodStep = building.woodStep;
        int stoneStep = building.stoneStep;

        if (woodStep < 0)
            woodStep = 0;

        if (woodStep > woodMaximumStep)
            woodStep = woodMaximumStep;

        if (stoneStep < 0)
            stoneStep = 0;

        if (stoneStep > stoneMaximumStep)
            stoneStep = stoneMaximumStep;

        // Pokud uz zacal kamen, drevena konstrukce musi byt cela.
        int renderedWoodStep =
            stoneStep > 0
                ? woodMaximumStep
                : woodStep;

        // Bez jedineho pracovniho kroku je videt stavebni plan.
        if (renderedWoodStep == 0 && stoneStep == 0) {
            for (int localY = 0;
                 localY < buildingHeight;
                 ++localY) {

                for (int localX = 0;
                     localX < buildingWidth;
                     ++localX) {

                    int planValue = getBuildingPlanValue(
                        building.type,
                        localX,
                        localY
                    );

                    if (planValue == 0)
                        continue;

                    SDL_Rect planTile = {
                        footprintX + localX * tileSize,
                        footprintY + localY * tileSize,
                        tileSize,
                        tileSize
                    };

                    SDL_SetRenderDrawColor(
                        renderer,
                        178, 139, 76, 255
                    );

                    SDL_RenderDrawRect(
                        renderer,
                        &planTile
                    );

                    if (planValue == 2) {
                        SDL_Rect entrance = {
                            planTile.x + tileSize / 4,
                            planTile.y + tileSize / 4,
                            tileSize / 2,
                            tileSize / 2
                        };

                        SDL_SetRenderDrawColor(
                            renderer,
                            255, 185, 45, 255
                        );

                        SDL_RenderDrawRect(
                            renderer,
                            &entrance
                        );
                    }
                }
            }

            continue;
        }

        // Kamenna vrstva je kotva pro celou budovu.
        int stoneDestinationX =
            footprintX +
            (footprintWidth - stoneFrameWidth) / 2;

        int stoneDestinationY =
            footprintY +
            footprintHeight -
            stoneFrameHeight;


        // Originalni KaM School pouziva virtualni
        // fPosition na local (2, 2). Renderer pak
        // aplikuje pivot a odecte jednu dlazdici:
        //
        // destination =
        // footprint + TILE_SIZE + originalni pivot
        //
        // Centrovani sprite posouvalo School vuci
        // logickemu Entrance doprava a dolu.

        // Drevo se zarovna vuci kameni podle rozdilu
        // originalnich pivotu z houses.rx.
        int woodDestinationX =
            stoneDestinationX +
            (woodPivotX - stonePivotX);

        int woodDestinationY =
            stoneDestinationY +
            (woodPivotY - stonePivotY);

        if (woodAtlas && renderedWoodStep > 0) {
            int frameColumn =
                renderedWoodStep % woodColumns;

            int frameRow =
                renderedWoodStep / woodColumns;

            SDL_Rect source = {
                frameColumn * woodFrameWidth,
                frameRow * woodFrameHeight,
                woodFrameWidth,
                woodFrameHeight
            };

            SDL_Rect destination = {
                woodDestinationX,
                woodDestinationY,
                woodFrameWidth,
                woodFrameHeight
            };

            SDL_RenderCopy(
                renderer,
                woodAtlas,
                &source,
                &destination
            );
        }

        // Kamenna vrstva se vykresluje pres hotove drevo.
        if (stoneAtlas && stoneStep > 0) {
            int frameColumn =
                stoneStep % stoneColumns;

            int frameRow =
                stoneStep / stoneColumns;

            SDL_Rect source = {
                frameColumn * stoneFrameWidth,
                frameRow * stoneFrameHeight,
                stoneFrameWidth,
                stoneFrameHeight
            };

            SDL_Rect destination = {
                stoneDestinationX,
                stoneDestinationY,
                stoneFrameWidth,
                stoneFrameHeight
            };

            SDL_RenderCopy(
                renderer,
                stoneAtlas,
                &source,
                &destination
            );
        }

        // SCHOOL ORIGINAL FLAG OVERLAY
        //
        // House overlaye jsou originální sprite-pixelové
        // offsety. Proto je umistujeme relativne k
        // hotovemu StonePic, ne pres TILE_SIZE.
        if (
            building.type == BuildingType::School &&
            isBuildingComplete(building)
        ) {
            // School StonePic:
            // pivot = (-89, -107)
            //
            // Flagpole:
            // pivot = (-2, -24)
            // move  = (-16, -118)
            //
            // Flag1:
            // pivot = (-20, 18)
            // move  = (7, -153)

            if (resources.schoolFlagpole) {
                const int polePivotX = -2;
                const int polePivotY = -24;
                const int poleMoveX = -16;
                const int poleMoveY = -118;

                int poleX =
                    stoneDestinationX +
                    (
                        polePivotX +
                        poleMoveX -
                        stonePivotX
                    );

                int poleY =
                    stoneDestinationY +
                    (
                        polePivotY +
                        poleMoveY -
                        stonePivotY
                    );

                SDL_Rect poleDestination = {
                    poleX,
                    poleY,
                    9,
                    35
                };

                SDL_RenderCopy(
                    renderer,
                    resources.schoolFlagpole,
                    nullptr,
                    &poleDestination
                );
            }

            if (resources.schoolFlagAtlas) {
                const int flagWidth = 45;
                const int flagHeight = 22;
                const int flagFrames = 10;

                const int flagPivotX = -20;
                const int flagPivotY = 18;
                const int flagMoveX = 7;
                const int flagMoveY = -153;

                int flagFrame =
                    (SDL_GetTicks() / 120) %
                    flagFrames;

                SDL_Rect flagSource = {
                    flagFrame * flagWidth,
                    0,
                    flagWidth,
                    flagHeight
                };

                int flagX =
                    stoneDestinationX +
                    (
                        flagPivotX +
                        flagMoveX -
                        stonePivotX
                    );

                int flagY =
                    stoneDestinationY +
                    (
                        flagPivotY +
                        flagMoveY -
                        stonePivotY
                    );

                SDL_Rect flagDestination = {
                    flagX,
                    flagY,
                    flagWidth,
                    flagHeight
                };

                SDL_RenderCopy(
                    renderer,
                    resources.schoolFlagAtlas,
                    &flagSource,
                    &flagDestination
                );
            }

            // SCHOOL WORK1-WORK5 TRAINING OVERLAY
            //
            // Atlas ma 25 bunek 16x20:
            // Work1 0..4, Work2 5..9, ... Work5 20..24.
            // Skutecny rozmer a pivot zustava per-frame.
            if (
                resources.schoolWorkAtlas &&
                building.schoolTrainingActive &&
                building.schoolTrainingStep >= 0 &&
                building.schoolTrainingStep <
                    150
            ) {
                static const int workWidths[25] = {
                    14, 14, 14, 14, 14,
                    14, 14, 14, 14, 14,
                    14, 14, 14, 15, 15,
                    15, 15, 15, 15, 16,
                    15, 15, 15, 15, 15
                };

                static const int workHeights[25] = {
                    13, 13, 13, 13, 13,
                    13, 13, 20, 20, 20,
                    20, 20, 20, 20, 20,
                    20, 20, 20, 13, 13,
                    13, 13, 13, 13, 13
                };

                static const int workPivotX[25] = {
                    -5, -5, -5, -5, -5,
                    -5, -5, -5, -5, -5,
                    -5, -5, -5, -13, -13,
                    -13, -13, -13, -13, -13,
                    -13, -13, -13, -13, -13
                };

                static const int workPivotY[25] = {
                    -76, -76, -76, -76, -76,
                    -76, -76, -76, -76, -76,
                    -76, -76, -76, -76, -76,
                    -76, -76, -76, -76, -76,
                    -76, -76, -76, -76, -76
                };

                const int workCellWidth = 16;
                const int workCellHeight = 20;
                const int workFramesPerPhase = 5;
                const int workTrainingSteps = 150;
                const int workPhaseSteps = 30;
                const int workMoveX = -17;
                const int workMoveY = 5;

                int workPhase =
                    building.schoolTrainingStep /
                    workPhaseSteps;

                if (workPhase > 4)
                    workPhase = 4;

                // Frame je odvozen primo z training kroku.
                // Kazda Work faze ma 30 kroku a 5 RX snimku,
                // tedy jeden RX snimek pripada na 6 kroku.
                int phaseStep =
                    building.schoolTrainingStep %
                    30;

                int phaseFrame =
                    (
                        phaseStep *
                        workFramesPerPhase
                    ) /
                    30;

                if (
                    phaseFrame >=
                        workFramesPerPhase
                ) {
                    phaseFrame =
                        workFramesPerPhase - 1;
                }

                int workFrame =
                    workPhase * workFramesPerPhase +
                    phaseFrame;

                int frameWidth =
                    workWidths[workFrame];

                int frameHeight =
                    workHeights[workFrame];

                SDL_Rect workSource = {
                    workFrame * workCellWidth,
                    0,
                    frameWidth,
                    frameHeight
                };

                int workX =
                    stoneDestinationX +
                    (
                        workPivotX[workFrame] +
                        workMoveX -
                        stonePivotX
                    );

                int workY =
                    stoneDestinationY +
                    (
                        workPivotY[workFrame] +
                        workMoveY -
                        stonePivotY
                    );

                SDL_Rect workDestination = {
                    workX,
                    workY,
                    frameWidth,
                    frameHeight
                };

                SDL_RenderCopy(
                    renderer,
                    resources.schoolWorkAtlas,
                    &workSource,
                    &workDestination
                );
            }
        }

        // DEBUG SCHOOL ENTRANCE OVERLAY
        // Cerveny kriz = stred vstupni dlazdice.
        // Zluty bod = doorway pixelovy posun.
        if (
            building.type == BuildingType::School &&
            isBuildingComplete(building)
        ) {
            BuildingEntrance entrance =
                getBuildingEntrance(
                    building.type
                );

            int entranceCenterX =
                footprintX +
                entrance.tileOffsetX *
                    tileSize +
                tileSize / 2 -
                18;

            int entranceCenterY =
                footprintY +
                entrance.tileOffsetY *
                    tileSize +
                tileSize / 2;

            SDL_SetRenderDrawColor(
                renderer,
                255, 0, 0, 255
            );

            SDL_RenderDrawLine(
                renderer,
                entranceCenterX - 10,
                entranceCenterY,
                entranceCenterX + 10,
                entranceCenterY
            );

            SDL_RenderDrawLine(
                renderer,
                entranceCenterX,
                entranceCenterY - 10,
                entranceCenterX,
                entranceCenterY + 10
            );

            int doorwayVisualX =
                entranceCenterX +
                entrance.pixelOffsetX -
                tileSize / 2;

            int doorwayVisualY =
                entranceCenterY +
                entrance.pixelOffsetY;

            SDL_Rect doorwayPoint = {
                doorwayVisualX - 3,
                doorwayVisualY - 3,
                7,
                7
            };

            SDL_SetRenderDrawColor(
                renderer,
                255, 230, 0, 255
            );

            SDL_RenderFillRect(
                renderer,
                &doorwayPoint
            );
        }

    }
}

bool canPlaceBuilding(
    const Building* buildings,
    int buildingCount,
    BuildingType type,
    int tileX,
    int tileY,
    int mapWidth,
    int mapHeight
)
{
    if (tileX < 0 || tileY < 0)
        return false;

    int buildingWidth = getBuildingWidth(type);
    int buildingHeight = getBuildingHeight(type);

    bool insideMap =
        tileX + buildingWidth <= mapWidth &&
        tileY + buildingHeight <= mapHeight;

    if (!insideMap)
        return false;

    for (int localY = 0; localY < buildingHeight; ++localY) {
        for (int localX = 0; localX < buildingWidth; ++localX) {
            int planValue = getBuildingPlanValue(
                type,
                localX,
                localY
            );

            if (planValue == 0)
                continue;

            int worldTileX = tileX + localX;
            int worldTileY = tileY + localY;

            bool tileIsWater =
                worldTileX < 10 &&
                worldTileY < 12;

            if (tileIsWater)
                return false;
        }
    }

    for (int i = 0; i < buildingCount; ++i) {
        if (buildingsOverlap(
                buildings[i],
                type,
                tileX,
                tileY
            )) {
            return false;
        }
    }

    return true;
}

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
)
{
    if (selectedTileX < 0 || selectedTileY < 0)
        return;

    bool previewValid = canPlaceBuilding(
        buildings,
        buildingCount,
        selectedType,
        selectedTileX,
        selectedTileY,
        mapWidth,
        mapHeight
    );

    int buildingWidth = getBuildingWidth(selectedType);
    int buildingHeight = getBuildingHeight(selectedType);

    for (int localY = 0; localY < buildingHeight; ++localY) {
        for (int localX = 0; localX < buildingWidth; ++localX) {
            int planValue = getBuildingPlanValue(
                selectedType,
                localX,
                localY
            );

            if (planValue == 0)
                continue;

            SDL_Rect tile = {
                (selectedTileX + localX) * tileSize -
                    (int)cameraX,
                (selectedTileY + localY) * tileSize -
                    (int)cameraY,
                tileSize,
                tileSize
            };

            if (!previewValid) {
                SDL_SetRenderDrawColor(
                    renderer,
                    255, 65, 65, 255
                );
            } else if (planValue == 2) {
                SDL_SetRenderDrawColor(
                    renderer,
                    255, 185, 45, 255
                );
            } else {
                SDL_SetRenderDrawColor(
                    renderer,
                    80, 255, 100, 255
                );
            }

            SDL_RenderDrawRect(renderer, &tile);

            if (planValue == 2) {
                SDL_Rect entranceMarker = {
                    tile.x + tileSize / 4,
                    tile.y + tileSize / 4,
                    tileSize / 2,
                    tileSize / 2
                };

                SDL_RenderDrawRect(
                    renderer,
                    &entranceMarker
                );
            }
        }
    }

    SDL_Texture* previewTexture = nullptr;
    int spriteWidth = 0;
    int spriteHeight = 0;

    switch (selectedType) {
        case BuildingType::Store:
            previewTexture = resources.store;
            spriteWidth = 144;
            spriteHeight = 136;
            break;

        case BuildingType::Bakery:
            previewTexture = resources.bakery;
            spriteWidth = 140;
            spriteHeight = 131;
            break;

        case BuildingType::Farm:
            previewTexture = resources.farm;
            spriteWidth = 175;
            spriteHeight = 124;
            break;

        case BuildingType::School:
            previewTexture = resources.school;
            spriteWidth = 171;
            spriteHeight = 142;
            break;
    }

    if (previewTexture) {
        int footprintWidth =
            buildingWidth * tileSize;

        int footprintHeight =
            buildingHeight * tileSize;

        SDL_Rect destination = {
            selectedTileX * tileSize -
                (int)cameraX +
                (footprintWidth - spriteWidth) / 2,

            selectedTileY * tileSize -
                (int)cameraY +
                footprintHeight -
                spriteHeight,

            spriteWidth,
            spriteHeight
        };

        SDL_SetTextureBlendMode(
            previewTexture,
            SDL_BLENDMODE_BLEND
        );

        SDL_SetTextureAlphaMod(
            previewTexture,
            previewValid ? 145 : 95
        );

        SDL_SetTextureColorMod(
            previewTexture,
            255,
            previewValid ? 255 : 150,
            previewValid ? 255 : 150
        );

        SDL_RenderCopy(
            renderer,
            previewTexture,
            nullptr,
            &destination
        );

        SDL_SetTextureAlphaMod(
            previewTexture,
            255
        );

        SDL_SetTextureColorMod(
            previewTexture,
            255,
            255,
            255
        );
    }
}

BuildingResources loadBuildingResources(
    SDL_Renderer* renderer
)
{
    BuildingResources resources = {};

    SDL_Surface* surface =
        SDL_LoadBMP("app0:assets/buildings/store.bmp");

    if (surface) {
        SDL_SetColorKey(
            surface,
            SDL_TRUE,
            SDL_MapRGB(surface->format, 255, 0, 255)
        );
        resources.store =
            SDL_CreateTextureFromSurface(renderer, surface);

        SDL_FreeSurface(surface);
    }


    surface = SDL_LoadBMP("app0:assets/buildings/bakery.bmp");

    if (surface) {
        SDL_SetColorKey(
            surface,
            SDL_TRUE,
            SDL_MapRGB(surface->format, 255, 0, 255)
        );

        resources.bakery =
            SDL_CreateTextureFromSurface(renderer, surface);

        SDL_FreeSurface(surface);
    }

    surface = SDL_LoadBMP("app0:assets/buildings/farm.bmp");

    if (surface) {
        SDL_SetColorKey(
            surface,
            SDL_TRUE,
            SDL_MapRGB(surface->format, 255, 0, 255)
        );

        resources.farm =
            SDL_CreateTextureFromSurface(renderer, surface);

        SDL_FreeSurface(surface);
    }

    resources.roadIcon = loadColorKeyTexture(
        renderer,
        "app0:assets/ui/buildings/road_icon.bmp"
    );

    resources.storeIcon = loadColorKeyTexture(
        renderer,
        "app0:assets/ui/buildings/store_icon.bmp"
    );

    resources.bakeryIcon = loadColorKeyTexture(
        renderer,
        "app0:assets/ui/buildings/bakery_icon.bmp"
    );

    resources.farmIcon = loadColorKeyTexture(
        renderer,
        "app0:assets/ui/buildings/farm_icon.bmp"
    );
    resources.storeWoodAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/construction/store/wood_atlas.bmp"
    );

    resources.storeStoneAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/construction/store/stone_atlas.bmp"
    );

    resources.bakeryWoodAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/construction/bakery/wood_atlas.bmp"
    );

    resources.bakeryStoneAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/construction/bakery/stone_atlas.bmp"
    );

    resources.farmWoodAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/construction/farm/wood_atlas.bmp"
    );

    resources.farmStoneAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/construction/farm/stone_atlas.bmp"
    );

    resources.school = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/school.bmp"
    );

    resources.schoolFlagpole =
        loadColorKeyTexture(
            renderer,
            "app0:assets/buildings/school/runtime/flagpole/flagpole.bmp"
        );

    resources.schoolFlagAtlas =
        loadColorKeyTexture(
            renderer,
            "app0:assets/buildings/school/runtime/flag_atlas.bmp"
        );

    resources.schoolWorkAtlas =
        loadColorKeyTexture(
            renderer,
            "app0:assets/buildings/school/runtime/work_atlas.bmp"
        );

    resources.schoolIcon = loadColorKeyTexture(
        renderer,
        "app0:assets/ui/buildings/school_icon.bmp"
    );

    resources.schoolWoodAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/construction/school/wood_atlas.bmp"
    );

    resources.schoolStoneAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/buildings/construction/school/stone_atlas.bmp"
    );

    return resources;
}

void destroyBuildingResources(
    BuildingResources* resources
)
{
    if (!resources)
        return;

    SDL_DestroyTexture(resources->store);
    SDL_DestroyTexture(resources->bakery);
    SDL_DestroyTexture(resources->farm);
    SDL_DestroyTexture(resources->school);

    SDL_DestroyTexture(resources->roadIcon);
    SDL_DestroyTexture(resources->storeIcon);
    SDL_DestroyTexture(resources->bakeryIcon);
    SDL_DestroyTexture(resources->farmIcon);
    SDL_DestroyTexture(resources->schoolIcon);

    SDL_DestroyTexture(resources->storeWoodAtlas);
    SDL_DestroyTexture(resources->storeStoneAtlas);

    SDL_DestroyTexture(resources->bakeryWoodAtlas);
    SDL_DestroyTexture(resources->bakeryStoneAtlas);

    SDL_DestroyTexture(resources->farmWoodAtlas);
    SDL_DestroyTexture(resources->farmStoneAtlas);

    SDL_DestroyTexture(resources->schoolWoodAtlas);
    SDL_DestroyTexture(resources->schoolStoneAtlas);

    resources->store = nullptr;
    resources->bakery = nullptr;
    resources->farm = nullptr;
    resources->school = nullptr;

    resources->roadIcon = nullptr;
    resources->storeIcon = nullptr;
    resources->bakeryIcon = nullptr;
    resources->farmIcon = nullptr;
    SDL_DestroyTexture(
        resources->schoolFlagpole
    );

    SDL_DestroyTexture(
        resources->schoolFlagAtlas
    );

    SDL_DestroyTexture(
        resources->schoolWorkAtlas
    );

    resources->schoolFlagpole = nullptr;
    resources->schoolFlagAtlas = nullptr;
    resources->schoolWorkAtlas = nullptr;

    resources->schoolIcon = nullptr;

    resources->storeWoodAtlas = nullptr;
    resources->storeStoneAtlas = nullptr;

    resources->bakeryWoodAtlas = nullptr;
    resources->bakeryStoneAtlas = nullptr;

    resources->farmWoodAtlas = nullptr;
    resources->farmStoneAtlas = nullptr;

    resources->schoolWoodAtlas = nullptr;
    resources->schoolStoneAtlas = nullptr;
}

int getBuildingWidth(BuildingType type)
{
    switch (type) {
        case BuildingType::Store:
            return 3;

        case BuildingType::Bakery:
            return 3;

        case BuildingType::Farm:
            return 4;

        case BuildingType::School:
            return 3;
    }

    return 3;
}

int getBuildingHeight(BuildingType type)
{
    switch (type) {
        case BuildingType::Store:
            return 3;

        case BuildingType::Bakery:
            return 3;

        case BuildingType::Farm:
            return 3;

        case BuildingType::School:
            return 3;
    }

    return 3;
}

int getBuildingPlanValue(
    BuildingType type,
    int localX,
    int localY
)
{
    static const int storePlan[3][3] = {
        {1, 1, 1},
        {1, 1, 1},
        {1, 2, 1}
    };

    static const int bakeryPlan[3][3] = {
        {1, 1, 1},
        {1, 1, 1},
        {1, 2, 1}
    };

    static const int farmPlan[3][4] = {
        {1, 1, 1, 1},
        {1, 1, 1, 1},
        {1, 2, 1, 1}
    };

    static const int schoolPlan[3][3] = {
        {1, 1, 1},
        {1, 1, 1},
        {1, 2, 1}
    };

    int width = getBuildingWidth(type);
    int height = getBuildingHeight(type);

    if (localX < 0 || localY < 0 ||
        localX >= width || localY >= height) {
        return 0;
    }

    switch (type) {
        case BuildingType::Store:
            return storePlan[localY][localX];

        case BuildingType::Bakery:
            return bakeryPlan[localY][localX];

        case BuildingType::Farm:
            return farmPlan[localY][localX];

        case BuildingType::School:
            return schoolPlan[localY][localX];
    }

    return 0;
}

int getBuildingWoodCost(BuildingType type)
{
    switch (type) {
        case BuildingType::Store:
            return 6;

        case BuildingType::Bakery:
            return 4;

        case BuildingType::Farm:
            return 4;

        case BuildingType::School:
            return 6;
    }

    return 0;
}

int getBuildingStoneCost(BuildingType type)
{
    switch (type) {
        case BuildingType::Store:
            return 5;

        case BuildingType::Bakery:
            return 3;

        case BuildingType::Farm:
            return 3;

        case BuildingType::School:
            return 5;
    }

    return 0;
}

int getBuildingWoodSteps(BuildingType type)
{
    switch (type) {
        case BuildingType::Store:
            return 24;

        case BuildingType::Bakery:
            return 21;

        case BuildingType::Farm:
            return 26;

        case BuildingType::School:
            return 21;
    }

    return 0;
}

int getBuildingStoneSteps(BuildingType type)
{
    switch (type) {
        case BuildingType::Store:
            return 75;

        case BuildingType::Bakery:
            return 54;

        case BuildingType::Farm:
            return 43;

        case BuildingType::School:
            return 65;
    }

    return 0;
}

bool isBuildingComplete(const Building& building)
{
    return
        building.woodStep >=
            getBuildingWoodSteps(building.type) &&
        building.stoneStep >=
            getBuildingStoneSteps(building.type);
}

BuildingEntrance getBuildingEntrance(
    BuildingType type
)
{
    switch (type) {
        case BuildingType::School:
            return {
                1,  // prostredni pole
                2,  // spodni rada footprintu
                17,
                10
            };

        case BuildingType::Store:
            return {
                1,
                2,
                19,
                7
            };

        default:
            return {
                0,
                0,
                0,
                0
            };
    }
}
