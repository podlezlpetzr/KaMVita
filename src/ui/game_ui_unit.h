#pragma once

#include <SDL2/SDL.h>
#include "../game/units.h"
#include "game_ui.h"

struct UnitUIState {
    int selectedUnitIndex;
    bool panelVisible;

    // Stranka, na kterou se vrati GUI pres 443 / Circle.
    GameUISection previousSection;
};

struct UnitUIResources {
    SDL_Texture* sidebarMiddle;
    // Original bsGame button stone GUI 402.
    SDL_Texture* buttonStone;

    SDL_Texture* back443;

    // Originalni gRes.Units[].GUIScroll portrety.
    SDL_Texture* workerScroll;
    SDL_Texture* serfScroll;

    // Originalni gRes.Units[].GUIScroll:
    //
    // Farmer = GUI 525
    // Baker  = GUI 527
    SDL_Texture* farmerScroll;
    SDL_Texture* bakerScroll;

    SDL_Texture* workerName;
    SDL_Texture* serfName;
    SDL_Texture* farmerName;
    SDL_Texture* bakerName;

    // Original KaM fnt_Grey English Unit text.
    SDL_Texture* conditionText;

    SDL_Texture* idleText;
    SDL_Texture* walkingText;

    SDL_Texture* roadText;
    SDL_Texture* fieldText;
    SDL_Texture* wineText;

    SDL_Texture* houseSiteText;
    SDL_Texture* houseText;
    SDL_Texture* repairText;

    SDL_Texture* serfDescription;
    SDL_Texture* workerDescription;

    // Originalni KaM Remake descriptions vykreslene
    // originalnim game.fntx.
    SDL_Texture* farmerDescription;
    SDL_Texture* bakerDescription;
};

void initUnitUI(
    UnitUIState* state
);

UnitUIResources loadUnitUIResources(
    SDL_Renderer* renderer
);

void destroyUnitUIResources(
    UnitUIResources* resources
);

int hitTestUnit(
    const Unit* units,
    int unitCount,
    float screenX,
    float screenY,
    float cameraX,
    float cameraY
);

void renderSelectedUnitMarker(
    SDL_Renderer* renderer,
    const Unit* units,
    int unitCount,
    int selectedUnitIndex,
    float cameraX,
    float cameraY
);

void renderUnitUI(
    SDL_Renderer* renderer,
    const UnitUIResources& resources,
    const UnitUIState& state,
    const Unit* units,
    int unitCount
);

SDL_Rect getUnitUIBackRect();

bool handleUnitUIBackTouch(
    UnitUIState* state,
    float touchX,
    float touchY
);

void closeUnitUI(
    UnitUIState* state
);
