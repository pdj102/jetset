#!/usr/bin/env bash
# Update z88dk (a git clone) and rebuild it with SDCC (zsdcc), then build and
# test this project with the new toolchain.
#
# usage: scripts/update-z88dk.sh [-f]     -f: rebuild even if nothing changed
#        Z88DK_ROOT=/other/path scripts/update-z88dk.sh
set -euo pipefail

Z88DK_ROOT=${Z88DK_ROOT:-$HOME/src/z88dk}
PROJECT=$(cd "$(dirname "$0")/.." && pwd)
LOG=$Z88DK_ROOT/update.log
force=0
[ "${1:-}" = "-f" ] && force=1

cd "$Z88DK_ROOT"
old=$(git rev-parse --short HEAD)

on_exit() {
    if [ $? -ne 0 ]; then
        echo "z88dk update FAILED (log: $LOG). To go back to the previous version:"
        echo "  cd $Z88DK_ROOT && git checkout $old && git submodule update --init --recursive \\"
        echo "    && BUILD_SDCC=1 BUILD_SDCC_HTTP=1 ./build.sh -C"
    fi
}
trap on_exit EXIT

if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
    echo "z88dk has local changes: commit or stash them first."
    exit 1
fi

git pull --ff-only
git submodule update --init --recursive
new=$(git rev-parse --short HEAD)

if [ "$old" = "$new" ] && [ $force -eq 0 ]; then
    echo "z88dk is already up to date ($new); use -f to rebuild anyway."
    exit 0
fi
echo "z88dk: $old -> $new"

# z88dk's CI result for the new commit (for information only)
ci=$(curl -fsS "https://api.github.com/repos/z88dk/z88dk/actions/runs?head_sha=$(git rev-parse HEAD)" |
     python3 -c 'import json,sys; r=json.load(sys.stdin)["workflow_runs"]; print(" ".join("%s=%s" % (x["name"], x["conclusion"]) for x in r) or "no runs yet")') ||
     ci="unknown (no network?)"
echo "z88dk CI for $new: $ci"

# Clean rebuild: the bin/ tools depend only on src/config.h, so a plain make
# after a pull keeps the old tools.
export BUILD_SDCC=1 BUILD_SDCC_HTTP=1
MAKE_CONCURRENCY=-j"$(nproc)" ./build.sh -C 2>&1 | tee "$LOG"
bin/zcc 2>&1 | head -1

# The project with the new toolchain: hardware build, tests, then the CSpect build last
make -C "$PROJECT" Z88DK_ROOT="$Z88DK_ROOT" clean
make -C "$PROJECT" Z88DK_ROOT="$Z88DK_ROOT" CFLAGS=-DNDEBUG
make -C "$PROJECT" Z88DK_ROOT="$Z88DK_ROOT" test
make -C "$PROJECT" Z88DK_ROOT="$Z88DK_ROOT" -B

echo "z88dk updated to $new (was $old); project builds and tests pass. Check bin/main.nex in CSpect."
