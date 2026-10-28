# Toolchain: z88dk and zsdcc

Traps in the compiler and libraries used for the Next build
(`zcc +zxn -clib=sdcc_iy`). The PC tests are built with gcc and can't catch
any of these. See [README.md](README.md) for the format and strengths.

## Code generation

### T-ZSDCC-1 · must · Copy a struct's fields into locals in functions that branch on a field
In a function that branches on one field (a `switch` or `if` chain on
`e->id`) and, inside the branches, compares fields with globals
(`e->source == actor_player`) and then uses a field, copy the fields it
needs into locals before the branching: `nge_entity_id_t target = e->target;`.
Why: zsdcc (z88dk git 9576e21668, Oct 2026) can keep a field's address in
HL across the branches, then load a global's address into HL for a
compare (`ld hl,_actor_player; sub a,(hl)`) and still read the field
through HL afterwards, so it reads the global instead. It happens only when
the register allocator chooses HL for the field's address. That depends on
register pressure (it showed with two or more such branches, never with one),
so it can't be predicted from the source. The original case is in
`messages_on_event()`. Peephole and optimisation settings
(`-Cs--no-peep`, `-SO0`, `-SO3`) don't change it: it comes from SDCC's code
generator.
Check: event and action handlers that `switch` on a field. On the generated
code, a read through `(hl)` after `ld hl,_<global>`, a compare and a branch,
with nothing loading HL in between, is the miscompile, unless the source
reads that global again.

### T-ZSDCC-2 · must · Never return a comma expression
No `return (a->reason = r, ACT_FAIL);`, and no macro that expands to one
after `return`. Use statements, or a function (`act_fail()`).
Why: zsdcc keeps only the last operand of a comma expression in a `return`,
and drops every assignment before it, even to globals.

### T-ZSDCC-3 · should · When the Next differs from the PC tests, read the asm
If behaviour on CSpect differs from passing PC tests and the logic is right,
compile the file with `zcc ... -S` and read the code for the failing path
before changing the logic. Record a new miscompile as a T- rule.

### T-ZSDCC-4 · must · No apostrophe anywhere in an __asm block
Not even in a `;` comment: write the other register set as "the other set:
hl", not `hl'`, and "the column bits", not "the column's bits".
Why: the C preprocessor reads the `__asm ... __endasm` block as C tokens,
so a lone `'` opens a character constant that never closes. The error
comes later in the file and names something else (`syntax error: token ->
'/'`), so the cause isn't obvious (z88dk git 9576e21668, Oct 2026).
Check: `'` between `__asm` and `__endasm` in any `.c` file.

## Libraries

### T-INC-1 · must · Only newlib headers: never add the classic include path
C files compile with zcc's own include path (`include/_DEVELOPMENT/common`).
Never add `-I$(Z88DK_ROOT)/include`, in a Makefile or the editor's include
path.
Why: the classic headers declare calls like `memcpy` with `__smallc`
argument order, while `-clib=sdcc_iy` links newlib. `memcpy(dst, src, n)`
then ran as `memcpy(n, src, dst)` with no warning.

### T-LIB-1 · must · No heap and no stdio files
No `malloc`, `free` or `fopen`: both heaps are size 0 (see "The stack" in
`docs/memory_map.md`). Output goes through nge streams (`io/nge_stream.h`).

## Integers

### T-INT-1 · should · Divide and take remainders unsigned
Cast to `unsigned int` before `/` and `%` when the values can't be negative:
`(unsigned int)rand() % n`.
Why: signed division pulls in the slower signed routine, and its result
needs sign handling.

### T-INT-2 · should · Cast 8-bit arithmetic back where it is narrowed
Write `(uint8_t)(x + 1)` when an 8-bit result is stored or passed on.
Why: C promotes `uint8_t` arithmetic to `int`. The cast says the wrap is
intended, and lets SDCC keep the operation in 8 bits.

## Memory

### T-STACK-1 · must · Keep locals small
The stack is 512 bytes, shared by every nested call. A function's locals
stay under about 64 bytes: no large arrays or structs on the stack. Copies
of kind records on the stack (`gen_creature_kind_t b`) are the largest
allowed, and say so in a comment.
Check: local arrays and structs, in functions that are called deep in the
call chain.

### T-STACK-2 · must · Banked calls nest at most 8 deep
`banked_call` keeps 4 bytes per nested banked call in a 32-byte stack of its
own (`CLIB_BANKING_STACK_SIZE`). A chain of banked functions calling banked
functions must not go deeper than 8.
Check: when a banked function calls another module's banked function, count
the chain from the main loop.
