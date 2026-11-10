# Action: equip

`ACT_EQUIP` puts a stowed item into a free equipment slot: it wields a weapon or puts on a
ring. The pattern every action follows is in [actions.md](actions.md). Slots and what fits
them are specified in [items.md](items.md).

Status: built. Code: [act_equip.c](../src/game/actions/act_equip.c), with exchange and
unequip.

## 1. Inputs and outputs

| In | Out | Events | Random |
|---|---|---|---|
| `actor` (alive); `item`; `slot`: the slot to use (`SLOT_*`), or `SLOT_NONE` for the first free slot the item fits | `slot`: the slot used; `target`: the item in the way, when refused for one | `EV_EQUIPPED` (p1 the slot) | No |

## 2. Requirements

- **R1** Equip fills a free slot only. If every slot the item fits is taken, it is refused,
  and `target` names the item in the way. Putting something on in place of another item is
  `ACT_EXCHANGE` ([action_exchange.md](action_exchange.md)).
- **R2** A two-handed item (`ITEMF_TWO_HANDS`) needs `SLOT_SHIELD` free as well, and a
  shield needs no two-handed item wielded (`inv_hands_taken()`).
- **R3** The item's worn effect counts before the event, so the stats panel shows it at
  once.

## 3. Design

1. **Context:** the actor, the item, the slot asked for.
2. **Validate**, all refused, in this order. The first five are `fitting_slots()`, which
   exchange shares:
   - the actor has an inventory;
   - it has the item;
   - the item is stowed;
   - it fits some slot (`gen_item_fits()`);
   - it fits a slot the actor has (`inv_slots_of()`), and the one asked for;
   - one of those slots is free, the first in slot order; if none, `target` is the item in
     the last of them;
   - the hands: nothing in the way for that slot (R2); if something is, `target` is it.

   `slot` is set. `ACT_CHECK` stops here.
3. **Resolve:** nothing to roll.
4. **Apply:** `item_slot[]` set, `effects_recompute()`, then `EV_EQUIPPED`, target the item,
   source the actor, p1 the slot.

Equipped items don't count towards the pack's capacity.

| Reason | Kind | When | The player is told |
|---|---|---|---|
| `ACT_R_NO_INVENTORY` | Refused | The actor can't carry things | None |
| `ACT_R_NOT_CARRIED` | Refused | The actor doesn't have the item | None |
| `ACT_R_ALREADY_EQUIPPED` | Refused | It is in a slot already | "You are already using that." |
| `ACT_R_CANT_EQUIP` | Refused | It fits no slot (a potion) | "You can't equip that." |
| `ACT_R_NO_SLOT` | Refused | The actor has no slot it fits, or not the one asked for | "You have nowhere to put that." |
| `ACT_R_SLOT_TAKEN` | Refused | Every slot it fits is taken, or the hands are; `target` is the item in the way | "You are already using a (item)." |

## 4. Callers

- **The player:** `w`, which opens the inventory screen's wear list, or `w` on the
  inventory screen while browsing, always with `SLOT_NONE`. The screen does the equip
  itself.
- **The AI:** `opt_wield()`, for an idle monster: it readies what it fights with (weapons,
  launchers and ammo) from its pack, testing `gen_item_fits()` first, and leaves armour
  alone. Because equip refuses a taken slot, a monster never takes off a better sword for a
  worse one.

## 5. Tests

`test_act`: the first free slot and a slot asked for, the rings' two slots, each refusal
with its `target`, a two-handed item with a shield worn and a shield with a two-hander
wielded, the worn effect counted before the event, and check mode.
