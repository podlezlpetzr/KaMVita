#pragma once

#include <SDL2/SDL.h>

struct TerrainResources {
    SDL_Texture* grass;
    SDL_Texture* water;
    SDL_Texture* dirt;
    SDL_Texture* sand;

    SDL_Texture* roadDig1;
    SDL_Texture* roadDig2;
    SDL_Texture* roadDig3;
    SDL_Texture* roadDig4;

    SDL_Texture* road248;
    SDL_Texture* road250;
    SDL_Texture* road252;
    SDL_Texture* road254;
};

TerrainResources loadTerrainResources(SDL_Renderer* renderer);
void destroyTerrainResources(TerrainResources* terrain);

SDL_Texture* loadTexture(
    SDL_Renderer* renderer,
    const char* path
);

void renderTerrain(
    SDL_Renderer* renderer,
    const TerrainResources& terrain,
    float cameraX,
    float cameraY,
    int screenWidth,
    int screenHeight,
    int tileSize,
    int mapWidth,
    int mapHeight
);
