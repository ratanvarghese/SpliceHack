/* NetHack 3.7	weapon.c	$NHDT-Date: 1607811730 2020/12/12 22:22:10 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.89 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *      This module contains code for calculation of "to hit" and damage
 *      bonuses for any given weapon used, as well as weapons selection
 *      code for monsters.
 */
#include "hack.h"

static void display_skill_db_info(int);
static int odd_material_damage(struct obj *);
static int weapon_distribution_bonus(int, boolean, int);
static void give_may_advance_msg(int);
static void finish_towel_change(struct obj *obj, int);
static boolean could_advance(int);
static boolean peaked_skill(int);
static int slots_required(int);
static void skill_advance(int);

/* Categories whose names don't come from OBJ_NAME(objects[type])
 */
#define PN_BARE_HANDED (-1) /* includes martial arts */
#define PN_TWO_WEAPONS (-2)
#define PN_RIDING (-3)
#define PN_POLEARMS (-4)
#define PN_SABER (-5)
#define PN_HAMMER (-6)
#define PN_FIREARMS (-7)
#define PN_WHIP (-8)
#define PN_ATTACK_SPELL (-9)
#define PN_HEALING_SPELL (-10)
#define PN_DIVINATION_SPELL (-11)
#define PN_ENCHANTMENT_SPELL (-12)
#define PN_CLERIC_SPELL (-13)
#define PN_ESCAPE_SPELL (-14)
#define PN_MATTER_SPELL (-15)

#define PN_POWER_ATTACK (-16)
#define PN_FLAMING_FISTS (-17)
#define PN_FREEZING_FISTS (-18)
#define PN_SHOCKING_FISTS (-19)
#define PN_STUNNING_FIST (-20)
#define PN_BACKSTAB (-21)
#define PN_CAREFUL_ATTACK (-22)
#define PN_RAT_TAMER (-23)
#define PN_DRAGON_TAMER (-24)
#define PN_ANIMAL_FRIENDSHIP (-25)
#define PN_BLOOD_RAGE (-26)
#define PN_PANACHE (-27)
#define PN_WILD_MAGIC (-28)
#define PN_BLOOD_MAGIC (-29)
#define PN_CODE_OF_HONOR (-30)
#define PN_DISARM (-31)
#define PN_SUNDER (-32)
#define PN_TUMBLING (-33)

#define PN_SPIDER_FRIEND (-34)

#define is_odd_material(obj, mat) \
    ((obj)->material == (mat) && !(objects[(obj)->otyp].oc_material == (mat)))

static NEARDATA const short skill_names_indices[P_NUM_SKILLS] = {
    0, DAGGER, KNIFE, AXE, PICK_AXE, SHORT_SWORD, BROADSWORD, LONG_SWORD,
    TWO_HANDED_SWORD, SCIMITAR, PN_SABER, CLUB, MACE, MORNING_STAR, FLAIL,
    PN_HAMMER, QUARTERSTAFF, PN_POLEARMS, SPEAR, TRIDENT, LANCE, BOW, SLING,
    PN_FIREARMS,
    CROSSBOW, DART, SHURIKEN, BOOMERANG, PN_WHIP, UNICORN_HORN,
    PN_ATTACK_SPELL, PN_HEALING_SPELL, PN_DIVINATION_SPELL,
    PN_ENCHANTMENT_SPELL, PN_CLERIC_SPELL, PN_ESCAPE_SPELL, PN_MATTER_SPELL,
    PN_BARE_HANDED, PN_TWO_WEAPONS, PN_RIDING,

    PN_POWER_ATTACK,
    PN_FLAMING_FISTS, PN_FREEZING_FISTS, PN_SHOCKING_FISTS, PN_STUNNING_FIST,
    PN_BACKSTAB, PN_CAREFUL_ATTACK, PN_RAT_TAMER, PN_DRAGON_TAMER, 
    PN_ANIMAL_FRIENDSHIP, PN_BLOOD_RAGE, PN_PANACHE, PN_WILD_MAGIC,
    PN_BLOOD_MAGIC, PN_CODE_OF_HONOR,
    PN_DISARM, PN_SUNDER, PN_TUMBLING,

    PN_SPIDER_FRIEND
};

/* note: entry [0] isn't used */
static NEARDATA const char *const odd_skill_names[] = {
    "no skill", "bare hands", /* use barehands_or_martial[] instead */
    "two weapon combat", "riding", "polearms", "saber", "hammer", "firearms",
    "whip", "attack spells", "healing spells", "divination spells",
    "enchantment spells", "clerical spells", "escape spells", "matter spells",

    "power attack",
    "flaming fists",
    "freezing fists",
    "shocking fists",
    "stunning fist",
    "backstab",
    "careful attack",
    "rat tamer",
    "dragon tamer",
    "animal friendship",
    "blood rage",
    "panache",
    "wild magic",
    "blood magic",
    "code of honor",
    "disarm",
    "sunder",
    "tumbling",

    "spider friend"
};
/* indexed via is_martial() */
static NEARDATA const char *const barehands_or_martial[] = {
    "bare handed combat", "martial arts"
};

#define P_NAME(type)                                    \
    ((skill_names_indices[type] > 0)                    \
         ? OBJ_NAME(objects[skill_names_indices[type]]) \
         : (type == P_BARE_HANDED_COMBAT)               \
               ? barehands_or_martial[martial_bonus()]  \
               : odd_skill_names[-skill_names_indices[type]])

static NEARDATA const char kebabable[] = { S_XORN, S_DRAGON, S_JABBERWOCK,
                                           S_NAGA, S_GIANT,  '\0' };

static void
give_may_advance_msg(int skill)
{
    if (!g.context.skip_enhance_msg) {
        g.context.skip_enhance_msg = TRUE;
    } else {
        return;
    }
    You_feel("more confident in your %sskills.",
             (skill == P_NONE) ? ""
                 : (skill <= P_LAST_WEAPON) ? "weapon "
                     : (skill <= P_LAST_SPELL) ? "spell casting "
                        : (skill < P_FIRST_ROLE) ? "fighting "
                          : (skill <= P_LAST_ROLE) ? "class "
                            : "cultural ");

    if (!g.context.enhance_tip) {
        g.context.enhance_tip = TRUE;
        pline("(Use the #enhance command to advance them.)");
    }
}

static void
display_skill_db_info(int skill) {
    char tmpbuf[BUFSZ];
    tmpbuf[0] = '\0';
    Sprintf(tmpbuf, "skill: %s", P_NAME(skill));
    checkfile(tmpbuf, (struct permonst *) 0, FALSE, TRUE, 0, (char *) 0);
}

/* weapon's skill category name for use as generalized description of weapon;
   mostly used to shorten "you drop your <weapon>" messages when slippery
   fingers or polymorph causes hero to involuntarily drop wielded weapon(s) */
const char *
weapon_descr(struct obj *obj)
{
    int skill = weapon_type(obj);
    const char *descr = P_NAME(skill);

    /* assorted special cases */
    switch (skill) {
    case P_NONE:
        /* not a weapon or weptool: use item class name;
           override class name for things where it sounds strange and
           for things that aren't unexpected to find being wielded:
           corpses, tins, eggs, and globs avoid "food",
           statues and boulders avoid "large rock",
           and towels and tin openers avoid "tool" */
        descr = (obj->otyp == CORPSE || obj->otyp == TIN || obj->otyp == EGG
                 || obj->otyp == STATUE || obj->otyp == BOULDER
                 || obj->otyp == TOWEL || obj->otyp == TIN_OPENER)
                ? OBJ_NAME(objects[obj->otyp])
                : obj->globby ? "glob"
                  : def_oc_syms[(int) obj->oclass].name;
        break;
    case P_SLING:
        if (is_ammo(obj))
            descr = (obj->otyp == ROCK || is_graystone(obj))
                        ? "stone"
                        /* avoid "rock"; what about known glass? */
                        : (obj->oclass == GEM_CLASS)
                            ? "gem"
                            /* in case somebody adds odd sling ammo */
                            : def_oc_syms[(int) obj->oclass].name;
        break;
    case P_BOW:
        if (is_ammo(obj))
            descr = "arrow";
        break;
    case P_CROSSBOW:
        if (is_ammo(obj))
            descr = "bolt";
        break;
    case P_FIREARM:
        if (is_ammo(obj))
            descr = "bullet";
        break;
    case P_FLAIL:
        if (obj->otyp == GRAPPLING_HOOK)
            descr = "hook";
        break;
    case P_PICK_AXE:
        /* even if "dwarvish mattock" hasn't been discovered yet */
        if (obj->otyp == DWARVISH_MATTOCK)
            descr = "mattock";
        break;
    default:
        break;
    }
    return makesingular(descr);
}

int
base_hitbonus(struct obj *otmp) {
    int tmp = 0;
    boolean Is_weapon;

    if (!otmp)
        return 0;
    
    Is_weapon = (otmp->oclass == WEAPON_CLASS || is_weptool(otmp));

    if (Is_weapon)
        tmp += otmp->spe;

    /* Put weapon specific "to hit" bonuses in below: */
    tmp += objects[otmp->otyp].oc_hitbon;

    return tmp;
}

float
role_bab()
{
    switch(Role_switch) {
    case PM_WIZARD:
    case PM_TOURIST:
    case PM_CONVICT:
    case PM_HEALER:
    case PM_CARTOMANCER:
        return 0.5;
    case PM_MONK:
    case PM_ARCHEOLOGIST:
    case PM_CLERIC:
    case PM_ROGUE:
    case PM_DRAGON_RIDER:
        return 0.75;
    default:
        return 1;
    }
}

int
role_powregen()
{
    switch(Role_switch) {
    case PM_WIZARD:
        return 2;
    case PM_TOURIST:
    case PM_MONK:
    case PM_ARCHEOLOGIST:
    case PM_CLERIC:
        return 3;
    default:
        return 4;
    }
}

int
botl_hitbonus()
{
    int tmp, tmp2;
    uchar aatyp = g.youmonst.data->mattk[0].aatyp;
    struct obj *weapon = uwep;

    /* tmp = abon() + u.uhitinc + maybe_polyd(g.youmonst.data->mlevel, u.ulevel); */
    tmp = abon() + u.uhitinc + (int) (maybe_polyd(g.youmonst.data->mlevel, u.ulevel) * role_bab());

    /* role/race adjustments */
    if (Role_if(PM_MONK) && !Upolyd) {
        if (uarm)
            tmp -= g.urole.spelarmr;
        else if (!uwep && !uarms)
            tmp += (u.ulevel / 3) + 2;
    }

    /* Power attack penalty */
    if (P_SKILL(P_POWER_ATTACK) > P_UNSKILLED
        || P_SKILL(P_CAREFUL_ATTACK) > P_UNSKILLED) {
        tmp -= P_SKILL(P_POWER_ATTACK);
        tmp += P_SKILL(P_CAREFUL_ATTACK);
        tmp += P_SKILL(P_CODE_OF_HONOR);
    }

    if (uwep && (uwep->otyp == HEAVY_IRON_BALL)) {
        tmp += 4;
    }

    if ((tmp2 = near_capacity()) != 0)
        tmp -= (tmp2 * 2) - 1;
    if (u.utrap)
        tmp -= 3;

    if (aatyp == AT_WEAP || aatyp == AT_CLAW) {
        if (weapon)
            tmp += base_hitbonus(uwep);
        if (weapon && !weapon->known)
            tmp -= max(0, weapon->spe);
        tmp += weapon_hit_bonus(weapon);
    } else if (aatyp == AT_KICK && martial_bonus()) {
        tmp += weapon_hit_bonus((struct obj *) 0);
    }

    return tmp;
}

/*
 *      hitval returns an integer representing the "to hit" bonuses
 *      of "otmp" against the monster.
 */
int
hitval(struct obj *otmp, struct monst *mon)
{
    int tmp = 0;
    struct permonst *ptr = mon->data;
    boolean Is_weapon = (otmp->oclass == WEAPON_CLASS || is_weptool(otmp));

    /* Add the weapon's basic to-hit bonus */
    tmp += base_hitbonus(otmp);

    /* Power attack penalty */
    tmp -= P_SKILL(P_POWER_ATTACK);
    tmp += P_SKILL(P_CAREFUL_ATTACK);
    tmp += P_SKILL(P_CODE_OF_HONOR);

    /* Put weapon vs. monster type "to hit" bonuses in below: */

    /* Blessed weapons used against undead or demons */
    if (Is_weapon && otmp->blessed
        && (is_demon(ptr) || is_undead(ptr) || is_vampshifter(mon)))
        tmp += 2;

    if (is_spear(otmp) && index(kebabable, ptr->mlet))
        tmp += 2;

    /* trident is highly effective against swimmers */
    if (otmp->otyp == TRIDENT && is_swimmer(ptr)) {
        if (is_pool(mon->mx, mon->my))
            tmp += 4;
        else if (ptr->mlet == S_EEL || ptr->mlet == S_SNAKE)
            tmp += 2;
    }

    /* Picks used against xorns and earth elementals */
    if (is_pick(otmp) && (passes_walls(ptr) && thick_skinned(ptr)))
        tmp += 2;

    /* Check specially named weapon "to hit" bonuses */
    if (otmp->oartifact)
        tmp += spec_abon(otmp, mon);

    return tmp;
}

#define LOW_ROLL 1
#define HIGH_ROLL 2
#define AVERAGE_ROLL 3
#define RANDOM_ROLL 4

char*
describe_dmgval(char *buf, struct obj *otmp, boolean bigmonst) {
    boolean Is_weapon = (otmp->oclass == WEAPON_CLASS || is_weptool(otmp));
    int otyp = otmp->otyp;
    int flatbon = bigmonst ? (objects[otyp].oc_wldam ? objects[otyp].oc_wldam : 0)
                  : (objects[otyp].oc_wsdam ? objects[otyp].oc_wsdam : 0);
    int spebon = 0;
    int matbon = 0;
    int low_dmg, high_dmg;

    if (Is_weapon && otmp->known) {
        spebon += otmp->spe;
    }

    matbon += odd_material_damage(otmp);
    if (P_SKILL(P_POWER_ATTACK) > P_UNSKILLED
        || P_SKILL(P_CAREFUL_ATTACK) > P_UNSKILLED) {
        matbon += P_SKILL(P_POWER_ATTACK);
        matbon -= P_SKILL(P_CAREFUL_ATTACK);
    }
    flatbon -= greatest_erosion(otmp);
    flatbon += spebon;
    if (flatbon < 1)
        flatbon = 1;

    low_dmg = 1 + spebon + matbon + weapon_distribution_bonus(otyp, bigmonst, LOW_ROLL);
    high_dmg = flatbon + matbon + weapon_distribution_bonus(otyp, bigmonst, HIGH_ROLL);
    
    Sprintf(eos(buf), "%d-%d%s", low_dmg, high_dmg, otmp->oartifact ? "++" : "");
    return buf;
}

static int
weapon_distribution_bonus(int otyp, boolean bigmonst, int distribution) {
    int num_dice = 0;
    int sides = 0;
    if (bigmonst) {
        switch (otyp) {
        case IRON_CHAIN:
        case CROSSBOW_BOLT:
        case MORNING_STAR:
        case NASTY_PIKE:
        case RUNESWORD:
        case ELVEN_BROADSWORD:
        case BROADSWORD:
            num_dice = 1;
            sides = 1;
            break;

        case FLAIL:
        case RANSEUR:
        case VOULGE:
        case SCYTHE:
            num_dice = 1;
            sides = 4;
            break;

        case ACIDIC_LIQUID:
        case HALBERD:
        case SPETUM:
            num_dice = 1;
            sides = 6;
            break;

        case BATTLE_AXE:
        case BARDICHE:
        case TRIDENT:
        case SPIKED_CHAIN:
            num_dice = 2;
            sides = 4;
            break;

        case TSURUGI:
        case DWARVISH_MATTOCK:
        case TWO_HANDED_SWORD:
            num_dice = 2;
            sides = 6;
            break;
        }
    } else {
        switch (otyp) {
        case IRON_CHAIN:
        case CROSSBOW_BOLT:
        case MACE:
        case WAR_HAMMER:
        case FLAIL:
        case SPETUM:
        case TRIDENT:
        case SPIKED_CHAIN:
            num_dice = 1;
            sides = 1;
            break;

        case BATTLE_AXE:
        case BARDICHE:
        case BILL_GUISARME:
        case GUISARME:
        case LUCERN_HAMMER:
        case MORNING_STAR:
        case RANSEUR:
        case BROADSWORD:
        case ELVEN_BROADSWORD:
        case RUNESWORD:
        case VOULGE:
        case SCYTHE:
            num_dice = 1;
            sides = 4;
            break;

        case ACIDIC_LIQUID:
            num_dice = 1;
            sides = 6;
            break;
        }
    }

    switch(distribution) {
    case LOW_ROLL:
        return num_dice;
    case HIGH_ROLL:
        return num_dice * sides;
    case AVERAGE_ROLL:
        return (int) (num_dice * sides / 2);
        break;
    case RANDOM_ROLL:
    default:
        d(num_dice, sides);
        break;
    }

    return 0;
}

static int
odd_material_damage(struct obj * otmp) {
    int tmp = 0;
    if ((is_odd_material(otmp, GLASS) || is_odd_material(otmp, ADAMANTINE)
         || is_odd_material(otmp, GEMSTONE))
        && (objects[otmp->otyp].oc_dir & (PIERCE | SLASH))) {
        /* glass, crystal, and adamantine are sharp */
        tmp += 3;
    }
    else if (is_odd_material(otmp, GOLD) || is_odd_material(otmp, PLATINUM)) {
        /* heavy metals */
        if (objects[otmp->otyp].oc_dir == WHACK) {
            tmp += 2;
        }
    }
    else if (is_odd_material(otmp, MINERAL)) {
        /* stone is heavy */
        if (objects[otmp->otyp].oc_dir == WHACK) {
            tmp += 1;
        }
    }
    else if (is_odd_material(otmp, PLASTIC) || is_odd_material(otmp, PAPER)) {
        /* just terrible weapons all around */
        tmp -= 2;
    }
    else if (is_odd_material(otmp, SLIME)) {
        /* even worse... */
        tmp -= 4;
    }
    else if (is_odd_material(otmp, WOOD)) {
        /* poor at holding an edge */
        if (is_blade(otmp)) {
            tmp -= 1;
        }
    }
    return tmp;
}

/* Historical note: The original versions of Hack used a range of damage
 * which was similar to, but not identical to the damage used in Advanced
 * Dungeons and Dragons.  I figured that since it was so close, I may as well
 * make it exactly the same as AD&D, adding some more weapons in the process.
 * This has the advantage that it is at least possible that the player would
 * already know the damage of at least some of the weapons.  This was circa
 * 1987 and I used the table from Unearthed Arcana until I got tired of typing
 * them in (leading to something of an imbalance towards weapons early in
 * alphabetical order).  The data structure still doesn't include fields that
 * fully allow the appropriate damage to be described (there's no way to say
 * 3d6 or 1d6+1) so we add on the extra damage in dmgval() if the weapon
 * doesn't do an exact die of damage.
 *
 * Of course new weapons were added later in the development of Nethack.  No
 * AD&D consistency was kept, but most of these don't exist in AD&D anyway.
 *
 * Second edition AD&D came out a few years later; luckily it used the same
 * table.  As of this writing (1999), third edition is in progress but not
 * released.  Let's see if the weapon table stays the same.  --KAA
 * October 2000: It didn't.  Oh, well.
 */

/*
 *      dmgval returns an integer representing the damage bonuses
 *      of "otmp" against the monster.
 */
int
dmgval(struct obj *otmp, struct monst *mon)
{
    int tmp = 0, otyp = otmp->otyp;
    struct permonst *ptr = mon->data;
    boolean Is_weapon = (otmp->oclass == WEAPON_CLASS || is_weptool(otmp));

    if (otyp == CREAM_PIE || otyp == APPLE_PIE || otyp == PUMPKIN_PIE)
        return 0;

    if (bigmonst(ptr) && objects[otyp].oc_wldam) {
        tmp = rnd(objects[otyp].oc_wldam);
    } else if (objects[otyp].oc_wsdam) {
        tmp = rnd(objects[otyp].oc_wsdam);
    }

    tmp += weapon_distribution_bonus(otyp, bigmonst(ptr), RANDOM_ROLL);

    if (Is_weapon) {
        tmp += otmp->spe;
    /* adjust for various materials */
    tmp += odd_material_damage(otmp);

    /* negative modifiers mustn't produce negative damage */
    if (tmp < 0)
        tmp = 0;
    }

    if (otmp->material <= LEATHER && thick_skinned(ptr))
        /* thick skinned/scaled creatures don't feel it */
        tmp = 0;
    if (ptr == &mons[PM_SHADE] && !shade_glare(otmp))
        tmp = 0;

    /* "very heavy iron ball"; weight increase is in increments */
    if (otyp == HEAVY_IRON_BALL && tmp > 0) {
        int wt = (int) objects[HEAVY_IRON_BALL].oc_weight;

        if ((int) otmp->owt > wt) {
            wt = ((int) otmp->owt - wt) / IRON_BALL_W_INCR;
            tmp += rnd(4 * wt);
            if (tmp > 25)
                tmp = 25; /* objects[].oc_wldam */
        }
    }

    /* power attack yields a flat damage bonus increase. */
    tmp += P_SKILL(P_POWER_ATTACK);
    tmp -= P_SKILL(P_CAREFUL_ATTACK);

    /* Put weapon vs. monster type damage bonuses in below: */
    if (Is_weapon || otmp->oclass == GEM_CLASS || otmp->oclass == BALL_CLASS
        || otmp->oclass == CHAIN_CLASS) {
        int bonus = 0;

        if (otmp->blessed
            && (is_undead(ptr) || is_demon(ptr) || is_vampshifter(mon)))
            bonus += rnd(4);
        if (is_axe(otmp) && is_wooden(ptr))
            bonus += rnd(4);
        if (mon_hates_material(mon, otmp->material))
            bonus += rnd(sear_damage(otmp->material));
        if (artifact_light(otmp) && otmp->lamplit && hates_light(ptr))
            bonus += rnd(8);

        /* if the weapon is going to get a double damage bonus, adjust
           this bonus so that effectively it's added after the doubling */
        if (bonus > 1 && otmp->oartifact && spec_dbon(otmp, mon, 25) >= 25)
            bonus = (bonus + 1) / 2;

        tmp += bonus;
    }

    if (tmp > 0) {
        /* It's debatable whether a rusted blunt instrument
           should do less damage than a pristine one, since
           it will hit with essentially the same impact, but
           there ought to some penalty for using damaged gear
           so always subtract erosion even for blunt weapons. */
        tmp -= greatest_erosion(otmp);
        if (tmp < 1)
            tmp = 1;
    }

    return  tmp;
}

/* Find an object that magr is wearing (or even magr's body itself) that has a
 * special damaging effect on mdef, and return the amount of bonus damage done.
 * The most damaging source has precedence; each source that causes special
 * damage makes its own roll for damage, and the highest roll will be applied.
 */
#undef LOW_ROLL
#undef HIGH_ROLL
#undef AVERAGE_ROLL
#undef RANDOM_ROLL

/* Find an object that magr is wearing (or even magr's body itself) that has a
 * special damaging effect on mdef, and return the amount of bonus damage done.
 * The most damaging source has precedence; each source that causes special
 * damage makes its own roll for damage, and the highest roll will be applied.
 *
 * hated_obj is set only on objects that cause material hatred; if none of the
 * applicable objects are of a hated material, it will not be set.
 */
int
special_dmgval(struct monst *magr,
               struct monst *mdef,
               long armask,  /* armor mask of all the slots that can be touching
                              * mdef */
               struct obj **hated_obj) /* ptr to offending object, can be NULL
                                        * if not wanted */
{
    boolean youattack = (magr == &g.youmonst);
    const int magr_material = monmaterial(monsndx(magr->data));
    int bonus = 0;
    int tmpbonus = 0;
    boolean try_body = FALSE;
    struct obj *gloves    = which_armor(magr, W_ARMG),
               *helm      = which_armor(magr, W_ARMH),
               *shield    = which_armor(magr, W_ARMS),
               *boots     = which_armor(magr, W_ARMF),
               *armor     = which_armor(magr, W_ARM),
               *cloak     = which_armor(magr, W_ARMC),
               *shirt     = which_armor(magr, W_ARMU),
               *leftring  = (youattack ? uleft : which_armor(magr, W_RINGL)),
               *rightring = (youattack ? uright : which_armor(magr, W_RINGR));

    if (hated_obj) {
        *hated_obj = 0;
    }

    /* Simple exclusions where we ignore a certain type of armor because it is
     * covered by some other equipment. */
    if (gloves) {
        leftring = rightring = NULL;
    }
    if (cloak) {
        armor = shirt = NULL;
    }
    if (armor) {
        shirt = NULL;
    }

    /* Cases where we want to count magr's body: the caller indicates a certain
     * slot is making contact, and magr is not wearing anything in that slot, so
     * their body must be making contact.
     * Note: in the gloves case, rings don't prevent magr's body from making
     * contact. */
    if (((armask & W_ARMG) && !gloves)
        || ((armask & W_ARMF) && !boots)
        || ((armask & W_ARMH) && !helm)
        || ((armask & (W_ARMC | W_ARM | W_ARMU))
            && !cloak && !armor && !shirt)) {
        try_body = TRUE;
    }

    if (try_body && mon_hates_material(mdef, magr_material)) {
        bonus = sear_damage(magr_material);
        if (hated_obj)
            *hated_obj = (struct obj *) &cg.zeroobj;
    }

    /* The order of armor slots in this array doesn't really matter because we
     * roll for everything that applies and take the highest damage. */
    struct {
        long mask;
        struct obj* obj;
    } array[9] = {
        { W_ARMG, gloves },
        { W_ARMH, helm   },
        { W_ARMS, shield },
        { W_ARMF, boots  },
        { W_ARM,  armor  },
        { W_ARMC, cloak  },
        { W_ARMU, shirt  },
        { W_RINGL, leftring },
        { W_RINGR, rightring }
    };

    int i;
    for (i = 0; i < 9; ++i) {
        if (array[i].obj && (armask & array[i].mask)) {
            tmpbonus = dmgval(array[i].obj, mdef);
            if (tmpbonus > bonus) {
                bonus = tmpbonus;
                if (hated_obj) {
                    /* Select hated_obj based on the maximum possible amount of
                     * damage that can be caused by its material, not the actual
                     * random amount of damage.
                     * E.g. if you manage to hit a monster that hates both
                     * silver and iron with a silver and an iron item at the
                     * same time, silver has a more severe effect dealing more
                     * damage so hated_obj should always be set to the silver
                     * one, so that searmsg will get called with the most
                     * appropriate message.
                     */
                    if (mon_hates_material(mdef, array[i].obj->material)
                        && (*hated_obj == NULL
                            || (sear_damage(array[i].obj->material)
                                > sear_damage((*hated_obj)->material)))) {
                        *hated_obj = array[i].obj;
                    }
                }
            }
        }
    }
    return bonus;
}

/* give a "silver <item> sears <target>" message (or similar for other
 * material); in addition to weapon hit, this is used for rings, boots for kick,
 * gloves for punch, or helm for headbutt.
 */
void
searmsg(struct monst *magr, struct monst *mdef, const struct obj *obj, boolean minimal)
{
    boolean youattack = (magr == &g.youmonst);
    boolean youdefend = (mdef == &g.youmonst);
    boolean has_flesh = (!noncorporeal(mdef->data) && !amorphous(mdef->data));

    if (!obj) {
        impossible("searmsg: nothing searing?");
        return;
    }
    if (!youdefend && !canspotmon(mdef)) {
        return;
    }

    char onamebuf[BUFSZ];
    char whose[BUFSZ];
    int mat;

    if (obj == &cg.zeroobj) {
        mat = monmaterial(monsndx(magr->data));
        Sprintf(onamebuf, "%s touch", materialnm[mat]);
        if (youattack) {
            Strcpy(whose, "your ");
        }
        else if (!magr) {
            impossible("searmsg: non-weapon attack with no aggressor?");
            return;
        }
        else {
            Strcpy(whose, s_suffix(y_monnam(magr)));
            Strcat(whose, " ");
        }
    }
    else {
        mat = obj->material;
        const char* matname = materialnm[mat];

        /* Why doesn't cxname receive a const struct obj? */
        char* cxnameobj = cxname((struct obj *) obj);

        /* Make it explicit to the player that this effect is from the material,
         * by prepending the material, but only if the object's name doesn't
         * already contain the material string somewhere.  (e.g. "sword" should
         * turn into "iron sword", but "engraved silver bell" shouldn't turn
         * into "silver engraved silver bell") */
        boolean alreadyin = (strstri(cxnameobj, matname) != NULL);
        if (!alreadyin) {
            Sprintf(onamebuf, "%s %s", matname, cxnameobj);
        }
        else {
            Strcpy(onamebuf, cxnameobj);
        }
        shk_your(whose, (struct obj *) obj);
    }

    if (minimal) {
        /* instead of "foo's obj", it will be "the [touch of] <material>;
         * whose becomes "the" in both cases */
        Strcpy(whose, "the ");
        if (mat == SILVER) { /* different formatting */
            Strcpy(onamebuf, "silver");
        }
        else {
            Sprintf(onamebuf, "touch of %s", materialnm[mat]);
        }
    }

    /* "Extra-minimal" case where we don't know what is doing the searing.
     * Note that this can assume it will be formatting some non-player entity
     * because it only applies when the player isn't involved. */
    if (!youattack && !youdefend && !canseemon(mdef) && minimal) {
        if (mat == SILVER || mat == COLD_IRON) {
            char defender[BUFSZ];
            if (has_flesh) {
                Sprintf(defender, "%s flesh", s_suffix(Monnam(mdef)));
            }
            else {
                Strcpy(defender, Monnam(mdef));
            }
            pline("%s is seared!", defender);
        }
        else {
            pline("%s recoils!", Monnam(mdef));
        }
        return;
    }

    /* char* whom = youdefend ? "you" : mon_nam(mdef); */
    char* whom = mon_nam(mdef);
    if (youdefend) {
        Strcpy(whom, "you");
    }

    if (mat == SILVER || mat == COLD_IRON) { /* more dramatic effects than other materials */
        /* note: s_suffix returns a modifiable buffer */
        if (has_flesh)
            whom = strcat(s_suffix(whom), " flesh");

        pline("%s%s %s %s!", upstart(whose), onamebuf,
              vtense(onamebuf, "sear"), whom);
    }
    else {
        whom = upstart(whom);
        pline("%s recoil%s from %s%s!", whom, (youdefend ? "" : "s"),
              whose, onamebuf);
    }
}

/* give a "silver <item> sears <target>" message;
   not used for weapon hit, so we only handle rings */
void
silver_sears(struct monst *magr UNUSED, struct monst *mdef,
             long silverhit)
{
    char rings[20]; /* plenty of room for "rings" */
    int ltyp = ((uleft && (silverhit & W_RINGL) != 0L)
                ? uleft->otyp : STRANGE_OBJECT),
        rtyp = ((uright && (silverhit & W_RINGR) != 0L)
                ? uright->otyp : STRANGE_OBJECT);
    boolean both,
        l_ag = (objects[ltyp].oc_material == SILVER && uleft->dknown),
        r_ag = (objects[rtyp].oc_material == SILVER && uright->dknown);

    if ((silverhit & (W_RINGL | W_RINGR)) != 0L) {
        /* plural if both the same type (so not multi_claw and both rings
           are non-Null) and either both known or neither known, or both
           silver (in case there is ever more than one type of silver ring)
           and both known; singular if multi_claw (where one of ltyp or
           rtyp will always be STRANGE_OBJECT) even if both rings are known
           silver [see hmonas(uhitm.c) for explanation of 'multi_claw'] */
        both = ((ltyp == rtyp && uleft->dknown == uright->dknown)
                || (l_ag && r_ag));
        Sprintf(rings, "ring%s", both ? "s" : "");
        Your("%s%s %s %s!",
             (l_ag || r_ag) ? "silver "
             : both ? ""
               : ((silverhit & W_RINGL) != 0L) ? "left "
                 : "right ",
             rings, vtense(rings, "sear"), mon_nam(mdef));
    }
}

static struct obj *oselect(struct monst *, int);
#define Oselect(x)                      \
    if ((otmp = oselect(mtmp, x)) != 0) \
        return otmp;

static struct obj *
oselect(struct monst *mtmp, int x)
{
    struct obj *otmp, *obest = 0;

    for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj) {
        if (otmp->otyp == x
            /* never select non-cockatrice corpses */
            && !((x == CORPSE || x == EGG)
                 && !touch_petrifies(&mons[otmp->corpsenm]))
            && (!otmp->oartifact || touch_artifact(otmp, mtmp))
            && !mon_hates_material(mtmp, otmp->material)) {
       	        if (!obest ||
             		    dmgval(otmp, &g.youmonst) > dmgval(obest, &g.youmonst))
             		    obest = otmp;
        }
    }
             	return obest;
}

/* TODO: have monsters use aklys' throw-and-return */
static NEARDATA const int rwep[] = {
    FRAG_GRENADE, GAS_GRENADE, BULLET, SHOTGUN_SHELL,
    DWARVISH_SPEAR, ELVEN_SPEAR, SPEAR, ORCISH_SPEAR, JAVELIN,
    THROWING_AXE,
    SHURIKEN, LIGHT_ARROW, YA, ELVEN_ARROW, DARK_ELVEN_ARROW,
    ARROW, ORCISH_ARROW,
    CROSSBOW_BOLT, ELVEN_DAGGER, DARK_ELVEN_DAGGER, DAGGER, ORCISH_DAGGER, KNIFE,
    FLINT, ROCK, LOADSTONE, LUCKSTONE, DART, FRUITCAKE, PINEAPPLE,
    /* CHAKRAM, BOOMERANG, */ CREAM_PIE
};

static NEARDATA const int pwep[] = { SPIKED_CHAIN,
                                     HALBERD,       BARDICHE, SPETUM,
                                     BILL_GUISARME, VOULGE,   RANSEUR,
                                     GUISARME,      GLAIVE,   LUCERN_HAMMER,
                                     BEC_DE_CORBIN, FAUCHARD, NASTY_PIKE,
                                     LANCE };

/* select a ranged weapon for the monster */
struct obj *
select_rwep(struct monst *mtmp)
{
    register struct obj *otmp;
    struct obj *mwep;
    boolean mweponly;
    int i;

    char mlet = mtmp->data->mlet;

    g.propellor = (struct obj *) &cg.zeroobj;
    Oselect(EGG);      /* cockatrice egg */
    if (mlet == S_KOP) /* pies are first choice for Kops */
        Oselect(CREAM_PIE);
    if (throws_rocks(mtmp->data)) /* ...boulders for giants */
        Oselect(BOULDER);

    /* Select polearms first; they do more damage and aren't expendable.
       But don't pick one if monster's weapon is welded, because then
       we'd never have a chance to throw non-wielding missiles. */
    /* The limit of 13 here is based on the monster polearm range limit
     * (defined as 5 in mthrowu.c).  5 corresponds to a distance of 2 in
     * one direction and 1 in another; one space beyond that would be 3 in
     * one direction and 2 in another; 3^2+2^2=13.
     */
    mwep = MON_WEP(mtmp);
    /* NO_WEAPON_WANTED means we already tried to wield and failed */
    mweponly = (mwelded(mwep) && mtmp->weapon_check == NO_WEAPON_WANTED);
    if (dist2(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy) <= 13
        && couldsee(mtmp->mx, mtmp->my)) {
        for (i = 0; i < SIZE(pwep); i++) {
            /* Only strong monsters can wield big (esp. long) weapons.
             * Big weapon is basically the same as bimanual.
             * All monsters can wield the remaining weapons.
             */
            if ((strongmonst(mtmp->data)
                  && (mtmp->misc_worn_check & W_ARMS) == 0)
                 || !objects[pwep[i]].oc_bimanual) {
                if ((otmp = oselect(mtmp, pwep[i])) != 0
                    && (otmp == mwep || !mweponly)
                    && !mon_hates_material(mtmp, otmp->material)) {
                    g.propellor = otmp; /* force the monster to wield it */
                    return otmp;
                }
            }
        }
    }

    /*
     * other than these two specific cases, always select the
     * most potent ranged weapon to hand.
     */
    for (i = 0; i < SIZE(rwep); i++) {
        int prop;

        /* shooting gems from slings; this goes just before the darts */
        /* (shooting rocks is already handled via the rwep[] ordering) */
        if (rwep[i] == DART && !likes_gems(mtmp->data)
            && m_carrying(mtmp, SLING)) { /* propellor */
            for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
                if (otmp->oclass == GEM_CLASS
                    && (otmp->otyp != LOADSTONE || !otmp->cursed)) {
                    g.propellor = m_carrying(mtmp, SLING);
                    return otmp;
                }
        }

        /* KMH -- This belongs here so darts will work */
        g.propellor = (struct obj *) &cg.zeroobj;

        prop = objects[rwep[i]].oc_skill;
        if (prop < 0) {
            switch (-prop) {
            case P_BOW:
                g.propellor = oselect(mtmp, FOOTBOW);
                if (!g.propellor)
                    g.propellor = (oselect(mtmp, YUMI));
                if (!g.propellor)
                    g.propellor = oselect(mtmp, ELVEN_BOW);
                if (!g.propellor)
                    g.propellor = (oselect(mtmp, DARK_ELVEN_BOW));
                if (!g.propellor)
                    g.propellor = oselect(mtmp, BOW);
                if (!g.propellor)
                    g.propellor = oselect(mtmp, ORCISH_BOW);
                break;
            case P_SLING:
                g.propellor = oselect(mtmp, SLING);
                break;
            case P_CROSSBOW:
                g.propellor = oselect(mtmp, CROSSBOW);
                break;
            case P_FIREARM:
                if ((objects[rwep[i]].w_ammotyp) == WP_BULLET) {
                    g.propellor = (oselect(mtmp, HEAVY_MACHINE_GUN));
                    if (!g.propellor) g.propellor = (oselect(mtmp, SUBMACHINE_GUN));
                    if (!g.propellor) g.propellor = (oselect(mtmp, SNIPER_RIFLE));
                    if (!g.propellor) g.propellor = (oselect(mtmp, RIFLE));
                    if (!g.propellor) g.propellor = (oselect(mtmp, PISTOL));
                } else if ((objects[rwep[i]].w_ammotyp) == WP_SHELL) {
                    g.propellor = (oselect(mtmp, AUTO_SHOTGUN));
                    if (!g.propellor) g.propellor = (oselect(mtmp, SHOTGUN));
                }
                break;
            }
            if ((otmp = MON_WEP(mtmp)) && mwelded(otmp) && otmp != g.propellor
                && mtmp->weapon_check == NO_WEAPON_WANTED)
                g.propellor = 0;
        }
        /* propellor = obj, propellor to use
         * propellor = &cg.zeroobj, doesn't need a propellor
         * propellor = 0, needed one and didn't have one
         */
        if (g.propellor != 0) {
            /* Note: cannot use m_carrying for loadstones, since it will
             * always select the first object of a type, and maybe the
             * monster is carrying two but only the first is unthrowable.
             */
            if (rwep[i] != LOADSTONE) {
                /* Don't throw a cursed weapon-in-hand or an artifact */
                if ((otmp = oselect(mtmp, rwep[i])) && !otmp->oartifact
                    && !(otmp == MON_WEP(mtmp) && mwelded(otmp)))
                    return otmp;
            } else
                for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj) {
                    if (otmp->otyp == LOADSTONE && !otmp->cursed)
                        return otmp;
                }
        }
    }

    /* failure */
    return (struct obj *) 0;
}

/* is 'obj' a type of weapon that any monster knows how to throw? */
boolean
monmightthrowwep(struct obj *obj)
{
    short idx;

    for (idx = 0; idx < SIZE(rwep); ++idx)
        if (obj->otyp == rwep[idx])
            return TRUE;
    return FALSE;
}

/* Weapons in order of preference */
static const NEARDATA short hwep[] = {
    CORPSE, /* cockatrice corpse */
    SPIKED_CHAIN,
    TSURUGI, RUNESWORD, ORNATE_MACE, FLAMING_LASH, DWARVISH_MATTOCK,
    TWO_HANDED_SWORD, FALCHION, BATTLE_AXE, EXECUTIONER_S_MACE,
    KATANA, UNICORN_HORN, CRYSKNIFE, TRIDENT, LONG_SWORD, ELVEN_BROADSWORD,
    BROADSWORD, SCIMITAR, SABER, MORNING_STAR, ELVEN_SHORT_SWORD,
    DARK_ELVEN_SHORT_SWORD,
    DWARVISH_SHORT_SWORD, SHORT_SWORD, ORCISH_SHORT_SWORD, MACE, AXE,
    DWARVISH_SPEAR, ELVEN_SPEAR, SPEAR, ORCISH_SPEAR, FLAIL,
    BULLWHIP, BASEBALL_BAT, QUARTERSTAFF, JAVELIN, AKLYS, CLUB, PICK_AXE, RUBBER_HOSE,
    WAR_HAMMER, ELVEN_DAGGER, DAGGER, ORCISH_DAGGER, ATHAME,
    SCALPEL, KNIFE, WORM_TOOTH, RIFLE, PISTOL, AUTO_SHOTGUN, SHOTGUN
};

/* select a hand to hand weapon for the monster */
struct obj *
select_hwep(struct monst *mtmp)
{
    register struct obj *otmp;
    register int i;
    boolean strong = strongmonst(mtmp->data);
    boolean wearing_shield = (mtmp->misc_worn_check & W_ARMS) != 0;

    /* prefer artifacts to everything else */
    for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj) {
        if (otmp->oclass == WEAPON_CLASS && otmp->oartifact
            && touch_artifact(otmp, mtmp)
            && ((strong && !wearing_shield)
            || !objects[otmp->otyp].oc_bimanual)
        && !mon_hates_material(mtmp, otmp->material))
            return otmp;
    }

    if (is_giant(mtmp->data)) /* giants just love to use clubs */
        Oselect(CLUB);

    /* only strong monsters can wield big (esp. long) weapons */
    /* big weapon is basically the same as bimanual */
    /* all monsters can wield the remaining weapons */
    for (i = 0; i < SIZE(hwep); i++) {
        if (hwep[i] == CORPSE && !(mtmp->misc_worn_check & W_ARMG)
            && !resists_ston(mtmp))
            continue;
        if ((strong && !wearing_shield) || !objects[hwep[i]].oc_bimanual)
            Oselect(hwep[i]);
    }

    /* failure */
    return (struct obj *) 0;
}

/* Called after polymorphing a monster, robbing it, etc....  Monsters
 * otherwise never unwield stuff on their own.  Might print message.
 */
void
possibly_unwield(struct monst *mon, boolean polyspot)
{
    struct obj *obj, *mw_tmp;

    if (!(mw_tmp = MON_WEP(mon)))
        return;
    for (obj = mon->minvent; obj; obj = obj->nobj)
        if (obj == mw_tmp)
            break;
    if (!obj) { /* The weapon was stolen or destroyed */
        MON_NOWEP(mon);
        mon->weapon_check = NEED_WEAPON;
        return;
    }
    if (!attacktype(mon->data, AT_WEAP)) {
        setmnotwielded(mon, mw_tmp);
        mon->weapon_check = NO_WEAPON_WANTED;
        obj_extract_self(obj);
        if (cansee(mon->mx, mon->my)) {
            pline("%s drops %s.", Monnam(mon), distant_name(obj, doname));
            newsym(mon->mx, mon->my);
        }
        /* might be dropping object into water or lava */
        if (!flooreffects(obj, mon->mx, mon->my, "drop")) {
            if (polyspot)
                bypass_obj(obj);
            place_object(obj, mon->mx, mon->my);
            stackobj(obj);
        }
        return;
    }
    /* The remaining case where there is a change is where a monster
     * is polymorphed into a stronger/weaker monster with a different
     * choice of weapons.  This has no parallel for players.  It can
     * be handled by waiting until mon_wield_item is actually called.
     * Though the monster still wields the wrong weapon until then,
     * this is OK since the player can't see it.  (FIXME: Not okay since
     * probing can reveal it.)
     * Note that if there is no change, setting the check to NEED_WEAPON
     * is harmless.
     * Possible problem: big monster with big cursed weapon gets
     * polymorphed into little monster.  But it's not quite clear how to
     * handle this anyway....
     */
    if (!(mwelded(mw_tmp) && mon->weapon_check == NO_WEAPON_WANTED))
        mon->weapon_check = NEED_WEAPON;
    return;
}

/* Let a monster try to wield a weapon, based on mon->weapon_check.
 * Returns 1 if the monster took time to do it, 0 if it did not.
 */
int
mon_wield_item(struct monst *mon)
{
    struct obj *obj;

    /* This case actually should never happen */
    if (mon->weapon_check == NO_WEAPON_WANTED)
        return 0;
    switch (mon->weapon_check) {
    case NEED_HTH_WEAPON:
        obj = select_hwep(mon);
        break;
    case NEED_RANGED_WEAPON:
        (void) select_rwep(mon);
        obj = g.propellor;
        break;
    case NEED_PICK_AXE:
        obj = m_carrying(mon, PICK_AXE);
        /* KMH -- allow other picks */
        if (!obj && !which_armor(mon, W_ARMS))
            obj = m_carrying(mon, DWARVISH_MATTOCK);
        break;
    case NEED_AXE:
        /* currently, only 2 types of axe */
        obj = m_carrying(mon, BATTLE_AXE);
        if (!obj || which_armor(mon, W_ARMS))
            obj = m_carrying(mon, AXE);
        break;
    case NEED_PICK_OR_AXE:
        /* prefer pick for fewer switches on most levels */
        obj = m_carrying(mon, DWARVISH_MATTOCK);
        if (!obj)
            obj = m_carrying(mon, BATTLE_AXE);
        if (!obj || which_armor(mon, W_ARMS)) {
            obj = m_carrying(mon, PICK_AXE);
            if (!obj)
                obj = m_carrying(mon, AXE);
        }
        break;
    default:
        impossible("weapon_check %d for %s?", mon->weapon_check,
                   mon_nam(mon));
        return 0;
    }
    if (obj && obj != &cg.zeroobj) {
        struct obj *mw_tmp = MON_WEP(mon);

        if (mw_tmp && mw_tmp->otyp == obj->otyp) {
            /* already wielding it */
            mon->weapon_check = NEED_WEAPON;
            return 0;
        }
        /* Actually, this isn't necessary--as soon as the monster
         * wields the weapon, the weapon welds itself, so the monster
         * can know it's cursed and needn't even bother trying.
         * Still....
         */
        if (mw_tmp && mwelded(mw_tmp)) {
            if (canseemon(mon)) {
                char welded_buf[BUFSZ];
                const char *mon_hand = mbodypart(mon, HAND);

                if (bimanual(mw_tmp))
                    mon_hand = makeplural(mon_hand);
                Sprintf(welded_buf, "%s welded to %s %s",
                        otense(mw_tmp, "are"), mhis(mon), mon_hand);

                if (obj->otyp == PICK_AXE) {
                    pline("Since %s weapon%s %s,", s_suffix(mon_nam(mon)),
                          plur(mw_tmp->quan), welded_buf);
                    pline("%s cannot wield that %s.", mon_nam(mon),
                          xname(obj));
                } else {
                    pline("%s tries to wield %s.", Monnam(mon), doname(obj));
                    pline("%s %s!", Yname2(mw_tmp), welded_buf);
                }
                mw_tmp->bknown = 1;
            }
            mon->weapon_check = NO_WEAPON_WANTED;
            return 1;
        }
        mon->mw = obj; /* wield obj */
        setmnotwielded(mon, mw_tmp);
        mon->weapon_check = NEED_WEAPON;
        if (canseemon(mon)) {
            boolean newly_welded;

            pline("%s wields %s!", Monnam(mon), doname(obj));
            /* 3.6.3: mwelded() predicate expects the object to have its
               W_WEP bit set in owormmask, but the pline here and for
               artifact_light don't want that because they'd have '(weapon
               in hand/claw)' appended; so we set it for the mwelded test
               and then clear it, until finally setting it for good below */
            obj->owornmask |= W_WEP;
            newly_welded = mwelded(obj);
            obj->owornmask &= ~W_WEP;
            if (newly_welded) {
                pline("%s %s to %s %s!", Tobjnam(obj, "weld"),
                      is_plural(obj) ? "themselves" : "itself",
                      s_suffix(mon_nam(mon)), mbodypart(mon, HAND));
                obj->bknown = 1;
            }
        }
        if (artifact_light(obj) && !obj->lamplit) {
            begin_burn(obj, FALSE);
            if (canseemon(mon))
                pline("%s %s in %s %s!", Tobjnam(obj, "shine"),
                      arti_light_description(obj), s_suffix(mon_nam(mon)),
                      mbodypart(mon, HAND));
            /* 3.6.3: artifact might be getting wielded by invisible monst */
            else if (cansee(mon->mx, mon->my))
                pline("Light begins shining %s.",
                      (distu(mon->mx, mon->my) <= 5 * 5)
                          ? "nearby"
                          : "in the distance");
        }
        obj->owornmask = W_WEP;
        return 1;
    }
    mon->weapon_check = NEED_WEAPON;
    return 0;
}

/* force monster to stop wielding current weapon, if any */
void
mwepgone(struct monst *mon)
{
    struct obj *mwep = MON_WEP(mon);

    if (mwep) {
        setmnotwielded(mon, mwep);
        mon->weapon_check = NEED_WEAPON;
    }
}

/* attack bonus for strength & dexterity */
int
abon(void)
{
    int sbon;
    int str = ACURR(A_STR), dex = ACURR(A_DEX);

    if (Upolyd)
        return (adj_lev(&mons[u.umonnum]) - 3);
    if (str < 6)
        sbon = -2;
    else if (str < 8)
        sbon = -1;
    else if (str < 17)
        sbon = 0;
    else if (str <= STR18(50))
        sbon = 1; /* up to 18/50 */
    else if (str < STR18(100))
        sbon = 2;
    else
        sbon = 3;

    /* Game tuning kludge: make it a bit easier for a low level character to
     * hit */
    sbon += (u.ulevel < 3) ? 1 : 0;

    if (dex < 4)
        return (sbon - 3);
    else if (dex < 6)
        return (sbon - 2);
    else if (dex < 8)
        return (sbon - 1);
    else if (dex < 14)
        return sbon;
    else
        return (sbon + dex - 14);
}

/* damage bonus for strength */
int
dbon(void)
{
    int str = ACURR(A_STR);

    if (Upolyd)
        return 0;

    if (str < 6)
        return -1;
    else if (str < 16)
        return 0;
    else if (str < 18)
        return 1;
    else if (str == 18)
        return 2; /* up to 18 */
    else if (str <= STR18(75))
        return 3; /* up to 18/75 */
    else if (str <= STR18(90))
        return 4; /* up to 18/90 */
    else if (str < STR18(100))
        return 5; /* up to 18/99 */
    else
        return 6;
}

/* called when wet_a_towel() or dry_a_towel() is changing a towel's wetness */
static void
finish_towel_change(struct obj *obj, int newspe)
{
    /* towel wetness is always between 0 (dry) and 7, inclusive */
    newspe = min(newspe, 7);
    obj->spe = max(newspe, 0);

    /* if hero is wielding this towel, don't give "you begin bashing with
       your [wet] towel" message if it's wet, do give one if it's dry */
    if (obj == uwep)
        g.unweapon = !is_wet_towel(obj);

    /* description might change: "towel" vs "moist towel" vs "wet towel" */
    if (carried(obj))
        update_inventory();
}

/* increase a towel's wetness */
void
wet_a_towel(struct obj *obj,
            int amt, /* positive: new value; negative: increment by -amt;
                        zero: no-op */
            boolean verbose)
{
    int newspe = (amt <= 0) ? obj->spe - amt : amt;

    /* new state is only reported if it's an increase */
    if (newspe > obj->spe) {
        if (verbose) {
            const char *wetness = (newspe < 3)
                                     ? (!obj->spe ? "damp" : "damper")
                                     : (!obj->spe ? "wet" : "wetter");

            if (carried(obj))
                pline("%s gets %s.", Yobjnam2(obj, (const char *) 0),
                      wetness);
            else if (mcarried(obj) && canseemon(obj->ocarry))
                pline("%s %s gets %s.", s_suffix(Monnam(obj->ocarry)),
                      xname(obj), wetness);
        }
    }

    if (newspe != obj->spe)
        finish_towel_change(obj, newspe);
}

/* decrease a towel's wetness; unlike when wetting, 0 is not a no-op */
void
dry_a_towel(struct obj *obj,
            int amt, /* positive or zero: new value;
                        negative: decrement by abs(amt) */
            boolean verbose)
{
    int newspe = (amt < 0) ? obj->spe + amt : amt;

    /* new state is only reported if it's a decrease */
    if (newspe < obj->spe) {
        if (verbose) {
            if (carried(obj))
                pline("%s dries%s.", Yobjnam2(obj, (const char *) 0),
                      !newspe ? " out" : "");
            else if (mcarried(obj) && canseemon(obj->ocarry))
                pline("%s %s drie%s.", s_suffix(Monnam(obj->ocarry)),
                      xname(obj), !newspe ? " out" : "");
        }
    }

    if (newspe != obj->spe)
        finish_towel_change(obj, newspe);
}

/* Express progress of training of a skill as a percentage, where every 100%
 * represents a full level of possible enhancement, e.g. a basic skill that
 * returns 150% for this means it can be advanced to skilled and is 50% of the
 * way to expert. */
static int
skill_training_percent(int skill)
{
    int percent = 0;
    int i;

    if (P_RESTRICTED(skill))
	    return 0;

    if (skill >= P_FIRST_ROLE)
        return 100;

    for (i = P_SKILL(skill); i < P_MAX_SKILL(skill); i++) {
        if (P_ADVANCE(skill) >= practice_needed_to_advance(i)) {
            percent += 100;
        } else {
            int mintrain = (i == P_UNSKILLED) ? 0 :
                practice_needed_to_advance(i - 1);
            int partial = (P_ADVANCE(skill) - mintrain) * 100 /
                (practice_needed_to_advance(i) - mintrain);
            percent += min(partial, 100);
            break;
        }
    }

    return percent;
}

/* copy the skill level name into the given buffer */
char *
skill_level_name(int skill, char *buf, boolean max)
{
    const char *ptr;

    switch (max ? P_MAX_SKILL(skill) : P_SKILL(skill)) {
    case P_UNSKILLED:
        ptr = "Unskilled";
        break;
    case P_BASIC:
        ptr = "Basic";
        break;
    case P_SKILLED:
        ptr = "Skilled";
        break;
    case P_EXPERT:
        ptr = "Expert";
        break;
    /* these are for unarmed combat/martial arts only */
    case P_MASTER:
        ptr = "Master";
        break;
    case P_GRAND_MASTER:
        ptr = "Grand Master";
        break;
    default:
        ptr = "Restricted";
        break;
    }
    Strcpy(buf, ptr);
    return buf;
}

const char *
skill_name(int skill)
{
    return P_NAME(skill);
}

/* return the # of slots required to advance the skill */
static int
slots_required(int skill)
{
    int tmp = P_SKILL(skill);

    /* The more difficult the training, the more slots it takes.
     *  unskilled -> basic      1
     *  basic -> skilled        2
     *  skilled -> expert       3
     */
    if (skill <= P_LAST_WEAPON || skill == P_TWO_WEAPON_COMBAT)
        return tmp;

    /* Fewer slots used up for unarmed or martial.
     *  unskilled -> basic      1
     *  basic -> skilled        1
     *  skilled -> expert       2
     *  expert -> master        2
     *  master -> grand master  3
     */
    return (tmp + 1) / 2;
}

/* return true if this skill can be advanced */
boolean
can_advance(int skill, boolean speedy)
{
    if (P_RESTRICTED(skill)
        || P_SKILL(skill) >= P_MAX_SKILL(skill)
        || u.skills_advanced >= P_SKILL_LIMIT)
        return FALSE;

    if (wizard && speedy)
        return TRUE;

    return (boolean) ((int) P_ADVANCE(skill)
                      >= practice_needed_to_advance(P_SKILL(skill))
                      && u.weapon_slots >= slots_required(skill));
}

/* return true if this skill could be advanced if more slots were available */
static boolean
could_advance(int skill)
{
    if (P_RESTRICTED(skill)
        || P_SKILL(skill) >= P_MAX_SKILL(skill)
        || u.skills_advanced >= P_SKILL_LIMIT)
        return FALSE;

    return (boolean) ((int) P_ADVANCE(skill)
                      >= practice_needed_to_advance(P_SKILL(skill)));
}

/* return true if this skill has reached its maximum and there's been enough
   practice to become eligible for the next step if that had been possible */
static boolean
peaked_skill(int skill)
{
    if (P_RESTRICTED(skill))
        return FALSE;

    return (boolean) (P_SKILL(skill) >= P_MAX_SKILL(skill)
                      && ((int) P_ADVANCE(skill)
                          >= practice_needed_to_advance(P_SKILL(skill))));
}

static void
skill_advance(int skill)
{
    u.weapon_slots -= slots_required(skill);
    P_SKILL(skill)++;
    u.skill_record[u.skills_advanced++] = skill;
    /* subtly change the advance message to indicate no more advancement */
    You("are now %s skilled in %s.",
        P_SKILL(skill) >= P_MAX_SKILL(skill) ? "most" : "more",
        P_NAME(skill));
    /* botl and inv will need update because of to-hit bonus */
    update_inventory();
    g.context.botl = 1;
}

static const struct skill_range {
    short first, last;
    const char *name;
} skill_ranges[] = {
    { P_FIRST_H_TO_H, P_LAST_H_TO_H, "Fighting Skills" },
    { P_FIRST_WEAPON, P_LAST_WEAPON, "Weapon Skills" },
    { P_FIRST_SPELL, P_LAST_SPELL, "Spellcasting Skills" },
    { P_FIRST_ROLE, P_LAST_ROLE, "Role-Specific Skills" },
    { P_FIRST_RACE, P_LAST_RACE, "Cultural Skills" }
};

/*
 * The `#enhance' extended command.  What we _really_ would like is
 * to keep being able to pick things to advance until we couldn't any
 * more.  This is currently not possible -- the menu code has no way
 * to call us back for instant action.  Even if it did, we would also need
 * to be able to update the menu since selecting one item could make
 * others unselectable.
 */
int
enhance_weapon_skill(void)
{
    int pass, i, n, len, longest, to_advance, eventually_advance, maxxed_cnt;
    char buf[BUFSZ], sklnambuf[BUFSZ], percentbuf[BUFSZ];
    char sklmaxnambuf[80];
    const char *prefix;
    menu_item *selected;
    anything any;
    winid win;
    boolean speedy = FALSE;

    /* player knows about #enhance, don't show tip anymore */
    g.context.enhance_tip = TRUE;

    if (wizard && yn("Advance skills without practice?") == 'y')
        speedy = TRUE;

    do {
        /* find longest available skill name, count those that can advance */
        to_advance = eventually_advance = maxxed_cnt = 0;
        for (longest = 0, i = 0; i < P_NUM_SKILLS; i++) {
            if (P_RESTRICTED(i))
                continue;
            if ((len = strlen(P_NAME(i))) > longest)
                longest = len;
            if (can_advance(i, speedy))
                to_advance++;
            else if (could_advance(i))
                eventually_advance++;
            else if (peaked_skill(i))
                maxxed_cnt++;
        }

        win = create_nhwindow(NHW_MENU);
        start_menu(win, MENU_BEHAVE_STANDARD);

        /* start with a legend if any entries will be annotated
           with "*" or "#" below */
        if (eventually_advance > 0 || maxxed_cnt > 0) {
            any = cg.zeroany;
            if (eventually_advance > 0) {
                Sprintf(buf, "(Skill%s flagged by \"*\" may be enhanced %s.)",
                        plur(eventually_advance),
                        (u.ulevel < MAXULEV)
                            ? "when you're more experienced"
                            : "if skill slots become available");
                add_menu(win, &nul_glyphinfo, &any, 0, 0,
                         ATR_NONE, buf, MENU_ITEMFLAGS_NONE);
            }
            if (maxxed_cnt > 0) {
                Sprintf(buf,
                 "(Skill%s flagged by \"#\" cannot be enhanced any further.)",
                        plur(maxxed_cnt));
                add_menu(win, &nul_glyphinfo, &any, 0, 0,
                         ATR_NONE, buf, MENU_ITEMFLAGS_NONE);
            }
            add_menu(win, &nul_glyphinfo, &any, 0, 0,
                     ATR_NONE, "", MENU_ITEMFLAGS_NONE);
        }

        /* List the skills, making ones that could be advanced
           selectable.  List the miscellaneous skills first.
           Possible future enhancement:  list spell skills before
           weapon skills for spellcaster roles. */
        for (pass = 0; pass < SIZE(skill_ranges); pass++)
            for (i = skill_ranges[pass].first; i <= skill_ranges[pass].last;
                 i++) {
                /* Print headings for skill types */
                any = cg.zeroany;
                if (i == skill_ranges[pass].first)
                    add_menu(win, &nul_glyphinfo, &any, 0, 0,
                             iflags.menu_headings,
                             skill_ranges[pass].name, MENU_ITEMFLAGS_NONE);

                if (P_RESTRICTED(i) && P_SKILL(i) <= P_UNSKILLED)
                    continue;
                /*
                 * Sigh, this assumes a monospaced font unless
                 * iflags.menu_tab_sep is set in which case it puts
                 * tabs between columns.
                 * The 12 is the longest skill level name.
                 * The "    " is room for a selection letter and dash, "a - ".
                 */
                if (can_advance(i, speedy))
                    prefix = ""; /* will be preceded by menu choice */
                else if (could_advance(i))
                    prefix = "  * ";
                else if (peaked_skill(i))
                    prefix = "  # ";
                else
                    prefix =
                        (to_advance + eventually_advance + maxxed_cnt > 0)
                            ? "    "
                            : "";
                (void) skill_level_name(i, sklnambuf, FALSE);
                (void) skill_level_name(i, sklmaxnambuf, TRUE);

                int percent = skill_training_percent(i);
                Sprintf(percentbuf, "%5d%%", skill_training_percent(i));
                boolean maxed = (P_SKILL(i) == P_MAX_SKILL(i));
                if ((P_SKILL(i) + (percent / 100)) == P_MAX_SKILL(i))
                    maxed = TRUE;

                if (wizard) {
                    if (!iflags.menu_tab_sep)
                        Snprintf(buf, sizeof(buf), " %s%-*s %-12s %5d(%4d)", prefix,
                                longest, P_NAME(i), sklnambuf, P_ADVANCE(i),
                                practice_needed_to_advance(P_SKILL(i)));
                    else
                        Snprintf(buf, sizeof(buf), " %s%s\t%s\t%5d(%4d)", prefix, P_NAME(i),
                                sklnambuf, P_ADVANCE(i),
                                practice_needed_to_advance(P_SKILL(i)));
                } else {
                    if (!iflags.menu_tab_sep)
                        Snprintf(buf, sizeof(buf), " %s %-*s [%s/%s]%s", prefix, longest,
                                P_NAME(i), sklnambuf, sklmaxnambuf, maxed ? " MAX" : !percent ? " " : percentbuf);
                    else
                        Snprintf(buf, sizeof(buf), " %s%s\t[%s/%s]\t%s", prefix, P_NAME(i),
                                    sklnambuf,
                                    sklmaxnambuf,
                                    maxed ? "   MAX" :
                                    !percent ? "      " : percentbuf);
                }
                any.a_int = can_advance(i, speedy) ? i + 1 : 0;
                add_menu(win, &nul_glyphinfo, &any, 0, 0,
                         ATR_NONE, buf, MENU_ITEMFLAGS_NONE);
            }

        Strcpy(buf, (to_advance > 0) ? "Pick a skill to advance:"
                                     : "Current skills:");
        if (!speedy)
            Sprintf(eos(buf), "  (%d slot%s available)", u.weapon_slots,
                    plur(u.weapon_slots));
        end_menu(win, buf);
        n = select_menu(win, to_advance ? PICK_ONE : PICK_NONE, &selected);
        destroy_nhwindow(win);
        if (n > 0) {
            n = selected[0].item.a_int - 1; /* get item selected */
            free((genericptr_t) selected);
            /* display database info and ask for confirmation */
            display_skill_db_info(n);
            if (yn("Enhance this skill?") == 'y') {
                skill_advance(n);
            } else {
                break;
            }
            /* check for more skills able to advance, if so then .. */
            for (n = i = 0; i < P_NUM_SKILLS; i++) {
                if (can_advance(i, speedy)) {
                    if (!speedy)
                        You_feel("you could be more dangerous!");
                    n++;
                    break;
                }
            }
        }
    } while (speedy && n > 0);
    return 0;
}

/*
 * Change from restricted to unrestricted, allowing P_BASIC as max.  This
 * function may be called with with P_NONE.  Used in pray.c as well as below.
 */
void
unrestrict_weapon_skill(int skill)
{
    if (skill < P_NUM_SKILLS && P_RESTRICTED(skill)) {
        P_SKILL(skill) = P_UNSKILLED;
        P_MAX_SKILL(skill) = P_BASIC;
        P_ADVANCE(skill) = 0;
        P_RESTRICTED(skill) = FALSE;
    }
}

void
use_skill(int skill, int degree)
{
    boolean advance_before;

    if (skill != P_NONE && !P_RESTRICTED(skill)) {
        advance_before = can_advance(skill, FALSE);
        P_ADVANCE(skill) += degree;
        if (!advance_before && can_advance(skill, FALSE))
            give_may_advance_msg(skill);
    }
}

void
add_weapon_skill(int n) /* number of slots to gain; normally one */
{
    int i, before, after;

    for (i = 0, before = 0; i < P_NUM_SKILLS; i++)
        if (can_advance(i, FALSE))
            before++;
    u.weapon_slots += n;
    for (i = 0, after = 0; i < P_NUM_SKILLS; i++)
        if (can_advance(i, FALSE))
            after++;
    if (before < after)
        give_may_advance_msg(P_NONE);
}

void
lose_weapon_skill(int n) /* number of slots to lose; normally one */
{
    int skill;

    while (--n >= 0) {
        /* deduct first from unused slots then from last placed one, if any */
        if (u.weapon_slots) {
            u.weapon_slots--;
        } else if (u.skills_advanced) {
            skill = u.skill_record[--u.skills_advanced];
            if (P_SKILL(skill) <= P_UNSKILLED)
                panic("lose_weapon_skill (%d)", skill);
            P_SKILL(skill)--; /* drop skill one level */
            /* Lost skill might have taken more than one slot; refund rest. */
            u.weapon_slots = slots_required(skill) - 1;
            /* It might now be possible to advance some other pending
               skill by using the refunded slots, but giving a message
               to that effect would seem pretty confusing.... */
        }
    }
}

void
drain_weapon_skill(int n) /* number of skills to drain */
{
    int skill;
    int i;
    int tmpskills[P_NUM_SKILLS];

    (void) memset((genericptr_t) tmpskills, 0, sizeof(tmpskills));

    while (--n >= 0) {
        if (u.skills_advanced) {
            /* Pick a random skill, deleting it from the list. */
            i = rn2(u.skills_advanced);
            skill = u.skill_record[i];
            tmpskills[skill] = 1;
            for (; i < u.skills_advanced - 1; i++) {
                u.skill_record[i] = u.skill_record[i + 1];
            }
            u.skills_advanced--;
            if (P_SKILL(skill) <= P_UNSKILLED)
                panic("drain_weapon_skill (%d)", skill);
            P_SKILL(skill)--;   /* drop skill one level */
            /* refund slots used for skill */
            u.weapon_slots += slots_required(skill);
            /* drain a random proportion of skill training */
            if (P_ADVANCE(skill))
                P_ADVANCE(skill) = rn2(P_ADVANCE(skill));
        }
    }

    for (skill = 0; skill < P_NUM_SKILLS; skill++)
        if (tmpskills[skill]) {
            You("forget %syour training in %s.",
                P_SKILL(skill) >= P_BASIC ? "some of " : "", P_NAME(skill));
        }
}

int
weapon_type(struct obj *obj)
{
    /* KMH -- now uses the object table */
    int type;

    if (!obj)
        return P_BARE_HANDED_COMBAT; /* Not using a weapon */
    if ((obj->otyp == HEAVY_IRON_BALL) && Role_if(PM_CONVICT))
        return objects[obj->otyp].oc_skill;
    if (obj->oclass != WEAPON_CLASS && obj->oclass != TOOL_CLASS
        && obj->oclass != GEM_CLASS)
        return P_NONE; /* Not a weapon, weapon-tool, or ammo */
    type = objects[obj->otyp].oc_skill;
    return (type < 0) ? -type : type;
}

int
uwep_skill_type(void)
{
    if (u.twoweap)
        return P_TWO_WEAPON_COMBAT;
    return weapon_type(uwep);
}

/*
 * Return hit bonus/penalty based on skill of weapon.
 * Treat restricted weapons as unskilled.
 */
int
weapon_hit_bonus(struct obj *weapon)
{
    int type, wep_type, skill, bonus = 0;
    static const char bad_skill[] = "weapon_hit_bonus: bad skill %d";

    wep_type = weapon_type(weapon);
    /* use two weapon skill only if attacking with one of the wielded weapons
     */
    type = (u.twoweap && (weapon == uwep || weapon == uswapwep))
               ? P_TWO_WEAPON_COMBAT
               : wep_type;
    if (type == P_NONE) {
        bonus = 0;
    } else if (type <= P_LAST_WEAPON) {
        switch (P_SKILL(type)) {
        default:
            impossible(bad_skill, P_SKILL(type)); /* fall through */
        case P_ISRESTRICTED:
        case P_UNSKILLED:
            bonus = -4;
            break;
        case P_BASIC:
            bonus = 0;
            break;
        case P_SKILLED:
            bonus = 2;
            break;
        case P_EXPERT:
            bonus = 3;
            break;
        }
    } else if (type == P_TWO_WEAPON_COMBAT) {
        skill = P_SKILL(P_TWO_WEAPON_COMBAT);
        if (P_SKILL(wep_type) < skill)
            skill = P_SKILL(wep_type);
        switch (skill) {
        default:
            impossible(bad_skill, skill); /* fall through */
        case P_ISRESTRICTED:
        case P_UNSKILLED:
            bonus = -9;
            break;
        case P_BASIC:
            bonus = -7;
            break;
        case P_SKILLED:
            bonus = -5;
            break;
        case P_EXPERT:
            bonus = -3;
            break;
        }
    } else if (type == P_BARE_HANDED_COMBAT) {
        /*
         *        b.h. m.a.
         * unskl:  +1  n/a
         * basic:  +1   +3
         * skild:  +2   +4
         * exprt:  +2   +5
         * mastr:  +3   +6
         * grand:  +3   +7
         */
        bonus = P_SKILL(type);
        bonus = max(bonus, P_UNSKILLED) - 1; /* unskilled => 0 */
        bonus = ((bonus + 2) * (martial_bonus() ? 2 : 1)) / 2;
    }

    /* KMH -- It's harder to hit while you are riding */
    if (u.usteed) {
        switch (P_SKILL(P_RIDING)) {
        case P_ISRESTRICTED:
        case P_UNSKILLED:
            bonus -= 2;
            break;
        case P_BASIC:
            bonus -= 1;
            break;
        case P_SKILLED:
            break;
        case P_EXPERT:
            break;
        }
        if (u.twoweap)
            bonus -= 2;
    }

    /* handle the to-hits of Luckless Folly */
    if (weapon && weapon->oartifact &&
        weapon->oartifact == ART_LUCKLESS_FOLLY) {
        bonus += 2 * Luck;
    }

    return bonus;
}

/*
 * Return damage bonus/penalty based on skill of weapon.
 * Treat restricted weapons as unskilled.
 */
int
weapon_dam_bonus(struct obj *weapon)
{
    int type, wep_type, skill, bonus = 0;

    wep_type = weapon_type(weapon);
    /* use two weapon skill only if attacking with one of the wielded weapons
     */
    type = (u.twoweap && (weapon == uwep || weapon == uswapwep))
               ? P_TWO_WEAPON_COMBAT
               : wep_type;
    if (type == P_NONE) {
        bonus = 0;
    } else if (type <= P_LAST_WEAPON) {
        switch (P_SKILL(type)) {
        default:
            impossible("weapon_dam_bonus: bad skill %d", P_SKILL(type));
        /* fall through */
        case P_ISRESTRICTED:
        case P_UNSKILLED:
            bonus = -2;
            break;
        case P_BASIC:
            bonus = 0;
            break;
        case P_SKILLED:
            bonus = 1;
            break;
        case P_EXPERT:
            bonus = 2;
            break;
        }
    } else if (type == P_TWO_WEAPON_COMBAT) {
        skill = P_SKILL(P_TWO_WEAPON_COMBAT);
        if (P_SKILL(wep_type) < skill)
            skill = P_SKILL(wep_type);
        switch (skill) {
        default:
        case P_ISRESTRICTED:
        case P_UNSKILLED:
            bonus = -3;
            break;
        case P_BASIC:
            bonus = -1;
            break;
        case P_SKILLED:
            bonus = 0;
            break;
        case P_EXPERT:
            bonus = 1;
            break;
        }
    } else if (type == P_BARE_HANDED_COMBAT) {
        /*
         *        b.h. m.a.
         * unskl:   0  n/a
         * basic:  +1   +3
         * skild:  +1   +4
         * exprt:  +2   +6
         * mastr:  +2   +7
         * grand:  +3   +9
         */
        bonus = P_SKILL(type);
        bonus = max(bonus, P_UNSKILLED) - 1; /* unskilled => 0 */
        bonus = ((bonus + 1) * (martial_bonus() ? 3 : 1)) / 2;
    }

    /* KMH -- Riding gives some thrusting damage */
    if (u.usteed && type != P_TWO_WEAPON_COMBAT) {
        switch (P_SKILL(P_RIDING)) {
        case P_ISRESTRICTED:
        case P_UNSKILLED:
            break;
        case P_BASIC:
            break;
        case P_SKILLED:
            bonus += 1;
            break;
        case P_EXPERT:
            bonus += 2;
            break;
        }
    }

    return bonus;
}

/*
 * Initialize weapon skill array for the game.  Start by setting all
 * skills to restricted, then set the skill for every weapon the
 * hero is holding, finally reading the given array that sets
 * maximums.
 */
void
skill_init(const struct def_skill *class_skill)
{
    struct obj *obj;
    int skmax, skill;

    /* initialize skill array; by default, everything is restricted */
    for (skill = 0; skill < P_NUM_SKILLS; skill++) {
        P_SKILL(skill) = P_ISRESTRICTED;
        P_MAX_SKILL(skill) = P_ISRESTRICTED;
        P_ADVANCE(skill) = 0;
        P_RESTRICTED(skill) = TRUE;
    }

    /* Set skill for all weapons in inventory to be basic */
    for (obj = g.invent; obj; obj = obj->nobj) {
        /* don't give skill just because of carried ammo, wait until
           we see the relevant launcher (prevents an archeologist's
           touchstone from inadvertently providing skill in sling) */
        if (is_ammo(obj))
            continue;

        skill = weapon_type(obj);
        if (skill != P_NONE) {
            P_SKILL(skill) = P_BASIC;
            P_RESTRICTED(skill) = FALSE;
        }
    }

    /* set skills for magic */
    if (Role_if(PM_HEALER) || Role_if(PM_MONK)) {
        P_SKILL(P_HEALING_SPELL) = P_BASIC;
        P_RESTRICTED(P_HEALING_SPELL) = FALSE;
    } else if (Role_if(PM_CLERIC)) {
        P_SKILL(P_CLERIC_SPELL) = P_BASIC;
        P_RESTRICTED(P_CLERIC_SPELL) = FALSE;
    } else if (Role_if(PM_WIZARD)) {
        P_SKILL(P_ATTACK_SPELL) = P_BASIC;
        P_SKILL(P_ENCHANTMENT_SPELL) = P_BASIC;
        P_RESTRICTED(P_ATTACK_SPELL) = FALSE;
        P_RESTRICTED(P_ENCHANTMENT_SPELL) = FALSE;
    }

    /* walk through array to set skill maximums */
    for (; class_skill->skill != P_NONE; class_skill++) {
        skmax = class_skill->skmax;
        skill = class_skill->skill;

        P_MAX_SKILL(skill) = skmax;
        if (P_SKILL(skill) == P_ISRESTRICTED) { /* skill pre-set */
            P_SKILL(skill) = P_UNSKILLED;
            P_RESTRICTED(skill) = FALSE;
        }
    }

    /* High potential fighters already know how to use their hands. */
    if (P_MAX_SKILL(P_BARE_HANDED_COMBAT) > P_EXPERT) {
        P_SKILL(P_BARE_HANDED_COMBAT) = P_BASIC;
        P_RESTRICTED(skill) = FALSE;
    }

    /* Roles that start with a mount know how to ride it */
    if (g.urole.petnum == PM_PONY || Role_if(PM_DRAGON_RIDER)) {
        P_SKILL(P_RIDING) = P_BASIC;
        P_RESTRICTED(skill) = TRUE;
    }

    /* All merfolk can use tridents */
    if (Race_if(PM_MERFOLK)) {
        P_SKILL(P_TRIDENT) = P_BASIC;
        P_MAX_SKILL(P_TRIDENT) = P_EXPERT;
        P_RESTRICTED(P_TRIDENT) = FALSE;
    }

    /*
     * Make sure we haven't missed setting the max on a skill
     * & set advance
     */
    for (skill = 0; skill < P_NUM_SKILLS; skill++) {
        if (!P_RESTRICTED(skill)) {
            if (P_MAX_SKILL(skill) < P_SKILL(skill)) {
                impossible("skill_init: curr > max: %s", P_NAME(skill));
                P_MAX_SKILL(skill) = P_SKILL(skill);
            }
            P_ADVANCE(skill) = practice_needed_to_advance(P_SKILL(skill) - 1);
        }
    }

    /* each role has a special spell; allow at least basic for its type
       (despite the function name, this works for spell skills too) */
    unrestrict_weapon_skill(spell_skilltype(g.urole.spelspec));
}

void
setmnotwielded(struct monst *mon, struct obj *obj)
{
    if (!obj)
        return;
    if (artifact_light(obj) && obj->lamplit) {
        end_burn(obj, FALSE);
        if (canseemon(mon))
            pline("%s in %s %s %s shining.", The(xname(obj)),
                  s_suffix(mon_nam(mon)), mbodypart(mon, HAND),
                  otense(obj, "stop"));
    }
    if (MON_WEP(mon) == obj)
        MON_NOWEP(mon);
    obj->owornmask &= ~W_WEP;
}

/*weapon.c*/
