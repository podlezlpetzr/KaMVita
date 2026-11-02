#include "kam_runtime_font.h"

#include <cstring>


#include "kam_runtime_font_data.inc"


static const RuntimeMetalGlyph*
findRuntimeMetalGlyph(
    char character
)
{
    for (
        int index = 0;
        index < RUNTIME_METAL_GLYPH_COUNT;
        ++index
    ) {
        if (
            RUNTIME_METAL_GLYPHS[index]
                .character == character
        ) {
            return &RUNTIME_METAL_GLYPHS[index];
        }
    }

    return nullptr;
}


bool loadRuntimeMetalFont(
    SDL_Renderer* renderer,
    RuntimeMetalFont* font
)
{
    if (!renderer || !font)
        return false;

    *font = {};

    SDL_Surface* surface =
        SDL_LoadBMP(
            "app0:assets/ui/options/"
            "runtime_font/metal_ascii.bmp"
        );

    if (!surface)
        return false;

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

    font->atlas =
        SDL_CreateTextureFromSurface(
            renderer,
            surface
        );

    SDL_FreeSurface(surface);

    if (!font->atlas) {
        *font = {};
        return false;
    }

    SDL_SetTextureBlendMode(
        font->atlas,
        SDL_BLENDMODE_BLEND
    );

    font->baseHeight = 13;
    font->wordSpacing = 6;
    font->charSpacing = 1;

    return true;
}


void destroyRuntimeMetalFont(
    RuntimeMetalFont* font
)
{
    if (!font)
        return;

    if (font->atlas) {
        SDL_DestroyTexture(
            font->atlas
        );
    }

    *font = {};
}


int measureRuntimeMetalText(
    const RuntimeMetalFont& font,
    const char* text
)
{
    if (!text)
        return 0;

    int width = 0;
    bool hasGlyph = false;

    for (
        int index = 0;
        text[index] != '\0';
        ++index
    ) {
        const char character = text[index];

        if (character == ' ') {
            width += font.wordSpacing;
            hasGlyph = true;
            continue;
        }

        const RuntimeMetalGlyph* glyph =
            findRuntimeMetalGlyph(
                character
            );

        if (!glyph)
            continue;

        width += glyph->advance;
        hasGlyph = true;
    }

    if (
        hasGlyph &&
        width >= font.charSpacing
    ) {
        width -= font.charSpacing;
    }

    return width;
}


void renderRuntimeMetalText(
    SDL_Renderer* renderer,
    const RuntimeMetalFont& font,
    const char* text,
    int x,
    int baselineY
)
{
    if (
        !renderer ||
        !font.atlas ||
        !text
    ) {
        return;
    }

    int cursorX = x;

    for (
        int index = 0;
        text[index] != '\0';
        ++index
    ) {
        const char character = text[index];

        if (character == ' ') {
            cursorX += font.wordSpacing;
            continue;
        }

        const RuntimeMetalGlyph* glyph =
            findRuntimeMetalGlyph(
                character
            );

        if (!glyph)
            continue;

        SDL_Rect source = {
            glyph->sourceX,
            glyph->sourceY,
            glyph->width,
            glyph->height
        };

        SDL_Rect destination = {
            cursorX,
            baselineY + glyph->yOffset,
            glyph->width,
            glyph->height
        };

        SDL_RenderCopy(
            renderer,
            font.atlas,
            &source,
            &destination
        );

        cursorX += glyph->advance;
    }
}


void renderRuntimeMetalTextCentered(
    SDL_Renderer* renderer,
    const RuntimeMetalFont& font,
    const char* text,
    const SDL_Rect& area
)
{
    if (!text)
        return;

    const int textWidth =
        measureRuntimeMetalText(
            font,
            text
        );

    const int x =
        area.x +
        (area.w - textWidth) / 2;

    const int y =
        area.y +
        (area.h - font.baseHeight) / 2;

    renderRuntimeMetalText(
        renderer,
        font,
        text,
        x,
        y
    );
}
