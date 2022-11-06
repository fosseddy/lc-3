#!/bin/bash

set -xe

FLAGS="-g -Wall -Wextra -std=c11 -pedantic"
#FLAGS="-g -Wextra -std=c11 -pedantic"

gcc $FLAGS -o lc3 lc3.c
