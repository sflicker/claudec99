/* Minimal SDL2 smoke-test: init (no subsystems), query version, quit.
 * Compile: --sysinclude -DSDL_DISABLE_IMMINTRIN_H
 * Link:    -lSDL2  */
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>

SDL_Window *window = NULL;

int main(void) {

    return 0;
}