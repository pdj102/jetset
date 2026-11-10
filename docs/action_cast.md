# Action: cast

`ACT_CAST` casts a spell the actor knows, or a creature's ability: its effect, at its power,
on the actor, on the first creature on a line, or on the creature on a chosen tile, as the
spell's shape says. The pattern every action follows is in [actions.md](actions.md), and the
line of fire is in its section 4.11. What spells there are, who knows them and what they
cost are in [spells.md](spells.md), whose requirements R12 to R18 this action meets. Mana is
in [mana.md](mana.md).

Status: built. Code: [act_cast.c](../src/game/actions/act_cast.c).

## 1. Inputs and outputs

| In | Out | Events | Random |
|---|---|---|---|
| `actor` (alive, placed in an area); `spell` (`SP_*`); `x`, `y` for a bolt or direct spell | `target`: the actor, the first creature in line, or the one on the tile. `spell` is left as it was, so a context can be checked and then done | `EV_CAST` (p1 the spell), `EV_MANA_SPENT` if it cost MP, then the effect's | No |

## 2. Requirements

- **R1** Spells and creature abilities are one action, for every caster.
- **R2** The line mode comes from the spell's shape (actions.md R26):

  | Shape | Target | Line |
  |---|---|---|
  | `SPELL_SELF` | The actor | None: `x`, `y` aren't read |
  | `SPELL_BOLT` | The first creature on the line | `ACT_LINE_BOLT` |
  | `SPELL_DIRECT` | The creature on the chosen tile, past any on the way | `ACT_LINE_DIRECT` |

- **R3** A bolt or direct spell can't target the caster. A spell that helps the caster is
  `SPELL_SELF`, so the player gets no prompt.
- **R4** An ability is level 0, which costs nothing: it needs no mana component and tests no
  MP (D1).
- **R5** A valid cast always happens: there is no casting roll, and a bolt can't miss. A
  saving throw belongs to the effect.
- **R6** The effect names the actor as its source, so a kill earns its XP (actions.md D13).
- **R7** A check on the actor's own tile asks whether the actor can cast the spell at all:
  `ACT_R_NO_TARGET` there means yes.

## 3. Design

1. **Context:** `spell_get()` into a local copy, as the table is in the spells' page, and its
   fields into locals (T-ZSDCC-1); the cost, `spell_cost()` of its level.
2. **Validate**, all refused, in this order:
   - the actor knows it (`spell_knows()`);
   - with a cost above 0, the actor has a mana component and the MP (`MANA_HAS()`);
   - `SPELL_SELF`: the target is the actor. Otherwise `act_line()` with the spell's range
     and its mode (R2), which refuses the actor's own tile with `ACT_R_NO_TARGET`, after the
     spell and its cost (R7).

   `ACT_CHECK` stops here.
3. **Resolve:** nothing to roll (R5).
4. **Apply:** `EV_CAST` (target the target, source the actor, p1 the spell); `mana_spend()`
   if the cost is above 0, which posts `EV_MANA_SPENT`; then `effect_apply()` at the spell's
   power, with the actor as the source (R6). MP is spent before the effect, so a spell can't
   restore what paid for it and then be charged.

| Reason | Kind | When | The player is told |
|---|---|---|---|
| `ACT_R_NO_SPELL` | Refused | The actor doesn't know it | "You don't know that spell." |
| `ACT_R_NO_MANA` | Refused | Not enough MP, or no mana | "You haven't the mana." |
| `ACT_R_OUT_OF_RANGE` | Refused | Beyond the spell's range | "That is out of reach." |
| `ACT_R_NO_LINE` | Refused | A wall, a closed door or the map's edge first | "Something is in the way." |
| `ACT_R_NO_TARGET` | Refused | No creature there, or the actor's own tile | "There is nothing there to attack." |

A rooted caster still casts, as nothing here tests being rooted. Casting at another
creature counts as attacking it: `ai_on_event()` reacts to `EV_CAST` as to a hit
(spells.md R18).

| Case | Events |
|---|---|
| A bolt that kills | 5: `EV_CAST`, `EV_MANA_SPENT`, `EV_DAMAGED`, `EV_DIED`, `EV_EFFECT_STARTED` |
| A timed effect | 4: `EV_CAST`, `EV_MANA_SPENT`, `EV_EFFECT_STARTED`, `EV_ATTRS_CHANGED` |

## 4. Callers

- **The player:** `m` opens the spell list (`screen_spell_run()`, page 104), which checks
  each spell on the player's own tile and shows the reason for one that can't be cast. A
  bolt or direct spell then asks for a tile with `choose_tile()`, `TARGET_CAST` and the
  spell's range. Backing out of either uses no turn.
- **The AI:** `opt_cast()`, first in a hunting monster's options, for kinds with
  `AI_CAN_CAST`:
  - hurt (half its hit points or less), each `SPELL_AI_HURT` spell it knows, on itself,
    with `ai_try()`;
  - seeing its target, each `SPELL_AI_TARGET` spell it knows, from a random one, checked by
    hand: it casts only if the check's `target` is its own target, as `opt_shoot()` does. The
    random start is drawn outside the action, so actions.md R5 holds.

  Out of MP, the checks refuse, so it falls back on its other options.

## 5. Decisions

- **D1 Abilities are spells at no cost.** A gaze or a shaman's spell is one action,
  `ACT_CAST`, with a row saying what it does, so a creature's ability is data (G-ARCH-7) and
  not an action per monster. Spitting is `ACT_FIRE` with a natural ranged attack.
  *Rejected:* `ACT_GAZE`, `ACT_BREATHE` and so on, which spend the section on verbs that
  differ only in their data.
- **D2 A shape says who a spell reaches, with three values.** `SPELL_BOLT` passes
  `ACT_LINE_BOLT` and `SPELL_DIRECT` passes `ACT_LINE_DIRECT`, so the shape stays the one
  field that says who a spell reaches. Hold and Enfeeble are direct, as the wands of sleep
  and slowness are, and the damaging spells are bolts (spells.md 4.12). *Rejected:* a flag
  beside the shape, which would be two fields for one answer.

## 6. Not built yet

From [spells.md](spells.md), section 6, the parts that touch the action:

- **Area spells and breath:** a `SPELL_BALL` and a walk over the targets in `act_cast()`,
  with each target's events (about 3 each) against a queue of 32. How it picks targets, and
  whether it hits the caster, needs deciding.
- **A shriek:** a self-targeted effect with an op that wakes the level, which is a stimulus,
  not an effect on a target.
- **Spells on doors, walls and tiles** (knock, stone to mud): an op that reads the caster's
  surroundings, or a tile passed to `effect_apply()`.
- **Spells that help others:** `SPELL_DIRECT` can reach another creature, but no monster has
  an ally.
- **A projectile animation for spells:** `EV_CAST` carries what it needs (animations.md R14).
- **Lost MP** if the effect can't apply (actions.md, section 8).

## 7. Tests

`test_act`: each shape; each refusal in order; an ability with no mana component; MP spent
and `EV_MANA_SPENT` only with a cost; the actor as source; a direct spell past a creature; the
own-tile check; `spell` unchanged; and check mode. `test_ai` has `opt_cast()` holding a spell
that would reach someone else, and `test_ui` the spell list.
