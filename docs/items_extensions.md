# Items extensions: requirements and design

Status: I1 implemented, with R12-R14 added after the first checks; CSpect
checks 1-9 passed (2026-11-12).

Adds item kinds that use the effects nothing generated
([effects_extensions.md](effects_extensions.md) D11 left the E3 rows without
items), from [todo.md](todo.md) I1. Data only: new rows in
`gen_item_pdata.c`, one new sprite, and a slimmer kind record to make room.
The record and generation are [items.md](items.md).

## 1. Requirements

- **R1** Every effect is made by some item kind, except those meant for
  monsters or innate: `EF_REGEN`, `EF_ROOT`, `EF_HOLD`, `EF_MANA_REGEN`, the
  drains (`EF_DRAIN_*`, for monster hits), and `EF_BLINK`, which needs an
  item aimed at a tile (todo I3).
- **R2** Timed ability buffs as potions: Dex, Con, Int, Wis (I1.1).
- **R3** Poison, acid and magic ward potions, beside fire and cold (I1.2).
- **R4** Heroism, aid and eye drops (I1.3).
- **R5** Bad potions: confusion, dread, paralysis, generated now, without
  waiting for identification (S10) (I1.4, answered 2026-11-12).
- **R6** Parchments read on oneself: detect traps, shield, blessing, deep
  descent, recall (I1.5).
- **R7** Aimed parchments: terror, confusion, hold, hex, bane; wands of acid
  and weakness (I1.6). Mind and body effects are direct, bolts fly
  ([action_aim.md](action_aim.md)).
- **R8** Weapons with a hit hook: bleeding, corrosion, burning (I1.7). A +1
  weapon with a hook needs nothing new: its bonus is in its attack.
- **R9** Ammo with a hit hook: barbed and fire arrows, poison and sleep
  darts, a flask of acid (I1.8).
- **R10** Worn: a Cha ring; fire, cold and poison immunity amulets (I1.9).
  Food with effects: elven waybread, three mushrooms (I1.10).
- **R11** The flask of oil sets what it hits burning (`EF_BURNING`) instead
  of one fire bolt, at the same damage a turn: 4, four times (I1.11,
  answered 2026-11-12).
- **R12 (New, from check 3 and 4)** A timed effect that isn't innate, starting
  on a monster the player sees, is said with its own text after the
  monster's name: "The goblin is terrified!", "The goblin catches fire!".
  Before, an effect on a monster said nothing, so a terror parchment or a
  fire arrow seemed to do nothing.
- **R13 (New)** A seen monster passing its saving throw is said: "The goblin
  shakes it off." An effect that runs out on a monster is not, to keep the
  log short.
- **R14 (New, from check 7)** An effect with a saving throw tries it when
  applied, for monsters and the player alike: passed, it doesn't start (or
  isn't refreshed), and the player is told "You resist." or "The goblin
  resists." (a monster in sight). The throw is still tried at its firings.
  Before, an effect always took hold, so "no effect" couldn't happen
  (answered 2026-11-12).

## 2. Decisions

- **D1 The item record keeps only the sprite and the bank** (answered
  2026-11-12). `render` is a `gen_item_render_t` of 2 bytes, pattern and
  palette flags; `gen_item_bank.c` makes the rest of `gen_render_t` (one
  still pattern, `LAYER_OBJECTS`), the same for every item. It saved 498
  bytes of page 109, without which the 42 kinds don't fit. *Rejected:*
  fewer kinds (about 25), which leaves less room for I2.
- **D2 New IDs at the end,** 166 to 207, so no number moves, as
  effects_extensions.md did. Rows sit in their subclass's section of the
  table.
- **D3 Powers follow what exists.** Ability buffs at 3, as the potion of
  might; wards and statuses at 1; heroism, aid, shield, blessing and bane at
  2; recall at 10, to reach depth 1 from anywhere; detection at 10, a
  10-tile radius. Rarities 1 to 3, as the special kinds before them.
- **D4 A mushroom sprite of its own** (answered 2026-11-12): one pattern,
  frame 85, its cap in the primary ramp so each bank gives a mushroom.
- **D5 Names follow the effects** shown while they last: "potion of
  insight" is Int and "potion of intuition" Wis, so the ring of Wis is
  renamed "ring of intuition" (was "ring of insight") (answered
  2026-11-12). Names over 19 characters are shortened: "antivenom", "potion
  of warding", "fireproof amulet".

- **D6 A third text per effect, `seen_text`** (answered 2026-11-12): in
  `effect_def_t`, read as `EFFECT_TEXT_SEEN`, 40 of them, 872 bytes of page
  121. *Rejected:* building it from the name ("The goblin is affected:
  Corrosion."), which reads badly; an end text per effect too, ~45 more.

- **D7 The throw when applied is in `effect_apply()`,** so wands,
  parchments, spells, hits, traps and drinking all get it. A new event,
  `EV_RESISTED`, carries it to the messages. `effect_apply_unsaved()` is for
  what can't be resisted (the Z development aid). The PC tests set
  `effects_apply_no_save`, so tests of what an effect does needn't roll for it
  to land. *Rejected:* the throw only for aimed and cast effects.

## 3. The kinds

B is `LEVEL_DEPTH_BOTTOM`. "Direct" is `ITEMF_DIRECT`.

| Kind | Hook, effect, power | Depth, rarity |
|---|---|---|
| serrated sword (sword's attack) | hit `EF_BLEEDING` 2 | 4-B, 1 |
| barbed spear (spear's attack) | hit `EF_BLEEDING` 1 | 3-B, 1 |
| acid blade (sword's attack) | hit `EF_CORROSION` 2 | 5-B, 1 |
| fire brand (mace's attack) | hit `EF_BURNING` 2 | 6-B, 1 |
| barbed arrows (10) | hit `EF_BLEEDING` 1 | 3-B, 2 |
| fire arrows (10) | hit `EF_BURNING` 1 | 4-B, 2 |
| poison darts (8, darts' attack) | hit `EF_POISON` 1 | 2-B, 2 |
| sleep darts (6, darts' attack) | hit `EF_SLEEP` 1 | 3-B, 1 |
| flask of acid (1, oil's attack) | hit `EF_CORROSION` 3 | 3-B, 2 |
| elven waybread | use `EF_HEROISM` 1 | 4-B, 2 |
| pale / violet / green mushroom | use `EF_HEALING` 2 / `EF_CONFUSE` 1 / `EF_POISON` 1 | 1-6, 2 |
| potion of grace / endurance / insight / intuition | use `EF_GRACE` / `EF_ENDURANCE` / `EF_INSIGHT` / `EF_INTUITION` 3 | 2-B, 2 |
| antivenom | use `EF_RESIST_POISON` 1 | 2-B, 2 |
| potion of acid ward | use `EF_RESIST_ACID` 1 | 4-B, 2 |
| potion of warding | use `EF_RESIST_MAGIC` 1 | 5-B, 1 |
| potion of heroism | use `EF_HEROISM` 2 | 3-B, 2 |
| potion of aid | use `EF_AID` 2 | 2-B, 2 |
| eye drops | use `EF_CURE_BLIND` 1 | 2-B, 2 |
| potion of confusion | use `EF_CONFUSE` 1 | 1-8, 2 |
| potion of dread | use `EF_FEAR` 1 | 2-8, 2 |
| potion of paralysis | use `EF_PARALYSE` 1 | 3-B, 1 |
| detection parchment | use `EF_DETECT_TRAPS` 10 | 1-B, 3 |
| shield parchment | use `EF_SHIELD` 2 | 1-B, 3 |
| blessing parchment | use `EF_BLESS` 2 | 1-B, 3 |
| descent parchment | use `EF_DESCENT` 1 | 2-B, 2 |
| recall parchment | use `EF_RECALL` 10 | 3-B, 2 |
| terror parchment | aim `EF_FEAR` 1, direct | 2-B, 3 |
| confusion parchment | aim `EF_CONFUSE` 1, direct | 2-B, 3 |
| hold parchment | aim `EF_PARALYSE` 1, direct | 4-B, 2 |
| hex parchment | aim `EF_HEX` 1, direct | 3-B, 2 |
| bane parchment | aim `EF_BANE` 2, direct | 2-B, 2 |
| wand of acid (5 charges) | aim `EF_BOLT_ACID` 6 | 4-B, 2 |
| wand of weakness (5 charges) | aim `EF_WEAKNESS` 2, direct | 3-B, 2 |
| ring of presence | worn `EF_WORN_CHA` 1 | 3-B, 1 |
| fireproof / frostproof / venomproof amulet | worn `EF_WORN_IMMUNE_FIRE` / `_COLD` / `_POISON` 1 | 7-B, 1 |

Changed: the flask of oil (R11) and the ring of intuition (D5).

## 4. Memory

Page 109 (the item table): 6,508 bytes before; 6,010 after D1; 7,603
with the 42 kinds, so 589 bytes are free (I2's AC field needs about 210).

## 5. Rarity

The new weights add 8% to a band's total at depth 1, 17% at depths 3 to 5,
and 27% at depth 10, so every older kind is that much rarer. Healing
potions go from 6.3% to 5.4% of items at depth 5. Not retuned: to be
judged in play.

## 6. Tests

`test_effects`: the throw when applied: passed, resisted with `EV_RESISTED`
and nothing started or refreshed; failed, started; none for an effect with no
throw or through `effect_apply_unsaved()`. `test_effects`: a seen text for exactly the timed effects that aren't
innate, each at most 24 characters. `test_ui`: an effect starting on a seen
monster is said, a save is said, running out isn't; aiming a wand of sleep
at a rat says both lines; a resist is said for a monster and the player. `test_gen`: every kind passes the catalogue rules for its subclass; every
effect is made by some item but for R1's list; the oil burns at 4;
`ITEMF_DIRECT` is on exactly the kinds of action_aim.md's table; an item's
sprite is its pattern and bank, still, on `LAYER_OBJECTS`.

## 7. CSpect checks

Dev aid: G puts the next 6 of these kinds at the player's feet, in ID order
(section 3's order), cycling through all 42 in 7 presses.

1. A potion of grace (third press) raises Dex for its turns; the stats
   panel shows it.
2. A detection parchment (fifth press) finds a hidden trap put down by T.
3. A terror parchment (sixth press) aimed at a monster makes it flee.
4. A flask of oil thrown at a monster (the Dev kit has one) burns it for
   several turns; fire arrows (first press) burn too.
5. The three mushrooms (second and third presses) look right in their banks.
6. A recall parchment (sixth press) read at depth 3 or deeper brings the
   player to depth 1 after 15 turns.
7. A terror parchment aimed at a monster in sight says "The goblin is
   terrified!" or "The goblin resists."; while it lasts, a save says "The
   goblin shakes it off." The same for a wand of sleep ("falls asleep").
8. A fire arrow that hits says "The goblin catches fire!" after the
   piercing damage, then fire damage on its next turns.
9. A potion of confusion drunk a few times says "You resist." now and then,
   instead of "You feel confused."

Results: 1-6 passed (2026-11-12); 3 and 4 found no message (R12, R13). 8
passed; 7 showed "You aim the ..." only, and passed with R14 (2026-11-12),
as did 9. A temporary CSpect trace of effect events, added to find why 7
failed, was removed once it passed.

## 8. As built

As designed, with R12 and R13 added after checks 3 and 4. The G dev aid (`player_bank.c`) changed from the stairway
parchment and levitation potion, already checked, to cycling these kinds.
