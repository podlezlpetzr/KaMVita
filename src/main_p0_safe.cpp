#include <SDL2/SDL.h>

static constexpr int SCREEN_WIDTH  = 960;
static constexpr int SCREEN_HEIGHT = 544;

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "KaM Vita",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        0
    );

    if (!window) {
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED
    );

    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;

    while (running) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // KaM-like dark background
        SDL_SetRenderDrawColor(renderer, 35, 31, 24, 255);
        SDL_RenderClear(renderer);

        // Temporary terrain area
        SDL_Rect terrain = {
            40,
            40,
            SCREEN_WIDTH - 80,
            SCREEN_HEIGHT - 80
        };

        SDL_SetRenderDrawColor(renderer, 64, 105, 55, 255);
        SDL_RenderFillRect(renderer, &terrain);

        // Temporary center marker
        SDL_Rect marker = {
            SCREEN_WIDTH / 2 - 10,
            SCREEN_HEIGHT / 2 - 10,
            20,
            20
        };

        SDL_SetRenderDrawColor(renderer, 220, 200, 120, 255);
        SDL_RenderFillRect(renderer, &marker);

        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
