#!/usr/bin/env sh
# Apply AgrabahPort patches to snesrecomp and recomp-ui idempotently.
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

apply_patch() {
    REPO_DIR="$1"
    PATCH_FILE="$2"
    LABEL="$3"

    if [ ! -d "$REPO_DIR/.git" ] && [ ! -f "$REPO_DIR/.git" ]; then
        echo "apply_patches: $LABEL submodule missing. Run: git submodule update --init --recursive" >&2
        exit 1
    fi
    if [ ! -f "$PATCH_FILE" ]; then
        echo "apply_patches: patch not found: $PATCH_FILE" >&2
        exit 1
    fi
    if git -C "$REPO_DIR" apply --reverse --check "$PATCH_FILE" >/dev/null 2>&1; then
        echo "apply_patches: $LABEL patch already applied."
        return 0
    fi
    git -C "$REPO_DIR" apply --check "$PATCH_FILE"
    git -C "$REPO_DIR" apply "$PATCH_FILE"
    echo "apply_patches: $LABEL patch applied cleanly."
}

apply_patch "$ROOT/recomp-ui" "$ROOT/patches/recomp-ui-live-log.patch" "recomp-ui"
apply_patch "$ROOT/snesrecomp" "$ROOT/patches/snesrecomp-agrabah.patch" "snesrecomp"
