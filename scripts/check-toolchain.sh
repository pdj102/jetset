#!/usr/bin/env sh
set -eu

missing=0

for tool in zcc z88dk-zsdcc m4 bison flex python3; do
    if command -v "$tool" >/dev/null 2>&1; then
        printf '%s: %s\n' "$tool" "$(command -v "$tool")"
    else
        printf '%s: missing\n' "$tool"
        missing=1
    fi
done

if [ -f /usr/include/boost/graph/adjacency_list.hpp ]; then
    printf '%s\n' 'boost graph headers: /usr/include/boost/graph/adjacency_list.hpp'
else
    printf '%s\n' 'boost graph headers: missing'
    missing=1
fi

if [ -z "${ZCCCFG:-}" ]; then
    printf '%s\n' 'ZCCCFG: missing (set it to your z88dk/lib/config directory)'
    missing=1
else
    printf 'ZCCCFG: %s\n' "$ZCCCFG"
fi

if [ "$missing" -ne 0 ]; then
    printf '%s\n' 'Install the missing z88dk or host tools, then run this check again.'
    exit 1
fi

printf '%s\n' 'Spectrum Next toolchain looks ready.'