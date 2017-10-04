#!/bin/sh

BINDIR=
if [[ -d /usr/local/opt/llvm ]]; then
  BINDIR="/usr/local/opt/llvm/bin/"
fi

${BINDIR}clang -E - -v < /dev/null 2>&1 | grep "^ /" | grep clang
