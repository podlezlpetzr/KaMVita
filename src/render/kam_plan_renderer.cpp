#include "kam_plan_renderer.h"


static SDL_Texture* loadMagentaTexture(
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

    if (texture) {
        SDL_SetTextureBlendMode(
            texture,
            SDL_BLENDMODE_BLEND
        );
    }

    return texture;
}


KaMPlanResources loadKaMPlanResources(
    SDL_Renderer* renderer
)
{
    KaMPlanResources resources = {};

    resources.roadMarkup =
        loadMagentaTexture(
            renderer,
            "app0:assets/terrain/plans/original/road_105.bmp"
        );

    resources.fieldMarkup =
        loadMagentaTexture(
            renderer,
            "app0:assets/terrain/plans/original/field_107.bmp"
        );

    resources.wineMarkup =
        loadMagentaTexture(
            renderer,
            "app0:assets/terrain/plans/original/wine_108.bmp"
        );

    return resources;
}


void destroyKaMPlanResources(
    KaMPlanResources* resources
)
{
    if (!resources)
        return;

    if (resources->roadMarkup)
        SDL_DestroyTexture(
            resources->roadMarkup
        );

    if (resources->fieldMarkup)
        SDL_DestroyTexture(
            resources->fieldMarkup
        );

    if (resources->wineMarkup)
        SDL_DestroyTexture(
            resources->wineMarkup
        );

    resources->roadMarkup = nullptr;
    resources->fieldMarkup = nullptr;
    resources->wineMarkup = nullptr;
}


void renderKaMMarkup(
    SDL_Renderer* renderer,
    SDL_Texture* texture,
    int tileScreenX,
    int tileScreenY,
    int tileSize
)
{
    if (!renderer || !texture)
        return;

    // Remake RenderMarkup kresli dva diagonalni quady.
    //
    // Originalni souradnice jsou v CELL_SIZE_PX = 40.
    // KaMVita pouziva TILE_SIZE = 32.
    //
    // Lokalni normalizovane body:
    //
    // diagonal 1:
    // (0, 0.10)
    // (0, -0.15)
    // (1, 0.75)
    // (1, 1.00)
    //
    // diagonal 2:
    // (0, 1.00)
    // (0, 0.75)
    // (1, -0.15)
    // (1, 0.10)
    //
    // Na plochem KaMVita terenu neni aplikovana
    // vyska gTerrain.Land[].Height.

    const float x0 =
        (float)tileScreenX;

    const float x1 =
        (float)(tileScreenX + tileSize);

    const float y0 =
        (float)tileScreenY;

    const float scale =
        (float)tileSize;

    const SDL_Color color = {
        255,
        255,
        255,
        255
    };

    SDL_Vertex vertices[8] = {};

    // Prvni diagonala: \

    vertices[0].position = {
        x0,
        y0 + scale * 0.10f
    };

    vertices[1].position = {
        x0,
        y0 - scale * 0.15f
    };

    vertices[2].position = {
        x1,
        y0 + scale * 0.75f
    };

    vertices[3].position = {
        x1,
        y0 + scale * 1.00f
    };

    // Druha diagonala: /

    vertices[4].position = {
        x0,
        y0 + scale * 1.00f
    };

    vertices[5].position = {
        x0,
        y0 + scale * 0.75f
    };

    vertices[6].position = {
        x1,
        y0 - scale * 0.15f
    };

    vertices[7].position = {
        x1,
        y0 + scale * 0.10f
    };

    for (int i = 0; i < 8; ++i) {
        vertices[i].color = color;
    }

    // Stejne UV poradi jako Remake.
    //
    // Zdrojovy sprite je vysoky uzky pruh.
    // UV jej mapuje po cele delce diagonalniho quadu.

    vertices[0].tex_coord = {1.0f, 0.0f};
    vertices[1].tex_coord = {0.0f, 0.0f};
    vertices[2].tex_coord = {0.0f, 1.0f};
    vertices[3].tex_coord = {1.0f, 1.0f};

    vertices[4].tex_coord = {1.0f, 0.0f};
    vertices[5].tex_coord = {0.0f, 0.0f};
    vertices[6].tex_coord = {0.0f, 1.0f};
    vertices[7].tex_coord = {1.0f, 1.0f};

    const int indices[12] = {
        0, 1, 2,
        0, 2, 3,

        4, 5, 6,
        4, 6, 7
    };

    SDL_RenderGeometry(
        renderer,
        texture,
        vertices,
        8,
        indices,
        12
    );
}
