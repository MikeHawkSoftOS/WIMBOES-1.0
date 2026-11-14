#!/bin/sh
# Build the UI natively with sanitizers and dump screenshots to /tmp/shots/*.ppm
cd "$(dirname "$0")/.." || exit 1
SRC=$(ls src/*.c | grep -v -e klibc.c -e hal_x86.c -e main.c)
gcc -DHOST -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Isrc $SRC test/host_main.c -o /tmp/wimboes_host && /tmp/wimboes_host
