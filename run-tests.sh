#!/bin/sh
# Build and run all unit tests on the build host.
# Requires a Qt 5 qmake (models are pure QtCore, no Silica needed).
# Override the qmake binary with: QMAKE=/path/to/qmake ./run-tests.sh
set -e

QMAKE="${QMAKE:-qmake}"
BUILD_DIR="${BUILD_DIR:-build-tests}"

if ! command -v "$QMAKE" >/dev/null 2>&1; then
    echo "error: '$QMAKE' not found - install Qt 5 or set QMAKE" >&2
    exit 1
fi

ROOT="$(cd "$(dirname "$0")" && pwd)"
mkdir -p "$ROOT/$BUILD_DIR"
cd "$ROOT/$BUILD_DIR"

"$QMAKE" "$ROOT/tests/tests.pro"
make -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu)"

# 'make check' runs every CONFIG+=testcase target and fails on test failure
make check
