# Inventory screen, second version: requirements and design

Status: implemented (2026-11-03); the PC tests pass. The CSpect and
hardware checks in section 7 are not run yet. Agreed on 2026-11-03, with the
questions answered in section 9. Section 11 lists where the build differs
from the design. Section 12, the exchange key, agreed and implemented
2026-11-04; its PC tests pass and its CSpect checks (12.5) passed 2026-11-04.

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


## 12. Exchange: requirements and design

Status: agreed and implemented 2026-11-04, with Q8 to Q10 answered; the PC tests
pass, and the CSpect checks in 12.5 passed 2026-11-04; hardware not yet. Section 12.7 lists
where the build differs. The action is `ACT_EXCHANGE`,
[actions.md](actions.md) section 6.14 (R40-R46, D31-D36): what it takes off and puts on,
and why. This section is its keys and its place in the screen.

### 12.1 Requirements

- **R18** In browse mode, `x` exchanges the selected item: a stowed item that can be
  equipped goes into its slot, and what was there goes into the pack. It is bundled (R1):
  the screen stays open and redraws, and the visit's one turn covers it (R2).
- **R19** `x` needs no free place in the pack (actions.md R41), and on a free slot it
  equips, as `w` does (actions.md R43).
- **R20** A refused exchange shows its reason on the status line and changes nothing (R4),
  as for the other bundled actions: "It is cursed: it won't come off.", "You can't carry any
  more." (actions.md R45).
- **R21 (New)** A ring with both ring slots in use asks which one goes: the list shows the
  two worn rings, lettered, under the title "Which Shall Go?", and a letter (or the cursor and
  ENTER) chooses; SPACE goes back to browse with nothing done. With one ring worn, `x`
  replaces that one and doesn't ask (actions.md R44). *Reason: the player must say which
  (user requirement 6), and the rings are the only items that fit two slots, so no other
  item asks.*
- **R22 (New)** `x` on an item in use is refused, as `w` is ("You are already using that.",
  `ACT_R_ALREADY_EQUIPPED`): the selected item is always the one put on (Q9).
- **R23 (Changed)** On the main screen the key is **`e`**, not `x`, which is look. It opens
  choose mode, "Exchange Thy Gear", listing the equippable items in the pack, as `w` does;
  a letter exchanges and closes (R16, D7), with R21's question for a ring. *Reason: `x` is
  look on the main screen, and stays look (Q8).*
- **R24** The help line shows ` x:swap` for a stowed item that can be equipped only when
  `w` is refused because the slot is taken. When the slot is free, `w` and `x` do the same
  and only ` w:wear` is shown. *Reason: the line is 40 characters, and
  `jk:select w:wear x:swap d:drop SPACE:done` is 41.*

### 12.2 Decisions

- **D8 `x` is checked and done as the other bundled actions are.** `allowed(ACT_EXCHANGE)`
  checks with `slot` `SLOT_NONE`, then `act_do(ACT_EXCHANGE, ACT_DO)` and the events, as in
  5.3. The cursor follows the item put on, now in use (D4); the item taken off appears in
  the pack. *Rejected:* the screen doing `r` and then `w`: it needs a free place, which is
  the case the request is for.
- **D9 The ring question is a choose list in the same screen.** On `x`, if the item fits
  both ring slots and both are taken, the screen switches its list to the two worn rings
  (a new `LIST_REPLACEABLE`, built from the slots the item fits), lettered, then does the
  exchange with `slot` set to the chosen ring's slot, and switches back to browse. It reuses
  choose mode's letters and drawing. *Rejected:* a question on the status line ("l: left,
  r: right"): the screen shows no left and right since D3, so the player would have to
  remember which ring is where; the names say which. *Rejected:* exchanging the left ring
  always: breaks requirement 6.
- **D10 The two messages, off then on, reach the status line and the log** (R5, actions.md
  D34). The status line has two rows (section 11), so both show.

### 12.3 Layout

The ring question, after `x` on a ring of armour +1 with two rings worn:

```
row 0  |       Which Shall Go?        Pack 20/20 |
row 2  | a)   1 ring of health +1                |   both in use: cyan
       | b)   1 ring of strength +1              |
row 29 |                                         |
row 31 | Letter/ENTER: take it off  SPACE: back
```

### 12.4 Memory

Page 116, estimated 200 bytes: the `x` key, the help rule (R24), `LIST_REPLACEABLE` and the
switch to the ring question, the `modes[]` row (7 bytes, actions.md 4.9) and the main
screen's choose mode. Page 114: the `e` key, about 10 bytes. Common: none.

### 12.5 Tests

PC tests (`test_ui`):

- `x` on a sword with a sword wielded and a full pack: exchanged, the screen open, one turn
  for the visit, the cursor on the new sword.
- `x` on a free slot: equipped, as `w`.
- `x` refused (a cursed item worn): the reason shown, nothing changed, no turn if nothing
  else was done.
- The ring question: both worn, `b` replaces the second; SPACE does nothing; one worn, no
  question.
- `x` on an item in use: refused.
- The help line: ` x:swap` only when `w` is refused for a taken slot; never over 40.
- Main screen `e`: choose mode, a letter exchanges and closes.

CSpect and hardware checks:

1. With a full pack, `i`, `x` on a second weapon: both messages on the status line, the
   pack's count unchanged, SPACE closes, one turn passed.
2. Two rings worn, `x` on a third: the question, a letter, the chosen one in the pack.
3. `x` on a free slot: equipped, one message.
4. `x` with a cursed item worn: refused, the screen stays.
5. `e` from the main screen: choose mode, a letter, closes; one turn.
6. The stats panel's AC and attack after closing.

### 12.6 Questions answered (2026-11-04)

- **Q8 (R23)** The main-screen key is **`e`**; look stays on `x`.
- **Q9 (R22)** `x` on a worn item is **refused**.
- **Q10 (R21, R23)** The titles **"Which Shall Go?"** and **"Exchange Thy Gear"**: agreed.

### 12.7 As built: differences from the design

- **Choose mode does the exchange itself (R23, D7).** `screen_inv_run()` returns an item, and
  the ring question's slot has no way out, so `e`'s letter exchanges inside the screen, as a
  bundled action would, then closes; it returns `SCREEN_INV_BACK` with `*used` set. One action,
  then the screen closes, as R16 wants. `item_command()` needs no change.
- **The check before the question (D8).** `x` on an item that will ask is not checked with
  `SLOT_NONE` first: that checks the left ring, so a cursed left ring would refuse an exchange
  of the right one. The exchange is checked with the slot chosen, and a refusal shows its reason
  (the cursed-ring test). The help line's ` x:swap` still asks with `SLOT_NONE`.
- **The question asks whenever two or more items in use could go,** not only "the rings":
  only rings fit two slots, so it is the same today, and needs no list of kinds.
- **The help line's table is one array of key and action,** walked by pointer: an index into
  a table in page 116 gave z80asm's "integer range" warning (as in pc_screen.md).
- **`MODE_WHICH` is `ACT_COUNT + 2`,** not `+ 1`, which is `KEY_STAY`: the question's answer is
  its mode, and equal to `KEY_STAY` it never closed (found by the PC tests).
- **Memory:** page 116 6,399 -> 7,031 bytes of 8,192, where 12.4 said about 200 more: the ring
  question, `relist()` and `draw_title()` split out of the action loop, and the new mode's row
  in `modes[]`. 1,161 bytes are left. Page 114: 17 bytes for `e`. Common memory: as
  actions.md section 9 says.
- **Tests:** `test_ui`'s exchange block works out each item's row from the list's rule (in
  use by slot, then the pack by ID) rather than fixing them, as destroyed items' IDs are reused.
  The question's own frame isn't seen by the tests (they see the last frame); what it does is,
  and the title it leaves.
