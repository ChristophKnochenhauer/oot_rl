#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BASELINE_TAG="oot-rl-baseline"
DEV_BRANCH="oot-rl-dev"

SUBMODULE_LEAVE_ORDER=(
    "external/Shipwright"
    "external/Shipwright/libultraship"
)

detect_mode() {
    local sub_dir="$1"
    cd "$sub_dir"
    
    local current_branch
    current_branch=$(git symbolic-ref --quiet --short HEAD || echo "")
    
    if [[ "$current_branch" == "$DEV_BRANCH" ]] || git rev-parse "$BASELINE_TAG" >/dev/null 2>&1; then
        echo "dev"
    elif [[ -f "$sub_dir/.patches_applied" ]]; then
        echo "applied"
    else
        echo "clean"
    fi
}

has_uncommitted=false
for sub_path in "${SUBMODULE_ORDER[@]}"; do
    sub_dir="$REPO_ROOT/$sub_path"
    if [[ -d "$sub_dir/.git" || -f "$sub_dir/.git" ]]; then
        mode=$(detect_mode "$sub_dir")
        if [[ "$mode" == "dev" ]]; then
            cd "$sub_dir"
            if [[ -n "$(git status --porcelain --ignore-submodules=all)" ]]; then
                echo "WARNING: $sub_path has uncommitted changes:"
                git status --short --ignore-submodules=all | head -5 | sed 's/^/    /'
                has_uncommitted=true
            fi
        fi
    fi
done


if $has_uncommitted; then
    echo ""
    read -rp "Discard and leave dev mode? [y/N] " confirm
    if [[ "$confirm" != "y" && "$confirm" != "Y" ]]; then
        echo "Aborted."
        exit 1
    fi
fi

leave_dev_for_submodule() {
    local sub_path="$1"
    local sub_dir="$REPO_ROOT/$sub_path"

    echo ""
    echo "[$sub_path] leaving dev mode"
    cd "$sub_dir"

    local current_branch
    current_branch=$(git symbolic-ref --quiet --short HEAD || echo "")
    if [[ "$current_branch" == "$DEV_BRANCH" ]]; then
        git checkout --detach HEAD >/dev/null 2>&1
    fi

    git branch -D "$DEV_BRANCH" >/dev/null 2>&1 || true
    git tag -d "$BASELINE_TAG" >/dev/null 2>&1 || true
    rm -f "$sub_dir/.patches_applied"

    local parent_dir
    parent_dir=$(dirname "$sub_dir")
    local sub_basename
    sub_basename=$(basename "$sub_dir")

    cd "$parent_dir"
    git submodule update --init --force "$sub_basename" >/dev/null 2>&1

    cd "$sub_dir"
    local pin
    pin=$(git rev-parse HEAD)
    echo "  reset to pin ${pin:0:7}"
}

for sub_path in "${SUBMODULE_LEAVE_ORDER[@]}"; do
    leave_dev_for_submodule "$sub_path"
done

echo ""
echo "Re-applying patches..."
"$REPO_ROOT/scripts/apply_patches.sh"

echo ""
echo "Done. Back to clean applied state."
