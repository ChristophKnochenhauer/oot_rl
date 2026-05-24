#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PATCH_ROOT="$REPO_ROOT/patches"
BASELINE_TAG="oot-rl-baseline"

declare -A SUBMODULES=(
    ["external/Shipwright"]="shipwright"
    ["external/Shipwright/libultraship"]="libultraship"
)

SUBMODULE_ORDER=(
    "external/Shipwright/libultraship"
    "external/Shipwright"
)

export_for_submodule() {
    local sub_path="$1"
    local patch_subdir="$2"
    local sub_dir="$REPO_ROOT/$sub_path"
    local patch_dir="$PATCH_ROOT/$patch_subdir"

    echo ""
    echo "[$sub_path] export"

    cd "$sub_dir"

    if ! git rev-parse "$BASELINE_TAG" >/dev/null 2>&1; then
        echo "  no $BASELINE_TAG tag — not in dev mode, skipping"
        return 0
    fi

    local new_commits
    new_commits=$(git rev-list --count "$BASELINE_TAG"..HEAD)

    if [[ "$new_commits" -eq 0 ]]; then
        echo "  no new commits since baseline"
        return 0
    fi

    mkdir -p "$patch_dir"

    shopt -s nullglob
    local existing_patches=("$patch_dir"/[0-9]*.patch)
    local start_num=$((${#existing_patches[@]} + 1))

    echo "  $new_commits new commit(s), numbering from $start_num"

    git format-patch \
        --start-number "$start_num" \
        -o "$patch_dir" \
        "$BASELINE_TAG"..HEAD | while read -r f; do
        echo "    $(basename "$f")"
    done
}

for sub_path in "${SUBMODULE_ORDER[@]}"; do
    export_for_submodule "$sub_path" "${SUBMODULES[$sub_path]}"
done

echo ""
echo "Export complete. Inspect with:"
echo "  ls patches/libultraship/ patches/shipwright/"
echo ""
echo "When happy, commit in main repo:"
echo "  git add patches/"
echo "  git commit"
echo ""
