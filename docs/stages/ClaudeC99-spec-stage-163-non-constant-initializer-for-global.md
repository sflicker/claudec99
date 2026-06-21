# ClaudeC99 stage 163 - non constant initializer for global issue

## Tasks
When trying to compile an SDL2 program i'm getting a compile error
Reduced program
```C
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
```

When trying to compile I'm getting an error that I don't see if compiled with `gcc`

```
cc99 --sysinclude  -DSDL_DISABLE_IMMINTRIN_H  -D__FLT_EPSILON__=1.1920928955078125e-07F non-constant-initializer-for-global.c  $(sdl2-config --cflags --libs)               
non-constant-initializer-for-global.c:8:35: error: non-constant initializer for global 'window'           
```

Investigate this issue and potentially fix it.