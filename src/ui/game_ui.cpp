#include "game_ui.h"
#include "game_ui_statistics.h"
#include "game_ui_ratios.h"
#include <cstdio>

void initGameUI(
    GameUIState* ui
)
{
    if (!ui)
        return;

    ui->buildSelection =
        GameUIBuildSelection::Road;

    ui->buildSectionActive = true;
    ui->buildToolSelection = 0;
    ui->buildHouseCatalogSelection = -1;

    ui->section = GameUISection::Build;
    ui->toolbarSelection = 0;
    ui->toolbarFocus = false;

    ui->ratiosTab = 0;
    ui->ratiosRow = 0;
    ui->ratiosFocusTabs = true;

    ui->schoolSelection =
        GameUISchoolSelection::Serf;

    ui->schoolQueueFocus = false;
    ui->schoolQueueSelection = 0;

    ui->selectedSchoolBuilding = -1;
}

void renderGameUI(
    SDL_Renderer* renderer,
    const BuildingResources& resources,
    const SchoolUIResources& schoolResources,
    const HouseIconResources& houseIcons,
    const GameUIState& ui,
    const Building* buildings,
    int buildingCount,
    const Unit* units,
    int unitCount,
    const WareDistribution& wareDistribution,
    float cursorX,
    float cursorY
)
{
    if (!renderer)
        return;

    const Building* selectedSchool = nullptr;

    if (
        buildings &&
        ui.section == GameUISection::School &&
        ui.selectedSchoolBuilding >= 0 &&
        ui.selectedSchoolBuilding < buildingCount
    ) {
        const Building& candidate =
            buildings[ui.selectedSchoolBuilding];

        if (
            candidate.active &&
            candidate.type == BuildingType::School
        ) {
            selectedSchool = &candidate;
        }
    }

    // Vita varianta autentickeho KaM sidebaru.
    //
    // PC Remake:
    //   407 = top
    //   554 = middle s logem
    //   404 = bottom / Panel_Controls
    //
    // Vita ma pouze 544 px vysky. Prostredni sprite 554
    // proto zamerne vynechavame a po horni casti 407
    // okamzite navazujeme cistym drevenym spritem 404.
    // Samotne originalni assety zustavaji beze zmeny.

    SDL_Rect sidebarFallback = {
        0,
        0,
        GAME_UI_PANEL_WIDTH,
        544
    };

    SDL_SetRenderDrawColor(
        renderer,
        35, 29, 22, 255
    );
    SDL_RenderFillRect(
        renderer,
        &sidebarFallback
    );

    // Horni cast 407.
    if (schoolResources.sidebarTop) {
        SDL_Rect topDestination = {
            0,
            0,
            GAME_UI_PANEL_WIDTH,
            200
        };

        SDL_RenderCopy(
            renderer,
            schoolResources.sidebarTop,
            nullptr,
            &topDestination
        );
    }

    // Od Y=200 az na spodek Vity pouzijeme sprite 404.
    // Zdrojovy sprite ma 224x400, potrebujeme prvnich
    // 344 px z jeho vysky.
    if (schoolResources.sidebarBottom) {
        SDL_Rect bottomSource = {
            0,
            0,
            224,
            344
        };

        SDL_Rect bottomDestination = {
            0,
            200,
            GAME_UI_PANEL_WIDTH,
            344
        };

        SDL_RenderCopy(
            renderer,
            schoolResources.sidebarBottom,
            &bottomSource,
            &bottomDestination
        );
    }

    // =====================================================
    // ORIGINAL KAM MAIN TOOLBAR
    // GUI 439 / 440 / 441 / 442
    // Original: 42x36, horizontalni krok 46 px.
    // =====================================================

    if (
        ui.section != GameUISection::Unit
    ) {
        const int toolbarX = 6;
        const int toolbarY = 194;

        for (int i = 0; i < 4; ++i) {
            SDL_Rect buttonArea = {
                toolbarX + i * 46,
                toolbarY,
                42,
                36
            };

            // Autenticky bsGame kamen.
            if (schoolResources.buttonStone) {
                SDL_Rect stoneSource = {
                    12 + i * 44,
                    122,
                    buttonArea.w,
                    buttonArea.h
                };

                SDL_RenderCopy(
                    renderer,
                    schoolResources.buttonStone,
                    &stoneSource,
                    &buttonArea
                );
            }

            // Jemny bevel.
            SDL_SetRenderDrawColor(
                renderer,
                226, 216, 190, 255
            );

            SDL_RenderDrawLine(
                renderer,
                buttonArea.x,
                buttonArea.y,
                buttonArea.x + buttonArea.w - 1,
                buttonArea.y
            );

            SDL_RenderDrawLine(
                renderer,
                buttonArea.x,
                buttonArea.y,
                buttonArea.x,
                buttonArea.y + buttonArea.h - 1
            );

            SDL_SetRenderDrawColor(
                renderer,
                43, 36, 29, 255
            );

            SDL_RenderDrawLine(
                renderer,
                buttonArea.x,
                buttonArea.y + buttonArea.h - 1,
                buttonArea.x + buttonArea.w - 1,
                buttonArea.y + buttonArea.h - 1
            );

            SDL_RenderDrawLine(
                renderer,
                buttonArea.x + buttonArea.w - 1,
                buttonArea.y,
                buttonArea.x + buttonArea.w - 1,
                buttonArea.y + buttonArea.h - 1
            );

            // Aktivni hlavni sekce je oznacena pouze
            // jemnym zapustenim. Rozmery a pozice toolbaru
            // zustavaji presne beze zmeny.
            bool toolbarButtonSelected =
                ui.section != GameUISection::School &&
                ui.toolbarSelection == i;

            if (toolbarButtonSelected) {
                SDL_SetRenderDrawBlendMode(
                    renderer,
                    SDL_BLENDMODE_BLEND
                );

                SDL_SetRenderDrawColor(
                    renderer,
                    55, 44, 31, 70
                );

                SDL_RenderFillRect(
                    renderer,
                    &buttonArea
                );

                SDL_SetRenderDrawBlendMode(
                    renderer,
                    SDL_BLENDMODE_NONE
                );
            }

            // Originalni GUI 439..442 se uz nacitaji
            // v mainButtons[0..3].
            SDL_Texture* glyph =
                schoolResources.mainButtons[i];

            bool toolbarButtonActive = false;

            switch (i) {
                case 0:
                    toolbarButtonActive =
                        ui.section ==
                            GameUISection::Build;
                    break;

                case 1:
                    toolbarButtonActive =
                        ui.section ==
                            GameUISection::Ratios;
                    break;

                case 2:
                    toolbarButtonActive =
                        ui.section ==
                            GameUISection::Statistics;
                    break;

                case 3:
                    toolbarButtonActive =
                        ui.section ==
                            GameUISection::Options;
                    break;

                default:
                    break;
            }

            if (glyph) {
                const Uint8 toolbarBrightness =
                    toolbarButtonActive
                        ? 255
                        : 185;

                SDL_SetTextureColorMod(
                    glyph,
                    toolbarBrightness,
                    toolbarBrightness,
                    toolbarBrightness
                );
                int glyphWidth = 0;
                int glyphHeight = 0;

                SDL_QueryTexture(
                    glyph,
                    nullptr,
                    nullptr,
                    &glyphWidth,
                    &glyphHeight
                );

                SDL_Rect glyphDestination = {
                    buttonArea.x +
                        (buttonArea.w - glyphWidth) / 2,
                    buttonArea.y +
                        (buttonArea.h - glyphHeight) / 2,
                    glyphWidth,
                    glyphHeight
                };

                // Build je skutecne funkcni.
                // Ostatni zustavaji normalne viditelne,
                // dokud nebudou mit vlastni sekce.
                SDL_RenderCopy(
                    renderer,
                    glyph,
                    nullptr,
                    &glyphDestination
                );

                SDL_SetTextureColorMod(
                    glyph,
                    255,
                    255,
                    255
                );
            }
        }
    }

    if (
        ui.section ==
            GameUISection::Ratios
    ) {
        renderGameUIRatios(
            renderer,
            schoolResources,
            houseIcons,
            ui,
            wareDistribution
        );
    }

    if (
        ui.section ==
            GameUISection::Statistics
    ) {
        const GameUIStatistics statistics =
            calculateGameUIStatistics(
                buildings,
                buildingCount,
                units,
                unitCount
            );

        renderGameUIStatistics(
            renderer,
            houseIcons,
            schoolResources,
            statistics
        );
    }

    // School GUI podle puvodniho KaM.
    if (ui.section == GameUISection::School) {
        // Originalni obsah School panelu ma sirku 180 px
        // a vysku 266 px. Na Vita panelu ho centrujeme.
        // Vita layout: School je posunuta blize
        // k levemu okraji sidebaru.
        const int panelX = 6;

        // School lezi nad autentickym KaM sidebarem.
        // Cely sidebar se proto neprekryva jednobarevnou plochou.

        // Puvodni School obsah zacina uvnitr
        // house panelu na Y = 76.
        // Na PC je School uvnitr Panel_Controls a Panel_House.
        // Vita ma pouze 544 px vysky, proto zachovame vnitrni
        // remake layout 266 px a ukotvime ho ke spodnimu okraji.
        const int panelY = 274;

        const int contentWidth = 180;
        const int contentHeight = 266;

        // Vita: School uz nema vlastni nepruhledny podklad.
        // Funkcni obsah lezi primo na autentickem dreve 404.
        SDL_Rect schoolPanel = {
            panelX - 6,
            panelY - 4,
            contentWidth + 12,
            contentHeight + 8
        };

        // schoolPanel zatim zachovavame jako layout reference,
        // ale nekreslime jeho vypln ani ramecek.
        (void)schoolPanel;

        // Horni oddeleni NEEDS / Gold.
        SDL_Rect needsArea = {
            panelX,
            panelY + 2,
            contentWidth,
            42
        };

        // NEEDS oblast zustava pouze jako layout reference.
        // Pozadi poskytuje primo autenticky sidebar 404.
        (void)needsArea;

        // Originalni fnt_Outline nazev budovy.
        // Vita: absolutni Y=224, centrovano v 180px School obsahu.
        if (schoolResources.schoolHouseText) {
            int titleWidth = 0;
            int titleHeight = 0;

            SDL_QueryTexture(
                schoolResources.schoolHouseText,
                nullptr,
                nullptr,
                &titleWidth,
                &titleHeight
            );

            SDL_Rect titleDestination = {
                panelX +
                    (contentWidth - titleWidth) / 2,
                229,
                titleWidth,
                titleHeight
            };

            SDL_RenderCopy(
                renderer,
                schoolResources.schoolHouseText,
                nullptr,
                &titleDestination
            );
        }

        // =================================================
        // ORIGINAL KaM HOUSE HEADER
        //
        // Original:
        // House panel Y + 42:
        // 0 delivery, 30 repair, 60 logo, 90 worker
        // health X=120, Y=57, 55x15.
        //
        // Vita School content zacina na panelY=274,
        // coz odpovida originalnimu House Y+76.
        // House top je tedy panelY-76 = 198.
        // =================================================

        const int houseTop = panelY - 76;

        // Vita layout:
        // na Y=224 bude originalni nazev budovy.
        // House controls jsou proto o 4 px nize nez
        // puvodni House +42, aby pod titulkem dychaly.
        const int houseControlsY = houseTop + 51;

        SDL_Rect deliveryButton = {
            panelX,
            houseControlsY,
            30,
            30
        };

        SDL_Rect repairButton = {
            panelX + 30,
            houseControlsY,
            30,
            30
        };

        SDL_Rect houseLogoArea = {
            panelX + 60,
            houseControlsY - 1,
            32,
            32
        };

        SDL_Rect workerArea = {
            panelX + 90,
            houseControlsY - 1,
            32,
            32
        };

        SDL_Rect healthBar = {
            panelX + 120,
            houseTop + 66,
            55,
            15
        };

        // -------------------------------------------------
        // Delivery + Repair bsGame podklady
        // -------------------------------------------------

        SDL_Rect houseButtons[2] = {
            deliveryButton,
            repairButton
        };

        for (int buttonIndex = 0;
             buttonIndex < 2;
             ++buttonIndex) {

            if (schoolResources.buttonStone) {
                SDL_Rect source = {
                    12 + buttonIndex * 34,
                    40,
                    houseButtons[buttonIndex].w,
                    houseButtons[buttonIndex].h
                };

                SDL_RenderCopy(
                    renderer,
                    schoolResources.buttonStone,
                    &source,
                    &houseButtons[buttonIndex]
                );
            }

            // Zasunutý game bevel.
            SDL_SetRenderDrawColor(
                renderer,
                38, 32, 26, 255
            );

            SDL_RenderDrawLine(
                renderer,
                houseButtons[buttonIndex].x,
                houseButtons[buttonIndex].y,
                houseButtons[buttonIndex].x +
                    houseButtons[buttonIndex].w - 1,
                houseButtons[buttonIndex].y
            );

            SDL_RenderDrawLine(
                renderer,
                houseButtons[buttonIndex].x,
                houseButtons[buttonIndex].y,
                houseButtons[buttonIndex].x,
                houseButtons[buttonIndex].y +
                    houseButtons[buttonIndex].h - 1
            );

            SDL_SetRenderDrawColor(
                renderer,
                184, 169, 137, 255
            );

            SDL_RenderDrawLine(
                renderer,
                houseButtons[buttonIndex].x,
                houseButtons[buttonIndex].y +
                    houseButtons[buttonIndex].h - 1,
                houseButtons[buttonIndex].x +
                    houseButtons[buttonIndex].w - 1,
                houseButtons[buttonIndex].y +
                    houseButtons[buttonIndex].h - 1
            );

            SDL_RenderDrawLine(
                renderer,
                houseButtons[buttonIndex].x +
                    houseButtons[buttonIndex].w - 1,
                houseButtons[buttonIndex].y,
                houseButtons[buttonIndex].x +
                    houseButtons[buttonIndex].w - 1,
                houseButtons[buttonIndex].y +
                    houseButtons[buttonIndex].h - 1
            );
        }

        if (selectedSchool) {
            // ---------------------------------------------
            // Delivery 37/38
            // ---------------------------------------------

            SDL_Texture* deliveryGlyph =
                selectedSchool->wareDeliveryEnabled
                    ? schoolResources.deliveryOn
                    : schoolResources.deliveryOff;

            if (deliveryGlyph) {
                int w = 0;
                int h = 0;

                SDL_QueryTexture(
                    deliveryGlyph,
                    nullptr,
                    nullptr,
                    &w,
                    &h
                );

                SDL_Rect dst = {
                    deliveryButton.x +
                        (deliveryButton.w - w) / 2,
                    deliveryButton.y +
                        (deliveryButton.h - h) / 2,
                    w,
                    h
                };

                SDL_RenderCopy(
                    renderer,
                    deliveryGlyph,
                    nullptr,
                    &dst
                );
            }

            // ---------------------------------------------
            // Repair 39/40
            // ---------------------------------------------

            SDL_Texture* repairGlyph =
                selectedSchool->repairEnabled
                    ? schoolResources.repairOn
                    : schoolResources.repairOff;

            if (repairGlyph) {
                int w = 0;
                int h = 0;

                SDL_QueryTexture(
                    repairGlyph,
                    nullptr,
                    nullptr,
                    &w,
                    &h
                );

                SDL_Rect dst = {
                    repairButton.x +
                        (repairButton.w - w) / 2,
                    repairButton.y +
                        (repairButton.h - h) / 2,
                    w,
                    h
                };

                SDL_RenderCopy(
                    renderer,
                    repairGlyph,
                    nullptr,
                    &dst
                );
            }
        }

        // -------------------------------------------------
        // Skutecna School ikona
        // -------------------------------------------------

        if (resources.schoolIcon) {
            int w = 0;
            int h = 0;

            SDL_QueryTexture(
                resources.schoolIcon,
                nullptr,
                nullptr,
                &w,
                &h
            );

            float scaleX =
                (float)houseLogoArea.w / (float)w;

            float scaleY =
                (float)houseLogoArea.h / (float)h;

            float scale =
                scaleX < scaleY ? scaleX : scaleY;

            if (scale > 1.0f)
                scale = 1.0f;

            int dw = (int)(w * scale);
            int dh = (int)(h * scale);

            SDL_Rect dst = {
                houseLogoArea.x +
                    (houseLogoArea.w - dw) / 2,
                houseLogoArea.y +
                    (houseLogoArea.h - dh) / 2,
                dw,
                dh
            };

            SDL_RenderCopy(
                renderer,
                resources.schoolIcon,
                nullptr,
                &dst
            );
        }

        // -------------------------------------------------
        // Worker 141
        // -------------------------------------------------

        if (schoolResources.houseWorker) {
            int w = 0;
            int h = 0;

            SDL_QueryTexture(
                schoolResources.houseWorker,
                nullptr,
                nullptr,
                &w,
                &h
            );

            SDL_Rect dst = {
                workerArea.x +
                    (workerArea.w - w) / 2,
                workerArea.y +
                    (workerArea.h - h) / 2,
                w,
                h
            };

            SDL_RenderCopy(
                renderer,
                schoolResources.houseWorker,
                nullptr,
                &dst
            );
        }

        // -------------------------------------------------
        // Health 55x15 podle TKMPercentBar
        // -------------------------------------------------

        SDL_SetRenderDrawColor(
            renderer,
            38, 33, 27, 255
        );

        SDL_RenderFillRect(
            renderer,
            &healthBar
        );

        SDL_SetRenderDrawColor(
            renderer,
            28, 24, 20, 255
        );

        SDL_RenderDrawLine(
            renderer,
            healthBar.x,
            healthBar.y,
            healthBar.x + healthBar.w - 1,
            healthBar.y
        );

        SDL_RenderDrawLine(
            renderer,
            healthBar.x,
            healthBar.y,
            healthBar.x,
            healthBar.y + healthBar.h - 1
        );

        SDL_SetRenderDrawColor(
            renderer,
            176, 163, 134, 255
        );

        SDL_RenderDrawLine(
            renderer,
            healthBar.x,
            healthBar.y + healthBar.h - 1,
            healthBar.x + healthBar.w - 1,
            healthBar.y + healthBar.h - 1
        );

        SDL_RenderDrawLine(
            renderer,
            healthBar.x + healthBar.w - 1,
            healthBar.y,
            healthBar.x + healthBar.w - 1,
            healthBar.y + healthBar.h - 1
        );

        int houseHealthWidth = 0;

        if (
            selectedSchool &&
            selectedSchool->maxHealth > 0
        ) {
            houseHealthWidth =
                (
                    (healthBar.w - 2) *
                    selectedSchool->health
                ) /
                selectedSchool->maxHealth;

            if (houseHealthWidth < 0)
                houseHealthWidth = 0;

            if (
                houseHealthWidth >
                    healthBar.w - 2
            ) {
                houseHealthWidth =
                    healthBar.w - 2;
            }
        }

        if (houseHealthWidth > 0) {
            SDL_Rect healthFill = {
                healthBar.x + 1,
                healthBar.y + 1,
                houseHealthWidth,
                healthBar.h - 2
            };

            SDL_SetRenderDrawColor(
                renderer,
                0, 170, 38, 255
            );

            SDL_RenderFillRect(
                renderer,
                &healthFill
            );

            SDL_SetRenderDrawColor(
                renderer,
                0, 69, 16, 255
            );

            SDL_RenderDrawLine(
                renderer,
                healthFill.x,
                healthFill.y,
                healthFill.x +
                    healthFill.w - 1,
                healthFill.y
            );

            SDL_RenderDrawLine(
                renderer,
                healthFill.x,
                healthFill.y,
                healthFill.x,
                healthFill.y +
                    healthFill.h - 1
            );
        }

        // Originalni fnt_Grey text "needs".
        if (schoolResources.needsText) {
            int needsWidth = 0;
            int needsHeight = 0;

            SDL_QueryTexture(
                schoolResources.needsText,
                nullptr,
                nullptr,
                &needsWidth,
                &needsHeight
            );

            SDL_Rect needsDestination = {
                panelX +
                    (contentWidth - needsWidth) / 2,
                panelY + 2,
                needsWidth,
                needsHeight
            };

            SDL_RenderCopy(
                renderer,
                schoolResources.needsText,
                nullptr,
                &needsDestination
            );
        }

        // Aktivni Gold radek podle originalniho TKMWaresRow:
        // vyska 21 px, ikony 14x14 zprava s krokem 14 px.
        SDL_Rect goldRow = {
            panelX,
            panelY + 21,
            contentWidth,
            21
        };

        // Jemny podklad podle originalniho TKMWaresRow.
        // Je tlumeny, aby zustala zachovana textura dreva.
        // Polopruhledny podklad: puvodni drevo 404
        // zustava citelne pod TKMWaresRow.
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
            &goldRow
        );

        SDL_SetRenderDrawBlendMode(
            renderer,
            SDL_BLENDMODE_NONE
        );

        // Zapusteny bevel.
        SDL_SetRenderDrawColor(
            renderer,
            42, 35, 28, 255
        );

        SDL_RenderDrawLine(
            renderer,
            goldRow.x,
            goldRow.y,
            goldRow.x + goldRow.w - 1,
            goldRow.y
        );

        SDL_RenderDrawLine(
            renderer,
            goldRow.x,
            goldRow.y,
            goldRow.x,
            goldRow.y + goldRow.h - 1
        );

        SDL_SetRenderDrawColor(
            renderer,
            158, 142, 109, 220
        );

        SDL_RenderDrawLine(
            renderer,
            goldRow.x,
            goldRow.y + goldRow.h - 1,
            goldRow.x + goldRow.w - 1,
            goldRow.y + goldRow.h - 1
        );

        SDL_RenderDrawLine(
            renderer,
            goldRow.x + goldRow.w - 1,
            goldRow.y,
            goldRow.x + goldRow.w - 1,
            goldRow.y + goldRow.h - 1
        );

        // Originalni fnt_Game text "Gold" vlevo.
        if (schoolResources.goldText) {
            int goldTextWidth = 0;
            int goldTextHeight = 0;

            SDL_QueryTexture(
                schoolResources.goldText,
                nullptr,
                nullptr,
                &goldTextWidth,
                &goldTextHeight
            );

            SDL_Rect goldTextDestination = {
                goldRow.x + 4,
                goldRow.y +
                    (goldRow.h - goldTextHeight) / 2,
                goldTextWidth,
                goldTextHeight
            };

            SDL_RenderCopy(
                renderer,
                schoolResources.goldText,
                nullptr,
                &goldTextDestination
            );
        }

        int visibleGold = 0;

        if (selectedSchool) {
            visibleGold = selectedSchool->schoolGold;

            if (visibleGold < 0)
                visibleGold = 0;
        }

        // Do 180px radku se bezpecne vejde 11 ikon.
        // Vyssi zasoba zustava funkcni, jen se vizualne omezi.
        int drawnGold = visibleGold;

        if (drawnGold > 11)
            drawnGold = 11;

        for (
            int goldIndex = drawnGold - 1;
            goldIndex >= 0;
            --goldIndex
        ) {
            SDL_Rect goldDestination = {
                goldRow.x + goldRow.w -
                    18 - goldIndex * 14,
                goldRow.y + 3,
                14,
                14
            };

            if (schoolResources.goldIcon) {
                SDL_RenderCopy(
                    renderer,
                    schoolResources.goldIcon,
                    nullptr,
                    &goldDestination
                );
            } else {
                SDL_SetRenderDrawColor(
                    renderer,
                    223, 177, 48, 255
                );

                SDL_RenderFillRect(
                    renderer,
                    &goldDestination
                );
            }
        }


        // Queue[0]: jednotka ve vycviku.
        SDL_Rect wipSlot = {
            panelX,
            panelY + 48,
            32,
            32
        };

        // WIP slot ve stylu originalniho KaM.
        // GUI 402 poskytuje kamen, okraje vytvareji
        // zapusteny bevel.
        if (schoolResources.buttonStone) {
            SDL_Rect wipStoneSource = {
                8,
                18,
                wipSlot.w,
                wipSlot.h
            };

            // WIP stone:
            // prazdny slot s GUI 41 ma stejny svetly kamen
            // jako originalni School ovladaci tlacitka.
            // Obsazeny WIP zustava lehce utlumeny,
            // aby ikona trenovane jednotky zustala citelna.
            bool wipHasUnit =
                selectedSchool &&
                selectedSchool->schoolQueue[0] >= 0;

            if (wipHasUnit) {
                SDL_SetTextureColorMod(
                    schoolResources.buttonStone,
                    138,
                    133,
                    119
                );
            } else {
                SDL_SetTextureColorMod(
                    schoolResources.buttonStone,
                    255,
                    255,
                    255
                );
            }

            SDL_RenderCopy(
                renderer,
                schoolResources.buttonStone,
                &wipStoneSource,
                &wipSlot
            );

            SDL_SetTextureColorMod(
                schoolResources.buttonStone,
                255,
                255,
                255
            );
        } else {
            SDL_SetRenderDrawColor(
                renderer,
                49, 45, 37, 255
            );

            SDL_RenderFillRect(
                renderer,
                &wipSlot
            );
        }

        // Zasunutý bevel: tmavy vrch/levy okraj.
        SDL_SetRenderDrawColor(
            renderer,
            34, 29, 24, 255
        );

        SDL_RenderDrawLine(
            renderer,
            wipSlot.x,
            wipSlot.y,
            wipSlot.x + wipSlot.w - 1,
            wipSlot.y
        );

        SDL_RenderDrawLine(
            renderer,
            wipSlot.x,
            wipSlot.y,
            wipSlot.x,
            wipSlot.y + wipSlot.h - 1
        );

        // Svetly spodni/pravy okraj.
        SDL_SetRenderDrawColor(
            renderer,
            181, 169, 139, 255
        );

        SDL_RenderDrawLine(
            renderer,
            wipSlot.x,
            wipSlot.y + wipSlot.h - 1,
            wipSlot.x + wipSlot.w - 1,
            wipSlot.y + wipSlot.h - 1
        );

        SDL_RenderDrawLine(
            renderer,
            wipSlot.x + wipSlot.w - 1,
            wipSlot.y,
            wipSlot.x + wipSlot.w - 1,
            wipSlot.y + wipSlot.h - 1
        );

        if (selectedSchool) {
            int profession =
                selectedSchool->schoolQueue[0];

            if (
                profession >= 0 &&
                profession < SCHOOL_PROFESSION_COUNT
            ) {
                SDL_Texture* icon =
                    schoolResources
                        .professions[profession]
                        .icon;

                if (icon) {
                    SDL_Rect iconDestination = {
                        wipSlot.x + 1,
                        wipSlot.y + 1,
                        wipSlot.w - 2,
                        wipSlot.h - 2
                    };

                    SDL_RenderCopy(
                        renderer,
                        icon,
                        nullptr,
                        &iconDestination
                    );
                }
            }
        }

        // Original KaM: prazdny WIP slot zobrazuje
        // GUI sprite 41 - question mark.
        bool wipOccupied =
            selectedSchool &&
            selectedSchool->schoolQueue[0] >= 0;

        if (
            !wipOccupied &&
            schoolResources.questionIcon
        ) {
            int questionWidth = 0;
            int questionHeight = 0;

            SDL_QueryTexture(
                schoolResources.questionIcon,
                nullptr,
                nullptr,
                &questionWidth,
                &questionHeight
            );

            SDL_Rect questionDestination = {
                wipSlot.x +
                    (wipSlot.w - questionWidth) / 2,
                wipSlot.y +
                    (wipSlot.h - questionHeight) / 2,
                questionWidth,
                questionHeight
            };

            SDL_RenderCopy(
                renderer,
                schoolResources.questionIcon,
                nullptr,
                &questionDestination
            );
        }

        // Viditelne CANCEL tlacitko pro obsazeny WIP slot.
        // Cervene X v pravem hornim rohu naznacuje,
        // ze klepnuti na tento slot zrusi vycvik.
        if (
            selectedSchool &&
            selectedSchool->schoolQueue[0] >= 0
        ) {
            SDL_Rect cancelBadge = {
                wipSlot.x + wipSlot.w - 11,
                wipSlot.y + 1,
                10,
                10
            };

            SDL_SetRenderDrawColor(
                renderer,
                104, 24, 20, 255
            );

            SDL_RenderFillRect(
                renderer,
                &cancelBadge
            );

            SDL_SetRenderDrawColor(
                renderer,
                255, 214, 178, 255
            );

            SDL_RenderDrawLine(
                renderer,
                cancelBadge.x + 2,
                cancelBadge.y + 2,
                cancelBadge.x + cancelBadge.w - 3,
                cancelBadge.y + cancelBadge.h - 3
            );

            SDL_RenderDrawLine(
                renderer,
                cancelBadge.x + cancelBadge.w - 3,
                cancelBadge.y + 2,
                cancelBadge.x + 2,
                cancelBadge.y + cancelBadge.h - 3
            );
        }

        // Gamepadovy focus queue[0]: WIP.
        if (
            ui.schoolQueueFocus &&
            ui.schoolQueueSelection == 0
        ) {
            SDL_Rect queueFocusOuter = {
                wipSlot.x - 2,
                wipSlot.y - 2,
                wipSlot.w + 4,
                wipSlot.h + 4
            };

            SDL_Rect queueFocusInner = {
                wipSlot.x - 1,
           wipSlot.y - 1,
                wipSlot.w + 2,
                wipSlot.h + 2
            };

            SDL_SetRenderDrawColor(
                renderer,
                255, 221, 116, 255
            );

            SDL_RenderDrawRect(
                renderer,
                &queueFocusOuter
            );

            SDL_RenderDrawRect(
                renderer,
                &queueFocusInner
            );
        }

        // Progress vycviku podle KaM Remake
        // TKMRenderUI.WritePercentBar.
        SDL_Rect progressOuter = {
            panelX + 34,
            panelY + 54,
            146,
            20
        };

        // Základ progress baru.
        SDL_SetRenderDrawColor(
            renderer,
            48, 43, 35, 255
        );

        SDL_RenderFillRect(
            renderer,
            &progressOuter
        );

        // Zasunutý bevel.
        SDL_SetRenderDrawColor(
            renderer,
            30, 26, 22, 255
        );

        SDL_RenderDrawLine(
            renderer,
            progressOuter.x,
            progressOuter.y,
            progressOuter.x +
                progressOuter.w - 1,
            progressOuter.y
        );

        SDL_RenderDrawLine(
            renderer,
            progressOuter.x,
            progressOuter.y,
            progressOuter.x,
            progressOuter.y +
                progressOuter.h - 1
        );

        SDL_SetRenderDrawColor(
            renderer,
            178, 166, 137, 255
        );

        SDL_RenderDrawLine(
            renderer,
            progressOuter.x,
            progressOuter.y +
                progressOuter.h - 1,
            progressOuter.x +
                progressOuter.w - 1,
            progressOuter.y +
                progressOuter.h - 1
        );

        SDL_RenderDrawLine(
            renderer,
            progressOuter.x +
                progressOuter.w - 1,
            progressOuter.y,
            progressOuter.x +
                progressOuter.w - 1,
            progressOuter.y +
                progressOuter.h - 1
        );

        int trainingStep = 0;

        if (selectedSchool) {
            trainingStep =
                selectedSchool->schoolTrainingStep;

            if (trainingStep < 0)
                trainingStep = 0;

            if (
                trainingStep >
                SCHOOL_TRAINING_STEPS
            ) {
                trainingStep =
                    SCHOOL_TRAINING_STEPS;
            }
        }

        // Remake:
        // Round((width - 2) * progress) + 2.
        int progressWidth =
            (
                (progressOuter.w - 2) *
                trainingStep
            ) /
            SCHOOL_TRAINING_STEPS + 2;

        if (trainingStep <= 0)
            progressWidth = 0;

        if (
            progressWidth >
            progressOuter.w - 2
        ) {
            progressWidth =
                progressOuter.w - 2;
        }

        SDL_Rect progressFill = {
            progressOuter.x + 1,
            progressOuter.y + 1,
            progressWidth,
            progressOuter.h - 2
        };

        // BAR_COLOR_GREEN z KaM Remake:
        // RGB 0, 170, 38.
        if (progressFill.w > 0) {
            SDL_SetRenderDrawColor(
                renderer,
                0, 170, 38, 255
            );

            SDL_RenderFillRect(
                renderer,
                &progressFill
            );

            // Vnitřní stín nahoře a vlevo.
            SDL_SetRenderDrawColor(
                renderer,
                0, 69, 16, 255
            );

            SDL_RenderDrawLine(
                renderer,
                progressFill.x,
                progressFill.y,
                progressFill.x +
                    progressFill.w - 1,
                progressFill.y
            );

            SDL_RenderDrawLine(
                renderer,
                progressFill.x,
                progressFill.y,
                progressFill.x,
                progressFill.y +
                    progressFill.h - 1
            );

            // Jemné světlo na spodní hraně výplně.
            SDL_SetRenderDrawColor(
                renderer,
                38, 205, 68, 255
            );

            SDL_RenderDrawLine(
                renderer,
                progressFill.x,
                progressFill.y +
                    progressFill.h - 1,
                progressFill.x +
                    progressFill.w - 1,
                progressFill.y +
                    progressFill.h - 1
            );
        }

        // Queue[1..5].
        for (int i = 0; i < SCHOOL_WAITING_SLOTS; ++i) {
            SDL_Rect queueSlot = {
                panelX + i * 36,
                panelY + 80,
                32,
                32
            };

            // Jemny podklad queue podle stejneho vizualniho
            // principu jako Gold TKMWaresRow.
            // Dreveny sidebar 404 zustava citelny.
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
                &queueSlot
            );

            SDL_SetRenderDrawBlendMode(
                renderer,
                SDL_BLENDMODE_NONE
            );

            // Zasunutý tmavy vrch a levy okraj.
            SDL_SetRenderDrawColor(
                renderer,
                34, 29, 24, 255
            );

            SDL_RenderDrawLine(
                renderer,
                queueSlot.x,
                queueSlot.y,
                queueSlot.x +
                    queueSlot.w - 1,
                queueSlot.y
            );

            SDL_RenderDrawLine(
                renderer,
                queueSlot.x,
                queueSlot.y,
                queueSlot.x,
                queueSlot.y +
                    queueSlot.h - 1
            );

            // Svetly spodni a pravy okraj.
            SDL_SetRenderDrawColor(
                renderer,
                168, 157, 130, 255
            );

            SDL_RenderDrawLine(
                renderer,
                queueSlot.x,
                queueSlot.y +
                    queueSlot.h - 1,
                queueSlot.x +
                    queueSlot.w - 1,
                queueSlot.y +
                    queueSlot.h - 1
            );

            SDL_RenderDrawLine(
                renderer,
                queueSlot.x +
                    queueSlot.w - 1,
                queueSlot.y,
                queueSlot.x +
                    queueSlot.w - 1,
                queueSlot.y +
                    queueSlot.h - 1
            );

            if (selectedSchool) {
                int profession =
                    selectedSchool
                        ->schoolQueue[i + 1];

                if (
                    profession >= 0 &&
                    profession <
                        SCHOOL_PROFESSION_COUNT
                ) {
                    SDL_Texture* icon =
                        schoolResources
                            .professions[profession]
                            .icon;

                    if (icon) {
                        SDL_Rect iconDestination = {
                            queueSlot.x + 1,
                            queueSlot.y + 1,
                            queueSlot.w - 2,
                            queueSlot.h - 2
                        };

                        SDL_RenderCopy(
                            renderer,
                            icon,
                            nullptr,
                            &iconDestination
                        );
                    }
                }
            }

            // Viditelne CANCEL X pro obsazeny
            // cekajici slot queue[1..5].
            if (
                selectedSchool &&
                selectedSchool->schoolQueue[i + 1] >= 0
            ) {
                SDL_Rect cancelBadge = {
                    queueSlot.x + queueSlot.w - 11,
                    queueSlot.y + 1,
                    10,
                    10
                };

                SDL_SetRenderDrawColor(
                    renderer,
                    104, 24, 20, 255
                );

                SDL_RenderFillRect(
                    renderer,
                    &cancelBadge
                );

                SDL_SetRenderDrawColor(
                    renderer,
                    255, 214, 178, 255
                );

                SDL_RenderDrawLine(
                    renderer,
                    cancelBadge.x + 2,
                    cancelBadge.y + 2,
                    cancelBadge.x +
                        cancelBadge.w - 3,
                    cancelBadge.y +
                        cancelBadge.h - 3
                );

                SDL_RenderDrawLine(
                    renderer,
                    cancelBadge.x +
                        cancelBadge.w - 3,
                    cancelBadge.y + 2,
                    cancelBadge.x + 2,
                    cancelBadge.y +
                        cancelBadge.h - 3
                );
            }
        }

        // Gamepadovy focus queue[1..5].
        if (
            ui.schoolQueueFocus &&
            ui.schoolQueueSelection >= 1 &&
            ui.schoolQueueSelection <
                SCHOOL_QUEUE_SIZE
        ) {
            int waitingIndex =
                ui.schoolQueueSelection - 1;

            SDL_Rect focusedWaitingSlot = {
                panelX + waitingIndex * 36,
                panelY + 80,
                32,
                32
            };

            SDL_Rect queueFocusOuter = {
                focusedWaitingSlot.x - 2,
                focusedWaitingSlot.y - 2,
                focusedWaitingSlot.w + 4,
                focusedWaitingSlot.h + 4
            };

            SDL_Rect queueFocusInner = {
                focusedWaitingSlot.x - 1,
                focusedWaitingSlot.y - 1,
                focusedWaitingSlot.w + 2,
                focusedWaitingSlot.h + 2
            };

            SDL_SetRenderDrawColor(
                renderer,
                255, 221, 116, 255
            );

            SDL_RenderDrawRect(
                renderer,
                &queueFocusOuter
            );

            SDL_RenderDrawRect(
                renderer,
                &queueFocusInner
            );
        }

        int selected =
            static_cast<int>(
                ui.schoolSelection
            );

        if (selected < 0)
            selected = 0;

        if (selected >= SCHOOL_PROFESSION_COUNT) {
            selected =
                SCHOOL_PROFESSION_COUNT - 1;
        }

        // Originalni fnt_Outline nazev vybrane profese.
        // Remake pouziva Y=116 a stred TB_WIDTH.
        if (
            selected >= 0 &&
            selected < SCHOOL_PROFESSION_COUNT &&
            schoolResources.professionText[selected]
        ) {
            SDL_Texture* professionLabel =
                schoolResources.professionText[selected];

            int labelWidth = 0;
            int labelHeight = 0;

            SDL_QueryTexture(
                professionLabel,
                nullptr,
                nullptr,
                &labelWidth,
                &labelHeight
            );

            SDL_Rect labelDestination = {
                panelX +
                    (contentWidth - labelWidth) / 2,
                panelY + 116 +
                    (20 - labelHeight) / 2,
                labelWidth,
                labelHeight
            };

            SDL_RenderCopy(
                renderer,
                professionLabel,
                nullptr,
                &labelDestination
            );
        }

        int previous = selected - 1;
        int next = selected + 1;

        struct SchoolPortrait {
            int profession;
            SDL_Rect area;
            bool enabled;
        };

        SchoolPortrait portraits[3] = {
            {
                previous,
                {
                    panelX,
                    panelY + 136,
                    54,
                    80
                },
                previous >= 0
            },
            {
                selected,
                {
                    panelX + 62,
                    panelY + 136,
                    54,
                    80
                },
                true
            },
            {
                next,
                {
                    panelX + 124,
                    panelY + 136,
                    54,
                    80
                },
                next < SCHOOL_PROFESSION_COUNT
            }
        };

        // Ramecky tri carouselovych pozic.
        for (int i = 0; i < 3; ++i) {
            bool portraitFocused =
                i == 1 &&
                !ui.schoolQueueFocus;

            // Original KaM selection style:
            // zvolena profese zustava plne jasna,
            // sousedni profese jsou pouze ztlumene.
            // Zadny dodatecny focus ramecek.

            if (!portraits[i].enabled)
                continue;

            SDL_Texture* texture =
                schoolResources
                    .professions[
                        portraits[i].profession
                    ]
                    .scroll;

            if (!texture)
                continue;

            int sourceWidth = 0;
            int sourceHeight = 0;

            SDL_QueryTexture(
                texture,
                nullptr,
                nullptr,
                &sourceWidth,
                &sourceHeight
            );

            if (
                sourceWidth <= 0 ||
                sourceHeight <= 0
            ) {
                continue;
            }

            int availableWidth =
                portraits[i].area.w - 4;

            int availableHeight =
                portraits[i].area.h - 4;

            float scaleX =
                (float)availableWidth /
                (float)sourceWidth;

            float scaleY =
                (float)availableHeight /
                (float)sourceHeight;

            float scale =
                scaleX < scaleY
                    ? scaleX
                    : scaleY;

            // Originalni grafiku nezvetsujeme.
            if (scale > 1.0f)
                scale = 1.0f;

            int drawWidth =
                (int)(
                    sourceWidth * scale
                );

            int drawHeight =
                (int)(
                    sourceHeight * scale
                );

            SDL_Rect destination = {
                portraits[i].area.x +
                    (
                        portraits[i].area.w -
                        drawWidth
                    ) / 2,

                portraits[i].area.y +
                    (
                        portraits[i].area.h -
                        drawHeight
               ) / 2,

                drawWidth,
                drawHeight
            };

            // Originalni vyber je zalozen na jasu:
            // prostredni aktivni profese 100 %,
            // predchozi/dalsi profese cca 45 %.
            const Uint8 portraitBrightness =
                portraitFocused
                    ? 255
                    : 115;

            SDL_SetTextureColorMod(
                texture,
                portraitBrightness,
                portraitBrightness,
                portraitBrightness
            );

            SDL_RenderCopy(
                renderer,
                texture,
                nullptr,
                &destination
            );

            // Texture se pouziva znovu i na dalsich mistech,
            // proto vzdy vratime plny jas.
            SDL_SetTextureColorMod(
                texture,
                255,
                255,
                255
            );
        }

        SDL_Rect leftButton = {
            panelX,
            panelY + 222,
            54,
            40
        };

        SDL_Rect trainButton = {
            panelX + 62,
            panelY + 222,
            54,
            40
        };

        SDL_Rect rightButton = {
            panelX + 124,
            panelY + 222,
            54,
            40
        };

        SDL_Rect buttons[3] = {
            leftButton,
            trainButton,
            rightButton
        };

        bool selectedQueueSlotOccupied = false;

        if (
            ui.schoolQueueFocus &&
            selectedSchool &&
            ui.schoolQueueSelection >= 0 &&
            ui.schoolQueueSelection <
                SCHOOL_QUEUE_SIZE
        ) {
            selectedQueueSlotOccupied =
                selectedSchool->schoolQueue[
                    ui.schoolQueueSelection
                ] >= 0;
        }

        for (int i = 0; i < 3; ++i) {
            bool buttonEnabled = true;

            if (ui.schoolQueueFocus) {
                // Ve queue rezimu je aktivni pouze
                // prostredni CANCEL tlacitko.
                if (i != 1)
                    buttonEnabled = false;

                if (
                    i == 1 &&
                    !selectedQueueSlotOccupied
                ) {
                    buttonEnabled = false;
                }
            } else {
                if (i == 0 && selected == 0)
                    buttonEnabled = false;

                if (
                    i == 2 &&
                    selected ==
                        SCHOOL_PROFESSION_COUNT - 1
                ) {
                    buttonEnabled = false;
                }
            }

            if (!buttonEnabled) {
                SDL_SetRenderDrawColor(
                    renderer,
                    52, 46, 38, 255
                );
            } else if (
                i == 1 &&
                ui.schoolQueueFocus
            ) {
                // Aktivni CANCEL.
                SDL_SetRenderDrawColor(
                    renderer,
                    112, 36, 29, 255
                );
            } else if (i == 1) {
                // Aktivni TRAIN.
                SDL_SetRenderDrawColor(
                    renderer,
                    126, 92, 42, 255
                );
            } else {
                SDL_SetRenderDrawColor(
                    renderer,
                    91, 69, 40, 255
                );
            }

            SDL_RenderFillRect(
                renderer,
                &buttons[i]
            );

            if (
                buttonEnabled &&
                i == 1 &&
                ui.schoolQueueFocus
            ) {
                SDL_SetRenderDrawColor(
                    renderer,
                    244, 138, 108, 255
                );
            } else {
                SDL_SetRenderDrawColor(
                    renderer,
                    buttonEnabled ? 220 : 98,
                    buttonEnabled ? 185 : 83,
                    buttonEnabled ? 105 : 58,
                    255
                );
            }

            SDL_RenderDrawRect(
                renderer,
                &buttons[i]
            );
        }

        // Sipka LEFT.
        if (
            !ui.schoolQueueFocus &&
            selected > 0
        ) {
            SDL_SetRenderDrawColor(
                renderer,
                232, 205, 142, 255
            );

            SDL_RenderDrawLine(
                renderer,
                leftButton.x + 34,
                leftButton.y + 10,
                leftButton.x + 18,
                leftButton.y + 20
            );

            SDL_RenderDrawLine(
                renderer,
                leftButton.x + 18,
                leftButton.y + 20,
                leftButton.x + 34,
                leftButton.y + 30
            );
        }

        if (ui.schoolQueueFocus) {
            // CANCEL symbol ve queue rezimu.
            SDL_SetRenderDrawColor(
                renderer,
                selectedQueueSlotOccupied
                    ? 255
                    : 112,
                selectedQueueSlotOccupied
                    ? 210
                    : 94,
                selectedQueueSlotOccupied
                    ? 177
                    : 76,
                255
            );

            SDL_Rect cancelSymbol = {
                trainButton.x + 16,
                trainButton.y + 10,
                22,
                20
            };

            SDL_RenderDrawRect(
                renderer,
                &cancelSymbol
            );

            SDL_RenderDrawLine(
                renderer,
                cancelSymbol.x + 5,
                cancelSymbol.y + 4,
                cancelSymbol.x +
                    cancelSymbol.w - 6,
                cancelSymbol.y +
                    cancelSymbol.h - 5
            );

            SDL_RenderDrawLine(
                renderer,
                cancelSymbol.x +
                    cancelSymbol.w - 6,
                cancelSymbol.y + 4,
                cancelSymbol.x + 5,
                cancelSymbol.y +
                    cancelSymbol.h - 5
            );
        } else {
            // TRAIN symbol ve vyberu profese.
            SDL_SetRenderDrawColor(
                renderer,
                242, 193, 54, 255
            );

            SDL_Rect trainSymbol = {
                trainButton.x + 17,
                trainButton.y + 11,
                20,
                18
            };

            SDL_RenderDrawRect(
                renderer,
                &trainSymbol
            );

            SDL_RenderDrawLine(
                renderer,
                trainSymbol.x + 4,
                trainSymbol.y + 9,
                trainSymbol.x + 16,
                trainSymbol.y + 9
            );
        }

        // Sipka RIGHT.
        if (
            !ui.schoolQueueFocus &&
            selected <
                SCHOOL_PROFESSION_COUNT - 1
        ) {
            SDL_SetRenderDrawColor(
                renderer,
                232, 205, 142, 255
            );

            SDL_RenderDrawLine(
                renderer,
                rightButton.x + 20,
                rightButton.y + 10,
                rightButton.x + 36,
                rightButton.y + 20
            );

            SDL_RenderDrawLine(
                renderer,
                rightButton.x + 36,
                rightButton.y + 20,
                rightButton.x + 20,
                rightButton.y + 30
            );
        }

        // ORIGINAL KAM SCHOOL GLYPHS - FINAL OVERLAY
        // GUI 35 = previous, GUI 42 = train, GUI 36 = next.
        // Kresli se jako posledni vrstva School panelu.
        if (!ui.schoolQueueFocus) {
            SDL_Texture* glyphs[3] = {
                schoolResources.schoolButtons[0],
                schoolResources.schoolButtons[1],
                schoolResources.schoolButtons[2]
            };

            SDL_Rect areas[3] = {
                leftButton,
                trainButton,
                rightButton
            };

            for (int i = 0; i < 3; ++i) {
                // Nejprve prekryj stary rucne kresleny symbol.
                if (schoolResources.buttonStone) {
                    SDL_Rect src = {
                        12 + i * 58,
                        40 + i * 70,
                        areas[i].w,
                        areas[i].h
                    };

                    SDL_RenderCopy(
                        renderer,
                        schoolResources.buttonStone,
                        &src,
                        &areas[i]
                    );
                }

                if (!glyphs[i])
                    continue;

                int gw = 0;
                int gh = 0;

                SDL_QueryTexture(
                    glyphs[i],
                    nullptr,
                    nullptr,
                    &gw,
                    &gh
                );

                SDL_Rect dst = {
                    areas[i].x + (areas[i].w - gw) / 2,
                    areas[i].y + (areas[i].h - gh) / 2,
                    gw,
                    gh
                };

                SDL_RenderCopy(
                    renderer,
                    glyphs[i],
                    nullptr,
                    &dst
                );
            }
        }
    }

    // =====================================================
    // KOMPLETNI ORIGINALNI KaM BUILD 439
    //
    // Akcni radek:
    // Road je v KaMVita funkcni.
    //
    // House grid:
    // puvodni GUIHouseOrder[27],
    // 5 sloupcu, ButtonFlat 33x33,
    // krok 37 px.
    //
    // Implementovane:
    // School, Farm, Bakery, Store.
    //
    // Ostatni budovy pouzivaji GUI 41,
    // stejne jako nedostupne houses v Remaku.
    // =====================================================

    if (ui.section == GameUISection::Build) {
        const int buttonSize = 33;
        const int buttonStep = 37;

        // Sidebar toolbar konci na Y=230.
        // Rozlozeni je prizpusobeno vysce Vita displeje,
        // ale zachovava originalni 33/37 geometrii.
        const int actionY = 282;
        const int housesStartY = 321;
        const int contentX = 6;

        // Road se vykresluje pouze v jednotnem
        // originalnim Build tool row nize.

        // -------------------------------------------------
        // Puvodni GUIHouseOrder[27]
        // -------------------------------------------------

        // ===============================================
        // ORIGINALNI KaM BUILD SELECTED DETAIL
        //
        // Remake:
        // Label_Build
        // Image_Build_Selected
        // GUI 353 Wood
        // GUI 352 Stone
        //
        // Hodnoty houses se ctou primo ze skutecne
        // stavebni logiky KaMVita.
        // ===============================================

        SDL_Texture* selectedBuildIcon = nullptr;
        SDL_Texture* selectedBuildName = nullptr;

        int selectedWoodCost = 0;
        int selectedStoneCost = 0;

        bool selectedHasWood = true;
        bool selectedHasStone = true;

        bool selectedIsBuilding = false;
        BuildingType selectedBuildingType =
            BuildingType::Store;

        // Field/Wine jsou samostatne Build tools.
        //
        // Originalni KM_GUIGameBuild.pas:
        // Field: Wood 0, Stone 0
        // Wine:  Wood 1, Stone 0
        //
        // Pro Road a houses zustava nize puvodni
        // GameUIBuildSelection logika.
        const bool fieldToolSelected =
            ui.buildToolSelection == 1;

        const bool wineToolSelected =
            ui.buildToolSelection == 2;

        const bool demolishToolSelected =
            ui.buildToolSelection == 3;

        const bool houseCatalogSelected =
            ui.buildHouseCatalogSelection >= 0 &&
            ui.buildHouseCatalogSelection <
                GUI_HOUSE_COUNT;

        if (fieldToolSelected) {
            selectedBuildIcon =
                schoolResources.buildFieldIcon;

            selectedBuildName =
                schoolResources.buildNameText[5];

            selectedWoodCost = 0;
            selectedStoneCost = 0;
            selectedHasWood = false;
            selectedHasStone = false;
        } else if (wineToolSelected) {
            selectedBuildIcon =
                schoolResources.buildWineIcon;

            selectedBuildName =
                schoolResources.buildNameText[6];

            selectedWoodCost = 1;
            selectedStoneCost = 0;
            selectedHasWood = true;
            selectedHasStone = false;
        } else if (demolishToolSelected) {
            // KaM Remake:
            // SetCost(
            //     cmErase,
            //     0,
            //     340,
            //     0,
            //     0,
            //     TX_BUILD_DEMOLISH
            // );

            selectedBuildIcon =
                schoolResources.buildRemoveIcon;

            selectedBuildName =
                schoolResources.buildDemolishText;

            selectedWoodCost = 0;
            selectedStoneCost = 0;

            selectedHasWood = false;
            selectedHasStone = false;
        } else if (houseCatalogSelected) {
            const HouseIconType houseType =
                GUI_HOUSE_ORDER[
                    ui.buildHouseCatalogSelection
                ];

            const int houseTypeIndex =
                static_cast<int>(
                    houseType
                );

            const HouseBuildInfo& buildInfo =
                getHouseBuildInfo(
                    houseType
                );

            selectedBuildIcon =
                getHouseIcon(
                    houseIcons,
                    houseType
                );

            if (
                houseTypeIndex >= 0 &&
                houseTypeIndex <
                    HOUSE_ICON_COUNT
            ) {
                selectedBuildName =
                    schoolResources
                        .buildHouseNameText[
                            houseTypeIndex
                        ];
            }

            selectedWoodCost =
                buildInfo.woodCost;

            selectedStoneCost =
                buildInfo.stoneCost;

            selectedHasWood =
                selectedWoodCost > 0;

            selectedHasStone =
                selectedStoneCost > 0;
        } else {
        switch (ui.buildSelection) {
            case GameUIBuildSelection::Road:
                selectedBuildIcon =
                    schoolResources.buildRoadIcon;

                selectedBuildName =
                    schoolResources.buildNameText[0];

                selectedHasWood = false;
                selectedWoodCost = 0;

                selectedHasStone = true;
                selectedStoneCost = 1;
                break;

            case GameUIBuildSelection::Store:
                selectedBuildIcon =
                    getHouseIcon(
                        houseIcons,
                        HouseIconType::Store
                    );

                selectedBuildName =
                    schoolResources.buildNameText[1];

                selectedBuildingType =
                    BuildingType::Store;

                selectedIsBuilding = true;
                break;

            case GameUIBuildSelection::Bakery:
                selectedBuildIcon =
                    getHouseIcon(
                        houseIcons,
                        HouseIconType::Bakery
                    );

                selectedBuildName =
                    schoolResources.buildNameText[2];

                selectedBuildingType =
                    BuildingType::Bakery;

                selectedIsBuilding = true;
                break;

            case GameUIBuildSelection::Farm:
                selectedBuildIcon =
                    getHouseIcon(
                        houseIcons,
                        HouseIconType::Farm
                    );

                selectedBuildName =
                    schoolResources.buildNameText[3];

                selectedBuildingType =
                    BuildingType::Farm;

                selectedIsBuilding = true;
                break;

            case GameUIBuildSelection::School:
                selectedBuildIcon =
                    getHouseIcon(
                        houseIcons,
                        HouseIconType::School
                    );

                selectedBuildName =
                    schoolResources.buildNameText[4];

                selectedBuildingType =
                    BuildingType::School;

                selectedIsBuilding = true;
                break;
        }
        }

        if (selectedIsBuilding) {
            selectedWoodCost =
                getBuildingWoodCost(
                    selectedBuildingType
                );

            selectedStoneCost =
                getBuildingStoneCost(
                    selectedBuildingType
                );

            selectedHasWood =
                selectedWoodCost > 0;

            selectedHasStone =
                selectedStoneCost > 0;
        }

        // -------------------------------------------------
        // Caption
        // -------------------------------------------------

        if (selectedBuildName) {
            int width = 0;
            int height = 0;

            SDL_QueryTexture(
                selectedBuildName,
                nullptr,
                nullptr,
                &width,
                &height
            );

            SDL_Rect destination = {
                6 + (180 - width) / 2,
                233,
                width,
                height
            };

            SDL_RenderCopy(
                renderer,
                selectedBuildName,
                nullptr,
                &destination
            );
        }

        // -------------------------------------------------
        // Selected icon
        // -------------------------------------------------

        if (selectedBuildIcon) {
            int width = 0;
            int height = 0;

            SDL_QueryTexture(
                selectedBuildIcon,
                nullptr,
                nullptr,
                &width,
                &height
            );

            SDL_Rect destination = {
                6 + (32 - width) / 2,
                247 + (32 - height) / 2,
                width,
                height
            };

            SDL_RenderCopy(
                renderer,
                selectedBuildIcon,
                nullptr,
                &destination
            );
        }

        // -------------------------------------------------
        // Wood / Stone icons
        // -------------------------------------------------

        if (schoolResources.buildWoodIcon) {
            int width = 0;
            int height = 0;

            SDL_QueryTexture(
                schoolResources.buildWoodIcon,
                nullptr,
                nullptr,
                &width,
                &height
            );

            SDL_Rect destination = {
                73 + (32 - width) / 2,
                247 + (32 - height) / 2,
                width,
                height
            };

            SDL_RenderCopy(
                renderer,
                schoolResources.buildWoodIcon,
                nullptr,
                &destination
            );
        }

        if (schoolResources.buildStoneIcon) {
            int width = 0;
            int height = 0;

            SDL_QueryTexture(
                schoolResources.buildStoneIcon,
                nullptr,
                nullptr,
                &width,
                &height
            );

            SDL_Rect destination = {
                128 + (32 - width) / 2,
                247 + (32 - height) / 2,
                width,
                height
            };

            SDL_RenderCopy(
                renderer,
                schoolResources.buildStoneIcon,
                nullptr,
                &destination
            );
        }

        // -------------------------------------------------
        // Cost values
        //
        // statisticsCounts:
        // 33 cells, each 32x18, values 0..32.
        // -------------------------------------------------

        if (schoolResources.statisticsCounts) {
            if (selectedHasWood) {
                SDL_Rect source = {
                    0,
                    selectedWoodCost * 18,
                    32,
                    18
                };

                SDL_Rect destination = {
                    103,
                    254,
                    32,
                    18
                };

                SDL_RenderCopy(
                    renderer,
                    schoolResources.statisticsCounts,
                    &source,
                    &destination
                );
            }

            if (selectedHasStone) {
                SDL_Rect source = {
                    0,
                    selectedStoneCost * 18,
                    32,
                    18
                };

                SDL_Rect destination = {
                    158,
                    254,
                    32,
                    18
                };

                SDL_RenderCopy(
                    renderer,
                    schoolResources.statisticsCounts,
                    &source,
                    &destination
                );
            }
        }

        // ===============================================
        // ORIGINALNI KaM BUILD TOOL ROW
        //
        // KM_GUIGameBuild.pas:
        // Road   335  X=0
        // Field  337  X=37
        // Wine   336  X=74
        // Remove 340  X=148
        //
        // Vita content origin X=6.
        // Field/Wine/Remove jsou zatim pouze autenticky
        // zobrazene. Herni logiku zatim nema KaMVita.
        // ===============================================

        SDL_Texture* buildToolIcons[4] = {
            schoolResources.buildRoadIcon,
            schoolResources.buildFieldIcon,
            schoolResources.buildWineIcon,
            schoolResources.buildRemoveIcon
        };

        const int buildToolX[4] = {
            6,
            43,
            80,
            154
        };

        const int buildToolY = 282;
        const int buildToolSize = 33;

        for (
            int toolIndex = 0;
            toolIndex < 4;
            ++toolIndex
        ) {
            SDL_Rect button = {
                buildToolX[toolIndex],
                buildToolY,
                buildToolSize,
                buildToolSize
            };

            // Vsechny ctyri originalni Build tools
            // jsou aktivni.
            const bool implemented = true;

            const bool selected =
                ui.buildToolSelection ==
                    toolIndex;

            SDL_SetRenderDrawBlendMode(
                renderer,
                SDL_BLENDMODE_BLEND
            );

            SDL_SetRenderDrawColor(
                renderer,
                selected ? 103 : 49,
                selected ? 84 : 42,
                selected ? 48 : 32,
                selected ? 205 : 125
            );

            SDL_RenderFillRect(
                renderer,
                &button
            );

            SDL_SetRenderDrawBlendMode(
                renderer,
                SDL_BLENDMODE_NONE
            );

            // Horni a levy okraj ButtonFlat.
            SDL_SetRenderDrawColor(
                renderer,
                35,
                30,
                25,
                255
            );

            SDL_RenderDrawLine(
                renderer,
                button.x,
                button.y,
                button.x + button.w - 1,
                button.y
            );

            SDL_RenderDrawLine(
                renderer,
                button.x,
                button.y,
                button.x,
                button.y + button.h - 1
            );

            // Spodni a pravy okraj.
            SDL_SetRenderDrawColor(
                renderer,
                selected ? 230 : 171,
                selected ? 194 : 154,
                selected ? 100 : 121,
                255
            );

            SDL_RenderDrawLine(
                renderer,
                button.x,
                button.y + button.h - 1,
                button.x + button.w - 1,
                button.y + button.h - 1
            );

            SDL_RenderDrawLine(
                renderer,
                button.x + button.w - 1,
                button.y,
                button.x + button.w - 1,
                button.y + button.h - 1
            );

            SDL_Texture* icon =
                buildToolIcons[toolIndex];

            if (!icon)
                continue;

            // Road, Field, Wine a Remove jsou funkcni.
            const Uint8 brightness =
                implemented
                    ? 255
                    : 115;

            SDL_SetTextureColorMod(
                icon,
                brightness,
                brightness,
                brightness
            );

            int width = 0;
            int height = 0;

            SDL_QueryTexture(
                icon,
                nullptr,
                nullptr,
                &width,
                &height
            );

            SDL_Rect destination = {
                button.x +
                    (button.w - width) / 2,
                button.y +
                    (button.h - height) / 2,
                width,
                height
            };

            SDL_RenderCopy(
                renderer,
                icon,
                nullptr,
                &destination
            );

            SDL_SetTextureColorMod(
                icon,
                255,
                255,
                255
            );
        }

        for (
            int houseIndex = 0;
            houseIndex < GUI_HOUSE_COUNT;
            ++houseIndex
        ) {
            const int column =
                houseIndex % 5;

            const int row =
                houseIndex / 5;

            SDL_Rect button = {
                contentX +
                    column * buttonStep,
                housesStartY +
                    row * buttonStep,
                buttonSize,
                buttonSize
            };

            const HouseIconType houseType =
                GUI_HOUSE_ORDER[houseIndex];

            bool implemented = false;

            const bool selected =
                ui.buildHouseCatalogSelection ==
                    houseIndex;

            switch (houseType) {
                case HouseIconType::School:
                case HouseIconType::Farm:
                case HouseIconType::Bakery:
                case HouseIconType::Store:
                    implemented = true;
                    break;

                default:
                    break;
            }

            SDL_SetRenderDrawBlendMode(
                renderer,
                SDL_BLENDMODE_BLEND
            );

            SDL_SetRenderDrawColor(
                renderer,
                selected ? 103 : 49,
                selected ? 84 : 42,
                selected ? 48 : 32,
                selected ? 205 : 125
            );

            SDL_RenderFillRect(
                renderer,
                &button
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
                button.x,
                button.y,
                button.x + button.w - 1,
                button.y
            );

            SDL_RenderDrawLine(
                renderer,
                button.x,
                button.y,
                button.x,
                button.y + button.h - 1
            );

            SDL_SetRenderDrawColor(
                renderer,
                selected ? 230 : 171,
                selected ? 194 : 154,
                selected ? 100 : 121,
                255
            );

            SDL_RenderDrawLine(
                renderer,
                button.x,
                button.y + button.h - 1,
                button.x + button.w - 1,
                button.y + button.h - 1
            );

            SDL_RenderDrawLine(
                renderer,
                button.x + button.w - 1,
                button.y,
                button.x + button.w - 1,
                button.y + button.h - 1
            );

            // Vsech 27 katalogovych pozic zobrazuje
            // originální House ikonu.
            //
            // Neimplementovane domy zustavaji ztlumene,
            // ale jejich metadata jsou plne dostupna.
            SDL_Texture* icon =
                getHouseIcon(
                    houseIcons,
                    houseType
                );

            if (!icon)
                continue;

            if (!implemented) {
                SDL_SetTextureColorMod(
                    icon,
                    115,
                    115,
                    115
                );
            } else {
                SDL_SetTextureColorMod(
                    icon,
                    255,
                    255,
                    255
                );
            }

            int width = 0;
            int height = 0;

            SDL_QueryTexture(
                icon,
                nullptr,
                nullptr,
                &width,
                &height
            );

            SDL_Rect destination = {
                button.x +
                    (button.w - width) / 2,
                button.y +
                    (button.h - height) / 2,
                width,
                height
            };

            SDL_RenderCopy(
                renderer,
                icon,
                nullptr,
                &destination
            );

            // Sdilene House textury pouzivaji take Ratios
            // a Statistics, proto vzdy obnovime plnou barvu.
            SDL_SetTextureColorMod(
                icon,
                255,
                255,
                255
            );
        }
    }

    // Oddelovaci prava hrana GUI.
    SDL_SetRenderDrawColor(
        renderer,
        163, 138, 91, 255
    );

    SDL_RenderDrawLine(
        renderer,
        GAME_UI_PANEL_WIDTH - 1,
        0,
        GAME_UI_PANEL_WIDTH - 1,
        543
    );

    // House katalog je od tohoto checkpointu soucasti
    // centralniho Game UI API. Jednotlive panely se na nej
    // prepoji v nasledujicim samostatnem rezu.
    (void)houseIcons;

    // cursorX/cursorY zatim pouze soucast API.
    (void)cursorX;
    (void)cursorY;
}

void moveGameUISelection(
    GameUIState* ui,
    int deltaX,
    int deltaY
)
{
    if (!ui || !ui->buildSectionActive)
        return;

    int current =
        static_cast<int>(
            ui->buildSelection
        );

    // Aktualni KaMVita Build obsahuje pet funkcnich
    // polozek v jedine rade podle ButtonFlat geometrie.
    const int itemCount = 5;

    if (deltaX < 0) {
        --current;

        if (current < 0)
            current = itemCount - 1;
    }

    if (deltaX > 0) {
        ++current;

        if (current >= itemCount)
            current = 0;
    }

    // V jedine rade nahoru/dolu nic neposouva.
    (void)deltaY;

    ui->buildSelection =
        static_cast<GameUIBuildSelection>(
            current
        );
}

static SDL_Texture* loadSchoolUITexture(
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


SchoolUIResources loadSchoolUIResources(
    SDL_Renderer* renderer
)
{
    SchoolUIResources resources = {};

    resources.sidebarTop =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/sidebar/sidebar_top.bmp"
        );

    resources.sidebarMiddle =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/sidebar/sidebar_middle.bmp"
        );

    resources.sidebarBottom =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/sidebar/sidebar_bottom.bmp"
        );

    resources.buttonStone =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/sidebar/buttons/gui_402.bmp"
        );

    static const char* mainButtonFiles[5] = {
        "gui_439.bmp",
        "gui_440.bmp",
        "gui_441.bmp",
        "gui_442.bmp",
        "gui_443.bmp"
    };

    for (int i = 0; i < 5; ++i) {
        char buttonPath[256];

        snprintf(
            buttonPath,
            sizeof(buttonPath),
            "app0:assets/ui/sidebar/buttons/%s",
            mainButtonFiles[i]
        );

        resources.mainButtons[i] =
            loadSchoolUITexture(
                renderer,
                buttonPath
            );
    }

    static const char* schoolButtonFiles[3] = {
        "previous.bmp",
        "train.bmp",
        "next.bmp"
    };

    for (int i = 0; i < 3; ++i) {
        char buttonPath[256];

        snprintf(
            buttonPath,
            sizeof(buttonPath),
            "app0:assets/ui/school/buttons/%s",
            schoolButtonFiles[i]
        );

        resources.schoolButtons[i] =
            loadSchoolUITexture(
                renderer,
                buttonPath
            );
    }

    resources.goldIcon =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/school/wares/gold.bmp"
        );

    // -----------------------------------------------------
    // Originalni Store GUI resources
    // GUI 351..378 = WareType 0..27
    // GUI 49       = accept/not-accept marker
    // -----------------------------------------------------

    for (int i = 0; i < STORE_WARE_COUNT; ++i) {
        char storeWarePath[256];

        snprintf(
            storeWarePath,
            sizeof(storeWarePath),
            "app0:assets/ui/store/wares/gui_%04d.bmp",
            STORE_GUI_FIRST_WARE_SPRITE + i
        );

        resources.storeWareIcons[i] =
            loadSchoolUITexture(
                renderer,
                storeWarePath
            );
    }

    resources.storeAcceptFlag =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/store/controls/accept_flag.bmp"
        );

    // Originalni GUI sprite 41 = question mark.
    resources.questionIcon =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/school/queue/question.bmp"
        );

    resources.deliveryOn =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/house/header/delivery_on.bmp"
        );

    resources.deliveryOff =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/house/header/delivery_off.bmp"
        );

    resources.repairOn =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/house/header/repair_on.bmp"
        );

    resources.repairOff =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/house/header/repair_off.bmp"
        );

    resources.houseWorker =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/house/header/worker.bmp"
        );

    // Originalni fnt_Outline nazev budovy.
    resources.schoolHouseText =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/house/text/school_house.bmp"
        );

    static const char* ratioWareFiles[4] = {
        "steel",
        "coal",
        "wood",
        "corn"
    };

    for (int i = 0; i < 4; ++i) {
        char path[256];

        snprintf(
            path,
            sizeof(path),
            "app0:assets/ui/ratios/wares/%s.bmp",
            ratioWareFiles[i]
        );

        resources.ratiosWareIcons[i] =
            loadSchoolUITexture(
                renderer,
                path
            );
    }

    resources.ratiosTrackbarThumb =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/ratios/trackbar_thumb.bmp"
        );

    // Road/Field/Wine/Remove nejsou HouseType.
    // Proto zustavaji samostatnymi Build resources.
    resources.buildRoadIcon =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/build/original/road.bmp"
        );

    resources.buildFieldIcon =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/build/tools/field.bmp"
        );

    resources.buildWineIcon =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/build/tools/wine.bmp"
        );

    resources.buildRemoveIcon =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/build/tools/remove.bmp"
        );

    resources.buildWoodIcon =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/build/costs/wood.bmp"
        );

    resources.buildStoneIcon =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/build/costs/stone.bmp"
        );

    static const char* buildNameFiles[7] = {
        "road",
        "store",
        "bakery",
        "farm",
        "school",
        "field",
        "wine"
    };

    for (int i = 0; i < 7; ++i) {
        char path[256];

        snprintf(
            path,
            sizeof(path),
            "app0:assets/ui/build/text/%s.bmp",
            buildNameFiles[i]
        );

        resources.buildNameText[i] =
            loadSchoolUITexture(
                renderer,
                path
            );
    }

    resources.buildDemolishText =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/build/text/demolish.bmp"
        );

    static const char* buildHouseNameFiles[29] = {
        "armor_smithy",
        "armor_workshop",
        "bakery",
        "barracks",
        "butchers",
        "coal_mine",
        "farm",
        "fisher_hut",
        "gold_mine",
        "inn",
        "iron_mine",
        "iron_smithy",
        "marketplace",
        "metallurgist",
        "mill",
        "quarry",
        "sawmill",
        "school",
        "siege_workshop",
        "stables",
        "store",
        "swine",
        "tannery",
        "town_hall",
        "watch_tower",
        "weapon_smithy",
        "weapon_workshop",
        "wineyard",
        "woodcutter",
    };

    for (int i = 0; i < 29; ++i) {
        char path[256];

        snprintf(
            path,
            sizeof(path),
            "app0:assets/ui/build/text/houses/%s.bmp",
            buildHouseNameFiles[i]
        );

        resources.buildHouseNameText[i] =
            loadSchoolUITexture(
                renderer,
                path
            );
    }

    resources.statisticsCounts =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/statistics/counts.bmp"
        );

    resources.statisticsWipCounts =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/statistics/wip_counts.bmp"
        );

    resources.needsText =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/school/text/needs.bmp"
        );

    resources.goldText =
        loadSchoolUITexture(
            renderer,
            "app0:assets/ui/school/text/gold.bmp"
        );

    static const char* professionTextFiles[
        SCHOOL_PROFESSION_COUNT
    ] = {
        "serf",
        "worker",
        "stonecutter",
        "woodcutter",
        "lamberjack",
        "fisher",
        "farmer",
        "baker",
        "animal_breeder",
        "butcher",
        "miner",
        "metallurgist",
        "smith",
        "recruit"
    };

    for (
        int i = 0;
        i < SCHOOL_PROFESSION_COUNT;
        ++i
    ) {
        char textPath[256];

        snprintf(
            textPath,
            sizeof(textPath),
            "app0:assets/ui/school/text/professions/%s.bmp",
            professionTextFiles[i]
        );

        resources.professionText[i] =
            loadSchoolUITexture(
                renderer,
                textPath
            );
    }

    static const char* professionNames[
        SCHOOL_PROFESSION_COUNT
    ] = {
        "serf",
        "worker",
        "stonecutter",
        "woodcutter",
        "lamberjack",
        "fisher",
        "farmer",
        "baker",
        "animal_breeder",
        "butcher",
        "miner",
        "metallurgist",
        "smith",
        "recruit"
    };

    for (
        int i = 0;
        i < SCHOOL_PROFESSION_COUNT;
        ++i
    ) {
        char iconPath[256];
        char scrollPath[256];

        snprintf(
            iconPath,
            sizeof(iconPath),
            "app0:assets/ui/school/professions/%s/icon.bmp",
            professionNames[i]
        );

        snprintf(
            scrollPath,
            sizeof(scrollPath),
            "app0:assets/ui/school/professions/%s/scroll.bmp",
            professionNames[i]
        );

        resources.professions[i].icon =
            loadSchoolUITexture(
                renderer,
                iconPath
            );

        resources.professions[i].scroll =
            loadSchoolUITexture(
                renderer,
                scrollPath
            );
    }

    return resources;
}


void destroySchoolUIResources(
    SchoolUIResources* resources
)
{
    if (!resources)
        return;

    if (resources->sidebarTop) {
        SDL_DestroyTexture(
            resources->sidebarTop
        );
    }

    if (resources->sidebarMiddle) {
        SDL_DestroyTexture(
            resources->sidebarMiddle
        );
    }

    if (resources->sidebarBottom) {
        SDL_DestroyTexture(
            resources->sidebarBottom
        );
    }

    if (resources->buttonStone) {
        SDL_DestroyTexture(
            resources->buttonStone
        );
    }

    for (int i = 0; i < 5; ++i) {
        if (resources->mainButtons[i]) {
            SDL_DestroyTexture(
                resources->mainButtons[i]
            );
        }
    }

    for (int i = 0; i < 3; ++i) {
        if (resources->schoolButtons[i]) {
            SDL_DestroyTexture(
                resources->schoolButtons[i]
            );
        }
    }

    if (resources->goldIcon) {
        SDL_DestroyTexture(
            resources->goldIcon
        );
    }

    for (int i = 0; i < STORE_WARE_COUNT; ++i) {
        if (resources->storeWareIcons[i]) {
            SDL_DestroyTexture(
                resources->storeWareIcons[i]
            );

            resources->storeWareIcons[i] = nullptr;
        }
    }

    if (resources->storeAcceptFlag) {
        SDL_DestroyTexture(
            resources->storeAcceptFlag
        );

        resources->storeAcceptFlag = nullptr;
    }

    if (resources->questionIcon) {
        SDL_DestroyTexture(
            resources->questionIcon
        );
    }

    if (resources->deliveryOn) {
        SDL_DestroyTexture(resources->deliveryOn);
    }

    if (resources->deliveryOff) {
        SDL_DestroyTexture(resources->deliveryOff);
    }

    if (resources->repairOn) {
        SDL_DestroyTexture(resources->repairOn);
    }

    if (resources->repairOff) {
        SDL_DestroyTexture(resources->repairOff);
    }

    if (resources->houseWorker) {
        SDL_DestroyTexture(resources->houseWorker);
    }

    if (resources->schoolHouseText) {
        SDL_DestroyTexture(
            resources->schoolHouseText
        );
    }

    for (int i = 0; i < 4; ++i) {
        if (resources->ratiosWareIcons[i]) {
            SDL_DestroyTexture(
                resources->ratiosWareIcons[i]
            );
        }
    }

    if (resources->ratiosTrackbarThumb) {
        SDL_DestroyTexture(
            resources->ratiosTrackbarThumb
        );
    }

    if (resources->buildRoadIcon) {
        SDL_DestroyTexture(
            resources->buildRoadIcon
        );

        resources->buildRoadIcon = nullptr;
    }

    if (resources->buildFieldIcon) {
        SDL_DestroyTexture(
            resources->buildFieldIcon
        );

        resources->buildFieldIcon = nullptr;
    }

    if (resources->buildWineIcon) {
        SDL_DestroyTexture(
            resources->buildWineIcon
        );

        resources->buildWineIcon = nullptr;
    }

    if (resources->buildRemoveIcon) {
        SDL_DestroyTexture(
            resources->buildRemoveIcon
        );

        resources->buildRemoveIcon = nullptr;
    }

    if (resources->buildWoodIcon) {
        SDL_DestroyTexture(
            resources->buildWoodIcon
        );

        resources->buildWoodIcon = nullptr;
    }

    if (resources->buildStoneIcon) {
        SDL_DestroyTexture(
            resources->buildStoneIcon
        );

        resources->buildStoneIcon = nullptr;
    }

    for (int i = 0; i < 7; ++i) {
        if (resources->buildNameText[i]) {
            SDL_DestroyTexture(
                resources->buildNameText[i]
            );

            resources->buildNameText[i] =
                nullptr;
        }
    }


    if (resources->buildDemolishText) {
        SDL_DestroyTexture(
            resources->buildDemolishText
        );

        resources->buildDemolishText =
            nullptr;
    }

    for (int i = 0; i < 29; ++i) {
        if (resources->buildHouseNameText[i]) {
            SDL_DestroyTexture(
                resources->buildHouseNameText[i]
            );

            resources->buildHouseNameText[i] =
                nullptr;
        }
    }

    if (resources->statisticsCounts) {
        SDL_DestroyTexture(
            resources->statisticsCounts
        );
    }

    if (resources->statisticsWipCounts) {
        SDL_DestroyTexture(
            resources->statisticsWipCounts
        );
    }

    if (resources->needsText) {
        SDL_DestroyTexture(
            resources->needsText
        );
    }

    if (resources->goldText) {
        SDL_DestroyTexture(
            resources->goldText
        );
    }

    for (
        int i = 0;
        i < SCHOOL_PROFESSION_COUNT;
        ++i
    ) {
        if (resources->professionText[i]) {
            SDL_DestroyTexture(
                resources->professionText[i]
            );
        }
    }

    for (
        int i = 0;
        i < SCHOOL_PROFESSION_COUNT;
        ++i
    ) {
        if (resources->professions[i].icon) {
            SDL_DestroyTexture(
                resources->professions[i].icon
            );
        }

        if (resources->professions[i].scroll) {
            SDL_DestroyTexture(
                resources->professions[i].scroll
            );
        }
    }

    *resources = {};

}


void moveSchoolUISelection(
    GameUIState* ui,
    int delta
)
{
    if (
        !ui ||
        ui->section != GameUISection::School
    ) {
        return;
    }

    int selection =
        static_cast<int>(
            ui->schoolSelection
        );

    selection += delta;

    if (selection < 0)
        selection = 0;

    if (
        selection >=
        SCHOOL_PROFESSION_COUNT
    ) {
        selection =
            SCHOOL_PROFESSION_COUNT - 1;
    }

    ui->schoolSelection =
        static_cast<GameUISchoolSelection>(
            selection
        );
}
