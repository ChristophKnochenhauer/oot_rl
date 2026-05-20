#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOH_DIR="$REPO_ROOT/external/Shipwright"
PATCH_DIR="$REPO_ROOT/patches"
SENTINEL="$SOH_DIR/.patches_applied"

if [[ ! -e "$SOH_DIR/.git" ]]; then
    echo "error: Shipwright submodule not initialised." >&2
    echo "Run: git submodule update --init --recursive" >&2
    exit 1
fi

cd "$SOH_DIR"

CURRENT_COMMIT="$(git rev-parse HEAD)"
if compgen -G "$PATCH_DIR"/[0-9]*.patch > /dev/null; then
    PATCHES_HASH="$(cat "$PATCH_DIR"/[0-9]*.patch | sha256sum | awk '{print $1}')"
else
    PATCHES_HASH="no-patches"
fi
SENTINEL_VALUE="${CURRENT_COMMIT}:${PATCHES_HASH}"

if [[ -f "$SENTINEL" ]] && [[ "$(cat "$SENTINEL")" == "$SENTINEL_VALUE" ]]; then
    echo "Patches already applied (commit $CURRENT_COMMIT, $(ls "$PATCH_DIR"/[0-9]*.patch 2>/dev/null | wc -l) patches)"
    exit 0
fi

echo "Resetting Shipwright to clean state at $CURRENT_COMMIT..."
git reset --hard "$CURRENT_COMMIT"
git clean -fd

shopt -s nullglob
patches=( "$PATCH_DIR"/[0-9]*.patch )

if [[ ${#patches[@]} -eq 0 ]]; then
    echo "No patches to apply."
    echo "$CURRENT_COMMIT" > "$SENTINEL"
    exit 0
fi

for p in "${patches[@]}"; do
    echo "Applying $(basename "$p")..."
    if ! git apply --3way --whitespace=nowarn "$p"; then
        echo ""
        echo "ERROR: Patch failed: $(basename "$p")" >&2
        echo "Resolve in $SOH_DIR, then regenerate patch with:" >&2
        echo "    cd $SOH_DIR && git diff > $p" >&2
        exit 1
    fi
done

echo "$SENTINEL_VALUE" > "$SENTINEL"
echo "All ${#patches[@]} patches applied successfully."