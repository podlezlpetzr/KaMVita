#include "game_ui_statistics.h"


GameUIStatistics calculateGameUIStatistics(
    const Building* buildings,
    int buildingCount,
    const Unit* units,
    int unitCount
)
{
    GameUIStatistics stats = {};

    // =====================================================
    // BUILDINGS
    //
    // Remake Statistics rozlisuje hotove Qty a WIP.
    // KaMVita pro tento ucel pouziva isBuildingComplete().
    // =====================================================

    if (buildings && buildingCount > 0) {
        for (
            int buildingIndex = 0;
            buildingIndex < buildingCount;
            ++buildingIndex
        ) {
            const Building& building =
                buildings[buildingIndex];

            if (!building.active)
                continue;

            const bool complete =
                isBuildingComplete(building);

            switch (building.type) {
                case BuildingType::Store:
                    if (complete)
                        ++stats.storeCount;
                    else
                        ++stats.storeWip;
                    break;

                case BuildingType::Bakery:
                    if (complete)
                        ++stats.bakeryCount;
                    else
                        ++stats.bakeryWip;
                    break;

                case BuildingType::Farm:
                    if (complete)
                        ++stats.farmCount;
                    else
                        ++stats.farmWip;
                    break;

                case BuildingType::School:
                    if (complete)
                        ++stats.schoolCount;
                    else
                        ++stats.schoolWip;

                    // Gold scitame ze vsech aktivnich School.
                    if (building.schoolGold > 0) {
                        stats.schoolGold +=
                            building.schoolGold;
                    }

                    // Queue[0] je WIP.
                    if (building.schoolQueue[0] >= 0) {
                        ++stats.schoolTrainingCount;
                    }

                    // Queue[1..N-1] jsou cekajici jednotky.
                    // Velikost odvozujeme primo z Building,
                    // aby Statistics nezaviselo na game_ui.h.
                    const int schoolQueueSize =
                        static_cast<int>(
                            sizeof(building.schoolQueue) /
                            sizeof(building.schoolQueue[0])
                        );

                    for (
                        int queueIndex = 1;
                        queueIndex < schoolQueueSize;
                        ++queueIndex
                    ) {
                        if (
                            building.schoolQueue[
                                queueIndex
                            ] >= 0
                        ) {
                            ++stats.schoolQueueCount;
                        }
                    }

                    break;
            }
        }
    }

    // =====================================================
    // UNITS
    //
    // Aktualni runtime KaMVita skutecne rozlisuje pouze
    // Worker a Serf, proto Statistics nevymysli dalsich
    // 12 profesi.
    // =====================================================

    if (units && unitCount > 0) {
        for (
            int unitIndex = 0;
            unitIndex < unitCount;
            ++unitIndex
        ) {
            const Unit& unit =
                units[unitIndex];

            if (!unit.active)
                continue;

            switch (unit.type) {
                case UnitType::Worker:
                    ++stats.workerCount;
                    break;

                case UnitType::Serf:
                    ++stats.serfCount;
                    break;
            }
        }
    }

    return stats;
}


#include "game_ui.h"
#include <SDL2/SDL.h>


namespace
{

void renderStatNumber(
    SDL_Renderer* renderer,
    SDL_Texture* atlas,
    int value,
    int x,
    int y
)
{
    if (!renderer || !atlas)
        return;

    if (value < 0)
        value = 0;

    if (value > 32)
        value = 32;

    SDL_Rect source = {
        0,
        value * 18,
        32,
        18
    };

    SDL_Rect destination = {
        x,
        y,
        32,
        18
    };

    SDL_RenderCopy(
        renderer,
        atlas,
        &source,
        &destination
    );
}


void renderStatBlock(
    SDL_Renderer* renderer,
    SDL_Texture* icon,
    SDL_Texture* countAtlas,
    SDL_Texture* wipAtlas,
    int x,
    int y,
    int count,
    int wip
)
{
    if (!renderer)
        return;

    // Remake Statistics pouziva 30x30 bevel blok.
    SDL_Rect block = {
        x,
        y,
        40,
        36
    };

    SDL_SetRenderDrawBlendMode(
        renderer,
        SDL_BLENDMODE_BLEND
    );

    SDL_SetRenderDrawColor(
        renderer,
        49, 42, 32, 125
    );

    SDL_RenderFillRect(
        renderer,
        &block
    );

    SDL_SetRenderDrawBlendMode(
        renderer,
        SDL_BLENDMODE_NONE
    );

    SDL_SetRenderDrawColor(
        renderer,
        35, 30, 25, 255
    );

    SDL_RenderDrawLine(
        renderer,
        block.x,
        block.y,
        block.x + block.w - 1,
        block.y
    );

    SDL_RenderDrawLine(
        renderer,
        block.x,
        block.y,
        block.x,
        block.y + block.h - 1
    );

    SDL_SetRenderDrawColor(
        renderer,
        171, 154, 121, 255
    );

    SDL_RenderDrawLine(
        renderer,
        block.x,
        block.y + block.h - 1,
        block.x + block.w - 1,
        block.y + block.h - 1
    );

    SDL_RenderDrawLine(
        renderer,
        block.x + block.w - 1,
        block.y,
        block.x + block.w - 1,
        block.y + block.h - 1
    );

    if (icon) {
        int iconWidth = 0;
        int iconHeight = 0;

        SDL_QueryTexture(
            icon,
            nullptr,
            nullptr,
            &iconWidth,
            &iconHeight
        );

        const int availableWidth = 27;
        const int availableHeight = 27;

        float scaleX =
            static_cast<float>(availableWidth) /
            static_cast<float>(iconWidth);

        float scaleY =
            static_cast<float>(availableHeight) /
            static_cast<float>(iconHeight);

        float scale =
            scaleX < scaleY
                ? scaleX
                : scaleY;

        if (scale > 1.0f)
            scale = 1.0f;

        int destinationWidth =
            static_cast<int>(iconWidth * scale);

        int destinationHeight =
            static_cast<int>(iconHeight * scale);

        SDL_Rect iconDestination = {
            block.x + 2 +
                (availableWidth - destinationWidth) / 2,
            block.y + 4 +
                (availableHeight - destinationHeight) / 2,
            destinationWidth,
            destinationHeight
        };

        SDL_RenderCopy(
            renderer,
            icon,
            nullptr,
            &iconDestination
        );
    }

    // Hotovy pocet dole vpravo.
    renderStatNumber(
        renderer,
        countAtlas,
        count,
        block.x + block.w - 28,
        block.y + block.h - 18
    );

    // Rozestaveny / trenovany pocet nahore vpravo.
    if (wip > 0) {
        renderStatNumber(
            renderer,
            wipAtlas,
            wip,
            block.x + block.w - 28,
            block.y - 1
        );
    }
}

}


void renderGameUIStatistics(
    SDL_Renderer* renderer,
    const HouseIconResources& houseIcons,
    const SchoolUIResources& uiResources,
    const GameUIStatistics& stats
)
{
    if (!renderer)
        return;

    // Toolbar konci na Y=230.
    // Statistics obsah zacina pod nim a nezasahuje
    // do zamceneho School layoutu.
    const int firstRowY = 242;
    const int secondRowY = 286;
    const int thirdRowY = 330;

    const int firstX = 6;
    const int stepX = 44;

    // Buildings: Store, Bakery, Farm, School.
    renderStatBlock(
        renderer,
        getHouseIcon(
            houseIcons,
            HouseIconType::Store
        ),
        uiResources.statisticsCounts,
        uiResources.statisticsWipCounts,
        firstX + stepX * 0,
        firstRowY,
        stats.storeCount,
        stats.storeWip
    );

    renderStatBlock(
        renderer,
        getHouseIcon(
            houseIcons,
            HouseIconType::Bakery
        ),
        uiResources.statisticsCounts,
        uiResources.statisticsWipCounts,
        firstX + stepX * 1,
        firstRowY,
        stats.bakeryCount,
        stats.bakeryWip
    );

    renderStatBlock(
        renderer,
        getHouseIcon(
            houseIcons,
            HouseIconType::Farm
        ),
        uiResources.statisticsCounts,
        uiResources.statisticsWipCounts,
        firstX + stepX * 2,
        firstRowY,
        stats.farmCount,
        stats.farmWip
    );

    renderStatBlock(
        renderer,
        getHouseIcon(
            houseIcons,
            HouseIconType::School
        ),
        uiResources.statisticsCounts,
        uiResources.statisticsWipCounts,
        firstX + stepX * 3,
        firstRowY,
        stats.schoolCount,
        stats.schoolWip
    );

    // Units: Worker a Serf jsou jedine runtime UnitType,
    // ktere KaMVita aktualne skutecne rozlisuje.
    renderStatBlock(
        renderer,
        uiResources.professions[
            static_cast<int>(
                GameUISchoolSelection::Worker
            )
        ].icon,
        uiResources.statisticsCounts,
        uiResources.statisticsWipCounts,
        firstX,
        secondRowY,
        stats.workerCount,
        0
    );

    renderStatBlock(
        renderer,
        uiResources.professions[
            static_cast<int>(
                GameUISchoolSelection::Serf
            )
        ].icon,
        uiResources.statisticsCounts,
        uiResources.statisticsWipCounts,
        firstX + stepX,
        secondRowY,
        stats.serfCount,
        0
    );

    // School Gold.
    renderStatBlock(
        renderer,
        uiResources.goldIcon,
        uiResources.statisticsCounts,
        uiResources.statisticsWipCounts,
        firstX,
        thirdRowY,
        stats.schoolGold,
        0
    );

    // Aktivni School training.
    renderStatBlock(
        renderer,
        uiResources.questionIcon,
        uiResources.statisticsCounts,
        uiResources.statisticsWipCounts,
        firstX + stepX,
        thirdRowY,
        stats.schoolTrainingCount,
        stats.schoolQueueCount
    );
}
