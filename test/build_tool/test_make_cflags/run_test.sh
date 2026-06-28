#!/bin/bash
set -e
CC99="${1:-cc99}"
make -f Makefile CC99="$CC99" --no-print-directory >&2
./main
