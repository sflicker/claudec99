# ClaudeC99 compile with sdl2

## Task
The following sdl2 sample program fails to compile.
```C
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

enum {
    WINDOW_WIDTH = 800,
    WINDOW_HEIGHT = 600
};

int main(void) {

    // SDL Setup

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Simple SDL2 Graphics",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (window == NULL) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    // body
    // ...

    // SDL Teardown
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
```

When I try compiling with cc99 I get a compile error
```
 cc99 --sysinclude sdl2_demo.c -o sdl2_demo $(sdl2-config --cflags --libs)
/usr/include/SDL2/SDL_main.h:152:50: error: expected ')', got '[' ('[')
```

But it works with gcc
```
gcc -std=c99 sdl2_demo.c -o sdl2_demo $(sdl2-config --cflags --libs)
```

Investigate this issue and potentially fix it.
