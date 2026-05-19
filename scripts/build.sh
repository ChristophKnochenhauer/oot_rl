#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

cd "$REPO_ROOT"

"$REPO_ROOT/scripts/apply_patches.sh"

echo "==> Configuring (build type: $BUILD_TYPE)..."
cmake -S . -B "$BUILD_DIR" \
      -G Ninja \
      -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo "==> Building..."
cmake --build "$BUILD_DIR" --parallel

echo ""
echo "Build complete."
echo ""
