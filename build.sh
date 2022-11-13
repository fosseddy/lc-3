#!/bin/bash

set -xe

if [ "$1" = "clean" ]; then
    rm asm lc3
    exit 0
fi

FLAGS_PROD="-g -Wall -Wextra -std=c11 -pedantic"
FLAGS_DEF="-g -Wextra -std=c11 -pedantic"

FLAGS=$FLAGS_DEF

if [ "$1" = "prod" ] || [ "$2" = "prod" ]; then
    FLAGS=$FLAGS_PROD
fi

if [ "$1" = "lc3" ]; then
    gcc $FLAGS -o lc3 lc3.c
elif [ "$1" = "asm" ]; then
    gcc $FLAGS -o asm asm.c
else
    gcc $FLAGS -o lc3 lc3.c &
    gcc $FLAGS -o asm asm.c
fi
