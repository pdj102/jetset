# Action: learn

`ACT_LEARN` learns a spell from a spell book the actor carries, into a free spell slot of
the spell's level. The pattern every action follows is in [actions.md](actions.md). Spell
slots, books and what is in them are in [spells.md](spells.md), section 10, whose
requirements R46 to R51 this action meets.

Status: built. Code: [act_learn.c](../src/game/actions/act_learn.c).

## 1. Inputs and outputs

| In | Out | Events | Random |
|---|---|---|---|
| `actor` (alive); `item`: the book; `spell` (`SP_*`), or `SP_NONE` to ask about the book only, in check mode (D2) | None | `EV_SPELL_LEARNED` (p1 the spell), from `spell_learn()` | No |

## 2. Requirements

- **R1** Only the player, with mana and spells components, learns: the Mage and the Dev,
  not the Fighter (actions.md R14).
- **R2** A book is read only from the pack: stowed, not equipped, not on the floor.
- **R3** A spell is learned only into a free slot of its level.
- **R4** The book is kept.

## 3. Design

1. **Context:** the book's kind; the spell's level, once there is a spell.
2. **Validate**, all refused, in this order:

   | Test | Reason | The player is told |
   |---|---|---|
   | The actor has the item, stowed (R2) | `ACT_R_NOT_CARRIED` | None |
   | It is a book (`ITEM_BOOK`) | `ACT_R_NOT_BOOK` | "That isn't a spell book." |
   | The actor is the player, with mana and spells (R1) | `ACT_R_CANT_LEARN` | "The spells mean nothing to you." |
   | `spell` is `SP_NONE`: success in check mode; an assert in do mode (D2) | | |
   | The spell is in the book (`spell_in_book()` with `gen_item_book_spells()`) | `ACT_R_NOT_IN_BOOK` | "That spell isn't in the book." |
   | Not known already | `ACT_R_KNOWN` | "You know that spell already." |
   | `spell_slots()` at its level above 0 | `ACT_R_BEYOND` | "That spell is beyond you for now." |
   | `spell_free()` at its level above 0 | `ACT_R_SLOTS_FULL` | "No room for more spells of that level." |

   An equipped book gives `ACT_R_NOT_CARRIED`, not `ACT_R_EQUIPPED`, as no book fits a slot.
   `ACT_CHECK` stops here.
3. **Resolve:** nothing to roll.
4. **Apply:** `spell_learn()`, the one way to add a spell, which posts `EV_SPELL_LEARNED`
   ("You learn Hold."). The book is kept (R4).

## 4. Callers

- **The player:** `b`, which opens the inventory screen's "Study" list of books, or `b` or
  ENTER on a book on the inventory screen. The inventory screen asks with `SP_NONE`, so a
  Fighter's book is refused with its reason before it opens. `item_command()` then opens the
  book screen (`screen_book_run()`, page 104), which asks `ACT_LEARN` in check mode for each
  row's colour (white: can learn, cyan: `ACT_R_KNOWN`, red: anything else), and does each
  learn itself, staying open. Closing it uses one turn if any spell was learned
  (spells.md, D22).
- **The AI:** none: only the player learns (R1).

## 5. Decisions

- **D1 Learning is an action.** It has the four steps of every action, so the book screen
  asks it in check mode for each row's colour and reason, as the spell list asks
  `ACT_CAST`. *Rejected:* calling `spell_learn()` from the screen, which puts the rules in
  the UI, where the PC tests can't reach them without driving the screen.
- **D2 With no spell chosen, the check asks only about the book.** `SP_NONE` stops
  validation after the actor's test, with success in check mode, so the inventory screen can
  refuse a Fighter's book with its reason before the book opens. `SP_NONE` is also
  `SLOT_NONE`, the value the inventory screen already puts in that byte. In do mode it is an
  assert. This is how `ACT_CAST` treats a bolt on the actor's own tile.
- **D3 Two refusals for no room.** "Beyond you" when the class has no slot at that level
  yet, and "no room" when it has slots and all are full. Both are red in the book, and the
  status line says which.

## 6. Tests

`test_act` (`test_learn`): each refusal in order; `SP_NONE` in check mode; a learn posting
`EV_SPELL_LEARNED` and keeping the book; slots filling; and check mode. `test_ui` has the book
screen's rows and keys.
