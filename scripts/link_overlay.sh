#!/usr/bin/env bash
#
# link_overlay.sh — symlink our own (non-upstream) source files from overlay/
# into the submodule trees.
#
# Files under overlay/<sub>/ are 100% ours: they have no upstream counterpart
# and can never conflict on a submodule bump, so we keep them as normal tracked
# files in the main repo instead of burying them in the patch series. They are
# symlinked into place (not copied) so editing overlay/ is reflected in the build
# immediately — no patch regeneration needed.
#
# The symlinks are recreated on every apply (git clean -fd in apply_patches.sh
# wipes them) and added to each submodule's .git/info/exclude so they never show
# up as untracked changes or get committed into the submodule.
#
# Called by apply_patches.sh and start_dev.sh. Idempotent.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OVERLAY_ROOT="$REPO_ROOT/overlay"

# submodule path -> overlay subdir
declare -A SUBMODULES=(
    ["external/Shipwright"]="shipwright"
    ["external/Shipwright/libultraship"]="libultraship"
)

link_overlay_for_submodule() {
    local sub_path="$1"
    local overlay_subdir="$2"
    local sub_dir="$REPO_ROOT/$sub_path"
    local overlay_dir="$OVERLAY_ROOT/$overlay_subdir"

    [[ -d "$overlay_dir" ]] || return 0
    [[ -e "$sub_dir/.git" ]] || return 0

    # Resolve the real git dir (submodule .git is a file pointing elsewhere).
    local gitdir exclude_file
    gitdir="$(cd "$sub_dir" && git rev-parse --git-dir)"
    exclude_file="$gitdir/info/exclude"
    mkdir -p "$(dirname "$exclude_file")"
    touch "$exclude_file"

    local count=0 src rel dest
    while IFS= read -r -d '' src; do
        rel="${src#"$overlay_dir"/}"
        dest="$sub_dir/$rel"
        mkdir -p "$(dirname "$dest")"
        ln -sfn "$src" "$dest"
        grep -qxF "$rel" "$exclude_file" || echo "$rel" >> "$exclude_file"
        count=$((count + 1))
    done < <(find "$overlay_dir" -type f -print0)

    echo "[$sub_path] linked $count overlay file(s)"
}

for sub_path in "external/Shipwright/libultraship" "external/Shipwright"; do
    link_overlay_for_submodule "$sub_path" "${SUBMODULES[$sub_path]}"
done
