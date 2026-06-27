cmake_minimum_required(VERSION 3.20)

project(sdl_plot C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2)
find_package(SDL2 CONFIG COMPONENTS sdL2main)

add_executable(sdl_plot src/sdl_plot.c)

if(TARGET SDL2::SDL2main)
    target_link_libraries(sdl_plot PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(sdl_plot PRIVATE SDL2::SDL2)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(sdl_plot PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()