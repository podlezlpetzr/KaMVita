#include "units.h"
#include "worker_action_atlas_config.h"
#include "worker_complete_atlas_config.h"
#include <cmath>
#include <vector>
#include <queue>
#include <algorithm>

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

UnitResources loadUnitResources(
    SDL_Renderer* renderer
)
{
    UnitResources resources = {};

    resources.workerIdle = loadColorKeyTexture(
        renderer,
        "app0:assets/units/worker/idle_s.bmp"
    );

    resources.workerWalkAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/units/worker/complete/actions/walk_atlas.bmp"
    );

    resources.workerWorkAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/units/worker/complete/actions/work_atlas.bmp"
    );

    resources.workerDigAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/units/worker/complete/actions/work1_atlas.bmp"
    );

    resources.workerPaveAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/units/worker/complete/actions/work2_atlas.bmp"
    );

    resources.serfWalkAtlas = loadColorKeyTexture(
        renderer,
        "app0:assets/units/serf/walk_atlas.bmp"
    );

    resources.serfCarryStoneAtlas =
        loadColorKeyTexture(
            renderer,
            "app0:assets/units/serf/carry_stone_atlas.bmp"
        );

    return resources;
}

void destroyUnitResources(
    UnitResources* resources
)
{
    if (!resources)
        return;

    SDL_DestroyTexture(resources->workerIdle);
    SDL_DestroyTexture(resources->workerWalkAtlas);
    SDL_DestroyTexture(resources->workerWorkAtlas);
    SDL_DestroyTexture(resources->workerDigAtlas);
    SDL_DestroyTexture(resources->workerPaveAtlas);
    SDL_DestroyTexture(resources->serfWalkAtlas);
    SDL_DestroyTexture(resources->serfCarryStoneAtlas);

    resources->workerIdle = nullptr;
    resources->workerWalkAtlas = nullptr;
    resources->workerWorkAtlas = nullptr;
    resources->workerDigAtlas = nullptr;
    resources->workerPaveAtlas = nullptr;
    resources->serfWalkAtlas = nullptr;
    resources->serfCarryStoneAtlas = nullptr;
}

void renderUnits(
    SDL_Renderer* renderer,
    const UnitResources& resources,
    const Unit* units,
    int unitCount,
    float cameraX,
    float cameraY
)
{
    if (!resources.workerWalkAtlas)
        return;

    const int cellWidth =
        WORKER_WALK_CELL_WIDTH;

    const int cellHeight =
        WORKER_WALK_CELL_HEIGHT;

    const int anchorX =
        WORKER_WALK_ANCHOR_X;

    const int anchorY =
        WORKER_WALK_ANCHOR_Y;

    // Originalni UnitStillFrames pro:
    // N, NE, E, SE, S, SW, W, NW
    static const int stillFrames[8] = {
        3, 2, 2, 1, 6, 7, 6, 6
    };

    for (int i = 0; i < unitCount; ++i) {
        if (!units[i].active)
            continue;

        if (!units[i].visible)
            continue;

        int direction = units[i].direction;

        if (direction < 0)
            direction = 0;

        if (direction > 7)
            direction = 7;

        if (
            units[i].type == UnitType::Worker &&
            units[i].state == UnitState::Working
        ) {
            SDL_Texture* actionAtlas = nullptr;

            int actionCellWidth = 0;
            int actionCellHeight = 0;
            int actionFrameCount = 0;
            int actionAnchorX = 0;
            int actionAnchorY = 0;

            if (
                units[i].workAction ==
                    UnitWorkAction::DigRoad
            ) {
                actionAtlas =
                    resources.workerDigAtlas;

                actionCellWidth =
                    WORKER_WORK1_CELL_WIDTH;

                actionCellHeight =
                    WORKER_WORK1_CELL_HEIGHT;

                actionFrameCount =
                    WORKER_WORK1_FRAME_COUNT;

                actionAnchorX =
                    WORKER_WORK1_ANCHOR_X;

                actionAnchorY =
                    WORKER_WORK1_ANCHOR_Y;
            } else if (
                units[i].workAction ==
                    UnitWorkAction::PaveRoad
            ) {
                actionAtlas =
                    resources.workerPaveAtlas;

                actionCellWidth =
                    WORKER_WORK2_CELL_WIDTH;

                actionCellHeight =
                    WORKER_WORK2_CELL_HEIGHT;

                actionFrameCount =
                    WORKER_WORK2_FRAME_COUNT;

                actionAnchorX =
                    WORKER_WORK2_ANCHOR_X;

                actionAnchorY =
                    WORKER_WORK2_ANCHOR_Y;
            } else {
                // Stavba budov zustava na kladivu.
                actionAtlas =
                    resources.workerWorkAtlas;

                actionCellWidth =
                    WORKER_WORK_CELL_WIDTH;

                actionCellHeight =
                    WORKER_WORK_CELL_HEIGHT;

                actionFrameCount =
                    WORKER_WORK_FRAME_COUNT;

                actionAnchorX =
                    WORKER_WORK_ANCHOR_X;

                actionAnchorY =
                    WORKER_WORK_ANCHOR_Y;
            }

            if (actionAtlas &&
                actionFrameCount > 0) {

                int actionFrame =
                    units[i].animationFrame %
                    actionFrameCount;

                SDL_Rect actionSource = {
                    actionFrame * actionCellWidth,
                    direction * actionCellHeight,
                    actionCellWidth,
                    actionCellHeight
                };

                SDL_Rect actionDestination = {
                    (int)units[i].worldX -
                        (int)cameraX +
                        actionAnchorX,

                    (int)units[i].worldY -
                        (int)cameraY +
                        actionAnchorY,

                    actionCellWidth,
                    actionCellHeight
                };

                SDL_RenderCopy(
                    renderer,
                    actionAtlas,
                    &actionSource,
                    &actionDestination
                );

                continue;
            }
        }

        bool isSerf =
            units[i].type == UnitType::Serf;

        SDL_Texture* walkAtlas =
            isSerf
                ? resources.serfWalkAtlas
                : resources.workerWalkAtlas;

        if (!walkAtlas)
            continue;

        int walkCellWidth =
            isSerf ? 43 : 56;

        int walkCellHeight =
            isSerf ? 47 : 50;

        int walkAnchorX =
            isSerf ? -18 : -21;

        int walkAnchorY =
            isSerf ? -38 : -42;

        int frame = 0;

        if (units[i].state == UnitState::Walking) {
            frame =
                units[i].animationFrame % 8;

            if (frame < 0)
                frame += 8;
        } else if (!isSerf) {
            frame = stillFrames[direction];
        }

        SDL_Rect source = {
            frame * walkCellWidth,
            direction * walkCellHeight,
            walkCellWidth,
            walkCellHeight
        };

        int doorwaySlideX = 0;
        int doorwaySlideY = 0;

        if (
            units[i].state ==
                UnitState::LeavingBuilding
        ) {
            doorwaySlideX =
                (int)(
                    units[i].doorwayOffsetX *
                    units[i].doorwayProgress
                );

            doorwaySlideY =
                (int)(
                    units[i].doorwayOffsetY *
                    units[i].doorwayProgress
                );
        }

        SDL_Rect destination = {
            (int)units[i].worldX -
                (int)cameraX +
                walkAnchorX +
                doorwaySlideX,

            (int)units[i].worldY -
                (int)cameraY +
                walkAnchorY +
                doorwaySlideY,

            walkCellWidth,
            walkCellHeight
        };

        SDL_RenderCopy(
            renderer,
            walkAtlas,
            &source,
            &destination
        );
    }
}


static bool isBuildingTileBlocked(
    int tileX,
    int tileY,
    const Building* buildings,
    int buildingCount
)
{
    for (int i = 0; i < buildingCount; ++i) {
        if (!buildings[i].active)
            continue;

        const Building& building =
            buildings[i];

        int localX =
            tileX - building.tileX;

        int localY =
            tileY - building.tileY;

        if (localX < 0 || localY < 0)
            continue;

        int width =
            getBuildingWidth(building.type);

        int height =
            getBuildingHeight(building.type);

        if (localX >= width ||
            localY >= height) {
            continue;
        }

        if (getBuildingPlanValue(
                building.type,
                localX,
                localY
            ) != 0) {
            return true;
        }
    }

    return false;
}

bool setUnitMoveTarget(
    Unit* unit,
    float targetWorldX,
    float targetWorldY,
    const Building* buildings,
    int buildingCount,
    int mapWidth,
    int mapHeight,
    int tileSize
)
{
    if (!unit || !unit->active)
        return false;

    if (mapWidth <= 0 ||
        mapHeight <= 0 ||
        tileSize <= 0) {
        return false;
    }

    int startX =
        (int)(unit->worldX / tileSize);

    int startY =
        (int)(unit->worldY / tileSize);

    int goalX =
        (int)(targetWorldX / tileSize);

    int goalY =
        (int)(targetWorldY / tileSize);

    if (startX < 0 || startX >= mapWidth ||
        startY < 0 || startY >= mapHeight ||
        goalX < 0 || goalX >= mapWidth ||
        goalY < 0 || goalY >= mapHeight) {
        return false;
    }

    if (isBuildingTileBlocked(
            goalX,
            goalY,
            buildings,
            buildingCount
        )) {
        return false;
    }

    const int nodeCount =
        mapWidth * mapHeight;

    std::vector<int> parent(
        nodeCount,
        -1
    );

    std::vector<unsigned char> visited(
        nodeCount,
        0
    );

    std::queue<int> open;

    int startNode =
        startY * mapWidth + startX;

    int goalNode =
        goalY * mapWidth + goalX;

    visited[startNode] = 1;
    open.push(startNode);

    static const int offsets[8][2] = {
        { 0, -1},
        { 1, -1},
        { 1,  0},
        { 1,  1},
        { 0,  1},
        {-1,  1},
        {-1,  0},
        {-1, -1}
    };

    while (!open.empty() &&
           !visited[goalNode]) {

        int current = open.front();
        open.pop();

        int currentX =
            current % mapWidth;

        int currentY =
            current / mapWidth;

        for (int direction = 0;
             direction < 8;
             ++direction) {

            int nextX =
                currentX +
                offsets[direction][0];

            int nextY =
                currentY +
                offsets[direction][1];

            if (nextX < 0 ||
                nextX >= mapWidth ||
                nextY < 0 ||
                nextY >= mapHeight) {
                continue;
            }

            if (isBuildingTileBlocked(
                    nextX,
                    nextY,
                    buildings,
                    buildingCount
                )) {
                continue;
            }

            bool diagonal =
                offsets[direction][0] != 0 &&
                offsets[direction][1] != 0;

            if (diagonal) {
                int sideAX = nextX;
                int sideAY = currentY;

                int sideBX = currentX;
                int sideBY = nextY;

                if (isBuildingTileBlocked(
                        sideAX,
                        sideAY,
                        buildings,
                        buildingCount
                    ) ||
                    isBuildingTileBlocked(
                        sideBX,
                        sideBY,
                        buildings,
                        buildingCount
                    )) {
                    continue;
                }
            }

            int nextNode =
                nextY * mapWidth + nextX;

            if (visited[nextNode])
                continue;

            visited[nextNode] = 1;
            parent[nextNode] = current;
            open.push(nextNode);
        }
    }

    if (!visited[goalNode])
        return false;

    std::vector<int> reversedPath;

    int node = goalNode;

    while (node != startNode &&
           node != -1) {

        reversedPath.push_back(node);
        node = parent[node];
    }

    std::reverse(
        reversedPath.begin(),
        reversedPath.end()
    );

    if (reversedPath.empty()) {
        unit->state = UnitState::Idle;
        unit->pathLength = 0;
        unit->pathIndex = 0;
        return true;
    }

    if ((int)reversedPath.size() >
        MAX_UNIT_PATH) {
        return false;
    }

    unit->pathLength =
        (int)reversedPath.size();

    unit->pathIndex = 0;

    for (int i = 0;
         i < unit->pathLength;
         ++i) {

        int pathNode = reversedPath[i];

        unit->pathX[i] =
            pathNode % mapWidth;

        unit->pathY[i] =
            pathNode / mapWidth;
    }

    unit->targetWorldX =
        (unit->pathX[0] + 0.5f) *
        tileSize;

    unit->targetWorldY =
        (unit->pathY[0] + 0.5f) *
        tileSize;

    unit->state = UnitState::Walking;
    unit->animationFrame = 0;
    unit->animationTimer = 0.0f;

    return true;
}


void updateUnits(
    Unit* units,
    int unitCount,
    float deltaSeconds,
    int tileSize,
    RoadState* roads,
    int mapWidth,
    int mapHeight
)
{
    if (!units)
        return;

    if (deltaSeconds < 0.0f)
        deltaSeconds = 0.0f;

    if (deltaSeconds > 0.05f)
        deltaSeconds = 0.05f;

    const float movementSpeed = 62.0f;
    const float animationFrameDuration = 0.11f;

    for (int i = 0; i < unitCount; ++i) {
        Unit& unit = units[i];

        if (!unit.active)
            continue;

        if (
            unit.state ==
                UnitState::LeavingBuilding
        ) {
            float dx =
                unit.doorwayTargetX -
                unit.doorwayStartX;

            float dy =
                unit.doorwayTargetY -
                unit.doorwayStartY;

            float distance =
                std::sqrt(
                    dx * dx +
                    dy * dy
                );

            if (distance <= 0.0f) {
                unit.doorwayProgress = 1.0f;
            } else {
                unit.doorwayProgress +=
                    (
                        movementSpeed *
                        deltaSeconds
                    ) / distance;
            }

            if (unit.doorwayProgress > 1.0f)
                unit.doorwayProgress = 1.0f;

            unit.worldX =
                unit.doorwayStartX +
                dx * unit.doorwayProgress;

            unit.worldY =
                unit.doorwayStartY +
                dy * unit.doorwayProgress;

            // V originalu je jednotka uvnitr
            // budovy neviditelna.
            unit.visible =
                unit.doorwayProgress > 0.0f;

            // Vystup ze School smeruje na jih.
            unit.direction = 4;

            unit.animationTimer +=
                deltaSeconds;

            while (
                unit.animationTimer >=
                    animationFrameDuration
            ) {
                unit.animationTimer -=
                    animationFrameDuration;

                unit.animationFrame =
                    (
                        unit.animationFrame + 1
                    ) % 8;
            }

            if (
                unit.doorwayProgress >= 1.0f
            ) {
                unit.worldX =
                    unit.doorwayTargetX;

                unit.worldY =
                    unit.doorwayTargetY;

                unit.targetWorldX =
                    unit.worldX;

                unit.targetWorldY =
                    unit.worldY;

                unit.doorwayProgress = 0.0f;

                unit.doorwayOffsetX = 0;
                unit.doorwayOffsetY = 0;

                unit.visible = true;
                unit.state = UnitState::Idle;

                unit.animationFrame = 0;
                unit.animationTimer = 0.0f;
            }

            continue;
        }

        if (unit.state == UnitState::Working) {
            if (!roads ||
                unit.targetRoadX < 0 ||
                unit.targetRoadX >= mapWidth ||
                unit.targetRoadY < 0 ||
                unit.targetRoadY >= mapHeight) {

                unit.state = UnitState::Idle;
                unit.targetRoadX = -1;
                unit.targetRoadY = -1;
                continue;
            }

            RoadState& road =
                roads[
                    unit.targetRoadY * mapWidth +
                    unit.targetRoadX
                ];

            if (road == RoadState::None ||
                road == RoadState::Complete) {

                unit.state = UnitState::Idle;
                unit.targetRoadX = -1;
                unit.targetRoadY = -1;
                continue;
            }

            if (
                road == RoadState::Plan ||
                road == RoadState::Dig1 ||
                road == RoadState::Dig2
            ) {
                unit.workAction =
                    UnitWorkAction::DigRoad;
            } else {
                unit.workAction =
                    UnitWorkAction::PaveRoad;
            }

            unit.workTimer += deltaSeconds;
            unit.animationTimer += deltaSeconds;

            while (
                unit.animationTimer >=
                animationFrameDuration
            ) {
                unit.animationTimer -=
                    animationFrameDuration;

                int workFrameCount =
                unit.workAction ==
                    UnitWorkAction::DigRoad
                    ? WORKER_WORK1_FRAME_COUNT
                    : WORKER_WORK2_FRAME_COUNT;

            unit.animationFrame =
                (unit.animationFrame + 1) %
                workFrameCount;
            }

            // Jeden stavebni krok cesty.
            if (unit.workTimer >= 0.65f) {
                unit.workTimer = 0.0f;

                int nextState =
                    (int)road + 1;

                if (nextState >
                    (int)RoadState::Complete) {

                    nextState =
                        (int)RoadState::Complete;
                }

                road =
                    (RoadState)nextState;

                if (road == RoadState::Complete) {
                    unit.state = UnitState::Idle;
                    unit.targetRoadX = -1;
                    unit.targetRoadY = -1;
                    unit.animationFrame = 0;
                    unit.animationTimer = 0.0f;
                }
            }

            continue;
        }

        if (unit.state != UnitState::Walking)
            continue;

        float dx =
            unit.targetWorldX - unit.worldX;

        float dy =
            unit.targetWorldY - unit.worldY;

        float distance =
            std::sqrt(dx * dx + dy * dy);

        if (distance <= 1.0f) {
            unit.worldX = unit.targetWorldX;
            unit.worldY = unit.targetWorldY;

            unit.pathIndex++;

            if (unit.pathIndex >=
                unit.pathLength) {

                unit.animationFrame = 0;
                unit.animationTimer = 0.0f;
                unit.pathLength = 0;
                unit.pathIndex = 0;

                if (unit.targetRoadX >= 0 &&
                    unit.targetRoadY >= 0) {

                    unit.state =
                        UnitState::Working;

                    unit.workTimer = 0.0f;
                } else {
                    unit.state =
                        UnitState::Idle;
                }

                continue;
            }

            unit.targetWorldX =
                (unit.pathX[unit.pathIndex] + 0.5f) *
                tileSize;

            unit.targetWorldY =
                (unit.pathY[unit.pathIndex] + 0.5f) *
                tileSize;

            continue;
        }

        float angle = std::atan2(dy, dx);

        const float PI = 3.14159265358979323846f;

        // 0=N, 1=NE, 2=E, 3=SE,
        // 4=S, 5=SW, 6=W, 7=NW.
        if (angle >= -PI / 8.0f &&
            angle < PI / 8.0f) {
            unit.direction = 2;
        } else if (
            angle >= PI / 8.0f &&
            angle < 3.0f * PI / 8.0f
        ) {
            unit.direction = 3;
        } else if (
            angle >= 3.0f * PI / 8.0f &&
            angle < 5.0f * PI / 8.0f
        ) {
            unit.direction = 4;
        } else if (
            angle >= 5.0f * PI / 8.0f &&
            angle < 7.0f * PI / 8.0f
        ) {
            unit.direction = 5;
        } else if (
            angle >= 7.0f * PI / 8.0f ||
            angle < -7.0f * PI / 8.0f
        ) {
            unit.direction = 6;
        } else if (
            angle >= -7.0f * PI / 8.0f &&
            angle < -5.0f * PI / 8.0f
        ) {
            unit.direction = 7;
        } else if (
            angle >= -5.0f * PI / 8.0f &&
            angle < -3.0f * PI / 8.0f
        ) {
            unit.direction = 0;
        } else {
            unit.direction = 1;
        }

        float movement =
            movementSpeed * deltaSeconds;

        if (movement >= distance) {
            unit.worldX = unit.targetWorldX;
            unit.worldY = unit.targetWorldY;
        } else {
            unit.worldX += dx / distance * movement;
            unit.worldY += dy / distance * movement;
        }

        unit.animationTimer += deltaSeconds;

        while (
            unit.animationTimer >=
            animationFrameDuration
        ) {
            unit.animationTimer -=
                animationFrameDuration;

            unit.animationFrame =
                (unit.animationFrame + 1) % 8;
        }
    }
}

UnitMovementType getUnitMovementType(
    UnitType type
)
{
    if (type == UnitType::Serf)
        return UnitMovementType::Carrier;

    return UnitMovementType::Worker;
}

