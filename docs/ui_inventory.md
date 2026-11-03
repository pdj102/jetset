# Inventory screen, second version: requirements and design

Status: implemented (2026-11-03); the PC tests pass. The CSpect and
hardware checks in section 7 are not run yet. Agreed on 2026-11-03, with the
questions answered in section 9. Section 11 lists where the build differs
from the design.

Builds on [ui_screens.md](ui_screens.md) (the screen framework and the first
inventory screen) and [items.md](items.md) (kinds, attacks, stacks). It
changes the inventory screen (`game/ui/screen_inv_bank.c`) and the item
command that opens it (`game/player/player_bank.c`). It reverses
ui_screens.md D7 and Q1 (one action closes the screen) and replaces its
R19 (the details panel).

## 1. Goal

- Managing the inventory takes fewer keys and fewer turns: several
  changes in one visit, for one turn.
- Each row says more about its item: how many there are, and what it does
  in a fight.

## 2. Terms

| Term | Meaning |
|---|---|
| Visit | The time from opening the inventory screen to closing it |
| Bundled action | An action done inside the screen, the screen staying open: equip, take off, drop, use |
| Closing action | An action that needs the map or a direction (aim, unlock) and so closes the screen, as today |
| Count | `item_count[]`: a stack's quantity, or a wand's charges, a light's burn time, a pick's uses, a treasure's gold, by subclass (items.md 4.2) |
| Stack | An item whose count is its quantity: ammo and treasure |
| Attack | An item kind's `attack_kind_t`: dice, sides, to-hit and damage bonus (`game/components/attack.h`) |
| Details | The text after an item's name on its row (R6-R9) |

## 3. Requirements

**Changed** or **New** marks a requirement that differs from the request,
with the reason.

### Several actions, one turn

- **R1** In browse mode, equip, take off, drop and use are done inside the
  screen, which stays open and redraws the list. The player can do any
  number of them in one visit.
- **R2** A visit in which at least one action succeeded uses exactly one
  turn, when the screen closes. A visit with none uses no turn.
- **R3 (Q1)** Use (drink, eat, read) is bundled: several items can be
  used in one visit, for its one turn. Aim and unlock are not: they need
  the map or a direction, which the screen covers, so they close the
  screen and are done by the item command, as today.
- **R3a (New)** If the player dies during a visit (a bad potion), the
  screen closes at once and the turn is used, as it would be in play.
- **R4** Each bundled action is still checked first (ui_screens.md R16):
  a refused one shows its reason on the status line and changes nothing.
- **R5 (New)** The words of a bundled action ("You wield the short
  sword.", "You feel better.") show on the status line, and also reach the message window,
  so the log tells what the turn did. *Reason: the player sees the action
  worked; the log stays complete.*

### Rows

- **R6** Each row shows the item's count, its name and its details, in
  fixed columns, so that names and details line up down the list.
- **R7** The count is right-aligned in 3 characters (a count is a byte:
  255 at most). It is the real count for stacks (ammo, treasure) and 1
  for everything else (Q2).
- **R8** An item with an attack (melee weapons, launchers, thrown ammo)
  shows it as dice, then to-hit and damage bonuses, signed:
  `1d6 [+1,+0]`, `1d8 [-1,-1]`. Both bonuses always show, so the columns
  line up.
- **R9 (New, Q4)** Ammo without dice (arrows, bolts, stones) shows only
  its bonuses, which add to the launcher's: `[+1,+1]`. Plain ammo, with
  no attack part, shows `[+0,+0]`. *Reason: the request's example
  `Arrows 1d4 [+2]` doesn't match the data: arrows have no dice of their
  own (items.md 4.6).*
- **R10 (New, Q6)** Other items' details: armour and shields their AC
  (`AC 3`); wands their charges, lights their burn time and picks their
  uses, as their count column shows 1 (Q2); the rest nothing for now
  (descriptions come later).
- **R11** The numbers are the item's own, not the player's totals: a
  bow's row doesn't add the arrows, and no row adds the player's
  abilities.

### Layout

- **R12** The list uses the full width of the screen (columns 0-39).
  There is no details panel.
- **R13** The pack's count and capacity (`Pack 12/20`) move to the title
  row.
- **R14 (Changed, D3)** Items in use are shown in a different ink, in
  place of the slot words ("(in hand)"), which no longer fit.
- **R15** Opening, scrolling and the redraw after an action each take
  at most 6 frames on hardware; a cursor move at most 2 (ui_screens.md
  R10).

### Unchanged

- **R16** Choose mode (`d`, `w`, `r`, `u`, `a`, `p` from the main screen)
  still does one action and closes: a letter finishes the command, as
  today (Q5). It shows the new rows.
- **R17** SPACE leaves; the help line lists only the actions allowed for
  the selected item.

## 4. Decisions

- **D1 The screen does bundled actions itself.** In browse mode the
  screen calls `act_do(..., ACT_DO)` for equip, take off, drop and use,
  redraws, and stays open; on closing it returns *a turn was used* or
  *nothing done*, and the item command ends the player's turn
  (`player_turn = 0`) once. Aim and unlock are returned as now.
  This reverses ui_screens.md R15 ("a screen only chooses") for these
  four actions. Accepted cost (Q1): several potions drunk for one turn,
  with no monster acting between them. **Rejected:** the screen collecting a list of actions
  and the command doing them after it closes: the screen couldn't show
  the result of one before the next (a full pack after a take off, a
  two-handed weapon blocking a shield), so its checks would be wrong.

- **D2 Events are processed after each bundled action, inside the
  screen.** Each action posts events (`EV_EQUIPPED`, `EV_ATTRS_CHANGED`,
  `EV_DROPPED`...) and the queue holds 16 (`NGE_EVENT_QUEUE`); a visit
  of ten changes would overflow it, and a use's effect posts several. The screen calls
  `nge_event_process(game_on_event)` after each action, as `game.c`
  does after each timer firing, through a game.c function
  (`game_pause_events()`, beside `game_pause_frame()`), with the message
  output sent to the status line as well as the window (R5).
  **Rejected:** processing them all on closing: overflow, and no words on
  the status line.

- **D3 In-use items by ink, not slot words.** The row is
  `count name details`: 3 + 1 + 19 + 1 + 14 = 38 columns, plus 2 for the
  choose mode's letter. The longest name is 19 (`ring of strength +1`)
  and the longest details 14 (`1d10 [+10,+10]`), leaving nothing for
  `(on hands)` (11). In-use items come first, as now, in another ink.
  **Rejected:**
  - "In use" and "Pack" headings: rows the cursor must skip, which
    `nge_list` doesn't do, and 2 of the 27 rows;
  - a short slot column (`hand`, `body`): names would be cut to 14
    (`sheaf of arrows +1` is 18).

  Lost: which ring is left and which right. Q7 asks whether that
  matters.

- **D4 The cursor follows its item.** After an action the list is
  collected again (in use first, then the pack). The cursor stays on the
  same item if it is still listed (it moved between in use and the
  pack); a dropped item's place goes to the next one, or the last.

- **D5 The bonuses are formatted by the screen, from the kind's
  attack.** The equipped item's attack component holds the same numbers,
  but stowed ammo has none (items.md 4.6), so the kind record is the one
  source for every item. A small function in page 116 writes the details
  into a 15-byte buffer; it is pure and PC tested. **Rejected:** a
  description string per kind: 160 kinds, and the numbers would be
  written twice.

- **D6 Attributes: recompute after each action, as now (the question
  asked).** See section 10.

- **D7 Choose mode keeps one action per visit (R16).** Its keys are a
  shortcut: `w` and a letter is the quickest way to wield one thing.
  Bundling there would need a second key to leave. **Rejected:** choose
  mode staying open: every shortcut becomes three keys.

## 5. Design

### 5.1 Layout

```
col 0 1 2   5 6                  25 26           39
row 0  |      Worldly Possessions      Pack 12/20 |   title (count right-aligned)
row 1  |                                          |
row 2  |     1 short sword        1d6 [+1,+0]     |   in use: another ink
       |     1 leather armour     AC 2            |
       |    15 sheaf of arrows    [+0,+0]         |
       |     1 short bow          1d6 [+0,+0]     |
       |     1 potion of healing                  |   the pack
       |     1 wand of fire       8 charges       |
row 28 |                                          |
row 29 | You wield the short sword.               |   status (one line)
row 30 |                                          |
row 31 | jk:select r:off d:drop SPACE:done             help line
```

Choose mode puts `a)` in columns 0-1. Panels: title (0-39, 0-1), list
(0-39, 2-28, 27 rows), status (0-39, 29-30). The help line says
`SPACE:done` once an action has been done, `SPACE:back` before.

### 5.2 The details (`describe()`, in `screen_inv_bank.c`)

| Item | Details |
|---|---|
| Attack with dice (weapon, launcher, thrown) | `<dice>d<sides> [<to_hit>,<bonus>]`, signed |
| Ammo, dice 0 | `[<to_hit>,<bonus>]`, `[+0,+0]` if it has no attack part |
| Armour, shield (`EF_WORN_AC` hook) | `AC <power>` |
| Wand | `<count> charges` |
| Light | `<count> turns` |
| Lock pick | `<count> uses` |
| Anything else | nothing |

Read from the kind record (`kind_index[id]`, the item table in page
108/109) with the generator's accessors, as `gen_item_fits()` and
`gen_item_subclass()` are; a new `gen_item_attack()` copies the attack
part to the stack (6 bytes).

### 5.3 The run (`screen_inv_run()`)

Its interface keeps its shape:

```c
#define SCREEN_INV_DONE  (ACT_COUNT + 1)   /* answer: bundled actions done, a turn used */
```

Browse mode, on `w`, `r`, `d`, `u` (or ENTER, the item's obvious
action): check (R4); do it with `act_do(ACT_DO)`; `game_pause_events()`
(D2); if the player is dead, close (R3a); else re-collect, place the
cursor (D4), redraw the list, the title's count and the help. On SPACE it
returns `SCREEN_INV_DONE` if anything succeeded, else `SCREEN_INV_BACK`.
On `a` (or ENTER on an aimable item) it returns `ACT_AIM`, as now; if
bundled actions came first, the aim's turn is the visit's one turn.

`item_command()` sets `player_turn = 0` on `SCREEN_INV_DONE`.

An action that fails in do mode after passing its check, and is marked
attempted (`ACT_RF_ATTEMPTED`), counts as done: the turn is used, as in
play.

### 5.4 Memory

Page 116 holds 5.5K of 8K: the screen grows by an estimated 0.5K
(`describe()`, the action loop). Common memory: `game_pause_events()`
and the tee in `screen_message_to()`, an estimated 40 bytes. To measure.

## 6. Scope

In: R1-R17; the new rows in both modes; PC tests; docs:
[ui_screens.md](ui_screens.md) (a pointer here from R15, R19, D7),
[items.md](items.md) D13 (names are no longer cut), `screen_inv.h`'s
comment, `memory_map.md` if the sizes change.

Later: item descriptions; the player's totals (to-hit with abilities,
the bow and the arrows combined), which belong to the PC screen; using up
counts (items.md section 6).

## 7. Tests

PC tests (`test_ui`):

- `describe()`: each row of the table in 5.2, negative bonuses, two-digit
  sides and bonuses, the longest case in 14 characters.
- A row: count right-aligned, names and details in their columns, in both
  modes; the longest name with the longest details fits 40.
- Browse: equip, take off, drop in one visit; the list and cursor after
  each (D4); one turn used; none when nothing succeeded or only refusals.
- A refused bundled action changes nothing and shows its reason.
- Several uses in one visit: each item used up, its effect applied, one
  turn in all.
- Aim after bundled actions: returned, one turn in all.
- A use that kills the player closes the screen.
- Choose mode unchanged: one action, closes.
- Events of ten actions in one visit don't overflow the queue.

CSpect and hardware checks:

1. `i`: rows aligned, counts right-aligned, weapons' dice and bonuses,
   ammo's bonuses, armour's AC; in-use items in their ink.
2. Wear, take off, drop and drink several items in one visit; each shows its
   words on the status line; SPACE closes; one turn passed (a monster
   moves once); the message window has every action's words.
3. `i` and SPACE with nothing done: no turn.
4. Take off with a full pack: refused, reason shown, the screen stays.
5. `w` from the main screen: choose mode, one letter, closes.
6. The stats panel shows the new AC and attack after closing.
7. Opening and the redraw after an action within R15 on hardware.

## 8. Risks

- **Event handlers during the pause (D2).** `game_on_event` was written
  for play: a handler that starts an animation, moves a sprite or changes
  the level would act under the screen. Each handler reached by equip,
  take off, drop and use must be read first.
- **Use effects that need the map.** The pending ops (teleport, mapping,
  summon, light) do nothing yet; when they are built, one used from the
  screen acts on a hidden map. Each must then either work unseen or make
  its item a closing action.
- **Death in the screen (R3a).** The death path must work from inside a
  screen: close it first, then let play's end of game run.
- **zsdcc and signed numbers.** `describe()` prints signed bytes; read
  its asm (T-ZSDCC-3 in [toolchain.md](guidelines/toolchain.md)).
- **Stats redraws.** Each change of AC redraws the stats panel on the
  hidden buffer (`EV_ATTRS_CHANGED`); unseen, but it adds to R15's redraw
  time. See section 10.

## 9. Questions answered (2026-11-03)

- **Q1 (R3)** Use is **bundled**, as equip, take off and drop are.
- **Q2 (R7)** The count column shows **1** for items that aren't stacks;
  a wand's charges, a light's burn time and a pick's uses are in the
  details.
- **Q3** Stack names **stay** as they are (`15 sheaf of arrows`).
- **Q4 (R9)** Ammo without dice shows its two bonuses: **agreed.**
- **Q5 (R16, D7)** Choose mode stays one action per visit: **agreed, for
  now.**
- **Q6 (R10)** Armour shows `AC 3`, the rest nothing: **agreed.**
- **Q7 (D3)** Slot words go, in-use items shown by ink: **agreed.**

## 10. The question asked: recalculate attributes only on leaving?

**Partly.** What the player sees is already that: the stats panel is on
the hidden buffer, so its new values show only when the screen closes.
But the recalculation itself should stay where it is, in each equip and
take off (`effects_recompute()` in `act_equip.c`):

- **The checks need it.** Each bundled action is checked against the
  player as they are now (R4). Anything that reads derived attributes in
  a check, or a worn effect that changes slots, capacity or two-handed
  rules later, would be judged on stale values if the recompute waited.
- **The actions are shared.** Monsters equip through the same functions.
  Deferring would need a flag on the action or a second path, a rule
  written twice (G-FLOW).
- **It is cheap.** It walks the player's owned entities once and stores
  the attributes; it draws nothing.

What can be deferred, if the hardware measurement (check 7) needs it, is
the **stats panel's redraw** (the `EV_ATTRS_CHANGED` handler), drawing it
once on closing instead of after each action. That changes no game state,
only when a hidden panel is painted.

## 11. As built: differences from the design

- **No `SCREEN_INV_DONE` (5.3).** `screen_inv_run()` gains an out
  parameter, `used`, set to 1 when actions done in the screen used the
  turn, and `item_command()` ends the turn on it. A return value couldn't
  say both "aim this" and "a turn was used", and the turn must stay used
  when the aim that follows is cancelled.
- **The screen calls `act_do()`, not `player_act()` (D1).** The chain of
  banked calls is then as deep as today's (`player_command()` ->
  `screen_inv_run()` -> `act_do()`, against `player_command()` ->
  `player_act()` -> `act_do()`), within T-STACK-2. It calls
  `messages_act_failed()` itself when an action fails in do mode.
- **`screen_message_copy()` (R5, D2)** in `screen_main.c` copies each
  message to a stream as well as the window, one a line. The status panel
  scrolls now (`NGE_PANEL_SCROLL`), so a use's two messages show on its
  two rows. A refusal still goes through `screen_message_to()` and only
  to the status line.
- **The event queue is 32 in the builds,** not 16 (`NGE_EVENT_QUEUE` in the
  Makefile). D2 stands: events are still handled after each action.
- **`gen_item_attack()` already existed (5.2);** nothing was added to the
  generator.
- **Inks:** in use cyan, the pack white; the pack's count cyan.
- **Counts of one:** `1 charge`, `1 turn`, `1 use`.
- **Tests (section 7).** The game over (R3a) is tested through
  `game_is_over()` (the tests stub it), not a real death: the PC tests
  don't link `game.c`. One visit does six actions, with the queue checked
  empty after it, in place of a test of ten. The stats panel's redraw is
  not deferred (section 10): to decide after check 7.
- **Memory:** page 116 5,548 -> 6,399 bytes of 8,192; page 117 46 bytes;
  common memory 95 bytes less (4,430 -> 4,335 free below the stack in a
  debug build), only the new functions and variables, no library
  routines (the map compared before and after). The signed printing
  (`put_signed()`, the AC's power) was read in the asm and is correct.

