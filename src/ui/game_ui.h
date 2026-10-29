#pragma once

#include <SDL2/SDL.h>

#include "../game/buildings.h"
#include "../game/units.h"
#include "../game/ware_distribution.h"

static constexpr int GAME_UI_PANEL_WIDTH = 220;

enum class GameUISection {
    Build,
    Ratios,
    Statistics,
    Options,
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

    // Originalni hlavni toolbar GUI 439..442:
    // 0 Build, 1 Ratios, 2 Statistics, 3 Options.
    int toolbarSelection;
    bool toolbarFocus;

    // Originalni Ratios 440 navigace.
    int ratiosTab;
    int ratiosRow;
    bool ratiosFocusTabs;

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
    // Autenticke casti leveho sidebaru z KaM GUI.RX:
    // sprite 407 = horni cast 224x200
    // sprite 554 = prostredni cast 224x168
    SDL_Texture* sidebarTop;
    SDL_Texture* sidebarMiddle;
    SDL_Texture* sidebarBottom;

    // Originalni KaM bsGame button resources.
    SDL_Texture* buttonStone;
    SDL_Texture* mainButtons[5];

    // School bsGame glyphy z GUI.RX:
    // 35 = previous, 42 = train, 36 = next.
    SDL_Texture* schoolButtons[3];

    // Originalni KaM GUI sprite 358 = Gold.
    SDL_Texture* goldIcon;
    SDL_Texture* questionIcon;

    // Originalni House header z GUI.RX.
    SDL_Texture* deliveryOn;
    SDL_Texture* deliveryOff;
    SDL_Texture* repairOn;
    SDL_Texture* repairOff;
    SDL_Texture* houseWorker;
    SDL_Texture* schoolHouseText;

    // Statistics 441: originalni game.fntx cisla.
    // Kazdy atlas ma 33 bunek 32x18 pro hodnoty 0..32.
    SDL_Texture* statisticsCounts;
    SDL_Texture* statisticsWipCounts;

    // Originalni Ratios 440 resources.
    SDL_Texture* ratiosWareIcons[4];
    SDL_Texture* ratiosHouseIcons[9];
    SDL_Texture* ratiosTrackbarThumb;

    // Originalni texty generovane z KaM FNTX fontu.
    SDL_Texture* needsText;
    SDL_Texture* goldText;
    SDL_Texture* professionText[SCHOOL_PROFESSION_COUNT];

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
    const Unit* units,
    int unitCount,
    const WareDistribution& wareDistribution,
    float cursorX,
    float cursorY
);

void moveGameUISelection(
    GameUIState* ui,
    int deltaX,
    int deltaY
);
