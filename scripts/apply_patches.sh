#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PATCH_ROOT="$REPO_ROOT/patches"

declare -A SUBMODULES=(
    ["external/Shipwright"]="shipwright"
    ["external/Shipwright/libultraship"]="libultraship"
)

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

    cd "$sub_dir"

    local current_commit
    current_commit="$(git rev-parse HEAD)"

    local patches_hash
    if compgen -G "$patch_dir"/[0-9]*.patch > /dev/null; then
        patches_hash="$(cat "$patch_dir"/[0-9]*.patch | sha256sum | awk '{print $1}')"
    else
        patches_hash="no-patches"
    fi
    local sentinel_value="${current_commit}:${patches_hash}"

    if [[ -f "$sentinel" ]] && [[ "$(cat "$sentinel")" == "$sentinel_value" ]]; then
        local count
        count=$(ls "$patch_dir"/[0-9]*.patch 2>/dev/null | wc -l)
        echo "[$sub_path] Patches already applied (commit ${current_commit:0:7}, $count patches)"
        return 0
    fi

    echo "[$sub_path] Resetting to clean state at ${current_commit:0:7}..."
    git reset --hard "$current_commit"
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

echo "Done."