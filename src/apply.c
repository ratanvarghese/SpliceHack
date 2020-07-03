/* NetHack 3.6	apply.c	$NHDT-Date: 1582155875 2020/02/19 23:44:35 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.318 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

/* Edited on 5/19/18 by NullCGT */

#include "hack.h"

static int FDECL(use_camera, (struct obj *));
static int FDECL(use_towel, (struct obj *));
static boolean FDECL(its_dead, (int, int, int *));
static int FDECL(use_stethoscope, (struct obj *));
static void FDECL(use_whistle, (struct obj *));
static void FDECL(use_magic_whistle, (struct obj *));
static int FDECL(use_leash, (struct obj *));
static int FDECL(use_mirror, (struct obj *));
static void FDECL(use_bell, (struct obj **));
static void FDECL(use_candelabrum, (struct obj *));
static void FDECL(use_candle, (struct obj **));
static void FDECL(use_lamp, (struct obj *));
static void FDECL(light_cocktail, (struct obj **));
static void FDECL(display_jump_positions, (int));
static void FDECL(use_tinning_kit, (struct obj *));
static void FDECL(use_figurine, (struct obj **));
static void FDECL(use_grease, (struct obj *));
static void FDECL(use_trap, (struct obj *));
static void FDECL(use_stone, (struct obj *));
static int NDECL(set_trap); /* occupation callback */
static int FDECL(use_whip, (struct obj *));
static void FDECL(display_polearm_positions, (int));
static int FDECL(use_pole, (struct obj *));
static int FDECL(use_cream_pie, (struct obj *));
static int FDECL(use_royal_jelly, (struct obj *));
static int FDECL(use_grapple, (struct obj *));
static int FDECL(do_break_wand, (struct obj *));
static int FDECL(flip_through_book, (struct obj *));
static boolean FDECL(figurine_location_checks, (struct obj *,
                                                    coord *, BOOLEAN_P));
static void FDECL(add_class, (char *, CHAR_P));
static void FDECL(setapplyclasses, (char *));
static boolean FDECL(check_jump, (genericptr_t, int, int));
static boolean FDECL(is_valid_jump_pos, (int, int, int, BOOLEAN_P));
static boolean FDECL(get_valid_jump_position, (int, int));
static boolean FDECL(get_valid_polearm_position, (int, int));
static boolean FDECL(find_poleable_mon, (coord *, int, int));

static const char no_elbow_room[] =
    "don't have enough elbow-room to maneuver.";

static int
use_camera(obj)
struct obj *obj;
{
    struct monst *mtmp;

    if (Underwater) {
        pline("Using your camera underwater would void the warranty.");
        return 0;
    }
    if (!getdir((char *) 0))
        return 0;

    if (obj->spe <= 0) {
        pline1(nothing_happens);
        return 1;
    }
    consume_obj_charge(obj, TRUE);

    if (obj->cursed && !rn2(2)) {
        (void) zapyourself(obj, TRUE);
    } else if (u.uswallow) {
        You("take a picture of %s %s.", s_suffix(mon_nam(u.ustuck)),
            mbodypart(u.ustuck, STOMACH));
    } else if (u.dz) {
        You("take a picture of the %s.",
            (u.dz > 0) ? surface(u.ux, u.uy) : ceiling(u.ux, u.uy));
    } else if (!u.dx && !u.dy) {
        (void) zapyourself(obj, TRUE);
    } else {
        mtmp = bhit(u.dx, u.dy, COLNO, FLASHED_LIGHT,
                    (int FDECL((*), (MONST_P, OBJ_P))) 0,
                    (int FDECL((*), (OBJ_P, OBJ_P))) 0, &obj);
        obj->ox = u.ux, obj->oy = u.uy; /* flash_hits_mon() wants this */
        if (mtmp)
            (void) flash_hits_mon(mtmp, obj);
        /* normally bhit() would do this but for FLASHED_LIGHT we want it
           to be deferred until after flash_hits_mon() */
        transient_light_cleanup();
    }
    return 1;
}

static int
use_towel(obj)
struct obj *obj;
{
    boolean drying_feedback = (obj == uwep);

    if (!freehand()) {
        You("have no free %s!", body_part(HAND));
        return 0;
    } else if (obj == ublindf) {
        You("cannot use it while you're wearing it!");
        return 0;
    } else if (obj->cursed) {
        long old;

        switch (rn2(3)) {
        case 2:
            old = (Glib & TIMEOUT);
            make_glib((int) old + rn1(10, 3)); /* + 3..12 */
            Your("%s %s!", makeplural(body_part(HAND)),
                 (old ? "are filthier than ever" : "get slimy"));
            if (is_wet_towel(obj))
                dry_a_towel(obj, -1, drying_feedback);
            return 1;
        case 1:
            if (!ublindf) {
                old = u.ucreamed;
                u.ucreamed += rn1(10, 3);
                pline("Yecch!  Your %s %s gunk on it!", body_part(FACE),
                      (old ? "has more" : "now has"));
                make_blinded(Blinded + (long) u.ucreamed - old, TRUE);
            } else {
                const char *what;

                what = (ublindf->otyp == LENSES)
                           ? "lenses"
                           :(ublindf->otyp == MASK)
                             ? "mask"
                             : (obj->otyp == ublindf->otyp) ? "other towel"
                                                            : "blindfold";
                if (ublindf->cursed) {
                    You("push your %s %s.", what,
                        rn2(2) ? "cock-eyed" : "crooked");
                } else {
                    struct obj *saved_ublindf = ublindf;
                    You("push your %s off.", what);
                    Blindf_off(ublindf);
                    dropx(saved_ublindf);
                }
            }
            if (is_wet_towel(obj))
                dry_a_towel(obj, -1, drying_feedback);
            return 1;
        case 0:
            break;
        }
    }

    if (Glib) {
        make_glib(0);
        You("wipe off your %s.",
            !uarmg ? makeplural(body_part(HAND)) : gloves_simple_name(uarmg));
        if (is_wet_towel(obj))
            dry_a_towel(obj, -1, drying_feedback);
        return 1;
    } else if (u.ucreamed) {
        Blinded -= u.ucreamed;
        u.ucreamed = 0;
        if (!Blinded) {
            pline("You've got the glop off.");
            if (!gulp_blnd_check()) {
                Blinded = 1;
                make_blinded(0L, TRUE);
            }
        } else {
            Your("%s feels clean now.", body_part(FACE));
        }
        if (is_wet_towel(obj))
            dry_a_towel(obj, -1, drying_feedback);
        return 1;
    }

    Your("%s and %s are already clean.", body_part(FACE),
         makeplural(body_part(HAND)));

    return 0;
}

/* maybe give a stethoscope message based on floor objects */
static boolean
its_dead(rx, ry, resp)
int rx, ry, *resp;
{
    char buf[BUFSZ];
    boolean more_corpses;
    struct permonst *mptr;
    struct obj *corpse = sobj_at(CORPSE, rx, ry),
               *statue = sobj_at(STATUE, rx, ry);

    if (!can_reach_floor(TRUE)) { /* levitation or unskilled riding */
        corpse = 0;               /* can't reach corpse on floor */
        /* you can't reach tiny statues (even though you can fight
           tiny monsters while levitating--consistency, what's that?) */
        while (statue && mons[statue->corpsenm].msize == MZ_TINY)
            statue = nxtobj(statue, STATUE, TRUE);
    }
    /* when both corpse and statue are present, pick the uppermost one */
    if (corpse && statue) {
        if (nxtobj(statue, CORPSE, TRUE) == corpse)
            corpse = 0; /* corpse follows statue; ignore it */
        else
            statue = 0; /* corpse precedes statue; ignore statue */
    }
    more_corpses = (corpse && nxtobj(corpse, CORPSE, TRUE));

    /* additional stethoscope messages from jyoung@apanix.apana.org.au */
    if (!corpse && !statue) {
        ; /* nothing to do */

    } else if (Hallucination) {
        if (!corpse) {
            /* it's a statue */
            Strcpy(buf, "You're both stoned");
        } else if (corpse->quan == 1L && !more_corpses) {
            int gndr = 2; /* neuter: "it" */
            struct monst *mtmp = get_mtraits(corpse, FALSE);

            /* (most corpses don't retain the monster's sex, so
               we're usually forced to use generic pronoun here) */
            if (mtmp) {
                mptr = mtmp->data = &mons[mtmp->mnum];
                /* TRUE: override visibility check--it's not on the map */
                gndr = pronoun_gender(mtmp, PRONOUN_NO_IT);
            } else {
                mptr = &mons[corpse->corpsenm];
                if (is_female(mptr))
                    gndr = 1;
                else if (is_male(mptr))
                    gndr = 0;
            }
            Sprintf(buf, "%s's dead", genders[gndr].he); /* "he"/"she"/"it" */
            buf[0] = highc(buf[0]);
        } else { /* plural */
            Strcpy(buf, "They're dead");
        }
        /* variations on "He's dead, Jim." (Star Trek's Dr McCoy) */
        You_hear("a voice say, \"%s, Jim.\"", buf);
        *resp = 1;
        return TRUE;

    } else if (corpse) {
        boolean here = (rx == u.ux && ry == u.uy),
                one = (corpse->quan == 1L && !more_corpses), reviver = FALSE;
        int visglyph, corpseglyph;

        visglyph = glyph_at(rx, ry);
        corpseglyph = obj_to_glyph(corpse, rn2);

        if (Blind && (visglyph != corpseglyph))
            map_object(corpse, TRUE);

        if (Role_if(PM_HEALER)) {
            /* ok to reset `corpse' here; we're done with it */
            do {
                if (obj_has_timer(corpse, REVIVE_MON))
                    reviver = TRUE;
                else
                    corpse = nxtobj(corpse, CORPSE, TRUE);
            } while (corpse && !reviver);
        }
        You("determine that %s unfortunate being%s %s%s dead.",
            one ? (here ? "this" : "that") : (here ? "these" : "those"),
            one ? "" : "s", one ? "is" : "are", reviver ? " mostly" : "");
        return TRUE;

    } else { /* statue */
        const char *what, *how;

        mptr = &mons[statue->corpsenm];
        if (Blind) { /* ignore statue->dknown; it'll always be set */
            Sprintf(buf, "%s %s",
                    (rx == u.ux && ry == u.uy) ? "This" : "That",
                    humanoid(mptr) ? "person" : "creature");
            what = buf;
        } else {
            what = mptr->mname;
            if (!type_is_pname(mptr))
                what = The(what);
        }
        how = "fine";
        if (Role_if(PM_HEALER)) {
            struct trap *ttmp = t_at(rx, ry);

            if (ttmp && ttmp->ttyp == STATUE_TRAP)
                how = "extraordinary";
            else if (Has_contents(statue))
                how = "remarkable";
        }

        pline("%s is in %s health for a statue.", what, how);
        return TRUE;
    }
    return FALSE; /* no corpse or statue */
}

static const char hollow_str[] = "a hollow sound.  This must be a secret %s!";

void
use_deck(obj)
struct obj *obj;
{
    char buf[BUFSZ];
    long draws;
    int index, pm, n;
    boolean goodcards = FALSE;
    boolean badcards = FALSE;
    struct monst *mtmp;
    struct obj *otmp;

    if (Blind) {
        You("can't play cards in the dark!");
        return;
    }
    if (obj->blessed || Role_if(PM_CARTOMANCER)) {
        goodcards = TRUE;
    } else if (obj->cursed) {
        badcards = TRUE;
    }

    if (obj->otyp == PLAYING_CARD_DECK) {
        if ((badcards && Luck == 13) || Luck <= 0) {
            pline("You draw a hand of five cards. It's not very good...");
        } else if ((badcards && Luck >= 5) || Luck < 5) {
            pline("You draw a hand of five cards. Two pair!");
        } else if ((badcards && Luck > 0) || Luck < 13) {
            pline("You draw a hand of five cards. Full house!");
        } else if ((badcards && Luck <= 0) || Luck == 13) {
            pline("You draw a hand of five cards. Wow, a royal flush!");
        }
        /* if blessed, indicate the luck value directly. */
        if (goodcards && Luck > 0) {
            pline("You shuffle the deck %d times.", Luck);
        } else {
            pline("You don't bother shuffling the deck.");
        }
        return;
    }
    /* deck of fate */
    makeknown(obj->otyp);
    getlin("How many cards will you draw?", buf);
    if (sscanf(buf, "%ld", &draws) != 1)
        draws = 0L;
    if (draws > 5)
        draws = 5;
    if (strlen(buf) <= 0L || draws <= 0L) {
        pline("You decide not to try your luck.");
        pline("The pack of cards vanishes in a puff of smoke.");
        useup(obj);
        return;
    }
    pline("You begin to draw from the deck of fate...");
    /* It would make sense not to give messages if blind, but that would make
       this already long string of spaghetti code even longer :( */
    for ( ; draws > 0; draws--) {
        index = rnd(22);
        /* wishes and disasters can be modified through BCU */
        if (badcards && index > 1) {
          index--;
        } else if (goodcards && index < 22) {
          index++;
        }
        switch(index) {
            case 1:
                pline("You draw The Tower...");
                explode(u.ux, u.uy, 15, rnd(30), TOOL_CLASS, EXPL_MAGICAL);
                explode(u.ux, u.uy, 11, rnd(30), TOOL_CLASS, EXPL_FIERY);
                (void) cancel_monst(&g.youmonst, obj, TRUE, FALSE, TRUE);
                break;
            case 2:
                pline("You draw the Wheel of Fortune... Two cards flip out of the deck.");
                draws += 2;
                break;
            case 3:
                if (!Blind) {
                    pline("You draw The Devil... Moloch's visage on the card grins at you.");
                } else {
                    pline("You draw The Devil...");
                }
                if ((pm = dlord(A_NONE)) != NON_PM) {
                    mtmp = makemon(&mons[pm], u.ux, u.uy, NO_MM_FLAGS);
                    if (mtmp) pline("%s appears from a cloud of noxious smoke!", Monnam(mtmp));
                    else pline("Something stinks!");
                }
                draws = 0;
                break;
            case 4:
                pline("You draw The Fool...");
                (void) adjattrib(A_INT, -rnd(3), FALSE);
                (void) adjattrib(A_WIS, -rnd(3), FALSE);
                forget_objects(10);
                pline("You feel foolish!");
                break;
            case 5:
                pline("You draw Death...");
                makemon(&mons[PM_GRIM_REAPER], u.ux, u.uy, NO_MM_FLAGS);
                pline("The Grim Reaper gently sets their hand upon the deck, stopping your draws.");
                draws = 0;
                break;
            case 6:
                pline("You draw Judgement...");
                punish(obj);
                break;
            case 7:
                pline("You draw The Emperor...");
                pline("You feel worthless.");
                attrcurse();
                attrcurse();
                break;
            case 8:
                pline("You draw The Hermit...");
                level_tele();
                forget_map(ALL_MAP);
                forget_traps();
                aggravate();
                break;
            case 9:
                pline("You draw The Hanged Man...");
                pline("A hangman arrives!");
                mtmp = makemon(&mons[PM_ROPE_GOLEM], u.ux, u.uy, NO_MM_FLAGS);
                break;
            case 10:
                pline("You draw Justice...");
                pline("You are frozen by the power of Justice!");
                nomul(-(rn1(30, 20)));
                g.multi_reason = "frozen by fate";
                g.nomovemsg = You_can_move_again;
                break;
            case 11:
                /* traditionally a good card? */
                pline("You draw Temperance...");
                destroy_arm(some_armor(&g.youmonst));
                destroy_arm(some_armor(&g.youmonst));
                pline("That ought to teach you.");
                break;
            case 12:
                pline("You draw The Lovers!");
                for (n = 0; n < 2; n++) {
                    if (!rn2(2))
                        mtmp = makemon(&mons[PM_INCUBUS],
                                       u.ux, u.uy, NO_MM_FLAGS);
                    else
                        mtmp = makemon(&mons[PM_SUCCUBUS],
                                       u.ux, u.uy, NO_MM_FLAGS);
                    if (mtmp) mtmp->mpeaceful = 1;
                }
                if (!Deaf && mtmp) {
                    You_hear("infernal giggling.");
                }
                break;
            case 13:
                if (!Blind) {
                    pline("You draw the Magician! The figure on the card winks!");
                } else
                    pline("You draw the Magician!");
                u.uenmax += rn1(20,10);
                u.uen = u.uenmax;
                break;
            case 14:
                pline("You draw Strength!");
                (void) adjattrib(A_STR, rn1(5, 4), FALSE);
                You("feel impossibly strong!");
                break;
            case 15:
                pline("You draw The High Priestess! You feel more devout.");
                adjalign(10);
                break;
            case 16:
                pline("You draw The Hierophant!");
                if (levl[u.ux][u.uy].typ != STAIRS &&
                      levl[u.ux][u.uy].typ != LADDER &&
                      levl[u.ux][u.uy].typ != AIR) {
                    levl[u.ux][u.uy].typ = ALTAR;
                    pline("The %s beneath you twists reshapes itself into an altar!", surface(u.ux, u.uy));
                } else {
                    You("feel a twinge of anxiety.");
                }
                break;
            case 17:
                pline("You draw the Emperess! Your throne arrives.");
                if (levl[u.ux][u.uy].typ != STAIRS &&
                      levl[u.ux][u.uy].typ != LADDER &&
                      levl[u.ux][u.uy].typ != AIR) {
                    levl[u.ux][u.uy].typ = THRONE;
                } else {
                    You("feel quite lordly.");
                }
                break;
            case 18:
                pline("You draw The Chariot!");
                unrestrict_weapon_skill(P_RIDING);
                mtmp = makemon(&mons[PM_NIGHTMARE],
                               u.ux, u.uy, MM_EDOG);
                if (mtmp) {
                    (void) initedog(mtmp);
                    otmp = mksobj(SADDLE, FALSE, FALSE);
                    put_saddle_on_mon(otmp, mtmp);
                    Your("steed arrives!");
                } else {
                    pline("It is time to ride to victory!");
                }
                break;
            case 19:
                pline("You draw the Sun! You are bathed in warmth!");
                /* as praying */
                if (!(HProtection & INTRINSIC)) {
                    HProtection |= FROMOUTSIDE;
                    if (!u.ublessed)
                        u.ublessed = rn1(3, 2);
                } else
                    u.ublessed += 3;
                break;
            case 20:
                pline("You draw The Moon!");
                if (Luck > 0) {
                    otmp = mksobj(MOONSTONE, FALSE, FALSE);
                    pline(Blind ? "Something phases through your foot." 
                        : "An object shimmers into existence at your feet!");
                    dropy(otmp);
                } else {
                    change_luck(3);
                    pline("Your luck is beginning to change...");
                }
                break;
            case 21:
                pline("You draw the Star!");
                identify_pack(0, FALSE);
                break;
            case 22:
                pline("You draw The World!");
                makewish();
                break;
            default:
                pline("You draw the twenty-three of cups, apparently.");
        }
    }
    pline("The pack of cards vanishes in a puff of smoke.");
    useup(obj);
    return;
}

/* Strictly speaking it makes no sense for usage of a stethoscope to
   not take any time; however, unless it did, the stethoscope would be
   almost useless.  As a compromise, one use per turn is free, another
   uses up the turn; this makes curse status have a tangible effect. */
static int
use_stethoscope(obj)
register struct obj *obj;
{
    struct monst *mtmp;
    struct rm *lev;
    int rx, ry, res;
    boolean interference = (u.uswallow && is_whirly(u.ustuck->data)
                            && !rn2(Role_if(PM_HEALER) ? 10 : 3));

    if (nohands(g.youmonst.data)) {
        You("have no hands!"); /* not `body_part(HAND)' */
        return 0;
    } else if (Deaf) {
        You_cant("hear anything!");
        return 0;
    } else if (!freehand()) {
        You("have no free %s.", body_part(HAND));
        return 0;
    }
    if (!getdir((char *) 0))
        return 0;

    res = (g.moves == g.context.stethoscope_move)
          && (g.youmonst.movement == g.context.stethoscope_movement);
    g.context.stethoscope_move = g.moves;
    g.context.stethoscope_movement = g.youmonst.movement;

    g.bhitpos.x = u.ux, g.bhitpos.y = u.uy; /* tentative, reset below */
    g.notonhead = u.uswallow;
    if (u.usteed && u.dz > 0) {
        if (interference) {
            pline("%s interferes.", Monnam(u.ustuck));
            mstatusline(u.ustuck);
        } else
            mstatusline(u.usteed);
        return res;
    } else if (u.uswallow && (u.dx || u.dy || u.dz)) {
        mstatusline(u.ustuck);
        return res;
    } else if (u.uswallow && interference) {
        pline("%s interferes.", Monnam(u.ustuck));
        mstatusline(u.ustuck);
        return res;
    } else if (u.dz) {
        if (Underwater)
            You_hear("faint splashing.");
        else if (u.dz < 0 || !can_reach_floor(TRUE))
            cant_reach_floor(u.ux, u.uy, (u.dz < 0), TRUE);
        else if (its_dead(u.ux, u.uy, &res))
            ; /* message already given */
        else if (Is_stronghold(&u.uz))
            You_hear("the crackling of hellfire.");
        else
            pline_The("%s seems healthy enough.", surface(u.ux, u.uy));
        return res;
    } else if (obj->cursed && !rn2(2)) {
        You_hear("your heart beat.");
        return res;
    }
    if (Stunned || (Confusion && !rn2(5)))
        confdir();
    if (!u.dx && !u.dy) {
        ustatusline();
        return res;
    }
    rx = u.ux + u.dx;
    ry = u.uy + u.dy;
    if (!isok(rx, ry)) {
        You_hear("a faint typing noise.");
        return 0;
    }
    if ((mtmp = m_at(rx, ry)) != 0) {
        const char *mnm = x_monnam(mtmp, ARTICLE_A, (const char *) 0,
                                   SUPPRESS_IT | SUPPRESS_INVISIBLE, FALSE);

        /* g.bhitpos needed by mstatusline() iff mtmp is a long worm */
        g.bhitpos.x = rx, g.bhitpos.y = ry;
        g.notonhead = (mtmp->mx != rx || mtmp->my != ry);

        if (mtmp->mundetected) {
            if (!canspotmon(mtmp))
                There("is %s hidden there.", mnm);
            mtmp->mundetected = 0;
            newsym(mtmp->mx, mtmp->my);
        } else if (mtmp->mappearance) {
            const char *what = "thing";
            boolean use_plural = FALSE;
            struct obj dummyobj, *odummy;

            switch (M_AP_TYPE(mtmp)) {
            case M_AP_OBJECT:
                /* FIXME?
                 *  we should probably be using object_from_map() here
                 */
                odummy = init_dummyobj(&dummyobj, mtmp->mappearance, 1L);
                /* simple_typename() yields "fruit" for any named fruit;
                   we want the same thing '//' or ';' shows: "slime mold"
                   or "grape" or "slice of pizza" */
                if (odummy->otyp == SLIME_MOLD
                    && has_mcorpsenm(mtmp) && MCORPSENM(mtmp) != NON_PM) {
                    odummy->spe = MCORPSENM(mtmp);
                    what = simpleonames(odummy);
                } else {
                    what = simple_typename(odummy->otyp);
                }
                use_plural = (is_boots(odummy) || is_gloves(odummy)
                              || odummy->otyp == LENSES);
                break;
            case M_AP_MONSTER: /* ignore Hallucination here */
                what = mons[mtmp->mappearance].mname;
                break;
            case M_AP_FURNITURE:
                what = defsyms[mtmp->mappearance].explanation;
                break;
            }
            seemimic(mtmp);
            pline("%s %s %s really %s.",
                  use_plural ? "Those" : "That", what,
                  use_plural ? "are" : "is", mnm);
        } else if (flags.verbose && !canspotmon(mtmp)) {
            There("is %s there.", mnm);
        }

        mstatusline(mtmp);
        if (!canspotmon(mtmp))
            map_invisible(rx, ry);
        return res;
    }
    if (unmap_invisible(rx,ry))
        pline_The("invisible monster must have moved.");

    lev = &levl[rx][ry];
    switch (lev->typ) {
    case SDOOR:
        You_hear(hollow_str, "door");
        cvt_sdoor_to_door(lev); /* ->typ = DOOR */
        feel_newsym(rx, ry);
        return res;
    case SCORR:
        You_hear(hollow_str, "passage");
        lev->typ = CORR, lev->flags = 0;
        unblock_point(rx, ry);
        feel_newsym(rx, ry);
        return res;
    }

    if (!its_dead(rx, ry, &res))
        You("hear nothing special."); /* not You_hear()  */
    return res;
}

static const char whistle_str[] = "produce a %s whistling sound.",
                  alt_whistle_str[] = "produce a %s, sharp vibration.";

static void
use_whistle(obj)
struct obj *obj;
{
    if (!can_blow(&g.youmonst)) {
        You("are incapable of using the whistle.");
    } else if (Underwater) {
        You("blow bubbles through %s.", yname(obj));
    } else {
        if (Deaf)
            You_feel("rushing air tickle your %s.", body_part(NOSE));
        else
            You(whistle_str, obj->cursed ? "shrill" : "high");
        wake_nearby();
        if (obj->cursed)
            vault_summon_gd();
    }
}

static void
use_magic_whistle(obj)
struct obj *obj;
{
    register struct monst *mtmp, *nextmon;

    if (!can_blow(&g.youmonst)) {
        You("are incapable of using the whistle.");
    } else if (obj->cursed && !rn2(2)) {
        You("produce a %shigh-%s.", Underwater ? "very " : "",
            Deaf ? "frequency vibration" : "pitched humming noise");
        wake_nearby();
    } else {
        int pet_cnt = 0, omx, omy;

        /* it's magic!  it works underwater too (at a higher pitch) */
        You(Deaf ? alt_whistle_str : whistle_str,
            Hallucination ? "normal"
            : (Underwater && !Deaf) ? "strange, high-pitched"
              : "strange");
        for (mtmp = fmon; mtmp; mtmp = nextmon) {
            nextmon = mtmp->nmon; /* trap might kill mon */
            if (DEADMONSTER(mtmp))
                continue;
            /* steed is already at your location, so not affected;
               this avoids trap issues if you're on a trap location */
            if (mtmp == u.usteed)
                continue;
            if (mtmp->mtame) {
                if (mtmp->mtrapped) {
                    /* no longer in previous trap (affects mintrap) */
                    mtmp->mtrapped = 0;
                    fill_pit(mtmp->mx, mtmp->my);
                }
                /* mimic must be revealed before we know whether it
                   actually moves because line-of-sight may change */
                if (M_AP_TYPE(mtmp))
                    seemimic(mtmp);
                omx = mtmp->mx, omy = mtmp->my;
                mnexto(mtmp);
                if (mtmp->mx != omx || mtmp->my != omy) {
                    mtmp->mundetected = 0; /* reveal non-mimic hider */
                    if (canspotmon(mtmp))
                        ++pet_cnt;
                    if (mintrap(mtmp) == 2)
                        change_luck(-1);
                }
            }
        }
        if (pet_cnt > 0)
            makeknown(obj->otyp);
    }
}

boolean
um_dist(x, y, n)
xchar x, y, n;
{
    return (boolean) (abs(u.ux - x) > n || abs(u.uy - y) > n);
}

int
number_leashed()
{
    int i = 0;
    struct obj *obj;

    for (obj = g.invent; obj; obj = obj->nobj)
        if (obj->otyp == LEASH && obj->leashmon != 0)
            i++;
    return i;
}

/* otmp is about to be destroyed or stolen */
void
o_unleash(otmp)
register struct obj *otmp;
{
    register struct monst *mtmp;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
        if (mtmp->m_id == (unsigned) otmp->leashmon)
            mtmp->mleashed = 0;
    otmp->leashmon = 0;
}

/* mtmp is about to die, or become untame */
void
m_unleash(mtmp, feedback)
register struct monst *mtmp;
boolean feedback;
{
    register struct obj *otmp;

    if (feedback) {
        if (canseemon(mtmp))
            pline("%s pulls free of %s leash!", Monnam(mtmp), mhis(mtmp));
        else
            Your("leash falls slack.");
    }
    for (otmp = g.invent; otmp; otmp = otmp->nobj)
        if (otmp->otyp == LEASH && otmp->leashmon == (int) mtmp->m_id)
            otmp->leashmon = 0;
    mtmp->mleashed = 0;
}

/* player is about to die (for bones) */
void
unleash_all()
{
    register struct obj *otmp;
    register struct monst *mtmp;

    for (otmp = g.invent; otmp; otmp = otmp->nobj)
        if (otmp->otyp == LEASH)
            otmp->leashmon = 0;
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
        mtmp->mleashed = 0;
}

#define MAXLEASHED 2

boolean
leashable(mtmp)
struct monst *mtmp;
{
    return (boolean) (mtmp->mnum != PM_LONG_WORM
                       && !unsolid(mtmp->data)
                       && (!nolimbs(mtmp->data) || has_head(mtmp->data)));
}

/* ARGSUSED */
static int
use_leash(obj)
struct obj *obj;
{
    coord cc;
    struct monst *mtmp;
    int spotmon;

    if (u.uswallow) {
        /* if the leash isn't in use, assume we're trying to leash
           the engulfer; if it is use, distinguish between removing
           it from the engulfer versus from some other creature
           (note: the two in-use cases can't actually occur; all
           leashes are released when the hero gets engulfed) */
        You_cant((!obj->leashmon
                  ? "leash %s from inside."
                  : (obj->leashmon == (int) u.ustuck->m_id)
                    ? "unleash %s from inside."
                    : "unleash anything from inside %s."),
                 noit_mon_nam(u.ustuck));
        return 0;
    }
    if (!obj->leashmon && number_leashed() >= MAXLEASHED) {
        You("cannot leash any more pets.");
        return 0;
    }

    if (!get_adjacent_loc((char *) 0, (char *) 0, u.ux, u.uy, &cc))
        return 0;

    if (cc.x == u.ux && cc.y == u.uy) {
        if (u.usteed && u.dz > 0) {
            mtmp = u.usteed;
            spotmon = 1;
            goto got_target;
        }
        pline("Leash yourself?  Very funny...");
        return 0;
    }

    /*
     * From here on out, return value is 1 == a move is used.
     */

    if (!(mtmp = m_at(cc.x, cc.y))) {
        There("is no creature there.");
        (void) unmap_invisible(cc.x, cc.y);
        return 1;
    }

    spotmon = canspotmon(mtmp);
 got_target:

    if (!spotmon && !glyph_is_invisible(levl[cc.x][cc.y].glyph)) {
        /* for the unleash case, we don't verify whether this unseen
           monster is the creature attached to the current leash */
        You("fail to %sleash something.", obj->leashmon ? "un" : "");
        /* trying again will work provided the monster is tame
           (and also that it doesn't change location by retry time) */
        map_invisible(cc.x, cc.y);
    } else if (!mtmp->mtame) {
        pline("%s %s leashed!", Monnam(mtmp),
              (!obj->leashmon) ? "cannot be" : "is not");
    } else if (!obj->leashmon) {
        /* applying a leash which isn't currently in use */
        if (mtmp->mleashed) {
            pline("This %s is already leashed.",
                  spotmon ? l_monnam(mtmp) : "creature");
        } else if (unsolid(mtmp->data)) {
            pline("The leash would just fall off.");
        } else if (nolimbs(mtmp->data) && !has_head(mtmp->data)) {
            pline("%s has no extremities the leash would fit.",
                  Monnam(mtmp));
        } else if (!leashable(mtmp)) {
            pline("The leash won't fit onto %s.",
                  spotmon ? y_monnam(mtmp) : l_monnam(mtmp));
        } else {
            You("slip the leash around %s.",
                spotmon ? y_monnam(mtmp) : l_monnam(mtmp));
            mtmp->mleashed = 1;
            obj->leashmon = (int) mtmp->m_id;
            mtmp->msleeping = 0;
        }
    } else {
        /* applying a leash which is currently in use */
        if (obj->leashmon != (int) mtmp->m_id) {
            pline("This leash is not attached to that creature.");
        } else if (obj->cursed) {
            pline_The("leash would not come off!");
            set_bknown(obj, 1);
        } else {
            mtmp->mleashed = 0;
            obj->leashmon = 0;
            You("remove the leash from %s.",
                spotmon ? y_monnam(mtmp) : l_monnam(mtmp));
        }
    }
    return 1;
}

/* assuming mtmp->mleashed has been checked */
struct obj *
get_mleash(mtmp)
struct monst *mtmp;
{
    struct obj *otmp;

    otmp = g.invent;
    while (otmp) {
        if (otmp->otyp == LEASH && otmp->leashmon == (int) mtmp->m_id)
            return otmp;
        otmp = otmp->nobj;
    }
    return (struct obj *) 0;
}

boolean
next_to_u()
{
    register struct monst *mtmp;
    register struct obj *otmp;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        if (mtmp->mleashed) {
            if (distu(mtmp->mx, mtmp->my) > 2)
                mnexto(mtmp);
            if (distu(mtmp->mx, mtmp->my) > 2) {
                for (otmp = g.invent; otmp; otmp = otmp->nobj)
                    if (otmp->otyp == LEASH
                        && otmp->leashmon == (int) mtmp->m_id) {
                        if (otmp->cursed)
                            return FALSE;
                        You_feel("%s leash go slack.",
                                 (number_leashed() > 1) ? "a" : "the");
                        mtmp->mleashed = 0;
                        otmp->leashmon = 0;
                    }
            }
        }
    }
    /* no pack mules for the Amulet */
    if (u.usteed && mon_has_amulet(u.usteed))
        return FALSE;
    return TRUE;
}

void
check_leash(x, y)
register xchar x, y;
{
    register struct obj *otmp;
    register struct monst *mtmp;

    for (otmp = g.invent; otmp; otmp = otmp->nobj) {
        if (otmp->otyp != LEASH || otmp->leashmon == 0)
            continue;
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if ((int) mtmp->m_id == otmp->leashmon)
                break;
        }
        if (!mtmp) {
            impossible("leash in use isn't attached to anything?");
            otmp->leashmon = 0;
            continue;
        }
        if (dist2(u.ux, u.uy, mtmp->mx, mtmp->my)
            > dist2(x, y, mtmp->mx, mtmp->my)) {
            if (!um_dist(mtmp->mx, mtmp->my, 3)) {
                ; /* still close enough */
            } else if (otmp->cursed && !breathless(mtmp->data)) {
                if (um_dist(mtmp->mx, mtmp->my, 5)
                    || (mtmp->mhp -= rnd(2)) <= 0) {
                    long save_pacifism = u.uconduct.killer;

                    Your("leash chokes %s to death!", mon_nam(mtmp));
                    /* hero might not have intended to kill pet, but
                       that's the result of his actions; gain experience,
                       lose pacifism, take alignment and luck hit, make
                       corpse less likely to remain tame after revival */
                    xkilled(mtmp, XKILL_NOMSG);
                    /* life-saving doesn't ordinarily reset this */
                    if (!DEADMONSTER(mtmp))
                        u.uconduct.killer = save_pacifism;
                } else {
                    pline("%s is choked by the leash!", Monnam(mtmp));
                    /* tameness eventually drops to 1 here (never 0) */
                    if (mtmp->mtame && rn2(mtmp->mtame))
                        mtmp->mtame--;
                }
            } else {
                if (um_dist(mtmp->mx, mtmp->my, 5)) {
                    pline("%s leash snaps loose!", s_suffix(Monnam(mtmp)));
                    m_unleash(mtmp, FALSE);
                } else {
                    You("pull on the leash.");
                    if (mtmp->data->msound != MS_SILENT)
                        switch (rn2(3)) {
                        case 0:
                            growl(mtmp);
                            break;
                        case 1:
                            yelp(mtmp);
                            break;
                        default:
                            whimper(mtmp);
                            break;
                        }
                }
            }
        }
    }
}

/* charisma is supposed to include qualities like leadership and personal
   magnetism rather than just appearance, but it has devolved to this... */
const char *
beautiful()
{
    const char *res;
    int cha = ACURR(A_CHA);

    /* don't bother complaining about the sexism; nethack is not real life */
    res = ((cha >= 25) ? "sublime" /* 25 is the maximum possible */
           : (cha >= 19) ? "splendorous" /* note: not "splendiferous" */
             : (cha >= 16) ? ((poly_gender() == 1) ? "beautiful" : "handsome")
               : (cha >= 14) ? ((poly_gender() == 1) ? "winsome" : "amiable")
                 : (cha >= 11) ? "cute"
                   : (cha >= 9) ? "plain"
                     : (cha >= 6) ? "homely"
                       : (cha >= 4) ? "ugly"
                         : "hideous"); /* 3 is the minimum possible */
    return res;
}

static const char look_str[] = "look %s.";

static int
use_mirror(obj)
struct obj *obj;
{
    const char *mirror, *uvisage;
    struct monst *mtmp;
    unsigned how_seen;
    char mlet;
    boolean vis, invis_mirror, useeit, monable;

    if (!getdir((char *) 0))
        return 0;
    invis_mirror = Invis;
    useeit = !Blind && (!invis_mirror || See_invisible);
    uvisage = beautiful();
    mirror = simpleonames(obj); /* "mirror" or "looking glass" */
    if (obj->cursed && !rn2(2)) {
        if (!Blind)
            pline_The("%s fogs up and doesn't reflect!", mirror);
        else
            pline("Nothing seems to happen.");
        return 1;
    }
    if (!u.dx && !u.dy && !u.dz) {
        if (!useeit) {
            You_cant("see your %s %s.", uvisage, body_part(FACE));
        } else {
            if (u.umonnum == PM_FLOATING_EYE) {
                if (Free_action) {
                    You("stiffen momentarily under your gaze.");
                } else {
                    if (Hallucination)
                        pline("Yow!  The %s stares back!", mirror);
                    else
                        pline("Yikes!  You've frozen yourself!");
                    if (!Hallucination || !rn2(4)) {
                        nomul(-rnd(MAXULEV + 6 - u.ulevel));
                        g.multi_reason = "gazing into a mirror";
                    }
                    g.nomovemsg = 0; /* default, "you can move again" */
                }
            } else if (is_vampire(g.youmonst.data)
                       || is_vampshifter(&g.youmonst)) {
                You("don't have a reflection.");
            } else if (u.umonnum == PM_UMBER_HULK) {
                pline("Huh?  That doesn't look like you!");
                make_confused(HConfusion + d(3, 4), FALSE);
            } else if (Hallucination) {
                You(look_str, hcolor((char *) 0));
            } else if (Sick) {
                You(look_str, "peaked");
            } else if (u.uhs >= WEAK) {
                You(look_str, "undernourished");
            } else if (Upolyd) {
                You("look like %s.", an(mons[u.umonnum].mname));
            } else {
                You("look as %s as ever.", uvisage);
            }
        }
        return 1;
    }
    if (u.uswallow) {
        if (useeit)
            You("reflect %s %s.", s_suffix(mon_nam(u.ustuck)),
                mbodypart(u.ustuck, STOMACH));
        return 1;
    }
    if (Underwater) {
        if (useeit)
            You(Hallucination ? "give the fish a chance to fix their makeup."
                              : "reflect the murky water.");
        return 1;
    }
    if (u.dz) {
        if (useeit)
            You("reflect the %s.",
                (u.dz > 0) ? surface(u.ux, u.uy) : ceiling(u.ux, u.uy));
        return 1;
    }
    mtmp = bhit(u.dx, u.dy, COLNO, INVIS_BEAM,
                (int FDECL((*), (MONST_P, OBJ_P))) 0,
                (int FDECL((*), (OBJ_P, OBJ_P))) 0, &obj);
    if (!mtmp || !haseyes(mtmp->data) || g.notonhead)
        return 1;

    /* couldsee(mtmp->mx, mtmp->my) is implied by the fact that bhit()
       targetted it, so we can ignore possibility of X-ray vision */
    vis = canseemon(mtmp);
    /* ways to directly see monster (excludes X-ray vision, telepathy,
       extended detection, type-specific warning) */
#define SEENMON (MONSEEN_NORMAL | MONSEEN_SEEINVIS | MONSEEN_INFRAVIS)
    how_seen = vis ? howmonseen(mtmp) : 0;
    /* whether monster is able to use its vision-based capabilities */
    monable = !mtmp->mcan && (!mtmp->minvis || perceives(mtmp->data));
    mlet = mtmp->data->mlet;
    if (mtmp->msleeping) {
        if (vis)
            pline("%s is too tired to look at your %s.", Monnam(mtmp),
                  mirror);
    } else if (!mtmp->mcansee) {
        if (vis)
            pline("%s can't see anything right now.", Monnam(mtmp));
    } else if (invis_mirror && !perceives(mtmp->data)) {
        if (vis)
            pline("%s fails to notice your %s.", Monnam(mtmp), mirror);
        /* infravision doesn't produce an image in the mirror */
    } else if ((how_seen & SEENMON) == MONSEEN_INFRAVIS) {
        if (vis) /* (redundant) */
            pline("%s in the dark.",
                  monverbself(mtmp, Monnam(mtmp), "are",
                              "too far away to see"));
        /* some monsters do special things */
    } else if (mlet == S_VAMPIRE || mlet == S_GHOST || is_vampshifter(mtmp)) {
        if (vis)
            pline("%s doesn't have a reflection.", Monnam(mtmp));
    } else if (monable && mtmp->data == &mons[PM_MEDUSA]) {
        if (mon_reflects(mtmp, "The gaze is reflected away by %s %s!"))
            return 1;
        if (vis)
            pline("%s is turned to stone!", Monnam(mtmp));
        g.stoned = TRUE;
        killed(mtmp);
    } else if (monable && mtmp->data == &mons[PM_FLOATING_EYE]) {
        int tmp = d((int) mtmp->m_lev, (int) mtmp->data->mattk[0].damd);
        if (!rn2(4))
            tmp = 120;
        if (vis)
            pline("%s is frozen by its reflection.", Monnam(mtmp));
        else
            You_hear("%s stop moving.", something);
        paralyze_monst(mtmp, (int) mtmp->mfrozen + tmp);
    } else if (monable && mtmp->data == &mons[PM_UMBER_HULK]) {
        if (vis)
            pline("%s confuses itself!", Monnam(mtmp));
        mtmp->mconf = 1;
    } else if (monable && (mlet == S_NYMPH || mtmp->data == &mons[PM_SUCCUBUS]
                           || mtmp->data == &mons[PM_INCUBUS])) {
        if (vis) {
            char buf[BUFSZ]; /* "She" or "He" */

            pline("%s in your %s.", /* "<mon> admires self in your mirror " */
                  monverbself(mtmp, Monnam(mtmp), "admire", (char *) 0),
                  mirror);
            pline("%s takes it!", upstart(strcpy(buf, mhe(mtmp))));
        } else
            pline("It steals your %s!", mirror);
        setnotworn(obj); /* in case mirror was wielded */
        freeinv(obj);
        (void) mpickobj(mtmp, obj);
        if (!tele_restrict(mtmp))
            (void) rloc(mtmp, TRUE);
    } else if (!is_unicorn(mtmp->data) && !humanoid(mtmp->data)
               && (!mtmp->minvis || perceives(mtmp->data)) && rn2(5)) {
        boolean do_react = TRUE;

        if (mtmp->mfrozen) {
            if (vis)
                You("discern no obvious reaction from %s.", mon_nam(mtmp));
            else
                You_feel(
                       "a bit silly gesturing the mirror in that direction.");
            do_react = FALSE;
        }
        if (do_react) {
            if (vis)
                pline("%s is frightened by its reflection.", Monnam(mtmp));
            monflee(mtmp, d(2, 4), FALSE, FALSE);
        }
    } else if (!Blind) {
        if (mtmp->minvis && !See_invisible)
            ;
        else if ((mtmp->minvis && !perceives(mtmp->data))
                 /* redundant: can't get here if these are true */
                 || !haseyes(mtmp->data) || g.notonhead || !mtmp->mcansee)
            pline("%s doesn't seem to notice %s reflection.", Monnam(mtmp),
                  mhis(mtmp));
        else
            pline("%s ignores %s reflection.", Monnam(mtmp), mhis(mtmp));
    }
    return 1;
}

static void
use_bell(optr)
struct obj **optr;
{
    register struct obj *obj = *optr;
    struct monst *mtmp;
    boolean wakem = FALSE, learno = FALSE,
            ordinary = (obj->otyp != BELL_OF_OPENING || !obj->spe),
            invoking =
                (obj->otyp == BELL_OF_OPENING && invocation_pos(u.ux, u.uy)
                 && !On_stairs(u.ux, u.uy));

    You("ring %s.", the(xname(obj)));

    if (Underwater || (u.uswallow && ordinary)) {
        pline("But the sound is muffled.");

    } else if (invoking && ordinary) {
        /* needs to be recharged... */
        pline("But it makes no sound.");
        learno = TRUE; /* help player figure out why */

    } else if (ordinary) {
        if (obj->cursed && !rn2(4)
            /* note: once any of them are gone, we stop all of them */
            && !(g.mvitals[PM_WOOD_NYMPH].mvflags & G_GONE)
            && !(g.mvitals[PM_WATER_NYMPH].mvflags & G_GONE)
            && !(g.mvitals[PM_MOUNTAIN_NYMPH].mvflags & G_GONE)
            && (mtmp = makemon(mkclass(S_NYMPH, 0), u.ux, u.uy, NO_MINVENT))
                   != 0) {
            You("summon %s!", a_monnam(mtmp));
            if (!obj_resists(obj, 93, 100)) {
                pline("%s shattered!", Tobjnam(obj, "have"));
                useup(obj);
                *optr = 0;
            } else
                switch (rn2(3)) {
                default:
                    break;
                case 1:
                    mon_adjust_speed(mtmp, 2, (struct obj *) 0);
                    break;
                case 2: /* no explanation; it just happens... */
                    g.nomovemsg = "";
                    g.multi_reason = NULL;
                    nomul(-rnd(2));
                    break;
                }
        }
        wakem = TRUE;

    } else {
        /* charged Bell of Opening */
        consume_obj_charge(obj, TRUE);

        if (u.uswallow) {
            if (!obj->cursed)
                (void) openit();
            else
                pline1(nothing_happens);

        } else if (obj->cursed) {
            coord mm;

            mm.x = u.ux;
            mm.y = u.uy;
            mkundead(&mm, FALSE, NO_MINVENT);
            wakem = TRUE;

        } else if (invoking) {
            pline("%s an unsettling shrill sound...", Tobjnam(obj, "issue"));
            obj->age = g.moves;
            learno = TRUE;
            wakem = TRUE;

        } else if (obj->blessed) {
            int res = 0;

            if (uchain) {
                unpunish();
                res = 1;
            } else if (u.utrap && u.utraptype == TT_BURIEDBALL) {
                buried_ball_to_freedom();
                res = 1;
            }
            res += openit();
            switch (res) {
            case 0:
                pline1(nothing_happens);
                break;
            case 1:
                pline("%s opens...", Something);
                learno = TRUE;
                break;
            default:
                pline("Things open around you...");
                learno = TRUE;
                break;
            }

        } else { /* uncursed */
            if (findit() != 0)
                learno = TRUE;
            else
                pline1(nothing_happens);
        }

    } /* charged BofO */

    if (learno) {
        makeknown(BELL_OF_OPENING);
        obj->known = 1;
    }
    if (wakem)
        wake_nearby();
}

static void
use_candelabrum(obj)
register struct obj *obj;
{
    const char *s = (obj->spe != 1) ? "candles" : "candle";

    if (obj->lamplit) {
        You("snuff the %s.", s);
        end_burn(obj, TRUE);
        return;
    }
    if (obj->spe <= 0) {
        struct obj *otmp;

        pline("This %s has no %s.", xname(obj), s);
        /* only output tip if candles are in inventory */
        for (otmp = g.invent; otmp; otmp = otmp->nobj)
            if (Is_candle(otmp))
                break;
        if (otmp)
            pline("To attach candles, apply them instead of the %s.",
                  xname(obj));
        return;
    }
    if (Underwater) {
        You("cannot make fire under water.");
        return;
    }
    if (u.uswallow || obj->cursed) {
        if (!Blind)
            pline_The("%s %s for a moment, then %s.", s, vtense(s, "flicker"),
                      vtense(s, "die"));
        return;
    }
    if (obj->spe < 7) {
        There("%s only %d %s in %s.", vtense(s, "are"), obj->spe, s,
              the(xname(obj)));
        if (!Blind)
            pline("%s lit.  %s dimly.", obj->spe == 1 ? "It is" : "They are",
                  Tobjnam(obj, "shine"));
    } else {
        pline("%s's %s burn%s", The(xname(obj)), s,
              (Blind ? "." : " brightly!"));
    }
    if (!invocation_pos(u.ux, u.uy) || On_stairs(u.ux, u.uy)) {
        pline_The("%s %s being rapidly consumed!", s, vtense(s, "are"));
        /* this used to be obj->age /= 2, rounding down; an age of
           1 would yield 0, confusing begin_burn() and producing an
           unlightable, unrefillable candelabrum; round up instead */
        obj->age = (obj->age + 1L) / 2L;

        /* to make absolutely sure the game doesn't become unwinnable as
           a consequence of a broken candelabrum */
        if (obj->age == 0) {
            impossible("Candelabrum with candles but no fuel?");
            obj->age = 1;
        }
    } else {
        if (obj->spe == 7) {
            if (Blind)
                pline("%s a strange warmth!", Tobjnam(obj, "radiate"));
            else
                pline("%s with a strange light!", Tobjnam(obj, "glow"));
        }
        obj->known = 1;
    }
    begin_burn(obj, FALSE);
}

static void
use_candle(optr)
struct obj **optr;
{
    register struct obj *obj = *optr;
    register struct obj *otmp;
    const char *s = (obj->quan != 1) ? "candles" : "candle";
    char qbuf[QBUFSZ], qsfx[QBUFSZ], *q;
    boolean was_lamplit;

    if (u.uswallow) {
        You(no_elbow_room);
        return;
    }

    /* obj is the candle; otmp is the candelabrum */
    otmp = carrying(CANDELABRUM_OF_INVOCATION);
    if (!otmp || otmp->spe == 7) {
        use_lamp(obj);
        return;
    }

    /* first, minimal candelabrum suffix for formatting candles */
    Sprintf(qsfx, " to\033%s?", thesimpleoname(otmp));
    /* next, format the candles as a prefix for the candelabrum */
    (void) safe_qbuf(qbuf, "Attach ", qsfx, obj, yname, thesimpleoname, s);
    /* strip temporary candelabrum suffix */
    if ((q = strstri(qbuf, " to\033")) != 0)
        Strcpy(q, " to ");
    /* last, format final "attach candles to candelabrum?" query */
    if (yn(safe_qbuf(qbuf, qbuf, "?", otmp, yname, thesimpleoname, "it"))
        == 'n') {
        use_lamp(obj);
        return;
    } else {
        if ((long) otmp->spe + obj->quan > 7L) {
            obj = splitobj(obj, 7L - (long) otmp->spe);
            /* avoid a grammatical error if obj->quan gets
               reduced to 1 candle from more than one */
            s = (obj->quan != 1) ? "candles" : "candle";
        } else
            *optr = 0;

        /* The candle's age field doesn't correctly reflect the amount
           of fuel in it while it's lit, because the fuel is measured
           by the timer. So to get accurate age updating, we need to
           end the burn temporarily while attaching the candle. */
        was_lamplit = obj->lamplit;
        if (was_lamplit)
            end_burn(obj, TRUE);

        You("attach %ld%s %s to %s.", obj->quan, !otmp->spe ? "" : " more", s,
            the(xname(otmp)));
        if (!otmp->spe || otmp->age > obj->age)
            otmp->age = obj->age;
        otmp->spe += (int) obj->quan;
        if (otmp->lamplit && !was_lamplit)
            pline_The("new %s magically %s!", s, vtense(s, "ignite"));
        else if (!otmp->lamplit && was_lamplit)
            pline("%s out.", (obj->quan > 1L) ? "They go" : "It goes");
        if (obj->unpaid)
            verbalize("You %s %s, you bought %s!",
                      otmp->lamplit ? "burn" : "use",
                      (obj->quan > 1L) ? "them" : "it",
                      (obj->quan > 1L) ? "them" : "it");
        if (obj->quan < 7L && otmp->spe == 7)
            pline("%s now has seven%s candles attached.", The(xname(otmp)),
                  otmp->lamplit ? " lit" : "");
        /* candelabrum's light range might increase */
        if (otmp->lamplit)
            obj_merge_light_sources(otmp, otmp);
        /* candles are no longer a separate light source */
        /* candles are now gone */
        useupall(obj);
        /* candelabrum's weight is changing */
        otmp->owt = weight(otmp);
        update_inventory();
    }
}

/* call in drop, throw, and put in box, etc. */
boolean
snuff_candle(otmp)
struct obj *otmp;
{
    boolean candle = Is_candle(otmp);

    if ((candle || otmp->otyp == CANDELABRUM_OF_INVOCATION)
        && otmp->lamplit) {
        char buf[BUFSZ];
        xchar x, y;
        boolean many = candle ? (otmp->quan > 1L) : (otmp->spe > 1);

        (void) get_obj_location(otmp, &x, &y, 0);
        if (otmp->where == OBJ_MINVENT ? cansee(x, y) : !Blind)
            pline("%s%scandle%s flame%s extinguished.", Shk_Your(buf, otmp),
                  (candle ? "" : "candelabrum's "), (many ? "s'" : "'s"),
                  (many ? "s are" : " is"));
        end_burn(otmp, TRUE);
        return TRUE;
    }
    return FALSE;
}

/* called when lit lamp is hit by water or put into a container or
   you've been swallowed by a monster; obj might be in transit while
   being thrown or dropped so don't assume that its location is valid */
boolean
snuff_lit(obj)
struct obj *obj;
{
    xchar x, y;

    if (obj->lamplit) {
        if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP
            || obj->otyp == LANTERN || obj->otyp == POT_OIL) {
            (void) get_obj_location(obj, &x, &y, 0);
            if (obj->where == OBJ_MINVENT ? cansee(x, y) : !Blind)
                pline("%s %s out!", Yname2(obj), otense(obj, "go"));
            end_burn(obj, TRUE);
            return TRUE;
        }
        if (snuff_candle(obj))
            return TRUE;
    }
    return FALSE;
}

/* Called when potentially lightable object is affected by fire_damage().
   Return TRUE if object was lit and FALSE otherwise --ALI */
boolean
catch_lit(obj)
struct obj *obj;
{
    xchar x, y;

    if (!obj->lamplit && (obj->otyp == MAGIC_LAMP || ignitable(obj))) {
        if ((obj->otyp == MAGIC_LAMP
             || obj->otyp == CANDELABRUM_OF_INVOCATION) && obj->spe == 0)
            return FALSE;
        else if (obj->otyp != MAGIC_LAMP && obj->age == 0)
            return FALSE;
        if (!get_obj_location(obj, &x, &y, 0))
            return FALSE;
        if (obj->otyp == CANDELABRUM_OF_INVOCATION && obj->cursed)
            return FALSE;
        if ((obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP
             || obj->otyp == LANTERN) && obj->cursed && !rn2(2))
            return FALSE;
        if (obj->where == OBJ_MINVENT ? cansee(x, y) : !Blind)
            pline("%s %s light!", Yname2(obj), otense(obj, "catch"));
        if (obj->otyp == POT_OIL)
            makeknown(obj->otyp);
        if (carried(obj) && obj->unpaid && costly_spot(u.ux, u.uy)) {
            /* if it catches while you have it, then it's your tough luck */
            check_unpaid(obj);
            verbalize("That's in addition to the cost of %s %s, of course.",
                      yname(obj), obj->quan == 1L ? "itself" : "themselves");
            bill_dummy_object(obj);
        }
        begin_burn(obj, FALSE);
        return TRUE;
    }
    return FALSE;
}

static void
use_lamp(obj)
struct obj *obj;
{
    char buf[BUFSZ];

    if (obj->lamplit) {
        if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP
            || obj->otyp == LANTERN)
            pline("%slamp is now off.", Shk_Your(buf, obj));
        else
            You("snuff out %s.", yname(obj));
        end_burn(obj, TRUE);
        return;
    }
    if (Underwater) {
        pline(!Is_candle(obj) ? "This is not a diving lamp"
                              : "Sorry, fire and water don't mix.");
        return;
    }
    /* magic lamps with an spe == 0 (wished for) cannot be lit */
    if ((!Is_candle(obj) && obj->age == 0)
        || (obj->otyp == MAGIC_LAMP && obj->spe == 0)) {
        if (obj->otyp == LANTERN)
            Your("lamp has run out of power.");
        else
            pline("This %s has no oil.", xname(obj));
        return;
    }
    if (obj->cursed && !rn2(2)) {
        if (!Blind)
            pline("%s for a moment, then %s.", Tobjnam(obj, "flicker"),
                  otense(obj, "die"));
    } else {
        if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP
            || obj->otyp == LANTERN) {
            check_unpaid(obj);
            pline("%slamp is now on.", Shk_Your(buf, obj));
        } else { /* candle(s) */
            pline("%s flame%s %s%s", s_suffix(Yname2(obj)), plur(obj->quan),
                  otense(obj, "burn"), Blind ? "." : " brightly!");
            if (obj->unpaid && costly_spot(u.ux, u.uy)
                && obj->age == 20L * (long) objects[obj->otyp].oc_cost) {
                const char *ithem = (obj->quan > 1L) ? "them" : "it";

                verbalize("You burn %s, you bought %s!", ithem, ithem);
                bill_dummy_object(obj);
            }
        }
        begin_burn(obj, FALSE);
    }
}

static void
light_cocktail(optr)
struct obj **optr;
{
    struct obj *obj = *optr; /* obj is a potion of oil */
    char buf[BUFSZ];
    boolean split1off;

    if (u.uswallow) {
        You(no_elbow_room);
        return;
    }

    if (obj->lamplit) {
        You("snuff the lit potion.");
        end_burn(obj, TRUE);
        /*
         * Free & add to re-merge potion.  This will average the
         * age of the potions.  Not exactly the best solution,
         * but its easy.
         */
        freeinv(obj);
        *optr = addinv(obj);
        return;
    } else if (Underwater) {
        There("is not enough oxygen to sustain a fire.");
        return;
    }

    split1off = (obj->quan > 1L);
    if (split1off)
        obj = splitobj(obj, 1L);

    You("light %spotion.%s", shk_your(buf, obj),
        Blind ? "" : "  It gives off a dim light.");

    if (obj->unpaid && costly_spot(u.ux, u.uy)) {
        /* Normally, we shouldn't both partially and fully charge
         * for an item, but (Yendorian Fuel) Taxes are inevitable...
         */
        check_unpaid(obj);
        verbalize("That's in addition to the cost of the potion, of course.");
        bill_dummy_object(obj);
    }
    makeknown(obj->otyp);

    begin_burn(obj, FALSE); /* after shop billing */
    if (split1off) {
        obj_extract_self(obj); /* free from inv */
        obj->nomerge = 1;
        obj = hold_another_object(obj, "You drop %s!", doname(obj),
                                  (const char *) 0);
        if (obj)
            obj->nomerge = 0;
    }
    *optr = obj;
}

static NEARDATA const char
    cuddly[] = { TOOL_CLASS, GEM_CLASS, 0 },
    cuddlier[] = { TOOL_CLASS, GEM_CLASS, FOOD_CLASS, 0 };

int
dorub()
{
    struct obj *obj;

    if (nohands(g.youmonst.data)) {
        You("aren't able to rub anything without hands.");
        return 0;
    }
    obj = getobj(carrying(LUMP_OF_ROYAL_JELLY) ? cuddlier : cuddly, "rub");
    if (!obj) {
        /* pline1(Never_mind); -- handled by getobj() */
        return 0;
    }
    if ((obj && obj->oclass == GEM_CLASS && obj->otyp != MOONSTONE) || obj->oclass == FOOD_CLASS) {
        if (is_graystone(obj)) {
            use_stone(obj);
            return 1;
        } else if (obj->otyp == LUMP_OF_ROYAL_JELLY) {
            return use_royal_jelly(obj);
        } else {
            pline("Sorry, I don't know how to use that.");
            return 0;
        }
    }
    if (!wield_tool(obj, "rub"))
        return 0;

    /* now uwep is obj */
    if (uwep->otyp == MAGIC_LAMP) {
        if (uwep->spe > 0 && !rn2(3)) {
            check_unpaid_usage(uwep, TRUE); /* unusual item use */
            /* bones preparation:  perform the lamp transformation
               before releasing the djinni in case the latter turns out
               to be fatal (a hostile djinni has no chance to attack yet,
               but an indebted one who grants a wish might bestow an
               artifact which blasts the hero with lethal results) */
            uwep->otyp = OIL_LAMP;
            uwep->spe = 0; /* for safety */
            uwep->age = rn1(500, 1000);
            if (uwep->lamplit)
                begin_burn(uwep, TRUE);
            djinni_from_bottle(uwep);
            makeknown(MAGIC_LAMP);
            update_inventory();
        } else if (rn2(2)) {
            You("%s smoke.", !Blind ? "see a puff of" : "smell");
        } else
            pline1(nothing_happens);
    } else if (uwep->otyp == MOONSTONE) {
        if (!uwep->lamplit) {
            begin_burn(uwep, FALSE);
        } else {
            pline("%s glowing.", Yobjnam2(uwep, "stop"));
            end_burn(uwep, FALSE);
            return 1;
        }
        makeknown(MOONSTONE);
        update_inventory();
        pline("This stone is quite cold!");
        if (!Blind)
            pline("%s begins to glow with a %s silvery light.", Yobjnam2(obj, "glow"), 
                (flags.moonphase < 2 || flags.moonphase > 5) ? "soft" : "bright");
    } else if (obj->otyp == LANTERN) {
        /* message from Adventure */
        pline("Rubbing the electric lamp is not particularly rewarding.");
        pline("Anyway, nothing exciting happens.");
    } else
        pline1(nothing_happens);
    return 1;
}

int
dojump()
{
    /* Physical jump */
    return jump(0);
}

enum jump_trajectory {
    jAny  = 0, /* any direction => magical jump */
    jHorz = 1,
    jVert = 2,
    jDiag = 3  /* jHorz|jVert */
};

/* callback routine for walk_path() */
static boolean
check_jump(arg, x, y)
genericptr arg;
int x, y;
{
    int traj = *(int *) arg;
    struct rm *lev = &levl[x][y];

    if (IS_STWALL(lev->typ))
        return FALSE;
    if (IS_DOOR(lev->typ)) {
        if (closed_door(x, y))
            return FALSE;
        if ((lev->doormask & D_ISOPEN) != 0 && traj != jAny
            /* reject diagonal jump into or out-of or through open door */
            && (traj == jDiag
                /* reject horizontal jump through horizontal open door
                   and non-horizontal (ie, vertical) jump through
                   non-horizontal (vertical) open door */
                || ((traj & jHorz) != 0) == (lev->horizontal != 0)))
            return FALSE;
        /* empty doorways aren't restricted */
    }
    /* let giants jump over boulders (what about Flying?
       and is there really enough head room for giants to jump
       at all, let alone over something tall?) */
    if (sobj_at(BOULDER, x, y) && !throws_rocks(g.youmonst.data))
        return FALSE;
    return TRUE;
}

static boolean
is_valid_jump_pos(x, y, magic, showmsg)
int x, y, magic;
boolean showmsg;
{
    if (!magic && !(HJumping & ~INTRINSIC) && !EJumping && distu(x, y) != 5) {
        /* The Knight jumping restriction still applies when riding a
         * horse.  After all, what shape is the knight piece in chess?
         */
        if (showmsg)
            pline("Illegal move!");
        return FALSE;
    } else if (distu(x, y) > (magic ? 6 + magic * 3 : 9)) {
        if (showmsg)
            pline("Too far!");
        return FALSE;
    } else if (!isok(x, y)) {
        if (showmsg)
            You("cannot jump there!");
        return FALSE;
    } else if (!cansee(x, y)) {
        if (showmsg)
            You("cannot see where to land!");
        return FALSE;
    } else {
        coord uc, tc;
        struct rm *lev = &levl[u.ux][u.uy];
        /* we want to categorize trajectory for use in determining
           passage through doorways: horizonal, vertical, or diagonal;
           since knight's jump and other irregular directions are
           possible, we flatten those out to simplify door checks */
        int diag, traj,
            dx = x - u.ux, dy = y - u.uy,
            ax = abs(dx), ay = abs(dy);

        /* diag: any non-orthogonal destination classifed as diagonal */
        diag = (magic || Passes_walls || (!dx && !dy)) ? jAny
               : !dy ? jHorz : !dx ? jVert : jDiag;
        /* traj: flatten out the trajectory => some diagonals re-classified */
        if (ax >= 2 * ay)
            ay = 0;
        else if (ay >= 2 * ax)
            ax = 0;
        traj = (magic || Passes_walls || (!ax && !ay)) ? jAny
               : !ay ? jHorz : !ax ? jVert : jDiag;
        /* walk_path doesn't process the starting spot;
           this is iffy:  if you're starting on a closed door spot,
           you _can_ jump diagonally from doorway (without needing
           Passes_walls); that's intentional but is it correct? */
        if (diag == jDiag && IS_DOOR(lev->typ)
            && (lev->doormask & D_ISOPEN) != 0
            && (traj == jDiag
                || ((traj & jHorz) != 0) == (lev->horizontal != 0))) {
            if (showmsg)
                You_cant("jump diagonally out of a doorway.");
            return FALSE;
        }
        uc.x = u.ux, uc.y = u.uy;
        tc.x = x, tc.y = y; /* target */
        if (!walk_path(&uc, &tc, check_jump, (genericptr_t) &traj)) {
            if (showmsg)
                There("is an obstacle preventing that jump.");
            return FALSE;
        }
    }
    return TRUE;
}

boolean
check_mon_jump(mtmp, x, y)
struct monst *mtmp;
int x, y;
{
    coord mc, tc;
    mc.x = mtmp->mx, mc.y = mtmp->my;
    tc.x = x, tc.y = y; /* target */

    int traj,
        dx = x - u.ux, dy = y - u.uy,
        ax = abs(dx), ay = abs(dy);
    /* traj: flatten out the trajectory => some diagonals re-classified */
    if (ax >= 2 * ay)
        ay = 0;
    else if (ay >= 2 * ax)
        ax = 0;
    traj = jAny;

    if (!walk_path(&mc, &tc, check_jump, (genericptr_t) & traj)) {
        return FALSE;
    }
    return TRUE;
}

static boolean
get_valid_jump_position(x,y)
int x,y;
{
    return (isok(x, y)
            && (ACCESSIBLE(levl[x][y].typ) || Passes_walls)
            && is_valid_jump_pos(x, y, g.jumping_is_magic, FALSE));
}

static void
display_jump_positions(state)
int state;
{
    if (state == 0) {
        tmp_at(DISP_BEAM, cmap_to_glyph(S_goodpos));
    } else if (state == 1) {
        int x, y, dx, dy;

        for (dx = -4; dx <= 4; dx++)
            for (dy = -4; dy <= 4; dy++) {
                x = dx + (int) u.ux;
                y = dy + (int) u.uy;
                if (get_valid_jump_position(x, y))
                    tmp_at(x, y);
            }
    } else {
        tmp_at(DISP_END, 0);
    }
}

int
jump(magic)
int magic; /* 0=Physical, otherwise skill level */
{
    coord cc;

    /* attempt "jumping" spell if hero has no innate jumping ability */
    if (!magic && !Jumping) {
        int sp_no;

        for (sp_no = 0; sp_no < MAXSPELL; ++sp_no)
            if (g.spl_book[sp_no].sp_id == NO_SPELL)
                break;
            else if (g.spl_book[sp_no].sp_id == SPE_JUMPING)
                return spelleffects(sp_no, FALSE);
    }

    if (!magic && (nolimbs(g.youmonst.data) || slithy(g.youmonst.data))) {
        /* normally (nolimbs || slithy) implies !Jumping,
           but that isn't necessarily the case for knights */
        You_cant("jump; you have no legs!");
        return 0;
    } else if (!magic && !Jumping) {
        You_cant("jump very far.");
        return 0;
    /* if steed is immobile, can't do physical jump but can do spell one */
    } else if (!magic && u.usteed && stucksteed(FALSE)) {
        /* stucksteed gave "<steed> won't move" message */
        return 0;
    } else if (u.uswallow) {
        if (magic) {
            You("bounce around a little.");
            return 1;
        }
        pline("You've got to be kidding!");
        return 0;
    } else if (u.uinwater) {
        if (magic) {
            You("swish around a little.");
            return 1;
        }
        pline("This calls for swimming, not jumping!");
        return 0;
    } else if (u.ustuck) {
        if (u.ustuck->mtame && !Conflict && !u.ustuck->mconf) {
            struct monst *mtmp = u.ustuck;

            set_ustuck((struct monst *) 0);
            You("pull free from %s.", mon_nam(mtmp));
            return 1;
        }
        if (magic) {
            You("writhe a little in the grasp of %s!", mon_nam(u.ustuck));
            return 1;
        }
        You("cannot escape from %s!", mon_nam(u.ustuck));
        return 0;
    } else if (Levitation || Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) {
        if (magic) {
            You("flail around a little.");
            return 1;
        }
        You("don't have enough traction to jump.");
        return 0;
    } else if (!magic && near_capacity() > UNENCUMBERED) {
        You("are carrying too much to jump!");
        return 0;
    } else if (!magic && (u.uhunger <= 100 || ACURR(A_STR) < 6)) {
        You("lack the strength to jump!");
        return 0;
    } else if (!magic && Wounded_legs) {
        legs_in_no_shape("jumping", u.usteed != 0);
        return 0;
    } else if (u.usteed && u.utrap) {
        pline("%s is stuck in a trap.", Monnam(u.usteed));
        return 0;
    }

    pline("Where do you want to jump?");
    cc.x = u.ux;
    cc.y = u.uy;
    g.jumping_is_magic = magic;
    getpos_sethilite(display_jump_positions, get_valid_jump_position);
    if (getpos(&cc, TRUE, "the desired position") < 0)
        return 0; /* user pressed ESC */
    if (!is_valid_jump_pos(cc.x, cc.y, magic, TRUE)) {
        return 0;
    } else {
        coord uc;
        int range, temp;

        if (u.utrap)
            switch (u.utraptype) {
            case TT_BEARTRAP: {
                long side = rn2(3) ? LEFT_SIDE : RIGHT_SIDE;

                You("rip yourself free of the bear trap!  Ouch!");
                losehp(Maybe_Half_Phys(rnd(10)), "jumping out of a bear trap",
                       KILLED_BY);
                set_wounded_legs(side, rn1(1000, 500));
                break;
            }
            case TT_PIT:
                You("leap from the pit!");
                break;
            case TT_WEB:
                You("tear the web apart as you pull yourself free!");
                deltrap(t_at(u.ux, u.uy));
                break;
            case TT_LAVA:
                You("pull yourself above the %s!", hliquid("lava"));
                reset_utrap(TRUE);
                return 1;
            case TT_BURIEDBALL:
            case TT_INFLOOR:
                You("strain your %s, but you're still %s.",
                    makeplural(body_part(LEG)),
                    (u.utraptype == TT_INFLOOR)
                        ? "stuck in the floor"
                        : "attached to the buried ball");
                set_wounded_legs(LEFT_SIDE, rn1(10, 11));
                set_wounded_legs(RIGHT_SIDE, rn1(10, 11));
                return 1;
            }

        /*
         * Check the path from uc to cc, calling hurtle_step at each
         * location.  The final position actually reached will be
         * in cc.
         */
        uc.x = u.ux;
        uc.y = u.uy;
        /* calculate max(abs(dx), abs(dy)) as the range */
        range = cc.x - uc.x;
        if (range < 0)
            range = -range;
        temp = cc.y - uc.y;
        if (temp < 0)
            temp = -temp;
        if (range < temp)
            range = temp;
        (void) walk_path(&uc, &cc, hurtle_jump, (genericptr_t) &range);
        /* hurtle_jump -> hurtle_step results in <u.ux,u.uy> == <cc.x,cc.y>
         * and usually moves the ball if punished, but does not handle all
         * the effects of landing on the final position.
         */
        teleds(cc.x, cc.y, TELEDS_NO_FLAGS);
        nomul(-1);
        g.multi_reason = "jumping around";
        g.nomovemsg = "";
        morehungry(rnd(25));
        return 1;
    }
}

boolean
tinnable(corpse)
struct obj *corpse;
{
    if (corpse->oeaten)
        return 0;
    if (corpse->odrained) 
        return 0;
    if (!mons[corpse->corpsenm].cnutrit)
        return 0;
    return 1;
}

static void
use_tinning_kit(obj)
struct obj *obj;
{
    struct obj *corpse, *can;

    /* This takes only 1 move.  If this is to be changed to take many
     * moves, we've got to deal with decaying corpses...
     */
    if (obj->spe <= 0) {
        You("seem to be out of tins.");
        return;
    }
    if (!(corpse = floorfood("tin", 2)))
        return;
    if (corpse->oeaten || corpse->odrained) {
        You("cannot tin %s which is partly eaten.", something);
        return;
    }
    if (touch_petrifies(&mons[corpse->corpsenm]) && !Stone_resistance
        && !uarmg) {
        char kbuf[BUFSZ];

        if (poly_when_stoned(g.youmonst.data))
            You("tin %s without wearing gloves.",
                an(mons[corpse->corpsenm].mname));
        else {
            pline("Tinning %s without wearing gloves is a fatal mistake...",
                  an(mons[corpse->corpsenm].mname));
            Sprintf(kbuf, "trying to tin %s without gloves",
                    an(mons[corpse->corpsenm].mname));
        }
        instapetrify(kbuf);
    }
    if (is_rider(&mons[corpse->corpsenm])) {
        if (revive_corpse(corpse, FALSE))
            verbalize("Yes...  But War does not preserve its enemies...");
        else
            pline_The("corpse evades your grasp.");
        return;
    }
    if (mons[corpse->corpsenm].cnutrit == 0) {
        pline("That's too insubstantial to tin.");
        return;
    }
    consume_obj_charge(obj, TRUE);

    if ((can = mksobj(TIN, FALSE, FALSE)) != 0) {
        static const char you_buy_it[] = "You tin it, you bought it!";

        can->corpsenm = corpse->corpsenm;
        can->cursed = obj->cursed;
        can->blessed = obj->blessed;
        can->owt = weight(can);
        can->known = 1;
        /* Mark tinned tins. No spinach allowed... */
        set_tin_variety(can, HOMEMADE_TIN);
        if (carried(corpse)) {
            if (corpse->unpaid)
                verbalize(you_buy_it);
            useup(corpse);
        } else {
            if (costly_spot(corpse->ox, corpse->oy) && !corpse->no_charge)
                verbalize(you_buy_it);
            useupf(corpse, 1L);
        }
        (void) hold_another_object(can, "You make, but cannot pick up, %s.",
                                   doname(can), (const char *) 0);
    } else
        impossible("Tinning failed.");
}

static NEARDATA const char stitchables[] = { ALL_CLASSES, ALLOW_NONE, 0 };

static void
use_sewing_kit(obj)
struct obj *obj;
{
    struct obj *corpse, *otmp;

    if (obj->spe <= 0) {
        You("are fresh out of thread.");
        return;
    }
    if (Glib || obj->cursed) {
        You("fumble with the sewing kit and prick your finger! Ouch!");
        losehp(1, "pricking a finger with a needle", KILLED_BY);
        dropx(obj);
        return;
    }
    /* Regular stuff */
    if (yn("Sew up an item?") == 'y') {
        otmp = getobj(stitchables, "sew");
            if (!otmp)
                return;
            if (inaccessible_equipment(otmp, "sew up", FALSE))
                return;
        if (otmp->material != LEATHER && otmp->material != CLOTH) {
            You("are unable to improve that with your sewing kit.");
            return;
        }
        if (otmp->oeroded && !rn2(3)) {
            otmp->oeroded -= 1;
            You("repair %s with your needle and thread.", yname(otmp));
            update_inventory();
        } else if (!otmp->oeroded) {
            You("practice your sewing with %s.", yname(otmp));
        } else {
            pline("Whoops! You prick your finger.");
            losehp(1, "pricking a finger with a needle", KILLED_BY);
        }
        consume_obj_charge(obj, TRUE);
        return;
    } else if (Race_if(PM_GHOUL) && !Upolyd) {
        /* Ghoul stuff */
        if (!(corpse = floorfood("graft", 2)))
            return;
        if (corpse->oeaten || corpse->odrained) {
            You("cannot sew %s which is partly eaten onto yourself.", something);
            return;
        }
        if (touch_petrifies(&mons[corpse->corpsenm]) && !Stone_resistance
            && !uarmg) {
            char kbuf[BUFSZ];

            if (poly_when_stoned(g.youmonst.data))
                You("sitch up %s without wearing gloves.",
                    an(mons[corpse->corpsenm].mname));
            else {
                pline("Sewing up %s without wearing gloves is a fatal mistake...",
                    an(mons[corpse->corpsenm].mname));
                Sprintf(kbuf, "trying to sew up %s without gloves",
                        an(mons[corpse->corpsenm].mname));
            }
            instapetrify(kbuf);
        }
        if (is_rider(&mons[corpse->corpsenm])) {
            if (revive_corpse(corpse, FALSE))
                verbalize("You are growing to be quite a stitch in my side, War.");
            else
                pline_The("corpse evades your grasp.");
            return;
        }
        if (mons[corpse->corpsenm].cnutrit == 0) {
            pline("That's too insubstantial to sew.");
            return;
        }
        if (unique_corpstat(&mons[corpse->corpsenm])) {
            pline("That's just asking for trouble.");
            return;
        }
        You("graft %s onto your body.", an(mons[corpse->corpsenm].mname));
        u.ugrave_arise = corpse->corpsenm;
        consume_obj_charge(obj, TRUE);
        return;
    }
}

void
use_unicorn_horn(optr)
struct obj **optr;
{
#define PROP_COUNT 7           /* number of properties we're dealing with */
    int idx, val, val_limit, trouble_count, unfixable_trbl, did_prop;
    int trouble_list[PROP_COUNT];
    struct obj *obj = (optr ? *optr : (struct obj *) 0);

    if (obj && obj->cursed) {
        long lcount = (long) rn1(90, 10);

        switch (rn2(13) / 2) { /* case 6 is half as likely as the others */
        case 0:
            make_sick((Sick & TIMEOUT) ? (Sick & TIMEOUT) / 3L + 1L
                                       : (long) rn1(ACURR(A_CON), 20),
                      xname(obj), TRUE, SICK_NONVOMITABLE);
            break;
        case 1:
            make_blinded((Blinded & TIMEOUT) + lcount, TRUE);
            break;
        case 2:
            if (!Confusion)
                You("suddenly feel %s.",
                    Hallucination ? "trippy" : "confused");
            make_confused((HConfusion & TIMEOUT) + lcount, TRUE);
            break;
        case 3:
            make_stunned((HStun & TIMEOUT) + lcount, TRUE);
            break;
        case 4:
            if (Vomiting)
                vomit();
            else
                make_vomiting(14L, FALSE);
            break;
        case 5:
            (void) make_hallucinated((HHallucination & TIMEOUT) + lcount,
                                     TRUE, 0L);
            break;
        case 6:
            if (Deaf) /* make_deaf() won't give feedback when already deaf */
                pline("Nothing seems to happen.");
            make_deaf((HDeaf & TIMEOUT) + lcount, TRUE);
            break;
        }
        return;
    }

/*
 * Entries in the trouble list use a very simple encoding scheme.
 */
#define prop_trouble(X) trouble_list[trouble_count++] = (X)
#define TimedTrouble(P) (((P) && !((P) & ~TIMEOUT)) ? ((P) & TIMEOUT) : 0L)

    trouble_count = unfixable_trbl = did_prop = 0;

    /* collect property troubles */
    if (TimedTrouble(Sick))
        prop_trouble(SICK);
    if (TimedTrouble(Blinded) > (long) u.ucreamed
        && !(u.uswallow
             && attacktype_fordmg(u.ustuck->data, AT_ENGL, AD_BLND)))
        prop_trouble(BLINDED);
    if (TimedTrouble(HWithering))
        prop_trouble(WITHERING);
    if (TimedTrouble(LarvaCarrier))
        prop_trouble(LARVACARRIER);
    if (TimedTrouble(HHallucination))
        prop_trouble(HALLUC);
    if (TimedTrouble(Vomiting))
        prop_trouble(VOMITING);
    if (TimedTrouble(HConfusion))
        prop_trouble(CONFUSION);
    if (TimedTrouble(HStun))
        prop_trouble(STUNNED);
    if (TimedTrouble(HDeaf))
        prop_trouble(DEAF);

    if (trouble_count == 0) {
        pline1(nothing_happens);
        return;
    } else if (trouble_count > 1)
        shuffle_int_array(trouble_list, trouble_count);

    /*
     *  Chances for number of troubles to be fixed
     *               0      1      2      3      4      5      6      7
     *   blessed:  22.7%  22.7%  19.5%  15.4%  10.7%   5.7%   2.6%   0.8%
     *  uncursed:  35.4%  35.4%  22.9%   6.3%    0      0      0      0
     */
    val_limit = rn2(d(2, (obj && obj->blessed) ? 4 : 2));
    if (val_limit > trouble_count)
        val_limit = trouble_count;

    /* fix [some of] the troubles */
    for (val = 0; val < val_limit; val++) {
        idx = trouble_list[val];

        switch (idx) {
        case SICK:
            make_sick(0L, (char *) 0, TRUE, SICK_ALL);
            did_prop++;
            break;
        case WITHERING:
            You("are no longer withering away. Whew!");
            set_itimeout(&HWithering, (long) 0);
            did_prop++;
            break;
        case LARVACARRIER:
            make_carrier(0L, TRUE);
            did_prop++;
            break;
        case BLINDED:
            make_blinded((long) u.ucreamed, TRUE);
            did_prop++;
            break;
        case HALLUC:
            (void) make_hallucinated(0L, TRUE, 0L);
            did_prop++;
            break;
        case VOMITING:
            make_vomiting(0L, TRUE);
            did_prop++;
            break;
        case CONFUSION:
            make_confused(0L, TRUE);
            did_prop++;
            break;
        case STUNNED:
            make_stunned(0L, TRUE);
            did_prop++;
            break;
        case DEAF:
            make_deaf(0L, TRUE);
            did_prop++;
            break;
        default:
            impossible("use_unicorn_horn: bad trouble? (%d)", idx);
            break;
        }
    }

    if (did_prop)
        g.context.botl = TRUE;
    else
        pline("Nothing seems to happen.");

#undef PROP_COUNT
#undef prop_trouble
#undef TimedTrouble
}

/*
 * Timer callback routine: turn figurine into monster
 */
void
fig_transform(arg, timeout)
anything *arg;
long timeout;
{
    struct obj *figurine = arg->a_obj;
    struct monst *mtmp;
    coord cc;
    boolean cansee_spot, silent, okay_spot;
    boolean redraw = FALSE;
    boolean suppress_see = FALSE;
    char monnambuf[BUFSZ], carriedby[BUFSZ];

    if (!figurine) {
        debugpline0("null figurine in fig_transform()");
        return;
    }
    silent = (timeout != g.monstermoves); /* happened while away */
    okay_spot = get_obj_location(figurine, &cc.x, &cc.y, 0);
    if (figurine->where == OBJ_INVENT || figurine->where == OBJ_MINVENT)
        okay_spot = enexto(&cc, cc.x, cc.y, &mons[figurine->corpsenm]);
    if (!okay_spot || !figurine_location_checks(figurine, &cc, TRUE)) {
        /* reset the timer to try again later */
        (void) start_timer((long) rnd(5000), TIMER_OBJECT, FIG_TRANSFORM,
                           obj_to_any(figurine));
        return;
    }

    cansee_spot = cansee(cc.x, cc.y);
    mtmp = make_familiar(figurine, cc.x, cc.y, TRUE);
    if (mtmp) {
        char and_vanish[BUFSZ];
        struct obj *mshelter = g.level.objects[mtmp->mx][mtmp->my];

        /* [m_monnam() yields accurate mon type, overriding hallucination] */
        Sprintf(monnambuf, "%s", an(m_monnam(mtmp)));
        and_vanish[0] = '\0';
        if ((mtmp->minvis && !See_invisible)
            || (mtmp->data->mlet == S_MIMIC
                && M_AP_TYPE(mtmp) != M_AP_NOTHING))
            suppress_see = TRUE;

        if (mtmp->mundetected) {
            if (hides_under(mtmp->data) && mshelter) {
                Sprintf(and_vanish, " and %s under %s",
                        locomotion(mtmp->data, "crawl"), doname(mshelter));
            } else if (mtmp->data->mlet == S_MIMIC
                       || mtmp->data->mlet == S_EEL) {
                suppress_see = TRUE;
            } else
                Strcpy(and_vanish, " and vanish");
        }

        switch (figurine->where) {
        case OBJ_INVENT:
            if (Blind || suppress_see)
                You_feel("%s %s from your pack!", something,
                         locomotion(mtmp->data, "drop"));
            else
                You_see("%s %s out of your pack%s!", monnambuf,
                        locomotion(mtmp->data, "drop"), and_vanish);
            break;

        case OBJ_FLOOR:
            if (cansee_spot && !silent) {
                if (suppress_see)
                    pline("%s suddenly vanishes!", an(xname(figurine)));
                else
                    You_see("a figurine transform into %s%s!", monnambuf,
                            and_vanish);
                redraw = TRUE; /* update figurine's map location */
            }
            break;

        case OBJ_MINVENT:
            if (cansee_spot && !silent && !suppress_see) {
                struct monst *mon;

                mon = figurine->ocarry;
                /* figurine carrying monster might be invisible */
                if (canseemon(figurine->ocarry)
                    && (!mon->wormno || cansee(mon->mx, mon->my)))
                    Sprintf(carriedby, "%s pack", s_suffix(a_monnam(mon)));
                else if (is_pool(mon->mx, mon->my))
                    Strcpy(carriedby, "empty water");
                else
                    Strcpy(carriedby, "thin air");
                You_see("%s %s out of %s%s!", monnambuf,
                        locomotion(mtmp->data, "drop"), carriedby,
                        and_vanish);
            }
            break;
#if 0
        case OBJ_MIGRATING:
            break;
#endif

        default:
            impossible("figurine came to life where? (%d)",
                       (int) figurine->where);
            break;
        }
    }
    /* free figurine now */
    if (carried(figurine)) {
        useup(figurine);
    } else {
        obj_extract_self(figurine);
        obfree(figurine, (struct obj *) 0);
    }
    if (redraw)
        newsym(cc.x, cc.y);
}

static boolean
figurine_location_checks(obj, cc, quietly)
struct obj *obj;
coord *cc;
boolean quietly;
{
    xchar x, y;

    if (carried(obj) && u.uswallow) {
        if (!quietly)
            You("don't have enough room in here.");
        return FALSE;
    }
    x = cc ? cc->x : u.ux;
    y = cc ? cc->y : u.uy;
    if (!isok(x, y)) {
        if (!quietly)
            You("cannot put the figurine there.");
        return FALSE;
    }
    if (IS_ROCK(levl[x][y].typ)
        && !(passes_walls(&mons[obj->corpsenm]) && may_passwall(x, y))) {
        if (!quietly)
            You("cannot place a figurine in %s!",
                IS_TREE(levl[x][y].typ) ? "a tree" : "solid rock");
        return FALSE;
    }
    if (sobj_at(BOULDER, x, y) && !passes_walls(&mons[obj->corpsenm])
        && !throws_rocks(&mons[obj->corpsenm])) {
        if (!quietly)
            You("cannot fit the figurine on the boulder.");
        return FALSE;
    }
    return TRUE;
}

void
use_mask(optr)
struct obj **optr;
{
    register struct obj *obj = *optr;
    if (!polyok(&mons[obj->corpsenm])) {
        pline("%s violently, then splits in two!", Tobjnam(obj, "shudder"));
        useup(obj);
        return;
    }
    if (!Unchanging) {
        polymon(obj->corpsenm);
        if (obj->cursed) {
            You1(shudder_for_moment);
            losehp(rnd(30), "system shock", KILLED_BY_AN);
            pline("%s, then splits in two!", Tobjnam(obj, "shudder"));
            useup(obj);
        }
    } else {
        pline("Unfortunately, no mask will hide what you truly are.");
    }

}

static void
use_figurine(optr)
struct obj **optr;
{
    register struct obj *obj = *optr;
    xchar x, y;
    coord cc;

    if (u.uswallow) {
        /* can't activate a figurine while swallowed */
        if (!figurine_location_checks(obj, (coord *) 0, FALSE))
            return;
    }
    if (!getdir((char *) 0)) {
        g.context.move = g.multi = 0;
        return;
    }
    x = u.ux + u.dx;
    y = u.uy + u.dy;
    cc.x = x;
    cc.y = y;
    /* Passing FALSE arg here will result in messages displayed */
    if (!figurine_location_checks(obj, &cc, FALSE))
        return;
    You("%s and it %stransforms.",
        (u.dx || u.dy) ? "set the figurine beside you"
                       : (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)
                          || is_pool(cc.x, cc.y))
                             ? "release the figurine"
                             : (u.dz < 0 ? "toss the figurine into the air"
                                         : "set the figurine on the ground"),
        Blind ? "supposedly " : "");
    (void) make_familiar(obj, cc.x, cc.y, FALSE);
    (void) stop_timer(FIG_TRANSFORM, obj_to_any(obj));
    useup(obj);
    if (Blind)
        map_invisible(cc.x, cc.y);
    *optr = 0;
}

static NEARDATA const char lubricables[] = { ALL_CLASSES, ALLOW_NONE, 0 };

static void
use_grease(obj)
struct obj *obj;
{
    struct obj *otmp;

    if (Glib) {
        pline("%s from your %s.", Tobjnam(obj, "slip"),
              fingers_or_gloves(FALSE));
        dropx(obj);
        return;
    }

    if (obj->spe > 0) {
        int oldglib;

        if ((obj->cursed || Fumbling) && !rn2(2)) {
            consume_obj_charge(obj, TRUE);

            pline("%s from your %s.", Tobjnam(obj, "slip"),
                  fingers_or_gloves(FALSE));
            dropx(obj);
            return;
        }
        otmp = getobj(lubricables, "grease");
        if (!otmp)
            return;
        if (inaccessible_equipment(otmp, "grease", FALSE))
            return;
        consume_obj_charge(obj, TRUE);

        oldglib = (int) (Glib & TIMEOUT);
        if (otmp != &cg.zeroobj) {
            You("cover %s with a thick layer of grease.", yname(otmp));
            otmp->greased = 1;
            if (obj->cursed && !nohands(g.youmonst.data)) {
                make_glib(oldglib + rn1(6, 10)); /* + 10..15 */
                pline("Some of the grease gets all over your %s.",
                      fingers_or_gloves(TRUE));
            }
        } else {
            make_glib(oldglib + rn1(11, 5)); /* + 5..15 */
            You("coat your %s with grease.", fingers_or_gloves(TRUE));
        }
    } else {
        if (obj->known)
            pline("%s empty.", Tobjnam(obj, "are"));
        else
            pline("%s to be empty.", Tobjnam(obj, "seem"));
    }
    update_inventory();
}

/* touchstones - by Ken Arnold */
static void
use_stone(tstone)
struct obj *tstone;
{
    static const char scritch[] = "\"scritch, scritch\"";
    static const char allowall[3] = { COIN_CLASS, ALL_CLASSES, 0 };
    static const char coins_gems[3] = { COIN_CLASS, GEM_CLASS, 0 };
    struct obj *obj;
    boolean do_scratch;
    const char *streak_color, *choices;
    char stonebuf[QBUFSZ];
    int oclass;

    /* in case it was acquired while blinded */
    if (!Blind)
        tstone->dknown = 1;
    /* when the touchstone is fully known, don't bother listing extra
       junk as likely candidates for rubbing */
    choices = (tstone->otyp == TOUCHSTONE && tstone->dknown
               && objects[TOUCHSTONE].oc_name_known)
                  ? coins_gems
                  : allowall;
    Sprintf(stonebuf, "rub on the stone%s", plur(tstone->quan));
    if ((obj = getobj(choices, stonebuf)) == 0)
        return;

    if (obj == tstone && obj->quan == 1L) {
        You_cant("rub %s on itself.", the(xname(obj)));
        return;
    }

    if (tstone->otyp == TOUCHSTONE && tstone->cursed
        && obj->oclass == GEM_CLASS && !is_graystone(obj)
        && !obj_resists(obj, 80, 100)) {
        if (Blind)
            pline("You feel something shatter.");
        else if (Hallucination)
            pline("Oh, wow, look at the pretty shards.");
        else
            pline("A sharp crack shatters %s%s.",
                  (obj->quan > 1L) ? "one of " : "", the(xname(obj)));
        useup(obj);
        return;
    }

    if (Blind) {
        pline(scritch);
        return;
    } else if (Hallucination) {
        pline("Oh wow, man: Fractals!");
        return;
    }

    do_scratch = FALSE;
    streak_color = 0;

    oclass = obj->oclass;
    /* prevent non-gemstone rings from being treated like gems */
    if (oclass == RING_CLASS
        && objects[obj->otyp].oc_material != GEMSTONE
        && objects[obj->otyp].oc_material != MINERAL)
        oclass = RANDOM_CLASS; /* something that's neither gem nor ring */

    switch (oclass) {
    case GEM_CLASS: /* these have class-specific handling below */
    case RING_CLASS:
        if (tstone->otyp != TOUCHSTONE) {
            do_scratch = TRUE;
        } else if (obj->oclass == GEM_CLASS
                   && (tstone->blessed
                       || (!tstone->cursed && (Role_if(PM_ARCHEOLOGIST)
                                               || Race_if(PM_GNOME))))) {
            makeknown(TOUCHSTONE);
            makeknown(obj->otyp);
            prinv((char *) 0, obj, 0L);
            return;
        } else {
            /* either a ring or the touchstone was not effective */
            if (obj->material == GLASS) {
                do_scratch = TRUE;
                break;
            }
        }
        streak_color = c_obj_colors[objects[obj->otyp].oc_color];
        break; /* gem or ring */

    default:
        switch (obj->material) {
        case CLOTH:
            pline("%s a little more polished now.", Tobjnam(tstone, "look"));
            return;
        case LIQUID:
            if (!obj->known) /* note: not "whetstone" */
                You("must think this is a wetstone, do you?");
            else
                pline("%s a little wetter now.", Tobjnam(tstone, "are"));
            return;
        case WAX:
            streak_color = "waxy";
            break; /* okay even if not touchstone */
        case WOOD:
            streak_color = "wooden";
            break; /* okay even if not touchstone */
        case GOLD:
            do_scratch = TRUE; /* scratching and streaks */
            streak_color = "golden";
            break;
        case SILVER:
            do_scratch = TRUE; /* scratching and streaks */
            streak_color = "silvery";
            if (obj->otyp == MOONSTONE)
                pline("%s shinier now.", Tobjnam(tstone, "are"));
            break;
        default:
            /* Objects passing the is_flimsy() test will not
               scratch a stone.  They will leave streaks on
               non-touchstones and touchstones alike. */
            if (is_flimsy(obj))
                streak_color = c_obj_colors[objects[obj->otyp].oc_color];
            else
                do_scratch = (tstone->otyp != TOUCHSTONE);
            break;
        }
        break; /* default oclass */
    }

    Sprintf(stonebuf, "stone%s", plur(tstone->quan));
    if (do_scratch)
        You("make %s%sscratch marks on the %s.",
            streak_color ? streak_color : (const char *) "",
            streak_color ? " " : "", stonebuf);
    else if (streak_color)
        You_see("%s streaks on the %s.", streak_color, stonebuf);
    else
        pline(scritch);
    return;
}

void
reset_trapset()
{
    g.trapinfo.tobj = 0;
    g.trapinfo.force_bungle = 0;
}

/* Place a landmine/bear trap.  Helge Hafting */
static void
use_trap(otmp)
struct obj *otmp;
{
    int ttyp, tmp;
    const char *what = (char *) 0;
    char buf[BUFSZ];
    int levtyp = levl[u.ux][u.uy].typ;
    const char *occutext = "setting the trap";

    if (nohands(g.youmonst.data))
        what = "without hands";
    else if (Stunned)
        what = "while stunned";
    else if (u.uswallow)
        what =
            is_animal(u.ustuck->data) ? "while swallowed" : "while engulfed";
    else if (Underwater)
        what = "underwater";
    else if (Levitation)
        what = "while levitating";
    else if (is_pool(u.ux, u.uy))
        what = "in water";
    else if (is_lava(u.ux, u.uy))
        what = "in lava";
    else if (On_stairs(u.ux, u.uy))
        what = (u.ux == xdnladder || u.ux == xupladder) ? "on the ladder"
                                                        : "on the stairs";
    else if (IS_FURNITURE(levtyp) || IS_ROCK(levtyp)
             || closed_door(u.ux, u.uy) || t_at(u.ux, u.uy))
        what = "here";
    else if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz))
        what = (levtyp == AIR)
                   ? "in midair"
                   : (levtyp == CLOUD)
                         ? "in a cloud"
                         : "in this place"; /* Air/Water Plane catch-all */
    if (what) {
        You_cant("set a trap %s!", what);
        reset_trapset();
        return;
    }
    ttyp = (otmp->otyp == LAND_MINE) ? LANDMINE : BEAR_TRAP;
    if (otmp == g.trapinfo.tobj && u.ux == g.trapinfo.tx
                                && u.uy == g.trapinfo.ty) {
        You("resume setting %s%s.", shk_your(buf, otmp),
            trapname(ttyp, FALSE));
        set_occupation(set_trap, occutext, 0);
        return;
    }
    g.trapinfo.tobj = otmp;
    g.trapinfo.tx = u.ux, g.trapinfo.ty = u.uy;
    tmp = ACURR(A_DEX);
    g.trapinfo.time_needed =
        (tmp > 17) ? 2 : (tmp > 12) ? 3 : (tmp > 7) ? 4 : 5;
    if (Blind)
        g.trapinfo.time_needed *= 2;
    tmp = ACURR(A_STR);
    if (ttyp == BEAR_TRAP && tmp < 18)
        g.trapinfo.time_needed += (tmp > 12) ? 1 : (tmp > 7) ? 2 : 4;
    /*[fumbling and/or confusion and/or cursed object check(s)
       should be incorporated here instead of in set_trap]*/
    if (u.usteed && P_SKILL(P_RIDING) < P_BASIC) {
        boolean chance;

        if (Fumbling || otmp->cursed)
            chance = (rnl(10) > 3);
        else
            chance = (rnl(10) > 5);
        You("aren't very skilled at reaching from %s.", mon_nam(u.usteed));
        Sprintf(buf, "Continue your attempt to set %s?",
                the(trapname(ttyp, FALSE)));
        if (yn(buf) == 'y') {
            if (chance) {
                switch (ttyp) {
                case LANDMINE: /* set it off */
                    g.trapinfo.time_needed = 0;
                    g.trapinfo.force_bungle = TRUE;
                    break;
                case BEAR_TRAP: /* drop it without arming it */
                    reset_trapset();
                    You("drop %s!", the(trapname(ttyp, FALSE)));
                    dropx(otmp);
                    return;
                }
            }
        } else {
            reset_trapset();
            return;
        }
    }
    You("begin setting %s%s.", shk_your(buf, otmp), trapname(ttyp, FALSE));
    set_occupation(set_trap, occutext, 0);
    return;
}

static int
set_trap()
{
    struct obj *otmp = g.trapinfo.tobj;
    struct trap *ttmp;
    int ttyp;

    if (!otmp || !carried(otmp) || u.ux != g.trapinfo.tx
        || u.uy != g.trapinfo.ty) {
        /* ?? */
        reset_trapset();
        return 0;
    }

    if (--g.trapinfo.time_needed > 0)
        return 1; /* still busy */

    ttyp = (otmp->otyp == LAND_MINE) ? LANDMINE : BEAR_TRAP;
    ttmp = maketrap(u.ux, u.uy, ttyp);
    if (ttmp) {
        ttmp->madeby_u = 1;
        feeltrap(ttmp);
        if (*in_rooms(u.ux, u.uy, SHOPBASE)) {
            add_damage(u.ux, u.uy, 0L); /* schedule removal */
        }
        if (!g.trapinfo.force_bungle)
            You("finish arming %s.", the(trapname(ttyp, FALSE)));
        if (((otmp->cursed || Fumbling) && (rnl(10) > 5))
            || g.trapinfo.force_bungle)
            dotrap(ttmp,
                   (unsigned) (g.trapinfo.force_bungle ? FORCEBUNGLE : 0));
    } else {
        /* this shouldn't happen */
        Your("trap setting attempt fails.");
    }
    useup(otmp);
    reset_trapset();
    return 0;
}

static int
use_whip(obj)
struct obj *obj;
{
    char buf[BUFSZ];
    struct monst *mtmp;
    struct obj *otmp;
    int rx, ry, proficient, res = 0;
    const char *msg_slipsfree = "The whip slips free.";
    const char *msg_snap = "Snap!";

    if (obj != uwep) {
        if (!wield_tool(obj, "lash"))
            return 0;
        else
            res = 1;
    }
    if (!getdir((char *) 0))
        return res;

    if (u.uswallow) {
        mtmp = u.ustuck;
        rx = mtmp->mx;
        ry = mtmp->my;
    } else {
        if (Stunned || (Confusion && !rn2(5)))
            confdir();
        rx = u.ux + u.dx;
        ry = u.uy + u.dy;
        if (!isok(rx, ry)) {
            You("miss.");
            return res;
        }
        mtmp = m_at(rx, ry);
    }

    /* fake some proficiency checks */
    proficient = 0;
    if (Role_if(PM_ARCHEOLOGIST))
        ++proficient;
    if (ACURR(A_DEX) < 6)
        proficient--;
    else if (ACURR(A_DEX) >= 14)
        proficient += (ACURR(A_DEX) - 14);
    if (Fumbling)
        --proficient;
    if (proficient > 3)
        proficient = 3;
    if (proficient < 0)
        proficient = 0;

    if (u.uswallow && attack(u.ustuck)) {
        There("is not enough room to flick your whip.");

    } else if (Underwater) {
        There("is too much resistance to flick your whip.");

    } else if (u.dz < 0) {
        You("flick a bug off of the %s.", ceiling(u.ux, u.uy));

    } else if ((!u.dx && !u.dy) || (u.dz > 0)) {
        int dam;

        /* Sometimes you hit your steed by mistake */
        if (u.usteed && !rn2(proficient + 2)) {
            You("whip %s!", mon_nam(u.usteed));
            kick_steed();
            return 1;
        }
        if (is_pool_or_lava(u.ux, u.uy)) {
            You("cause a small splash.");
            if (is_lava(u.ux, u.uy))
                (void) fire_damage(uwep, FALSE, u.ux, u.uy);
            return 1;
        }
        if (Levitation || u.usteed) {
            /* Have a shot at snaring something on the floor */
            otmp = g.level.objects[u.ux][u.uy];
            if (otmp && otmp->otyp == CORPSE && otmp->corpsenm == PM_HORSE) {
                pline("Why beat a dead horse?");
                return 1;
            }
            if (otmp && proficient) {
                You("wrap your whip around %s on the %s.",
                    an(singular(otmp, xname)), surface(u.ux, u.uy));
                if (rnl(6) || pickup_object(otmp, 1L, TRUE) < 1)
                    pline1(msg_slipsfree);
                return 1;
            }
        }
        dam = rnd(2) + dbon() + obj->spe;
        if (dam <= 0)
            dam = 1;
        You("hit your %s with your whip.", body_part(FOOT));
        Sprintf(buf, "killed %sself with %s whip", uhim(), uhis());
        losehp(Maybe_Half_Phys(dam), buf, NO_KILLER_PREFIX);
        return 1;

    } else if ((Fumbling || Glib) && !rn2(5)) {
        pline_The("whip slips out of your %s.", body_part(HAND));
        dropx(obj);

    } else if (u.utrap && u.utraptype == TT_PIT) {
        /*
         * Assumptions:
         *
         * if you're in a pit
         *    - you are attempting to get out of the pit
         * or, if you are applying it towards a small monster
         *    - then it is assumed that you are trying to hit it
         * else if the monster is wielding a weapon
         *    - you are attempting to disarm a monster
         * else
         *    - you are attempting to hit the monster.
         *
         * if you're confused (and thus off the mark)
         *    - you only end up hitting.
         *
         */
        const char *wrapped_what = (char *) 0;

        if (mtmp) {
            if (bigmonst(mtmp->data)) {
                wrapped_what = strcpy(buf, mon_nam(mtmp));
            } else if (proficient) {
                if (attack(mtmp))
                    return 1;
                else
                    pline1(msg_snap);
            }
        }
        if (!wrapped_what) {
            if (IS_FURNITURE(levl[rx][ry].typ))
                wrapped_what = something;
            else if (sobj_at(BOULDER, rx, ry))
                wrapped_what = "a boulder";
        }
        if (wrapped_what) {
            coord cc;

            cc.x = rx;
            cc.y = ry;
            You("wrap your whip around %s.", wrapped_what);
            if (proficient && rn2(proficient + 2)) {
                if (!mtmp || enexto(&cc, rx, ry, g.youmonst.data)) {
                    You("yank yourself out of the pit!");
                    teleds(cc.x, cc.y, TELEDS_ALLOW_DRAG);
                    reset_utrap(TRUE);
                    g.vision_full_recalc = 1;
                }
            } else {
                pline1(msg_slipsfree);
            }
            if (mtmp)
                wakeup(mtmp, TRUE);
        } else
            pline1(msg_snap);

    } else if (mtmp) {
        if (!canspotmon(mtmp) && !glyph_is_invisible(levl[rx][ry].glyph)) {
            pline("A monster is there that you couldn't see.");
            map_invisible(rx, ry);
        }
        otmp = MON_WEP(mtmp); /* can be null */
        if (otmp) {
            char onambuf[BUFSZ];
            const char *mon_hand;
            boolean gotit = proficient && (!Fumbling || !rn2(10));

            Strcpy(onambuf, cxname(otmp));
            if (gotit) {
                mon_hand = mbodypart(mtmp, HAND);
                if (bimanual(otmp))
                    mon_hand = makeplural(mon_hand);
            } else
                mon_hand = 0; /* lint suppression */

            You("wrap your whip around %s.", yname(otmp));
            if (gotit && mwelded(otmp)) {
                pline("%s welded to %s %s%c",
                      (otmp->quan == 1L) ? "It is" : "They are", mhis(mtmp),
                      mon_hand, !otmp->bknown ? '!' : '.');
                set_bknown(otmp, 1);
                gotit = FALSE; /* can't pull it free */
            }
            if (gotit) {
                obj_extract_self(otmp);
                possibly_unwield(mtmp, FALSE);
                setmnotwielded(mtmp, otmp);

                switch (rn2(proficient + 1)) {
                case 2:
                    /* to floor near you */
                    You("yank %s to the %s!", yname(otmp),
                        surface(u.ux, u.uy));
                    place_object(otmp, u.ux, u.uy);
                    stackobj(otmp);
                    break;
                case 3:
#if 0
                    /* right to you */
                    if (!rn2(25)) {
                        /* proficient with whip, but maybe not
                           so proficient at catching weapons */
                        int hitu, hitvalu;

                        hitvalu = 8 + otmp->spe;
                        hitu = thitu(hitvalu, dmgval(otmp, &g.youmonst),
                                     &otmp, (char *)0);
                        if (hitu) {
                            pline_The("%s hits you as you try to snatch it!",
                                      the(onambuf));
                        }
                        place_object(otmp, u.ux, u.uy);
                        stackobj(otmp);
                        break;
                    }
#endif /* 0 */
                    /* right into your inventory */
                    You("snatch %s!", yname(otmp));
                    if (otmp->otyp == CORPSE
                        && touch_petrifies(&mons[otmp->corpsenm]) && !uarmg
                        && !Stone_resistance
                        && !(poly_when_stoned(g.youmonst.data)
                             && polymon(PM_STONE_GOLEM))) {
                        char kbuf[BUFSZ];

                        Sprintf(kbuf, "%s corpse",
                                an(mons[otmp->corpsenm].mname));
                        pline("Snatching %s is a fatal mistake.", kbuf);
                        instapetrify(kbuf);
                    }
                    (void) hold_another_object(otmp, "You drop %s!",
                                               doname(otmp), (const char *) 0);
                    break;
                default:
                    /* to floor beneath mon */
                    You("yank %s from %s %s!", the(onambuf),
                        s_suffix(mon_nam(mtmp)), mon_hand);
                    obj_no_longer_held(otmp);
                    place_object(otmp, mtmp->mx, mtmp->my);
                    stackobj(otmp);
                    break;
                }
            } else {
                pline1(msg_slipsfree);
            }
            wakeup(mtmp, TRUE);
        } else {
            if (M_AP_TYPE(mtmp) && !Protection_from_shape_changers
                && !sensemon(mtmp))
                stumble_onto_mimic(mtmp);
            else
                You("flick your whip towards %s.", mon_nam(mtmp));
            if (proficient) {
                if (attack(mtmp))
                    return 1;
                else
                    pline1(msg_snap);
            }
        }

    } else if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) {
        /* it must be air -- water checked above */
        You("snap your whip through thin air.");

    } else {
        pline1(msg_snap);
    }
    return 1;
}

static const char
    not_enough_room[] = "There's not enough room here to use that.",
    where_to_hit[] = "Where do you want to hit?",
    cant_see_spot[] = "won't hit anything if you can't see that spot.",
    cant_reach[] = "can't reach that spot from here.";

/* find pos of monster in range, if only one monster */
static boolean
find_poleable_mon(pos, min_range, max_range)
coord *pos;
int min_range, max_range;
{
    struct monst *mtmp;
    coord mpos;
    boolean impaired;
    int x, y, lo_x, hi_x, lo_y, hi_y, rt, glyph;

    if (Blind)
        return FALSE; /* must be able to see target location */
    impaired = (Confusion || Stunned || Hallucination);
    mpos.x = mpos.y = 0; /* no candidate location yet */
    rt = isqrt(max_range);
    lo_x = max(u.ux - rt, 1), hi_x = min(u.ux + rt, COLNO - 1);
    lo_y = max(u.uy - rt, 0), hi_y = min(u.uy + rt, ROWNO - 1);
    for (x = lo_x; x <= hi_x; ++x) {
        for (y = lo_y; y <= hi_y; ++y) {
            if (distu(x, y) < min_range || distu(x, y) > max_range
                || !isok(x, y) || !cansee(x, y))
                continue;
            glyph = glyph_at(x, y);
            if (!impaired
                && glyph_is_monster(glyph)
                && (mtmp = m_at(x, y)) != 0
                && (mtmp->mtame || (mtmp->mpeaceful && flags.confirm)))
                continue;
            if (glyph_is_monster(glyph)
                || glyph_is_warning(glyph)
                || glyph_is_invisible(glyph)
                || (glyph_is_statue(glyph) && impaired)) {
                if (mpos.x)
                    return FALSE; /* more than one candidate location */
                mpos.x = x, mpos.y = y;
            }
        }
    }
    if (!mpos.x)
        return FALSE; /* no candidate location */
    *pos = mpos;
    return TRUE;
}

static boolean
get_valid_polearm_position(x, y)
int x, y;
{
    return (isok(x, y) && ACCESSIBLE(levl[x][y].typ)
            && distu(x, y) >= g.polearm_range_min
            && distu(x, y) <= g.polearm_range_max);
}

static void
display_polearm_positions(state)
int state;
{
    if (state == 0) {
        tmp_at(DISP_BEAM, cmap_to_glyph(S_goodpos));
    } else if (state == 1) {
        int x, y, dx, dy;

        for (dx = -4; dx <= 4; dx++)
            for (dy = -4; dy <= 4; dy++) {
                x = dx + (int) u.ux;
                y = dy + (int) u.uy;
                if (get_valid_polearm_position(x, y)) {
                    tmp_at(x, y);
                }
            }
    } else {
        tmp_at(DISP_END, 0);
    }
}

/* Distance attacks by pole-weapons */
static int
use_pole(obj)
struct obj *obj;
{
    int res = 0, typ, max_range, min_range, glyph;
    coord cc;
    struct monst *mtmp;
    struct monst *hitm = g.context.polearm.hitmon;

    /* Are you allowed to use the pole? */
    if (u.uswallow) {
        pline(not_enough_room);
        return 0;
    }
    if (obj != uwep) {
        if (!wield_tool(obj, "swing"))
            return 0;
        else
            res = 1;
    }
    /* assert(obj == uwep); */

    /*
     * Calculate allowable range (pole's reach is always 2 steps):
     *  unskilled and basic: orthogonal direction, 4..4;
     *  skilled: as basic, plus knight's jump position, 4..5;
     *  expert: as skilled, plus diagonal, 4..8.
     *      ...9...
     *      .85458.
     *      .52125.
     *      9410149
     *      .52125.
     *      .85458.
     *      ...9...
     *  (Note: no roles in nethack can become expert or better
     *  for polearm skill; Yeoman in slash'em can become expert.)
     */
    min_range = 4;
    typ = uwep_skill_type();
    if (typ == P_NONE || P_SKILL(typ) <= P_BASIC)
        max_range = 4;
    else if (P_SKILL(typ) == P_SKILLED)
        max_range = 5;
    else
        max_range = 8; /* (P_SKILL(typ) >= P_EXPERT) */

    g.polearm_range_min = min_range;
    g.polearm_range_max = max_range;

    /* Prompt for a location */
    pline(where_to_hit);
    cc.x = u.ux;
    cc.y = u.uy;
    if (!find_poleable_mon(&cc, min_range, max_range) && hitm
        && !DEADMONSTER(hitm) && cansee(hitm->mx, hitm->my)
        && distu(hitm->mx, hitm->my) <= max_range
        && distu(hitm->mx, hitm->my) >= min_range) {
        cc.x = hitm->mx;
        cc.y = hitm->my;
    }
    getpos_sethilite(display_polearm_positions, get_valid_polearm_position);
    if (getpos(&cc, TRUE, "the spot to hit") < 0)
        return res; /* ESC; uses turn iff polearm became wielded */

    glyph = glyph_at(cc.x, cc.y);
    if (distu(cc.x, cc.y) > max_range) {
        pline("Too far!");
        return res;
    } else if (distu(cc.x, cc.y) < min_range) {
        pline("Too close!");
        return res;
    } else if (!cansee(cc.x, cc.y) && !glyph_is_monster(glyph)
               && !glyph_is_invisible(glyph) && !glyph_is_statue(glyph)) {
        You(cant_see_spot);
        return res;
    } else if (!couldsee(cc.x, cc.y)) { /* Eyes of the Overworld */
        You(cant_reach);
        return res;
    }

    g.context.polearm.hitmon = (struct monst *) 0;
    /* Attack the monster there */
    g.bhitpos = cc;
    if ((mtmp = m_at(g.bhitpos.x, g.bhitpos.y)) != (struct monst *) 0) {
        if (attack_checks(mtmp, uwep))
            return res;
        if (overexertion())
            return 1; /* burn nutrition; maybe pass out */
        g.context.polearm.hitmon = mtmp;
        check_caitiff(mtmp);
        g.notonhead = (g.bhitpos.x != mtmp->mx || g.bhitpos.y != mtmp->my);
        (void) thitmonst(mtmp, uwep);
    } else if (glyph_is_statue(glyph) /* might be hallucinatory */
               && sobj_at(STATUE, g.bhitpos.x, g.bhitpos.y)) {
        struct trap *t = t_at(g.bhitpos.x, g.bhitpos.y);

        if (t && t->ttyp == STATUE_TRAP
            && activate_statue_trap(t, t->tx, t->ty, FALSE)) {
            ; /* feedback has been give by animate_statue() */
        } else {
            /* Since statues look like monsters now, we say something
               different from "you miss" or "there's nobody there".
               Note:  we only do this when a statue is displayed here,
               because the player is probably attempting to attack it;
               other statues obscured by anything are just ignored. */
            pline("Thump!  Your blow bounces harmlessly off the statue.");
            wake_nearto(g.bhitpos.x, g.bhitpos.y, 25);
        }
    } else {
        /* no monster here and no statue seen or remembered here */
        (void) unmap_invisible(g.bhitpos.x, g.bhitpos.y);
        You("miss; there is no one there to hit.");
    }
    u_wipe_engr(2); /* same as for melee or throwing */
    return 1;
}

static int
use_cream_pie(obj)
struct obj *obj;
{
    boolean wasblind = Blind;
    boolean wascreamed = u.ucreamed;
    boolean several = FALSE;

    if (obj->quan > 1L) {
        several = TRUE;
        obj = splitobj(obj, 1L);
    }
    if (Hallucination)
        You("give yourself a facial.");
    else
        pline("You immerse your %s in %s%s.", body_part(FACE),
              several ? "one of " : "",
              several ? makeplural(the(xname(obj))) : the(xname(obj)));
    if (can_blnd((struct monst *) 0, &g.youmonst, AT_WEAP, obj)) {
        int blindinc = rnd(25);
        u.ucreamed += blindinc;
        make_blinded(Blinded + (long) blindinc, FALSE);
        if (!Blind || (Blind && wasblind))
            pline("There's %ssticky goop all over your %s.",
                  wascreamed ? "more " : "", body_part(FACE));
        else /* Blind  && !wasblind */
            You_cant("see through all the sticky goop on your %s.",
                     body_part(FACE));
    }

    setnotworn(obj);
    /* useup() is appropriate, but we want costly_alteration()'s message */
    costly_alteration(obj, COST_SPLAT);
    obj_extract_self(obj);
    delobj(obj);
    return 0;
}

static int
use_royal_jelly(obj)
struct obj *obj;
{
    static const char allowall[2] = { ALL_CLASSES, 0 };
    int oldcorpsenm;
    unsigned was_timed;
    struct obj *eobj;

    if (obj->quan > 1L)
        obj = splitobj(obj, 1L);
    /* remove from inventory so that it won't be offered as a choice
       to rub on itself */
    freeinv(obj);

    /* right now you can rub one royal jelly on an entire stack of eggs */
    eobj = getobj(allowall, "rub the royal jelly on");
    if (!eobj) {
        addinv(obj); /* put the unused lump back; if it came from
                      * a split, it should merge back */
        /* pline1(Never_mind); -- getobj() took care of this */
        return 0;
    }

    You("smear royal jelly all over %s.", yname(eobj));
    if (eobj->otyp != EGG) {
        pline1(nothing_happens);
        goto useup_jelly;
    }

    oldcorpsenm = eobj->corpsenm;
    if (eobj->corpsenm == PM_KILLER_BEE)
        eobj->corpsenm = PM_QUEEN_BEE;

    if (obj->cursed) {
        if (eobj->timed || eobj->corpsenm != oldcorpsenm)
            pline("The %s %s feebly.", xname(eobj), otense(eobj, "quiver"));
        else
            pline("Nothing seems to happen.");
        kill_egg(eobj);
        goto useup_jelly;
    }

    was_timed = eobj->timed;
    if (eobj->corpsenm != NON_PM) {
        if (!eobj->timed)
            attach_egg_hatch_timeout(eobj, 0L);
        /* blessed royal jelly will make the hatched creature think
           you're the parent - but has no effect if you laid the egg */
        if (obj->blessed && !eobj->spe)
            eobj->spe = 2;
    }

    if ((eobj->timed && !was_timed) || eobj->spe == 2
        || eobj->corpsenm != oldcorpsenm)
        pline("The %s %s briefly.", xname(eobj), otense(eobj, "quiver"));
    else
        pline("Nothing seems to happen.");

 useup_jelly:
    /* not useup() because we've already done freeinv() */
    setnotworn(obj);
    obfree(obj, (struct obj *) 0);
    return 1;
}

static int
use_grapple(obj)
struct obj *obj;
{
    int res = 0, typ, max_range = 4, tohit;
    boolean save_confirm;
    coord cc;
    struct monst *mtmp;
    struct obj *otmp;

    /* Are you allowed to use the hook? */
    if (u.uswallow) {
        pline(not_enough_room);
        return 0;
    }
    if (obj != uwep) {
        if (!wield_tool(obj, "cast"))
            return 0;
        else
            res = 1;
    }
    /* assert(obj == uwep); */

    /* Prompt for a location */
    pline(where_to_hit);
    cc.x = u.ux;
    cc.y = u.uy;
    if (getpos(&cc, TRUE, "the spot to hit") < 0)
        return res; /* ESC; uses turn iff grapnel became wielded */

    /* Calculate range; unlike use_pole(), there's no minimum for range */
    typ = uwep_skill_type();
    if (typ == P_NONE || P_SKILL(typ) <= P_BASIC)
        max_range = 4;
    else if (P_SKILL(typ) == P_SKILLED)
        max_range = 5;
    else
        max_range = 8;
    if (obj->oartifact == ART_GLEIPNIR)
        max_range = max_range * 3;
    if (distu(cc.x, cc.y) > max_range) {
        pline("Too far!");
        return res;
    } else if (!cansee(cc.x, cc.y)) {
        You(cant_see_spot);
        return res;
    } else if (!couldsee(cc.x, cc.y)) { /* Eyes of the Overworld */
        You(cant_reach);
        return res;
    }

    /* What do you want to hit? */
    tohit = rn2(5);
    if (typ != P_NONE && P_SKILL(typ) >= P_SKILLED) {
        winid tmpwin = create_nhwindow(NHW_MENU);
        anything any;
        char buf[BUFSZ];
        menu_item *selected;

        any = cg.zeroany; /* set all bits to zero */
        any.a_int = 1; /* use index+1 (cant use 0) as identifier */
        start_menu(tmpwin, MENU_BEHAVE_STANDARD);
        any.a_int++;
        Sprintf(buf, "an object on the %s", surface(cc.x, cc.y));
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf,
                 MENU_ITEMFLAGS_NONE);
        any.a_int++;
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "a monster",
                 MENU_ITEMFLAGS_NONE);
        any.a_int++;
        Sprintf(buf, "the %s", surface(cc.x, cc.y));
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf,
                 MENU_ITEMFLAGS_NONE);
        end_menu(tmpwin, "Aim for what?");
        tohit = rn2(4);
        if (select_menu(tmpwin, PICK_ONE, &selected) > 0
            && rn2(P_SKILL(typ) > P_SKILLED ? 20 : 2))
            tohit = selected[0].item.a_int - 1;
        free((genericptr_t) selected);
        destroy_nhwindow(tmpwin);
    }

    /* possibly scuff engraving at your feet;
       any engraving at the target location is unaffected */
    if (tohit == 2 || !rn2(2))
        u_wipe_engr(rnd(2));

    /* What did you hit? */
    switch (tohit) {
    case 0: /* Trap */
        /* FIXME -- untrap needs to deal with non-adjacent traps */
        break;
    case 1: /* Object */
        if ((otmp = g.level.objects[cc.x][cc.y]) != 0) {
            You("snag an object from the %s!", surface(cc.x, cc.y));
            (void) pickup_object(otmp, 1L, FALSE);
            /* If pickup fails, leave it alone */
            newsym(cc.x, cc.y);
            return 1;
        }
        break;
    case 2: /* Monster */
        g.bhitpos = cc;
        if ((mtmp = m_at(cc.x, cc.y)) == (struct monst *) 0)
            break;
        g.notonhead = (g.bhitpos.x != mtmp->mx || g.bhitpos.y != mtmp->my);
        save_confirm = flags.confirm;
        if (((verysmall(mtmp->data) && !rn2(4)) ||
            (obj->oartifact == ART_GLEIPNIR))
            && enexto(&cc, u.ux, u.uy, (struct permonst *) 0)) {
            flags.confirm = FALSE;
            (void) attack_checks(mtmp, uwep);
            flags.confirm = save_confirm;
            check_caitiff(mtmp); /* despite fact there's no damage */
            You("pull in %s!", mon_nam(mtmp));
            mtmp->mundetected = 0;
            rloc_to(mtmp, cc.x, cc.y);
            return 1;
        } else if ((!bigmonst(mtmp->data) && !strongmonst(mtmp->data))
                   || rn2(4)) {
            flags.confirm = FALSE;
            (void) attack_checks(mtmp, uwep);
            flags.confirm = save_confirm;
            check_caitiff(mtmp);
            (void) thitmonst(mtmp, uwep);
            return 1;
        }
    /*FALLTHRU*/
    case 3: /* Surface */
        if (IS_AIR(levl[cc.x][cc.y].typ) || is_pool(cc.x, cc.y))
            pline_The("hook slices through the %s.", surface(cc.x, cc.y));
        else {
            You("are yanked toward the %s!", surface(cc.x, cc.y));
            hurtle(sgn(cc.x - u.ux), sgn(cc.y - u.uy), 1, FALSE);
            spoteffects(TRUE);
        }
        return 1;
    default: /* Yourself (oops!) */
        if (P_SKILL(typ) <= P_BASIC) {
            You("hook yourself!");
            losehp(Maybe_Half_Phys(rn1(10, 10)), "a grappling hook",
                   KILLED_BY);
            return 1;
        }
        break;
    }
    pline1(nothing_happens);
    return 1;
}

#define BY_OBJECT ((struct monst *) 0)

/* return 1 if the wand is broken, hence some time elapsed */
static int
do_break_wand(obj)
struct obj *obj;
{
    static const char nothing_else_happens[] = "But nothing else happens...";
    register int i, x, y;
    register struct monst *mon;
    int dmg, damage;
    boolean affects_objects;
    boolean shop_damage = FALSE;
    boolean fillmsg = FALSE;
    int expltype = EXPL_MAGICAL;
    char confirm[QBUFSZ], buf[BUFSZ];
    boolean is_fragile = (!strcmp(OBJ_DESCR(objects[obj->otyp]), "balsa"));

    if (!paranoid_query(ParanoidBreakwand,
                       safe_qbuf(confirm,
                                 "Are you really sure you want to break ",
                                 "?", obj, yname, ysimple_name, "the wand")))
        return 0;

    if (nohands(g.youmonst.data)) {
        You_cant("break %s without hands!", yname(obj));
        return 0;
    } else if (ACURR(A_STR) < (is_fragile ? 5 : 10)) {
        You("don't have the strength to break %s!", yname(obj));
        return 0;
    }
    pline("Raising %s high above your %s, you %s it in two!", yname(obj),
          body_part(HEAD), is_fragile ? "snap" : "break");

    /* [ALI] Do this first so that wand is removed from bill. Otherwise,
     * the freeinv() below also hides it from setpaid() which causes problems.
     */
    if (obj->unpaid) {
        check_unpaid(obj); /* Extra charge for use */
        costly_alteration(obj, COST_DSTROY);
    }

    g.current_wand = obj; /* destroy_item might reset this */
    freeinv(obj);       /* hide it from destroy_item instead... */
    setnotworn(obj);    /* so we need to do this ourselves */

    if (!zappable(obj)) {
        pline(nothing_else_happens);
        goto discard_broken_wand;
    }
    /* successful call to zappable() consumes a charge; put it back */
    obj->spe++;
    /* might have "wrested" a final charge, taking it from 0 to -1;
       if so, we just brought it back up to 0, which wouldn't do much
       below so give it 1..3 charges now, usually making it stronger
       than an ordinary last charge (the wand is already gone from
       inventory, so perm_invent can't accidentally reveal this) */
    if (!obj->spe)
        obj->spe = rnd(3);

    obj->ox = u.ux;
    obj->oy = u.uy;
    dmg = obj->spe * 4;
    affects_objects = FALSE;

    switch (obj->otyp) {
    case WAN_WISHING:
    case WAN_NOTHING:
    case WAN_LOCKING:
    case WAN_PROBING:
    case WAN_ENLIGHTENMENT:
    case WAN_OPENING:
    case WAN_SECRET_DOOR_DETECTION:
        pline(nothing_else_happens);
        goto discard_broken_wand;
    case WAN_DEATH:
    case WAN_LIGHTNING:
    case WAN_SONICS:
        dmg *= 4;
        goto wanexpl;
    case WAN_FIRE:
        expltype = EXPL_FIERY;
        /*FALLTHRU*/
    case WAN_ACID:
    case WAN_POISON_GAS:
        if (expltype == EXPL_MAGICAL)
            expltype = EXPL_NOXIOUS;
        /*FALLTHRU*/
    case WAN_COLD:
        if (expltype == EXPL_MAGICAL)
            expltype = EXPL_FROSTY;
        dmg *= 2;
        /*FALLTHRU*/
    case WAN_MAGIC_MISSILE:
    case WAN_PSIONICS:
    wanexpl:
        explode(u.ux, u.uy, -(obj->otyp), dmg, WAND_CLASS, expltype);
        makeknown(obj->otyp); /* explode describes the effect */
        goto discard_broken_wand;
    case WAN_STRIKING:
        /* we want this before the explosion instead of at the very end */
        pline("A wall of force smashes down around you!");
        dmg = d(1 + obj->spe, 6); /* normally 2d12 */
        /*FALLTHRU*/
    case WAN_WINDSTORM:
        pline("A tornado surrounds you!");
        affects_objects = TRUE;
        break;
    case WAN_WATER:
        pline("KER-SPLOOSH!");
        affects_objects = TRUE;
        break;
    case WAN_CANCELLATION:
    case WAN_POLYMORPH:
    case WAN_TELEPORTATION:
    case WAN_UNDEAD_TURNING:
        affects_objects = TRUE;
        break;
    default:
        break;
    }

    /* magical explosion and its visual effect occur before specific effects
     */
    /* [TODO?  This really ought to prevent the explosion from being
       fatal so that we never leave a bones file where none of the
       surrounding targets (or underlying objects) got affected yet.] */
    if (obj->otyp != WAN_WINDSTORM && obj->otyp != WAN_WATER)
        explode(obj->ox, obj->oy, -(obj->otyp), rnd(dmg), WAND_CLASS,
                EXPL_MAGICAL);

    /* prepare for potential feedback from polymorph... */
    zapsetup();

    /* this makes it hit us last, so that we can see the action first */
    for (i = 0; i <= 8; i++) {
        g.bhitpos.x = x = obj->ox + xdir[i];
        g.bhitpos.y = y = obj->oy + ydir[i];
        if (!isok(x, y))
            continue;

        if (obj->otyp == WAN_DIGGING) {
            schar typ;

            if (dig_check(BY_OBJECT, FALSE, x, y)) {
                if (IS_WALL(levl[x][y].typ) || IS_DOOR(levl[x][y].typ)) {
                    /* normally, pits and holes don't anger guards, but they
                     * do if it's a wall or door that's being dug */
                    watch_dig((struct monst *) 0, x, y, TRUE);
                    if (*in_rooms(x, y, SHOPBASE))
                        shop_damage = TRUE;
                }
                /*
                 * Let liquid flow into the newly created pits.
                 * Adjust corresponding code in music.c for
                 * drum of earthquake if you alter this sequence.
                 */
                typ = fillholetyp(x, y, FALSE);
                if (typ != ROOM) {
                    levl[x][y].typ = typ, levl[x][y].flags = 0;
                    liquid_flow(x, y, typ, t_at(x, y),
                                fillmsg
                                  ? (char *) 0
                                  : "Some holes are quickly filled with %s!");
                    fillmsg = TRUE;
                } else
                    digactualhole(x, y, BY_OBJECT, (rn2(obj->spe) < 3
                                                    || (!Can_dig_down(&u.uz)
                                                        && !levl[x][y].candig))
                                                      ? PIT
                                                      : HOLE);
            }
            continue;
        } else if (obj->otyp == WAN_CREATE_MONSTER ||
                    obj->otyp == WAN_CREATE_HORDE) {
            /* u.ux,u.uy creates it near you--x,y might create it in rock */
            (void) makemon((struct permonst *) 0, u.ux, u.uy, NO_MM_FLAGS);
            continue;
        } else if (x != u.ux || y != u.uy) {
            /*
             * Wand breakage is targetting a square adjacent to the hero,
             * which might contain a monster or a pile of objects or both.
             * Handle objects last; avoids having undead turning raise an
             * undead's corpse and then attack resulting undead monster.
             * obj->bypass in bhitm() prevents the polymorphing of items
             * dropped due to monster's polymorph and prevents undead
             * turning that kills an undead from raising resulting corpse.
             */
            if ((mon = m_at(x, y)) != 0) {
                (void) bhitm(mon, obj);
                /* if (g.context.botl) bot(); */
            }
            if (affects_objects && g.level.objects[x][y]) {
                (void) bhitpile(obj, bhito, x, y, 0);
                if (g.context.botl)
                    bot(); /* potion effects */
            }
        } else {
            /*
             * Wand breakage is targetting the hero.  Using xdir[]+ydir[]
             * deltas for location selection causes this case to happen
             * after all the surrounding squares have been handled.
             * Process objects first, in case damage is fatal and leaves
             * bones, or teleportation sends one or more of the objects to
             * same destination as hero (lookhere/autopickup); also avoids
             * the polymorphing of gear dropped due to hero's transformation.
             * (Unlike with monsters being hit by zaps, we can't rely on use
             * of obj->bypass in the zap code to accomplish that last case
             * since it's also used by retouch_equipment() for polyself.)
             */
            if (affects_objects && g.level.objects[x][y]) {
                (void) bhitpile(obj, bhito, x, y, 0);
                if (g.context.botl)
                    bot(); /* potion effects */
            }
            damage = zapyourself(obj, FALSE);
            if (damage) {
                Sprintf(buf, "killed %sself by breaking a wand", uhim());
                losehp(Maybe_Half_Phys(damage), buf, NO_KILLER_PREFIX);
            }
            if (g.context.botl)
                bot(); /* blindness */
        }
    }

    /* potentially give post zap/break feedback */
    zapwrapup();

    /* Note: if player fell thru, this call is a no-op.
       Damage is handled in digactualhole in that case */
    if (shop_damage)
        pay_for_damage("dig into", FALSE);

    if (obj->otyp == WAN_LIGHT)
        litroom(TRUE, obj); /* only needs to be done once */

discard_broken_wand:
    obj = g.current_wand; /* [see dozap() and destroy_item()] */
    g.current_wand = 0;
    if (obj)
        delobj(obj);
    nomul(0);
    return 1;
}

static void
add_class(cl, class)
char *cl;
char class;
{
    char tmp[2];

    tmp[0] = class;
    tmp[1] = '\0';
    Strcat(cl, tmp);
}

static const char tools[] = { TOOL_CLASS, WEAPON_CLASS, WAND_CLASS, 0 };

/* augment tools[] if various items are carried */
static void
setapplyclasses(class_list)
char class_list[];
{
    register struct obj *otmp;
    int otyp;
    boolean knowoil, knowtouchstone;
    boolean addpotions, addstones, addfood, addspellbooks;

    knowoil = objects[POT_OIL].oc_name_known;
    knowtouchstone = objects[TOUCHSTONE].oc_name_known;
    addpotions = addstones = addfood = addspellbooks = FALSE;
    for (otmp = g.invent; otmp; otmp = otmp->nobj) {
        otyp = otmp->otyp;
        if (otyp == POT_OIL
            || (otmp->oclass == POTION_CLASS
                && (!otmp->dknown
                    || (!knowoil && !objects[otyp].oc_name_known))))
            addpotions = TRUE;
        if (otyp == TOUCHSTONE
            || (is_graystone(otmp)
                && (!otmp->dknown
                    || (!knowtouchstone && !objects[otyp].oc_name_known))))
            addstones = TRUE;
        if (otyp == CREAM_PIE || otyp == EUCALYPTUS_LEAF
            || otyp == LUMP_OF_ROYAL_JELLY)
            addfood = TRUE;
        if (otmp->oclass == SPBOOK_CLASS)
            addspellbooks = TRUE;
    }

    class_list[0] = '\0';
    if (addpotions || addstones)
        add_class(class_list, ALL_CLASSES);
    Strcat(class_list, tools);
    if (addpotions)
        add_class(class_list, POTION_CLASS);
    if (addstones)
        add_class(class_list, GEM_CLASS);
    if (addfood)
        add_class(class_list, FOOD_CLASS);
    if (addspellbooks)
        add_class(class_list, SPBOOK_CLASS);
}

/* the 'a' command */
int
doapply()
{
    struct obj *obj;
    struct obj *pseudo;
    register int res = 1;
    char class_list[MAXOCLASSES + 2];
    register boolean can_use = FALSE;

    if (nohands(g.youmonst.data)) {
        You("aren't able to use or apply tools in your current form.");
        return 0;
    }
    if (check_capacity((char *) 0))
        return 0;

    setapplyclasses(class_list); /* tools[] */
    obj = getobj(class_list, "use or apply");
    if (!obj)
        return 0;

    if (!retouch_object(&obj, FALSE))
        return 1; /* evading your grasp costs a turn; just be
                     grateful that you don't drop it as well */

    if (obj->oclass == WAND_CLASS)
        return do_break_wand(obj);

    if (obj->oclass == SPBOOK_CLASS)
        return flip_through_book(obj);

    switch (obj->otyp) {
    case BLINDFOLD:
    case LENSES:
    case MASK:
    case EARMUFFS:
        if (obj == ublindf) {
            if (!cursed(obj))
                Blindf_off(obj);
        } else if (!ublindf) {
            Blindf_on(obj);
        } else {
            You("are already %s.", ublindf->otyp == TOWEL
                                       ? "covered by a towel"
                                       : ublindf->otyp == BLINDFOLD
                                             ? "wearing a blindfold"
                                             : ublindf->otyp == EARMUFFS
                                             ? "wearing earmuffs"
                                             : ublindf->otyp == LENSES
                                             ? "wearing lenses"
                                             : "wearing a mask");
        }
        break;
    case CREAM_PIE:
        res = use_cream_pie(obj);
        break;
    case LUMP_OF_ROYAL_JELLY:
        res = use_royal_jelly(obj);
        break;
    case RAZOR_WHIP:
    case BULLWHIP:
        res = use_whip(obj);
        break;
    case GRAPPLING_HOOK:
        res = use_grapple(obj);
        break;
    case COFFIN:
    case LARGE_BOX:
    case CHEST:
    case ICE_BOX:
    case SACK:
    case BAG_OF_HOLDING:
    case OILSKIN_SACK:
        res = use_container(&obj, 1, FALSE);
        break;
    case BAG_OF_TRICKS:
    case BAG_OF_RATS:
        (void) bagotricks(obj, FALSE, (int *) 0);
        break;
    case CAN_OF_GREASE:
        use_grease(obj);
        break;
    case LOCK_PICK:
    case CREDIT_CARD:
    case SKELETON_KEY:
        res = (pick_lock(obj, 0, 0, NULL) != 0);
        break;
    case PICK_AXE:
    case DWARVISH_MATTOCK:
        res = use_pick_axe(obj);
        break;
    case TINNING_KIT:
        use_tinning_kit(obj);
        break;
    case SEWING_KIT:
        use_sewing_kit(obj);
        break;
    case LEASH:
        res = use_leash(obj);
        break;
    case SADDLE:
        res = use_saddle(obj);
        break;
    case MAGIC_WHISTLE:
        use_magic_whistle(obj);
        break;
    case PEA_WHISTLE:
        use_whistle(obj);
        break;
    case EUCALYPTUS_LEAF:
        /* MRKR: Every Australian knows that a gum leaf makes an excellent
         * whistle, especially if your pet is a tame kangaroo named Skippy.
         */
        if (obj->blessed) {
            use_magic_whistle(obj);
            /* sometimes the blessing will be worn off */
            if (!rn2(49)) {
                if (!Blind) {
                    pline("%s %s.", Yobjnam2(obj, "glow"), hcolor("brown"));
                    set_bknown(obj, 1);
                }
                unbless(obj);
            }
        } else {
            use_whistle(obj);
        }
        break;
    case STETHOSCOPE:
        res = use_stethoscope(obj);
        break;
    case MIRROR:
        res = use_mirror(obj);
        break;
    case BELL:
    case BELL_OF_OPENING:
        use_bell(&obj);
        break;
    case CANDELABRUM_OF_INVOCATION:
        use_candelabrum(obj);
        break;
    case WAX_CANDLE:
    case TALLOW_CANDLE:
    case CALLING_CANDLE:
    case AUTOMATON_CANDLE:
    case SPIRIT_CANDLE:
        use_candle(&obj);
        break;
    case OIL_LAMP:
    case MAGIC_LAMP:
    case LANTERN:
        use_lamp(obj);
        break;
    case POT_OIL:
        light_cocktail(&obj);
        break;
    case EXPENSIVE_CAMERA:
        res = use_camera(obj);
        break;
    case TOWEL:
        res = use_towel(obj);
        break;
    case CRYSTAL_BALL:
        use_crystal_ball(&obj);
        break;
    case MAGIC_MARKER:
        res = dowrite(obj);
        break;
    case TIN_OPENER:
        res = use_tin_opener(obj);
        break;
    case PLAYING_CARD_DECK:
    case DECK_OF_FATE:
        use_deck(obj);
        break;
    case FIGURINE:
        use_figurine(&obj);
        break;
    case UNICORN_HORN:
        use_unicorn_horn(&obj);
        break;
    case FLUTE:
    case MAGIC_FLUTE:
    case TOOLED_HORN:
    case FROST_HORN:
    case FIRE_HORN:
    case HORN_OF_BLASTING:
    case HARP:
    case MAGIC_HARP:
    case BUGLE:
    case LUTE:
    case BAGPIPE:
    case LEATHER_DRUM:
    case DRUM_OF_EARTHQUAKE:
        res = do_play_instrument(obj);
        break;
    case KEG:
        if (obj->spe > 0) {
            consume_obj_charge(obj, TRUE);
            pseudo = mksobj(POT_BOOZE, FALSE, FALSE);
            pseudo->blessed = obj->blessed;
            pseudo->cursed = obj->cursed;
            u.uconduct.alcohol++;
            You("chug some booze from %s.",
                    yname(obj));
            (void) peffects(pseudo);
            obfree(pseudo, (struct obj *) 0);
        } else if (Hallucination) 
            pline("Where has the rum gone?");
        else
            pline("It's empty.");
        break;
    case MEDICAL_KIT:
    		if (Role_if(PM_HEALER)) can_use = TRUE;
    		else if ((Role_if(PM_PRIEST) || Role_if(PM_MONK) ||
          Role_if(PM_SAMURAI)) &&
    			!rn2(2)) can_use = TRUE;
    		else if(!rn2(4)) can_use = TRUE;

    		if (obj->cursed && rn2(3)) can_use = FALSE;
    		if (obj->blessed && rn2(3)) can_use = TRUE;

    		makeknown(MEDICAL_KIT);
    		if (obj->cobj) {
    		    struct obj *otmp;
    		    for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
    			  if (otmp->otyp == PILL)
    			      break;
    		    if (!otmp)
    			      You_cant("find any more pills in %s.", yname(obj));
    		    else if (!is_edible(otmp))
    			      You("find, but cannot eat, a white pill in %s.",
    			        yname(obj));
    		    else {
          			check_unpaid(obj);
          			if (otmp->quan > 1L) {
          			    otmp->quan--;
          			    obj->owt = weight(obj);
          			} else {
          			    obj_extract_self(otmp);
          			    obfree(otmp, (struct obj *)0);
          			}
          			/*
          			 * Note that while white and pink pills share the
          			 * same otyp value, they are quite different.
          			 */
          			You("take a white pill from %s and swallow it.",
          				  yname(obj));
          			if (can_use) {
          			    if (Sick) make_sick(0L, (char *) 0,TRUE ,SICK_ALL);
          			    else if (Blinded > (long)(u.ucreamed+1))
          				      make_blinded(u.ucreamed ?
          					        (long)(u.ucreamed+1) : 0L, TRUE);
          			    else if (HHallucination)
          				      make_hallucinated(0L, TRUE, 0L);
          			    else if (Vomiting) make_vomiting(0L, TRUE);
          			    else if (HConfusion) make_confused(0L, TRUE);
          			    else if (HStun) make_stunned(0L, TRUE);
          			    else if (u.uhp < u.uhpmax) {
                				u.uhp += rn1(10,10);
                				if (u.uhp > u.uhpmax) u.uhp = u.uhpmax;
                				You_feel("better.");
                				g.context.botl = TRUE;
          			    } else pline1(nothing_happens);
          			} else if (!rn2(3))
          			    pline("Nothing seems to happen.");
          			else if (!Sick)
          			    make_sick(rn1(10,10), "bad pill", TRUE,
          			      SICK_VOMITABLE);
          			else {
          			    You("seem to have made your condition worse!");
          			    losehp(rn1(10,10), "a drug overdose", KILLED_BY);
          			}
    		    }
    		} else You("seem to be out of medical supplies.");
    		break;
    case HORN_OF_PLENTY: /* not a musical instrument */
        (void) hornoplenty(obj, FALSE);
        break;
    case LAND_MINE:
    case BEARTRAP:
        use_trap(obj);
        break;
    case FLINT:
    case LUCKSTONE:
    case LOADSTONE:
    case MOONSTONE:
    case TOUCHSTONE:
        use_stone(obj);
        break;
    case FRAG_GRENADE:
	case GAS_GRENADE:
		if (!obj->oarmed) {
			You("arm %s.", yname(obj));
			arm_bomb(obj, TRUE);
            update_inventory();
		} else pline("It's already armed!");
		break;
    default:
        /* Pole-weapons can strike at a distance */
        if (is_pole(obj)) {
            res = use_pole(obj);
            break;
        } else if (is_pick(obj) || is_axe(obj)) {
            res = use_pick_axe(obj);
            break;
        }
        pline("Sorry, I don't know how to use that.");
        nomul(0);
        return 0;
    }
    if (res && obj && obj->oartifact)
        arti_speak(obj);
    nomul(0);
    return res;
}

/* Keep track of unfixable troubles for purposes of messages saying you feel
 * great.
 */
int
unfixable_trouble_count(is_horn)
boolean is_horn;
{
    int unfixable_trbl = 0;

    if (Stoned)
        unfixable_trbl++;
    if (Slimed)
        unfixable_trbl++;
    if (Strangled)
        unfixable_trbl++;
    if (Wounded_legs && !u.usteed)
        unfixable_trbl++;
    /* lycanthropy is undesirable, but it doesn't actually make you feel bad
       so don't count it as a trouble which can't be fixed */

    /*
     * Unicorn horn can fix these when they're timed but not when
     * they aren't.  Potion of restore ability doesn't touch them,
     * so they're always unfixable for the not-unihorn case.
     * [Most of these are timed only, so always curable via horn.
     * An exception is Stunned, which can be forced On by certain
     * polymorph forms (stalker, bats).]
     */
    if (Sick && (!is_horn || (Sick & ~TIMEOUT) != 0L))
        unfixable_trbl++;
    if (Stunned && (!is_horn || (HStun & ~TIMEOUT) != 0L))
        unfixable_trbl++;
    if (Confusion && (!is_horn || (HConfusion & ~TIMEOUT) != 0L))
        unfixable_trbl++;
    if (Hallucination && (!is_horn || (HHallucination & ~TIMEOUT) != 0L))
        unfixable_trbl++;
    if (Vomiting && (!is_horn || (Vomiting & ~TIMEOUT) != 0L))
        unfixable_trbl++;
    if (Deaf && (!is_horn || (HDeaf & ~TIMEOUT) != 0L))
        unfixable_trbl++;

    return unfixable_trbl;
}

static int
flip_through_book(obj)
struct obj *obj;
{
    if (Underwater) {
        pline("You don't want to get the pages even more soggy, do you?");
        return 0;
    }

    You("flip through the pages of the spellbook.");

    if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
        if (Deaf) {
            You_see("the pages glow faintly %s.", hcolor(NH_RED));
        } else {
            You_hear("the pages make an unpleasant %s sound.",
                    Hallucination ? "chuckling"
                                  : "rustling");
        }
        return 1;
    } else if (Blind) {
        pline("The pages feel %s.",
              Hallucination ? "freshly picked"
                            : "rough and dry");
        return 1;
    } else if (obj->otyp == SPE_BLANK_PAPER) {
        pline("This spellbook %s.",
              Hallucination ? "doesn't have much of a plot"
                            : "has nothing written in it");
        makeknown(obj->otyp);
        return 1;
    }

    if (Hallucination) {
        You("enjoy the animated initials.");
    } else {
        static const char* fadeness[] = {
            "fresh",
            "slightly faded",
            "very faded",
            "extremely faded",
            "barely visible"
        };

        int index = min(obj->spestudied, MAX_SPELL_STUDY);
        pline("The%s ink in this spellbook is %s.",
              objects[obj->otyp].oc_magic ? " magical" : "",
              fadeness[index]);
    }

    return 1;
}

/*apply.c*/
