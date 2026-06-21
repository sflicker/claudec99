#! /bin/sh

cc99 --sysinclude -DSDL_DISABLE_IMMINTRIN_H  -D__FLT_EPSILON__=1.1920928955078125e-07F test_sdl2_init.c -o test_sdl2_init $(sdl2-config --cflags --libs)
cc99 --sysinclude -DSDL_DISABLE_IMMINTRIN_H  -D__FLT_EPSILON__=1.1920928955078125e-07F sdl2_demo.c -o sdl2_demo $(sdl2-config --cflags --libs)
cc99 --sysinclude -DSDL_DISABLE_IMMINTRIN_H  -D__FLT_EPSILON__=1.1920928955078125e-07F pong.c -o pong $(sdl2-config --cflags --libs)

