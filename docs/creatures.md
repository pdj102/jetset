# Creatures: the roster and rooted creatures

Status: implemented (2026-10-31): 21 kinds in
[gen_creature_pdata.c](../src/game/gen/gen_creature_pdata.c), the new kinds
drawn with their own rest sprites (section "Stand-ins"), and rooted creatures.
D1 to D6 and Q1, Q2 agreed the same day. The roster in section 1 is kept
current: 54 random kinds since [creature_extensions.md](creature_extensions.md)
(2026-11-14). The CSpect checks in section 6 passed (2026-10-31), check 3
after a fix (section 8), and the PC screen shows "Rooted" in red. Not yet tried
on hardware. Section 8 lists where the build differs from the design.

## 1. The roster

54 kinds appear at random, with the player's three classes besides. Depths
run from 1 to 10 (`LEVEL_DEPTH_BOTTOM`), and no depth holds fewer than 10 or
more than 22 kinds. The first 18 joined the rat and the goblin in this
design (2026-10-31); the other 34, and what makes many of them work, came
with [creature_extensions.md](creature_extensions.md) (2026-11-14), which
also narrowed the older kinds' depths so each depth keeps its character.

A shape is one drawing, 2 rest patterns. No shape has attack or dead frames
yet. 20 drawn monster shapes carry the 54 kinds (the goblin's 7, the
ogre's 5), with the player's for the bandit, the coins for the mimic and
the VFX's orb for the will-o'-wisp. Kinds also come in bands: a leader and
its members in one room (creature_extensions.md M3, `gen_bands[]`).

| Depth | Rarity | Kind | Shape | Bank | Hit dice | Works today | Later |
|---|---|---|---|---|---|---|---|
| 1-2 | 10 | rat | rat | earth | <1 | Poison bite. Beast |  |
| 1-3 | 8 | bat | bat | purple | <1 | Fast, flies, erratic flight (`EF_ERRATIC`) |  |
| 1-3 | 5 | fire beetle | bug | orange | 1 | Hard shell, immune to fire. Beast |  |
| 1-3 | 6 | goblin | goblin | green | 1-1 | Weak to fire, wields, opens doors. Leads a war band (2-3) |  |
| 1-3 | 7 | green snake | snake | green | 1 | Poison bite |  |
| 1-3 | 6 | jackal | dog | gold | <1 | Fast. Beast. Packs (band) |  |
| 1-3 | 8 | kobold | kobold | red | <1 | Packs (band) |  |
| 1-3 | 5 | kobold slinger | kobold | earth | <1 | Kit: sling, stones. Skirmisher |  |
| 1-3 | 5 | lichen | lichen | bone | 1 | Rooted, a touch that holds, slow |  |
| 1-3 | 5 | stirge | bat | red | 1 | Flies. Bite bleeds |  |
| 2-4 | 5 | bandit | player | earth | 2 | Wears all it finds | Steals and runs (M5.4) |
| 2-4 | 5 | goblin archer | goblin | orange | 1-1 | Kit: short bow, arrows. Skirmisher. War band |  |
| 2-4 | 6 | green slime | slime | lime | 2 | Acid touch, resists blunt and pierce, slow | Splits (M5.3) |
| 2-4 | 6 | zombie | zombie | green | 2 | Slow. Undead, mindless |  |
| 2-5 | 3 | gas spore | eye | iron | 1 | Floats, slow, no attack. Death: a fire burst (power 8) |  |
| 2-5 | 4 | shrieker | shrieker | purple | 1 | Rooted, no attack. `SP_SHRIEK` wakes the monsters around |  |
| 3-5 | 4 | bombardier beetle | bug | gold | 2 | Spray: range 3, acid and corrosion. Immune to acid. Beast |  |
| 3-5 | 5 | ghoul | zombie | iron | 2 | Claws paralyse. Undead. Leads a ghoul pack |  |
| 3-5 | 6 | giant spider | spider | dark | 3 | Strong poison | Webs (M5.6) |
| 3-5 | 4 | goblin shaman | goblin | purple | 2 | Magic Bolt, Cure Wounds, Enfeeble, Mend (hurt allies). Skirmisher |  |
| 3-5 | 8 | orc | goblin | iron | 2 | Wields and wears, opens doors |  |
| 3-5 | 7 | skeleton | skeleton | bone | 2 | Weak to blunt, resists pierce. Undead, mindless (never flees). Leads a bone patrol |  |
| 3-5 | 6 | wolf | dog | iron | 2 | Beast. Packs (band) |  |
| 4-6 | 3 | floating eye | eye | blue | 3+ | Rooted, no attack. Struck: paralyses who hits it |  |
| 4-6 | 5 | gold snake | snake | gold | 3 | Spit: range 3, poison, blinds |  |
| 4-6 | 4 | harpy | bat | flesh | 3 | Flies. Song: `SP_CONFUSE` |  |
| 4-6 | 3 | hobgoblin | goblin | red | 2 | Kit: short sword, shield. Drinks potions. Leads a troop |  |
| 4-6 | 5 | orc archer | goblin | earth | 2 | Kit: crossbow, bolts. Skirmisher. War party |  |
| 4-6 | 4 | shadow | ghost | shadow | 3 | Touch drains Str, resists weapons. Undead, mindless |  |
| 4-6 | 4 | skeleton archer | skeleton | iron | 2 | Kit: short bow, arrows. Skirmisher. Undead, mindless |  |
| 4-7 | 3 | mimic | coins | gold | 4 | Rooted, looks like treasure | Hidden until touched (M5.5) |
| 5-7 | 5 | ghost | ghost | bone | 5 | Resists weapons, floats. Undead | Passes through walls (M5.2) |
| 5-7 | 8 | ogre | ogre | earth | 5 | Hits hard, wields. Giant |  |
| 5-7 | 5 | owlbear | ogre | flesh | 5 | Hug holds. Beast |  |
| 5-8 | 3 | will-o'-wisp | orb | blue | 5 | Fast, flies, erratic, hard to hit. Magic Bolt, Blink |  |
| 6-8 | 3 | basilisk | lizard | green | 6 | Slow. Gaze: range 4, paralyses. Beast |  |
| 6-8 | 2 | orc warlord | goblin | dark | 5 | Kit: battle axe, chain mail. War cry (`SP_WAR_CRY`). Leads a war party |  |
| 6-8 | 5 | warg | dog | dark | 4 | Quick. Beast. Packs (band) |  |
| 6-8 | 4 | wight | zombie | dark | 4 | Touch drains Con, resists weapons. Undead |  |
| 6-8 | 5 | wraith | ghost | dark | 6 | Touch weakens, resists weapons. Undead | Passes through walls (M5.2) |
| 6-9 | 4 | wyvern | dragon | green | 6 | Flies, sting poisons. Dragon |  |
| 7-10 | 4 | hell hound | dog | orange | 6 | Fire breath, immune to fire, weak to cold. Beast. Packs |  |
| 7-10 | 5 | hill giant | ogre | iron | 8 | Boulder: range 5. Wields. Giant |  |
| 7-10 | 4 | mummy | zombie | bone | 7 | Slow, touch drains Con, resists blunt and pierce, weak to fire. Undead, mindless |  |
| 7-10 | 3 | necromancer | robe | dark | 6 | Kit: quarterstaff, robe. Frost Bolt, Hold, Summon Undead, Blink. Skirmisher, drinks potions. Holds court |  |
| 7-10 | 6 | troll | ogre | green | 7 | Regenerates; weak to fire and acid. Giant |  |
| 8-10 | 3 | banshee | ghost | ice | 7 | Floats, resists weapons. Wail (`SP_WAIL`): a fear burst. Undead | Passes through walls (M5.2) |
| 8-10 | 3 | black pudding | slime | dark | 10 | Slow, mindless. Immune to acid, cold and poison. Bite and struck corrode | Splits (M5.3) |
| 8-10 | 4 | minotaur | ogre | red | 9 | Quick, hits hard. Giant |  |
| 8-10 | 4 | salamander | lizard | orange | 7 | Bite and struck burn. Immune to fire, weak to cold |  |
| 9-10 | 2 | dragon | dragon | red | 10 | Fire breath, immune to fire, weak to cold, flies. Dragon |  |
| 9-10 | 3 | frost drake | dragon | ice | 9 | Frost breath, immune to cold, weak to fire, flies. Dragon |  |
| 9-10 | 2 | vampire | robe | red | 9 | Regenerates, resists weapons, weak to fire. Hold, Blink. Undead | Life drain (M5.1) |
| 10 | 1 | lich | robe | bone | 11 | Touch paralyses. Power Bolt, Frost Bolt, Hold, Summon Undead. Skirmisher. Undead. Its crypt |  |

Numbers follow the rat and goblin: hit points about 4.5 a hit die, to-hit
about the hit dice, XP from the classic B/X table with its bonus for a
special ability. They have not been balanced by play.

### Stand-ins

The table names a shape, not a tag: `SHAPE_LICHEN(BANK_BONE)`. Every shape now
has its own rest art (`<shape>.idle`, 2 frames, drawn 2026-10-31 by the
workflow in [sprite_style.md](sprite_style.md) section 7), and lichen and
shrieker, one shape when planned, became two. No shape has attack or dead
frames yet. The orc and shaman are recolours of the redrawn goblin, the mimic
borrows the coins. The bandit in the player's
sprite has not been looked at: bank 0's layout is only a guide, so its colours
in earth may need work.

### Side effect: events while generating

The troll is the first monster with an innate effect, so generating a level
can now post `EV_EFFECT_STARTED`. The messages speak only for the player, so
nothing shows. `test_level` was changed from "no events" to "only those".

## 2. Goal of rooted creatures

The lichen, shrieker, floating eye and mimic stay where they were generated and
still attack what comes next to them. Being rooted is a condition, so anything can
have it: for ever for a creature that can't move, or for a while for one held
by a touch or, later, a spell.

## 3. Requirements

- **R1 (Changed)** A rooted creature, the player included, never changes tile
  by its own action. Trying to move or take the stairs is a struggle: it fails
  and uses the turn, as a missed attack does. Reason: CSpect check 3. No time
  passes during the player's turn (game_loop.md R7), so a refused step left a
  player held by a dead lichen with a hold that never ran out.
- **R2** It keeps its other behaviour: it wakes, notices the player, and
  attacks what is in reach, if it has an attack.
- **R3 (Changed)** Being rooted is a status, `STATUS_ROOTED`, granted by an
  effect. Reason: the review of D2, so one rule covers monsters that never
  move and a temporary hold.
- **R4** A rooted creature with nothing to do waits, as every AI turn ends
  in one action (ai_default_brain.md R9).
- **R5** The lichen, shrieker, floating eye and mimic are rooted by an innate
  effect, which nothing cures.
- **R5a (New)** Effects that last for ever are of two kinds. A **permanent**
  effect is one a creature has gained: a cure ends it. An **innate** effect is
  part of what the creature is, as a plant's roots or a troll's regeneration:
  nothing ends it. Reason: the review of D5.
- **R6 (New)** A temporary hold, `EF_HOLD`, ends with time, a saving throw or
  a cure. Reason: the temporary case from the review of D2.
- **R7 (New)** Bumping still attacks and opens: a rooted player next to a
  monster can fight it. Reason: otherwise a player held by a lichen couldn't
  hit it.
- **R8 (New)** The player is told: "You can't move!" on a failed step, and
  "Rooted" on the PC screen with the other statuses.

## 4. Decisions

- **D1 A capability, `AI_CAN_MOVE`, worked out by `ai_can()` (agreed).** Like
  the other capabilities, it comes from what the creature is now (ai.md D3):
  set unless `STATUS_ROOTED` is, so it changes when a hold starts and ends.
  `ai_step()` is the one place the AI moves (`opt_wander()` and
  `ai_step_toward()` both reach it), and it returns 0 without the bit, so
  every state follows. The bit is the free `0x80` in `ai_can_t`, written as a
  constant, so the zsdcc signed-shift trap doesn't apply. **Rejected:** a
  second brain, `AI_BRAIN_ROOTED`: a brain's table chooses states, and these
  want the same states with fewer options, so the difference would go into
  the states as `if (brain == ...)`, which ai.md section 2 rules out.
  **Rejected:** a very high speed: a rooted creature would also attack
  rarely.
- **D2 (revised) A condition: `STATUS_ROOTED` (0x08), in
  `STATUS_CONDITIONS`.** It uses what statuses already are: an effect grants
  it, the OR of a creature's effects keeps it, and `ai_can()` reads
  `stats_status[]` in slot 0 with no banked call. The kinds name an innate
  effect, `EF_ROOT`, in their free `GEN_HOOK_INNATE` hook, so the kind record
  and `ai_kind_t` don't change and no table is added. `EF_ROOT` is timed, for
  ever (`cycles` 0), innate (D5), with a name (the definitions' test and the
  PC screen expect one) but no start or end text, no tick and the longest
  period (255 ticks). The cost is one effect entity and timer per rooted creature, as
  the troll's regeneration: at most 5 a level, and 1 `EV_EFFECT_STARTED`
  each when generated, within the 16-event queue. **Rejected:** a `rooted`
  byte in `ai_kind_t` and a new `ai_rooted[]` table (the first proposal): 100
  bytes for monsters only, and a hold would need a second way in.
  **Rejected:** a base status per kind (`stats_status_base[]`), which needs no
  entity: 100 bytes of slot 0, which has 1.7K left, and a second source of
  statuses against attributes.md R13.
- **D3 A step refused for being rooted is not "lost" (agreed).**
  `opt_approach()` today raises `AI_STIM_LOST` when it can't step, which would
  send a rooted hunter back to idle each turn and lose the player next to it.
  It returns without the stimulus when `AI_CAN_MOVE` isn't set, so it stays in
  hunt.
- **D4 The rule lives in the actions.** With the player able to be rooted,
  this is a rule every actor shares, so it goes in `act_move()` and the
  stairs actions as `ACT_R_ROOTED`, and `ai_can()` mirrors it (ai.md R5b).
  It is an attempted reason, not a refused one (R1), so it comes in the
  resolve step, after the check mode: `ACT_CHECK` passes, as for an attack
  that may miss. A blocker still gives `ACT_R_BLOCKED_ENTITY` first, so
  bump-to-attack works (R7). The AI doesn't waste turns struggling: `ai_step()`
  tests `AI_CAN_MOVE` first. An actor without stats is never rooted. **Rejected:** the rule in `ai_step()` alone:
  a held player would walk away.
- **D5 (revised) Innate is a mark on the definition, `EFFECT_INNATE`.**
  `effect_cure()` ends every effect granting the status, so a panacea would
  otherwise set a lichen walking; it now skips an effect marked
  `EFFECT_INNATE`, beside `EFFECT_STACK` in a definition's flags. An effect
  with `cycles` 0 and no mark is permanent, and a cure ends it. `EF_ROOT` and
  `EF_REGEN` are marked. A test holds that an innate effect is timed, lasts
  for ever and has no saving throw, and that `GEN_HOOK_INNATE` names only
  innate effects. So rooted that can be cured, for a player, is a row of its
  own with the same status and no mark, added with what causes it (a curse);
  no row is added now. **Rejected:** `cycles` 0 alone deciding, the first
  proposal: it can't tell a plant's roots from a curse. **Rejected:** marking
  each active effect by how it was applied, so one row serves both: a new
  byte per entity in page 121 and another argument to `effect_apply()`, to
  save a row. **Rejected:** keeping `STATUS_ROOTED` out of
  `STATUS_CONDITIONS`: a hold is an impairment, in red, and cures should end
  it.
- **D6 `EF_HOLD`, a temporary hold.** Timed, granting `STATUS_ROOTED`:
  "You are stuck fast!", "You pull free.", 6 periods of `TURNS(1)`, a save on
  Str against 14. The period is shorter than the lichen's 20 ticks on purpose:
  a hit refreshes a hold and restarts its timer (effects.md R13), so at
  `TURNS(2)` a lichen's next hit could cancel the firing, and with it the save,
  and the player would stay held. Both new effects go in the timed block of `effects.h`. The
  lichen's hit hook changes from `EF_SLOW` to `EF_HOLD` (Q2), so the game has
  a hold before it has spells.

## 5. Scope

- **In:** R1 to R8: `STATUS_ROOTED`, `EF_ROOT`, `EF_HOLD`, `ACT_R_ROOTED` and
  its message, `AI_CAN_MOVE`, `EFFECT_INNATE` and `effect_cure()`, the PC
  screen's status name, the four kinds' hooks and the lichen's touch;
  the documents in section 5.1; the tests in section 6.
- **Later:** every other "Later" in the table, each with its own spec. A
  mimic that hides is one: this step only keeps it still. A spell that holds
  needs aimed effects (effects.md section 6). A permanent rooted effect comes
  with its cause (D5). The main screen's status line
  shows only "Poisoned", and is left so.

### 5.1 Documents changed with the build

**effects.md:**

- Section 1: an effect is still one of three durations; timed effects that
  last for ever are permanent or innate.
- Section 2: new terms, Permanent and Innate. Period, cycles: "lasts for ever"
  points to them.
- Section 3: a new requirement, as R5a here: a cure ends a permanent effect,
  not an innate one, which ends only with its target.
- 4.1: `flags` is `EFFECT_STACK`, `EFFECT_INNATE`.
- 4.2: `OP_CURE` skips innate effects.
- 4.3: the innate hook names only innate effects.
- 4.4: `effect_cure()` skips them too.
- Section 5: a new decision, D5 here, with what it rejected.
- Section 7: the innate rules in the list of what `test_effects` covers.
- Section 8, Entities: an innate effect refused because no entity is free is
  now visible: `create()` ignores the result, so a lichen made then would walk.
  Accepted: only a monster made at a full 100 entities, and levels are far
  below that. Regeneration lost that way never mattered.

**attributes.md:** R14 and 4.7 (cures don't end innate effects); 4.5, the row
for `STATUS_ROOTED`.

**items.md:** 4.4, the innate hook row names innate effects.

## 6. Tests

PC tests:

- **`test_ai`:** `ai_can()` with and without the status, and the bit back
  when a hold ends; a rooted hunter next to the player attacks; one out of
  reach stays on its tile and stays in hunt; an idle one never moves over many
  turns; it wakes as usual. Existing exact `ai_can()` masks gain
  `AI_CAN_MOVE`, in `test_gen` too.
- **`test_act`:** a rooted actor's move fails with `ACT_R_ROOTED` and uses the
  turn, its check passes; the stairs the same; a blocked move still `ACT_R_BLOCKED_ENTITY`; an actor
  without stats moves.
- **`test_effects`:** `EF_HOLD` ends by time, save and cure; `EF_ROOT` isn't
  ended by `effect_cure()`; a poison on the same creature still is; every
  `EFFECT_INNATE` definition is timed, with `cycles` 0 and no save; a hold
  refreshed by a lichen's hits still gets its save.
- **`test_gen`:** the four kinds have `STATUS_ROOTED` when generated and no
  others do; every `GEN_HOOK_INNATE` names an innate effect.

CSpect checks:

1. Find a lichen (depth 1-5): it stays on its tile while awake, and attacks
   when the player stands next to it.
2. A shrieker and a floating eye stay still and never attack.
3. Held by a lichen, a step says "You can't move!" and uses
   the turn, attacking the lichen works, and "You pull free." ends it, with the
   lichen alive or dead, using only the direction keys.

## 7. Questions

- **Q1 (answered):** a rooted creature with no attack sits awake with no
  behaviour until its shriek or gaze is built: harmless XP until then.
- **Q2 (answered):** the lichen's touch is `EF_HOLD`, not `EF_SLOW`: the
  classic sticky lichen, and the game's first way to be held.

## 8. As built: differences from the design

- **Generating a rooted creature posts two events, not one (D2):**
  `EV_ATTRS_CHANGED` as well as `EV_EFFECT_STARTED`, as its status changes.
  Five rooted creatures on a level post 10 of the queue's 16, and `test_level`
  generates every depth at many seeds without filling it. Its check allows
  both events.
- **D3:** a rooted hunter stays in hunt only while it sees its target. Out of
  sight it is lost and goes idle as usual, in `opt_approach()`, rather than
  hunting a goal it can never reach.
- **`act_rooted()`** in `act_shared.c` is the one test, used by `act_move()`
  and the stairs.
- **A struggle uses the turn (R1, D4).** The first build refused the step at
  no cost, and CSpect check 3 found a hold that never ended once the lichen
  was dead: the player's turn stops the clock, so only the wait key let it run.
  The PC tests drove the timer directly and couldn't see that.
- **Memory:** common memory 1 byte less, 4,733 free (the reason table). Page
  112 up from $1C1A to $1C70, page 120 from $1461 to $1470, page 121 from
  $9A1 to $A04, page 130 from $12A1 to $1316. Page 107 is $52A with all 21
  kinds.
- **The asm was read** for `effect_cure()`, `act_rooted()`, `ai_can()`,
  `ai_step()` and `opt_approach()`: field offsets, the `EFFECT_INNATE` bit and
  the rooted and move bits are right.
- **Tests:** `test_ai` 136 checks, `test_act` 203, `test_effects` 158,
  `test_gen` 78. The PC screen's "Rooted" and the "You can't move!" message
  have no PC test: CSpect check 3 covers the message.
