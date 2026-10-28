# House standards

Standards this project follows that are common but not universal. See
[README.md](README.md) for the format and strengths.

## Types

### H-TYPE-1 · should · Type names end in _t; struct tags drop it
`typedef struct level_room { ... } level_room_t;`. The tag is the type
name without `_t`, so either form names the same thing. Exceptions:
- pointer-to-function types end in `_fn` (`nge_timer_dispatch_fn`);
- `NGE_FILE`, which follows stdio's `FILE`.

### H-TYPE-2 · must · Values stored in data are uint8_t constants, not C enums
A value kept in a table, a component, an event payload or a kind record is a
`uint8_t` from a `#define` group (H-CONST-1 to H-CONST-5). A C `enum` is only for values
that are never stored in data (`game_state_t`).
Why: SDCC makes an `enum` int-sized, two bytes per entry in tables indexed by
entity, and data files and editors deal in numbers.

### H-TYPE-3 · should · Truth values are uint8_t 0 or 1
Functions that answer yes or no return `uint8_t`, 1 or 0, and say so in
`@return`. No `<stdbool.h>`.

## Constant groups

### H-CONST-1 · should · A group of related constants shares one prefix
`ACT_MOVE`, `ACT_PICKUP`, ...; `ACT_R_NONE`, `ACT_R_BLOCKED`, ...; `FT_DOOR`,
`FT_STAIRS_UP`. The prefix is unique to the group, and the group is defined
in one place, in order.

### H-CONST-2 · must · A group of numbered values ends in <PREFIX>_COUNT
Values numbered from 0 with no gaps end with `<PREFIX>_COUNT`, the number of
values (`ACT_COUNT`, `FT_COUNT`):
- valid values are `v < <PREFIX>_COUNT`. Asserts and range checks use the
  count, never `v <= <last value>`;
- tables indexed by the group are declared `[<PREFIX>_COUNT]`, with
  designated initialisers (`[FT_DOOR] = { ... }`) so order can't slip.

Not for bit-flag groups (H-CONST-4), or for open-ended ID ranges whose
values are added in several places (event IDs from `NGE_EVENT_USER`).
Check: `grep -nE "<= *[A-Z_]+[A-Z]\b"` for range checks against a last
value; `#define` groups with no `_COUNT`; counts not named `_COUNT`
(`_KINDS`, `_HOOKS`, `S` plurals).

### H-CONST-3 · must · _MAX and _MIN are inclusive limits
`X_MAX` is the largest value allowed, and `X_MIN` the smallest: valid values
satisfy `X_MIN <= v && v <= X_MAX`. For a capacity, `X_MAX` is the most
there can be (`NGE_ENTITY_MAX` entities, `SCREEN_NAME_MAX` characters), so a
table holds `X_MAX` entries and its indexes are `< X_MAX`. The limit word
comes last: `NGE_MAP_HEIGHT_MAX`, not `NGE_MAP_MAX_HEIGHT`.

### H-CONST-4 · should · Bit flags are masks named by meaning
A flag group's values are single-bit masks written in hex (`FEAT_OPEN
0x01`, `FCAN_ASCEND 0x02`). A mask covering several bits ends in `_MASK`.
A macro that turns a value number into its bit ends in `_BIT`
(`DMG_BIT(type)`, `SLOT_BIT(slot)`).

### H-CONST-5 · should · "None" and "invalid" are different values, each with a name
- **None:** a member of the group meaning "nothing", inside its valid range
  and usually 0: `<PREFIX>_NONE` (`EF_NONE`, `SLOT_NONE`, `CLASS_NONE`). It
  can be stored in data: a kind's hook that is `EF_NONE` has no effect.
- **Invalid:** a value outside the group's valid range, meaning "no valid
  value": `<PREFIX>_INVALID` (`NGE_ENTITY_INVALID`). It is returned when
  there is nothing to give, and is never a valid entry in data.

Functions return and callers test the name, never the number (`0xFF`).
Check: functions documented as returning a raw number for "none" or "not
found".

### H-CONST-6 · should · Numbers with a meaning are named
A number that stands for something (a size, a tile, a limit, a sentinel) is
a named constant. Exceptions: 0, 1 and -1 as plain arithmetic, and a value
explained by a comment on the same line where it is used once.

### H-CHECK-1 · must · Compile-time constraints use the check typedef
An assumption the code depends on (a size that must fit a byte, a power of
two, a limit below another) is checked where it is relied on with
`typedef char <name>_check[(<condition>) ? 1 : -1];`
and a comment saying why. When the condition is false, the array's size is -1, which is a compile error
Check: `_Static_assert` anywhere in `src/`, and run-time asserts of
conditions that are known at compile time.

## Documentation

### H-DOC-1 · should · Public headers carry Doxygen comments
A public header starts with a `/** @file ... @brief ... */` block that says
what the module is, how it is used and how it is banked. Every function,
type, macro and constant group it declares has a `/** @brief */` comment, or
`/**< */` after members and constants. Use `@param` and `@return` when a
name doesn't say it all, and always for sentinel returns ("or
`NGE_ENTITY_INVALID` if ...").

### H-DOC-2 · should · Internal headers and source files use plain comments
`_internal.h` files and `.c` files use `/* */` comments, not Doxygen. The
first comment in every `.c` file says what it holds and where it lives in
memory: common, a page number, slot 0 or slot 1.

### H-DOC-3 · should · Comments say why
A comment explains a reason, a rule or a trap, not what the next line
plainly does. A workaround names its cause (the zsdcc version, the
symptom), so it can be removed when the cause goes.

### H-DOC-4 · should · Paths in comments are written from src/
`game/level/level.h`, `ecs/nge_owner.h` (nge paths from `src/nge`, as
includes are), so they can be found without guessing.

### H-DOC-5 · should · Headers change with the code
A change of behaviour, parameter, return value or banking updates the
header comment in the same commit, and the module's row in
`src/game/README.md` or `src/nge/README.md` if the summary changes.
