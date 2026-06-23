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

void drawWalls() {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    int width = 10;
    int x1 = width;
    int x2 = WINDOW_WIDTH - width;
    int y1 = width;
    int y2 = WINDOW_HEIGHT - width;

    SDL_Rect rect1 = { x1, y1, width, WINDOW_HEIGHT };
    SDL_Rect rect2 = { x1, y2, WINDOW_WIDTH, width};
    SDL_Rect rect3 = { x2, y1, width, WINDOW_HEIGHT };
    SDL_Rect rect4 = { x1, y1, WINDOW_WIDTH, width };

    SDL_RenderFillRect(renderer, &rect1);
    SDL_RenderFillRect(renderer, &rect2);
    SDL_RenderFillRect(renderer, &rect3);
    SDL_RenderFillRect(renderer, &rect4);

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

            drawWalls();



            SDL_RenderPresent(renderer);
        }
    }

    // SDL Teardown
    teardown(0);

    return 0;
}