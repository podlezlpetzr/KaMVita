#pragma once

#include <SDL2/SDL.h>

#include "../game/ware_distribution.h"
#include "house_icons.h"


struct GameUIState;
struct SchoolUIResources;


void renderGameUIRatios(
    SDL_Renderer* renderer,
    const SchoolUIResources& resources,
    const HouseIconResources& houseIcons,
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
