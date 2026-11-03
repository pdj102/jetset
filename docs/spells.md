# Spells: design specification

How Jetset's spells work: what a spell is, who knows which, how one is cast and paid
for, and how new ones are learned.

Spells are cast with an action ([actions.md](actions.md)), do what an effect says
([effects.md](effects.md)) and cost mana ([mana.md](mana.md)). Character levels are in
[player_character.md](player_character.md), and how a monster chooses is in
[ai.md](ai.md).

Status: proposed, 2026-11-02. `SPELL_DIRECT` added 2026-11-03, agreed in actions.md
(section 6.13, D30). Being built, in the steps of [spells_plan.md](spells_plan.md).
This spec replaces the sketch in actions.md, section 6.6.

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

 level_up() ---------> spell_learn_level() --+
 later: OP_LEARN ----------------------------+--> spell_learn() --> EV_SPELL_LEARNED
```

## 2. Terms

| Term | Meaning |
|---|---|
| Spell | A row of `spell_defs[]`, `SP_*`. `SP_NONE` is an empty entry in a list |
| Spell level | How powerful a spell is, 0 to 5. Not a character level, nor a depth |
| Ability | A spell of level 0: it costs nothing |
| Cost | The MP a cast takes, worked out from the spell level |
| Shape | Who a spell reaches: `SPELL_SELF`, `SPELL_BOLT` or `SPELL_DIRECT` |
| Bolt | A spell sent along a line, stopping at the first creature (actions.md, D11) |
| Direct | A spell sent along a line to the creature on the chosen tile, past any on the way (actions.md, 6.13) |
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
- **R9** On reaching a character level, the player learns every spell whose row names
  that level. A row naming 0 is never learned this way.
- **R10** The player's kind lists every spell of `learn` 1, as level 1 is never gained. A
  test holds this.
- **R11** Spell level L is learned at character level 2L - 1 or later, the pace of classic
  B/X. A test holds this.

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
- **R17** The effect's source is the actor, so a kill earns its XP (actions.md, D17).
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
| `shape` | `SPELL_SELF`, `SPELL_BOLT` or `SPELL_DIRECT` |
| `ef`, `power` | The effect and its power |
| `range` | A bolt's or direct spell's reach, through `nge_map_within()`; 0 for `SPELL_SELF` |
| `learn` | The character level the player learns it at, or 0 for never |
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
| `spell_learn_level(id, level)` | Learns each row with that `learn` |
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

**Cast a spell.** In: actor (alive, placed in an area), spell (`SP_*`), and x, y for a bolt
or a direct spell.
Out: target. `spell` stays as it was, so `ai_try()` can pass the context twice.

1. **Context:** `spell_get()` and `spell_cost()`. Fields go into locals (T-ZSDCC-1).
2. **Validate:**
   - not known: `ACT_R_NO_SPELL`;
   - cost above 0, and no mana component or `!MANA_HAS()`: `ACT_R_NO_MANA`;
   - `SPELL_SELF`: the target is the actor;
   - `SPELL_BOLT` and `SPELL_DIRECT`: the actor's own tile is `ACT_R_NO_TARGET`, then
     `act_line()` with `ACT_LINE_BOLT` or `ACT_LINE_DIRECT`, which may give
     `ACT_R_OUT_OF_RANGE`, `ACT_R_NO_LINE` or `ACT_R_NO_TARGET`.
3. **Resolve:** nothing to roll (R16).
4. **Apply:** `EV_CAST`, `mana_spend()` if the cost is above 0, then `effect_apply()` with
   the actor as source.

| Addition | Value |
|---|---|
| Action | `ACT_CAST` 15, a case in `act_do()` |
| Refused reasons | `ACT_R_NO_MANA` 38, `ACT_R_NO_SPELL` 39, `ACT_R_COUNT` 40 |
| Texts | "You haven't the mana.", "You don't know that spell." |
| `act_t` table, actions.md 4.2 | `target` out; `spell` in; x, y for a bolt or a direct spell |

`ACT_R_NO_TARGET` on the actor's own tile means the spell can be cast, as it does for fire:
the list asks that way, before there is a tile.

### 4.5 Shapes

| Shape | Target | Who reads x, y |
|---|---|---|
| `SPELL_SELF` | The actor | Nobody |
| `SPELL_BOLT` | The first creature on the line, from `act_line()` with `ACT_LINE_BOLT` (actions.md, 6.2) | `act_line()` |
| `SPELL_DIRECT` | The creature on the chosen tile, past any on the way, from `act_line()` with `ACT_LINE_DIRECT` (actions.md, 6.13) | `act_line()` |

A wall or closed door stops either line. A bolt suits what flies, such as fire; direct suits
what doesn't, such as a hold or a teleport of another (actions.md, D28). In section 4.12,
Hold and Enfeeble are direct, as the wands of sleep and slowness are, and the damaging spells
are bolts.

A bolt or a direct spell can't target its caster. Aim allows that for a staff of healing, but a spell that
helps the caster is `SPELL_SELF`, so the player gets no prompt. Helping another creature
isn't possible yet, and no monster has an ally to help.

### 4.6 Learning

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

The powers follow the items: a wand of striking is 4, a wand of fire 6, a staff of power
12, a healing draught 3 and a potion 6.

**Creatures.** The goblin shaman knows Magic Bolt, Cure Wounds and Enfeeble, with its 6 MP.
It is the only caster for now, and its "later: casts" comment goes.

### 4.13 Changes to other specs

Made when this is built:

| Spec | Change |
|---|---|
| actions.md | Section 6.6 points here; tables in 4.1, 4.2, 4.5 and 4.7; R27 no longer "later"; reasons and events out of 6.1 |
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
  `ACT_PRAY`, an action that differs only in its data (actions.md, D20).
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
- **D7 The player learns automatically.** `level_up()` runs in an event handler, and no
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
- **D15 Abilities are level 0, in this table** (actions.md, D20). The rule costs one test, so
  it is here from the start, though no kind uses it yet (section 6).
- **D16 The first list uses only working effects.** A spell that takes MP and does nothing
  reads as a bug. *Rejected:* adding pending rows as items do (items.md, R12).

## 6. Not built yet

| Item | What's there, what's missing |
|---|---|
| Area spells | Needs `SPELL_BALL` and a walk in `act_cast()`: each target's events, about 3 each, against a queue of 32. How it picks targets, and whether it hits the caster, needs deciding |
| Abilities in the creature table | The rule is here. A floating eye's gaze is a row. Level 0 is free, so the AI needs a rate, or it uses one every turn |
| A dragon's breath | A bolt ability works now. Real breath needs an area |
| A shriek | A self-targeted effect with an op that wakes the level |
| Spells on doors, walls and tiles | Knock, stone to mud, webs. Needs an op that reads the caster's surroundings, or a tile passed to `effect_apply()` (D3) |
| Prayers | `school` and a list of their own (D2) |
| Learning from scrolls and shrines | `spell_learn()` is ready. Needs `OP_LEARN`, its rows, and items or features that name them |
| Forgetting | No rule needs it |
| Classes | Every character can cast until player_character.md R16. Then `learn` and maximum MP depend on the class |
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
| Blind | `EF_BLIND` | A rule that reads `STATUS_BLINDED` |
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

