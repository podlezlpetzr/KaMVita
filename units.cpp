#include "units.h"
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
        "app0:assets/units/worker/walk_atlas.bmp"
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

    resources->workerIdle = nullptr;
    resources->workerWalkAtlas = nullptr;
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

    const int cellWidth = 56;
    const int cellHeight = 50;

    const int anchorX = -21;
    const int anchorY = -42;

    // Originalni UnitStillFrames pro:
    // N, NE, E, SE, S, SW, W, NW
    static const int stillFrames[8] = {
        3, 2, 2, 1, 6, 7, 6, 6
    };

    for (int i = 0; i < unitCount; ++i) {
        if (!units[i].active)
            continue;

        int direction = units[i].direction;

        if (direction < 0)
            direction = 0;

        if (direction > 7)
            direction = 7;

        int frame;

        if (units[i].state == UnitState::Walking) {
            frame = units[i].animationFrame % 8;

            if (frame < 0)
                frame += 8;
        } else {
            frame = stillFrames[direction];
        }

        SDL_Rect source = {
            frame * cellWidth,
            direction * cellHeight,
            cellWidth,
            cellHeight
        };

        SDL_Rect destination = {
            (int)units[i].worldX -
                (int)cameraX +
                anchorX,

            (int)units[i].worldY -
                (int)cameraY +
                anchorY,

            cellWidth,
            cellHeight
        };

        SDL_RenderCopy(
            renderer,
            resources.workerWalkAtlas,
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
    int tileSize
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

                unit.state = UnitState::Idle;
                unit.animationFrame = 0;
                unit.animationTimer = 0.0f;
                unit.pathLength = 0;
                unit.pathIndex = 0;
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
    switch (type) {
        case UnitType::Worker:
            return UnitMovementType::Worker;
    }

    return UnitMovementType::Worker;
}
