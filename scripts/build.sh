#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/build-cmake}"

cd "$REPO_ROOT"

"$REPO_ROOT/scripts/apply_patches.sh"

echo "==> Configuring..."
cmake -S . -B "$BUILD_DIR" -GNinja

echo "==> Building..."
cmake --build "$BUILD_DIR" "$@"

echo ""
echo "Build complete."
echo ""
