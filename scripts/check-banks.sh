#!/bin/sh
# Check that banked C code carries its page in its address.
#
# usage: check-banks.sh <file.map> [source dir, default src]
#
# A call to an NGE_BANKED (__banked) function stores the function's address
# after "call banked_call"; banked_call maps the page in bits 16-23 of that
# address into slot 6 (and the next page into slot 7). z88dk's default
# section origins for the Next ($C000/$E000) have no page in them, so every
# banked call would map page 0 and jump into the wrong code. Each bank that
# holds code needs its origin set in src/nge/banks.inc, e.g.
#   #pragma define CRT_ORG_PAGE_42 = 0x2AC000     (page 42 = 0x2A)
#
# It also checks that banked_call is nge's own, not z88dk's.
#
# Symbols from C files (module names ending in _c) in BANK_n sections and
# even PAGE_n sections are banked code or its literals and constants: their
# page must be 2n (BANK_n) or n (PAGE_n). Odd PAGE_n sections hold data only
# (a module's large private data sets, at $E000), which needs no page in its
# address; banked code must not be there, which is checked in the sources
# (#pragma codeseg). Data in .asm files is mapped by hand and is not checked.

map="$1"
srcdir="${2:-src}"
if [ ! -f "$map" ]; then
    echo "check-banks: map file '$map' not found" >&2
    exit 1
fi
if [ ! -d "$srcdir" ]; then
    echo "check-banks: source directory '$srcdir' not found" >&2
    exit 1
fi

# banked_call maps the page into slot 6 ($C000); an odd page is assembled for
# slot 7 ($E000), so code placed there would be in the wrong place
odd=$(grep -rnE '^[[:space:]]*#pragma[[:space:]]+codeseg[[:space:]]+PAGE_[0-9]*[13579]\b' \
      --include='*.c' "$srcdir")
if [ -n "$odd" ]; then
    echo "$odd" | while IFS= read -r line; do
        echo "error: $line: banked code must be in an even page (or a BANK_n section)"
    done
    exit 1
fi

awk '
# banked_call must be the nge version (src/nge/core/nge_banked_call.asm), which
# preserves the interrupt state; the z88dk version enables interrupts on every call
$1 == "banked_call" && $2 == "=" {
    module = $8; sub(/,$/, "", module)
    if (module != "nge_banked_call_asm") {
        printf "error: banked_call comes from module %s, not nge_banked_call_asm: z88dk%ss version enables interrupts on every banked call.\n", module, "\047"
        bad = 1
    }
}

# _name = $28C000 ; addr, public, , module_c, PAGE_40, file.c:12
$2 == "=" && $3 ~ /^\$/ && $5 == "addr," {
    module = $8; sub(/,$/, "", module)
    section = $9; sub(/,$/, "", section)
    if (module !~ /_c$/) next
    if (section ~ /^BANK_[0-9]+$/) { n = substr(section, 6); want = 2 * n }
    else if (section ~ /^PAGE_[0-9]+$/) { n = substr(section, 6); want = n + 0 }
    else next

    # Odd pages: private data only, read from slot 7 by its own module
    if (section ~ /^PAGE_/ && want % 2 == 1) next

    hex = toupper(substr($3, 2)); value = 0
    for (i = 1; i <= length(hex); i++) {
        value = value * 16 + index("0123456789ABCDEF", substr(hex, i, 1)) - 1
    }
    page = int(value / 65536)
    if (page != want) {
        printf "error: %s (%s in %s) is at $%s, so banked calls would map page %d, not %d.\n", \
               $1, module, section, hex, page, want
        printf "       Set its origin in src/nge/banks.inc: #pragma define CRT_ORG_%s = 0x%02X%04X\n", \
               section, want, value % 65536
        bad = 1
    }
}
END { exit bad }
' "$map"
