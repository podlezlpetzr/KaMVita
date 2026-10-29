#include "game_ui_ratios.h"

#include "game_ui.h"


namespace
{

static constexpr int RATIOS_TAB_X = 6;
static constexpr int RATIOS_TAB_Y = 244;
static constexpr int RATIOS_TAB_STEP = 40;
static constexpr int RATIOS_TAB_SIZE = 32;

static constexpr int RATIOS_HEADER_X = 10;
static constexpr int RATIOS_HEADER_Y = 300;

static constexpr int RATIOS_ROW_ICON_X = 10;
static constexpr int RATIOS_FIRST_ROW_Y = 350;
static constexpr int RATIOS_ROW_STEP = 48;

static constexpr int RATIOS_TRACK_X = 48;
static constexpr int RATIOS_TRACK_WIDTH = 136;
static constexpr int RATIOS_TRACK_HEIGHT = 20;
static constexpr int RATIOS_THUMB_WIDTH = 24;


bool inside(
    float x,
    float y,
    const SDL_Rect& area
)
{
    return
        x >= area.x &&
        x < area.x + area.w &&
        y >= area.y &&
        y < area.y + area.h;
}


WareRatioType activeWare(
    const GameUIState& ui
)
{
    int tab = ui.ratiosTab;

    if (tab < 0)
        tab = 0;

    if (tab >= WARE_RATIO_TAB_COUNT)
        tab = WARE_RATIO_TAB_COUNT - 1;

    return static_cast<WareRatioType>(tab);
}


SDL_Texture* targetIcon(
    const SchoolUIResources& resources,
    int tab,
    int row
)
{
    static const int iconTable[4][4] = {
        {0, 1, -1, -1},
        {2, 3, 0, 1},
        {4, 5, -1, -1},
        {6, 7, 8, -1}
    };

    if (
        tab < 0 ||
        tab >= 4 ||
        row < 0 ||
        row >= 4
    ) {
        return nullptr;
    }

    const int iconIndex =
        iconTable[tab][row];

    if (iconIndex < 0)
        return nullptr;

    return resources.ratiosHouseIcons[
        iconIndex
    ];
}


void renderCentered(
    SDL_Renderer* renderer,
    SDL_Texture* texture,
    const SDL_Rect& area
)
{
    if (!renderer || !texture)
        return;

    int width = 0;
    int height = 0;

    SDL_QueryTexture(
        texture,
        nullptr,
        nullptr,
        &width,
        &height
    );

    SDL_Rect destination = {
        area.x + (area.w - width) / 2,
        area.y + (area.h - height) / 2,
        width,
        height
    };

    SDL_RenderCopy(
        renderer,
        texture,
        nullptr,
        &destination
    );
}


void renderBevel(
    SDL_Renderer* renderer,
    const SDL_Rect& area,
    bool focused
)
{
    SDL_SetRenderDrawBlendMode(
        renderer,
        SDL_BLENDMODE_BLEND
    );

    SDL_SetRenderDrawColor(
        renderer,
        focused ? 72 : 49,
        focused ? 59 : 42,
        focused ? 38 : 32,
        focused ? 180 : 125
    );

    SDL_RenderFillRect(
        renderer,
        &area
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
        area.x,
        area.y,
        area.x + area.w - 1,
        area.y
    );

    SDL_RenderDrawLine(
        renderer,
        area.x,
        area.y,
        area.x,
        area.y + area.h - 1
    );

    SDL_SetRenderDrawColor(
        renderer,
        focused ? 218 : 171,
        focused ? 185 : 154,
        focused ? 102 : 121,
        255
    );

    SDL_RenderDrawLine(
        renderer,
        area.x,
        area.y + area.h - 1,
        area.x + area.w - 1,
        area.y + area.h - 1
    );

    SDL_RenderDrawLine(
        renderer,
        area.x + area.w - 1,
        area.y,
        area.x + area.w - 1,
        area.y + area.h - 1
    );
}


void renderTrackBar(
    SDL_Renderer* renderer,
    const SchoolUIResources& resources,
    int x,
    int y,
    int value,
    bool focused
)
{
    SDL_Rect track = {
        x + 2,
        y + 2,
        RATIOS_TRACK_WIDTH - 4,
        RATIOS_TRACK_HEIGHT - 4
    };

    renderBevel(
        renderer,
        track,
        false
    );

    const int usableWidth =
        RATIOS_TRACK_WIDTH -
        RATIOS_THUMB_WIDTH -
        4;

    const int thumbX =
        x + 2 +
        (
            usableWidth *
            (WARE_RATIO_MAX - value)
        ) /
        (
            WARE_RATIO_MAX -
            WARE_RATIO_MIN
        );

    SDL_Rect thumbArea = {
        thumbX,
        y,
        RATIOS_THUMB_WIDTH,
        RATIOS_TRACK_HEIGHT
    };

    if (focused) {
        SDL_SetRenderDrawColor(
            renderer,
            230, 190, 90, 255
        );

        SDL_Rect focus = {
            thumbArea.x - 1,
            thumbArea.y - 1,
            thumbArea.w + 2,
            thumbArea.h + 2
        };

        SDL_RenderDrawRect(
            renderer,
            &focus
        );
    }

    renderCentered(
        renderer,
        resources.ratiosTrackbarThumb,
        thumbArea
    );

    if (resources.statisticsCounts) {
        SDL_Rect numberSource = {
            0,
            value * 18,
            32,
            18
        };

        SDL_Rect numberDestination = {
            thumbArea.x +
                (thumbArea.w - 32) / 2,
            thumbArea.y + 1,
            32,
            18
        };

        SDL_RenderCopy(
            renderer,
            resources.statisticsCounts,
            &numberSource,
            &numberDestination
        );
    }
}

}


void renderGameUIRatios(
    SDL_Renderer* renderer,
    const SchoolUIResources& resources,
    const GameUIState& ui,
    const WareDistribution& distribution
)
{
    if (!renderer)
        return;

    for (
        int tab = 0;
        tab < WARE_RATIO_TAB_COUNT;
        ++tab
    ) {
        SDL_Rect tabArea = {
            RATIOS_TAB_X +
                tab * RATIOS_TAB_STEP,
            RATIOS_TAB_Y,
            RATIOS_TAB_SIZE,
            RATIOS_TAB_SIZE
        };

        const bool focused =
            ui.ratiosFocusTabs &&
            ui.ratiosTab == tab;

        renderBevel(
            renderer,
            tabArea,
            focused
        );

        SDL_Texture* icon =
            resources.ratiosWareIcons[tab];

        if (icon) {
            SDL_SetTextureColorMod(
                icon,
                ui.ratiosTab == tab
                    ? 255
                    : 150,
                ui.ratiosTab == tab
                    ? 255
                    : 150,
                ui.ratiosTab == tab
                    ? 255
                    : 150
            );

            renderCentered(
                renderer,
                icon,
                tabArea
            );

            SDL_SetTextureColorMod(
                icon,
                255,
                255,
                255
            );
        }
    }

    SDL_Rect headerArea = {
        RATIOS_HEADER_X,
        RATIOS_HEADER_Y,
        32,
        32
    };

    renderCentered(
        renderer,
        resources.ratiosWareIcons[
            ui.ratiosTab
        ],
        headerArea
    );

    const WareRatioType ware =
        activeWare(ui);

    const int targetCount =
        getWareRatioTargetCount(ware);

    for (
        int row = 0;
        row < targetCount;
        ++row
    ) {
        const int rowY =
            RATIOS_FIRST_ROW_Y +
            row * RATIOS_ROW_STEP;

        SDL_Rect iconArea = {
            RATIOS_ROW_ICON_X,
            rowY,
            32,
            32
        };

        renderBevel(
            renderer,
            iconArea,
            !ui.ratiosFocusTabs &&
                ui.ratiosRow == row
        );

        renderCentered(
            renderer,
            targetIcon(
                resources,
                ui.ratiosTab,
                row
            ),
            iconArea
        );

        const int value =
            getWareDistributionValue(
                distribution,
                ware,
                row
            );

        renderTrackBar(
            renderer,
            resources,
            RATIOS_TRACK_X,
            rowY + 6,
            value,
            !ui.ratiosFocusTabs &&
                ui.ratiosRow == row
        );
    }
}


bool handleGameUIRatiosTouch(
    GameUIState* ui,
    WareDistribution* distribution,
    float touchX,
    float touchY
)
{
    if (!ui || !distribution)
        return false;

    for (
        int tab = 0;
        tab < WARE_RATIO_TAB_COUNT;
        ++tab
    ) {
        SDL_Rect tabArea = {
            RATIOS_TAB_X +
                tab * RATIOS_TAB_STEP,
            RATIOS_TAB_Y,
            RATIOS_TAB_SIZE,
            RATIOS_TAB_SIZE
        };

        if (inside(touchX, touchY, tabArea)) {
            ui->ratiosTab = tab;
            ui->ratiosRow = 0;
            ui->ratiosFocusTabs = true;
            return true;
        }
    }

    const WareRatioType ware =
        activeWare(*ui);

    const int targetCount =
        getWareRatioTargetCount(ware);

    for (
        int row = 0;
        row < targetCount;
        ++row
    ) {
        SDL_Rect trackArea = {
            RATIOS_TRACK_X,
            RATIOS_FIRST_ROW_Y +
                row * RATIOS_ROW_STEP,
            RATIOS_TRACK_WIDTH,
            32
        };

        if (!inside(touchX, touchY, trackArea))
            continue;

        const int localX =
            static_cast<int>(
                touchX - trackArea.x
            );

        int value =
            WARE_RATIO_MAX -
            (
                localX *
                (
                    WARE_RATIO_MAX -
                    WARE_RATIO_MIN
                ) +
                RATIOS_TRACK_WIDTH / 2
            ) /
            RATIOS_TRACK_WIDTH;

        setWareDistributionValue(
            distribution,
            ware,
            row,
            value
        );

        ui->ratiosRow = row;
        ui->ratiosFocusTabs = false;
        return true;
    }

    return false;
}


void handleGameUIRatiosGamepad(
    GameUIState* ui,
    WareDistribution* distribution,
    bool leftPressed,
    bool rightPressed,
    bool upPressed,
    bool downPressed,
    bool crossPressed
)
{
    if (!ui || !distribution)
        return;

    if (ui->ratiosFocusTabs) {
        if (leftPressed) {
            --ui->ratiosTab;

            if (ui->ratiosTab < 0)
                ui->ratiosTab =
                    WARE_RATIO_TAB_COUNT - 1;

            ui->ratiosRow = 0;
        }

        if (rightPressed) {
            ++ui->ratiosTab;

            if (
                ui->ratiosTab >=
                    WARE_RATIO_TAB_COUNT
            ) {
                ui->ratiosTab = 0;
            }

            ui->ratiosRow = 0;
        }

        if (downPressed || crossPressed) {
            ui->ratiosFocusTabs = false;
            ui->ratiosRow = 0;
        }

        return;
    }

    const WareRatioType ware =
        activeWare(*ui);

    const int targetCount =
        getWareRatioTargetCount(ware);

    if (targetCount <= 0)
        return;

    if (upPressed) {
        if (ui->ratiosRow <= 0) {
            ui->ratiosRow = 0;
            ui->ratiosFocusTabs = true;
        } else {
            --ui->ratiosRow;
        }
    }

    if (downPressed) {
        ++ui->ratiosRow;

        if (ui->ratiosRow >= targetCount)
            ui->ratiosRow = targetCount - 1;
    }

    int value =
        getWareDistributionValue(
            *distribution,
            ware,
            ui->ratiosRow
        );

    if (leftPressed)
        --value;

    if (rightPressed)
        ++value;

    setWareDistributionValue(
        distribution,
        ware,
        ui->ratiosRow,
        value
    );

    if (crossPressed)
        ui->ratiosFocusTabs = true;
}
