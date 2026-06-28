#!/bin/bash
set -e
CC99="${1:-cc99}"
mkdir -p build
cmake -B build -S . \
    -DCMAKE_C_COMPILER="$CC99" \
    -DCMAKE_C_FLAGS="" \
    --no-warn-unused-cli \
    -Wno-dev \
    >/dev/null 2>&1
cmake --build build >/dev/null 2>&1
./build/hello
