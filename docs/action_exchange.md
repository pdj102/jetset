# Action: exchange

`ACT_EXCHANGE` puts a stowed item on in place of what is in its slot, in one action: the
item in the slot goes into the pack, and the new item comes out of it into the slot. The
pattern every action follows is in [actions.md](actions.md). The keys and the inventory
screen's part are in [ui_inventory.md](ui_inventory.md), section 12.

Status: built. Code: [act_equip.c](../src/game/actions/act_equip.c), with equip and
unequip.

Without it, an actor wearing a short sword must take it off (`ACT_UNEQUIP`) and then wield
the long sword (`ACT_EQUIP`): two actions, and the first needs a free place in the pack.
Inside the inventory screen a visit is one turn whatever is done (ui_inventory.md R2), so
what the exchange adds there is that a full pack doesn't stop it. From the main screen it
also halves the time.

## 1. Inputs and outputs

| In | Out | Events | Random |
|---|---|---|---|
| `actor` (alive); `item`: the item to put on; `slot`: the slot to use, or `SLOT_NONE` | `slot`: the slot used; `target`: the first item taken off, or `NGE_ENTITY_INVALID` if none | `EV_UNEQUIPPED` for each item taken off, then `EV_EQUIPPED` (p1 the slot) | No |

## 2. Requirements

- **R1** `ACT_EXCHANGE` puts a stowed item in a slot it fits, and stows whatever was in that
  slot, in one action and one turn (actions.md R10). Any actor with an inventory can do it
  (actions.md R1).
- **R2** An exchange of one item for one needs no free place in the pack: the item put on
  leaves the pack as the one taken off enters it.
- **R3** If anything that must come off can't, the exchange is refused and nothing changes:
  a cursed item gives `ACT_R_CURSED`, with the item as `target`.
- **R4** If the slot is free, the exchange equips the item, as `ACT_EQUIP` would. That is a
  success, not a failure.
- **R5** The exchange replaces an item that is in use rather than filling a free slot. An
  item that fits two slots (the rings) replaces the one in the slot asked for. Asked for no
  slot, it replaces the one in the first taken slot, and fills a free slot only if all the
  slots it fits are free.
- **R6** A two-handed weapon put on in place of a weapon *and* a shield takes both off, so
  two items enter the pack and one leaves it. That needs one free place, or it is refused
  with `ACT_R_FULL` (`target`: the shield).
- **R7** A shield put on with a two-handed weapon wielded exchanges the weapon (the shield
  slot is free, but the hands are not). A two-handed weapon put on with only a shield worn
  exchanges the shield. Each is one for one (R2).

## 3. Design

1. **Context:** the actor, the item, the slot asked for.
2. **Validate**, all refused, in this order:
   - the checks shared with equip, `fitting_slots()` (D2): `ACT_R_NO_INVENTORY`,
     `ACT_R_NOT_CARRIED`, `ACT_R_ALREADY_EQUIPPED`, `ACT_R_CANT_EQUIP`, `ACT_R_NO_SLOT`. They
     leave the mask of slots the item fits, the actor has and, if one was asked for, that
     slot;
   - the slot: the first slot in the mask with an item in it, or else the first in the mask
     (R4, R5);
   - what comes off, up to two: the item in the slot, then `inv_hands_taken()`'s answer for
     the slot (R6, R7);
   - none of them is cursed: `ACT_R_CURSED`, `target` the cursed one (R3);
   - room, only when two come off: `ACT_R_FULL`, `target` the second (R6);
   - out: `slot`, and `target` the first to come off, or invalid. `ACT_CHECK` stops here.
3. **Resolve:** nothing to roll.
4. **Apply:** what comes off to `SLOT_NONE`, the item to the slot, `effects_recompute()`
   once, then `EV_UNEQUIPPED` for each item taken off in order, and `EV_EQUIPPED` for the
   item, p1 the slot (D4).

What comes off is held in a two-byte array with loops, which is smaller than two named
locals.

**Cases:**

| Worn | Put on | Slot asked | Result |
|---|---|---|---|
| Short sword | Long sword | None | Sword off, long sword on |
| Nothing in the weapon slot | Long sword | None | Long sword on (R4) |
| Ring L, ring R free | Ring | None | Ring L off, the new one in L (R5) |
| Rings L and R | Ring | `SLOT_RING_R` | Ring R off, the new one in R |
| Rings L and R | Ring | None | Ring L off, the new one in L |
| Sword and shield | Two-handed sword | None | Both off, if the pack has a free place; else `ACT_R_FULL` (R6) |
| Shield only | Two-handed sword | None | Shield off, the two-hander on (R7) |
| Two-handed sword | Shield | None | The two-hander off, the shield on (R7) |
| Cursed helmet | Helmet | None | `ACT_R_CURSED`, nothing changes (R3) |

**Reasons.** No reason is its own (D5). Each is one that equip or unequip gives, with its
text: `ACT_R_NO_INVENTORY`, `ACT_R_NOT_CARRIED`, `ACT_R_ALREADY_EQUIPPED`,
`ACT_R_CANT_EQUIP`, `ACT_R_NO_SLOT`, `ACT_R_CURSED`, `ACT_R_FULL`. `ACT_R_SLOT_TAKEN` can't
happen: what is in the way is exchanged.

## 4. Callers

- **The player:** `e` on the main screen opens the inventory screen's exchange list, and
  `x` on the inventory screen exchanges the selected item. For a ring with both ring slots
  taken, the screen asks which ring goes and passes that slot. Everyone else passes
  `SLOT_NONE`. The screen does the exchange itself (ui_inventory.md, section 12).
- **The AI:** none (D6).

## 5. Decisions

- **D1 A new action, not a change to `ACT_EQUIP`.** Equip keeps its rule: the first free
  slot, refused when every slot is taken. The AI relies on that refusal: `opt_wield()` arms
  monsters with what they pick up and must not strip off a better sword for a worse one,
  which an equip that exchanged would do. *Rejected:* `ACT_EQUIP` exchanging when the slot
  is taken, for that reason, and because the `w` key would then also lose the free ring slot
  (R5 wants the opposite of what `w` does). *Rejected:* a flag in `act_t` asking equip to
  exchange: `act_t` has no spare field, and one action with two rules is two actions in one
  (actions.md D2).
- **D2 It doesn't call equip and unequip; it shares their code.** An action never calls an
  action (actions.md R13), and unequip then equip would need the free place R2 rules out. So
  `act_exchange()` is in `act_equip.c`, and the checks it shares with equip are in
  `fitting_slots()`, which both call. The rules it reads are the ones equip and unequip
  read: `gen_item_fits()`, `inv_equipped()`, `inv_hands_taken()`, `inv_cursed()`,
  `inv_count()` (actions.md R12). *Rejected:* the caller combining unequip and equip
  (actions.md D6): two turns outside the screen, and a full pack refuses the first.
- **D3 The slot is an input, as for equip.** `slot` is the slot to exchange in, or
  `SLOT_NONE` for the action to choose by R5. Only the rings need it. *Rejected:* `target`
  as the item to replace: the same answer for the rings, but it needs a check that the
  target is in a slot the item fits, and "nothing to replace" (R4) has no target to name.
- **D4 The existing events, in order: off, then on.** `EV_UNEQUIPPED` for each item taken
  off, then `EV_EQUIPPED`. The messages read "You take off a short sword." then "You equip a
  long sword.", and every handler of the two events (the stats panel, messages, monsters
  seen arming) works unchanged. The worn effects are recomputed once, before the events, so
  there is one `EV_ATTRS_CHANGED`. *Rejected:* a new `EV_EXCHANGED` with both items: a new
  message with two names, and a new case in every handler of the two events.
- **D5 No new reasons.** Every refusal is one that equip or unequip already gives, with its
  text.
- **D6 The AI doesn't exchange.** The action serves monsters (R1), but no option calls it:
  that needs a rule for which of two items is better, which nothing has. `opt_wield()` stays
  on `ACT_EQUIP`.

## 6. Not built yet

- **Monsters exchanging** (D6): a rule for which of two items is better, and an option that
  uses it.

## 7. Tests

`test_act`:

- every row of the cases table, with the events in order and the slots after;
- R2: a full pack exchanges one for one; R6 with a full pack is `ACT_R_FULL` and changes
  nothing, and with a free place both come off;
- a cursed item in the slot: `ACT_R_CURSED`, `target` the cursed item, nothing changed. A
  cursed shield in the way of a two-hander isn't tested, as no shield kind is cursed;
- each shared refusal, as for equip;
- check mode: the same answer, the same `slot` and `target`, nothing changed, no events;
- worn effects: the old item's effect gone and the new one's counted, one
  `EV_ATTRS_CHANGED`;
- `act_do(ACT_EXCHANGE)` matches the direct call;
- `ACT_EQUIP` unchanged: it still refuses a taken slot (D1).

`test_ui` has the keys and the ring question.
