#pragma once

#include <SDL2/SDL.h>

struct KaMPlanResources {
    SDL_Texture* roadMarkup;
    SDL_Texture* fieldMarkup;
    SDL_Texture* wineMarkup;
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
