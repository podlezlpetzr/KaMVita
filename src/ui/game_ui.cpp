#include "game_ui.h"
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

    ui->section = GameUISection::Build;

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
    const GameUIState& ui,
    const Building* buildings,
    int buildingCount,
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

        // Docasny Gold symbol.
        // Funkcni pocet zlata pripojime ke konkretni School.
        SDL_Rect goldOuter = {
            panelX + 74,
            panelY + 23,
            32,
            14
        };

        SDL_SetRenderDrawColor(
            renderer,
            95, 69, 30, 255
        );

        SDL_RenderFillRect(
            renderer,
            &goldOuter
        );

        SDL_SetRenderDrawColor(
            renderer,
            242, 193, 54, 255
        );

        SDL_RenderDrawRect(
            renderer,
            &goldOuter
        );

        int visibleGold = 0;

        if (selectedSchool) {
            visibleGold = selectedSchool->schoolGold;

            if (visibleGold < 0)
                visibleGold = 0;

            if (visibleGold > goldOuter.w - 10)
                visibleGold = goldOuter.w - 10;
        }

        SDL_Rect goldInner = {
            goldOuter.x + 5,
            goldOuter.y + 4,
            visibleGold,
            goldOuter.h - 8
        };

        if (goldInner.w > 0) {
            SDL_RenderFillRect(
                renderer,
                &goldInner
            );
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

            SDL_SetTextureColorMod(
                schoolResources.buttonStone,
                138,
                133,
                119
            );

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

            // Queue slot ve stylu originalniho KaM.
            if (schoolResources.buttonStone) {
                SDL_Rect queueStoneSource = {
                    8 + i * 34,
                    92,
                    queueSlot.w,
                    queueSlot.h
                };

                // Prázdné sloty jsou subtilnější než WIP.
                SDL_SetTextureColorMod(
                    schoolResources.buttonStone,
                    115,
                    111,
                    100
                );

                SDL_RenderCopy(
                    renderer,
                    schoolResources.buttonStone,
                    &queueStoneSource,
                    &queueSlot
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
                    45, 41, 34, 255
                );

                SDL_RenderFillRect(
                    renderer,
                    &queueSlot
                );
            }

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

            SDL_SetRenderDrawColor(
                renderer,
                portraitFocused ? 104 : 65,
                portraitFocused ? 80 : 54,
                portraitFocused ? 42 : 35,
                255
            );

            SDL_RenderFillRect(
                renderer,
                &portraits[i].area
            );

            SDL_SetRenderDrawColor(
                renderer,
                portraitFocused ? 224 : 124,
                portraitFocused ? 184 : 101,
                portraitFocused ? 97 : 61,
                255
            );

            SDL_RenderDrawRect(
                renderer,
                &portraits[i].area
            );

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

            SDL_RenderCopy(
                renderer,
                texture,
                nullptr,
                &destination
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

    // BUILD ikony zobrazujeme pouze v Build sekci.
    // School ma vlastni panel a nesmi byt prekryta build menu.
    if (ui.section == GameUISection::Build) {
        // BUILD ikony dole.
        SDL_Texture* icons[5] = {
            resources.roadIcon,
            resources.storeIcon,
            resources.bakeryIcon,
            resources.farmIcon,
            resources.schoolIcon
        };

        const int columns = 3;
        const int slotSize = 58;
        const int gap = 6;
        const int startX = 12;
        const int startY = 295;

        for (int i = 0; i < 5; ++i) {
            int column = i % columns;
            int row = i / columns;

            SDL_Rect slot = {
                startX + column * (slotSize + gap),
                startY + row * (slotSize + gap),
                slotSize,
                slotSize
            };

            bool selected =
                (int)ui.buildSelection == i;

            if (selected) {
                SDL_SetRenderDrawColor(
                    renderer,
                    181, 142, 60, 255
                );
            } else {
                SDL_SetRenderDrawColor(
                    renderer,
                    73, 59, 38, 255
                );
            }

            SDL_RenderFillRect(
                renderer,
                &slot
            );

            SDL_SetRenderDrawColor(
                renderer,
                selected ? 255 : 137,
                selected ? 220 : 112,
                selected ? 115 : 70,
                255
            );

            SDL_RenderDrawRect(
                renderer,
                &slot
            );

            if (icons[i]) {
                int iconWidth = 0;
                int iconHeight = 0;

                SDL_QueryTexture(
                    icons[i],
                    nullptr,
                    nullptr,
                    &iconWidth,
                    &iconHeight
                );

                const int maxIconWidth = slot.w - 10;
                const int maxIconHeight = slot.h - 10;

                float scaleX =
                    (float)maxIconWidth / iconWidth;

                float scaleY =
                    (float)maxIconHeight / iconHeight;

                float scale =
                    scaleX < scaleY
                        ? scaleX
                        : scaleY;

                // Originalni male KaM ikony nezvetsovat
                // nad jejich puvodni velikost.
                if (scale > 1.0f)
                    scale = 1.0f;

                int drawWidth =
                    (int)(iconWidth * scale);

                int drawHeight =
                    (int)(iconHeight * scale);

                SDL_Rect iconRect = {
                    slot.x +
                        (slot.w - drawWidth) / 2,
                    slot.y +
                        (slot.h - drawHeight) / 2,
                    drawWidth,
                    drawHeight
                };

                SDL_RenderCopy(
                    renderer,
                    icons[i],
                    nullptr,
                    &iconRect
                );
            }
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
        static_cast<int>(ui->buildSelection);

    const int columns = 3;
    const int itemCount = 5;

    int column = current % columns;
    int row = current / columns;

    if (deltaX < 0) {
        if (column > 0)
            current--;
    } else if (deltaX > 0) {
        if (column < columns - 1 &&
            current + 1 < itemCount) {
            current++;
        }
    }

    if (deltaY < 0) {
        if (row > 0)
            current -= columns;
    } else if (deltaY > 0) {
        if (current + columns < itemCount)
            current += columns;
    }

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
