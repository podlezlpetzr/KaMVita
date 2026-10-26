#include "terrain.h"

SDL_Texture* loadTexture(
    SDL_Renderer* renderer,
    const char* path
)
{
    SDL_Surface* surface = SDL_LoadBMP(path);

    if (!surface)
        return nullptr;

    SDL_Texture* texture =
        SDL_CreateTextureFromSurface(renderer, surface);

    SDL_FreeSurface(surface);

    return texture;
}

static SDL_Texture* loadOverlayTexture(
    SDL_Renderer* renderer,
    const char* path
)
{
    SDL_Surface* surface = SDL_LoadBMP(path);

    if (!surface)
        return nullptr;

    // KaM overlay tiles pouzivaji jednobarevne
    // pozadi jako pruhlednou cast obrazku.
    // Barvu vezmeme z leveho horniho pixelu,
    // takze nemusime napevno predpokladat RGB.
    Uint32 transparentPixel = 0;

    SDL_LockSurface(surface);

    const Uint8* firstPixel =
        static_cast<const Uint8*>(
            surface->pixels
        );

    switch (surface->format->BytesPerPixel) {
        case 1:
            transparentPixel =
                *firstPixel;
            break;

        case 2:
            transparentPixel =
                *reinterpret_cast<const Uint16*>(
                    firstPixel
                );
            break;

        case 3:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
            transparentPixel =
                firstPixel[0] << 16 |
                firstPixel[1] << 8 |
                firstPixel[2];
#else
            transparentPixel =
                firstPixel[0] |
                firstPixel[1] << 8 |
                firstPixel[2] << 16;
#endif
            break;

        case 4:
            transparentPixel =
                *reinterpret_cast<const Uint32*>(
                    firstPixel
                );
            break;
    }

    SDL_UnlockSurface(surface);

    SDL_SetColorKey(
        surface,
        SDL_TRUE,
        transparentPixel
    );

    SDL_Texture* texture =
        SDL_CreateTextureFromSurface(
            renderer,
            surface
        );

    SDL_FreeSurface(surface);

    if (texture) {
        SDL_SetTextureBlendMode(
            texture,
            SDL_BLENDMODE_BLEND
        );
    }

    return texture;
}


static SDL_Texture* loadRoadTexture(
    SDL_Renderer* renderer,
    const char* path
)
{
    SDL_Surface* surface = SDL_LoadBMP(path);

    if (!surface)
        return nullptr;

    // Originalni road overlay pouziva cervenou
    // jako pruhledne pozadi.
    SDL_SetColorKey(
        surface,
        SDL_TRUE,
        SDL_MapRGB(surface->format, 255, 0, 0)
    );

    SDL_Texture* texture =
        SDL_CreateTextureFromSurface(
            renderer,
            surface
        );

    SDL_FreeSurface(surface);

    if (texture) {
        SDL_SetTextureBlendMode(
            texture,
            SDL_BLENDMODE_BLEND
        );
    }

    return texture;
}

TerrainResources loadTerrainResources(SDL_Renderer* renderer)
{
    TerrainResources terrain = {};

    terrain.grass =
        loadTexture(renderer, "app0:assets/terrain/grass.bmp");

    terrain.water =
        loadTexture(renderer, "app0:assets/terrain/water.bmp");

    terrain.dirt =
        loadTexture(renderer, "app0:assets/terrain/dirt.bmp");

    terrain.sand =
        loadTexture(renderer, "app0:assets/terrain/sand.bmp");

    terrain.roadDig1 = loadOverlayTexture(
        renderer,
        "app0:assets/terrain/roads/original/dig1.bmp"
    );

    terrain.roadDig2 = loadOverlayTexture(
        renderer,
        "app0:assets/terrain/roads/original/dig2.bmp"
    );

    terrain.roadDig3 = loadOverlayTexture(
        renderer,
        "app0:assets/terrain/roads/original/dig3.bmp"
    );

    terrain.roadDig4 = loadOverlayTexture(
        renderer,
        "app0:assets/terrain/roads/original/dig4.bmp"
    );

    terrain.road248 = loadOverlayTexture(
        renderer,
        "app0:assets/terrain/roads/original/road_248.bmp"
    );

    terrain.road250 = loadOverlayTexture(
        renderer,
        "app0:assets/terrain/roads/original/road_250.bmp"
    );

    terrain.road252 = loadOverlayTexture(
        renderer,
        "app0:assets/terrain/roads/original/road_252.bmp"
    );

    terrain.road254 = loadOverlayTexture(
        renderer,
        "app0:assets/terrain/roads/original/road_254.bmp"
    );

    return terrain;
}

void destroyTerrainResources(TerrainResources* terrain)
{
    if (!terrain)
        return;

    if (terrain->grass)
        SDL_DestroyTexture(terrain->grass);

    if (terrain->water)
        SDL_DestroyTexture(terrain->water);

    if (terrain->dirt)
        SDL_DestroyTexture(terrain->dirt);

    if (terrain->sand)
        SDL_DestroyTexture(terrain->sand);

    if (terrain->roadDig1)
        SDL_DestroyTexture(terrain->roadDig1);

    if (terrain->roadDig2)
        SDL_DestroyTexture(terrain->roadDig2);

    if (terrain->roadDig3)
        SDL_DestroyTexture(terrain->roadDig3);

    if (terrain->roadDig4)
        SDL_DestroyTexture(terrain->roadDig4);

    if (terrain->road248)
        SDL_DestroyTexture(terrain->road248);

    if (terrain->road250)
        SDL_DestroyTexture(terrain->road250);

    if (terrain->road252)
        SDL_DestroyTexture(terrain->road252);

    if (terrain->road254)
        SDL_DestroyTexture(terrain->road254);

    terrain->grass = nullptr;
    terrain->water = nullptr;
    terrain->dirt = nullptr;
    terrain->sand = nullptr;

    terrain->roadDig1 = nullptr;
    terrain->roadDig2 = nullptr;
    terrain->roadDig3 = nullptr;
    terrain->roadDig4 = nullptr;
    terrain->road248 = nullptr;
    terrain->road250 = nullptr;
    terrain->road252 = nullptr;
    terrain->road254 = nullptr;
}

void renderTerrain(
    SDL_Renderer* renderer,
    const TerrainResources& terrain,
    float cameraX,
    float cameraY,
    int screenWidth,
    int screenHeight,
    int tileSize,
    int mapWidth,
    int mapHeight
)
{
    int startTileX = (int)cameraX / tileSize;
    int startTileY = (int)cameraY / tileSize;

    int endTileX =
        ((int)cameraX + screenWidth) / tileSize + 2;

    int endTileY =
        ((int)cameraY + screenHeight) / tileSize + 2;

    if (startTileX < 0)
        startTileX = 0;

    if (startTileY < 0)
        startTileY = 0;

    if (endTileX > mapWidth)
        endTileX = mapWidth;

    if (endTileY > mapHeight)
        endTileY = mapHeight;

    for (int y = startTileY; y < endTileY; ++y) {
        for (int x = startTileX; x < endTileX; ++x) {
            SDL_Rect tile = {
                x * tileSize - (int)cameraX,
                y * tileSize - (int)cameraY,
                tileSize,
                tileSize
            };

            SDL_Texture* terrainTexture = terrain.grass;

            if (x < 10 && y < 12) {
                terrainTexture = terrain.water;
            } else if (
                x < 13 &&
                y < 15 &&
                (x >= 10 || y >= 12)
            ) {
                terrainTexture = terrain.sand;
            } else if (
                x >= 24 &&
                x <= 39 &&
                y >= 16 &&
                y <= 30
            ) {
                terrainTexture = terrain.dirt;
            }

            if (terrainTexture) {
                SDL_RenderCopy(
                    renderer,
                    terrainTexture,
                    nullptr,
                    &tile
                );
            }
        }
    }
}
