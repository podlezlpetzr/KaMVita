#pragma once

#include <SDL2/SDL.h>


// Kompletní katalog 29 House GUI ikon z KaM Remake.
//
// Hodnoty odpovídají jedinému centrálnímu pořadí
// používanému HouseIconResources::icons[].
//
// Tento enum je záměrně oddělený od BuildingType:
// - HouseIconType obsahuje všechny autentické ikony,
// - BuildingType obsahuje pouze skutečně implementované
//   budovy KaMVita.
enum class HouseIconType {
    ArmorSmithy = 0,
    ArmorWorkshop,
    Bakery,
    Barracks,
    Butchers,
    CoalMine,
    Farm,
    FisherHut,
    GoldMine,
    Inn,
    IronMine,
    IronSmithy,
    Marketplace,
    Metallurgist,
    Mill,
    Quarry,
    Sawmill,
    School,
    SiegeWorkshop,
    Stables,
    Store,
    Swine,
    Tannery,
    TownHall,
    WatchTower,
    WeaponSmithy,
    WeaponWorkshop,
    Wineyard,
    Woodcutter,

    Count
};


static constexpr int HOUSE_ICON_COUNT =
    static_cast<int>(
        HouseIconType::Count
    );


// Originální KaM Remake Build menu zobrazuje
// 27 budov v GUIHouseOrder.
//
// TownHall a SiegeWorkshop jsou součástí úplného
// katalogu 29 ikon, ale nejsou v tomto Build pořadí.
static constexpr int GUI_HOUSE_COUNT = 27;


struct HouseIconResources {
    SDL_Texture* icons[HOUSE_ICON_COUNT];
};


// Originální pořadí House tlačítek z:
//
// KM_InterfaceGame.pas
// GUIHouseOrder[1..27]
extern const HouseIconType
    GUI_HOUSE_ORDER[GUI_HOUSE_COUNT];


// Načte všech 29 originálních House GUI ikon.
//
// Jednotlivá chybějící textura zůstane nullptr.
// Funkce vrací resource objekt vždy v bezpečně
// inicializovaném stavu.
HouseIconResources loadHouseIconResources(
    SDL_Renderer* renderer
);


// Uvolní všechny načtené House textury
// a nastaví jejich ukazatele na nullptr.
void destroyHouseIconResources(
    HouseIconResources* resources
);


// Bezpečný přístup k ikoně.
// Při chybném typu vrátí nullptr.
SDL_Texture* getHouseIcon(
    const HouseIconResources& resources,
    HouseIconType type
);


// Vrátí stabilní interní název používaný
// pro cesty assetů a diagnostiku.
const char* getHouseIconName(
    HouseIconType type
);
