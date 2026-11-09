#!/usr/bin/env python3
"""
Difficulty balance check (docs/difficulty.md).

Reads the game's own tables from the C sources, so it follows the game as
it changes: the depths, the XP table, the per-level counts in the level
generator, and every creature, item and feature kind with its band of
depths and rarity. It works out what an average level holds at each depth,
then compares the player's pace with the targets below.

    python3 scripts/balance.py            # the report; exit 1 if a check fails
    python3 scripts/balance.py --sim 2000 # also simulate 2000 games for the spread
    python3 scripts/balance.py --clear 0.75 --gold

If the C changes shape (a constant renamed, a table moved), the script
stops with an error naming what it could not find (R12): fix the parser or
the model, never let it guess.
"""

import argparse
import random
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# ---- Targets: the design's choices (docs/difficulty.md, section 4) --------

TARGETS = {
    # D2: the level on arrival at depth d is d (level 1 at depth 1)
    "arrive_level": lambda depth: depth,
    # D3: how much of each depth the target player clears (1.0: every monster)
    "clear": 1.0,
    # D4: off until treasure turns into gold in play (pc_gain_gold)
    "gold_xp": False,
    # D5: |level on arrival - target|: within ok passes, within warn warns, else fails
    "ok": 0.5,
    "warn": 1.0,
    # D6: XP beyond the last level is accepted (Q2): C2 reports it, never warns
    # D7: monster kinds that can appear at every depth, at least
    "kinds_min": 3,
    # D9: hits for the average monster to kill the player at the target level (Q3)
    "hits": (5, 9),
}

# ---- Sources --------------------------------------------------------------

SRC = {
    "level_h": "src/game/level/level.h",
    "level_internal_h": "src/game/level/level_internal.h",
    "pc_h": "src/game/components/pc.h",
    "pc_c": "src/game/components/pc.c",
    "creatures": "src/game/gen/gen_creature_pdata.c",
    "items": "src/game/gen/gen_item_pdata.c",
    "features": "src/game/gen/gen_feature_pdata.c",
}


class ParseError(Exception):
    pass


def read(key):
    path = ROOT / SRC[key]
    if not path.exists():
        raise ParseError(f"{SRC[key]} not found")
    return path.read_text()


def define(text, name, key):
    m = re.search(rf"#define\s+{name}\s+(\d+)", text)
    if not m:
        raise ParseError(f"#define {name} not found in {SRC[key]}")
    return int(m.group(1))


def field(block, pattern, name, kind, default=None):
    m = re.search(pattern, block)
    if m:
        return int(m.group(1))
    if default is not None:
        return default
    raise ParseError(f"{name} not found for {kind}")


def kind_blocks(text, prefix):
    """The designated initialisers [PREFIX_X] = { ... } of a table, by name."""
    parts = re.split(rf"\[({prefix}_[A-Z0-9_]+)\]\s*=\s*\{{", text)
    return {parts[i]: parts[i + 1] for i in range(1, len(parts) - 1, 2)
            if not parts[i].endswith("_COUNT")}   # the table's own size, [CR_COUNT] = {


def where(block, kind, bottom):
    block = block.replace("LEVEL_DEPTH_BOTTOM", str(bottom))
    m = re.search(r"\.where\s*=\s*\{\s*\.depth\s*=\s*(\d+),\s*\.depth_max\s*=\s*(\d+),"
                  r"\s*\.rarity\s*=\s*(\d+)", block)
    if not m:
        raise ParseError(f".where not found for {kind}")
    return tuple(int(g) for g in m.groups())


def load():
    level_h = read("level_h")
    li = read("level_internal_h")
    pc_h = read("pc_h")
    pc_c = read("pc_c")

    g = {"bottom": define(level_h, "LEVEL_DEPTH_BOTTOM", "level_h")}
    for name in ("ROOMS_MIN", "ROOMS_MAX", "ITEMS_MIN", "ITEMS_MAX", "MONSTERS_MIN",
                 "MONSTERS_MAX", "LOCKS_MAX", "FEATURES_MAX", "CONTENTS_MAX"):
        g[name.lower()] = define(li, "LEVEL_" + name, "level_internal_h")
    g["level_min"] = define(pc_h, "PC_LEVEL_MIN", "pc_h") if "PC_LEVEL_MIN" in pc_h else 1
    g["level_max"] = define(pc_h, "PC_LEVEL_MAX", "pc_h")
    g["hit_die"] = define(pc_h, "PC_HIT_DIE", "pc_h")

    m = re.search(r"level_xp\[[^\]]*\]\s*=\s*\{([^}]*)\}", pc_c)
    if not m:
        raise ParseError(f"level_xp[] not found in {SRC['pc_c']}")
    g["level_xp"] = [int(x.replace(",", "")) for x in re.findall(r"\d[\d,]*", m.group(1))]
    if len(g["level_xp"]) != g["level_max"] - g["level_min"] + 1:
        raise ParseError("level_xp[] length does not match PC_LEVEL_MIN..PC_LEVEL_MAX")

    bottom = g["bottom"]
    creatures = []
    for kind, b in kind_blocks(read("creatures"), "CR").items():
        d0, d1, rarity = where(b, kind, bottom)
        if rarity == 0:
            continue   # never at random: placed by hand (not counted, R10)
        melee = re.search(r"\.melee\s*=\s*\{[^}]*\.dice\s*=\s*(\d+),\s*\.sides\s*=\s*(\d+),"
                          r"\s*\.bonus\s*=\s*(-?\d+)", b)
        creatures.append({
            "kind": kind, "d0": d0, "d1": d1, "rarity": rarity,
            "xp": field(b, r"\.xp\s*=\s*(\d+)", "xp", kind),
            "hp": field(b, r"\.hp_max\s*=\s*(\d+)", "hp_max", kind),
            "ac": field(b, r"\.ac\s*=\s*(\d+)", "ac", kind),
            "dmg": (int(melee.group(1)) * (int(melee.group(2)) + 1) / 2 + int(melee.group(3)))
                   if melee else 0.0,
        })

    items = []
    for kind, b in kind_blocks(read("items"), "IT").items():
        d0, d1, rarity = where(b, kind, bottom)
        if rarity == 0:
            continue
        treasure = "ITEM_TREASURE" in b
        items.append({"kind": kind, "d0": d0, "d1": d1, "rarity": rarity,
                      "gold": field(b, r"\.count\s*=\s*(\d+)", "count", kind, 1) if treasure else 0})

    features = []
    for kind, b in kind_blocks(read("features"), "FT").items():
        d0, d1, rarity = where(b, kind, bottom)
        if rarity == 0:
            continue
        features.append({"kind": kind, "d0": d0, "d1": d1, "rarity": rarity,
                         "holds": field(b, r"\.holds\s*=\s*(\d+)", "holds", kind, 0)})

    if not creatures or not items:
        raise ParseError("no random creatures or items parsed: has the table format changed?")
    return g, creatures, items, features


# ---- The model (docs/difficulty.md, section 5) -----------------------------

def pool(table, depth):
    return [k for k in table if k["d0"] <= depth <= k["d1"]]


def weighted_mean(kinds, key):
    total = sum(k["rarity"] for k in kinds)
    return sum(k[key] * k["rarity"] for k in kinds) / total if total else 0.0


def mean_between(lo, hi):
    return (lo + hi) / 2   # roll_between() in level_gen.c: uniform, both ends included


def per_level(g, items, features, depth):
    """Expected monsters, item draws and gold at one depth (the model: M1-M4)."""
    monsters = mean_between(g["monsters_min"], g["monsters_max"])
    feats = pool(features, depth)
    n_features = mean_between(0, g["features_max"])
    containers = [f for f in feats if f["holds"] > 0]
    share = sum(f["rarity"] for f in containers) / max(1, sum(f["rarity"] for f in feats))
    each = weighted_mean([dict(f, h=(1 + f["holds"]) / 2) for f in containers], "h")
    contents = min(g["contents_max"], n_features * share * each)
    locked = g["locks_max"] / 2          # M3: about one a level (level_gen.c, place_locks())
    draws = mean_between(g["items_min"], g["items_max"]) + locked + contents
    gold = draws * weighted_mean(pool(items, depth), "gold")
    return monsters, draws, gold


def fractional_level(g, xp):
    table = g["level_xp"]
    for i in range(len(table) - 1, -1, -1):
        if xp >= table[i]:
            if i == len(table) - 1:
                return float(g["level_min"] + i)
            return g["level_min"] + i + (xp - table[i]) / (table[i + 1] - table[i])
    return float(g["level_min"])


def expected(g, creatures, items, features, clear, gold_xp):
    rows = []
    cum = 0.0
    for depth in range(1, g["bottom"] + 1):
        arrive = fractional_level(g, cum)
        monsters, draws, gold = per_level(g, items, features, depth)
        mpool = pool(creatures, depth)
        mxp = monsters * weighted_mean(mpool, "xp") * clear
        gxp = gold * clear if gold_xp else 0.0
        rows.append({"depth": depth, "arrive": arrive, "target": TARGETS["arrive_level"](depth),
                     "monsters": monsters, "kinds": len(mpool), "avg_xp": weighted_mean(mpool, "xp"),
                     "mxp": mxp, "draws": draws, "gold": gold, "gxp": gxp, "cum_before": cum,
                     "hp": weighted_mean(mpool, "hp"), "dmg": weighted_mean(mpool, "dmg"),
                     "ac": weighted_mean(mpool, "ac")})
        cum += mxp + gxp
    return rows, cum


def simulate(g, creatures, items, features, clear, gold_xp, runs, seed):
    """The spread: each game rolls the counts and picks kinds by rarity, as level_gen.c does."""
    rng = random.Random(seed)
    at = {d: [] for d in range(1, g["bottom"] + 1)}
    for _ in range(runs):
        xp = 0
        for depth in range(1, g["bottom"] + 1):
            at[depth].append(fractional_level(g, xp))
            mpool = pool(creatures, depth)
            for _ in range(rng.randint(g["monsters_min"], g["monsters_max"])):
                if mpool and rng.random() < clear:
                    xp += rng.choices(mpool, weights=[k["rarity"] for k in mpool])[0]["xp"]
            if gold_xp:
                _, draws, _ = per_level(g, items, features, depth)
                ipool = pool(items, depth)
                for _ in range(round(draws)):
                    if rng.random() < clear:
                        xp += rng.choices(ipool, weights=[k["rarity"] for k in ipool])[0]["gold"]
    return at


def percentile(values, p):
    s = sorted(values)
    return s[min(len(s) - 1, int(p / 100 * len(s)))]


# ---- The report ------------------------------------------------------------

def status(delta):
    a = abs(delta)
    return "ok" if a <= TARGETS["ok"] else ("WARN" if a <= TARGETS["warn"] else "FAIL")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--clear", type=float, default=TARGETS["clear"], help="share of each depth cleared")
    ap.add_argument("--gold", action="store_true", default=TARGETS["gold_xp"], help="count XP for gold")
    ap.add_argument("--sim", type=int, default=0, metavar="N", help="simulate N games for the spread")
    ap.add_argument("--seed", type=int, default=1)
    args = ap.parse_args()

    try:
        g, creatures, items, features = load()
    except ParseError as e:
        print(f"balance: cannot read the game's tables: {e}", file=sys.stderr)
        return 2

    rows, total = expected(g, creatures, items, features, args.clear, args.gold)
    failures = 0
    warnings = 0

    print(f"Inputs: depths 1-{g['bottom']}, levels {g['level_min']}-{g['level_max']}, "
          f"monsters {g['monsters_min']}-{g['monsters_max']}, items {g['items_min']}-{g['items_max']}, "
          f"rooms {g['rooms_min']}-{g['rooms_max']}, {len(creatures)} random creature kinds, "
          f"{len(items)} item kinds")
    print(f"Player: clears {args.clear:.0%} of each depth, XP for gold {'on' if args.gold else 'off'}")
    print(f"Level XP: {', '.join(str(x) for x in g['level_xp'])}\n")

    print("Pacing (C1): level on arrival at each depth")
    print(f"{'depth':>5} {'monsters':>8} {'kinds':>5} {'avg XP':>7} {'mon XP':>7} {'gold':>6} "
          f"{'gold XP':>7} {'XP before':>9} {'level':>6} {'target':>6}  status")
    for r in rows:
        s = status(r["arrive"] - r["target"])
        failures += s == "FAIL"
        warnings += s == "WARN"
        print(f"{r['depth']:>5} {r['monsters']:>8.1f} {r['kinds']:>5} {r['avg_xp']:>7.0f} {r['mxp']:>7.0f} "
              f"{r['gold']:>6.0f} {r['gxp']:>7.0f} {r['cum_before']:>9.0f} {r['arrive']:>6.2f} "
              f"{r['target']:>6}  {s}")
    final = fractional_level(g, total)
    print(f"{'end':>5} {'':>8} {'':>5} {'':>7} {'':>7} {'':>6} {'':>7} {total:>9.0f} {final:>6.2f}")

    cap = g["level_xp"][-1]
    wasted = max(0.0, total - cap) / total if total else 0.0
    reached = next((r["depth"] for r in rows if r["cum_before"] + r["mxp"] + r["gxp"] >= cap), None)
    print(f"\nCap (C2): level {g['level_max']} reached while clearing depth {reached}; "
          f"{wasted:.0%} of the dungeon's XP comes after it  (report)")

    thin = [r["depth"] for r in rows if r["kinds"] < TARGETS["kinds_min"]]
    s = "ok" if not thin else "WARN"
    warnings += s == "WARN"
    print(f"Variety (C3): fewer than {TARGETS['kinds_min']} monster kinds at depths "
          f"{thin if thin else 'none'}  {s}")
    empty = [d for d in range(1, g["bottom"] + 1) if not pool(items, d)]
    if empty:
        failures += 1
    print(f"Coverage (C4): depths with no random items {empty if empty else 'none'}  "
          f"{'FAIL' if empty else 'ok'}")

    lo, hi = TARGETS["hits"]
    print(f"\nPower (C5): the average monster at each depth, against the player at the target level; "
          f"hits to kill the player {lo}-{hi}")
    print(f"{'depth':>5} {'mon HP':>6} {'mon dmg':>7} {'mon AC':>6} {'player HP':>9} {'hits to kill player':>19}  status")
    con_mod = 0   # an average Constitution
    for r in rows:
        lvl = r["target"]
        php = g["hit_die"] + (lvl - 1) * max(1, (g["hit_die"] + 1) / 2 + con_mod)
        hits = php / r["dmg"] if r["dmg"] else float("inf")
        s = "ok" if lo <= hits <= hi else "WARN"
        warnings += s == "WARN"
        print(f"{r['depth']:>5} {r['hp']:>6.1f} {r['dmg']:>7.1f} {r['ac']:>6.1f} {php:>9.1f} {hits:>19.1f}  {s}")

    if args.sim:
        at = simulate(g, creatures, items, features, args.clear, args.gold, args.sim, args.seed)
        print(f"\nSpread (C6): level on arrival over {args.sim} simulated games (10th / 50th / 90th percentile)")
        for d, vals in at.items():
            print(f"{d:>5}  {percentile(vals, 10):5.2f} {percentile(vals, 50):5.2f} {percentile(vals, 90):5.2f}")

    print(f"\n{failures} failed, {warnings} warnings")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
