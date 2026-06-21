/* Minimal SDL2 smoke-test: init (no subsystems), query version, quit.
 * Compile: --sysinclude -DSDL_DISABLE_IMMINTRIN_H
 * Link:    -lSDL2  */
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

enum {
    WINDOW_WIDTH = 800,
    WINDOW_HEIGHT = 600
};

bool setup() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow(
        "Simple SDL2 Graphics",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (window == NULL) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

void teardown(int returnCode) {

    if (renderer != NULL) {
        SDL_DestroyRenderer(renderer);
    }
    if (window != NULL) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();

    exit(returnCode);
}

int main(void) {
    // SDL Setup
    if (!setup()) {
        teardown(1);
    }

    // body
    bool running = true;
    while(running) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
            }

            /* Dark blud backgroupd */
            SDL_SetRenderDrawColor(renderer, 20, 30, 60, 255);
            SDL_RenderClear(renderer);

            SDL_Rect red_rectangle = {
                .x = 100,
                .y = 100,
                .w = 250,
                .h = 150
            };

            SDL_SetRenderDrawColor(renderer, 220, 60, 60, 255);
            SDL_RenderFillRect(renderer, &red_rectangle);

            SDL_Rect green_rectangle = {
                .x = 450,
                .y = 300,
                .w = 200,
                .h = 200
            };

            SDL_SetRenderDrawColor(renderer, 60, 200, 100, 255);
            SDL_RenderFillRect(renderer, &green_rectangle);

            /* Draw a while diagonal line. */
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawLine(renderer, 50, 550, 750, 50);

            SDL_RenderPresent(renderer);
        }
    }

    // SDL Teardown
    teardown(0);

    return 0;
}