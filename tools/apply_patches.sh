#!/usr/bin/env sh
# Apply AgrabahPort optional UI patches (Live Log viewer in recomp-ui) idempotently.
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
PATCH="$ROOT/patches/recomp-ui-live-log.patch"
UI_DIR="$ROOT/recomp-ui"

if [ ! -d "$UI_DIR/.git" ] && [ ! -f "$UI_DIR/.git" ]; then
    echo "apply_patches: recomp-ui submodule missing. Run: git submodule update --init --recursive" >&2
    exit 1
fi

if [ ! -f "$PATCH" ]; then
    echo "apply_patches: patch not found: $PATCH" >&2
    exit 1
fi

if git -C "$UI_DIR" apply --reverse --check "$PATCH" >/dev/null 2>&1; then
    echo "apply_patches: Live Log patch already applied."
    exit 0
fi

git -C "$UI_DIR" apply --check "$PATCH"
git -C "$UI_DIR" apply "$PATCH"
echo "apply_patches: Live Log patch applied cleanly to recomp-ui."
