# Spells: design specification

How Jetset's spells work: what a spell is, who knows which, how one is cast and paid
for, and how new ones are learned.

Spells are cast with an action ([action_cast.md](action_cast.md)), do what an effect says
([effects.md](effects.md)) and cost mana ([mana.md](mana.md)). Character levels are in
[player_character.md](player_character.md), and how a monster chooses is in
[ai.md](ai.md).

Status: implemented 2026-11-03, in the 5 steps of [spells_plan.md](spells_plan.md). CSpect
checks 1 to 5, 7 and 8 passed; check 6 in part: the shaman casts and comes to melee out of
MP, and healing itself was checked only by the PC tests. Differences from the design are in
section 9. Proposed 2026-11-02; `SPELL_DIRECT` added 2026-11-03 with the line modes
(actions.md 4.11, action_cast.md D2).

**Spell slots and spell books** (to-do S1, S2, S3): implemented 2026-11-10, in section 10,
in steps 6 to 9 of [spells_plan.md](spells_plan.md), part 2. Every CSpect check of section
10.6 passed. Differences from the design are in section 10.9. It changes R9 to R11, D7 and
section 4.6: learning moves from level-ups to spell books.

## 1. Overview

**A spell is data.** Each is a row in a constant table, `spell_defs[]`, that says how
powerful it is, who it reaches and which effect it applies. Adding a spell means adding
a row. Only a new shape or a new kind of op needs code.

**A spell has an effect, and an effect is what it does.** Every spell names exactly one
`EF_*` and a power. The spell says who gets the effect and what it costs, and the effect
says what happens to them. So a wand of fire and Fire Bolt share a row of
`effect_defs[]`.

**Everyone casts by the same rules.** The player and monsters use one action, `ACT_CAST`,
one table of known spells and one mana component. Only how they choose differs: the
player from a list, a monster through the AI.

**A spell's level sets its cost.** Levels run from 1 to 5, and MP cost is worked out from
the level, so a higher spell always costs more. Level 0 is an ability, which costs
nothing.

**The player learns by levelling up.** A spell's row says at which character level it is
learned. Later ways of learning, such as a scroll or a shrine, call the same function.

```text
 player: m key --> spell list --> target_choose() --+
                                                    +--> act_cast()
 AI: opt_cast(), in the hunt state -----------------+       |
                                                            |
    context    spell_get(): the row, by a->spell            |
    validate   known? enough MP? a target for its shape?    |
                  refused --> reason set, nothing changed   |
                  ACT_CHECK stops here                      |
    resolve    nothing to roll                              |
    apply      EV_CAST                                      |
               mana_spend() ------> EV_MANA_SPENT           |
               effect_apply(ef, power, target, actor) ------+--> instant or timed

 b key: a book ------> ACT_LEARN (section 10) --+
 later: OP_LEARN -------------------------------+--> spell_learn() --> EV_SPELL_LEARNED
```

## 2. Terms

| Term | Meaning |
|---|---|
| Spell | A row of `spell_defs[]`, `SP_*`. `SP_NONE` is an empty entry in a list |
| Spell level | How powerful a spell is, 0 to 5. Not a character level, nor a depth |
| Ability | A spell of level 0: it costs nothing |
| Cost | The MP a cast takes, worked out from the spell level |
| Shape | Who a spell reaches: `SPELL_SELF`, `SPELL_BOLT`, `SPELL_DIRECT` or `SPELL_TILE` |
| Bolt | A spell sent along a line, stopping at the first creature (actions.md, D11) |
| Direct | A spell sent along a line to the creature on the chosen tile, past any on the way (actions.md, 4.11) |
| Known spells | The set of spells an entity can cast, in `COMPONENT_SPELLS` |
| Learn | Add a spell to the known set, with `spell_learn()` |
| Caster | An entity with `COMPONENT_SPELLS` |
| Power | As in effects.md: what the effect's amounts are multiplied by |

## 3. Requirements

### Definitions

- **R1** A spell is a row of data. A new spell needs code only if it needs a new shape or
  a new kind of op.
- **R2** Every spell names one effect, which is not `EF_NONE` and not worn (effects.md,
  R7). A spell has no other way of changing the game.
- **R3** A spell's strength is the power in its row. It doesn't depend on who casts it.
- **R4** A spell has a level from 0 to 5. Level 0 is an ability.
- **R5** Cost depends only on the level, and rises with every level above 0.

### Knowing and learning

- **R6** A caster has a set of known spells. The player and monsters keep it in the same
  table, through the same functions.
- **R7** A kind's record lists the spells its creatures start with. A kind that lists
  none doesn't get the component.
- **R8** `spell_learn()` is the only way to add a spell, whatever the cause. Learning a
  spell already known changes nothing and posts nothing.
- **R9 (Changed, S1)** ~~On reaching a character level, the player learns every spell
  whose row names that level.~~ Replaced by R36 and R46: a level-up gives spell slots, and
  spells are learned from books.
- **R10 (Changed, S1)** ~~The player's kind lists every spell of `learn` 1.~~ Replaced by
  R38: a class's starting spells fit its level 1 slots.
- **R11 (Changed, S1)** Spell level L can be learned at character level 2L - 1 or later,
  the pace of classic B/X. The slot table now holds this (R35).

### Casting

- **R12** Casting is one action, `ACT_CAST`, for every caster and every spell, and takes
  one turn (actions.md, R10).
- **R13** `a->spell` names the spell. It is a name for the byte `act_t` already has for
  a slot, a damage type or an effect, so the context doesn't grow, and `act_cast()`
  doesn't write to it.
- **R14** A cast is refused if the actor doesn't know the spell, can't pay for it, or has
  no target for its shape. A refused cast costs nothing and posts nothing.
- **R15** An ability needs no mana component and tests no MP.
- **R16** A valid cast always happens: there is no casting roll, and a bolt can't miss. A
  saving throw belongs to the effect (effects.md, R4).
- **R17** The effect's source is the actor, so a kill earns its XP (actions.md, D13).
- **R18** Casting at a creature other than the caster counts as attacking it, whatever
  the effect, so a monster reacts to a hold as it does to a hit.

### Mana

- **R19** Every entity with mana gets it back over time: 1 MP every 20 turns.
- **R20** Maximum MP becomes an attribute, `ATTR_MP_MAX` (player_character.md, R18).
- **R21** A new character level raises the player's maximum MP by 1d4 + the Intelligence
  modifier, at least 1, and current MP by the same.

### The player

- **R22** The `m` key lists the spells the player knows, with each one's level and cost,
  and a spell that can't be cast says why, as the inventory screen does.
- **R23** A bolt or a direct spell then asks for a tile with `target_choose()`. Cancelling at either step
  uses no turn.
- **R24** The player is told what they cast, what they see a monster cast, and each spell
  they learn.

### The AI

- **R25** A monster with spells tries them before anything else in the hunt state.
- **R26** A monster casts only what the action allows, so it can run out of MP and fall
  back on its other options.
- **R27** A monster holds a bolt that would hit something other than its target, as
  `opt_shoot()` does.
- **R28** A row says when the AI uses it, so a spell the AI can't use well is simply never
  chosen.

### Constraints

- **R29** The module is banked: its code in one page and its data in the next. Its callers
  map neither (effects.md, R19).
- **R30** The module adds no library routine to common memory ([memory_map.md](memory_map.md)).
- **R31** Check mode changes nothing and draws no random numbers (actions.md, R5).
- **R32** The largest cast stays within half the event queue (events.md, R25).

## 4. Design

### 4.1 The definition

`spell_def_t`, in `src/game/spells/spells.h`:

| Field | Meaning |
|---|---|
| `name` | Shown in the list and in messages, 14 characters at most |
| `level` | 0 to 5 |
| `shape` | `SPELL_SELF`, `SPELL_BOLT`, `SPELL_DIRECT` or `SPELL_TILE` |
| `ef`, `power` | The effect and its power |
| `range` | A bolt's or direct spell's reach, through `nge_map_within()`; 0 for `SPELL_SELF` |
| ~~`learn`~~ | Goes (S1, D18): spells are learned from books |
| `ai` | `SPELL_AI_NEVER`, `SPELL_AI_HURT` or `SPELL_AI_TARGET` |

`SP_NONE` is 0, and its row is empty. `SPELL_COUNT` is at most 32 (section 4.3).

### 4.2 Levels and cost

`spell_cost()` reads a table by spell level:

| Spell level | 0 | 1 | 2 | 3 | 4 | 5 |
|---|---|---|---|---|---|---|
| MP | 0 | 1 | 2 | 4 | 6 | 9 |

With R19 and R21, a player has about 27 MP at character level 10, so 3 casts of a level 5
spell. At level 1 they have 5 MP, which is 5 Magic Bolts and then a long wait. These are
starting values, to tune in CSpect.

### 4.3 Known spells

`COMPONENT_SPELLS` is the next component, `NGE_COMPONENT_USER + 12`, which takes
`COMPONENT_COUNT` to 13 of the 20 the game can have. Its table is the module's page data,
as the AI's and the effects' are:

```c
#define SPELL_SET_BYTES  4
uint8_t spell_known[NGE_ENTITY_MAX][SPELL_SET_BYTES];   /* page 135: 1 bit a spell */

#define SPELLS_IS(id)  NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_SPELLS)
```

The macro is in common reach, so `ai_can()` asks without a banked call.

| Function | What it does |
|---|---|
| `spell_add(id, list)` | Gives the component with the kind's list, posting nothing |
| `spell_knows(id, sp)` | 1 if `id` is a caster and knows `sp` |
| `spell_learn(id, sp)` | Sets the bit and posts `EV_SPELL_LEARNED`, unless already known |
| ~~`spell_learn_level(id, level)`~~ | Goes (S1, D18), with `learn`. Section 10.5.2 adds the slot and book functions |
| `spell_next(id, sp, ai)` | The next known spell after `sp`, wrapping, with that `ai` value, or `SP_NONE`. `ai` may be `SPELL_AI_ANY` |
| `spell_get(sp, def)` | Copies a row into `*def`, on the caller's stack |
| `spell_cost(level)` | The MP for a spell level |
| `spell_name(sp, buf, size)` | Copies a name, for the messages and the list |

`gen_creature_kind_t` gains `uint8_t spells[GEN_SPELLS_MAX]`, 4 entries, `SP_NONE` for
unused. `create()` calls `spell_add()` if the first entry isn't `SP_NONE`. The player's
record lists Magic Bolt, so a new game posts no `EV_SPELL_LEARNED` (R10).

**A bit, as bytes.** Bit n is `spell_known[id][n >> 3]` and mask `(uint8_t)(1u << (n & 7))`.
The cast matters: zsdcc makes a constant `1 << 7` -128, and the PC tests miss it.

### 4.4 `act_cast()`

The action is specified in [action_cast.md](action_cast.md): its inputs, steps, reasons,
events and callers. It meets R12 to R18 here. Its additions are `ACT_CAST` 15,
`ACT_R_NO_SPELL` and `ACT_R_NO_MANA`, with the texts "You don't know that spell." and "You
haven't the mana."

### 4.5 Shapes

| Shape | Target | Who reads x, y |
|---|---|---|
| `SPELL_SELF` | The actor | Nobody |
| `SPELL_BOLT` | The first creature on the line, from `act_line()` with `ACT_LINE_BOLT` (actions.md, 4.11) | `act_line()` |
| `SPELL_DIRECT` | The creature on the chosen tile, past any on the way, from `act_line()` with `ACT_LINE_DIRECT` (actions.md, 4.11) | `act_line()` |
| `SPELL_TILE` | The caster, sent to a free tile in range and line, never a creature's (`effect_apply_at()`, effects_extensions.md 3.6) | `act_line()`, then the effect |
| `SPELL_BALL` (I3, proposed) | Every creature within 1 of the chosen tile, the caster too, from `act_line()` with `ACT_LINE_DIRECT` ([items_aimed.md](items_aimed.md)) | `act_line()`, then the area walk |
| `SPELL_BURST` (I3, proposed) | Every creature within 2 of the caster, not the caster | Nobody |

Items share these shapes (I3): an aimed item's shape is in its kind's flags (items.md 4.12).

A wall or closed door stops either line. A bolt suits what flies, such as fire; direct suits
what doesn't, such as a hold or a teleport of another (action_aim.md, D3). In section 4.12,
Hold and Enfeeble are direct, as the wands of sleep and slowness are, and the damaging spells
are bolts.

A bolt or a direct spell can't target its caster. Aim allows that for a staff of healing, but a spell that
helps the caster is `SPELL_SELF`, so the player gets no prompt. Helping another creature
isn't possible yet, and no monster has an ally to help.

### 4.6 Learning

**Changed (S1):** this section describes the build of 2026-11-03. Section 10.5 replaces
it: `level_up()` no longer calls `spell_learn_level()`, which goes with the `learn` field.
`OP_LEARN` stays the way for a scroll or shrine to teach a spell.

`level_up()` in [pc.c](../src/game/components/pc.c) calls `spell_learn_level(id, pc_level)`
directly, after the level has gone up. It runs inside `EV_DIED`'s handler or in
`pc_gain_gold()`, so a chooser is impossible there, and learning is automatic (D7).

A scroll or a shrine needs only a new op, `OP_LEARN`, with the spell as its `arg`, and an
effect row for each spell taught. That is section 6, and nothing here needs to change.

### 4.7 Mana

**Getting MP back.** `EF_MANA_REGEN` is a timed effect: innate, for ever, `on_tick`
`{ OP_MANA, 0, 1 }`, period `TURNS(20)`, named "Mana flow". A kind has only one innate hook
and the player's is `EF_REGEN`, so the generator applies it as a rule: any kind with mana
gets it, straight after its innate hook. A shaman therefore recovers too, slowly. Being a
timed effect, it is numbered below `EF_TIMED_COUNT`, which becomes 16.

**The maximum.** `mana_mp_max_base[]` joins `mana_mp_max[]` in slot 0, with `ATTR_MP_MAX`,
a row of `effects_game_attrs[]` and a part of `effects_game_store()`, behind a test for the
component, as speed has.
A lower maximum takes current MP down with it (mana.md, D2).

**A new level.** After the hit points, if the player has mana:

```c
gain = (int16_t)(1 + (rand() & 3)) + STATS_MOD(stats_ability[ABIL_INT][id]);   /* at least 1 */
effects_set_base(id, ATTR_MP_MAX, ...);
mana_restore(id, (uint8_t)gain);                                               /* EV_MANA_RESTORED */
```

The player's class doesn't matter: `pc_class` is still a placeholder, and every character
can cast until classes come (section 6).

### 4.8 The player

`m` is free. `cast_command()` in [player_bank.c](../src/game/player/player_bank.c):

```text
 no spells known --> "You know no spells."
 screen_spell_run(&sp), a chooser --> back: no turn
 a check on the player's own tile, in the list: refusal shows on the status line
 SPELL_BOLT, SPELL_DIRECT --> choose_tile(TARGET_CAST, range, &a) --> cancelled: no turn
 player_act(ACT_CAST)
```

`screen_spell_run()` is new, in page 104, beside the PC screen: lettered rows of name, level
and cost, the MP left at the foot, and a spell that can't be cast shown dim with its reason
on the status line. It lists spells, not entities, so `screen_inv_run()` can't serve.
`TARGET_CAST` adds a purpose and prompt to `target_bank.c`.

### 4.9 The AI

`ai_can()` sets `AI_CAN_CAST` for `SPELLS_IS(id)`. `opt_cast()` comes first in `think_hunt()`:

```c
return opt_cast(c) || opt_attack(c) || opt_drink(c) || opt_shoot(c) || opt_approach(c);
```

```text
 opt_cast(c):
   not AI_CAN_CAST --> 0
   hurt (HP at half or less, as opt_drink()):
       each SPELL_AI_HURT spell known: ai_try(ACT_CAST) --> cast: 1
   sees its target:
       start at a random spell (rand() & 31)
       each SPELL_AI_TARGET spell known, from there:
           check: passes, and a.target is the target --> cast: 1
   0
```

The random start gives a shaman some variety for no extra rule, and draws outside the action,
so R31 holds. It may refresh a timed effect already on the target, which wastes MP and
nothing else (D9).

A rooted caster still casts, as nothing in the action tests rooted.

### 4.10 Events and text

| Event | Target | Source | Payload |
|---|---|---|---|
| `EV_CAST` | The entity aimed at, or the caster | The caster | p1: `SP_*` |
| `EV_SPELL_LEARNED` | The entity | None | p1: `SP_*` |

Both go at the end of the list in `events.h`.

| Handler | `EV_CAST` | `EV_SPELL_LEARNED` |
|---|---|---|
| `messages_on_event()` | "You cast Magic Bolt.", or "The goblin shaman casts Magic Bolt." if seen | "You learn Magic Bolt.", for the player only |
| `ai_on_event()` | `AI_STIM_HURT`, if target and source differ (R18) | |

**Event order:** `EV_CAST`, `EV_MANA_SPENT`, then the effect's. It reads as "You cast", then
the result, and MP is spent before the effect, so a spell can't restore what paid for it and
then be charged.

| Case | Events |
|---|---|
| A bolt that kills | 5: `EV_CAST`, `EV_MANA_SPENT`, `EV_DAMAGED`, `EV_DIED`, `EV_EFFECT_STARTED` |
| A timed effect | 4: `EV_CAST`, `EV_MANA_SPENT`, `EV_EFFECT_STARTED`, `EV_ATTRS_CHANGED` |
| A new level | 5, up from 3: adds `EV_MANA_RESTORED` and `EV_SPELL_LEARNED` |

A level-up's events come from one handler call and can't drain, so 3 levels from one gain
would post 16 with `EV_XP_GAINED`, which fills today's queue. `NGE_EVENT_QUEUE` becomes 32,
for 80 bytes of common memory (D10).

### 4.11 Banking and memory

`src/game/spells/` is pair 134/135, the next free, with an origin in `banks.inc`.

| Where | What | About |
|---|---|---|
| Page 134 | `spells_bank.c`: the functions | 0.5K |
| Page 135 | `spells_pdata.c`: `spell_defs[]`, names, costs, `spell_known[]` | 0.1K + 400 bytes |
| `BANK_56` | `act_cast.c`, on `act_aim.c`'s 706 bytes | 0.7K of 6.1K free |
| Page 104 | The spell list | 1K of 3.9K free |
| Page 110 | `level_up()` and `mana.c` | 0.1K of 1.1K free |
| Page 107 | 4 bytes for each of 21 creature kinds | 84 bytes |
| Page 114, 118, 124, 130 | The command, messages, a prompt, `opt_cast()` | A few hundred bytes each |
| Page 121 | `EF_MANA_REGEN` | 30 bytes |
| Slot 0 | `mana_mp_max_base[]` | 100 of 1.5K free |
| Common | `act_do()`'s case, 2 reason bytes, `ai_on_event()`'s case, the queue | About 110 bytes |

A new `ACT_*` also adds 7 bytes to `modes[]` in page 116 (actions.md, 4.9).

Random numbers use a mask, not `%`, and no code here multiplies, so nothing new links into
common memory (R30). The map must be compared before and after.

### 4.12 The first spells

**Changed (S2):** the "Learned at" column goes with the `learn` field. Each spell is in a
book instead (section 10.5.3).

Only effects that work today, and one spell for each character level:

| Learned at | `SP_*` | Name | Level | MP | Shape | Range | Effect, power | AI |
|---|---|---|---|---|---|---|---|---|
| 1 | `SP_MAGIC_BOLT` | Magic Bolt | 1 | 1 | Bolt | 8 | `EF_BOLT_MAGIC`, 3 | Target |
| 2 | `SP_CURE_WOUNDS` | Cure Wounds | 1 | 1 | Self | | `EF_HEALING`, 4 | Hurt |
| 3 | `SP_HOLD` | Hold | 2 | 2 | Direct | 6 | `EF_HOLD`, 1 | Target |
| 4 | `SP_ENFEEBLE` | Enfeeble | 2 | 2 | Direct | 6 | `EF_WEAKNESS`, 2 | Target |
| 5 | `SP_FIRE_BOLT` | Fire Bolt | 3 | 4 | Bolt | 8 | `EF_BOLT_FIRE`, 8 | Target |
| 6 | `SP_FROST_BOLT` | Frost Bolt | 3 | 4 | Bolt | 8 | `EF_BOLT_COLD`, 8 | Target |
| 7 | `SP_STONE_SKIN` | Stoneskin | 4 | 6 | Self | | `EF_STONE_SKIN`, 3 | Never |
| 8 | `SP_HASTE` | Haste | 4 | 6 | Self | | `EF_HASTE`, 3 | Never |
| 9 | `SP_POWER_BOLT` | Power Bolt | 5 | 9 | Bolt | 8 | `EF_BOLT_MAGIC`, 16 | Target |
| 10 | `SP_RENEWAL` | Renewal | 5 | 9 | Self | | `EF_RENEW`, 2 | Hurt |
| - | `SP_BLINK` | Blink | 2 | 2 | Tile | 6 | `EF_BLINK`, 1 | Never (effects_extensions.md R22) |

The powers follow the items: a wand of striking is 4, a wand of fire 6, a staff of power
12, a healing draught 3 and a potion 6.

**Creatures.** The goblin shaman knows Magic Bolt, Cure Wounds and Enfeeble, with its 6 MP.
It is the only caster for now, and its "later: casts" comment goes.

### 4.13 Changes to other specs

Made when this is built:

| Spec | Change |
|---|---|
| actions.md, action_cast.md | The action's spec in action_cast.md; its rows in the tables of actions.md 4.1, 4.2 and 4.6 |
| effects.md | `act_cast()` among the callers of `effect_apply()`; `OP_LEARN` when it comes |
| mana.md | R13, R14 and R15 done; D2's attribute built |
| player_character.md | R18 done; `level_up()`'s two additions |
| events.md | Sections 4.3, 4.6, 4.7 and 4.9; the queue of 32 moves out of "Not built yet" |
| ai.md, ai_default_brain.md | `AI_CAN_CAST`, `opt_cast()`, R18's stimulus |
| targeting.md | `TARGET_CAST` |
| attributes.md | `ATTR_MP_MAX` |
| creatures.md | The shaman |
| memory_map.md | Pair 134/135; slot 0 |
| `src/game/README.md` | The module, the component |

## 5. Decisions

- **D1 `a->spell` names the spell, and `act_t` gains no byte.** `act_t` has one byte with a
  name for each use, in an anonymous union: `slot`, `damage`, `effect` and now `spell`, with
  `kind` for code that doesn't care which. It is already an input: equip reads `slot`. A
  number needs one table, and the action reads everything else from the row, so the caller
  can't choose a cost or a power. *Rejected:* `effect` and `power` fields, which let a
  caller cast anything, leave cost and level with no source, and grow every action's
  context on the stack. *Rejected:* a `spell` byte of its own, a byte on every context for
  one action, and another for prayers. *Rejected:* `kind` alone, where every use needs a
  comment to say what it holds. *Rejected:* a named union, `a.u.spell`, which changes every
  use for nothing an anonymous one doesn't give.
- **D2 Prayers are rows in the same table.** A prayer differs from a spell in who knows it
  and perhaps what pays for it, not in how it reaches its target or what it does. So when
  they come, a `school` field and one test in `act_cast()` are enough, and `ACT_CAST`, `spell`
  and the callers stay. *Rejected:* adding `school` now, with one value. *Rejected:* an
  `ACT_PRAY`, an action that differs only in its data (action_cast.md, D1).
- **D3 Every spell is an effect.** There is one way to change a creature, and no spell needs
  code of its own. Self-targeted ops cover the spells that don't look like effects: light,
  mapping, teleport, summon and a shrieker's shriek. *Rejected:* a spell op of its own beside
  the effect, which would be a second copy of `effect_defs[]`. Three limits follow:
  - An effect says what happens to one target, not who the targets are, so the row needs a
    shape (R2, section 4.5).
  - Effects act only on creatures (effects.md, R6), and `effect_apply()` takes no tile, so
    spells on a door, a wall or a tile have no legal target yet (section 6).
  - Amounts are flat, amount x power, with no dice. A wand is the same today, and it costs a
    spell some variety and no complexity.
- **D4 Cost comes from the level, not the row.** It makes "higher costs more" impossible to
  break (R5), and a row a byte smaller. *Rejected:* a `cost` field, where a row could be
  dearer than one above it, and whose only use would be exceptions no spell needs.
- **D5 Power is fixed by the row.** Monsters have no level, so scaling by caster level would
  be a player's rule in a shared mechanic. A stronger spell is a higher spell, which is what
  spell levels are for. *Rejected:* power rising with `pc_level`, which adds a multiply and
  makes Magic Bolt outgrow Fire Bolt's place in the list.
- **D6 Known spells are one bit set per caster, in the module's page data.** The set is
  needed by both the player and monsters (player_character.md, R1), and a table by entity
  keeps one code path. Bytes avoid 32-bit shifts, which risk a library routine in common
  memory. *Rejected:* an entity per known spell, like items, which spends entities out of
  100. *Rejected:* a kind's mask for monsters and a variable for the player, which saves 400
  bytes of a nearly empty page for two code paths. *Rejected:* slot 0, which has less room
  and no reader outside this module.
- **D7 (Replaced by D18) The player learns automatically.** `level_up()` runs in an event handler, and no
  chooser can run there (events.md, section 4.4). The list is short, so a choice would be
  little to give up. *Rejected:* a pending choice held until the next turn, which needs a
  new state for a decision the first list doesn't have.
- **D8 `spell_learn()` is the only way in,** so a scroll or shrine posts the same event and
  gets the same message. Learning becomes an op, so those causes are data.
- **D9 The AI casts from a random start and checks nothing about the effect.** Asking whether
  an effect is already on the target needs a new function in the effects module, and the cost
  of not asking is a refreshed effect and some MP, which a monster has little of anyway.
  *Rejected:* highest level first, which is all a shaman would ever cast.
- **D10 The queue goes to 32 now.** Spells add 2 events to a level-up, and all come from one
  handler call. Events.md expected to need this for the first action touching many entities,
  and this gets there first. *Rejected:* gaining one level a kill, which changes
  player_character.md R9 to fit a buffer.
- **D11 Maximum MP becomes an attribute.** R18 said so, and level-up then matches hit points
  line for line. It costs 100 bytes of slot 0, and a ring of mana is then data. *Rejected:*
  raising `mana_mp_max[]` directly, as mana.md D2 allows until an item needs more, which
  saves those bytes and goes against an agreed requirement.
- **D12 Mana regeneration is a rule of the generator, not a hook.** A kind has one innate
  hook and the player's is used. A rule covers every kind with mana, so no kind can forget.
  *Rejected:* a second innate hook, 2 bytes on every kind for one row. *Rejected:* a counter
  in the component, when an innate effect already does this for hit points. Each caster
  spends an entity on it.
- **D13 No casting roll.** Nothing in the objectives asks for failure, and a wand has none.
  Saves are the effect's. *Rejected:* a roll against Intelligence, which needs numbers for a
  class system that doesn't exist.
- **D14 A cast at another creature is an attack (R18).** A hold does no damage, so
  `EV_DAMAGED` wouldn't wake its target. `EV_CAST` does it, from one test in `ai_on_event()`.
  *Rejected:* posting `EV_HIT`, which carries a roll and would lunge the caster.
- **D15 Abilities are level 0, in this table** (action_cast.md, D1). The rule costs one test, so
  it is here from the start, though no kind uses it yet (section 6).
- **D16 The first list uses only working effects.** A spell that takes MP and does nothing
  reads as a bug. *Rejected:* adding pending rows as items do (items.md, R12).

## 6. Not built yet

| Item | What's there, what's missing |
|---|---|
| Area spells | `SPELL_BALL` and `SPELL_BURST` proposed by I3 ([items_aimed.md](items_aimed.md)): a shared walk, at most 8 targets, a ball catching its caster. Needs rows, and an AI rule for catching itself |
| Abilities in the creature table | The rule is here. A floating eye's gaze is a row. Level 0 is free, so the AI needs a rate, or it uses one every turn |
| A dragon's breath | A bolt ability works now. Real breath needs an area |
| A shriek | A self-targeted effect with an op that wakes the level |
| Spells on doors, walls and tiles | Knock, stone to mud, webs. Needs an op that reads the caster's surroundings, or a tile passed to `effect_apply()` (D3) |
| Prayers | `school` and a list of their own (D2) |
| Learning from scrolls and shrines | `spell_learn()` is ready. Needs `OP_LEARN`, its rows, and items or features that name them. Should it obey slots (section 10)? |
| Forgetting | No rule needs it |
| Classes | Slots depend on the class (section 10). Maximum MP gained at a level doesn't yet |
| Spell variety | Dice in effects, power by level, a chance to fail |
| Spells that help others | `SPELL_DIRECT` can reach another creature. Needs allies |
| A projectile animation | `EV_CAST` carries what it needs (animations.md, R14) |
| Buffs in the AI | Stoneskin and Haste are `SPELL_AI_NEVER`, as a monster would recast them every turn |
| The PC screen | A count or a list of spells known |

Spells waiting for a place in the list, or a rule:

| Spell | Effect | Waits for |
|---|---|---|
| Slow | `EF_SLOW` | Nothing: a level-up slot, or scrolls |
| Antidote | `EF_CURE` | The same |
| Purify | `EF_CURE_ALL` | The same |
| Remove Curse | `EF_UNCURSE` | The same |
| Fire Ward, Frost Ward | `EF_RESIST_FIRE`, `EF_RESIST_COLD` | The same |
| Might | `EF_MIGHT` | The same |
| Sleep | `EF_SLEEP` | A rule that reads `STATUS_UNCONSCIOUS` (attributes.md, "Not built yet") |
| Blind | `EF_BLIND` | Nothing: a level-up slot, or scrolls (blinded is -4 to hit, `EF_BLIND`'s `ATTR_TO_HIT` modifier) |
| Teleport, Light, Magic Map, Identify, Summon, Enchant | Their rows | Their pending ops (effects.md, 4.8) |

## 7. Tests

**`test_spell`** (new):

- the definitions: every row well formed, with an effect that isn't worn, a level in range,
  a range only on bolts, a name that fits, and `SPELL_COUNT` within the set;
- cost rising with every level (R5), and `learn` obeying 2L - 1 (R11);
- a kind's list, and the player starting with Magic Bolt;
- `spell_learn()`: the event, a second learn posting nothing, and every bit of all 4 bytes;
- `spell_learn_level()`: each level, and `learn` 0 never learned;
- `spell_next()`: wrapping, the `ai` filter, and none known;
- an entity ID reused: a new creature has no old spells;
- the asserts.

**`test_act`:**

- each refusal, in order, with nothing changed and no events;
- a self spell, a bolt, MP spent, and the event order;
- a kill naming the actor, so XP follows;
- a creature in the way, a wall, range exactly, and the own tile;
- an ability on an entity without mana;
- the MP left at exactly the cost;
- a check changing nothing, including `rand()`, and `spell` unchanged;
- a timed effect with no entity free: the MP are spent (section 8);
- `act_do()`, the reasons' kinds, and the peak of the largest cast (R32).

**`test_ai`:** `AI_CAN_CAST`; a shaman bolting, healing when hurt, and falling back when out
of MP; holding fire with a creature in the way; a hold waking a sleeper, and a self cast not.

**`test_pc`:** a level adds MP and the spell, with their events, and a level with no mana
component; 3 levels from one gain inside half the queue.

**`test_mana`, `test_effects`:** `ATTR_MP_MAX` with its clamp, `EF_MANA_REGEN` ticking and
stopping at the maximum, and innate, so a cure doesn't end it.

**`test_gen`:** the shaman's list, and mana regeneration on every kind with mana.

**`test_ui`:** the messages, the reasons' texts and the `m` key. `test_target`: `TARGET_CAST`.

**CSpect, by the user:**

1. A new game: `m` lists Magic Bolt; MP shows on the screen and goes down by 1 when cast.
2. A bolt at a monster: the cursor is green to range 8, the damage shows, and XP comes with a
   kill.
3. With no MP: the list says why, and no turn passes.
4. Waiting 20 turns gives 1 MP back.
5. `E` for a level: "You learn Cure Wounds.", and MP goes up.
6. A goblin shaman casts, heals itself, and runs out and comes to melee.
7. A sleeping monster hit by Hold wakes.
8. Common memory free, before and after, from the map.

Results (2026-11-03): 1 to 5 and 7 passed. 6: casting at range and melee out of MP passed;
healing itself and holding a bolt were not seen in CSpect. 8: 4,611 bytes free before step 1
and 4,429 after step 4, 182 less, of which 80 are the queue, with no new library routine.

## 8. Known risks

- **MP lost.** If no entity is free for a timed effect, `effect_apply()` refuses it and the
  MP stay spent. This breaks actions.md R3 in the way `act_use()` and `act_aim()` already do
  (actions.md, section 8), and is accepted for the same reason.
- **Entities.** Each caster holds one more active effect for good, so a level of shamans takes
  a few more of the 100. A generator that finds no entity free leaves a caster without
  regeneration, which matters no more than a monster losing `EF_REGEN`.
- **The queue.** 32 holds about 6 levels from one gain, with its 5 events each. Going from level
  1 to level 8 on one dragon would still overflow it (events.md, section 8). A level 1 character
  killing a dragon isn't a case that needs designing for, and a debug build says so if it comes.
- **XP from timed effects.** A kill by an effect that ticks names the effect, not the caster
  (effects.md, R11), so it earns nothing. No first spell does this. A poison spell would.
- **`source` on a timed effect.** `effect_apply()`'s `source` is ignored for timed effects
  (effects.md, section 8), so R17 holds only for instant ones. No hold or curse needs it.
- **`EV_AIMED` has the same gap as R18.** A wand of slow at a sleeper doesn't wake it. This is
  today's behaviour, and this spec leaves it alone.
- **`ATTR_COUNT` goes to 13.** The build must check that nothing holds a set of attributes in
  8 bits, and that `check-effects-core` still passes.
- **Negative power.** `effects_game_op()` casts a scaled amount to `uint8_t` (effects.md,
  section 8). A row must not give an op a negative power: `test_spell` holds this.
- **Balance.** Every number in sections 4.2, 4.7 and 4.12 is a guess until played, and
  `rand()` differs on the PC, so only CSpect can settle them.
- **Check cost.** `opt_cast()` may run up to 4 checks a turn, each walking a line. `ai_can()`
  and sight come first, and only kinds with spells pay (actions.md, section 8).
- **Banked depth.** A level-up now reaches `spell_learn_level()` and `spell_learn()` from
  inside a handler, at the depth of the 6-level chain through `effects_set_base()`. Count it
  again against T-STACK-2 (events.md, section 8).
- **zsdcc.** `act_cast()` and `opt_cast()` read a copied row in code that branches, so fields
  go into locals first (T-ZSDCC-1). Bit masks need the cast in section 4.3, and the PC tests
  miss both.

## 9. As built

Differences from the design above, step by step ([spells_plan.md](spells_plan.md)).

**Step 1, mana (2026-11-03).**

- Setting the maximum MP posts its own `EV_ATTRS_CHANGED`, as hit points' does. With
  step 2, a level-up posts 6 events, not the 5 of section 4.10: `EV_ATTRS_CHANGED`,
  `EV_HEALED`, `EV_ATTRS_CHANGED`, `EV_MANA_RESTORED`, `EV_LEVEL_GAINED`,
  `EV_SPELL_LEARNED`. 3 levels from one gain post 19 with `EV_XP_GAINED`: more than half
  the queue, which the handler's call can't drain, and within the 32 with the kill's own
  events. A queue of 32 holds 5 levels from one gain, not 6 (section 8).
- `PC_MANA_DIE` (4) names the die, beside `PC_HIT_DIE`.

**Step 2, the module (2026-11-03).**

- `spell_add()` takes a count with the list: `spell_add(id, list, count)`, so the module
  doesn't need the generator's `GEN_SPELLS_MAX`.
- `spell_learn_level()` runs after `EV_LEVEL_GAINED` is posted, so the messages read
  "Welcome to experience level 2." and then "You learn Cure Wounds.".
- `spell_next()` takes a spell below `SPELL_COUNT`, not any of 0 to 31. The AI's random
  start (section 4.9) is made to fit in step 4.
- Page 134 is about 1.5K with asserts, not 0.5K, and page 135 0.6K.

**Step 3, `ACT_CAST` and the `m` key (2026-11-03).**

- No dim colour: the text has four inks. A spell that can't be cast now is red in the
  list, and choosing it gives the reason on the status line, as the inventory does.
- The list shows every spell known on one screen, with no scrolling: 10 at most.
- The help line of the play screen is full, so it doesn't list `m`.
- Abilities aren't tested in `test_act`: no row has level 0 yet.
- `act_cast()` is 612 bytes of `BANK_56`, and page 104 gained 1.7K, not 1K.
- zsdcc miscompiled the list's first draw: `i < w->list.count` added the field's offset to
  HL again on each pass, so only the first spell was drawn until the cursor moved. The
  bound is copied into a local first (T-ZSDCC-3). The PC tests can't see it; CSpect did.

**Step 4, monsters that cast (2026-11-03).**

- The random start is `rand() & 15`, with 11 to 15 folded back to 1 to 5, not
  `rand() & 31`: `spell_next()` takes only spells below `SPELL_COUNT`. Spells 1 to 5 are
  twice as likely to be tried first, which matters little while a caster knows 2 or 3.
- `opt_cast()` and `opt_drink()` share `hurt()`, half hit points or less.
- A debug build places a goblin shaman out of sight, asleep, beside the dev bandit
  (`level_dev_bandit()`), for the CSpect check.
- Page 130 grew by 545 bytes; common memory is unchanged.


## 10. Spell slots and spell books (to-do S1, S2, S3)

Status: implemented 2026-11-10, in steps 6 to 9 of [spells_plan.md](spells_plan.md), part
2; CSpect checks 1 to 8 of section 10.6 passed. Proposed the same day; Q1 to Q4 answered
then (section 10.8), Q5 to Q7 still open for tuning in play. Differences from the design are
in section 10.9. New spells are a later step: this part only uses the 10 spells of section
4.12.

### 10.1 Overview

**A level-up gives room, a book gives spells.** A spell caster has a number of **spell
slots** at each spell level, read from a table by class and character level. Each spell
known fills one slot of its level. Spells are learned only by reading a **spell book** the
player carries, and only into a free slot. A level-up no longer teaches anything: it may
add slots, and says so.

**Everything reuses what is there.** A book is an item kind with a new subclass, and its
`data.book_spells` byte names its list of spells. Learning is an action, `ACT_LEARN`, which ends in
`spell_learn()`, so the message and event are the ones a level-up used to give. The book
screen is the spell list in a second mode. Choosing a book is the inventory screen in a new
mode, as for every other item command.

```text
 b key --> screen_inv_run(ACT_LEARN): a book in the pack --+
 browse: b, or ENTER on a book ----------------------------+
                                                           v
            screen_book_run(book): the book's spells, coloured known / can learn / can't
                 letter --> act_do(ACT_LEARN, book, spell) in the screen
                               validate: carried? a book? a caster? in the book?
                                         known? a slot at that level? one free?
                               apply:    spell_learn() --> EV_SPELL_LEARNED
                 the screen stays open; SPACE closes: one turn if anything was learned

 level_up() --> EV_LEVEL_GAINED --> messages: "Welcome to experience level 3."
                                   and, if the level adds a slot,
                                   "You can learn more spells: read a spell book."
```

### 10.2 Terms

| Term | Meaning |
|---|---|
| Spell slot | Room for one known spell of one spell level |
| Slot table | `spell_slots_table[]`: slots by class, character level and spell level |
| Free slot | A slot of a spell level with no known spell in it: slots less spells known at that level |
| Spell book | An item of subclass `ITEM_BOOK`. Its kind's `data.book_spells` is a `SPELL_BOOK_*` |
| Book list | A book's spells, a row of `spell_books[]` |
| Read | Open a book's screen. Learning from it is the action |

### 10.3 Requirements

To-do features are in brackets. **New** or **Changed** marks a requirement that differs
from the to-do list, with the reason.

#### Spell slots (S1)

- **R33 [S1.1]** A table gives the number of spells the player can know at each spell
  level, 1 to 5. Every known spell of a level fills a slot of that level, the starting
  spells included.
- **R34 [S1.2]** The table is indexed by class and character level.
- **R35 [S1.3]** Slots never fall as the character level rises, and the first slot of spell
  level L comes at character level 2L - 1 (R11). A test holds both.
- **R36 [S1.4]** Only a player character with the mana component has slots. The Fighter has
  none and can't learn. The Dev has the Mage's table (Q4).
- **R37 [S1.4b]** Monsters don't learn and have no slots: they know their kind's list, as
  now (R7). Slots are a player rule only.
- **R38 (New)** A class's starting spells fit its level 1 slots. A test holds this.
  *Reason: otherwise a new character would start over its own limits.*
- **R39 [S1.5]** When a level-up adds a slot at any spell level, the player is told they
  can learn more spells, after the "Welcome to experience level" message.

#### Spell books (S2)

- **R40 [S2.1]** A new item subclass, `ITEM_BOOK`. Books are made, carried, dropped and
  named as other items are. They can't be used (`u`), worn or aimed.
- **R41 [S2.1b]** One new sprite pattern, shown in a different colourway for each book.
- **R42 [S2.2]** A book holds a fixed list of spells, from its kind. Every spell of level 1
  or more is in at least one book. A test holds this.
- **R43 [S2.3-S2.5] (Changed)** Three books: the **basic spellbook** (spells of levels 1
  and 2), the **adept's spellbook** (levels 3 and 4) and the **master's spellbook** (level
  5). *Reason: a name has 19 characters at most, and "intermediate spellbook" has 22.*
- **R44 [S2.6]** The Mage and the Dev start with a basic spellbook, carried.
- **R45 (New)** Books are found in the dungeon, each from about the depth at which the
  player gets slots for its spells (the target level curve, [difficulty.md](difficulty.md)
  section 6). *Reason: a Mage who finds no book never learns again.*

#### Learning from books (S3)

- **R46 [S3.1]** The player reads a book they carry: `b`, or `b` or ENTER on a book in the
  inventory. Spells are learned only from books, never on a level-up (Q1).
- **R47 [S3.2]** The book screen lists every spell in the book, with its level and cost,
  coloured: known, can be learned now, can't be learned. Choosing one that can't be learned
  says why on the status line. The foot shows the free slots at each spell level.
- **R48 [S3.3] (Changed, Q3)** The player chooses a spell to learn, and may learn several
  before closing the book. Closing it takes one turn if anything was learned, and none if
  not. *Reason: the user's answer to Q3.*
- **R49 [S3.4]** A spell is learned only into a free slot of its level. The reasons a spell
  can't be learned are: known already, no slot at its level yet, and every slot of its level
  full.
- **R50 (New, Q2)** A book isn't used up. *Reason: the player comes back to it when a
  level-up adds slots.*
- **R51 (New)** A book is read only from the pack, not from the floor. *Reason: it is an
  item action, and every item action takes an item the actor carries.*

#### Constraints

- **R52** As R29 to R31: the slot table and book lists are in page 135, nothing new links
  into common memory, and `ACT_LEARN`'s check changes nothing and draws no random number.

### 10.4 Decisions

- **D17 Slots count known spells; there is no memorising.** A slot is room to know a spell,
  as S1.1 says, and a known spell can be cast as often as MP allows, as now. *Rejected:*
  B/X memorising, where a slot holds one cast a day: the game has no rest, and MP already
  limits casting.
- **D18 Books only (Q1).** Learning has one way in for the player, and a level-up only adds
  room. `spell_learn_level()` and the `learn` field go. The player's starting spells stay
  in the class's creature kind (R7). *Rejected:* both ways, where an automatic spell could
  take a slot the player wanted for another.
- **D19 The slot table is in the spells module, by `PC_CLASS_*`.** It is spell data, and
  the module reads `pc_class` and `pc_level`, which are in common memory. 3 classes x 10
  levels x 5 spell levels is 150 bytes of page 135. *Rejected:* a slots field in the
  chargen class record: page 140 would have to be called from inside an action, and the
  table would be split from the rule that reads it. *Rejected:* a formula (a curve with a
  rate per class): S2.7 of the old list asked for tables, and a table can be tuned a cell at
  a time.
- **D20 A book's kind names its list through `data.book_spells`.** The byte keys and lock
  picks already used, now a named union with a reader per meaning, `gen_item_book_spells()`
  for books ([items.md](items.md), section 4.7). The lists are in the spells module, `spell_books[]`,
  by `SPELL_BOOK_*`, so a book of fire spells later is a row and an item kind. *Rejected:* a
  spell level range in the book, which needs no list: every new spell would join a book
  whether wanted or not, and a book of a theme couldn't be made. *Rejected:* a list in the
  item kind, which grows all 163 kinds for 3 of them.
- **D21 Learning is an action, `ACT_LEARN`.** In [action_learn.md](action_learn.md), D1.
- **D22 Several spells in one reading, one turn (Q3).** The book screen does each
  `ACT_LEARN` itself and stays open, and closing it uses a turn if one was learned: the
  inventory's way with equip and drop in browse mode (ui_inventory.md). `a->used` is set
  by each learn, and the screen keeps whether any was. *Rejected:* one spell then close.
- **D23 A new key, `b`, to read.** `b` is free. The inventory screen chooses the book, in
  a mode of its own (`ACT_LEARN`, list `LIST_BOOKS`), and in browse mode `b`, or ENTER on a
  book, does the same. *Rejected:* `u` on a book. `act_use()` applies a use hook and
  destroys the item, so a book would need exceptions in the action, the inventory's
  usable list and its help line.
- **D24 With no spell chosen, the check asks only about the book.** In [action_learn.md](action_learn.md), D2.
- **D25 Two refusals for no room.** In [action_learn.md](action_learn.md), D3.
- **D26 The level-up message reads the table, not the player.** `messages_on_event()` runs
  after the queue drains, perhaps after more levels. It compares the slot table's rows for
  the event's level and the one before, so each level's message is right whatever has
  happened since. It says "more spells", not which, and the book screen says the rest.
- **D27 The slot figures.** The Mage's (and the Dev's), to tune in play:

  | Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
  |---|---|---|---|---|---|---|---|---|---|---|
  | Spell level 1 | 2 | 3 | 3 | 3 | 4 | 4 | 4 | 4 | 5 | 5 |
  | Spell level 2 | | | 1 | 2 | 2 | 2 | 3 | 3 | 3 | 4 |
  | Spell level 3 | | | | | 1 | 2 | 2 | 2 | 3 | 3 |
  | Spell level 4 | | | | | | | 1 | 2 | 2 | 2 |
  | Spell level 5 | | | | | | | | | 1 | 2 |
  | Total | 2 | 3 | 4 | 5 | 7 | 8 | 10 | 11 | 14 | 16 |

  Every level adds a slot, so every level-up says so (R39). A new Mage knows Magic Bolt and
  can learn one more level 1 spell at once. By level 10 a Mage can know all 10 of today's
  spells, with 6 slots left for the spells to come. Today's book has 2 spells of level 1,
  so the third level 1 slot (level 2) has nothing to fill it until more spells come. The
  Fighter's row is all 0.

- **D28 Book contents and where they're found.** Today's spells, by level (R43):

  | Book | Kind | Spells | Depths | Rarity | Colourway |
  |---|---|---|---|---|---|
  | basic spellbook | `IT_BOOK_BASIC` | Magic Bolt, Cure Wounds, Hold, Enfeeble, Blink (added with effects_extensions.md) | 1-5 | 4 | `BANK_BLUE` |
  | adept's spellbook | `IT_BOOK_ADEPT` | Fire Bolt, Frost Bolt, Stoneskin, Haste | 4-8 | 10 | `BANK_PURPLE` |
  | master's spellbook | `IT_BOOK_MASTER` | Power Bolt, Renewal | 8-10 | 10 | `BANK_GOLD` |

  *Changed in step 6 (CSpect check 7):* the first rarities, 2, 2 and 1, against item weights
  of 214 to 420 a depth and 4 to 10 items a level, gave about a 5% chance of a book a level,
  and half a book a game. Levels now place 9 to 15 items (`LEVEL_ITEMS_*`,
  [difficulty.md](difficulty.md) section 6), and with these rarities a player who clears
  each depth finds a book at some depth of its band with a chance of about 50% (basic, which
  the Mage starts with), 80% (adept) and 75% (master).

  The first level 3 slot comes at level 5, the player's level on reaching depth 5 (D2 of
  difficulty.md), so the adept's book starts a depth earlier, at 4. The master's starts at
  8, for the level 5 slot at level 9. A Fighter finds books too, and can only carry them.

### 10.5 Design

#### 10.5.1 The slot table (`spells_pdata.c`, page 135)

```c
/* Slots by class, character level - 1, spell level - 1 (D19, D27) */
const uint8_t spell_slots_table[PC_CLASS_COUNT][PC_LEVEL_MAX][SPELL_LEVEL_MAX];
```

#### 10.5.2 The functions (`spells.h`, page 134)

| Function | What it does |
|---|---|
| `spell_slots(id, level)` | Slots at spell level 1-5: the table's for the player (`PC_IS()`) with mana, else 0 |
| `spell_free(id, level)` | `spell_slots()` less the spells `id` knows of that level, 0 if none |
| `spell_slots_new(cls, pc_level)` | 1 if character level `pc_level` gives class `cls` a slot that `pc_level - 1` didn't (D26) |
| `spell_book(book, list)` | Copies book `book`'s spells into `list` (common memory, `SPELL_BOOK_MAX`); returns the count |
| `spell_in_book(book, sp)` | 1 if `sp` is in book `book` |

`spell_learn_level()` goes. `spell_def_t` loses `learn`, a byte a row.

#### 10.5.3 The books (`spells.h`, `spells_pdata.c`)

```c
#define SPELL_BOOK_BASIC   0
#define SPELL_BOOK_ADEPT   1
#define SPELL_BOOK_MASTER  2
#define SPELL_BOOK_COUNT   3
#define SPELL_BOOK_MAX     8    /* spells in a book, at most; SP_NONE for unused */

const uint8_t spell_books[SPELL_BOOK_COUNT][SPELL_BOOK_MAX];   /* page 135: 24 bytes */
```

The items, in `gen_kinds.h` after `IT_QUEST`, so no number moves: `IT_BOOK_BASIC` 160,
`IT_BOOK_ADEPT` 161, `IT_BOOK_MASTER` 162, `IT_COUNT` 163. `ITEM_BOOK` 16, `ITEM_COUNT` 17.
Each row has `.subclass = ITEM_BOOK`, `.data.book_spells = SPELL_BOOK_*`, the new `SPRITE_BOOK` in its
colourway, no hook and no slot, and the band of D28.

The sprite is frame 84 of `assets/Sprites.aseprite`, tag `book`: a closed book drawn in
the layout's variable slots, so each bank colours its cover. A first version is drawn with
the Aseprite command line, for the user to redraw if they wish.

Kits (`chargen_bank.c`): `{ IT_BOOK_BASIC, 0 }` added to `kit_mage[]` and `kit_dev[]`.

#### 10.5.4 `act_learn()` (`act_learn.c`, `BANK_56`)

The action is specified in [action_learn.md](action_learn.md): its inputs, the order of its
refusals and their texts, and its callers. It meets R46 to R51. Its additions are
`ACT_LEARN` 20 and the reasons `ACT_R_NOT_BOOK` 45 to `ACT_R_SLOTS_FULL` 50, all refused.
The class's mana and spells components come with the kind: the Mage and Dev have both, the
Fighter neither.

#### 10.5.5 The book screen (`screen_spell_bank.c`, page 104)

`screen_book_run(book)` is the spell list in a second mode, sharing its rows, cursor and
keys:

```text
row  0          basic spellbook                (the book's name, yellow)
row  2      Spell           Level  MP          (cyan)
row  3   a) Magic Bolt          1   1          cyan: known
row  4   b) Cure Wounds         1   1          white: can learn now
row  5   c) Hold                2   2          red: can't learn now
row  6   d) Enfeeble            2   2          red
row 25   cyan: known  red: can't learn yet
row 26   Free slots  L1:1 L2:0 L3:0 L4:0 L5:0
row 29   status: "You learn Cure Wounds." or why not
row 31   Letter/ENTER: learn  SPACE: close
```

- A row's colour is `ACT_LEARN` asked in check mode: success is white, `ACT_R_KNOWN` cyan,
  anything else red.
- A white row chosen: `ACT_LEARN` is done in the screen, as the inventory's `act_here()`
  does: messages to the status line, `game_pause_events()`, then every row and the foot
  redrawn, as one learn can turn others of its level red.
- A cyan or red row chosen: its reason on the status line, as now.
- It returns 1 if any spell was learned, so `item_command()` uses the turn (D22).

The first screen's code stays as it is for casting: the mode picks the list (known spells
or the book's), the action asked (`ACT_CAST` or `ACT_LEARN`), the title and the foot.

#### 10.5.6 The inventory and the command (`screen_inv_bank.c` page 116, `player_bank.c` page 114)

- A mode `[ACT_LEARN] = { "Study", "Letter/ENTER: read it  SPACE: back", "You have no spell
  book.", LIST_BOOKS }`, and `LIST_BOOKS`: stowed items of subclass `ITEM_BOOK`.
- Browse mode: `b` is `ACT_LEARN`; ENTER on a book is `ACT_LEARN` (`obvious_action()`); the
  help line's keys gain " b:read". It isn't `BUNDLED()`: like aim, it closes the inventory
  and returns to the command.
- `player_command()`: `b` is `item_command(ACT_LEARN)`. `item_command()`, for `ACT_LEARN`,
  opens `screen_book_run(a.item)` and uses the turn if it returns 1.
- The help screen's keys page gains `b`. The play screen's help line is full and doesn't.

#### 10.5.7 The level-up (`pc.c` page 110, `messages_bank.c` page 118)

- `level_up()` loses its call to `spell_learn_level()`. A level-up posts 5 events, not 6
  (section 9, step 1), so 3 levels from one gain post 16 with `EV_XP_GAINED`.
- `messages_on_event()`, on `EV_LEVEL_GAINED` for the player: after "Welcome to experience
  level 3.", if `MANA_IS()` and `spell_slots_new(pc_class, p1)`, "You can learn more
  spells: read a spell book."

#### 10.5.8 Memory

| Where | What | About |
|---|---|---|
| Page 135 | The slot table, the book lists; less `learn` | +160 bytes, of 0.8K used |
| Page 134 | The five functions; less `spell_learn_level()` | +0.2K, of 1.5K used |
| `BANK_56` | `act_learn.c` | 0.4K, of 2.7K free |
| Page 104 | The book mode | 0.5K, of about 1.7K free |
| Page 116 | A mode, a list, a browse key | about 100 bytes; 7 bytes of `modes[]` (actions.md 4.9) |
| Pages 114, 118, 110 | The command; a message and 6 reason texts; a call less | small |
| Page 109 | 3 item kinds | about 120 bytes |
| Page 140 | 2 kit entries | 4 bytes |
| Common | `act_do()`'s case, 6 reason bytes | about 15 bytes |
| Sprites | 1 pattern | 85 of 128 used |

#### 10.5.9 Balance (difficulty.md R13)

The slots follow the **XP needed** curve, through the character level: spell level L at
level 2L - 1. The books follow the **target level** curve (D28). `make balance` before
(2026-11-10): 0 failed, 4 warnings (C1 at depths 3, 4 and 9; C5 at depth 1). Three random
item kinds change only the item pools, so C1 to C5 shouldn't move; the after figure goes in
"As built".

#### 10.5.10 Changes to other specs

| Spec | Change |
|---|---|
| actions.md, action_learn.md | The action's spec in action_learn.md; its rows in the tables of actions.md 4.1, 4.2 and 4.6 |
| items.md | `ITEM_BOOK`, the three kinds |
| ui_inventory.md | The `ACT_LEARN` mode, `LIST_BOOKS`, `b` in browse mode |
| character_creation.md | The book in the Mage's and Dev's kits (D4) |
| player_character.md | `level_up()` no longer learns |
| help_screen.md | The keys page: `b` |
| difficulty.md | Section 6: slots are S1, and they follow the table of D27 |
| memory_map.md, `src/game/README.md` | The pages' contents, the spells module's row |
| todo.md | S1, S2, S3 status |

### 10.6 Tests

**`test_spell`:**

- the slot table: never falling with character level (R35); the first slot of spell level
  L at level 2L - 1 (R11); the Fighter's all 0; the Dev's the Mage's (R36);
- each class's starting spells within its level 1 slots (R38);
- `spell_slots()`: 0 for a monster, a Fighter and an entity with no mana; the table's for
  the Mage at each level;
- `spell_free()`: falls as spells are learned, never below 0;
- `spell_slots_new()`: each level, and 0 for the Fighter;
- the books: every entry a spell of level 1 or more, no spell twice in a book, every spell
  of level 1 or more in some book (R42), and each book's levels as R43 says;
- `spell_book()` and `spell_in_book()`;
- the definitions without `learn`; the tests of `spell_learn_level()` go.

**`test_act`:** each refusal of `ACT_LEARN` in order, with nothing changed and no events: a
book on the floor, a potion, a Fighter, a monster, a spell not in the book, one known, one
beyond the level, slots full; `SP_NONE` passing in check mode; a learn with its event; the
last free slot filled, then the next refused; check mode changing nothing, including
`rand()`; `act_do()` and the reasons' kinds.

**`test_pc`:** a level-up learns nothing and posts 5 events; 3 levels within half the queue.

**`test_gen`:** the book kinds: `ITEM_BOOK`, a `data.book_spells` below `SPELL_BOOK_COUNT`, no hook, no
slot; subclass shares by depth still sum.

**`test_chargen`:** the Mage and Dev carry a basic spellbook, not equipped; the Fighter
doesn't.

**`test_ui`:**

- `b` with no book: "You have no spell book."; with one: the inventory in "Study" mode;
- a Fighter choosing a book: the reason on the status line, the screen still open;
- the book screen's rows and colours, the foot's free slots;
- learning: the message on the status line, the row turning cyan, others of its level
  turning red when the slots fill, the screen staying open;
- closing after a learn uses the turn; closing after none doesn't;
- a red row and a cyan row: their reasons;
- browse mode: `b` and ENTER on a book open it; " b:read" on the help line;
- the level-up message: with a new slot, without one, and not for a Fighter;
- the reasons' texts fit one line.

**CSpect, by the user:**

1. A new Mage: the inventory holds a basic spellbook. `m` lists only Magic Bolt.
2. `b`: the Study screen lists the book; choosing it opens the book: Magic Bolt cyan, Cure
   Wounds white, Hold and Enfeeble red; the foot reads "L1:1".
3. Choose Hold: "That spell is beyond you for now." Choose Cure Wounds: "You learn Cure
   Wounds.", the row turns cyan, the foot "L1:0". SPACE: one turn passes (a monster in
   view moves once). `m` lists both spells.
4. `E` twice: "Welcome to experience level 3." then "You can learn more spells: read a spell
   book." No "You learn". The book now has Hold and Enfeeble white; learn Hold, then
   Enfeeble is red: "No room for more spells of that level."
5. Open the book and close it without learning: no turn passes.
6. A Fighter: `b` on the basic spellbook a debug build places by the start (step 8):
   "The spells mean nothing to you." `E`: no level-up message about spells.
7. Books lie in the dungeon at their depths, show in their colours, and `u` refuses them.
8. Common memory free, before and after, from the map.

### 10.7 Known risks

- **Slots with nothing to fill them.** Until more spells come, some slots (D27) have no
  spell, and the level-up message announces them anyway. This is accepted until the next
  step adds spells.
- **Slots that shrink.** If the table is retuned so a level has fewer slots than a player
  knows, `spell_free()` stays 0 and nothing is forgotten. R35's test prevents it in the
  shipped table.
- **The inventory.** A book is one of 20 places in the pack, and is kept (R50). The Dev's kit
  goes to 18 of 20.
- **Page 104.** About 1.7K free before this; the book mode is the largest addition. If it
  doesn't fit, the book screen goes to a free pair (142/143), at the cost of a second copy
  of the row drawing.
- **The status line during a learn.** `act_here()` copies messages to the status line; the
  book screen needs the same, and `game_pause_events()` must run so `EV_SPELL_LEARNED`'s
  message appears before the rows are redrawn.
- **zsdcc.** The table reads three indices; each comes from a byte, and the row's address
  must be worked out without a multiply that links a library routine (R30): the index is
  `pc_class * 50 + (level - 1) * 5`, so it is written as a pointer walked by rows, or the
  map is checked for `__mulint`.

### 10.8 Questions

Answered 2026-11-10:

- **Q1 (D18)** Books replace level-up learning? **Books only.**
- **Q2 (R50)** Is a book used up? **No, it is kept.**
- **Q3 (R48, D22)** How many spells a reading? **Several, for one turn.**
- **Q4 (R36)** The Dev's slots? **The Mage's.**

Open, for review:

- **Q5 (D27)** The slot figures, and whether Intelligence should add slots (it doesn't, in
  this design).
- **Q6 (D28)** The book names, depths and rarities.
- **Q7 (D23)** `b` for read.

### 10.9 As built

Differences from sections 10.1 to 10.8, by step:

- **Step 6.** The books' rarities are 4, 10 and 10, not 2, 2 and 1, and a level places 9 to
  15 random items, not 4 to 10: the first figures gave about one book in 20 levels (D28).
- **Step 7.** The level-up message is "You can learn more spells from a book.": the spec's
  "You can learn more spells: read a spell book." has 45 characters, and a message line
  holds 40. Page 134 grew by 832 bytes, not about 0.2K: the five functions and their
  asserts; it has 5.8K free. The slot table is read by its three indices directly: the map
  shows no new library routine (10.7).
- **Step 8.**
  - Common memory fell by 32 bytes, not about 15: `act_do()`'s case with its banked call is
    26, and the reason table 6.
  - Page 104 grew by 1,072 bytes (0.5K estimated): it has 642 bytes free. The book screen
    stayed there; it shares the spell list's rows, keys and loop (`run()`).
  - The book screen's key line draws "cyan: known" in cyan and "red: can't learn yet" in
    red.
  - The book opens after the Study (or browse) screen has closed, and closing flushes the
    keys, as every screen does. Keys typed before the book appears are dropped, and the PC
    tests press a key for that closing frame, as they do for aim.
  - The help screen: `b` on the keys page, and `b` in the choosing heading; the inventory's
    lines had no room (help_screen.md, section 10).
  - The debug build's basic spellbook lies at the player's feet in a new game
    (`level_dev_bandit()`), for check 6.
- **Step 9.** `make balance` after: 0 failed, 4 warnings, the same as before (10.5.9): the
  books change only the item pools, and the extra items a level don't count in C1 to C5.
