#include "house_icons.h"

#include <cstdio>


const HouseIconType
    GUI_HOUSE_ORDER[GUI_HOUSE_COUNT] = {
        // Row 1
        HouseIconType::School,
        HouseIconType::Inn,
        HouseIconType::Quarry,
        HouseIconType::Woodcutter,
        HouseIconType::Sawmill,

        // Row 2
        HouseIconType::Farm,
        HouseIconType::Mill,
        HouseIconType::Bakery,
        HouseIconType::Swine,
        HouseIconType::Butchers,

        // Row 3
        HouseIconType::Wineyard,
        HouseIconType::GoldMine,
        HouseIconType::CoalMine,
        HouseIconType::Metallurgist,
        HouseIconType::WeaponWorkshop,

        // Row 4
        HouseIconType::Tannery,
        HouseIconType::ArmorWorkshop,
        HouseIconType::Stables,
        HouseIconType::IronMine,
        HouseIconType::IronSmithy,

        // Row 5
        HouseIconType::WeaponSmithy,
        HouseIconType::ArmorSmithy,
        HouseIconType::Barracks,
        HouseIconType::Store,
        HouseIconType::WatchTower,

        // Row 6
        HouseIconType::FisherHut,
        HouseIconType::Marketplace
    };


namespace
{

static const char* HOUSE_ICON_NAMES[
    HOUSE_ICON_COUNT
] = {
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
    "woodcutter"
};


static_assert(
    sizeof(HOUSE_ICON_NAMES) /
        sizeof(HOUSE_ICON_NAMES[0]) ==
        HOUSE_ICON_COUNT,
    "House icon name catalog must contain 29 entries"
);


bool isValidHouseIconIndex(int index)
{
    return
        index >= 0 &&
        index < HOUSE_ICON_COUNT;
}


SDL_Texture* loadHouseIconTexture(
    SDL_Renderer* renderer,
    const char* path
)
{
    if (!renderer || !path)
        return nullptr;

    SDL_Surface* surface =
        SDL_LoadBMP(path);

    if (!surface)
        return nullptr;

    // Extrahované KaM sprity používají
    // magentu jako transparentní color key.
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

}


HouseIconResources loadHouseIconResources(
    SDL_Renderer* renderer
)
{
    HouseIconResources resources = {};

    if (!renderer)
        return resources;

    for (
        int iconIndex = 0;
        iconIndex < HOUSE_ICON_COUNT;
        ++iconIndex
    ) {
        char path[256];

        std::snprintf(
            path,
            sizeof(path),
            "app0:assets/ui/houses/icons/%s.bmp",
            HOUSE_ICON_NAMES[iconIndex]
        );

        resources.icons[iconIndex] =
            loadHouseIconTexture(
                renderer,
                path
            );
    }

    return resources;
}


void destroyHouseIconResources(
    HouseIconResources* resources
)
{
    if (!resources)
        return;

    for (
        int iconIndex = 0;
        iconIndex < HOUSE_ICON_COUNT;
        ++iconIndex
    ) {
        if (resources->icons[iconIndex]) {
            SDL_DestroyTexture(
                resources->icons[iconIndex]
            );
        }

        resources->icons[iconIndex] =
            nullptr;
    }
}


SDL_Texture* getHouseIcon(
    const HouseIconResources& resources,
    HouseIconType type
)
{
    const int iconIndex =
        static_cast<int>(type);

    if (!isValidHouseIconIndex(iconIndex))
        return nullptr;

    return resources.icons[iconIndex];
}


const char* getHouseIconName(
    HouseIconType type
)
{
    const int iconIndex =
        static_cast<int>(type);

    if (!isValidHouseIconIndex(iconIndex))
        return "invalid";

    return HOUSE_ICON_NAMES[iconIndex];
}
