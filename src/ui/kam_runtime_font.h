#pragma once

#include <SDL2/SDL.h>


struct RuntimeMetalGlyph {
    char character;
    int sourceX;
    int sourceY;
    int width;
    int height;
    int yOffset;
    int advance;
};


struct RuntimeMetalFont {
    SDL_Texture* atlas;
    int baseHeight;
    int wordSpacing;
    int charSpacing;
};


bool loadRuntimeMetalFont(
    SDL_Renderer* renderer,
    RuntimeMetalFont* font
);


void destroyRuntimeMetalFont(
    RuntimeMetalFont* font
);


int measureRuntimeMetalText(
    const RuntimeMetalFont& font,
    const char* text
);


void renderRuntimeMetalText(
    SDL_Renderer* renderer,
    const RuntimeMetalFont& font,
    const char* text,
    int x,
    int baselineY
);


void renderRuntimeMetalTextCentered(
    SDL_Renderer* renderer,
    const RuntimeMetalFont& font,
    const char* text,
    const SDL_Rect& area
);
