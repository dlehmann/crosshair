#!/usr/bin/env bash
# Configures and builds crosshair into ./build, without installing anything.
# Useful for development/testing - use install.sh to build AND install it.
#
# Usage: ./build.sh [Release|Debug|RelWithDebInfo|MinSizeRel]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BUILD_TYPE="${1:-Release}"

echo "==> Configuring (${BUILD_TYPE})"
cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

echo "==> Building"
cmake --build "${BUILD_DIR}" --parallel

echo
echo "Build complete: ${BUILD_DIR}/crosshair"
echo "Run it directly, e.g.:"
echo "  ${BUILD_DIR}/crosshair"
