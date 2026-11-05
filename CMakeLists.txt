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

set(VITA_APP_NAME "KaM Vita DataRuntime")
set(VITA_TITLEID "KAMV00002")
set(VITA_VERSION "00.01")

find_package(SDL2 REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/original_kam/original_kam_resources.cpp
    src/render/terrain.cpp
    src/render/kam_plan_renderer.cpp
    src/game/buildings.cpp
    src/game/house_plan_catalog.cpp
    src/game/units.cpp
    src/game/save_units_v11.cpp
    src/game/save_slots.cpp
    src/game/ware_distribution.cpp
    src/ui/game_ui.cpp
    src/ui/game_ui_unit.cpp
    src/ui/house_icons.cpp
    src/ui/game_ui_statistics.cpp
    src/ui/game_ui_ratios.cpp
    src/ui/kam_runtime_font.cpp
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

# ============================================================
# KAMVITA VITA ELF SIZE OPTIMIZATION
#
# Vita ELF ma RX a RW LOAD segmenty zarovnane na 0x10000.
# vita-elf-create potrebuje na konci RX segmentu prostor pro
# SCE metadata.
#
# main.cpp a game_ui.cpp jsou nejvetsi KaMVita orchestration
# / UI translation units. Pro tyto dva soubory pouzivame -Os,
# aby RX segment zustal pod hranici dalsiho LOAD segmentu.
#
# Jde pouze o compile-time size optimalizaci. Gameplay ani
# GUI logika se tim nemeni.
# ============================================================

set_source_files_properties(
    src/main.cpp
    src/ui/game_ui.cpp
    src/game/save_slots.cpp
    src/game/buildings.cpp
    src/game/units.cpp
    PROPERTIES
        COMPILE_OPTIONS "-Os"
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
        assets/buildings/construction/quarry/wood_atlas.bmp
        assets/buildings/construction/quarry/wood_atlas.bmp
    FILE
        assets/buildings/construction/quarry/stone_atlas.bmp
        assets/buildings/construction/quarry/stone_atlas.bmp
    FILE
        assets/buildings/construction/mill/wood_atlas.bmp
        assets/buildings/construction/mill/wood_atlas.bmp
    FILE
        assets/buildings/construction/mill/stone_atlas.bmp
        assets/buildings/construction/mill/stone_atlas.bmp
    FILE
        assets/buildings/mill/runtime/turbine_atlas.bmp
        assets/buildings/mill/runtime/turbine_atlas.bmp
    FILE
        assets/buildings/mill/runtime/idle_atlas.bmp
        assets/buildings/mill/runtime/idle_atlas.bmp
    FILE
        assets/buildings/tablets/original/quarry_265.bmp
        assets/buildings/tablets/original/quarry_265.bmp
    FILE
        assets/buildings/tablets/original/mill_273.bmp
        assets/buildings/tablets/original/mill_273.bmp

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
        assets/units/serf/complete/actions/walk_arm_atlas.bmp
        assets/units/serf/complete/actions/walk_arm_atlas.bmp

    FILE
        assets/units/serf/carry_stone_atlas.bmp
        assets/units/serf/carry_stone_atlas.bmp

    FILE
        assets/units/serf/carry_gold_atlas.bmp
        assets/units/serf/carry_gold_atlas.bmp

    FILE
        assets/units/serf/carry_flour_atlas.bmp
        assets/units/serf/carry_flour_atlas.bmp

    FILE
        assets/units/serf/carry_bread_atlas.bmp
        assets/units/serf/carry_bread_atlas.bmp

    FILE
        assets/units/serf/carry_corn_atlas.bmp
        assets/units/serf/carry_corn_atlas.bmp

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
        assets/units/thoughts/eat_atlas.bmp
        assets/units/thoughts/eat_atlas.bmp
    FILE
        assets/units/thoughts/death_atlas.bmp
        assets/units/thoughts/death_atlas.bmp
    FILE
        assets/units/thoughts/quest_atlas.bmp
        assets/units/thoughts/quest_atlas.bmp
    FILE
        assets/units/baker/complete/actions/walk_atlas.bmp
        assets/units/baker/complete/actions/walk_atlas.bmp

    FILE
        assets/units/baker/complete/actions/work_atlas.bmp
        assets/units/baker/complete/actions/work_atlas.bmp

    FILE
        assets/units/baker/complete/actions/eat_atlas.bmp
        assets/units/baker/complete/actions/eat_atlas.bmp

    FILE
        assets/units/baker/complete/actions/die_atlas.bmp
        assets/units/baker/complete/actions/die_atlas.bmp

    FILE
        assets/units/farmer/complete/actions/walk_atlas.bmp
        assets/units/farmer/complete/actions/walk_atlas.bmp

    FILE
        assets/units/farmer/complete/actions/work_atlas.bmp
        assets/units/farmer/complete/actions/work_atlas.bmp

    FILE
        assets/units/farmer/complete/actions/work1_atlas.bmp
        assets/units/farmer/complete/actions/work1_atlas.bmp

    FILE
        assets/units/farmer/complete/actions/walktool_atlas.bmp
        assets/units/farmer/complete/actions/walktool_atlas.bmp

    FILE
        assets/units/farmer/complete/actions/walkbooty_atlas.bmp
        assets/units/farmer/complete/actions/walkbooty_atlas.bmp

    FILE
        assets/units/farmer/complete/actions/eat_atlas.bmp
        assets/units/farmer/complete/actions/eat_atlas.bmp

    FILE
        assets/units/farmer/complete/actions/die_atlas.bmp
        assets/units/farmer/complete/actions/die_atlas.bmp

    FILE
        assets/units/worker/complete/actions/eat_atlas.bmp
        assets/units/worker/complete/actions/eat_atlas.bmp
    FILE
        assets/units/worker/complete/actions/die_atlas.bmp
        assets/units/worker/complete/actions/die_atlas.bmp
    FILE
        assets/units/serf/complete/actions/eat_atlas.bmp
        assets/units/serf/complete/actions/eat_atlas.bmp
    FILE
        assets/units/serf/complete/actions/die_atlas.bmp
        assets/units/serf/complete/actions/die_atlas.bmp
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
        assets/buildings/bakery/runtime/bread_supply_atlas.bmp
        assets/buildings/bakery/runtime/bread_supply_atlas.bmp

    FILE
        assets/buildings/bakery/runtime/smoke_atlas.bmp
        assets/buildings/bakery/runtime/smoke_atlas.bmp

    FILE
        assets/buildings/store/runtime/flag1_atlas.bmp
        assets/buildings/store/runtime/flag1_atlas.bmp

    FILE
        assets/buildings/store/runtime/flag2_atlas.bmp
        assets/buildings/store/runtime/flag2_atlas.bmp

    FILE
        assets/buildings/bakery/runtime/flagpole.bmp
        assets/buildings/bakery/runtime/flagpole.bmp
    FILE
        assets/buildings/bakery/runtime/flag1_atlas.bmp
        assets/buildings/bakery/runtime/flag1_atlas.bmp
    FILE
        assets/buildings/bakery/runtime/idle_atlas.bmp
        assets/buildings/bakery/runtime/idle_atlas.bmp

    FILE
        assets/buildings/farm/runtime/idle_atlas.bmp
        assets/buildings/farm/runtime/idle_atlas.bmp

    FILE
        assets/buildings/farm/runtime/flag1_atlas.bmp
        assets/buildings/farm/runtime/flag1_atlas.bmp

    FILE
        assets/buildings/farm/runtime/corn_supply_atlas.bmp
        assets/buildings/farm/runtime/corn_supply_atlas.bmp

    FILE
        assets/buildings/bakery/runtime/work2_atlas.bmp
        assets/buildings/bakery/runtime/work2_atlas.bmp

    FILE
        assets/buildings/bakery/runtime/work3_atlas.bmp
        assets/buildings/bakery/runtime/work3_atlas.bmp

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
        assets/ui/bakery/text/flour.bmp
        assets/ui/bakery/text/flour.bmp

    FILE
        assets/ui/bakery/text/loaves.bmp
        assets/ui/bakery/text/loaves.bmp

    FILE
        assets/ui/bakery/text/delivers_x2.bmp
        assets/ui/bakery/text/delivers_x2.bmp

    FILE
        assets/ui/farm/text/delivers_x1.bmp
        assets/ui/farm/text/delivers_x1.bmp

    FILE
        assets/ui/farm/text/corn.bmp
        assets/ui/farm/text/corn.bmp

    # Runtime Metal font atlas for dynamic Save names

    FILE
        assets/ui/options/runtime_font/metal_ascii.bmp
        assets/ui/options/runtime_font/metal_ascii.bmp

    FILE
        assets/ui/options/runtime_font/metal_ascii.json
        assets/ui/options/runtime_font/metal_ascii.json

    # Original KaM Remake Settings fnt_Outline headings

    FILE
        assets/ui/settings/text/graphics.bmp
        assets/ui/settings/text/graphics.bmp

    FILE
        assets/ui/settings/text/controls.bmp
        assets/ui/settings/text/controls.bmp

    # Original KaM Remake Settings middle headings

    FILE
        assets/ui/settings/text/sound.bmp
        assets/ui/settings/text/sound.bmp

    FILE
        assets/ui/settings/text/gameplay.bmp
        assets/ui/settings/text/gameplay.bmp

    # Original KaM Remake Settings popup assets

    FILE
        assets/ui/settings/original/guimain_0015.bmp
        assets/ui/settings/original/guimain_0015.bmp

    FILE
        assets/ui/settings/original/guimain_0009.bmp
        assets/ui/settings/original/guimain_0009.bmp

    # Original KaM Remake Options ENG text

    FILE
        assets/ui/options/text/load_game.bmp
        assets/ui/options/text/load_game.bmp

    FILE
        assets/ui/options/text/save_game.bmp
        assets/ui/options/text/save_game.bmp

    FILE
        assets/ui/options/text/settings.bmp
        assets/ui/options/text/settings.bmp

    FILE
        assets/ui/options/text/quit_mission.bmp
        assets/ui/options/text/quit_mission.bmp

    FILE
        assets/ui/options/text/game_time.bmp
        assets/ui/options/text/game_time.bmp

    FILE
        assets/ui/options/text/map.bmp
        assets/ui/options/text/map.bmp

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
        assets/terrain/fields/runtime/corn_obj58_sprite61.bmp
        assets/terrain/fields/runtime/corn_obj58_sprite61.bmp
        assets/terrain/fields/runtime/corn_obj58_sprite62.bmp
        assets/terrain/fields/runtime/corn_obj58_sprite62.bmp
        assets/terrain/fields/runtime/corn_obj58_sprite63.bmp
        assets/terrain/fields/runtime/corn_obj58_sprite63.bmp
        assets/terrain/fields/runtime/corn_obj59_sprite64.bmp
        assets/terrain/fields/runtime/corn_obj59_sprite64.bmp
        assets/terrain/fields/runtime/corn_obj59_sprite65.bmp
        assets/terrain/fields/runtime/corn_obj59_sprite65.bmp
        assets/terrain/fields/runtime/corn_obj59_sprite66.bmp
        assets/terrain/fields/runtime/corn_obj59_sprite66.bmp

    FILE
        assets/terrain/fields/runtime/corn_cut.bmp
        assets/terrain/fields/runtime/corn_cut.bmp

    FILE
        assets/terrain/fields/runtime/wine_ground.bmp
        assets/terrain/fields/runtime/wine_ground.bmp

    FILE
        assets/terrain/fields/fences/runtime/corn_horizontal.bmp
        assets/terrain/fields/fences/runtime/corn_horizontal.bmp

    FILE
        assets/terrain/fields/fences/runtime/corn_vertical.bmp
        assets/terrain/fields/fences/runtime/corn_vertical.bmp

    FILE
        assets/terrain/fields/fences/runtime/wine_horizontal.bmp
        assets/terrain/fields/fences/runtime/wine_horizontal.bmp

    FILE
        assets/terrain/fields/fences/runtime/wine_vertical.bmp
        assets/terrain/fields/fences/runtime/wine_vertical.bmp

    FILE
        assets/terrain/plans/original/road_105.bmp
        assets/terrain/plans/original/road_105.bmp

    FILE
        assets/terrain/plans/original/field_107.bmp
        assets/terrain/plans/original/field_107.bmp

    FILE
        assets/terrain/plans/original/wine_108.bmp
        assets/terrain/plans/original/wine_108.bmp

    FILE
        assets/buildings/plans/original/house_horizontal_105.bmp
        assets/buildings/plans/original/house_horizontal_105.bmp

    FILE
        assets/buildings/plans/original/house_vertical_117.bmp
        assets/buildings/plans/original/house_vertical_117.bmp

    FILE
        assets/buildings/plans/original/house_wip_horizontal_463.bmp
        assets/buildings/plans/original/house_wip_horizontal_463.bmp

    FILE
        assets/buildings/plans/original/house_wip_vertical_467.bmp
        assets/buildings/plans/original/house_wip_vertical_467.bmp

    FILE
        assets/buildings/tablets/original/store_262.bmp
        assets/buildings/tablets/original/store_262.bmp

    FILE
        assets/buildings/tablets/original/bakery_258.bmp
        assets/buildings/tablets/original/bakery_258.bmp

    FILE
        assets/buildings/tablets/original/farm_259.bmp
        assets/buildings/tablets/original/farm_259.bmp

    FILE
        assets/buildings/tablets/original/school_264.bmp
        assets/buildings/tablets/original/school_264.bmp

    FILE
        assets/ui/units/back_443.bmp
        assets/ui/units/back_443.bmp

    FILE
        assets/ui/units/text/condition.bmp
        assets/ui/units/text/condition.bmp

    FILE
        assets/ui/units/text/idle.bmp
        assets/ui/units/text/idle.bmp

    FILE
        assets/ui/units/text/walking.bmp
        assets/ui/units/text/walking.bmp

    FILE
        assets/ui/units/text/road.bmp
        assets/ui/units/text/road.bmp

    FILE
        assets/ui/units/text/field.bmp
        assets/ui/units/text/field.bmp

    FILE
        assets/ui/units/text/wine.bmp
        assets/ui/units/text/wine.bmp

    FILE
        assets/ui/units/text/house_site.bmp
        assets/ui/units/text/house_site.bmp

    FILE
        assets/ui/units/text/house.bmp
        assets/ui/units/text/house.bmp

    FILE
        assets/ui/units/text/repair.bmp
        assets/ui/units/text/repair.bmp

    FILE
        assets/ui/units/text/serf_description.bmp
        assets/ui/units/text/serf_description.bmp

    FILE
        assets/ui/units/text/worker_description.bmp
        assets/ui/units/text/worker_description.bmp

    FILE
        assets/ui/units/text/farmer_description.bmp
        assets/ui/units/text/farmer_description.bmp

    FILE
        assets/ui/units/text/baker_description.bmp
        assets/ui/units/text/baker_description.bmp

    # Originalni KaM Store GUI 351..378

    FILE
        assets/ui/store/wares/gui_0351.bmp
        assets/ui/store/wares/gui_0351.bmp

    FILE
        assets/ui/store/wares/gui_0352.bmp
        assets/ui/store/wares/gui_0352.bmp

    FILE
        assets/ui/store/wares/gui_0353.bmp
        assets/ui/store/wares/gui_0353.bmp

    FILE
        assets/ui/store/wares/gui_0354.bmp
        assets/ui/store/wares/gui_0354.bmp

    FILE
        assets/ui/store/wares/gui_0355.bmp
        assets/ui/store/wares/gui_0355.bmp

    FILE
        assets/ui/store/wares/gui_0356.bmp
        assets/ui/store/wares/gui_0356.bmp

    FILE
        assets/ui/store/wares/gui_0357.bmp
        assets/ui/store/wares/gui_0357.bmp

    FILE
        assets/ui/store/wares/gui_0358.bmp
        assets/ui/store/wares/gui_0358.bmp

    FILE
        assets/ui/store/wares/gui_0359.bmp
        assets/ui/store/wares/gui_0359.bmp

    FILE
        assets/ui/store/wares/gui_0360.bmp
        assets/ui/store/wares/gui_0360.bmp

    FILE
        assets/ui/store/wares/gui_0361.bmp
        assets/ui/store/wares/gui_0361.bmp

    FILE
        assets/ui/store/wares/gui_0362.bmp
        assets/ui/store/wares/gui_0362.bmp

    FILE
        assets/ui/store/wares/gui_0363.bmp
        assets/ui/store/wares/gui_0363.bmp

    FILE
        assets/ui/store/wares/gui_0364.bmp
        assets/ui/store/wares/gui_0364.bmp

    FILE
        assets/ui/store/wares/gui_0365.bmp
        assets/ui/store/wares/gui_0365.bmp

    FILE
        assets/ui/store/wares/gui_0366.bmp
        assets/ui/store/wares/gui_0366.bmp

    FILE
        assets/ui/store/wares/gui_0367.bmp
        assets/ui/store/wares/gui_0367.bmp

    FILE
        assets/ui/store/wares/gui_0368.bmp
        assets/ui/store/wares/gui_0368.bmp

    FILE
        assets/ui/store/wares/gui_0369.bmp
        assets/ui/store/wares/gui_0369.bmp

    FILE
        assets/ui/store/wares/gui_0370.bmp
        assets/ui/store/wares/gui_0370.bmp

    FILE
        assets/ui/store/wares/gui_0371.bmp
        assets/ui/store/wares/gui_0371.bmp

    FILE
        assets/ui/store/wares/gui_0372.bmp
        assets/ui/store/wares/gui_0372.bmp

    FILE
        assets/ui/store/wares/gui_0373.bmp
        assets/ui/store/wares/gui_0373.bmp

    FILE
        assets/ui/store/wares/gui_0374.bmp
        assets/ui/store/wares/gui_0374.bmp

    FILE
        assets/ui/store/wares/gui_0375.bmp
        assets/ui/store/wares/gui_0375.bmp

    FILE
        assets/ui/store/wares/gui_0376.bmp
        assets/ui/store/wares/gui_0376.bmp

    FILE
        assets/ui/store/wares/gui_0377.bmp
        assets/ui/store/wares/gui_0377.bmp

    FILE
        assets/ui/store/wares/gui_0378.bmp
        assets/ui/store/wares/gui_0378.bmp

    # Originalni KaM Store accept marker GUI 49
    FILE
        assets/ui/store/controls/accept_flag.bmp
        assets/ui/store/controls/accept_flag.bmp

)
