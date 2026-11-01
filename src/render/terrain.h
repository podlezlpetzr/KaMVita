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

    // Originalni KaM Field terrain stages.
    //
    // SetField(ft_Corn):
    // Terrain 62 -> cornEmpty
    // Terrain 61 -> cornSown
    // Terrain 59 -> cornYoung
    // Terrain 60 -> cornGround
    // Terrain 63 -> cornCut
    SDL_Texture* cornEmpty;
    SDL_Texture* cornSown;
    SDL_Texture* cornYoung;
    SDL_Texture* cornGround;

    // Originalni rxTrees corn overlays:
    //
    // Obj 58 = mensi zelena kukurice.
    // Obj 59 = plne vzrostla kukurice.
    // Originalni rxTrees sprites pouzivane
    // 12krokovou animaci corn Obj58 a Obj59.
    SDL_Texture* cornObj58Sprites[3];
    SDL_Texture* cornObj59Sprites[3];

    SDL_Texture* cornCut;

    // SetField(ft_Wine) pouziva Terrain 55
    // pro vsechny Wine stages. Grapes Obj 54..57
    // budou samostatna overlay vrstva.
    SDL_Texture* wineGround;

    // Skutecne originalni KaM rxGui fence sprites:
    //
    // GUI 461 = Corn horizontal, source 10x47
    // GUI 465 = Corn vertical, source 8x78
    // GUI 462 = Wine horizontal, source 10x47
    // GUI 466 = Wine vertical, source 5x69
    SDL_Texture* cornFenceHorizontal;
    SDL_Texture* cornFenceVertical;
    SDL_Texture* wineFenceHorizontal;
    SDL_Texture* wineFenceVertical;
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
