/* Minimal SDL2 smoke-test: init (no subsystems), query version, quit.
 * Compile: --sysinclude -DSDL_DISABLE_IMMINTRIN_H
 * Link:    -lSDL2  */
#include <SDL2/SDL.h>
#include <stdio.h>

int main(void) {
    SDL_version ver;

    if (SDL_Init(0) != 0)
        return 1;

    SDL_GetVersion(&ver);
    printf("SDL2 %d.%d.%d\n", (int)ver.major, (int)ver.minor, (int)ver.patch);

    SDL_Quit();
    return 42;
}
