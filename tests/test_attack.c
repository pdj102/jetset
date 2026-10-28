/* game/components: the attack component, the hit rule and damage. */
#include "test.h"

#include <stdlib.h>

#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"

#include "game/components/attack.h"
#include "game/components/health.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/gen/gen.h"

#define AREA  NGE_OWNER_AREA(0)

static nge_entity_id_t with_attack(int8_t to_hit, uint8_t dice, uint8_t sides, int8_t bonus)
{
    attack_kind_t b;
    nge_entity_id_t id = nge_entity_create();

    b.to_hit = to_hit;
    b.dice = dice;
    b.sides = sides;
    b.bonus = bonus;
    b.type = DMG_PIERCE;
    b.range = 1;
    attack_add(id, ATTACK_MELEE, &b);
    return id;
}

int main(void)
{
    health_kind_t h = { 5, 0, 0, 0, 0 };   /* 5 hp; the AC is set below */
    attack_kind_t b;
    nge_entity_id_t id;
    nge_entity_id_t target;
    nge_entity_id_t holder;
    nge_entity_id_t item;
    int to_hit;
    int ac;
    int roll;
    int lo;
    int hi;
    int i;
    int ok;
    uint8_t d;

    test_init();
    nge_entity_init();

    /* ---- add and remove ---- */
    id = with_attack(-2, 3, 6, 4);
    CHECK(ATTACK_HAS(id, ATTACK_MELEE) && attack_to_hit[ATTACK_MELEE][id] == -2 &&
          attack_dice[ATTACK_MELEE][id] == 3 && attack_sides[ATTACK_MELEE][id] == 6 &&
          attack_bonus[ATTACK_MELEE][id] == 4 && attack_type[ATTACK_MELEE][id] == DMG_PIERCE &&
          attack_range[ATTACK_MELEE][id] == 1);
    CHECK(!NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_RANGED));
    attack_remove(id, ATTACK_MELEE);
    CHECK(!ATTACK_HAS(id, ATTACK_MELEE));
    b.dice = 1;
    CHECK(ATTACK_KIND_HAS(&b));
    b.dice = 0;
    CHECK(!ATTACK_KIND_HAS(&b));

    /* ---- the hit rule: 20 hits, 1 misses, else roll + to hit >= AC ---- */
    target = nge_entity_create();
    health_add(target, &h);
    ok = 1;
    for (to_hit = -5; to_hit <= 5 && ok; to_hit++) {
        id = with_attack((int8_t)to_hit, 1, 4, 0);
        for (ac = 0; ac <= 30 && ok; ac++) {
            health_ac[target] = (uint8_t)ac;   /* as effects_recompute() would set it */
            for (roll = 1; roll <= 20 && ok; roll++) {
                int want = roll == 20 || (roll != 1 && roll + to_hit >= ac);

                ok = attack_hits(id, ATTACK_MELEE, target, (uint8_t)roll) == want;
            }
        }
        nge_entity_destroy(id);
        nge_entity_release(nge_entity_next_dying());
    }
    CHECK(ok);

    /* ---- damage: dice + bonus, the whole range, at least 1, at most 255 ---- */
    srand(4);
    id = with_attack(0, 3, 6, 2);                             /* 3d6 + 2: 5 to 20 */
    lo = 255;
    hi = 0;
    for (i = 0; i < 2000; i++) {
        d = attack_damage(id, ATTACK_MELEE);
        lo = d < lo ? d : lo;
        hi = d > hi ? d : hi;
    }
    CHECK(lo == 5 && hi == 20);
    id = with_attack(0, 2, 6, -20);                           /* always below 1 */
    ok = 1;
    for (i = 0; i < 100 && ok; i++) {
        ok = attack_damage(id, ATTACK_MELEE) == 1;
    }
    CHECK(ok);
    id = with_attack(0, 200, 2, 100);                         /* always over 255 */
    ok = 1;
    for (i = 0; i < 20 && ok; i++) {
        ok = attack_damage(id, ATTACK_MELEE) == 255;
    }
    CHECK(ok);

    /* ---- the weapon used: a wielded melee weapon, or none ---- */
    holder = gen_creature(CR_PLAYER, AREA);
    CHECK(attack_weapon(holder) == NGE_ENTITY_INVALID);         /* nothing wielded */
    item = gen_item(IT_SWORD, AREA);
    inv_stow(item, holder);
    CHECK(attack_weapon(holder) == NGE_ENTITY_INVALID);         /* carried, not wielded */
    item_slot[item] = SLOT_WEAPON;                              /* as the equip action would */
    CHECK(attack_weapon(holder) == item);
    attack_remove(item, ATTACK_MELEE);
    CHECK(attack_weapon(holder) == NGE_ENTITY_INVALID);         /* wielded, but not a melee weapon */
    CHECK(attack_weapon(target) == NGE_ENTITY_INVALID);         /* no inventory */

    /* ---- asserts ---- */
    b.to_hit = 0;
    b.dice = 1;
    b.sides = 4;
    b.bonus = 0;
    b.type = DMG_BLUNT;
    b.range = 2;                                              /* melee reaches 1 */
    TEST_EXPECT_HALT(attack_add(id, ATTACK_MELEE, &b));
    CHECK(test_halt_count == 1);
    b.range = 1;
    b.dice = 0;                                               /* no dice */
    TEST_EXPECT_HALT(attack_add(id, ATTACK_MELEE, &b));
    CHECK(test_halt_count == 1);
    b.dice = 1;
    TEST_EXPECT_HALT(attack_add(id, ATTACK_USE_COUNT, &b));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(attack_hits(id, ATTACK_MELEE, target, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(attack_weapon(NGE_ENTITY_MAX));
    CHECK(test_halt_count == 1);

    return test_summary("attack");
}
