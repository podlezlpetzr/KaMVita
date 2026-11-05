#pragma once

#include <SDL2/SDL.h>

struct KaMPlanResources {
    SDL_Texture* roadMarkup;
    SDL_Texture* fieldMarkup;
    SDL_Texture* wineMarkup;

    // Originalni House plan:
    // GUI 105 horizontal, GUI 117 vertical.
    SDL_Texture* housePlanHorizontal;
    SDL_Texture* housePlanVertical;

    // Originalni House WIP fence:
    // GUI 463 horizontal, GUI 467 vertical.
    SDL_Texture* houseWipHorizontal;
    SDL_Texture* houseWipVertical;

    // Aktualni BuildingType poradi:
    // 0 Store, 1 Bakery, 2 Farm, 3 School,
    // 4 Quarry, 5 Mill.
    //
    // Textury se nacitaji jednou pri startu.
    SDL_Texture* houseTablets[6];
};

KaMPlanResources loadKaMPlanResources(
    SDL_Renderer* renderer
);

void destroyKaMPlanResources(
    KaMPlanResources* resources
);

void renderKaMMarkup(
    SDL_Renderer* renderer,
    SDL_Texture* texture,
    int tileScreenX,
    int tileScreenY,
    int tileSize
);

void renderKaMHouseEdge(
    SDL_Renderer* renderer,
    SDL_Texture* horizontalTexture,
    SDL_Texture* verticalTexture,
    int cellScreenX,
    int cellScreenY,
    int tileSize,
    int direction
);
