#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOH_DIR="$REPO_ROOT/external/Shipwright"
PATCH_DIR="$REPO_ROOT/patches"
SENTINEL="$SOH_DIR/.patches_applied"

if [[ ! -d "$SOH_DIR/.git" ]]; then
    echo "error: Shipwright submodule not initialised." >&2
    echo "Run: git submodule update --init --recursive" >&2
    exit 1
fi

cd "$SOH_DIR"

CURRENT_COMMIT="$(git rev-parse HEAD)"

if [[ -f "$SENTINEL" ]] && [[ "$(cat "$SENTINEL")" == "$CURRENT_COMMIT" ]]; then
    echo "Patches already applied for commit $CURRENT_COMMIT"
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

echo "$CURRENT_COMMIT" > "$SENTINEL"
echo "All ${#patches[@]} patches applied successfully."