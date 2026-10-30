#include "game_ui_unit.h"


static SDL_Texture* loadUnitUIColorKeyTexture(
    SDL_Renderer* renderer,
    const char* path
)
{
    SDL_Surface* surface =
        SDL_LoadBMP(path);

    if (!surface)
        return nullptr;

    SDL_SetColorKey(
        surface,
        SDL_TRUE,
        SDL_MapRGB(
            surface->format,
            255,
            0,
            255
        )
    );

    SDL_Texture* texture =
        SDL_CreateTextureFromSurface(
            renderer,
            surface
        );

    SDL_FreeSurface(surface);
    return texture;
}


void initUnitUI(
    UnitUIState* state
)
{
    if (!state)
        return;

    state->selectedUnitIndex = -1;
    state->panelVisible = false;
}


UnitUIResources loadUnitUIResources(
    SDL_Renderer* renderer
)
{
    UnitUIResources resources = {};

    resources.sidebarMiddle =
        loadUnitUIColorKeyTexture(
            renderer,
            "app0:assets/ui/sidebar/"
            "sidebar_middle.bmp"
        );

    resources.buttonStone =
        loadUnitUIColorKeyTexture(
            renderer,
            "app0:assets/ui/sidebar/buttons/"
            "gui_402.bmp"
        );

    resources.back443 =
        loadUnitUIColorKeyTexture(
            renderer,
            "app0:assets/ui/units/"
            "back_443.bmp"
        );

    // Remake Image_UnitPic pouziva
    // gRes.Units[UnitType].GUIScroll.
    resources.workerScroll =
        loadUnitUIColorKeyTexture(
            renderer,
            "app0:assets/ui/school/"
            "worker_scroll.bmp"
        );

    resources.serfScroll =
        loadUnitUIColorKeyTexture(
            renderer,
            "app0:assets/ui/school/"
            "serf_scroll.bmp"
        );

    resources.workerName =
        loadUnitUIColorKeyTexture(
            renderer,
            "app0:assets/ui/school/text/"
            "professions/worker.bmp"
        );

    resources.serfName =
        loadUnitUIColorKeyTexture(
            renderer,
            "app0:assets/ui/school/text/"
            "professions/serf.bmp"
        );

    resources.conditionText =
        loadUnitUIColorKeyTexture(
            renderer,
            "app0:assets/ui/units/text/condition.bmp"
        );

    resources.idleText =
        loadUnitUIColorKeyTexture(
            renderer,
            "app0:assets/ui/units/text/idle.bmp"
        );

    resources.walkingText =
        loadUnitUIColorKeyTexture(
            renderer,
            "app0:assets/ui/units/text/walking.bmp"
        );

    resources.roadText =
        loadUnitUIColorKeyTexture(
            renderer,
            "app0:assets/ui/units/text/road.bmp"
        );

    resources.fieldText =
        loadUnitUIColorKeyTexture(
            renderer,
            "app0:assets/ui/units/text/field.bmp"
        );

    resources.wineText =
        loadUnitUIColorKeyTexture(
            renderer,
            "app0:assets/ui/units/text/wine.bmp"
        );

    resources.houseSiteText =
        loadUnitUIColorKeyTexture(
            renderer,
            "app0:assets/ui/units/text/house_site.bmp"
        );

    resources.houseText =
        loadUnitUIColorKeyTexture(
            renderer,
            "app0:assets/ui/units/text/house.bmp"
        );

    resources.repairText =
        loadUnitUIColorKeyTexture(
            renderer,
            "app0:assets/ui/units/text/repair.bmp"
        );

    resources.serfDescription =
        loadUnitUIColorKeyTexture(
            renderer,
            "app0:assets/ui/units/text/serf_description.bmp"
        );

    resources.workerDescription =
        loadUnitUIColorKeyTexture(
            renderer,
            "app0:assets/ui/units/text/worker_description.bmp"
        );

    return resources;
}


void destroyUnitUIResources(
    UnitUIResources* resources
)
{
    if (!resources)
        return;

    SDL_DestroyTexture(
        resources->sidebarMiddle
    );

    SDL_DestroyTexture(
        resources->buttonStone
    );

    SDL_DestroyTexture(
        resources->back443
    );

    SDL_DestroyTexture(
        resources->workerScroll
    );

    SDL_DestroyTexture(
        resources->serfScroll
    );

    SDL_DestroyTexture(
        resources->workerName
    );

    SDL_DestroyTexture(
        resources->serfName
    );

    SDL_DestroyTexture(
        resources->conditionText
    );

    SDL_DestroyTexture(
        resources->idleText
    );

    SDL_DestroyTexture(
        resources->walkingText
    );

    SDL_DestroyTexture(
        resources->roadText
    );

    SDL_DestroyTexture(
        resources->fieldText
    );

    SDL_DestroyTexture(
        resources->wineText
    );

    SDL_DestroyTexture(
        resources->houseSiteText
    );

    SDL_DestroyTexture(
        resources->houseText
    );

    SDL_DestroyTexture(
        resources->repairText
    );

    SDL_DestroyTexture(
        resources->serfDescription
    );

    SDL_DestroyTexture(
        resources->workerDescription
    );

    *resources = {};
}


int hitTestUnit(
    const Unit* units,
    int unitCount,
    float screenX,
    float screenY,
    float cameraX,
    float cameraY
)
{
    if (!units)
        return -1;

    // Prochazime odzadu, aby pri prekryvu dostala
    // prednost naposledy renderovana jednotka.
    for (
        int i = unitCount - 1;
        i >= 0;
        --i
    ) {
        const Unit& unit = units[i];

        if (
            !unit.active ||
            !unit.visible
        ) {
            continue;
        }

        const bool serf =
            unit.type == UnitType::Serf;

        const int selectionWidth =
            serf ? 43 : 56;

        const int selectionHeight =
            serf ? 47 : 50;

        const int anchorX =
            serf ? -18 : -21;

        const int anchorY =
            serf ? -38 : -42;

        SDL_Rect area = {
            (int)unit.worldX -
                (int)cameraX +
                anchorX -
                5,

            (int)unit.worldY -
                (int)cameraY +
                anchorY -
                5,

            selectionWidth + 10,
            selectionHeight + 10
        };

        if (
            screenX >= area.x &&
            screenX < area.x + area.w &&
            screenY >= area.y &&
            screenY < area.y + area.h
        ) {
            return i;
        }
    }

    return -1;
}


void renderSelectedUnitMarker(
    SDL_Renderer* renderer,
    const Unit* units,
    int unitCount,
    int selectedUnitIndex,
    float cameraX,
    float cameraY
)
{
    if (
        !renderer ||
        !units ||
        selectedUnitIndex < 0 ||
        selectedUnitIndex >= unitCount
    ) {
        return;
    }

    const Unit& unit =
        units[selectedUnitIndex];

    if (
        !unit.active ||
        !unit.visible
    ) {
        return;
    }

    SDL_SetRenderDrawBlendMode(
        renderer,
        SDL_BLENDMODE_BLEND
    );

    SDL_SetRenderDrawColor(
        renderer,
        255,
        220,
        70,
        210
    );

    const int centerX =
        (int)unit.worldX -
        (int)cameraX;

    const int centerY =
        (int)unit.worldY -
        (int)cameraY -
        3;

    SDL_Rect marker = {
        centerX - 15,
        centerY - 8,
        30,
        12
    };

    SDL_RenderDrawRect(
        renderer,
        &marker
    );

    SDL_Rect innerMarker = {
        marker.x + 2,
        marker.y + 2,
        marker.w - 4,
        marker.h - 4
    };

    SDL_RenderDrawRect(
        renderer,
        &innerMarker
    );
}


static SDL_Texture* getUnitScroll(
    const UnitUIResources& resources,
    UnitType type
)
{
    return
        type == UnitType::Serf
            ? resources.serfScroll
            : resources.workerScroll;
}


static SDL_Texture* getUnitName(
    const UnitUIResources& resources,
    UnitType type
)
{
    return
        type == UnitType::Serf
            ? resources.serfName
            : resources.workerName;
}




static SDL_Texture* getUnitActivityText(
    const UnitUIResources& resources,
    const Unit& unit
)
{
    if (
        unit.state == UnitState::Walking ||
        unit.state == UnitState::LeavingBuilding
    ) {
        return resources.walkingText;
    }

    if (
        unit.state != UnitState::Working
    ) {
        return resources.idleText;
    }

    switch (unit.workAction) {
        case UnitWorkAction::DigRoad:
        case UnitWorkAction::PaveRoad:
            return resources.roadText;

        case UnitWorkAction::BuildField:
            return resources.fieldText;

        case UnitWorkAction::BuildWine:
        case UnitWorkAction::BuildWineWood:
            return resources.wineText;

        case UnitWorkAction::BuildHouse:
            // Current House phase is terrain preparation:
            // Remake "Flattening house site".
            return resources.houseSiteText;

        case UnitWorkAction::WaitForStone:
        case UnitWorkAction::None:
        default:
            return resources.idleText;
    }
}


static SDL_Texture* getUnitDescriptionText(
    const UnitUIResources& resources,
    UnitType type
)
{
    return
        type == UnitType::Serf
            ? resources.serfDescription
            : resources.workerDescription;
}


SDL_Rect getUnitUIBackRect()
{
    // Originalni KaM toolbar radek zacina na Y=194.
    // Aktivni dotykova plocha zustava 42x36,
    // zatimco GUI 443 se kresli v nativni velikosti 28x19.
    return {
        6,
        194,
        42,
        36
    };
}


void closeUnitUI(
    UnitUIState* state
)
{
    if (!state)
        return;

    state->selectedUnitIndex = -1;
    state->panelVisible = false;
}


bool handleUnitUIBackTouch(
    UnitUIState* state,
    float touchX,
    float touchY
)
{
    if (
        !state ||
        !state->panelVisible
    ) {
        return false;
    }

    const SDL_Rect backArea =
        getUnitUIBackRect();

    const bool inside =
        touchX >= backArea.x &&
        touchX < backArea.x + backArea.w &&
        touchY >= backArea.y &&
        touchY < backArea.y + backArea.h;

    if (!inside)
        return false;

    closeUnitUI(state);
    return true;
}


void renderUnitUI(
    SDL_Renderer* renderer,
    const UnitUIResources& resources,
    const UnitUIState& state,
    const Unit* units,
    int unitCount
)
{
    if (
        !renderer ||
        !units ||
        !state.panelVisible ||
        state.selectedUnitIndex < 0 ||
        state.selectedUnitIndex >= unitCount
    ) {
        return;
    }

    const Unit& unit =
        units[state.selectedUnitIndex];

    if (
        !unit.active ||
        !unit.visible
    ) {
        return;
    }


    // =====================================================
    // KaM Remake Unit page
    //
    // Original:
    //
    // Panel_Unit:
    //   TB_PAD, 44, TB_WIDTH, 332
    //
    // Label name:
    //   0,16,TB_WIDTH,30
    //
    // GUIScroll:
    //   0,38,54,100
    //
    // Condition:
    //   65,40,116,30
    //
    // ConditionBar:
    //   65,55,116,15
    //
    // Task:
    //   65,80,116,60
    //
    // Description:
    //   0,152,TB_WIDTH,200
    //
    // Vita layout:
    // Panel_Controls zacina pod sdilenou horní oblasti.
    // Back 443 zustava na toolbar Y=194.
    // Samotny Unit obsah zacina niz.
    // =====================================================

    const int controlsX = 6;
    const int controlsY = 194;

    const int unitPanelX =
        controlsX;

    const int unitPanelY =
        controlsY + 44;

    const int unitPanelWidth = 180;

    const int unitPanelHeight =
        544 - unitPanelY;


    // -----------------------------------------------------
    // AUTHENTIC SIDEBAR BACKGROUND
    //
    // Zakladni renderGameUI() uz vykreslil originalni
    // sidebarTop 407 a sidebarBottom 404.
    //
    // Unit page je skutecna GameUISection, a proto zde
    // nekreslime zadny dalsi jednobarevny panel.
    // -----------------------------------------------------


    // =====================================================
    // BACK 443
    // =====================================================

    const SDL_Rect backArea =
        getUnitUIBackRect();

    // Samotny GUI glyph 443.
    // Provizorni hnedy podklad a vlastni ramecek jsou
    // odstraneny. Autenticky bsGame stone doplnime
    // ze stejneho zdroje jako 439..442.

    // Original bsGame stone for Back 443.
    // Same GUI 402 source used by toolbar buttons 439..442.
    if (resources.buttonStone) {
        SDL_Rect stoneSource = {
            12,
            122,
            backArea.w,
            backArea.h
        };

        SDL_RenderCopy(
            renderer,
            resources.buttonStone,
            &stoneSource,
            &backArea
        );
    }

    if (resources.back443) {
        int bw = 0;
        int bh = 0;

        SDL_QueryTexture(
            resources.back443,
            nullptr,
            nullptr,
            &bw,
            &bh
        );

        if (bw > 0 && bh > 0) {
            SDL_Rect dst = {
                backArea.x +
                    (backArea.w - bw) / 2,

                backArea.y +
                    (backArea.h - bh) / 2,

                bw,
                bh
            };

            SDL_RenderCopy(
                renderer,
                resources.back443,
                nullptr,
                &dst
            );
        }
    }


    // =====================================================
    // UNIT NAME
    //
    // Remake relative Y = 16.
    // =====================================================

    SDL_Texture* name =
        getUnitName(
            resources,
            unit.type
        );

    if (name) {
        int nw = 0;
        int nh = 0;

        SDL_QueryTexture(
            name,
            nullptr,
            nullptr,
            &nw,
            &nh
        );

        if (nw > 0 && nh > 0) {
            SDL_Rect dst = {
                unitPanelX +
                    (
                        unitPanelWidth -
                        nw
                    ) / 2,

                unitPanelY + 16 +
                    (30 - nh) / 2,

                nw,
                nh
            };

            SDL_RenderCopy(
                renderer,
                name,
                nullptr,
                &dst
            );
        }
    }


    // =====================================================
    // ORIGINAL GUISCROLL PORTRAIT
    //
    // Remake destination: 0,38,54,100.
    //
    // Extracted GUIScroll is 54x80.
    // We keep the original image aspect and do not stretch
    // it vertically to invent missing pixels.
    // =====================================================

    SDL_Texture* portrait =
        getUnitScroll(
            resources,
            unit.type
        );

    if (portrait) {
        int pw = 0;
        int ph = 0;

        SDL_QueryTexture(
            portrait,
            nullptr,
            nullptr,
            &pw,
            &ph
        );

        if (pw > 0 && ph > 0) {
            SDL_Rect portraitDestination = {
                unitPanelX,
                unitPanelY + 38,
                pw,
                ph
            };

            SDL_RenderCopy(
                renderer,
                portrait,
                nullptr,
                &portraitDestination
            );
        }
    }


    // =====================================================
    // LIVE KAM REMAKE CONDITION
    //
    // ConditionBar_Unit.Position =
    //     Sender.Condition / UNIT_MAX_CONDITION
    //
    // Remake geometry: 65,55,116,15.
    // =====================================================

    // Original Remake fnt_Grey "Condition".
    if (resources.conditionText) {
        int conditionTextWidth = 0;
        int conditionTextHeight = 0;

        SDL_QueryTexture(
            resources.conditionText,
            nullptr,
            nullptr,
            &conditionTextWidth,
            &conditionTextHeight
        );

        SDL_Rect conditionLabel = {
            unitPanelX + 65,
            unitPanelY + 40,
            conditionTextWidth,
            conditionTextHeight
        };

        SDL_RenderCopy(
            renderer,
            resources.conditionText,
            nullptr,
            &conditionLabel
        );
    }

    SDL_Rect conditionFrame = {
        unitPanelX + 65,
        unitPanelY + 55,
        116,
        15
    };

    int displayedCondition =
        unit.condition;

    if (displayedCondition < 0)
        displayedCondition = 0;

    if (
        displayedCondition >
        UNIT_MAX_CONDITION
    ) {
        displayedCondition =
            UNIT_MAX_CONDITION;
    }

    SDL_SetRenderDrawColor(
        renderer,
        18,
        18,
        14,
        255
    );

    SDL_RenderFillRect(
        renderer,
        &conditionFrame
    );

    SDL_Rect conditionFill = {
        conditionFrame.x + 2,
        conditionFrame.y + 2,

        (
            conditionFrame.w - 4
        ) *
            displayedCondition /
            UNIT_MAX_CONDITION,

        conditionFrame.h - 4
    };

    if (conditionFill.w > 0) {
        SDL_SetRenderDrawColor(
            renderer,
            35,
            188,
            52,
            255
        );

        SDL_RenderFillRect(
            renderer,
            &conditionFill
        );
    }

    SDL_SetRenderDrawColor(
        renderer,
        188,
        157,
        91,
        255
    );

    SDL_RenderDrawRect(
        renderer,
        &conditionFrame
    );


    // =====================================================
    // TASK AREA
    //
    // Remake: 65,80,116,60.
    //
    // Tento obdelnik pouze rezervuje presnou Remake
    // geometrii. Text doplnime z originalniho fnt_Grey
    // pipeline, ne vlastnim provizornim symbolem.
    // =====================================================

    SDL_Rect taskArea = {
        unitPanelX + 65,
        unitPanelY + 80,
        116,
        60
    };

    // Original Remake Label_UnitTask using fnt_Grey.
    SDL_Texture* activityText =
        getUnitActivityText(
            resources,
            unit
        );

    if (activityText) {
        int activityWidth = 0;
        int activityHeight = 0;

        SDL_QueryTexture(
            activityText,
            nullptr,
            nullptr,
            &activityWidth,
            &activityHeight
        );

        SDL_Rect activitySource = {
            0,
            0,
            activityWidth,
            activityHeight
        };

        int visibleWidth =
            activityWidth;

        if (visibleWidth > taskArea.w)
            visibleWidth = taskArea.w;

        activitySource.w =
            visibleWidth;

        SDL_Rect activityDestination = {
            taskArea.x,
            taskArea.y,
            visibleWidth,
            activityHeight
        };

        SDL_RenderCopy(
            renderer,
            activityText,
            &activitySource,
            &activityDestination
        );
    }


    // =====================================================
    // DESCRIPTION AREA
    //
    // Remake: 0,152,TB_WIDTH,200.
    // Na Vita displeji je spodni cast omezena dostupnou
    // vyskou obrazovky.
    // =====================================================

    const int descriptionY =
        unitPanelY + 152;

    const int descriptionBottom =
        unitPanelY +
        unitPanelHeight;

    const int descriptionHeight =
        descriptionBottom >
            descriptionY
                ? descriptionBottom -
                    descriptionY
                : 0;

    if (descriptionHeight > 0) {
        SDL_Rect descriptionArea = {
            unitPanelX,
            descriptionY,
            unitPanelWidth,
            descriptionHeight
        };

        // Original Remake unit description rendered with
        // fnt_Grey and pre-wrapped to Unit panel width.
        SDL_Texture* descriptionText =
            getUnitDescriptionText(
                resources,
                unit.type
            );

        if (descriptionText) {
            int descriptionWidth = 0;
            int descriptionHeight = 0;

            SDL_QueryTexture(
                descriptionText,
                nullptr,
                nullptr,
                &descriptionWidth,
                &descriptionHeight
            );

            int visibleWidth =
                descriptionWidth;

            int visibleHeight =
                descriptionHeight;

            if (
                visibleWidth >
                descriptionArea.w
            ) {
                visibleWidth =
                    descriptionArea.w;
            }

            if (
                visibleHeight >
                descriptionArea.h
            ) {
                visibleHeight =
                    descriptionArea.h;
            }

            if (
                visibleWidth > 0 &&
                visibleHeight > 0
            ) {
                SDL_Rect source = {
                    0,
                    0,
                    visibleWidth,
                    visibleHeight
                };

                SDL_Rect destination = {
                    descriptionArea.x,
                    descriptionArea.y,
                    visibleWidth,
                    visibleHeight
                };

                SDL_RenderCopy(
                    renderer,
                    descriptionText,
                    &source,
                    &destination
                );
            }
        }
    }
}
