#!/bin/sh

clang -E - -v < /dev/null 2>&1 | grep "^ /usr" | grep clang
