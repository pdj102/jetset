/*
 * The player character screen: banked in page 104, on its own, as the
 * other screens fill page 116. It passes its literals only to common code
 * (the help line, the output path, the panels); what other pages fill in
 * (an effect's name) goes to buffers on the stack.
 */
#ifdef __SDCC
#pragma codeseg PAGE_104
#pragma constseg PAGE_104
#endif

#include "game/ui/screen_pc.h"

#include <string.h>

#include "core/nge_util.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_timer.h"
#include "io/nge_print.h"
#include "io/nge_text.h"
#include "ui/nge_panel.h"

#include "game/actors.h"
#include "game/components/attack.h"
#include "game/components/health.h"
#include "game/components/mana.h"
#include "game/components/pc.h"
#include "game/components/stats.h"
#include "game/effects/effects.h"
#include "game/level/level.h"
#include "game/ui/screen_internal.h"

/* The screen's panels, by PANEL_* */
#define PANEL_TITLE      0
#define PANEL_CHARACTER  1   /* level, XP, gold, depth */
#define PANEL_ABILITIES  2
#define PANEL_BODY       3   /* attributes, defences, statuses, effects, the key */
#define PANEL_COUNT      4

static const nge_panel_t panels[PANEL_COUNT] = {
    [PANEL_TITLE]     = { { .x1 = 0, .x2 = 39, .y1 = 0, .y2 = 1 }, SCREEN_PAPER, NGE_PANEL_CLIP },
    [PANEL_CHARACTER] = { { .x1 = 0, .x2 = 19, .y1 = 2, .y2 = 8 }, SCREEN_PAPER, NGE_PANEL_CLIP },
    [PANEL_ABILITIES] = { { .x1 = 20, .x2 = 39, .y1 = 2, .y2 = 8 }, SCREEN_PAPER, NGE_PANEL_CLIP },
    [PANEL_BODY]      = { { .x1 = 0, .x2 = 39, .y1 = 9, .y2 = 30 }, SCREEN_PAPER, NGE_PANEL_CLIP },
};

/* The character panel: labels in column 0, numbers right-aligned in 6 columns from COL_CHAR_VALUE */
#define COL_CHAR_VALUE  12

/* The abilities panel: the header on row 0, an ability a row; columns of the score, modifier and base */
#define COL_ABIL_NOW   5    /* width 4 */
#define COL_ABIL_MOD   11   /* width 3: " +3" */
#define COL_ABIL_BASE  15   /* width 4 */

/* The body panel's rows (the screen's rows less 9) */
#define ROW_ATTR_HEADER  1
#define ROW_HP           2
#define ROW_MP           3
#define ROW_AC           4
#define ROW_SPEED        5
#define ROW_ATTACK       6
#define ROW_DMG_HEADER   8
#define ROW_IMMUNE       9    /* then resist, then vulnerable */
#define ROW_STATUS       13
#define ROW_EFF_HEADER   15
#define ROW_EFFECTS      16   /* the first of EFFECT_ROWS */
#define ROW_KEY          21
#define EFFECT_ROWS      5

/* The body's attribute columns: now (pools), value and base, right-aligned, then speed's word */
#define COL_ATTR_NOW    9    /* width 3 */
#define COL_ATTR_VALUE  14   /* width 5 */
#define COL_ATTR_BASE   21   /* width 4 */
#define COL_ATTR_WORD   27

/* The defences grid: a column of 3 per damage type, from COL_DMG */
#define COL_DMG  8

/* The effects' columns: the name, the turns left (right-aligned, width 6), the saving throw */
#define COL_EFF_TURNS  16
#define COL_EFF_SAVE   24

#define SPEED_NORMAL   10       /* ticks between turns at normal speed (game/components/stats.h) */
#define TURNS_ALWAYS   0xFFFF   /* an effect that lasts for ever */
#define TITLE_MAX      (PC_NAME_MAX + 13)   /* "<name> the Fighter", and its NUL */

static const char *const class_names[PC_CLASS_COUNT] = { "Fighter" };

/* The damage types' two-letter names (the grid) and words (the attack), by DMG_* */
static const char dmg_short[DMG_COUNT][3] = { "Bl", "Sl", "Pi", "Fi", "Co", "Po", "Ac", "Ma" };
static const char *const dmg_words[DMG_COUNT] = {
    "blunt", "slash", "pierce", "fire", "cold", "poison", "acid", "magic",
};

/* The abilities' names, by ABIL_* */
static const char ability_names[ABIL_COUNT][4] = { "Str", "Dex", "Con", "Int", "Wis", "Cha" };

/* The statuses' names, by bit (STATUS_*) */
static const char *const status_names[3] = { "Poisoned", "Blinded", "Unconscious" };

/* ---- Numbers ---- */

uint8_t screen_pc_ink(uint8_t value, uint8_t base, uint8_t lower_is_better)
{
    if (value == base) {
        return NGE_INK_WHITE;
    }
    return ((value > base) != (lower_is_better != 0)) ? NGE_INK_CYAN : NGE_INK_RED;
}

/* A signed number, its sign always shown: "+2", "+0", "-1" */
static void put_signed(int8_t v, NGE_FILE *s)
{
    nge_putc((v < 0) ? '-' : '+', s);
    nge_putu16((uint16_t)((v < 0) ? -v : v), s);
}

/* An ability's modifier (-5 to +10), signed and right-aligned in 3 columns */
static void put_mod(int8_t m, NGE_FILE *s)
{
    if (m > -10 && m < 10) {
        nge_putc(' ', s);
    }
    put_signed(m, s);
}

/* At column x of row y: n right-aligned in width columns, in ink */
static void put_at(uint8_t y, uint8_t x, uint16_t n, uint8_t width, uint8_t ink, NGE_FILE *s)
{
    nge_text_at(y, x, s);
    nge_text_ink(ink, s);
    nge_putu16w(n, width, ' ', s);
}

/* At column x of row y: text, in ink */
static void puts_at(uint8_t y, uint8_t x, const char *text, uint8_t ink, NGE_FILE *s)
{
    nge_text_at(y, x, s);
    nge_text_ink(ink, s);
    nge_puts(text, s);
}

/* ---- The title, the character and the abilities ---- */

static void draw_title(NGE_FILE *s)
{
    char title[TITLE_MAX];

    nge_util_assert(pc_class < PC_CLASS_COUNT);

    strcpy(title, pc_name);   /* slot 0: PC_NAME_MAX characters at most */
    strcat(title, " the ");
    strcat(title, class_names[pc_class]);
    nge_text_ink(NGE_INK_YELLOW, s);
    nge_panel_centre(&panels[PANEL_TITLE], 0, title, s);
}

static void draw_character(NGE_FILE *s)
{
    puts_at(0, 0, "Level", NGE_INK_WHITE, s);
    put_at(0, COL_CHAR_VALUE, pc_level, 6, NGE_INK_WHITE, s);
    puts_at(1, 0, "XP", NGE_INK_WHITE, s);
    put_at(1, COL_CHAR_VALUE, pc_xp, 6, NGE_INK_WHITE, s);
    puts_at(2, 0, "Next", NGE_INK_WHITE, s);
    if (pc_level < PC_LEVEL_MAX) {
        put_at(2, COL_CHAR_VALUE, pc_xp_needed((uint8_t)(pc_level + 1)), 6, NGE_INK_WHITE, s);
    } else {
        puts_at(2, COL_CHAR_VALUE, "   max", NGE_INK_WHITE, s);
    }
    puts_at(3, 0, "Gold", NGE_INK_WHITE, s);
    put_at(3, COL_CHAR_VALUE, pc_gold, 6, NGE_INK_WHITE, s);
    puts_at(4, 0, "Depth", NGE_INK_WHITE, s);
    put_at(4, COL_CHAR_VALUE, level_depth, 6, NGE_INK_WHITE, s);
    puts_at(5, 0, "Deepest", NGE_INK_WHITE, s);
    put_at(5, COL_CHAR_VALUE, pc_depth_max, 6, NGE_INK_WHITE, s);
}

static void draw_abilities(NGE_FILE *s)
{
    nge_entity_id_t id = actor_player;
    const char *name = ability_names[0];   /* walked by pointer: an index splits the banked address (integer range) */
    uint8_t value;
    uint8_t ink;
    uint8_t a;

    puts_at(0, 0, "      Now  Mod Base", NGE_INK_YELLOW, s);
    for (a = 0; a < ABIL_COUNT; a++, name += sizeof(ability_names[0])) {
        value = stats_ability[a][id];
        ink = screen_pc_ink(value, stats_ability_base[a][id], 0);
        puts_at((uint8_t)(1 + a), 1, name, NGE_INK_WHITE, s);
        put_at((uint8_t)(1 + a), COL_ABIL_NOW, value, 4, ink, s);
        nge_text_at((uint8_t)(1 + a), COL_ABIL_MOD, s);
        put_mod(STATS_MOD(value), s);
        put_at((uint8_t)(1 + a), COL_ABIL_BASE, stats_ability_base[a][id], 4, NGE_INK_WHITE, s);
    }
}

/* ---- The body ---- */

/* An attribute's row: its value, in its ink against its base, and its base */
static void draw_attr(uint8_t y, const char *label, uint8_t value, uint8_t base, uint8_t lower_is_better,
                      NGE_FILE *s)
{
    puts_at(y, 0, label, NGE_INK_WHITE, s);
    put_at(y, COL_ATTR_VALUE, value, 5, screen_pc_ink(value, base, lower_is_better), s);
    put_at(y, COL_ATTR_BASE, base, 4, NGE_INK_WHITE, s);
}

static void draw_attributes(NGE_FILE *s)
{
    nge_entity_id_t id = actor_player;
    uint8_t speed = stats_speed[id];

    puts_at(ROW_ATTR_HEADER, 0, "         Now  Value  Base", NGE_INK_YELLOW, s);
    draw_attr(ROW_HP, "HP", health_hp_max[id], health_hp_max_base[id], 0, s);
    put_at(ROW_HP, COL_ATTR_NOW, health_hp[id], 3, NGE_INK_WHITE, s);
    if (MANA_IS(id)) {
        /* Maximum MP isn't an attribute yet (docs/mana.md, D2): its base is its maximum */
        draw_attr(ROW_MP, "MP", mana_mp_max[id], mana_mp_max[id], 0, s);
        put_at(ROW_MP, COL_ATTR_NOW, mana_mp[id], 3, NGE_INK_WHITE, s);
    }
    draw_attr(ROW_AC, "AC", health_ac[id], health_ac_base[id], 0, s);
    draw_attr(ROW_SPEED, "Speed", speed, stats_speed_base[id], 1, s);
    puts_at(ROW_SPEED, COL_ATTR_WORD, (speed < SPEED_NORMAL) ? "fast" : (speed == SPEED_NORMAL) ? "normal" : "slow",
            NGE_INK_WHITE, s);
}

/* The melee attack the player makes now: their wielded weapon's, or their own */
static void draw_attack(NGE_FILE *s)
{
    nge_entity_id_t attacker = attack_weapon(actor_player);
    int8_t bonus;

    if (attacker == NGE_ENTITY_INVALID) {
        attacker = actor_player;
    }
    puts_at(ROW_ATTACK, 0, "Attack  ", NGE_INK_WHITE, s);
    if (!ATTACK_HAS(attacker, ATTACK_MELEE)) {
        nge_puts("none", s);
        return;
    }
    put_signed(attack_to_hit[ATTACK_MELEE][attacker], s);
    nge_puts(" to hit  ", s);
    nge_putu16(attack_dice[ATTACK_MELEE][attacker], s);
    nge_putc('d', s);
    nge_putu16(attack_sides[ATTACK_MELEE][attacker], s);
    bonus = attack_bonus[ATTACK_MELEE][attacker];
    if (bonus != 0) {
        put_signed(bonus, s);
    }
    nge_putc(' ', s);
    nge_puts(dmg_words[attack_type[ATTACK_MELEE][attacker]], s);
}

/* One row of the defences grid: a * where the value has the type, cyan or red (worse) where only effects give it */
static void draw_defence(uint8_t y, const char *label, uint8_t value, uint8_t base, uint8_t worse, NGE_FILE *s)
{
    uint8_t t;
    uint8_t bit;

    puts_at(y, 0, label, NGE_INK_WHITE, s);
    for (t = 0; t < DMG_COUNT; t++) {
        bit = DMG_BIT(t);
        if (value & bit) {
            puts_at(y, (uint8_t)(COL_DMG + 3 * t), "*",
                    (base & bit) ? NGE_INK_WHITE : (worse ? NGE_INK_RED : NGE_INK_CYAN), s);
        }
    }
}

static void draw_defences(NGE_FILE *s)
{
    nge_entity_id_t id = actor_player;
    uint8_t t;

    puts_at(ROW_DMG_HEADER, 0, "Damage", NGE_INK_YELLOW, s);
    for (t = 0; t < DMG_COUNT; t++) {
        nge_text_at(ROW_DMG_HEADER, (uint8_t)(COL_DMG + 3 * t), s);
        nge_puts(dmg_short[t], s);
    }
    draw_defence(ROW_IMMUNE, "Immune", health_immune[id], health_immune_base[id], 0, s);
    draw_defence(ROW_IMMUNE + 1, "Resist", health_resist[id], health_resist_base[id], 0, s);
    draw_defence(ROW_IMMUNE + 2, "Vuln.", health_vuln[id], health_vuln_base[id], 1, s);
}

/* The statuses by name, conditions in red; "None" */
static void draw_statuses(NGE_FILE *s)
{
    uint8_t status = stats_status[actor_player];
    uint8_t bit;
    uint8_t i;

    puts_at(ROW_STATUS, 0, "Status ", NGE_INK_YELLOW, s);
    if (status == 0) {
        puts_at(ROW_STATUS, 8, "None", NGE_INK_WHITE, s);
        return;
    }
    nge_text_at(ROW_STATUS, 8, s);
    for (i = 0; i < sizeof(status_names) / sizeof(status_names[0]); i++) {
        bit = (uint8_t)(1 << i);
        if (status & bit) {
            nge_text_ink((bit & STATUS_CONDITIONS) ? NGE_INK_RED : NGE_INK_WHITE, s);
            nge_puts(status_names[i], s);
            nge_putc(' ', s);
        }
    }
}

/* An active effect's turns left, by its timer: TURNS_ALWAYS if it has no end */
static uint16_t turns_left(nge_entity_id_t effect)
{
    uint8_t cycles = nge_timer_cycles[effect];
    uint16_t ticks;

    if (cycles == 0) {
        return TURNS_ALWAYS;
    }
    ticks = (uint16_t)((uint16_t)(cycles - 1) * nge_timer_period[effect] + nge_timer_count[effect]);
    return (uint16_t)((ticks + NGE_TIMER_TICKS_PER_TURN - 1) / NGE_TIMER_TICKS_PER_TURN);
}

/* Effect ef's row: its name (and how many), the turns left, its saving throw */
static void draw_effect(uint8_t y, uint8_t ef, uint8_t count, uint16_t turns, NGE_FILE *s)
{
    char name[EFFECT_NAME_MAX + 1];
    uint8_t ability;
    uint8_t dc;

    effect_text(ef, EFFECT_TEXT_NAME, name, sizeof(name));
    puts_at(y, 0, name, NGE_INK_WHITE, s);
    if (count > 1) {
        nge_puts(" x", s);
        nge_putu16(count, s);
    }
    if (turns == TURNS_ALWAYS) {
        puts_at(y, COL_EFF_TURNS, "always", NGE_INK_WHITE, s);
    } else {
        put_at(y, COL_EFF_TURNS, turns, 6, NGE_INK_WHITE, s);
    }
    dc = effect_save_of(ef, &ability);
    if (dc != 0) {
        puts_at(y, COL_EFF_SAVE, ability_names[ability], NGE_INK_WHITE, s);
        nge_putc(' ', s);
        nge_putu16(dc, s);
    }
}

/* The active effects, one row per kind (instances counted), in EF_* order; "and n more" past the rows */
static void draw_effects(NGE_FILE *s)
{
    uint8_t count[EF_TIMED_COUNT];    /* only timed effects are active: EF_COUNT would be too much for the stack */
    uint16_t turns[EF_TIMED_COUNT];   /* the longest left, by kind */
    nge_entity_id_t id;
    uint16_t t;
    uint8_t kinds = 0;
    uint8_t ef;
    uint8_t i;
    uint8_t y = ROW_EFFECTS;

    memset(count, 0, sizeof(count));
    NGE_OWNER_FOR_EACH_OWNED(i, id, actor_player) {
        if (NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_EFFECT)) {
            ef = effect_kind_of(id);
            nge_util_assert(ef < EF_TIMED_COUNT);
            t = turns_left(id);
            if (count[ef]++ == 0) {
                kinds++;
                turns[ef] = t;
            } else if (t > turns[ef]) {
                turns[ef] = t;
            }
        }
    }

    puts_at(ROW_EFF_HEADER, 0, "Effect           Turns  Save", NGE_INK_YELLOW, s);
    if (kinds == 0) {
        puts_at(y, 0, "None", NGE_INK_WHITE, s);
        return;
    }
    for (ef = 0; ef < EF_TIMED_COUNT; ef++) {
        if (count[ef] == 0) {
            continue;
        }
        if (y == ROW_EFFECTS + EFFECT_ROWS - 1 && kinds > 1) {
            puts_at(y, 0, "and ", NGE_INK_WHITE, s);   /* the last row, and more than one left */
            nge_putu16(kinds, s);
            nge_puts(" more", s);
            return;
        }
        draw_effect(y++, ef, count[ef], turns[ef], s);
        kinds--;
    }
}

static void draw_key(NGE_FILE *s)
{
    puts_at(ROW_KEY, 0, "Cyan: better than base", NGE_INK_CYAN, s);
    puts_at(ROW_KEY, 24, "Red: worse", NGE_INK_RED, s);
}

static void draw(NGE_FILE **streams)
{
    NGE_FILE *body = streams[PANEL_BODY];

    draw_title(streams[PANEL_TITLE]);
    draw_character(streams[PANEL_CHARACTER]);
    draw_abilities(streams[PANEL_ABILITIES]);
    draw_attributes(body);
    draw_attack(body);
    draw_defences(body);
    draw_statuses(body);
    draw_effects(body);
    draw_key(body);
    screen_help("SPACE:back");
}

void screen_pc_run(void) NGE_BANKED
{
    NGE_FILE *streams[PANEL_COUNT];
    uint8_t ok;

    nge_util_assert(PC_IS(actor_player));

    screen_open();
    ok = nge_panel_open_all(SCREEN_BUFFER_OVERLAY, panels, PANEL_COUNT, streams);
    nge_util_assert(ok);
    draw(streams);
    while (nge_key_ascii(screen_wait_key()) != ' ') {
    }
    nge_panel_close_all(streams, PANEL_COUNT);
    screen_close();
}
