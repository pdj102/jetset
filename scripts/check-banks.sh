#!/bin/sh
# Check that banked C code carries its page in its address.
#
# usage: check-banks.sh <file.map>
#
# A call to an NGE_BANKED (__banked) function stores the function's address
# after "call banked_call"; banked_call maps the page in bits 16-23 of that
# address into slot 6 (and the next page into slot 7). z88dk's default
# section origins for the Next ($C000/$E000) have no page in them, so every
# banked call would map page 0 and jump into the wrong code. Each bank that
# holds code needs its origin set in src/nge/banks.inc, e.g.
#   #pragma define CRT_ORG_BANK_20 = 0x28C000     (bank 20 = page 40 = 0x28)
#
# Symbols from C files (module names ending in _c) in BANK_n or PAGE_n
# sections are banked code: their page must be 2n (BANK_n) or n (PAGE_n).
# Data in .asm files is mapped by hand and is not checked.

map="$1"
if [ ! -f "$map" ]; then
    echo "check-banks: map file '$map' not found" >&2
    exit 1
fi

awk '
# _name = $28C000 ; addr, public, , module_c, BANK_20, file.c:12
$2 == "=" && $3 ~ /^\$/ && $5 == "addr," {
    module = $8; sub(/,$/, "", module)
    section = $9; sub(/,$/, "", section)
    if (module !~ /_c$/) next
    if (section ~ /^BANK_[0-9]+$/) { n = substr(section, 6); want = 2 * n }
    else if (section ~ /^PAGE_[0-9]+$/) { n = substr(section, 6); want = n + 0 }
    else next

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
