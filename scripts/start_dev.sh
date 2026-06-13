#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PATCH_ROOT="$REPO_ROOT/patches"
BASELINE_TAG="oot-rl-baseline"
DEV_BRANCH="oot-rl-dev"

declare -A SUBMODULES=(
    ["external/Shipwright"]="shipwright"
    ["external/Shipwright/libultraship"]="libultraship"
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

SUBMODULE_ORDER=(
    "external/Shipwright/libultraship"
    "external/Shipwright"
)

enter_dev_for_submodule() {
    local sub_path="$1"
    local patch_subdir="$2"
    local sub_dir="$REPO_ROOT/$sub_path"
    local patch_dir="$PATCH_ROOT/$patch_subdir"

    echo ""
    echo "[$sub_path] entering dev mode"

    if [[ ! -e "$sub_dir/.git" ]]; then
        echo "error: submodule $sub_path not initialised." >&2
        exit 1
    fi

    cd "$sub_dir"

    local current_branch
    current_branch=$(git symbolic-ref --quiet --short HEAD || echo "")
    if [[ "$current_branch" == "$DEV_BRANCH" ]]; then
        git checkout --detach HEAD >/dev/null 2>&1
    fi
    git branch -D "$DEV_BRANCH" >/dev/null 2>&1 || true
    git tag -d "$BASELINE_TAG" >/dev/null 2>&1 || true

    rm -f "$sub_dir/.patches_applied"

    local pin_commit
    pin_commit="$(git rev-parse HEAD)"
    echo "  pin: ${pin_commit:0:7}"

    git reset --hard "$pin_commit" >/dev/null
    git clean -fd >/dev/null
    git checkout -b "$DEV_BRANCH" >/dev/null 2>&1

    if [[ ! -d "$patch_dir" ]]; then
        echo "  no patches directory, baseline = pin"
        git tag "$BASELINE_TAG"
        return 0
    fi

    shopt -s nullglob
    local patches=("$patch_dir"/[0-9]*.patch)

    if [[ ${#patches[@]} -eq 0 ]]; then
        echo "  no patches found, baseline = pin"
        git tag "$BASELINE_TAG"
        return 0
    fi

    for p in "${patches[@]}"; do
        echo "  applying $(basename "$p")"
        if ! git am --3way --whitespace=nowarn "$p" >/dev/null 2>&1; then
            echo ""
            echo "ERROR: git am failed on $(basename "$p")" >&2
            echo "Resolve manually:" >&2
            echo "  cd $sub_dir" >&2
            echo "  # fix conflicts" >&2
            echo "  git add <files>" >&2
            echo "  git am --continue" >&2
            echo "Then run start_dev.sh again, or set the tag manually:" >&2
            echo "  git tag $BASELINE_TAG" >&2
            exit 1
        fi
    done

    git tag "$BASELINE_TAG"
    echo "  baseline at HEAD (after ${#patches[@]} patches)"
}

for sub_path in "${SUBMODULE_ORDER[@]}"; do
    sub_dir="$REPO_ROOT/$sub_path"
    if [[ -d "$sub_dir/.git" || -f "$sub_dir/.git" ]]; then
        mode=$(detect_mode "$sub_dir")
        if [[ "$mode" == "dev" ]]; then
            cd "$sub_dir"
            if [[ -n "$(git status --porcelain --ignore-submodules=all)" ]]; then
                echo "ERROR: $sub_path is in dev mode with uncommitted changes." >&2
                echo "Commit or stash them, then run start_dev.sh again." >&2
                exit 1
            fi
        fi
    fi
done

for sub_path in "${SUBMODULE_ORDER[@]}"; do
    enter_dev_for_submodule "$sub_path" "${SUBMODULES[$sub_path]}"
done

# Link our own files from overlay/ into the submodules so dev-mode builds see
# them. They stay untracked/excluded — dev mode is only for editing upstream
# files; to change our own files, edit overlay/ directly (no dev mode needed).
"$REPO_ROOT/scripts/link_overlay.sh"

echo ""
echo "Dev mode active."
echo ""
echo "  edit upstream:  \$EDITOR external/Shipwright/..."
echo "  edit our code:  \$EDITOR overlay/...        (no dev mode needed)"
echo "  commit:         cd external/Shipwright/...; git add ...; git commit"
echo "  build:          cmake --build build-cmake --target smoke_test"
echo "  export:         ./scripts/export_patches.sh"
echo "  leave:          ./scripts/leave_dev.sh"
echo ""
