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
# holds code needs its origin set in src/banks.inc, e.g.
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
#
# Page data files (modules named *_pdata_c, see "Page data" in
# docs/memory_map.md) must have every symbol in an odd PAGE_n section: an
# initialised variable, a missing nge-dataseg marker or a missing constseg
# pragma would silently put it in common memory instead.
#
# Every PAGE_n section must fit in its 8K page: the linker reports an even
# page growing into a non-empty odd page, but not other overflows.
#
# Always-mapped data pages: data only, no page in their addresses, no code:
# - page 50, the ECS data, permanently in slot 0, assembled for $0100 (above
#   the null-pointer guard area $0000-$00FF): $0100-$1FFF;
# - page 51, the map data, permanently in slot 1: $2000-$3FFF.
# Common data files (modules named *_cdata_c, see "Always-mapped data pages"
# in docs/memory_map.md) must have every symbol in one of them, inside its
# range, and each page's section must stay inside its range.

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
fixed=$(grep -rnE '^[[:space:]]*#pragma[[:space:]]+(codeseg|constseg)[[:space:]]+PAGE_5[01]\b' \
      --include='*.c' "$srcdir")
if [ -n "$fixed" ]; then
    echo "$fixed" | while IFS= read -r line; do
        echo "error: $line: pages 50 and 51 are always-mapped data pages (slots 0 and 1): uninitialised variables only (nge-dataseg marker)"
    done
    exit 1
fi

odd=$(grep -rnE '^[[:space:]]*#pragma[[:space:]]+codeseg[[:space:]]+PAGE_[0-9]*[13579]\b' \
      --include='*.c' "$srcdir")
if [ -n "$odd" ]; then
    echo "$odd" | while IFS= read -r line; do
        echo "error: $line: banked code must be in an even page (or a BANK_n section)"
    done
    exit 1
fi

awk '
BEGIN {
    # Always-mapped data pages: address range [lo, hi) and where it is
    lo["PAGE_50"] = 256;   hi["PAGE_50"] = 8192;    where["PAGE_50"] = "slot 0 above the guard area ($0100-$1FFF)"
    lo["PAGE_51"] = 8192;  hi["PAGE_51"] = 16384;   where["PAGE_51"] = "slot 1 ($2000-$3FFF)"
}

function hexval(h,    v, i) {
    h = toupper(h); v = 0
    for (i = 1; i <= length(h); i++) v = v * 16 + index("0123456789ABCDEF", substr(h, i, 1)) - 1
    return v
}

# __PAGE_n_head = $E000 ; const, ...   (and _tail): section bounds
$1 ~ /^__PAGE_[0-9]+_(head|tail)$/ && $2 == "=" {
    name = $1; sub(/^__/, "", name)
    end = name; sub(/^PAGE_[0-9]+_/, "", end)
    sub(/_(head|tail)$/, "", name)
    bound[name, end] = hexval(substr($3, 2))
    pages[name] = 1
}

# Page data files: every symbol in an odd page
$2 == "=" && $3 ~ /^\$/ && $5 == "addr," {
    module = $8; sub(/,$/, "", module)
    section = $9; sub(/,$/, "", section)
    # (every C module also has a local label "zxn" in no section: not data)
    if (module ~ /_pdata_c$/ && section != "" && !(section ~ /^PAGE_[0-9]*[13579]$/)) {
        printf "error: %s (%s) is in %s, not its odd page: page data files may hold only uninitialised variables (with the nge-dataseg marker) and const data (with #pragma constseg)\n", \
               $1, module, section
        bad = 1
    }
}

# Common data files: every symbol in an always-mapped data page, inside its range
$2 == "=" && $3 ~ /^\$/ && $5 == "addr," {
    module = $8; sub(/,$/, "", module)
    section = $9; sub(/,$/, "", section)
    if (module ~ /_cdata_c$/ && section != "") {
        addr = hexval(substr($3, 2))
        if (!(section in lo) || addr < lo[section] || addr >= hi[section]) {
            printf "error: %s (%s) is in %s at %s, not inside an always-mapped data page (PAGE_50 at $0100-$1FFF, PAGE_51 at $2000-$3FFF): common data files may hold only uninitialised variables (with the nge-dataseg marker)\n", \
                   $1, module, section, $3
            bad = 1
        }
    }
}

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
    # Always-mapped data pages: data, checked above
    if (section in lo) next

    hex = toupper(substr($3, 2)); value = 0
    for (i = 1; i <= length(hex); i++) {
        value = value * 16 + index("0123456789ABCDEF", substr(hex, i, 1)) - 1
    }
    page = int(value / 65536)
    if (page != want) {
        printf "error: %s (%s in %s) is at $%s, so banked calls would map page %d, not %d.\n", \
               $1, module, section, hex, page, want
        printf "       Set its origin in src/banks.inc: #pragma define CRT_ORG_%s = 0x%02X%04X\n", \
               section, want, value % 65536
        bad = 1
    }
}
END {
    for (p in pages) {
        size = bound[p, "tail"] - bound[p, "head"]
        if (size > 8192) {
            printf "error: %s holds %d bytes, more than its 8K page\n", p, size
            bad = 1
        }
    }
    for (p in lo) {
        if ((p, "tail") in bound && bound[p, "tail"] > bound[p, "head"] &&
            (bound[p, "head"] < lo[p] || bound[p, "tail"] > hi[p])) {
            printf "error: %s is at $%04X-$%04X, outside %s: check CRT_ORG_%s in src/banks.inc and its size\n", \
                   p, bound[p, "head"], bound[p, "tail"] - 1, where[p], p
            bad = 1
        }
    }
    exit bad
}
' "$map"
