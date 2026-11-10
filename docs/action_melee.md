# Action: melee

`ACT_MELEE` attacks an entity next to the actor with the wielded weapon, or with the
actor's own attack. The pattern every action follows is in [actions.md](actions.md). The
attack rules are in [attack.h](../src/game/components/attack.h).

Status: built. Code: [act_melee.c](../src/game/actions/act_melee.c), with the strike in
`act_shared.c`.

## 1. Inputs and outputs

| In | Out | Events | Random |
|---|---|---|---|
| `actor` (alive, placed in an area); `target`, or with `NGE_ENTITY_INVALID` a direction `dx`, `dy` | `target`; `item`: the weapon, or `NGE_ENTITY_INVALID` for the actor's own attack; on a hit `amount` (the damage taken) and `damage` (its type) | `EV_HIT` or `EV_MISSED`, then the damage's and the hit effect's | Yes: 1d20 |

## 2. Design

1. **Context:** the weapon in `SLOT_WEAPON` (`attack_weapon()`), and the attacker, whose
   attack component is used: the weapon, or the actor if it has none. With no `target`, the
   first entity with health on the tile in direction `dx`, `dy`. `target` and `item` are
   set.
2. **Validate**, all refused, in this order:
   - the attacker has a melee attack;
   - there is a target, and it isn't the actor;
   - it is alive, with health above 0;
   - it is in the actor's area, not on the actor's tile, and within the attack's reach
     (`nge_map_within()` with its range, 1 today).

   `ACT_CHECK` stops here.
3. **Resolve and apply:** `act_strike()` with the attacker, `ATTACK_MELEE`, the attacker's
   hit hook, and the actor's own bonuses: to hit from `attack_actor_to_hit()` (its Strength
   modifier, and `ATTACK_BLINDED_TO_HIT`, -4, if it is blinded) and to damage from
   `attack_actor_to_damage()` (its Strength modifier), both 0 without stats (actions.md,
   4.12). The attack's own to-hit and to-damage still apply: these add to them. A hit posts `EV_HIT` (p1 the roll, p2 the
   weapon), damages the target, and applies the hit hook of the weapon, or of the actor if
   it has none, to a target left alive: a poisonous bite. A miss posts `EV_MISSED` and fails
   with `ACT_R_MISSED`, attempted.

A hit is 1d20 plus the attack's to-hit plus the actor's Strength modifier (less 4 if it is
blinded) at or above the target's AC; 20 always hits and 1 always misses (`attack_hits()`). Damage is the attack's
dice plus its to-damage plus the Strength modifier, at least 1 (`attack_damage()`). Monsters
have stats too, so their Strength counts.

| Reason | Kind | When | The player is told |
|---|---|---|---|
| `ACT_R_NO_ATTACK` | Refused | No weapon with a melee attack, and none of its own | "You have nothing to attack with." |
| `ACT_R_NO_TARGET` | Refused | Nothing there, or the actor itself | "There is nothing there to attack." |
| `ACT_R_CANT_DAMAGE` | Refused | No health, or dead | "You can't attack that." |
| `ACT_R_OUT_OF_RANGE` | Refused | Elsewhere, on its own tile, or beyond reach | "That is out of reach." |
| `ACT_R_MISSED` | Attempted | The roll missed | `EV_MISSED`'s message |

Breakable features (a web, a boulder) have health, so they are struck by the same code
([features.md](features.md), 4.10).

Largest case: `EV_HIT`, `EV_DAMAGED`, `EV_DIED`, `EV_EFFECT_STARTED` and
`EV_ATTRS_CHANGED`, within the half of the queue events.md R25 allows.

## 3. Callers

- **The player:** bump: a move refused by something with health becomes a melee on it. A
  step into a known web strikes it.
- **The AI:** `opt_attack()`, on its target next to it.

## 4. Tests

`test_act`: hits, misses, and hit effects at the kind's power; Strength +2 and -2 on the
roll, +2 on the damage of the weapon and of the actor's own attack; blinded, -4 on the roll and
the damage unchanged; the weapon's attack and the
actor's own; each refusal; reach to all 8 neighbours and no further; a target on the actor's
own tile refused; and check mode drawing no numbers.
