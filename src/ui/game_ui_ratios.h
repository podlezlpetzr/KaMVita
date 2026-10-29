#pragma once

#include <SDL2/SDL.h>

#include "../game/ware_distribution.h"


struct GameUIState;
struct SchoolUIResources;


void renderGameUIRatios(
    SDL_Renderer* renderer,
    const SchoolUIResources& resources,
    const GameUIState& ui,
    const WareDistribution& distribution
);


bool handleGameUIRatiosTouch(
    GameUIState* ui,
    WareDistribution* distribution,
    float touchX,
    float touchY
);


void handleGameUIRatiosGamepad(
    GameUIState* ui,
    WareDistribution* distribution,
    bool leftPressed,
    bool rightPressed,
    bool upPressed,
    bool downPressed,
    bool crossPressed
);
