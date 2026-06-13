#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PATCH_ROOT="$REPO_ROOT/patches"

DEV_BRANCH="oot-rl-dev"
BASELINE_TAG="oot-rl-baseline"

declare -A SUBMODULES=(
    ["external/Shipwright"]="shipwright"
    ["external/Shipwright/libultraship"]="libultraship"
)

get_pin_commit() {
    local sub_dir="$1"
    local parent_dir
    parent_dir=$(dirname "$sub_dir")
    local sub_basename
    sub_basename=$(basename "$sub_dir")

    (cd "$parent_dir" && git ls-tree HEAD "$sub_basename" 2>/dev/null | awk '{print $3}')
}

has_dev_artefacts() {
    local sub_dir="$1"
    (
        cd "$sub_dir"
        git show-ref --verify --quiet "refs/heads/$DEV_BRANCH" \
            || git rev-parse "$BASELINE_TAG" >/dev/null 2>&1
    )
}

apply_for_submodule() {
    local sub_path="$1"
    local patch_subdir="$2"
    local sub_dir="$REPO_ROOT/$sub_path"
    local patch_dir="$PATCH_ROOT/$patch_subdir"
    local sentinel="$sub_dir/.patches_applied"

    if [[ ! -e "$sub_dir/.git" ]]; then
        echo "error: submodule $sub_path not initialised." >&2
        echo "Run: git submodule update --init --recursive" >&2
        exit 1
    fi

    if [[ ! -d "$patch_dir" ]]; then
        echo "No patches directory for $sub_path (looked at $patch_dir), skipping."
        return 0
    fi

    local pin_commit
    pin_commit=$(get_pin_commit "$sub_dir")
    if [[ -z "$pin_commit" ]]; then
        echo "ERROR: Could not resolve pin commit for $sub_path." >&2
        echo "       Is the parent repository in a strange state?" >&2
        exit 1
    fi

    cd "$sub_dir"

    if has_dev_artefacts "$sub_dir"; then
        if [[ -n "$(git status --porcelain --ignore-submodules=all)" ]]; then
            echo "ERROR: $sub_path has dev-mode artefacts AND uncommitted changes." >&2
            echo "       Either commit them, stash them, or run leave_dev.sh first." >&2
            exit 1
        fi
        echo "[$sub_path] WARNING: dev-mode artefacts found, cleaning up:"
        if git show-ref --verify --quiet "refs/heads/$DEV_BRANCH"; then
            echo "             - deleting branch $DEV_BRANCH"
        fi
        if git rev-parse "$BASELINE_TAG" >/dev/null 2>&1; then
            echo "             - deleting tag $BASELINE_TAG"
        fi
        echo "             (commits remain reachable via git reflog if needed)"
    fi

    local patches_hash
    if compgen -G "$patch_dir"/[0-9]*.patch > /dev/null; then
        patches_hash="$(cat "$patch_dir"/[0-9]*.patch | sha256sum | awk '{print $1}')"
    else
        patches_hash="no-patches"
    fi
    local sentinel_value="${pin_commit}:${patches_hash}"

    if [[ -f "$sentinel" ]] \
        && [[ "$(cat "$sentinel")" == "$sentinel_value" ]] \
        && [[ "$(git rev-parse HEAD)" == "$pin_commit" ]] \
        && ! has_dev_artefacts "$sub_dir"; then
        local count
        count=$(ls "$patch_dir"/[0-9]*.patch 2>/dev/null | wc -l)
        echo "[$sub_path] Patches already applied (pin ${pin_commit:0:7}, $count patches)"
        return 0
    fi

    local current_branch
    current_branch=$(git symbolic-ref --quiet --short HEAD || echo "")
    if [[ -n "$current_branch" ]]; then
        git checkout --detach HEAD >/dev/null 2>&1
    fi

    git branch -D "$DEV_BRANCH" >/dev/null 2>&1 || true
    git tag -d "$BASELINE_TAG" >/dev/null 2>&1 || true

    echo "[$sub_path] Resetting to pin ${pin_commit:0:7}..."
    git reset --hard "$pin_commit"
    git clean -fd

    shopt -s nullglob
    local patches=( "$patch_dir"/[0-9]*.patch )

    if [[ ${#patches[@]} -eq 0 ]]; then
        echo "[$sub_path] No patches to apply."
        echo "$sentinel_value" > "$sentinel"
        return 0
    fi

    for p in "${patches[@]}"; do
        echo "[$sub_path] Applying $(basename "$p")..."
        if ! git apply --3way --whitespace=nowarn "$p"; then
            echo ""
            echo "ERROR: Patch failed: $(basename "$p")" >&2
            echo "Resolve in $sub_dir, then regenerate patch with:" >&2
            echo "    cd $sub_dir && git diff > $p" >&2
            exit 1
        fi
    done

    echo "$sentinel_value" > "$sentinel"
    echo "[$sub_path] Applied ${#patches[@]} patches."
}

for sub_path in "external/Shipwright/libultraship" "external/Shipwright"; do
    apply_for_submodule "$sub_path" "${SUBMODULES[$sub_path]}"
done

# Link our own (non-upstream) files from overlay/ into the submodules. Final pass
# so it survives the per-submodule reset --hard / clean -fd above.
"$REPO_ROOT/scripts/link_overlay.sh"

echo "Done."
