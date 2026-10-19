# C: universal rules

Rules that hold for C code anywhere. See [README.md](README.md) for the
format and strengths.

## Naming

### C-NAME-1 · should · Functions, variables, parameters, members and tags are snake_case
Lower case words joined by underscores: `level_can_enter`, `free_before`,
`struct level_room`. No camelCase or Hungarian prefixes.

### C-NAME-2 · should · Macros and constants are ALL_CAPS
Object-like macros, `enum` constants and `#define` constants are upper
case with underscores: `LEVEL_DEPTH_BOTTOM`, `ACT_R_NO_STAIRS`.
Exception: a function-like macro that stands in for a function may be named
like one (`nge_util_assert()`, `nge_timer_stopped()`, `level_pending()`).
It must evaluate each argument at most once, be usable wherever a call is,
and be replaceable by a real function without changing any caller. Debug
macros that compile to nothing with `NDEBUG`, like `assert()`, evaluate
none (see C-IDIOM-1).
Check: `grep -n "#define [a-z]"`. Each hit must meet the exception.

### C-NAME-3 · must · Never declare, define or #define a reserved identifier
Names beginning with `_` followed by a capital letter, or with `__`, are
reserved for the compiler and library. So are all names beginning with `_`
at file scope. Declaring or defining one, or using one as a macro name, is
undefined behaviour (C11 7.1.3). The toolchain's own names (`__SDCC`,
`__z88dk_fastcall`) are used, never defined. A macro parameter is neither a
declaration nor a macro name, so it isn't covered here. A name like
`_Expression` breaks C-NAME-1 instead.
Check: `grep -nE "\b_[A-Z_a-z]" src`, outside toolchain names and macro
parameter lists.

### C-NAME-4 · should · File-local functions and variables are static
A function or variable used only in its own `.c` file is `static`, and is
not given the module prefix. Everything else is declared in a header.
Exception: a static function that other files' comments or documents name
keeps its module prefix, so the name can be found from them. These are the
dispatch functions, `game_on_event()` and `game_on_timer()`, named in
`game/game.h`, `game/timers.h` and the P- rules.

## Headers

### C-HDR-1 · must · Headers have include guards named after the file
`#ifndef <BASENAME>_H` / `#define <BASENAME>_H` / `#endif`, where BASENAME
is the file's base name in capitals (`level_internal.h` →
`LEVEL_INTERNAL_H`). Base names are unique in the project (N-FILE-2), so
guards are too. No `#pragma once`.

### C-HDR-2 · should · A header compiles on its own
A header includes the headers for every type and macro it uses
(`<stdint.h>`, `core/nge_bank.h` for `NGE_BANKED`), so it can be included
first in any file.

### C-HDR-3 · should · A source file includes its own header first
`level_travel.c` includes `game/level/level_internal.h`, which includes
`level.h`, before anything else, which checks C-HDR-2. Then come system
headers, nge headers, and game headers, each group sorted.

### C-HDR-4 · must · Headers declare; source files define
Headers hold `extern` declarations, types, macros and prototypes. Variables
and functions are defined in exactly one `.c` file. The only definitions in
headers are macros and the compile-time check typedefs (H-CHECK-1).

### C-HDR-5 · should · A file includes only the headers it uses
Each `#include` provides a name the file itself uses: a function, type,
macro or constant. Remove an include when its last use goes. A header the
file needs only because another header needs it isn't included: that
header includes it (C-HDR-2).
Why: an unused include is a false dependency. The build rebuilds the file
whenever that header changes, and a reader, or a check of N-LAYER-1, takes
it for a real one.
Check: for each include, search the file for a name the header declares.

## Preprocessor

### C-PRE-1 · must · Macro parameters and expression bodies are parenthesised
Each use of a parameter is in parentheses, and so is a macro that expands to
an expression: `#define NGE_OWNER_AREA(a) ((nge_owner_t)(NGE_ENTITY_MAX + 1 + (a)))`.

### C-PRE-2 · must · A statement macro is one statement
A macro that expands to statements is wrapped in `do { ... } while (0)`, or
is a single expression, so it is safe after an `if` with no braces.
Check: macros containing `;` or `if` outside `do { } while (0)`.

### C-PRE-3 · should · Build switches with a value are tested with #if
Switches that are 0 or 1 (`GAME_DEV_KIT`) are tested with `#if`, so `-DX=0`
turns them off. `#ifdef` is only for symbols that are defined or not:
`NDEBUG`, `__SDCC`.

## Idioms

### C-IDIOM-1 · must · Asserts have no side effects
`nge_util_assert()` is compiled out with `NDEBUG`, so its argument must not
change state. Call first, then assert the result held in a local:
`gone = nge_entity_destroy(id); nge_util_assert(gone); (void)gone;`.
Check: asserts whose argument calls a function that changes state, or uses
`=`, `++` or `--`.

### C-IDIOM-2 · must · What a function acquires, it releases on every path
A function that opens a stream, maps a page into a slot or stops the clock
for its own purposes undoes it before every return. With more than one exit
after acquiring, use one `cleanup:` label at the end and `goto cleanup`, not
a copy of the release code before each `return`.

### C-IDIOM-3 · should · String comparisons are explicit
`strcmp(a, b) == 0`, never `!strcmp(a, b)` or `if (strcmp(a, b))`.

### C-IDIOM-4 · must · Writes into fixed buffers are bounded
Text written into a fixed array is limited by the buffer's size
(`strncat(line, s, sizeof(line) - 1 - strlen(line))`, `gen_name(id, buf,
sizeof(buf))`). `strcpy` and `sprintf` are only for copying a literal that
is known to fit.

### C-IDIOM-5 · should · Sentinels are compared by name
Test against the named value: `id != NGE_ENTITY_INVALID`, `p != NULL`,
`kind != KIND_INVALID`. Never compare with the raw number (`0xFF`) or rely on
truthiness for a value that isn't a boolean.

### C-IDIOM-6 · should · Use fixed-width types for data
Game and engine data uses `<stdint.h>` types sized to the range
(`uint8_t` for IDs, depths, counts below 256). `int` and `unsigned int` only
where an interface needs them (stream `put` returns `int`) or for the casts
in T-INT-1.

### C-IDIOM-7 · should · Array bounds come from the array
Loops and size arguments over a local or static array use `sizeof(a)` or the
constant the array was declared with, never a repeated literal.
