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


// Ceny puvodnich domu byly overeny primo z:
// resources/original_kam/houses.dat
//
// Verified layout:
// - BeastAnim prefix: 2100 B
// - TKMHouseDat:      1688 B
// - WoodCost:         offset 1528
// - StoneCost:        offset 1529
//
// Marketplace v puvodnim houses.dat neni a pouziva
// Remake override Wood=5, Stone=6.
static const HouseBuildInfo HOUSE_BUILD_INFO[
    HOUSE_ICON_COUNT
] = {
    {
        "armor_smithy",
        "Armor Smithy",
        4,
        3
    },
    {
        "armor_workshop",
        "Armor Workshop",
        4,
        3
    },
    {
        "bakery",
        "Bakery",
        4,
        3
    },
    {
        "barracks",
        "Barracks",
        6,
        6
    },
    {
        "butchers",
        "Butcher's",
        4,
        3
    },
    {
        "coal_mine",
        "Coal Mine",
        3,
        2
    },
    {
        "farm",
        "Farm",
        4,
        3
    },
    {
        "fisher_hut",
        "Fisher's Hut",
        4,
        3
    },
    {
        "gold_mine",
        "Gold Mine",
        3,
        2
    },
    {
        "inn",
        "Inn",
        6,
        5
    },
    {
        "iron_mine",
        "Iron Mine",
        3,
        2
    },
    {
        "iron_smithy",
        "Iron Smithy",
        4,
        3
    },
    {
        "marketplace",
        "Marketplace",
        5,
        6
    },
    {
        "metallurgist",
        "Metallurgist's",
        4,
        3
    },
    {
        "mill",
        "Mill",
        4,
        3
    },
    {
        "quarry",
        "Quarry",
        3,
        2
    },
    {
        "sawmill",
        "Sawmill",
        4,
        3
    },
    {
        "school",
        "School",
        6,
        5
    },
    {
        "siege_workshop",
        "Siege Workshop",
        4,
        2
    },
    {
        "stables",
        "Stables",
        6,
        5
    },
    {
        "store",
        "Store",
        6,
        5
    },
    {
        "swine",
        "Swine Farm",
        4,
        3
    },
    {
        "tannery",
        "Tannery",
        4,
        3
    },
    {
        "town_hall",
        "Town Hall",
        6,
        5
    },
    {
        "watch_tower",
        "Watchtower",
        3,
        2
    },
    {
        "weapon_smithy",
        "Weapon Smithy",
        4,
        3
    },
    {
        "weapon_workshop",
        "Weapon Workshop",
        4,
        3
    },
    {
        "wineyard",
        "Wineyard",
        4,
        3
    },
    {
        "woodcutter",
        "Woodcutter's",
        3,
        2
    }
};


static_assert(
    sizeof(HOUSE_BUILD_INFO) /
        sizeof(HOUSE_BUILD_INFO[0]) ==
        HOUSE_ICON_COUNT,
    "House build metadata must contain 29 entries"
);


static const HouseBuildInfo INVALID_HOUSE_BUILD_INFO = {
    "invalid",
    "Invalid",
    0,
    0
};


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


const HouseBuildInfo& getHouseBuildInfo(
    HouseIconType type
)
{
    const int iconIndex =
        static_cast<int>(type);

    if (!isValidHouseIconIndex(iconIndex)) {
        return INVALID_HOUSE_BUILD_INFO;
    }

    return HOUSE_BUILD_INFO[iconIndex];
}
