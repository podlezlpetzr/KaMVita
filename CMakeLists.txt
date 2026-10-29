cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VITASDK})
        set(
            CMAKE_TOOLCHAIN_FILE
            "$ENV{VITASDK}/share/vita.toolchain.cmake"
            CACHE PATH "VitaSDK toolchain file"
        )
    else()
        message(
            FATAL_ERROR
            "VITASDK environment variable is not set"
        )
    endif()
endif()

project(KaMVita LANGUAGES C CXX)

include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "KaM Vita")
set(VITA_TITLEID "KAMV00001")
set(VITA_VERSION "00.01")

find_package(SDL2 REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/render/terrain.cpp
    src/game/buildings.cpp
    src/game/units.cpp
    src/game/ware_distribution.cpp
    src/ui/game_ui.cpp
    src/ui/house_icons.cpp
    src/ui/game_ui_statistics.cpp
    src/ui/game_ui_ratios.cpp
)

target_link_options(${PROJECT_NAME}
    PRIVATE
    "-Wl,-z,max-page-size=0x10000"
)

target_compile_features(${PROJECT_NAME}
    PRIVATE
    cxx_std_11
)

target_link_libraries(${PROJECT_NAME}
    SDL2::SDL2-static

    png
    z

    ScePower_stub
    SceDisplay_stub
    SceCtrl_stub
    SceAudio_stub
    SceAudioIn_stub
    SceSysmodule_stub
    SceGxm_stub
    SceCommonDialog_stub
    SceAppMgr_stub
    SceTouch_stub
    SceHid_stub
    SceMotion_stub
    SceIofilemgr_stub

    m
)

vita_create_self(
    eboot.bin
    ${PROJECT_NAME}
)

vita_create_vpk(
    ${PROJECT_NAME}.vpk
    ${VITA_TITLEID}
    eboot.bin

    VERSION ${VITA_VERSION}
    NAME ${VITA_APP_NAME}

    FILE
        assets/terrain/test_tile.bmp
        assets/terrain/test_tile.bmp

    FILE
        assets/terrain/grass.bmp
        assets/terrain/grass.bmp

    FILE
        assets/terrain/water.bmp
        assets/terrain/water.bmp

    FILE
        assets/terrain/dirt.bmp
        assets/terrain/dirt.bmp

    FILE
        assets/terrain/sand.bmp
        assets/terrain/sand.bmp

    FILE
        assets/buildings/store.bmp
        assets/buildings/store.bmp

    FILE
        assets/buildings/bakery.bmp
        assets/buildings/bakery.bmp

    FILE
        assets/buildings/farm.bmp
        assets/buildings/farm.bmp

    FILE
        assets/ui/buildings/store_icon.bmp
        assets/ui/buildings/store_icon.bmp

    FILE
        assets/ui/buildings/bakery_icon.bmp
        assets/ui/buildings/bakery_icon.bmp

    FILE
        assets/ui/buildings/farm_icon.bmp
        assets/ui/buildings/farm_icon.bmp

    FILE
        assets/buildings/construction/store/wood_atlas.bmp
        assets/buildings/construction/store/wood_atlas.bmp

    FILE
        assets/buildings/construction/store/stone_atlas.bmp
        assets/buildings/construction/store/stone_atlas.bmp

    FILE
        assets/buildings/construction/bakery/wood_atlas.bmp
        assets/buildings/construction/bakery/wood_atlas.bmp

    FILE
        assets/buildings/construction/bakery/stone_atlas.bmp
        assets/buildings/construction/bakery/stone_atlas.bmp

    FILE
        assets/buildings/construction/farm/wood_atlas.bmp
        assets/buildings/construction/farm/wood_atlas.bmp

    FILE
        assets/buildings/construction/farm/stone_atlas.bmp
        assets/buildings/construction/farm/stone_atlas.bmp

    FILE
        assets/buildings/school.bmp
        assets/buildings/school.bmp

    FILE
        assets/ui/buildings/school_icon.bmp
        assets/ui/buildings/school_icon.bmp

    FILE
        assets/buildings/construction/school/wood_atlas.bmp
        assets/buildings/construction/school/wood_atlas.bmp

    FILE
        assets/buildings/construction/school/stone_atlas.bmp
        assets/buildings/construction/school/stone_atlas.bmp

    FILE
        assets/units/worker/idle_s.bmp
        assets/units/worker/idle_s.bmp

    FILE
        assets/ui/units/worker_icon.bmp
        assets/ui/units/worker_icon.bmp

    FILE
        assets/units/worker/walk_atlas.bmp
        assets/units/worker/walk_atlas.bmp

    FILE
        assets/ui/buildings/road_icon.bmp
        assets/ui/buildings/road_icon.bmp

    FILE
        assets/terrain/roads/dig1.bmp
        assets/terrain/roads/dig1.bmp

    FILE
        assets/terrain/roads/dig2.bmp
        assets/terrain/roads/dig2.bmp

    FILE
        assets/terrain/roads/dig3.bmp
        assets/terrain/roads/dig3.bmp

    FILE
        assets/terrain/roads/dig4.bmp
        assets/terrain/roads/dig4.bmp

    FILE
        assets/terrain/roads/road_complete.bmp
        assets/terrain/roads/road_complete.bmp

    FILE
        assets/units/worker/work_n_atlas.bmp
        assets/units/worker/work_n_atlas.bmp

    FILE
        assets/terrain/roads/original/dig1.bmp
        assets/terrain/roads/original/dig1.bmp

    FILE
        assets/terrain/roads/original/dig2.bmp
        assets/terrain/roads/original/dig2.bmp

    FILE
        assets/terrain/roads/original/dig3.bmp
        assets/terrain/roads/original/dig3.bmp

    FILE
        assets/terrain/roads/original/dig4.bmp
        assets/terrain/roads/original/dig4.bmp

    FILE
        assets/terrain/roads/original/road_248.bmp
        assets/terrain/roads/original/road_248.bmp

    FILE
        assets/terrain/roads/original/road_250.bmp
        assets/terrain/roads/original/road_250.bmp

    FILE
        assets/terrain/roads/original/road_252.bmp
        assets/terrain/roads/original/road_252.bmp

    FILE
        assets/terrain/roads/original/road_254.bmp
        assets/terrain/roads/original/road_254.bmp

    FILE
        assets/units/worker/work1_n_atlas.bmp
        assets/units/worker/work1_n_atlas.bmp

    FILE
        assets/units/worker/work2_n_atlas.bmp
        assets/units/worker/work2_n_atlas.bmp

    FILE
        assets/units/serf/walk_atlas.bmp
        assets/units/serf/walk_atlas.bmp

    FILE
        assets/units/serf/carry_stone_atlas.bmp
        assets/units/serf/carry_stone_atlas.bmp

    FILE
        assets/ui/school/serf_icon.bmp
        assets/ui/school/serf_icon.bmp

    FILE
        assets/ui/school/serf_scroll.bmp
        assets/ui/school/serf_scroll.bmp

    FILE
        assets/ui/school/worker_icon.bmp
        assets/ui/school/worker_icon.bmp

    FILE
        assets/ui/school/worker_scroll.bmp
        assets/ui/school/worker_scroll.bmp

    FILE
        assets/units/worker/complete/actions/walk_atlas.bmp
        assets/units/worker/complete/actions/walk_atlas.bmp

    FILE
        assets/units/worker/complete/actions/work_atlas.bmp
        assets/units/worker/complete/actions/work_atlas.bmp

    FILE
        assets/units/worker/complete/actions/work1_atlas.bmp
        assets/units/worker/complete/actions/work1_atlas.bmp

    FILE
        assets/units/worker/complete/actions/work2_atlas.bmp
        assets/units/worker/complete/actions/work2_atlas.bmp

    FILE
        assets/units/worker/complete/actions/die_atlas.bmp
        assets/units/worker/complete/actions/die_atlas.bmp

    FILE
        assets/units/worker/complete/actions/eat_atlas.bmp
        assets/units/worker/complete/actions/eat_atlas.bmp

    FILE
        assets/buildings/school/runtime/flagpole/flagpole.bmp
        assets/buildings/school/runtime/flagpole/flagpole.bmp

    FILE
        assets/buildings/school/runtime/flag_atlas.bmp
        assets/buildings/school/runtime/flag_atlas.bmp

    FILE
        assets/buildings/school/runtime/work_atlas.bmp
        assets/buildings/school/runtime/work_atlas.bmp

    FILE
        assets/ui/school/professions/animal_breeder/icon.bmp
        assets/ui/school/professions/animal_breeder/icon.bmp

    FILE
        assets/ui/school/professions/animal_breeder/scroll.bmp
        assets/ui/school/professions/animal_breeder/scroll.bmp

    FILE
        assets/ui/school/professions/baker/icon.bmp
        assets/ui/school/professions/baker/icon.bmp

    FILE
        assets/ui/school/professions/baker/scroll.bmp
        assets/ui/school/professions/baker/scroll.bmp

    FILE
        assets/ui/school/professions/butcher/icon.bmp
        assets/ui/school/professions/butcher/icon.bmp

    FILE
        assets/ui/school/professions/butcher/scroll.bmp
        assets/ui/school/professions/butcher/scroll.bmp

    FILE
        assets/ui/school/professions/farmer/icon.bmp
        assets/ui/school/professions/farmer/icon.bmp

    FILE
        assets/ui/school/professions/farmer/scroll.bmp
        assets/ui/school/professions/farmer/scroll.bmp

    FILE
        assets/ui/school/professions/fisher/icon.bmp
        assets/ui/school/professions/fisher/icon.bmp

    FILE
        assets/ui/school/professions/fisher/scroll.bmp
        assets/ui/school/professions/fisher/scroll.bmp

    FILE
        assets/ui/school/professions/lamberjack/icon.bmp
        assets/ui/school/professions/lamberjack/icon.bmp

    FILE
        assets/ui/school/professions/lamberjack/scroll.bmp
        assets/ui/school/professions/lamberjack/scroll.bmp

    FILE
        assets/ui/school/professions/metallurgist/icon.bmp
        assets/ui/school/professions/metallurgist/icon.bmp

    FILE
        assets/ui/school/professions/metallurgist/scroll.bmp
        assets/ui/school/professions/metallurgist/scroll.bmp

    FILE
        assets/ui/school/professions/miner/icon.bmp
        assets/ui/school/professions/miner/icon.bmp

    FILE
        assets/ui/school/professions/miner/scroll.bmp
        assets/ui/school/professions/miner/scroll.bmp

    FILE
        assets/ui/school/professions/recruit/icon.bmp
        assets/ui/school/professions/recruit/icon.bmp

    FILE
        assets/ui/school/professions/recruit/scroll.bmp
        assets/ui/school/professions/recruit/scroll.bmp

    FILE
        assets/ui/school/professions/serf/icon.bmp
        assets/ui/school/professions/serf/icon.bmp

    FILE
        assets/ui/school/professions/serf/scroll.bmp
        assets/ui/school/professions/serf/scroll.bmp

    FILE
        assets/ui/school/professions/smith/icon.bmp
        assets/ui/school/professions/smith/icon.bmp

    FILE
        assets/ui/school/professions/smith/scroll.bmp
        assets/ui/school/professions/smith/scroll.bmp

    FILE
        assets/ui/school/professions/stonecutter/icon.bmp
        assets/ui/school/professions/stonecutter/icon.bmp

    FILE
        assets/ui/school/professions/stonecutter/scroll.bmp
        assets/ui/school/professions/stonecutter/scroll.bmp

    FILE
        assets/ui/school/professions/woodcutter/icon.bmp
        assets/ui/school/professions/woodcutter/icon.bmp

    FILE
        assets/ui/school/professions/woodcutter/scroll.bmp
        assets/ui/school/professions/woodcutter/scroll.bmp

    FILE
        assets/ui/school/professions/worker/icon.bmp
        assets/ui/school/professions/worker/icon.bmp

    FILE
        assets/ui/school/professions/worker/scroll.bmp
        assets/ui/school/professions/worker/scroll.bmp
    FILE
        assets/ui/sidebar/sidebar_top.bmp
        assets/ui/sidebar/sidebar_top.bmp

    FILE
        assets/ui/sidebar/sidebar_middle.bmp
        assets/ui/sidebar/sidebar_middle.bmp

    FILE
        assets/ui/sidebar/sidebar_bottom.bmp
        assets/ui/sidebar/sidebar_bottom.bmp

    FILE
        assets/ui/sidebar/buttons/gui_402.bmp
        assets/ui/sidebar/buttons/gui_402.bmp

    FILE
        assets/ui/sidebar/buttons/gui_439.bmp
        assets/ui/sidebar/buttons/gui_439.bmp

    FILE
        assets/ui/sidebar/buttons/gui_440.bmp
        assets/ui/sidebar/buttons/gui_440.bmp

    FILE
        assets/ui/sidebar/buttons/gui_441.bmp
        assets/ui/sidebar/buttons/gui_441.bmp

    FILE
        assets/ui/sidebar/buttons/gui_442.bmp
        assets/ui/sidebar/buttons/gui_442.bmp

    FILE
        assets/ui/sidebar/buttons/gui_443.bmp
        assets/ui/sidebar/buttons/gui_443.bmp

    FILE
        assets/ui/school/buttons/previous.bmp
        assets/ui/school/buttons/previous.bmp

    FILE
        assets/ui/school/buttons/train.bmp
        assets/ui/school/buttons/train.bmp

    FILE
        assets/ui/school/buttons/next.bmp
        assets/ui/school/buttons/next.bmp

    FILE
        assets/ui/school/wares/gold.bmp
        assets/ui/school/wares/gold.bmp

    FILE
        assets/ui/school/text/needs.bmp
        assets/ui/school/text/needs.bmp

    FILE
        assets/ui/school/text/gold.bmp
        assets/ui/school/text/gold.bmp

    FILE
        assets/ui/school/text/professions/serf.bmp
        assets/ui/school/text/professions/serf.bmp

    FILE
        assets/ui/school/text/professions/worker.bmp
        assets/ui/school/text/professions/worker.bmp

    FILE
        assets/ui/school/text/professions/stonecutter.bmp
        assets/ui/school/text/professions/stonecutter.bmp

    FILE
        assets/ui/school/text/professions/woodcutter.bmp
        assets/ui/school/text/professions/woodcutter.bmp

    FILE
        assets/ui/school/text/professions/lamberjack.bmp
        assets/ui/school/text/professions/lamberjack.bmp

    FILE
        assets/ui/school/text/professions/fisher.bmp
        assets/ui/school/text/professions/fisher.bmp

    FILE
        assets/ui/school/text/professions/farmer.bmp
        assets/ui/school/text/professions/farmer.bmp

    FILE
        assets/ui/school/text/professions/baker.bmp
        assets/ui/school/text/professions/baker.bmp

    FILE
        assets/ui/school/text/professions/animal_breeder.bmp
        assets/ui/school/text/professions/animal_breeder.bmp

    FILE
        assets/ui/school/text/professions/butcher.bmp
        assets/ui/school/text/professions/butcher.bmp

    FILE
        assets/ui/school/text/professions/miner.bmp
        assets/ui/school/text/professions/miner.bmp

    FILE
        assets/ui/school/text/professions/metallurgist.bmp
        assets/ui/school/text/professions/metallurgist.bmp

    FILE
        assets/ui/school/text/professions/smith.bmp
        assets/ui/school/text/professions/smith.bmp

    FILE
        assets/ui/school/text/professions/recruit.bmp
        assets/ui/school/text/professions/recruit.bmp


    FILE
        assets/ui/school/queue/question.bmp
        assets/ui/school/queue/question.bmp


    FILE
        assets/ui/house/header/delivery_on.bmp
        assets/ui/house/header/delivery_on.bmp


    FILE
        assets/ui/house/header/delivery_off.bmp
        assets/ui/house/header/delivery_off.bmp


    FILE
        assets/ui/house/header/repair_on.bmp
        assets/ui/house/header/repair_on.bmp


    FILE
        assets/ui/house/header/repair_off.bmp
        assets/ui/house/header/repair_off.bmp


    FILE
        assets/ui/house/header/worker.bmp
        assets/ui/house/header/worker.bmp


    FILE
        assets/ui/house/text/school_house.bmp
        assets/ui/house/text/school_house.bmp


    FILE
        assets/ui/statistics/counts.bmp
        assets/ui/statistics/counts.bmp


    FILE
        assets/ui/statistics/wip_counts.bmp
        assets/ui/statistics/wip_counts.bmp


    FILE
        assets/ui/ratios/wares/steel.bmp
        assets/ui/ratios/wares/steel.bmp


    FILE
        assets/ui/ratios/wares/coal.bmp
        assets/ui/ratios/wares/coal.bmp


    FILE
        assets/ui/ratios/wares/wood.bmp
        assets/ui/ratios/wares/wood.bmp


    FILE
        assets/ui/ratios/wares/corn.bmp
        assets/ui/ratios/wares/corn.bmp


    FILE
        assets/ui/ratios/trackbar_thumb.bmp
        assets/ui/ratios/trackbar_thumb.bmp




















    FILE
        assets/ui/build/original/road.bmp
        assets/ui/build/original/road.bmp


    FILE
        assets/ui/build/original/store.bmp
        assets/ui/build/original/store.bmp


    FILE
        assets/ui/build/original/bakery.bmp
        assets/ui/build/original/bakery.bmp


    FILE
        assets/ui/build/original/farm.bmp
        assets/ui/build/original/farm.bmp


    FILE
        assets/ui/build/original/school.bmp
        assets/ui/build/original/school.bmp


    FILE
        assets/ui/houses/icons/armor_smithy.bmp
        assets/ui/houses/icons/armor_smithy.bmp

    FILE
        assets/ui/houses/icons/armor_workshop.bmp
        assets/ui/houses/icons/armor_workshop.bmp

    FILE
        assets/ui/houses/icons/bakery.bmp
        assets/ui/houses/icons/bakery.bmp

    FILE
        assets/ui/houses/icons/barracks.bmp
        assets/ui/houses/icons/barracks.bmp

    FILE
        assets/ui/houses/icons/butchers.bmp
        assets/ui/houses/icons/butchers.bmp

    FILE
        assets/ui/houses/icons/coal_mine.bmp
        assets/ui/houses/icons/coal_mine.bmp

    FILE
        assets/ui/houses/icons/farm.bmp
        assets/ui/houses/icons/farm.bmp

    FILE
        assets/ui/houses/icons/fisher_hut.bmp
        assets/ui/houses/icons/fisher_hut.bmp

    FILE
        assets/ui/houses/icons/gold_mine.bmp
        assets/ui/houses/icons/gold_mine.bmp

    FILE
        assets/ui/houses/icons/inn.bmp
        assets/ui/houses/icons/inn.bmp

    FILE
        assets/ui/houses/icons/iron_mine.bmp
        assets/ui/houses/icons/iron_mine.bmp

    FILE
        assets/ui/houses/icons/iron_smithy.bmp
        assets/ui/houses/icons/iron_smithy.bmp

    FILE
        assets/ui/houses/icons/marketplace.bmp
        assets/ui/houses/icons/marketplace.bmp

    FILE
        assets/ui/houses/icons/metallurgist.bmp
        assets/ui/houses/icons/metallurgist.bmp

    FILE
        assets/ui/houses/icons/mill.bmp
        assets/ui/houses/icons/mill.bmp

    FILE
        assets/ui/houses/icons/quarry.bmp
        assets/ui/houses/icons/quarry.bmp

    FILE
        assets/ui/houses/icons/sawmill.bmp
        assets/ui/houses/icons/sawmill.bmp

    FILE
        assets/ui/houses/icons/school.bmp
        assets/ui/houses/icons/school.bmp

    FILE
        assets/ui/houses/icons/siege_workshop.bmp
        assets/ui/houses/icons/siege_workshop.bmp

    FILE
        assets/ui/houses/icons/stables.bmp
        assets/ui/houses/icons/stables.bmp

    FILE
        assets/ui/houses/icons/store.bmp
        assets/ui/houses/icons/store.bmp

    FILE
        assets/ui/houses/icons/swine.bmp
        assets/ui/houses/icons/swine.bmp

    FILE
        assets/ui/houses/icons/tannery.bmp
        assets/ui/houses/icons/tannery.bmp

    FILE
        assets/ui/houses/icons/town_hall.bmp
        assets/ui/houses/icons/town_hall.bmp

    FILE
        assets/ui/houses/icons/watch_tower.bmp
        assets/ui/houses/icons/watch_tower.bmp

    FILE
        assets/ui/houses/icons/weapon_smithy.bmp
        assets/ui/houses/icons/weapon_smithy.bmp

    FILE
        assets/ui/houses/icons/weapon_workshop.bmp
        assets/ui/houses/icons/weapon_workshop.bmp

    FILE
        assets/ui/houses/icons/wineyard.bmp
        assets/ui/houses/icons/wineyard.bmp

    FILE
        assets/ui/houses/icons/woodcutter.bmp
        assets/ui/houses/icons/woodcutter.bmp

    FILE
        assets/ui/build/actions/road.bmp
        assets/ui/build/actions/road.bmp

    FILE
        assets/ui/build/actions/wine.bmp
        assets/ui/build/actions/wine.bmp

    FILE
        assets/ui/build/actions/field.bmp
        assets/ui/build/actions/field.bmp

    FILE
        assets/ui/build/actions/cancel.bmp
        assets/ui/build/actions/cancel.bmp

    FILE
        assets/ui/build/wares/stone.bmp
        assets/ui/build/wares/stone.bmp

    FILE
        assets/ui/build/wares/wood.bmp
        assets/ui/build/wares/wood.bmp

    FILE
        assets/ui/build/controls/unavailable.bmp
        assets/ui/build/controls/unavailable.bmp

    FILE
        assets/ui/build/controls/road.bmp
        assets/ui/build/controls/road.bmp

    FILE
        assets/ui/build/controls/field.bmp
        assets/ui/build/controls/field.bmp

    FILE
        assets/ui/build/controls/wine.bmp
        assets/ui/build/controls/wine.bmp

    FILE
        assets/ui/build/controls/cancel.bmp
        assets/ui/build/controls/cancel.bmp

    FILE
        assets/ui/build/controls/stone.bmp
        assets/ui/build/controls/stone.bmp

    FILE
        assets/ui/build/controls/wood.bmp
        assets/ui/build/controls/wood.bmp

    FILE
        assets/ui/build/tools/field.bmp
        assets/ui/build/tools/field.bmp

    FILE
        assets/ui/build/tools/wine.bmp
        assets/ui/build/tools/wine.bmp

    FILE
        assets/ui/build/tools/remove.bmp
        assets/ui/build/tools/remove.bmp

    FILE
        assets/ui/build/costs/wood.bmp
        assets/ui/build/costs/wood.bmp

    FILE
        assets/ui/build/costs/stone.bmp
        assets/ui/build/costs/stone.bmp

    FILE
        assets/ui/build/text/road.bmp
        assets/ui/build/text/road.bmp

    FILE
        assets/ui/build/text/store.bmp
        assets/ui/build/text/store.bmp

    FILE
        assets/ui/build/text/bakery.bmp
        assets/ui/build/text/bakery.bmp

    FILE
        assets/ui/build/text/farm.bmp
        assets/ui/build/text/farm.bmp

    FILE
        assets/ui/build/text/school.bmp
        assets/ui/build/text/school.bmp

    FILE
        assets/ui/build/text/field.bmp
        assets/ui/build/text/field.bmp

    FILE
        assets/ui/build/text/wine.bmp
        assets/ui/build/text/wine.bmp

    FILE
        assets/ui/build/text/houses/armor_smithy.bmp
        assets/ui/build/text/houses/armor_smithy.bmp

    FILE
        assets/ui/build/text/houses/armor_workshop.bmp
        assets/ui/build/text/houses/armor_workshop.bmp

    FILE
        assets/ui/build/text/houses/bakery.bmp
        assets/ui/build/text/houses/bakery.bmp

    FILE
        assets/ui/build/text/houses/barracks.bmp
        assets/ui/build/text/houses/barracks.bmp

    FILE
        assets/ui/build/text/houses/butchers.bmp
        assets/ui/build/text/houses/butchers.bmp

    FILE
        assets/ui/build/text/houses/coal_mine.bmp
        assets/ui/build/text/houses/coal_mine.bmp

    FILE
        assets/ui/build/text/houses/farm.bmp
        assets/ui/build/text/houses/farm.bmp

    FILE
        assets/ui/build/text/houses/fisher_hut.bmp
        assets/ui/build/text/houses/fisher_hut.bmp

    FILE
        assets/ui/build/text/houses/gold_mine.bmp
        assets/ui/build/text/houses/gold_mine.bmp

    FILE
        assets/ui/build/text/houses/inn.bmp
        assets/ui/build/text/houses/inn.bmp

    FILE
        assets/ui/build/text/houses/iron_mine.bmp
        assets/ui/build/text/houses/iron_mine.bmp

    FILE
        assets/ui/build/text/houses/iron_smithy.bmp
        assets/ui/build/text/houses/iron_smithy.bmp

    FILE
        assets/ui/build/text/houses/marketplace.bmp
        assets/ui/build/text/houses/marketplace.bmp

    FILE
        assets/ui/build/text/houses/metallurgist.bmp
        assets/ui/build/text/houses/metallurgist.bmp

    FILE
        assets/ui/build/text/houses/mill.bmp
        assets/ui/build/text/houses/mill.bmp

    FILE
        assets/ui/build/text/houses/quarry.bmp
        assets/ui/build/text/houses/quarry.bmp

    FILE
        assets/ui/build/text/houses/sawmill.bmp
        assets/ui/build/text/houses/sawmill.bmp

    FILE
        assets/ui/build/text/houses/school.bmp
        assets/ui/build/text/houses/school.bmp

    FILE
        assets/ui/build/text/houses/siege_workshop.bmp
        assets/ui/build/text/houses/siege_workshop.bmp

    FILE
        assets/ui/build/text/houses/stables.bmp
        assets/ui/build/text/houses/stables.bmp

    FILE
        assets/ui/build/text/houses/store.bmp
        assets/ui/build/text/houses/store.bmp

    FILE
        assets/ui/build/text/houses/swine.bmp
        assets/ui/build/text/houses/swine.bmp

    FILE
        assets/ui/build/text/houses/tannery.bmp
        assets/ui/build/text/houses/tannery.bmp

    FILE
        assets/ui/build/text/houses/town_hall.bmp
        assets/ui/build/text/houses/town_hall.bmp

    FILE
        assets/ui/build/text/houses/watch_tower.bmp
        assets/ui/build/text/houses/watch_tower.bmp

    FILE
        assets/ui/build/text/houses/weapon_smithy.bmp
        assets/ui/build/text/houses/weapon_smithy.bmp

    FILE
        assets/ui/build/text/houses/weapon_workshop.bmp
        assets/ui/build/text/houses/weapon_workshop.bmp

    FILE
        assets/ui/build/text/houses/wineyard.bmp
        assets/ui/build/text/houses/wineyard.bmp

    FILE
        assets/ui/build/text/houses/woodcutter.bmp
        assets/ui/build/text/houses/woodcutter.bmp

    FILE
        assets/ui/build/text/demolish.bmp
        assets/ui/build/text/demolish.bmp

    FILE
        assets/terrain/fields/runtime/corn_empty.bmp
        assets/terrain/fields/runtime/corn_empty.bmp

    FILE
        assets/terrain/fields/runtime/corn_sown.bmp
        assets/terrain/fields/runtime/corn_sown.bmp

    FILE
        assets/terrain/fields/runtime/corn_young.bmp
        assets/terrain/fields/runtime/corn_young.bmp

    FILE
        assets/terrain/fields/runtime/corn_ground.bmp
        assets/terrain/fields/runtime/corn_ground.bmp

    FILE
        assets/terrain/fields/runtime/corn_cut.bmp
        assets/terrain/fields/runtime/corn_cut.bmp

    FILE
        assets/terrain/fields/runtime/wine_ground.bmp
        assets/terrain/fields/runtime/wine_ground.bmp

)
