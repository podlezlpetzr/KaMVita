#pragma once

#include <SDL2/SDL.h>

#include "../game/buildings.h"

static constexpr int GAME_UI_PANEL_WIDTH = 220;

enum class GameUISection {
    Build,
    School
};

enum class GameUISchoolSelection {
    Serf = 0,
    Worker,
    StoneCutter,
    Woodcutter,
    Lamberjack,
    Fisher,
    Farmer,
    Baker,
    AnimalBreeder,
    Butcher,
    Miner,
    Metallurgist,
    Smith,
    Recruit
};

static constexpr int SCHOOL_PROFESSION_COUNT = 14;
static constexpr int SCHOOL_QUEUE_SIZE = 6;
static constexpr int SCHOOL_WAITING_SLOTS = 5;

static constexpr int SCHOOL_TRAINING_STEPS = 150;
static constexpr int SCHOOL_PHASE_STEPS = 30;
static constexpr int SCHOOL_IDLE_STEPS = 10;

static constexpr int SCHOOL_GOLD_COST = 1;

enum class GameUIBuildSelection {
    Road = 0,
    Store = 1,
    Bakery = 2,
    Farm = 3,
    School = 4
};

struct GameUIState {
    GameUIBuildSelection buildSelection;
    bool buildSectionActive;

    GameUISection section;
    GameUISchoolSelection schoolSelection;

    // False: ovladani carouselu profesi.
    // True: ovladani School queue[0..5].
    bool schoolQueueFocus;
    int schoolQueueSelection;

    int selectedSchoolBuilding;
};

void initGameUI(
    GameUIState* ui
);

struct SchoolProfessionUIResources {
    SDL_Texture* icon;
    SDL_Texture* scroll;
};

struct SchoolUIResources {
    SchoolProfessionUIResources
        professions[SCHOOL_PROFESSION_COUNT];
};

SchoolUIResources loadSchoolUIResources(
    SDL_Renderer* renderer
);

void destroySchoolUIResources(
    SchoolUIResources* resources
);

void moveSchoolUISelection(
    GameUIState* ui,
    int delta
);

void renderGameUI(
    SDL_Renderer* renderer,
    const BuildingResources& buildingResources,
    const SchoolUIResources& schoolResources,
    const GameUIState& ui,
    const Building* buildings,
    int buildingCount,
    float cursorX,
    float cursorY
);

void moveGameUISelection(
    GameUIState* ui,
    int deltaX,
    int deltaY
);
