# Action: fire

`ACT_FIRE` shoots at the first creature on the line towards a chosen tile: with a launcher
and its ammo, with thrown ammo, or with the actor's own ranged attack. The pattern every
action follows is in [actions.md](actions.md), and the line of fire is in its section 4.11.

Status: built. Code: [act_fire.c](../src/game/actions/act_fire.c).

## 1. Inputs and outputs

| In | Out | Events | Random |
|---|---|---|---|
| `actor` (alive, placed in an area); `x`, `y`: the tile aimed at | `target`: the first creature in line; `item`: the launcher or thrown item, or `NGE_ENTITY_INVALID`; on a hit `amount` and `damage`; in a check, `amount` is the range | `EV_FIRED`, then `EV_HIT` or `EV_MISSED`, then the damage's and the hit effect's | Yes: 1d20 |

## 2. Requirements

- **R1** Launchers with their ammo, thrown items, and creatures' own ranged attacks (a
  spit) are all fired at a chosen tile.
- **R2** A shot is a bolt: it stops at the first creature on its line (actions.md D11).
- **R3** A shot is used up, hit or miss. A stack that runs out is destroyed.
- **R4** The hit hook is the ammo's, not the launcher's (items.md, section 4.6), so a flask
  of oil burns on a hit.
- **R5** A check on the actor's own tile asks whether the actor can fire at all.

## 3. Design

**What shoots,** in order, from `inv_equipped()` (`find_shooter()`):

| In `SLOT_AMMO` | In `SLOT_RANGED` | Who attacks | Used up |
|---|---|---|---|
| `GROUP_THROWN` | Anything | The ammo's own ranged attack | One of the ammo |
| Ammo of the launcher's group | A launcher | The launcher, with the ammo's to-hit and damage bonus | One of the ammo |
| Ammo of another group | A launcher | Refused: `ACT_R_WRONG_AMMO` | |
| Nothing | A launcher | Refused: `ACT_R_NO_AMMO` | |
| Arrows, or nothing | Nothing | The actor's own `ATTACK_RANGED`, or `ACT_R_NO_ATTACK` | Nothing |

Arrows with no launcher are no use, so they fall through to the actor's own attack, and the
player gets `ACT_R_NO_ATTACK`.

**Steps:**

1. **Context:** the shooter above. `item` is the launcher or the thrown item, and `amount`
   its range, from its ranged attack.
2. **Validate:** the shooter, then `act_line()` with the range and `ACT_LINE_BOLT` (D3). The
   actor's own tile is always `ACT_R_NO_TARGET`, and only after the shooter is settled, so
   that reason in a check means the actor can fire, with the range in `amount` (R5).
   `ACT_CHECK` stops here.
3. **Resolve and apply:** the ammo's to-hit and damage bonus, from `gen_item_attack()` on
   its kind, if the ammo isn't the attacker itself. `EV_FIRED` (target the creature in line,
   source the actor, p1 the item), then `act_strike()` with the attacker, `ATTACK_RANGED`,
   the hit hook of the shot (or of the attacker if there is no shot), and the bonuses: the
   ammo's plus the actor's own: to hit from `attack_actor_to_hit()` (its Dexterity modifier,
   and `ATTACK_BLINDED_TO_HIT`, -4, if it is blinded) and to damage from
   `attack_actor_to_damage()` (its Dexterity modifier), both 0 without stats. The action adds
   them up into its to_hit and to_damage before `act_strike()` (actions.md, 4.12). The attacker's own to-hit and
   to-damage still apply: these add to them. A thrown item takes Dexterity too, and so do
   monsters, which have stats. A miss fails with `ACT_R_MISSED`, attempted. Then, hit or miss, the
   shot's `item_count[]` goes down by one, and at 0 the item is destroyed (R3).

`EV_FIRED` comes first in both cases, for the projectile animation (animations.md R14).

**The data.** `attack_hits()` and `attack_damage()` read one entity's row, and ammo with
dice 0 has no attack component, so its bonus is in its kind. They take a bonus, which keeps
the rule in the component (actions.md R12), and `gen_item_attack()` reads it, banked with
the item table. `gen_item_group()` is in the same place.

| Reason | Kind | When | The player is told |
|---|---|---|---|
| `ACT_R_NO_ATTACK` | Refused | Nothing to fire, and no ranged attack of its own | "You have nothing to attack with." |
| `ACT_R_NO_AMMO` | Refused | A launcher, with nothing in `SLOT_AMMO` | "You have no ammunition ready." |
| `ACT_R_WRONG_AMMO` | Refused | The ammo doesn't fit the launcher | "That ammunition doesn't fit." |
| `ACT_R_OUT_OF_RANGE` | Refused | The tile is beyond the range | "That is out of reach." |
| `ACT_R_NO_LINE` | Refused | A wall, a closed door or the map's edge first | "Something is in the way." |
| `ACT_R_NO_TARGET` | Refused | No creature on the line, or the actor's own tile | "There is nothing there to attack." |
| `ACT_R_MISSED` | Attempted | The roll missed | `EV_MISSED`'s message |

Largest case: `EV_FIRED`, `EV_HIT`, `EV_DAMAGED`, `EV_DIED`, `EV_EFFECT_STARTED` and
`EV_ATTRS_CHANGED`, 6 events, within the half of the queue events.md R25 allows.

## 4. Callers

- **The player:** `f`. The command first checks on the player's own tile: any reason but
  `ACT_R_NO_TARGET` is said at once, before a cursor appears. Then `target_choose()` with
  `TARGET_FIRE` and the range in `amount`, and the action on the chosen tile.
- **The AI:** `opt_shoot()`, for a monster with `AI_CAN_RANGED` that sees its target. It
  checks by hand, not with `ai_try()`, and fires only if the check's `target` is its own
  target. Sight sees past creatures and a shot stops at the first, so otherwise monsters
  would shoot each other. It then takes its next option, usually a step.
  `AI_CAN_RANGED` is an own ranged attack or anything in `SLOT_AMMO`; `act_fire()` decides
  the rest (G-ARCH-1). It reads `inv_equipped()`, so it runs only for kinds with an ammo
  slot: today the bandit alone.

The messages say "The bandit fires." when the player sees it, and "readies a" for launchers
and ammo put on. The PC screen shows the ranged attack (ranged_ai_components.md R5).

## 5. Decisions

- **D1 Ammo is used up, hit or miss, and isn't recovered.** Landing ammo would need a new
  entity for each shot, out of 100, or a stack per tile. *Rejected:* splitting off a landed
  stack; arrows lying about would be the commonest item in the game. A chance of breaking,
  with the rest landing, can be added later and needs no change to this design's shape.
- **D2 Fire is one action for launchers, thrown ammo and natural attacks.** All three are
  "the ranged attack of whoever or whatever is shooting", and differ only in which entity
  that is and what is used up. A dart in `SLOT_AMMO` is fired, so there is no `ACT_THROW`.
  *Rejected:* a general throw of any item, which needs rules for what every item does on
  landing, and is a way to lose the quest item.
- **D3 Fire is always a bolt, with no flag.** Ammo that seeks its target isn't wanted, and a
  shot that passes creatures would undo actions.md D11. If it is wanted later, fire reads
  `ITEMF_DIRECT` from the ammo and nothing else changes. *Rejected:* reading the flag now,
  which costs bytes for no item that has it.

## 6. Not built yet

- **Ammo that lands or breaks** (D1), and **seeking ammo** (D3): options for later, with no
  item that needs them yet.
- **Critical hit damage** A natural 20 always hits but does normal damage. Double the damage. 
- **Sleeping  target bonus** Attacks on a sleeping target receive a bonus
- **Unseen target penalty** Attacks on invisible targets receive a penalty (easier to implement as AC+)

## 7. Tests

`test_act`: each row of the shooter table; a shot used up on a hit and on a miss; the ammo's
bonuses and hit effect; Dexterity +2 and -2 on the roll and +2 on the damage;
blinded, -4 on the roll and the damage unchanged; the last of a stack and its slot emptying; a creature in the way hit
in place of the one aimed at; the own-tile check with the range in `amount`; and `act_do()`.
`test_target` has the `f` key, and the same tiles in range as `target_in_range()`. `test_ai`
has `opt_shoot()` holding its fire.
