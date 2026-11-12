#!/bin/sh
# Run a zcc compile and fail it on zsdcc warning 147.
#
# usage: zcc-checked.sh <zcc> <args...> -o <file.o>
#
# Warning 147 ("excess elements in struct initializer") is what zsdcc gives
# when an initialiser names an anonymous union's member other than the first:
# it then lays the bytes out wrong (the value lands in a later field) and
# still writes the object file. Name the union instead (docs/items.md D19).
# On warning 147 the object file is deleted, so the next make retries it.

out=""
prev=""
for a in "$@"; do
    [ "$prev" = "-o" ] && out=$a
    prev=$a
done

log=$(mktemp) || exit 1
"$@" >"$log" 2>&1
st=$?
cat "$log"
if grep -q 'warning 147' "$log"; then
    rm -f "$log" ${out:+"$out"}
    echo "error: zsdcc warning 147 is an error here: name the union (scripts/zcc-checked.sh)" >&2
    exit 1
fi
rm -f "$log"
exit $st
