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
    src/ui/game_ui.cpp
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
)
