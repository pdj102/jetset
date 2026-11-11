/*
 * The anim module's visual effects (docs/vfx.md), banked in page 126 with
 * the rest of the module: shots and bolts flying, glows, splashes, hit
 * flashes and the stand-ins of creatures killed, started from events and
 * played by the engine's VFX module (vfx/nge_vfx.h). Its tables are
 * constants in this page.
 */
#ifdef __SDCC
#pragma codeseg PAGE_126
#pragma constseg PAGE_126
#endif

#include "game/anim/anim_internal.h"

#include "data/sprite_data.h"
#include "ecs/components/nge_location.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render.h"
#include "sprite/nge_sprite.h"
#include "vfx/nge_vfx.h"

#include "game/actors.h"
#include "game/components/health.h"
#include "game/components/kind.h"
#include "game/effects/effects.h"
#include "game/events.h"
#include "game/gen/gen_kinds.h"
#include "game/level/level.h"
#include "game/spells/spells.h"

/* An effect's look (anim_ef_fx[]): its bank in the low nibble, and whether it glows */
#define FX_GLOWS     0x10
#define FX_BANK(fx)  ((uint8_t)((fx) & 0x0F))
#define FX_NONE      0      /* no bolt and no glow (bank 0 is the player's, never an effect's) */

/* The splash's colour, by damage type (docs/vfx.md D6) */
static const uint8_t anim_dmg_bank[DMG_COUNT] = {
    [DMG_BLUNT]  = BANK_RED,
    [DMG_SLASH]  = BANK_RED,
    [DMG_PIERCE] = BANK_RED,
    [DMG_FIRE]   = BANK_ORANGE,
    [DMG_COLD]   = BANK_ICE,
    [DMG_POISON] = BANK_LIME,
    [DMG_ACID]   = BANK_LIME,
    [DMG_MAGIC]  = BANK_PURPLE,
};

/*
 * Each effect's colour, for the bolt that carries it and the glow it shows
 * (D6, D8). Innate and worn effects show nothing. Damaging bolts don't
 * glow: their splash shows (R3)
 */
#define HEAL   (BANK_GREEN | FX_GLOWS)
#define BUFF   (BANK_GOLD | FX_GLOWS)
#define CURSE  (BANK_DARK | FX_GLOWS)

static const uint8_t anim_ef_fx[EF_COUNT] = {
    [EF_POISON]      = BANK_LIME | FX_GLOWS,
    [EF_RENEW]       = HEAL,
    [EF_HASTE]       = BUFF,
    [EF_SLOW]        = CURSE,
    [EF_MIGHT]       = BUFF,
    [EF_WEAKNESS]    = CURSE,
    [EF_STONE_SKIN]  = BUFF,
    [EF_RESIST_FIRE] = BUFF,
    [EF_RESIST_COLD] = BUFF,
    [EF_BLIND]       = CURSE,
    [EF_SLEEP]       = CURSE,
    [EF_HOLD]        = CURSE,
    [EF_DESCENT]     = BANK_PURPLE | FX_GLOWS,
    [EF_RECALL]      = BANK_PURPLE | FX_GLOWS,
    [EF_GRACE]       = BUFF,
    [EF_ENDURANCE]   = BUFF,
    [EF_INSIGHT]     = BUFF,
    [EF_INTUITION]   = BUFF,
    [EF_RESIST_POISON] = BUFF,
    [EF_RESIST_ACID] = BUFF,
    [EF_RESIST_MAGIC] = BUFF,
    [EF_AID]         = HEAL,
    [EF_SHIELD]      = BUFF,
    [EF_HEX]         = CURSE,
    [EF_DRAIN_STR]   = CURSE,
    [EF_DRAIN_DEX]   = CURSE,
    [EF_DRAIN_CON]   = CURSE,
    [EF_DRAIN_INT]   = CURSE,
    [EF_DRAIN_WIS]   = CURSE,
    [EF_DRAIN_CHA]   = CURSE,
    [EF_BURNING]     = BANK_ORANGE | FX_GLOWS,
    [EF_BLEEDING]    = BANK_RED | FX_GLOWS,
    [EF_CORROSION]   = BANK_LIME | FX_GLOWS,
    [EF_BLESS]       = BUFF,
    [EF_BANE]        = CURSE,
    [EF_HEROISM]     = BUFF,
    [EF_HEALING]     = HEAL,
    [EF_CURE]        = HEAL,
    [EF_CURE_ALL]    = HEAL,
    [EF_CURE_BLIND]  = HEAL,
    [EF_MANA]        = BANK_BLUE | FX_GLOWS,
    [EF_VITALITY]    = BUFF,
    [EF_RAISE_STR]    = BUFF,
    [EF_RAISE_DEX]   = BUFF,
    [EF_RAISE_CON]   = BUFF,
    [EF_RAISE_INT]   = BUFF,
    [EF_RAISE_WIS]   = BUFF,
    [EF_RAISE_CHA]   = BUFF,
    [EF_BOLT_MAGIC]  = BANK_PURPLE,
    [EF_BOLT_FIRE]   = BANK_ORANGE,
    [EF_BOLT_COLD]   = BANK_ICE,
    [EF_BOLT_ACID]   = BANK_LIME,
    [EF_LIGHT]       = BUFF,
    [EF_IDENTIFY]    = BANK_BLUE | FX_GLOWS,
    [EF_MAP]         = BANK_BLUE | FX_GLOWS,
    [EF_TELEPORT]    = BANK_PURPLE | FX_GLOWS,
    [EF_UNCURSE]     = BUFF,
    [EF_ENCHANT]     = BUFF,
    [EF_SUMMON]      = BANK_PURPLE | FX_GLOWS,
    [EF_LEVEL_TELEPORT] = BANK_PURPLE | FX_GLOWS,
};

/* On the current level, with a place: where a VFX can show it. A dying entity counts (its stand-in) */
static uint8_t placed(nge_entity_id_t id)
{
    return id < NGE_ENTITY_MAX && NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE &&
           NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_LOCATION) && NGE_OWNER_OF(id) == LEVEL_AREA;
}

/* Placed, and drawn by the render system: one a copy can be made of */
static uint8_t drawn(nge_entity_id_t id)
{
    return placed(id) && NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_SPRITE);
}

/* A look of 4-bit patterns in a bank */
static void fx_look(nge_vfx_look_t *look, uint8_t first, uint8_t count, uint8_t rate, uint8_t bank)
{
    look->first = first;
    look->count = count;
    look->rate = rate;
    look->flags = NGE_SPRITE_PALETTE(bank);
    look->mode = NGE_SPRITE_PATTERN4;
}

/* A bolt from one entity's tile to another's; nothing if one isn't placed, or they are the same */
static void fly(nge_entity_id_t from, nge_entity_id_t to, const nge_vfx_look_t *look, uint8_t speed)
{
    if (from == to || !placed(from) || !placed(to)) {
        return;
    }
    nge_vfx_bolt(NGE_LOCATION_PX(from), NGE_LOCATION_PY(from), NGE_LOCATION_PX(to), NGE_LOCATION_PY(to), speed, look);
}

/* A magic bolt in an effect's colour, from the caster to the target, if the effect has a look */
static void fly_effect(nge_entity_id_t from, nge_entity_id_t to, uint8_t ef)
{
    nge_vfx_look_t look;
    uint8_t fx;

    if (ef >= EF_COUNT) {
        return;
    }
    fx = anim_ef_fx[ef];
    if (fx == FX_NONE) {
        return;
    }
    fx_look(&look, SPRITE_FX_ORB, SPRITE_FX_ORB_COUNT, ANIM_FX_RATE, FX_BANK(fx));
    fly(from, to, &look, ANIM_MAGIC_SPEED);
}

/* A shot from the shooter: an arrow from a bow or crossbow, a stone or blade otherwise (p1: the launcher or thrown item) */
static void fly_shot(nge_entity_id_t from, nge_entity_id_t to, nge_entity_id_t item)
{
    nge_vfx_look_t look;
    uint8_t group = GROUP_NONE;

    if (item < NGE_ENTITY_MAX && NGE_ENTITY_STATE(item) != NGE_ENTITY_FREE && KIND_IS(item, CLASS_ITEM)) {
        group = gen_item_group(kind_index[item]);
    }
    if (group == GROUP_ARROW || group == GROUP_BOLT) {
        fx_look(&look, SPRITE_FX_ARROW, 1, ANIM_FX_RATE, BANK_RED);   /* the fletching's colour */
        look.mode |= NGE_VFX_DIRECTED;
    } else {
        fx_look(&look, SPRITE_FX_THROWN, SPRITE_FX_THROWN_COUNT, ANIM_FX_RATE, BANK_IRON);
    }
    fly(from, to, &look, ANIM_SHOT_SPEED);
}

/* An entity's own pattern, shown at its place in the flash bank for ANIM_FLASH_FRAMES, after delay (D11, D9) */
static void flash(nge_entity_id_t id, const nge_render_look_t *shown, uint8_t delay)
{
    nge_vfx_look_t look;

    look.first = shown->pattern;
    look.count = 1;
    look.rate = ANIM_FLASH_FRAMES;
    look.flags = (uint8_t)((shown->flags & (uint8_t)~NGE_SPRITE_PALETTE(0x0F)) | NGE_SPRITE_PALETTE(BANK_FLASH));
    look.mode = shown->mode;
    nge_vfx_burst((uint16_t)(NGE_LOCATION_PX(id) + shown->ox), (uint16_t)(NGE_LOCATION_PY(id) + shown->oy), &look, delay);
}

/* Damage: the hit flash if it survived, and the splash in the damage type's colour (R4, R22) */
static void damaged(nge_entity_id_t id, uint8_t taken, uint8_t type)
{
    nge_render_look_t shown;
    nge_vfx_look_t look;

    if (taken == 0 || !placed(id)) {
        return;   /* immune: the message says so (R5) */
    }
    if (NGE_ENTITY_IS_ALIVE(id) && drawn(id)) {
        /* One killed is dying already: its stand-in flashes instead (D11) */
        nge_render_get(id, &shown);
        flash(id, &shown, ANIM_IMPACT_DELAY);
    }
    fx_look(&look, SPRITE_FX_SPLASH, SPRITE_FX_SPLASH_COUNT, ANIM_FX_RATE,
            type < DMG_COUNT ? anim_dmg_bank[type] : BANK_RED);
    nge_vfx_burst(NGE_LOCATION_PX(id), NGE_LOCATION_PY(id), &look, ANIM_IMPACT_DELAY);
}

/*
 * A creature killed: destroyed at once, so a stand-in shows it until the
 * blow lands, then it flashes, then a puff (D9). The player isn't destroyed
 * (persistent): the game ends instead
 */
static void died(nge_entity_id_t id)
{
    nge_render_look_t shown;
    nge_vfx_look_t look;
    uint16_t x;
    uint16_t y;
    uint8_t lands;

    if (NGE_ENTITY_IS_ALIVE(id) || !drawn(id)) {
        return;
    }
    nge_render_get(id, &shown);
    x = (uint16_t)(NGE_LOCATION_PX(id) + shown.ox);
    y = (uint16_t)(NGE_LOCATION_PY(id) + shown.oy);
    lands = nge_vfx_arrival(x, y);
    if (lands < ANIM_IMPACT_DELAY) {
        lands = ANIM_IMPACT_DELAY;
    }

    look.first = shown.pattern;   /* itself, until the blow lands */
    look.count = 1;
    look.rate = 1;
    look.flags = shown.flags;
    look.mode = (uint8_t)(shown.mode | NGE_VFX_HOLD);
    nge_vfx_burst(x, y, &look, lands);
    flash(id, &shown, lands);
    fx_look(&look, SPRITE_FX_PUFF, SPRITE_FX_PUFF_COUNT, ANIM_FX_RATE, BANK_BONE);
    nge_vfx_burst(NGE_LOCATION_PX(id), NGE_LOCATION_PY(id), &look, (uint8_t)(lands + ANIM_FLASH_FRAMES));
}

/* An effect taking hold: its glow, if it has one (R3); after a bolt to it, when the bolt lands (D4) */
static void glow(nge_entity_id_t id, uint8_t ef)
{
    nge_vfx_look_t look;
    uint8_t fx;

    if (ef >= EF_COUNT || !placed(id) || !NGE_ENTITY_IS_ALIVE(id)) {
        return;
    }
    fx = anim_ef_fx[ef];
    if (!(fx & FX_GLOWS)) {
        return;
    }
    fx_look(&look, SPRITE_FX_GLOW, SPRITE_FX_GLOW_COUNT, ANIM_FX_RATE, FX_BANK(fx));
    nge_vfx_burst(NGE_LOCATION_PX(id), NGE_LOCATION_PY(id), &look, 0);
}

void anim_vfx_event(nge_event_id_t ev, nge_entity_id_t target, nge_entity_id_t source, uint8_t p1, uint8_t p2)
{
    spell_def_t def;   /* a copy: the table is in the spells' page */

    switch (ev) {
    case EV_FIRED:
        fly_shot(source, target, p1);
        break;
    case EV_CAST:
        if (target != source && p1 < SPELL_COUNT) {
            spell_get(p1, &def);
            fly_effect(source, target, def.ef);
        }
        break;
    case EV_AIMED:
        if (target != source && p1 < NGE_ENTITY_MAX) {
            fly_effect(source, target, GEN_HOOK_EF(gen_hook_of(p1, GEN_HOOK_AIM)));
        }
        break;
    case EV_EFFECT_STARTED:
        glow(target, p1);
        break;
    case EV_DAMAGED:
        damaged(target, p1, EV_DMG_TYPE(p2));
        break;
    case EV_DIED:
        died(target);
        break;
    default:
        break;
    }
}
