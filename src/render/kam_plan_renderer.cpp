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

    resources.housePlanHorizontal =
        loadMagentaTexture(
            renderer,
            "app0:assets/buildings/plans/original/house_horizontal_105.bmp"
        );

    resources.housePlanVertical =
        loadMagentaTexture(
            renderer,
            "app0:assets/buildings/plans/original/house_vertical_117.bmp"
        );

    resources.houseWipHorizontal =
        loadMagentaTexture(
            renderer,
            "app0:assets/buildings/plans/original/house_wip_horizontal_463.bmp"
        );

    resources.houseWipVertical =
        loadMagentaTexture(
            renderer,
            "app0:assets/buildings/plans/original/house_wip_vertical_467.bmp"
        );

    resources.houseTablets[0] =
        loadMagentaTexture(
            renderer,
            "app0:assets/buildings/tablets/original/store_262.bmp"
        );

    resources.houseTablets[1] =
        loadMagentaTexture(
            renderer,
            "app0:assets/buildings/tablets/original/bakery_258.bmp"
        );

    resources.houseTablets[2] =
        loadMagentaTexture(
            renderer,
            "app0:assets/buildings/tablets/original/farm_259.bmp"
        );

    resources.houseTablets[3] =
        loadMagentaTexture(
            renderer,
            "app0:assets/buildings/tablets/original/school_264.bmp"
        );
    resources.houseTablets[4] =
        loadMagentaTexture(
            renderer,
            "app0:assets/buildings/tablets/original/quarry_265.bmp"
        );
    resources.houseTablets[5] =
        loadMagentaTexture(
            renderer,
            "app0:assets/buildings/tablets/original/mill_273.bmp"
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

    if (resources->housePlanHorizontal)
        SDL_DestroyTexture(
            resources->housePlanHorizontal
        );

    if (resources->housePlanVertical)
        SDL_DestroyTexture(
            resources->housePlanVertical
        );

    if (resources->houseWipHorizontal)
        SDL_DestroyTexture(
            resources->houseWipHorizontal
        );

    if (resources->houseWipVertical)
        SDL_DestroyTexture(
            resources->houseWipVertical
        );

    for (int i = 0; i < 6; ++i) {
        if (resources->houseTablets[i]) {
            SDL_DestroyTexture(
                resources->houseTablets[i]
            );
        }
    }

    resources->roadMarkup = nullptr;
    resources->fieldMarkup = nullptr;
    resources->wineMarkup = nullptr;

    resources->housePlanHorizontal = nullptr;
    resources->housePlanVertical = nullptr;

    resources->houseWipHorizontal = nullptr;
    resources->houseWipVertical = nullptr;

    for (int i = 0; i < 6; ++i) {
        resources->houseTablets[i] = nullptr;
    }
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


void renderKaMHouseEdge(
    SDL_Renderer* renderer,
    SDL_Texture* horizontalTexture,
    SDL_Texture* verticalTexture,
    int cellScreenX,
    int cellScreenY,
    int tileSize,
    int direction
)
{
    if (!renderer)
        return;

    // Prevod originalniho KaM RenderFence().
    //
    // direction:
    // 0 = N
    // 1 = E
    // 2 = S
    // 3 = W
    //
    // Remake pouziva CELL_SIZE_PX = 40.
    // KaMVita pouziva TILE_SIZE = 32.
    //
    // Originalni presah hrany:
    // 3 / 40 tile.

    const float edgeExtension =
        3.0f *
        (float)tileSize /
        40.0f;

    const SDL_Color color = {
        255,
        255,
        255,
        255
    };

    // ------------------------------------------------------
    // N / S
    //
    // RenderFence:
    //
    // pX-1-3/40, y1
    // pX-1-3/40, y1-FenceY
    // pX  +3/40, y2-FenceY
    // pX  +3/40, y2
    //
    // Na plochem KaMVita terenu jsou y1/y2 shodne.
    // ------------------------------------------------------

    if (
        direction == 0 ||
        direction == 2
    ) {
        if (!horizontalTexture)
            return;

        int textureWidth = 0;
        int textureHeight = 0;

        SDL_QueryTexture(
            horizontalTexture,
            nullptr,
            nullptr,
            &textureWidth,
            &textureHeight
        );

        if (
            textureWidth <= 0 ||
            textureHeight <= 0
        ) {
            return;
        }

        // RenderFence pouziva PxWidth jako tloustku
        // horizontalniho fence quadu.
        const float fenceThickness =
            (float)textureWidth *
            (float)tileSize /
            40.0f;

        float edgeY =
            (float)cellScreenY;

        if (direction == 2) {
            edgeY +=
                (float)tileSize;
        }

        SDL_Vertex vertices[4] = {};

        vertices[0].position = {
            (float)cellScreenX -
                edgeExtension,
            edgeY
        };

        vertices[1].position = {
            (float)cellScreenX -
                edgeExtension,
            edgeY -
                fenceThickness
        };

        vertices[2].position = {
            (float)cellScreenX +
                (float)tileSize +
                edgeExtension,
            edgeY -
                fenceThickness
        };

        vertices[3].position = {
            (float)cellScreenX +
                (float)tileSize +
                edgeExtension,
            edgeY
        };

        for (int i = 0; i < 6; ++i) {
            vertices[i].color = color;
        }

        // Presne poradi UV odpovida RenderFence.
        vertices[0].tex_coord = {1.0f, 0.0f};
        vertices[1].tex_coord = {0.0f, 0.0f};
        vertices[2].tex_coord = {0.0f, 1.0f};
        vertices[3].tex_coord = {1.0f, 1.0f};

        const int indices[6] = {
            0, 1, 2,
            0, 2, 3
        };

        SDL_RenderGeometry(
            renderer,
            horizontalTexture,
            vertices,
            4,
            indices,
            6
        );

        return;
    }

    // ------------------------------------------------------
    // E / W
    //
    // RenderFence:
    //
    // W:
    // x1 = pX-1-3/40
    //
    // E:
    // x1 = pX-1+3/40-FenceX
    //
    // Vertikalni textura si zachovava celou nativni
    // vysku odpovidajici jednomu terennimu useku.
    // ------------------------------------------------------

    if (!verticalTexture)
        return;

    int textureWidth = 0;
    int textureHeight = 0;

    SDL_QueryTexture(
        verticalTexture,
        nullptr,
        nullptr,
        &textureWidth,
        &textureHeight
    );

    if (
        textureWidth <= 0 ||
        textureHeight <= 0
    ) {
        return;
    }

    const float fenceWidth =
        (float)textureWidth *
        (float)tileSize /
        40.0f;

    float edgeX = 0.0f;

    if (direction == 3) {
        // West.
        edgeX =
            (float)cellScreenX -
            edgeExtension;
    } else {
        // East.
        edgeX =
            (float)cellScreenX +
            (float)tileSize +
            edgeExtension -
            fenceWidth;
    }

    const float topY =
        (float)cellScreenY -
        edgeExtension;

    const float bottomY =
        (float)cellScreenY +
        (float)tileSize +
        edgeExtension;

    SDL_Vertex vertices[4] = {};

    vertices[0].position = {
        edgeX,
        topY
    };

    vertices[1].position = {
        edgeX + fenceWidth,
        topY
    };

    vertices[2].position = {
        edgeX + fenceWidth,
        bottomY
    };

    vertices[3].position = {
        edgeX,
        bottomY
    };

    for (int i = 0; i < 6; ++i) {
        vertices[i].color = color;
    }

    vertices[0].tex_coord = {0.0f, 0.0f};
    vertices[1].tex_coord = {1.0f, 0.0f};
    vertices[2].tex_coord = {1.0f, 1.0f};
    vertices[3].tex_coord = {0.0f, 1.0f};

    const int indices[6] = {
        0, 1, 2,
        0, 2, 3
    };

    SDL_RenderGeometry(
        renderer,
        verticalTexture,
        vertices,
        4,
        indices,
        6
    );
}
