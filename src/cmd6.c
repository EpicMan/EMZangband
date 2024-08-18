/* File: cmd6.c */

/* Purpose: Object commands */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "angband.h"
#include "script.h"


/*
 * This file includes code for eating food, drinking potions,
 * reading scrolls, aiming wands, using staffs, zapping rods,
 * and activating artifacts.
 *
 * In all cases, if the player becomes "aware" of the item's use
 * by testing it, mark it as "aware" and reward some experience
 * based on the object's level, always rounding up.  If the player
 * remains "unaware", mark that object "kind" as "tried".
 *
 * This code now correctly handles the unstacking of wands, staffs,
 * and rods.  Note the overly paranoid warning about potential pack
 * overflow, which allows the player to use and drop a stacked item.
 *
 * In all "unstacking" scenarios, the "used" object is "carried" as if
 * the player had just picked it up.  In particular, this means that if
 * the use of an item induces pack overflow, that item will be dropped.
 *
 * For simplicity, these routines induce a full "pack reorganization"
 * which not only combines similar items, but also reorganizes various
 * items to obey the current "sorting" method.  This may require about
 * 400 item comparisons, but only occasionally.
 *
 * There may be a BIG problem with any "effect" that can cause "changes"
 * to the inventory.  For example, a "scroll of recharging" can cause
 * a wand/staff to "disappear", moving the inventory up.  Luckily, the
 * scrolls all appear BEFORE the staffs/wands, so this is not a problem.
 * But, for example, a "staff of recharging" could cause MAJOR problems.
 * In such a case, it will be best to either (1) "postpone" the effect
 * until the end of the function, or (2) "change" the effect, say, into
 * giving a staff "negative" charges, or "turning a staff into a stick".
 * It seems as though a "rod of recharging" might in fact cause problems.
 * The basic problem is that the act of recharging (and destroying) an
 * item causes the inducer of that action to "move", causing "o_ptr" to
 * no longer point at the correct item, with horrifying results.
 *
 * Note that food/potions/scrolls no longer use bit-flags for effects,
 * but instead use the "sval" (which is also used to sort the objects).
 */

static void do_cmd_eat_food_aux(object_type *o_ptr)
{
	bool ident = FALSE;

	/* Sound */
	sound(SOUND_EAT);

	/* Take a turn */
	p_ptr->state.energy_use = 100;

	/* Eat the food */
	switch (o_ptr->sval)
	{
	case SV_FOOD_BLINDNESS:
		if (!FLAG(p_ptr, TR_RES_BLIND))
		{
			ident = TRUE;
			inc_blind(200 + randint0(201));
		}
		break;

	case SV_FOOD_PARANOIA:
		if (!FLAG(p_ptr, TR_RES_FEAR))
		{
			ident = TRUE;
			inc_afraid(10 + randint0(11));
		}
		break;

	case SV_FOOD_CONFUSION:
		if (!FLAG(p_ptr, TR_RES_CONF))
		{
			ident = TRUE;
			inc_confused(10 + randint0(11));
		}
		break;

	case SV_FOOD_HALLUCINATION:
		if (!FLAG(p_ptr, TR_RES_CHAOS))
		{
			ident = TRUE;
			inc_image(250 + randint0(251));
		}
		break;

	case SV_FOOD_CURE_POISON:
		ident = clear_poisoned();
		break;

	case SV_FOOD_CURE_BLINDNESS:
		ident = clear_blind();
		break;

	case SV_FOOD_CURE_PARANOIA:
		ident = clear_afraid();
		break;

	case SV_FOOD_CURE_CONFUSION:
		ident = clear_confused();
		break;

	case SV_FOOD_WEAKNESS:
		ident = TRUE;
		do_dec_stat(A_STR);
		take_hit(damroll(6, 6), "poisonous food");
		break;

	case SV_FOOD_UNHEALTH:
		ident = TRUE;
		do_dec_stat(A_CON);
		take_hit(damroll(10, 10), "poisonous food");
		break;

	case SV_FOOD_RESTORE_CON:
		ident = do_res_stat(A_CON);
		break;

	case SV_FOOD_RESTORING:
		if (do_res_stat(A_STR)) ident = TRUE;
		if (do_res_stat(A_INT)) ident = TRUE;
		if (do_res_stat(A_WIS)) ident = TRUE;
		if (do_res_stat(A_DEX)) ident = TRUE;
		if (do_res_stat(A_CON)) ident = TRUE;
		if (do_res_stat(A_CHR)) ident = TRUE;
		break;

	case SV_FOOD_STUPIDITY:
		ident = TRUE;
		do_dec_stat(A_INT);
		take_hit(damroll(8, 8), "poisonous food");
		break;

	case SV_FOOD_NAIVETY:
		ident = TRUE;
		do_dec_stat(A_WIS);
		take_hit(damroll(8, 8), "poisonous food");
		break;

	case SV_FOOD_POISON:
		if (res_pois_lvl())
		{
			ident = TRUE;
			inc_poisoned(10 + randint0(11));
		}
		break;

	case SV_FOOD_SICKNESS:
		ident = TRUE;
		do_dec_stat(A_CON);
		take_hit(damroll(6, 6), "poisonous food");
		break;

	case SV_FOOD_PARALYSIS:
		if (!FLAG(p_ptr, TR_FREE_ACT))
		{
			ident = TRUE;
			inc_paralyzed(10 + randint0(11));
		}
		break;

	case SV_FOOD_RESTORE_STR:
		ident = do_res_stat(A_STR);
		break;

	case SV_FOOD_DISEASE:
		ident = TRUE;
		do_dec_stat(A_STR);
		take_hit(damroll(10, 19), "poisonous food");
		break;

	case SV_FOOD_CURE_SERIOUS:
		ident = hp_player(damroll(4, 8));
		break;

	case SV_FOOD_RATION:
	case SV_FOOD_BISCUIT:
	case SV_FOOD_JERKY:
	case SV_FOOD_SLIME_MOLD:
	case SV_FOOD_PINT_OF_ALE:
	case SV_FOOD_PINT_OF_WINE:
		msgf("That tastes good.");
		break;

	case SV_FOOD_WAYBREAD:
		msgf("That tastes good.");
		clear_poisoned();
		hp_player(damroll(4, 8));
		break;
	}

	/* Is Identity known? */
	if (!ident) ident = object_aware_p(o_ptr);

	if (!(object_aware_p(o_ptr)))
	{
		chg_virtue(V_PATIENCE, -1);
		chg_virtue(V_CHANCE, 1);
	}

	/* We have tried it */
	object_tried(o_ptr);

	/* The player is now aware of the object */
	if (ident && !object_aware_p(o_ptr))
	{
		/* Object level */
		int lev = get_object_level(o_ptr);

		object_aware(o_ptr);
		gain_exp((lev + p_ptr->lev / 2) / p_ptr->lev);
	}
	
	/* Notice changes */
	notice_item();	

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	/* Food can feed the player */
	if (p_ptr->rp.prace == RACE_VAMPIRE)
	{
		/* Reduced nutritional benefit */
		(void)set_food(p_ptr->food + (o_ptr->pval / 10));
		msgf
			("Mere victuals hold scant sustenance for a being such as yourself.");
		if (p_ptr->food < PY_FOOD_ALERT)	/* Hungry */
			msgf("Your hunger can only be satisfied with fresh blood!");
	}
	else if (FLAG(p_ptr, TR_CANT_EAT))
	{
		if (p_ptr->rp.prace == RACE_SKELETON)
		{
			if (!((o_ptr->sval == SV_FOOD_WAYBREAD) ||
				  (o_ptr->sval < SV_FOOD_BISCUIT)))
			{
				object_type *q_ptr;
	
				msgf("The food falls through your jaws!");

				/* Create the item */
				q_ptr = object_prep(lookup_kind(o_ptr->tval, o_ptr->sval));

				/* Drop the object from heaven */
				drop_near(q_ptr, -1, p_ptr->px, p_ptr->py);
			}
			else
			{
				msgf("The food falls through your jaws and vanishes!");
			}
		}
		else if ((p_ptr->rp.prace == RACE_GOLEM) ||
				 (p_ptr->rp.prace == RACE_ZOMBIE) ||
				 (p_ptr->rp.prace == RACE_SPECTRE) || (p_ptr->rp.prace == RACE_GHOUL))
		{
			msgf("The food of mortals is poor sustenance for you.");
			(void)set_food(p_ptr->food + ((o_ptr->pval) / 20));
		}
		else
		{
			msgf("This food is poor sustenance for you.");
			set_food(p_ptr->food + ((o_ptr->pval) / 20));
		}
	}
	else
	{
		(void)set_food(p_ptr->food + o_ptr->pval);
	}

	/* Destroy a food item */
	item_increase(o_ptr, -1);

	make_noise(1);
}


/*
 * Eat some food (from the pack or floor)
 */
void do_cmd_eat_food(void)
{
	object_type *o_ptr;
	cptr q, s;


	/* Restrict choices to food */
	item_tester_tval = TV_FOOD;

	/* Get an item */
	q = "Eat which item? ";
	s = "You have nothing to eat.";

	o_ptr = get_item(q, s, (USE_INVEN | USE_FLOOR));

	/* Not a valid item */
	if (!o_ptr) return;

	/* Eat the object */
	do_cmd_eat_food_aux(o_ptr);
}

bool cure_wounds(int hp, bool cure_blind, bool cure_confusion, bool cure_poisoned)
{
	bool ident = hp_player(hp);

	if (cure_blind)
		ident |= clear_blind();

	if (cure_confusion)
		ident |= clear_confused();

	if (cure_poisoned)
	{
		ident |= clear_poisoned();
		ident |= clear_stun();
		ident |= clear_cut();
	}
	else if (cure_confusion)
	{
		ident |= inc_cut(-50);
	}
	else if (cure_blind)
	{
		ident |= inc_cut(-10);
	}

	return ident;
}

static bool restore_mana()
{
	bool ident = FALSE;
	if (p_ptr->csp < p_ptr->msp)
	{
		p_ptr->csp = p_ptr->msp;
		p_ptr->csp_frac = 0;
		msgf("You feel your head clear.");
		p_ptr->redraw |= PR_MANA;
		p_ptr->window |= PW_PLAYER;
		p_ptr->window |= PW_SPELL;
		ident = TRUE;
	}

		return ident;
}

static void do_life_potion()
{
	msgf("You feel life flow through your body!");
	restore_level();
	clear_poisoned();
	clear_blind();
	clear_confused();
	clear_image();
	clear_stun();
	clear_cut();
	do_res_stat(A_STR);
	do_res_stat(A_CON);
	do_res_stat(A_DEX);
	do_res_stat(A_WIS);
	do_res_stat(A_INT);
	do_res_stat(A_CHR);

	/*Recalculate max.hitpoints*/
	update_stuff();
	hp_player(5000);
}

/*
 * Quaff a potion (from the pack or the floor)
 */
static void do_cmd_quaff_potion_aux(object_type *o_ptr)
{
	bool ident;

	/* Sound */
	sound(SOUND_QUAFF);

	/* Take a turn */
	p_ptr->state.energy_use = 100;

	/* Is Identity known? */
	ident = object_aware_p(o_ptr);

	/* Quaff the potion */
	switch (o_ptr->sval)
	{
	case SV_POTION_SLIME_MOLD:
	case SV_POTION_APPLE_JUICE:
	case SV_POTION_WATER:
		msgf("You feel less thirsty.");
		ident = TRUE;
		break;

	case SV_POTION_INC_STR:
		ident = do_inc_stat(A_STR);
		break;

	case SV_POTION_DEC_STR:
		ident = do_dec_stat(A_STR);
		break;

	case SV_POTION_RES_STR:
		ident = do_res_stat(A_STR);
		break;

	case SV_POTION_INC_INT:
		ident = do_inc_stat(A_INT);
		break;

	case SV_POTION_DEC_INT:
		ident = do_dec_stat(A_INT);
		break;

	case SV_POTION_RES_INT:
		ident = do_res_stat(A_INT);
		break;

	case SV_POTION_INC_WIS:
		ident = do_inc_stat(A_WIS);
		break;

	case SV_POTION_DEC_WIS:
		ident = do_dec_stat(A_WIS);
		break;

	case SV_POTION_RES_WIS:
		ident = do_res_stat(A_WIS);
		break;

	case SV_POTION_INC_CHR:
		ident = do_inc_stat(A_CHR);
		break;

	case SV_POTION_DEC_CHR:
		ident = do_dec_stat(A_CHR);
		break;

	case SV_POTION_RES_CHR:
		ident = do_res_stat(A_CHR);
		break;

	case SV_POTION_CURING:
		ident = cure_wounds(150, TRUE, TRUE, TRUE);
		ident |= clear_image();
		break;

	case SV_POTION_INVULNERABILITY:
		ident = TRUE;
		inc_invuln(7 + randint(8));
		break;

	case SV_POTION_NEW_LIFE:
		do_cmd_rerate();
		if (p_ptr->muta1 || p_ptr->muta2 || p_ptr->muta3)
			msgf("You are cured of all mutations.");
		p_ptr->muta1 = 0;
		p_ptr->muta2 = 0;
		p_ptr->muta3 = 0;
		p_ptr->update |= (PU_BONUS);
		handle_stuff();
		ident = TRUE;
		break;

	case SV_POTION_CURE_SERIOUS:
		ident = cure_wounds(75, TRUE, TRUE, FALSE);
		break;

	case SV_POTION_CURE_CRITICAL:
		ident = cure_wounds(150, TRUE, TRUE, TRUE);
		break;

	case SV_POTION_HEALING:
		ident = cure_wounds(300, TRUE, TRUE, TRUE);
		break;

	case SV_POTION_INC_CON:
		ident = do_inc_stat(A_CON);
		break;

	case SV_POTION_DEC_CON:
		ident = do_dec_stat(A_CON);
		break;

	case SV_POTION_RES_CON:
		ident = do_res_stat(A_CON);
		break;

	case SV_POTION_EXPERIENCE:
		ident = TRUE;
		if (p_ptr->exp < PY_MAX_EXP)
		{
			int ee = MIN(100000, 10 + p_ptr->exp / 2);
			msgf("You feel more experienced.");
			gain_exp(ee);
		}
	break;

	case SV_POTION_SLEEP:
		msgf("You fall asleep.");
		if (ironman_nightmare)
		{
			msgf("A horrible vision enters your mind.");
			have_nightmare();
		}
		inc_paralyzed(4 + randint0(5));
		ident = TRUE;
		break;

	case SV_POTION_BLINDNESS:
		if (!FLAG(p_ptr, TR_RES_BLIND))
		{
			ident = TRUE;
			inc_blind(100 + randint0(101));
		}
		break;

	case SV_POTION_CONFUSION:
		if (!FLAG(p_ptr, TR_RES_CONF) && inc_confused(15 + randint0(21))) ident = TRUE;
		if (!FLAG(p_ptr, TR_RES_CHAOS))
		{
			if (one_in_(2))
				if (inc_image(150 + randint0(151)))
					ident = TRUE;

			if (one_in_(13))
			{
				ident = TRUE;
				if (p_ptr->depth)
				{
					if (one_in_(3)) lose_all_info();
					teleport_player(250);
					wiz_dark();
				}
				else 
					teleport_player(250);

				msgf("You wake up somewhere with a sore head...");
				msgf("You can't remember a thing, or how you got here!");
			}
		}
		break;

	case SV_POTION_POISON:
		if (res_pois_lvl())
			ident = inc_poisoned(10 + randint0(16));
		break;

	case SV_POTION_SPEED:
		ident = inc_fast(15 + randint0(26));
		break;

	case SV_POTION_SLOWNESS:
		ident = inc_slow(15 + randint0(26));
		break;

	case SV_POTION_INC_DEX:
		ident = do_inc_stat(A_DEX);
		break;

	case SV_POTION_DEC_DEX:
		ident = do_dec_stat(A_DEX);
		break;

	case SV_POTION_RES_DEX:
		ident = do_res_stat(A_DEX);
		break;

	case SV_POTION_LOSE_MEMORIES:
		if (!FLAG(p_ptr, TR_HOLD_LIFE))
		{
			msgf("You feel your memories fade.");
			lose_exp(p_ptr->exp / 4);
			ident = TRUE;
		}
		break;

	case SV_POTION_SALT_WATER:
		ident = TRUE;
		msgf("The potion makes you vomit!");
		if (p_ptr->food >= PY_FOOD_STARVE) set_food(PY_FOOD_STARVE);
		clear_poisoned();
		inc_paralyzed(4);
		break;

	case SV_POTION_ENLIGHTENMENT:
		msgf("An image of your surroundings forms in your mind...");
		wiz_lite();
		ident = TRUE;
		break;

	case SV_POTION_BERSERK_STRENGTH:
		ident = clear_afraid();
		if (inc_shero(25 + randint0(26))) ident = TRUE;
		if (hp_player(30)) ident = TRUE;
		break;

	case SV_POTION_BOLDNESS:
		ident = clear_afraid();
		break;

	case SV_POTION_RESTORE_EXP:
		ident = restore_level();
		break;

	case SV_POTION_RESIST_HEAT:
		ident = inc_oppose_fire(10 + randint0(11));
		break;

	case SV_POTION_RESIST_COLD:
		ident = inc_oppose_cold(10 + randint0(11));
		break;

	case SV_POTION_DETECT_INVIS:
		ident = inc_tim_invis(12 + randint0(13));
		break;

	case SV_POTION_SLOW_POISON:
		ident = inc_poisoned(-1 * (p_ptr->tim.poisoned / 2 + 1));
		break;

	case SV_POTION_CURE_POISON:
		ident = clear_poisoned();
		break;

	case SV_POTION_RESTORE_MANA:
		if (p_ptr->csp < p_ptr->msp)
		{
			ident = TRUE;
			p_ptr->csp = p_ptr->msp;
			p_ptr->csp_frac = 0;
			msgf("You feel your head clear.");
			p_ptr->redraw |= PR_MANA;
			p_ptr->window |= (PW_PLAYER | PW_SPELL);
		}
		break;

	case SV_POTION_INFRAVISION:
		ident = inc_tim_infra(100 + randint0(101));
		break;

	case SV_POTION_RESISTANCE:
	{
		int duration = 20 + randint0(21);
		ident = inc_oppose_acid(duration);
		if (inc_oppose_fire(duration)) ident = TRUE;
		if (inc_oppose_cold(duration)) ident = TRUE;
		if (inc_oppose_elec(duration)) ident = TRUE;
	}
	break;

	case SV_POTION_DEATH:
		msgf("A feeling of Death flows through your body.");
			take_hit(5000, "a potion of Death");
			ident = TRUE;
			break;

	case SV_POTION_RUINATION:
		msgf("Your nerves and muscles feel weak and lifeless!");
		take_hit(damroll(10, 10), "a potion of Ruination");
		dec_stat(A_DEX, 25, TRUE);
		dec_stat(A_WIS, 25, TRUE);
		dec_stat(A_CON, 25, TRUE);
		dec_stat(A_STR, 25, TRUE);
		dec_stat(A_CHR, 25, TRUE);
		dec_stat(A_INT, 25, TRUE);
		ident = TRUE;
		break;

	case SV_POTION_DETONATIONS:
		msgf("Massive explosions rupture your body!");
		take_hit(damroll(50, 20), "a potion of Detonations");
		inc_stun(75);
		inc_cut(5000);
		ident = TRUE;
		break;

	case SV_POTION_AUGMENTATION:
		ident |= do_inc_stat(A_STR);
		ident |= do_inc_stat(A_INT);
		ident |= do_inc_stat(A_WIS);
		ident |= do_inc_stat(A_DEX);
		ident |= do_inc_stat(A_CON);
		ident |= do_inc_stat(A_CHR);
		break;

	case SV_POTION_STAR_HEALING:
		ident = cure_wounds(1200, TRUE, TRUE, TRUE);
		break;

	case SV_POTION_LIFE:
		do_life_potion();
		break;

	case SV_POTION_SELF_KNOWLEDGE:
		msgf("You begin to know yourself a little better...");
		message_flush();
		self_knowledge();
		ident = TRUE;
		break;

	case SV_POTION_STAR_ENLIGHTENMENT:
		msgf("You begin to feel more enlightened...");
		message_flush();
		wiz_lite();
		do_inc_stat(A_INT);
		do_inc_stat(A_WIS);
		detect_traps(TRUE); detect_doors(); detect_stairs();
		detect_treasure(); detect_objects_gold(); detect_objects_normal();
		identify_pack();
		self_knowledge();
		ident = TRUE;
		break;

	case SV_POTION_CURE_LIGHT:
		ident = cure_wounds(38, TRUE, FALSE, FALSE);
		break;
	}


	if (p_ptr->rp.prace == RACE_SKELETON)
	{
		msgf("Some of the fluid falls through your jaws!");
		(void)potion_smash_effect(0, p_ptr->px, p_ptr->py, o_ptr);
	}

	if (!(object_aware_p(o_ptr)))
	{
		chg_virtue(V_PATIENCE, -1);
		chg_virtue(V_CHANCE, 1);
	}

	/* The item has been tried */
	object_tried(o_ptr);

	/* An identification was made */
	if (ident && !object_aware_p(o_ptr))
	{
		/* Object level */
		int lev = get_object_level(o_ptr);

		object_aware(o_ptr);
		gain_exp((lev + p_ptr->lev / 2) / p_ptr->lev);
	}

	/* Notice changes */
	notice_item();

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	/* Potions can feed the player */
	switch (p_ptr->rp.prace)
	{
		case RACE_VAMPIRE:
			(void)set_food(p_ptr->food + (o_ptr->pval / 10));
			break;
		case RACE_SKELETON:
			/* Do nothing */
			break;
		case RACE_GOLEM:
		case RACE_ZOMBIE:
		case RACE_SPECTRE:
		case RACE_GHOUL:
			(void)set_food(p_ptr->food + ((o_ptr->pval) / 20));
			break;
		default:
			(void)set_food(p_ptr->food + o_ptr->pval);
	}

	/* Reduce and describe items */
	item_increase(o_ptr, -1);

	make_noise(1);
}


void do_cmd_quaff_potion(void)
{
	object_type *o_ptr;
	cptr q, s;

	/* Restrict choices to potions */
	item_tester_tval = TV_POTION;

	/* Get an item */
	q = "Quaff which potion? ";
	s = "You have no potions to quaff.";

	o_ptr = get_item(q, s, (USE_INVEN | USE_FLOOR));

	/* Not a valid item */
	if (!o_ptr) return;

	/* Quaff the potion */
	do_cmd_quaff_potion_aux(o_ptr);
}

static bool summon_monsters(int num, int kind)
{
	bool ident = FALSE;

	for (int k = 0; k < num; k++)
	{
		if (summon_specific(0, p_ptr->px, p_ptr->py, p_ptr->depth, kind, TRUE, FALSE, FALSE))
			ident = TRUE;
	}

	return ident;
}

/*
 * Read a scroll (from the pack or floor).
 *
 * Certain scrolls can be "aborted" without losing the scroll.  These
 * include scrolls with no effects but recharge or identify, which are
 * cancelled before use.  XXX Reading them still takes a turn, though.
 */
static void do_cmd_read_scroll_aux(object_type *o_ptr)
{
	object_type temp, *j_ptr;
	bool ident = FALSE, used_up = TRUE;

	/* Take a turn */
	p_ptr->state.energy_use = 100;

	/* Is Identity known? */
	ident = object_aware_p(o_ptr);

	/* Copy item into temp */
	COPY(&temp, o_ptr, object_type);

	/* Read the scroll */

	switch (o_ptr->sval)
	{
	case SV_SCROLL_ENCHANT_WEAPON_TO_HIT:
		used_up = enchant_spell(1, 0, 0);
		break;

	case SV_SCROLL_ENCHANT_WEAPON_TO_DAM:
		used_up = enchant_spell(0, 1, 0);
		break;

	case SV_SCROLL_ENCHANT_ARMOR:
		used_up = enchant_spell(0, 0, 1);
		break;

	case SV_SCROLL_IDENTIFY:
		used_up = ident_scroll(o_ptr->k_idx);
		break;

	case SV_SCROLL_STAR_IDENTIFY:
		used_up = identify_fully();
		break;

	case SV_SCROLL_RUMOR:
		msgf("There is a message on the scroll. It says:"); message_flush();
		{
			errr err;
			char buf[1024];

			switch (randint1(20))
			{
			case 1:
				err = get_rnd_line("chainswd.txt", 0, buf);
				break;
			case 2:
				err = get_rnd_line("error.txt", 0, buf);
				break;
			case 3:
			case 4:
			case 5:
				err = get_rnd_line("death.txt", 0, buf);
				break;
			default:
				err = get_rnd_line("rumors.txt", 0, buf);
				break;
			}

			/* An error occured */
			if (err) strcpy(buf, "Some rumors are wrong.");
	
			msgf(buf); message_flush();
		}
		msgf("The scroll disappears in a puff of smoke!");
		ident = TRUE;
		break;

	case SV_SCROLL_CHAOS:
		fire_ball(GF_CHAOS, 0, 400, 4);
		if (!FLAG(p_ptr, TR_RES_CHAOS))
			take_hit(rand_range(150, 300), "a Scroll of Logrus");
		ident = TRUE;
		break;

	case SV_SCROLL_REMOVE_CURSE:
		if (remove_curse())
		{
			msgf("You feel as if someone is watching over you.");
			ident = TRUE;
		}
		break;

	case SV_SCROLL_LIGHT:
		ident = lite_area(damroll(2, 8), 2);
		break;

	case SV_SCROLL_FIRE:
		fire_ball(GF_FIRE, 0, 350, 4);
		if (res_fire_lvl() > 3)
			take_hit(rand_range(100, 200), "a Scroll of Fire");
		ident = TRUE;
		break;
	case SV_SCROLL_ICE:
		fire_ball(GF_COLD, 0, 300, 4);
		if (res_cold_lvl() > 3)
			take_hit(rand_range(50, 100), "a Scroll of Ice");
		ident = TRUE;
		break;

	case SV_SCROLL_SUMMON_MONSTER:
		ident = summon_monsters(randint1(3), 0);
		break;

	case SV_SCROLL_PHASE_DOOR:
		teleport_player(10);
		ident = TRUE;
		break;

	case SV_SCROLL_TELEPORT:
		teleport_player(100);
		ident = TRUE;
		break;

	case SV_SCROLL_TELEPORT_LEVEL:
		teleport_player_level();
		ident = TRUE;
		break;

	case SV_SCROLL_MONSTER_CONFUSION:
		if (!p_ptr->state.confusing)
		{
			msgf("Your hands begin to glow.");
			p_ptr->state.confusing = TRUE;
			p_ptr->redraw |= PR_STATUS;
			ident = TRUE;
		}
		break;

	case SV_SCROLL_MAPPING:
		ident = TRUE;
		map_area();
		break;

	case SV_SCROLL_RUNE_OF_PROTECTION:
		used_up = warding_glyph();
		ident = TRUE;
		break;

	case SV_SCROLL_STAR_REMOVE_CURSE:
		if (remove_all_curse())
		{
			ident = TRUE;
			msgf("You feel as if someone is watching over you.");
		}
		break;

	case SV_SCROLL_DETECT_GOLD:
		ident = detect_treasure();
		if (detect_objects_gold()) ident = TRUE;
		break;

	case SV_SCROLL_DETECT_ITEM:
		ident = detect_objects_normal();
		break;

	case SV_SCROLL_DETECT_TRAP:
		ident = detect_traps(ident);
		break;

	case SV_SCROLL_DETECT_DOOR:
		ident = detect_doors();
		if (detect_stairs()) ident = TRUE;
		break;

	case SV_SCROLL_ACQUIREMENT:
		ident = TRUE;
		acquirement(p_ptr->px, p_ptr->py, 1, TRUE, FALSE);
		break;

	case SV_SCROLL_STAR_ACQUIREMENT:
		ident = TRUE;
		acquirement(p_ptr->px, p_ptr->py, rand_range(2, 3), TRUE, FALSE);
		break;

	case SV_SCROLL_MASS_GENOCIDE:
		ident = TRUE;
		mass_genocide(TRUE);
		break;

	case SV_SCROLL_DETECT_INVIS:
		ident = detect_monsters_invis();
		break;

	case SV_SCROLL_AGGRAVATE_MONSTER:
		ident = TRUE;
		msgf("There is a high pitched humming noise.");
		aggravate_monsters(0);
		break;

	case SV_SCROLL_TRAP_CREATION:
		ident = trap_creation();
		break;

	case SV_SCROLL_TRAP_DOOR_DESTRUCTION:
		ident = destroy_doors_touch();
		break;

	case SV_SCROLL_ARTIFACT:
		used_up = artifact_scroll();
		ident = TRUE;
		break;

	case SV_SCROLL_RECHARGING:
		ident = TRUE;
		used_up = recharge(130);
		break;

	case SV_SCROLL_GENOCIDE:
		ident = TRUE;
		genocide(TRUE);
		break;

	case SV_SCROLL_DARKNESS:
		ident = unlite_area(10, 3);
		if (!FLAG(p_ptr, TR_RES_BLIND) && !FLAG(p_ptr, TR_RES_DARK))
			inc_blind(rand_range(3, 8));
		break;
		
	case SV_SCROLL_PROTECTION_FROM_EVIL:
		ident = inc_protevil(3 * p_ptr->lev + randint1(25));
		break;

	case SV_SCROLL_SATISFY_HUNGER:
		ident = set_food(PY_FOOD_MAX - 1);
		break;

	case SV_SCROLL_DISPEL_UNDEAD:
		ident = dispel_undead(60);
		break;

	case SV_SCROLL_STAR_ENCHANT_WEAPON:
		used_up = enchant_spell(randint1(5), randint1(5), 0);
		ident = TRUE;
		break;

	case SV_SCROLL_CURSE_WEAPON:
		ident = curse_weapon();
		break;

	case SV_SCROLL_STAR_ENCHANT_ARMOR:
		used_up = enchant_spell(0, 0, rand_range(2, 7));
		ident = TRUE;
		break;

	case SV_SCROLL_CURSE_ARMOR:
		ident = curse_armor();
		break;

	case SV_SCROLL_SUMMON_UNDEAD:
		ident = summon_monsters(randint1(3), SUMMON_UNDEAD);
		break;

	case SV_SCROLL_BLESSING:
		ident = inc_blessed(rand_range(6, 18));
		break;

	case SV_SCROLL_HOLY_CHANT:
		ident = inc_blessed(rand_range(12, 36));
		break;

	case SV_SCROLL_HOLY_PRAYER:
		ident = inc_blessed(rand_range(24, 72));
		break;

	case SV_SCROLL_WORD_OF_RECALL:
		word_of_recall();
		break;

	case SV_SCROLL_STAR_DESTRUCTION:
		ident = TRUE;
		if (!destroy_area(p_ptr->px, p_ptr->py, 15))
			msgf("The dungeon trembles...");
		break;

	case SV_SCROLL_MUNDANITY:
		used_up = mundane_spell();
		ident = TRUE;
		break;
	}

	/*
	 * Counter the side effect of use_object on an identify scroll
	 * by finding the original item
	 */
	OBJ_ITT_START (p_ptr->inventory, j_ptr)
	{
		/* retrieve the pointer of the original scroll */
		if (object_equal(&temp, j_ptr)) o_ptr = j_ptr;
	}
	OBJ_ITT_END;

	/* Hack - the scroll may already be destroyed by its effect */
	if (o_ptr->k_idx)
	{
		if (!(object_aware_p(o_ptr)))
		{
			chg_virtue(V_PATIENCE, -1);
			chg_virtue(V_CHANCE, 1);
		}

		/* The item was tried */
		object_tried(o_ptr);

		/* An identification was made */
		if (ident && !object_aware_p(o_ptr))
		{
			/* Object level */
			int lev = get_object_level(o_ptr);

			object_aware(o_ptr);
			gain_exp((lev + p_ptr->lev / 2) / p_ptr->lev);
		}
		
		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		/* Hack.  Do the sorting now */
		o_ptr = reorder_pack_watch(o_ptr);

		/* Hack.  Do the combining now */
		o_ptr = combine_pack_watch(o_ptr);

		/* Hack -- allow certain scrolls to be "preserved" */
		if (!used_up) return;

		sound(SOUND_SCROLL);
	
		/* Destroy a scroll */
		item_increase(o_ptr, -1);
	}
	
	make_noise(1);
}


void do_cmd_read_scroll(void)
{
	object_type *o_ptr;
	cptr q, s;

	/* Check some conditions */
	if (p_ptr->tim.blind)
	{
		msgf("You can't see anything.");
		return;
	}
	if (no_lite())
	{
		msgf("You have no light to read by.");
		return;
	}
	if (p_ptr->tim.confused)
	{
		msgf("You are too confused!");
		return;
	}


	/* Restrict choices to scrolls */
	item_tester_tval = TV_SCROLL;

	/* Get an item */
	q = "Read which scroll? ";
	s = "You have no scrolls to read.";

	o_ptr = get_item(q, s, (USE_INVEN | USE_FLOOR));

	/* Not a valid item */
	if (!o_ptr) return;

	/* Read the scroll */
	do_cmd_read_scroll_aux(o_ptr);
}


/*
 * Use a staff.
 *
 * One charge of one staff disappears.
 *
 * Hack -- staffs of identify can be "cancelled".
 */
static void do_cmd_use_staff_aux(object_type *o_ptr)
{
	int chance, lev;
	bool ident, use_charge;

	/* Mega-Hack -- refuse to use a pile from the ground */
	if (floor_item(o_ptr) && (o_ptr->number > 1))
	{
		msgf("You must first pick up the staffs.");
		return;
	}

	/* Take a turn */
	p_ptr->state.energy_use = 100;

	/* Is Identity known? */
	ident = object_aware_p(o_ptr);

	/* Extract the item level */
	lev = get_object_level(o_ptr);

	/* Base chance of success */
	chance = p_ptr->skills[SKILL_DEV];

	/* Confusion hurts skill */
	if (p_ptr->tim.confused) chance = chance / 2;

	/* Hight level objects are harder */
	chance = chance - lev / 2;

	/* Give everyone a (slight) chance */
	if ((chance < USE_DEVICE) && one_in_(USE_DEVICE - chance + 1))
	{
		chance = USE_DEVICE;
	}

	/* Roll for usage */
	if ((chance < USE_DEVICE) || (randint1(chance) < USE_DEVICE))
	{
		if (flush_failure) flush();
		msgf("You failed to use the staff properly.");
		sound(SOUND_FAIL);
		return;
	}

	/* Notice empty staffs */
	if (o_ptr->pval <= 0)
	{
		if (flush_failure) flush();
		msgf("The staff has no charges left.");
		o_ptr->info |= (OB_EMPTY);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);
		
		/* Notice changes */
		notice_item();

		return;
	}


	/* Sound */
	sound(SOUND_ZAP);

	/* Use the staff */
	use_charge = TRUE;
	
	switch (o_ptr->sval)
	{
	case SV_STAFF_DETECT_TRAP:
		ident = detect_traps(ident);
		break;

	case SV_STAFF_DETECT_GOLD:
		ident = detect_treasure();
		if (detect_objects_gold()) ident = TRUE;
		break;

	case SV_STAFF_DETECT_ITEM:
		ident = detect_objects_normal();
		break;

	case SV_STAFF_TELEPORTATION:
		ident = TRUE;
		teleport_player(100);
		break;

	case SV_STAFF_EARTHQUAKES:
		ident = TRUE;
		if (!earthquake(p_ptr->px, p_ptr->py, 10))
			msgf("The dungeon trembles...");
		break;

	case SV_STAFF_SUMMONING:
		ident = summon_monsters(randint1(4), 0);
		break;

	case SV_STAFF_LITE:
		ident = lite_area(damroll(2, 8), 2);
		break;

	case SV_STAFF_DESTRUCTION:
		ident = destroy_area(p_ptr->px, p_ptr->py, 15);
		break;

	case SV_STAFF_STARLITE:
		if (p_ptr->tim.blind == 0)
			msgf("The end of the staff glows brightly...");
		starlite();
		ident = TRUE;
		break;

	case SV_STAFF_HASTE_MONSTERS:
		ident = speed_monsters();
		break;

	case SV_STAFF_SLOW_MONSTERS:
		ident = slow_monsters();
		break;

	case SV_STAFF_SLEEP_MONSTERS:
		ident = sleep_monsters();
		break;

	case SV_STAFF_CURE_LIGHT:
		ident = hp_player(50);
		break;

	case SV_STAFF_DETECT_INVIS:
		ident = detect_monsters_invis();
		break;

	case SV_STAFF_SPEED:
		ident = inc_fast(rand_range(15, 45));
		break;

	case SV_STAFF_SLOWNESS:
		ident = inc_slow(rand_range(15, 45));
		break;

	case SV_STAFF_DETECT_DOOR:
		ident = detect_doors();
		if (detect_stairs()) ident = TRUE;
		break;

	case SV_STAFF_REMOVE_CURSE:
		ident = remove_curse();
		if (ident && !p_ptr->tim.blind)
			msgf("The staff glows blue for a moment...");
		break;

	case SV_STAFF_DETECT_EVIL:
		ident = detect_monsters_evil();
			break;

	case SV_STAFF_CURING:
		ident = cure_wounds(150, TRUE, TRUE, TRUE);
			if (clear_image()) ident = TRUE;
		break;

	case SV_STAFF_DISPEL_EVIL:
		ident = dispel_evil(60);
		break;

	case SV_STAFF_PROBING:
		ident = probing();
		break;

	case SV_STAFF_DARKNESS:
		if (!FLAG(p_ptr, TR_RES_BLIND) && !FLAG(p_ptr, TR_RES_DARK))
		{
			ident = inc_blind(rand_range(4, 8));
			if (unlite_area(10, 3)) ident = TRUE;
		}
		break;

	case SV_STAFF_GENOCIDE:
		genocide(TRUE);
		ident = TRUE;
		break;

	case SV_STAFF_POWER:
		dispel_monsters(300);
		ident = TRUE;
		break;

	case SV_STAFF_THE_MAGI:
		ident = do_res_stat(A_INT);
		if (restore_mana()) ident = TRUE;
		break;

	case SV_STAFF_IDENTIFY:
		use_charge = ident_spell();
		ident = TRUE;
		break;

	case SV_STAFF_HOLINESS:
		if (dispel_evil(300)) ident = TRUE;
		if (inc_protevil(randint1(25) + 3 * p_ptr->lev)) ident = TRUE;
		if (clear_poisoned()) ident = TRUE;
		if (clear_afraid()) ident = TRUE;
		if (hp_player(50)) ident = TRUE;
		if (clear_stun()) ident = TRUE;
		if (clear_cut()) ident = TRUE;
		break;

	case SV_STAFF_MAPPING:
		map_area();
		ident = TRUE;
		break;

	case SV_STAFF_HEALING:
		ident = hp_player(300);
		if (clear_stun()) ident = TRUE;
		if (clear_cut()) ident = TRUE;
		break;
	}

	/* Hack - the staff may destroy itself when activated on the ground */
	if (o_ptr->k_idx)
	{
		if (!(object_aware_p(o_ptr)))
		{
			chg_virtue(V_PATIENCE, -1);
			chg_virtue(V_CHANCE, 1);
		}

		/* Tried the item */
		object_tried(o_ptr);

		/* An identification was made */
		if (ident && !object_aware_p(o_ptr))
		{
			object_aware(o_ptr);
			gain_exp((lev + p_ptr->lev / 2) / p_ptr->lev);
		}
		
		/* Notice changes */
		notice_item();

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);


		/* Hack -- some uses are "free" */
		if (!use_charge) return;

		/* XXX Hack -- unstack if necessary */
		if (o_ptr->number > 1)
		{
			/* Split object */
			o_ptr = item_split(o_ptr, 1);

			/* Use a single charge */
			o_ptr->pval--;

			/* Unstack the used item */
			o_ptr = inven_carry(o_ptr);

			/* Notice weight changes */
			p_ptr->update |= PU_WEIGHT;

			/* Paranoia */
			if (!o_ptr)
			{
				msgf("Too many dungeon objects - staff lost!");

				make_noise(1);

				/* Exit */
				return;
			}

			/* Message */
			msgf("You unstack your staff.");
		}
		else
		{
			/* Use a single charge */
			o_ptr->pval--;
		}

		/* Describe charges in the pack */
		if (o_ptr) item_charges(o_ptr);
	}

	make_noise(1);
}


void do_cmd_use_staff(void)
{
	object_type *o_ptr;
	cptr q, s;

	/* Restrict choices to wands */
	item_tester_tval = TV_STAFF;

	/* Get an item */
	q = "Use which staff? ";
	s = "You have no staff to use.";

	o_ptr = get_item(q, s, (USE_INVEN | USE_FLOOR));

	/* Not a valid item */
	if (!o_ptr) return;

	do_cmd_use_staff_aux(o_ptr);
}

bool wand_of_wonder(int dir)
{
	bool ident = FALSE;
	int sval = randint0(SV_WAND_WONDER);

	switch (sval)
	{
	case SV_WAND_HEAL_MONSTER:
		ident = heal_monster(dir);
		break;

	case SV_WAND_HASTE_MONSTER:
		ident = speed_monster(dir);
		break;

	case SV_WAND_CLONE_MONSTER:
		ident = clone_monster(dir);
		break;

	case SV_WAND_TELEPORT_AWAY:
		ident = teleport_monster(dir);
		break;

	case SV_WAND_DISARMING:
		ident = disarm_trap(dir);
		break;

	case SV_WAND_TRAP_DOOR_DEST:
		ident = destroy_door(dir);
		break;

	case SV_WAND_STONE_TO_MUD:
		ident = wall_to_mud(dir);
		break;

	case SV_WAND_LITE:
		msgf("A line of blue shimmering light appears.");
		lite_line(dir, damroll(6, 8));
		ident = TRUE;
		break;

	case SV_WAND_SLEEP_MONSTER:
		ident = sleep_monster(dir);
		break;

	case SV_WAND_SLOW_MONSTER:
		ident = slow_monster(dir);
		break;

	case SV_WAND_CONFUSE_MONSTER:
		ident = confuse_monster(dir, 20);
		break;

	case SV_WAND_FEAR_MONSTER:
		ident = fear_monster(dir, 20);
		break;

	case SV_WAND_DRAIN_LIFE:
		ident = drain_life(dir, 150);
		break;

	case SV_WAND_POLYMORPH:
		ident = poly_monster(dir);
		break;

	case SV_WAND_STINKING_CLOUD:
		ident = fire_ball(GF_POIS, dir, 15, 2);
		break;

	case SV_WAND_MAGIC_MISSILE:
		ident = fire_bolt_or_beam(20, GF_MISSILE, dir, damroll(2, 6));
		break;

	case SV_WAND_ACID_BOLT:
		ident = fire_bolt_or_beam(20, GF_ACID, dir, damroll(6, 8));
		break;

	case SV_WAND_CHARM_MONSTER:
		ident = charm_monster(dir, 45);
		break;

	case SV_WAND_FIRE_BOLT:
		ident = fire_bolt_or_beam(20, GF_FIRE, dir, damroll(10, 8));
		break;

	case SV_WAND_COLD_BOLT:
		ident = fire_bolt_or_beam(20, GF_COLD, dir, damroll(6, 8));
		break;

	case SV_WAND_ACID_BALL:
		ident = fire_ball(GF_ACID, dir, 125, 2);
		break;

	case SV_WAND_ELEC_BALL:
		ident = fire_ball(GF_ELEC, dir, 75, 2);
		break;

	case SV_WAND_FIRE_BALL:
		ident = fire_ball(GF_FIRE, dir, 150, 2);
		break;

	case SV_WAND_COLD_BALL:
		ident = fire_ball(GF_COLD, dir, 100, 2);
		break;
	}
	
	return ident;
}

/*
 * Aim a wand (from the pack or floor).
 *
 * Use a single charge from a single item.
 * Handle "unstacking" in a logical manner.
 *
 * For simplicity, you cannot use a stack of items from the
 * ground.  This would require too much nasty code.
 *
 * There are no wands which can "destroy" themselves, in the inventory
 * or on the ground, so we can ignore this possibility.  Note that this
 * required giving "wand of wonder" the ability to ignore destruction
 * by electric balls.
 *
 * All wands can be "cancelled" at the "Direction?" prompt for free.
 *
 * Note that the basic "bolt" wands do slightly less damage than the
 * basic "bolt" rods, but the basic "ball" wands do the same damage
 * as the basic "ball" rods.
 */
static void do_cmd_aim_wand_aux(object_type *o_ptr)
{
	bool use_charge;
	int chance, dir, lev;
	bool ident;

	/* Mega-Hack -- refuse to use a pile from the ground */
	if (floor_item(o_ptr) && (o_ptr->number > 1))
	{
		msgf("You must first pick up the wands.");
		return;
	}

	/* Notice empty wandss */
	if (o_ptr->pval <= 0)
	{
		if (flush_failure) flush();
		msgf("The wand has no charges left.");
		o_ptr->info |= (OB_EMPTY);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);
		
		/* Notice changes */
		notice_item();

		return;
	}

	/* Allow direction to be cancelled for free */
	if (!get_aim_dir(&dir)) return;
	
	/* Is Identity known? */
	ident = object_aware_p(o_ptr);

	/* Take a turn */
	p_ptr->state.energy_use = MIN(75, 200 - 5 * p_ptr->skills[SKILL_DEV] / 8);

	/* Get the object level */
	lev = k_info[o_ptr->k_idx].level;

	/* Base chance of success */
	chance = p_ptr->skills[SKILL_DEV];

	/* Confusion hurts skill */
	if (p_ptr->tim.confused) chance /= 2;

	/* Hight level objects are harder */
	chance = chance - lev / 2;

	/* Give everyone a (slight) chance */
	if ((chance < USE_DEVICE) && one_in_(USE_DEVICE - chance + 1))
	{
		chance = USE_DEVICE;
	}

	/* Roll for usage */
	if ((chance < USE_DEVICE) || (randint1(chance) < USE_DEVICE))
	{
		if (flush_failure) flush();
		msgf("You failed to use the wand properly.");
		sound(SOUND_FAIL);
		return;
	}

	/* Sound */
	sound(SOUND_ZAP);

	/* Aim the wand */
	use_charge = TRUE;
	switch (o_ptr->sval)
	{
	case SV_WAND_LITE:
		msgf("A line of blue shimmering light appears.");
		lite_line(dir, damroll(6, 8));
		ident = TRUE;
		break;

	case SV_WAND_CHARM_MONSTER:
		ident = charm_monster(dir, 45);
		break;

	case SV_WAND_COLD_BOLT:
		ident = fire_bolt_or_beam(20, GF_COLD, dir, damroll(6, 8));
		break;

	case SV_WAND_FIRE_BOLT:
		ident = fire_bolt_or_beam(20, GF_FIRE, dir, damroll(10, 8));
		break;

	case SV_WAND_STONE_TO_MUD:
		ident = wall_to_mud(dir);
		break;

	case SV_WAND_POLYMORPH:
		ident = poly_monster(dir);
		break;

	case SV_WAND_HEAL_MONSTER:
		ident = heal_monster(dir);
		break;

	case SV_WAND_HASTE_MONSTER:
		ident = speed_monster(dir);
		break;

	case SV_WAND_SLOW_MONSTER:
		ident = slow_monster(dir);
		break;

	case SV_WAND_CONFUSE_MONSTER:
		ident = confuse_monster(dir, 20);
		break;

	case SV_WAND_SLEEP_MONSTER:
		ident = sleep_monster(dir);
		break;

	case SV_WAND_DRAIN_LIFE:
		ident = drain_life(dir, 150);
		break;

	case SV_WAND_TRAP_DOOR_DEST:
		ident = destroy_door(dir);
		break;

	case SV_WAND_MAGIC_MISSILE:
		ident = fire_bolt_or_beam(20, GF_MISSILE, dir, damroll(2, 6));
		break;

	case SV_WAND_CLONE_MONSTER:
		ident = clone_monster(dir);
		break;

	case SV_WAND_FEAR_MONSTER:
		ident = fear_monster(dir, 20);
		break;

	case SV_WAND_TELEPORT_AWAY:
		ident = teleport_monster(dir);
		break;

	case SV_WAND_DISARMING:
		ident = disarm_trap(dir);
		break;

	case SV_WAND_ELEC_BALL:
		ident = fire_ball(GF_ELEC, dir, 75, 2);
		break;

	case SV_WAND_COLD_BALL:
		ident = fire_ball(GF_COLD, dir, 100, 2);
		break;

	case SV_WAND_FIRE_BALL:
		ident = fire_ball(GF_FIRE, dir, 150, 2);
		break;

	case SV_WAND_STINKING_CLOUD:
		ident = fire_ball(GF_POIS, dir, 15, 2);
		break;

	case SV_WAND_ACID_BALL:
		ident = fire_ball(GF_ACID, dir, 125, 2);
		break;

	case SV_WAND_WONDER:
		ident = wand_of_wonder(dir);
		break;

	case SV_WAND_ACID_BOLT:
		ident = fire_bolt_or_beam(20, GF_ACID, dir, damroll(6, 8));
		break;

	case SV_WAND_DRAGON_FIRE:
		ident = fire_ball(GF_FIRE, dir, 250, 3);
		break;

	case SV_WAND_DRAGON_COLD:
		ident = fire_ball(GF_COLD, dir, 200, 3);
		break;

	case SV_WAND_DRAGON_BREATH:
	{
		int element = rand_range(GF_ELEC, GF_FIRE);
		int dam = 200;
		if (element == GF_ACID) dam = 250;
		else if (element == GF_ELEC) dam = 150;

		ident = fire_ball(element, dir, dam, 3);
	}
	break;

	case SV_WAND_ANNIHILATION:
		ident = fire_ball(GF_DISINTEGRATE, dir, rand_range(125, 225), 2);
		break;

	case SV_WAND_ROCKETS:
		msgf("You launch a rocket!"); 
		fire_ball(GF_ROCKET, dir, 250, 2);
		ident = TRUE;
		break;
	}

	/* Hack - wands may destroy themselves if activated on the ground */
	if (o_ptr->k_idx)
	{
		/* Mark it as tried */
		object_tried(o_ptr);

		/* Apply identification */
		if (ident && !object_aware_p(o_ptr))
		{
			int lev = get_object_level(o_ptr);

			object_aware(o_ptr);
			gain_exp((lev + p_ptr->lev / 2) / p_ptr->lev);
		}
		
		/* Notice changes */
		notice_item();

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		/* Hack -- some uses are "free" */
		if (!use_charge) return;

		/* Use a single charge */
		o_ptr->pval--;
		o_ptr->ac++;

		/* Describe the charges */
		item_charges(o_ptr);
	}

	make_noise(1);
}


void do_cmd_aim_wand(void)
{
	object_type *o_ptr;
	cptr q, s;

	/* Restrict choices to wands */
	item_tester_tval = TV_WAND;

	/* Get an item */
	q = "Aim which wand? ";
	s = "You have no wand to aim.";

	o_ptr = get_item(q, s, (USE_INVEN | USE_FLOOR));

	/* Not a valid item */
	if (!o_ptr) return;

	/* Aim the wand */
	do_cmd_aim_wand_aux(o_ptr);
}

static bool restore_all_stats()
{
	bool ident = FALSE;

	for (int i = 0; i < 6; i++)
		ident |= res_stat(i);

	return ident;
}

/*
 * Activate (zap) a Rod
 *
 * Unstack fully charged rods as needed.
 *
 * Hack -- rods of perception/genocide can be "cancelled"
 * All rods can be cancelled at the "Direction?" prompt
 *
 * pvals are defined for each rod in k_info. -LM-
 */
static void do_cmd_zap_rod_aux(object_type *o_ptr)
{
	int chance, dir, lev;
	bool ident;

	/* Hack -- let perception get aborted */
	bool use_charge = TRUE;

	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* Mega-Hack -- refuse to use a pile from the ground */
	if (floor_item(o_ptr) && (o_ptr->number > 1))
	{
		msgf("You must first pick up the rods.");
		return;
	}

	/* A single rod is still charging */
	if ((o_ptr->number == 1) && (o_ptr->timeout))
	{
		if (flush_failure) flush();
		msgf("The rod is still charging.");
		return;
	}
	/* A stack of rods lacks enough energy. */
	else if ((o_ptr->number > 1)
			 && (o_ptr->timeout > (o_ptr->number - 1) * k_ptr->pval))
	{
		if (flush_failure) flush();
		msgf("The rods are all still charging.");
		return;
	}

	/* Get a direction (unless KNOWN not to need it) */
	if (((o_ptr->sval >= SV_ROD_MIN_DIRECTION) && (o_ptr->sval != SV_ROD_HAVOC))
		|| !object_aware_p(o_ptr))
	{
		/* Get a direction, allow cancel */
		if (!get_aim_dir(&dir)) return;
	}

	/* Take a turn */
	p_ptr->state.energy_use = MIN(75, 200 - 5 * p_ptr->skills[SKILL_DEV] / 8);

	/* Not identified yet */
	ident = FALSE;
	
	/* Is Identity known? */
	ident = object_aware_p(o_ptr);

	/* Extract the item level */
	lev = get_object_level(o_ptr);

	/* Base chance of success */
	chance = p_ptr->skills[SKILL_DEV];

	/* Confusion hurts skill */
	if (p_ptr->tim.confused) chance = chance / 2;

	/* Hight level objects are harder */
	chance = chance - lev / 2;

	/* Give everyone a (slight) chance */
	if ((chance < USE_DEVICE) && one_in_(USE_DEVICE - chance + 1))
	{
		chance = USE_DEVICE;
	}

	/* Roll for usage */
	if ((chance < USE_DEVICE) || (randint1(chance) < USE_DEVICE))
	{
		if (flush_failure) flush();
		msgf("You failed to use the rod properly.");
		sound(SOUND_FAIL);
		return;
	}

	/* Sound */
	sound(SOUND_ZAP);

	/* Increase the timeout by the rod kind's pval. -LM- */
	o_ptr->timeout += k_ptr->pval;

	/* Zap the rod */
	switch (o_ptr->sval)
	{
	case SV_ROD_HAVOC:
		ident = TRUE;
		call_chaos();
		break;

	case SV_ROD_DETECT_DOOR:
		ident = detect_doors();
		ident |= detect_stairs();
		break;

	case SV_ROD_DETECT_TRAP:
		ident = detect_traps(ident);
		break;

	case SV_ROD_PROBING:
		ident = probing();
		break;

	case SV_ROD_RECALL:
		word_of_recall();
		ident = TRUE;
		break;

	case SV_ROD_ILLUMINATION:
		ident = lite_area(damroll(4, 8), 2);
		break;

	case SV_ROD_LITE:
		msgf("A line of blue shimmering light appears.");
		lite_line(dir, damroll(6, 8));
		ident = TRUE;
		break;

	case SV_ROD_ELEC_BOLT:
		ident = fire_bolt_or_beam(10, GF_ELEC, dir, damroll(5, 8));
		break;

	case SV_ROD_COLD_BOLT:
		ident = fire_bolt_or_beam(10, GF_COLD, dir, damroll(6, 8));
		break;

	case SV_ROD_FIRE_BOLT:
		ident = fire_bolt_or_beam(10, GF_FIRE, dir, damroll(10, 8));
		break;

	case SV_ROD_POLYMORPH:
		ident |= poly_monster(dir);
		break;

	case SV_ROD_SLOW_MONSTER:
		ident |= slow_monster(dir);
		break;

	case SV_ROD_SLEEP_MONSTER:
		ident |= sleep_monster(dir);
		break;

	case SV_ROD_DRAIN_LIFE:
		ident |= drain_life(dir, 150);
		break;

	case SV_ROD_TELEPORT_AWAY:
		ident |= teleport_monster(dir);
		break;

	case SV_ROD_DISARMING:
		ident |= disarm_trap(dir);
		break;

	case SV_ROD_ELEC_BALL:
		ident = fire_ball(GF_ELEC, dir, 75, 2);
		break;

	case SV_ROD_COLD_BALL:
		ident = fire_ball(GF_COLD, dir, 100, 2);
		break;

	case SV_ROD_FIRE_BALL:
		ident = fire_ball(GF_FIRE, dir, 150, 2);
		break;

	case SV_ROD_ACID_BALL:
		ident = fire_ball(GF_ACID, dir, 125, 2);
		break;

	case SV_ROD_ACID_BOLT:
		ident = fire_bolt_or_beam(10, GF_ACID, dir, damroll(6, 8));
		break;

	case SV_ROD_MAPPING:
		map_area();
		ident = TRUE;
		break;

	case SV_ROD_IDENTIFY:
		use_charge = ident_spell();
		ident = TRUE;
		break;

	case SV_ROD_CURING:
		ident = cure_wounds(200, TRUE, TRUE, TRUE);
		ident |= clear_image();
		break;

	case SV_ROD_HEALING:
		ident |= hp_player(500);
		ident |= clear_stun();
		ident |= clear_cut();
		break;

	case SV_ROD_DETECTION:
		ident |= detect_all();
		break;

	case SV_ROD_RESTORATION:
		ident |= restore_level();
		ident |= restore_all_stats();
		break;

	case SV_ROD_SPEED:
		ident |= inc_fast(rand_range(15, 45));
		break;

	case SV_ROD_PESTICIDE:
		ident = fire_ball(GF_POIS, dir, 8, 3);
		break;
	}

	if (!(object_aware_p(o_ptr)))
	{
		chg_virtue(V_PATIENCE, -1);
		chg_virtue(V_CHANCE, 1);
	}

	/* Tried the object */
	object_tried(o_ptr);

	/* Successfully determined the object function */
	if (ident && !object_aware_p(o_ptr))
	{
		object_aware(o_ptr);
		gain_exp((lev + p_ptr->lev / 2) / p_ptr->lev);
	}
	
	/* Notice changes */
	notice_item();

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	/* Hack -- deal with cancelled zap */
	if (!use_charge)
	{
		o_ptr->timeout -= k_ptr->pval;
		return;
	}

	make_noise(1);
}


void do_cmd_zap_rod(void)
{
	object_type *o_ptr;
	cptr q, s;

	/* Restrict choices to rods */
	item_tester_tval = TV_ROD;

	/* Get an item */
	q = "Zap which rod? ";
	s = "You have no rod to zap.";

	o_ptr = get_item(q, s, (USE_INVEN | USE_FLOOR));

	/* Not a valid item */
	if (!o_ptr) return;

	/* Zap the rod */
	do_cmd_zap_rod_aux(o_ptr);
}


/*
 * Hook to determine if an object is activatable
 */
static bool item_tester_hook_activate(const object_type *o_ptr)
{
	/* Check statues */
	if (o_ptr->tval == TV_STATUE) return (TRUE);

	/* Ignore dungeon objects */
	if (o_ptr->iy || o_ptr->ix) return (FALSE);

	/* Not known */
	if (!object_known_p(o_ptr)) return (FALSE);

	/* Check activation flag */
	if (FLAG(o_ptr, TR_ACTIVATE)) return (TRUE);

	/* Assume not */
	return (FALSE);
}


/*
 * Hack -- activate the ring of power
 */
void ring_of_power(int dir)
{
	/* Pick a random effect */
	switch (randint1(10))
	{
		case 1:
		case 2:
		{
			/* Message */
			msgf("You are surrounded by a malignant aura.");
			sound(SOUND_EVIL);

			/* Decrease all stats (permanently) */
			(void)dec_stat(A_STR, 50, TRUE);
			(void)dec_stat(A_INT, 50, TRUE);
			(void)dec_stat(A_WIS, 50, TRUE);
			(void)dec_stat(A_DEX, 50, TRUE);
			(void)dec_stat(A_CON, 50, TRUE);
			(void)dec_stat(A_CHR, 50, TRUE);

			/* Lose some experience (permanently) */
			p_ptr->exp -= (p_ptr->exp / 4);
			p_ptr->max_exp -= (p_ptr->exp / 4);
			check_experience();

			break;
		}

		case 3:
		{
			/* Message */
			msgf("You are surrounded by a powerful aura.");

			/* Dispel monsters */
			(void)dispel_monsters(1000);
			break;
		}

		case 4:
		case 5:
		case 6:
		{
			/* Mana Ball */
			(void)fire_ball(GF_MANA, dir, 300, 3);
			break;
		}

		case 7:
		case 8:
		case 9:
		case 10:
		{
			/* Mana Bolt */
			(void)fire_bolt(GF_MANA, dir, 250);
			break;
		}
	}
}


/*
 * Activate a wielded object.  Wielded objects never stack.
 * And even if they did, activatable objects never stack.
 *
 * Note that it always takes a turn to activate an artifact, even if
 * the user hits "escape" at the "direction" prompt.
 */
static void do_cmd_activate_aux(object_type *o_ptr)
{
	int lev, chance;

	/* Take a turn */
	p_ptr->state.energy_use = MIN(75, 200 - 5 * p_ptr->skills[SKILL_DEV] / 8);

	/* Extract the item level */
	lev = get_object_level(o_ptr);

	/* Base chance of success */
	chance = p_ptr->skills[SKILL_DEV];

	/* Confusion hurts skill */
	if (p_ptr->tim.confused) chance /= 2;

	/* Cursed items are difficult to activate */
	if (cursed_p(o_ptr)) chance /= 3;

	/* High level objects are harder */
	chance = chance - lev / 2;

	/* Give everyone a (slight) chance */
	if ((chance < USE_DEVICE) && one_in_(USE_DEVICE - chance + 1))
	{
		chance = USE_DEVICE;
	}

	/* Roll for usage */
	if ((chance < USE_DEVICE) || (randint1(chance) < USE_DEVICE))
	{
		if (flush_failure) flush();
		msgf("You failed to activate it properly.");
		sound(SOUND_FAIL);
		return;
	}

	/* Check the recharge */
	if (o_ptr->timeout)
	{
		msgf("It whines, glows and fades...");
		return;
	}


	/* Activate the artifact */
	msgf(MSGT_ZAP, "You activate it...");

	/* Sound */
	sound(SOUND_ZAP);

	/* Activate the object */
	int dir = 0;
	/*Artifact?*/
	if (o_ptr->a_idx)
		activate_artifact(o_ptr);
	/* Activating a ring or DSM? */
	else if (get_aim_dir(&dir))
	{
		if (o_ptr->tval == TV_RING)
		{

			switch (o_ptr->sval)
			{
			case SV_RING_FLAMES:
				fire_ball(GF_FIRE, dir, 100, 2);
				inc_oppose_fire(rand_range(20, 40));
				o_ptr->timeout = rand_range(25, 50);
				break;

			case SV_RING_ACID:
				fire_ball(GF_ACID, dir, 100, 2);
				inc_oppose_acid(rand_range(20, 40));
				o_ptr->timeout = rand_range(25, 50);
				break;

			case SV_RING_ICE:
				fire_ball(GF_COLD, dir, 100, 2);
				inc_oppose_cold(rand_range(20, 40));
				o_ptr->timeout = rand_range(25, 50);
				break;
			}
		}
		else if (o_ptr->tval == TV_DRAG_ARMOR)
		{
			int damtype = GF_NONE;

			switch (o_ptr->sval)
			{
			case SV_DRAGON_BLACK:
				msgf("You breathe acid.");
				fire_ball(GF_ACID, dir, 430, 2);
				o_ptr->timeout = rand_range(50, 100);
				break;

			case SV_DRAGON_BLUE:
				msgf("You breathe lightning.");
				fire_ball(GF_ELEC, dir, 330, 2);
				o_ptr->timeout = rand_range(50, 100);
				break;

			case SV_DRAGON_WHITE:
				msgf("You breathe frost.");
				fire_ball(GF_COLD, dir, 370, 2);
				o_ptr->timeout = rand_range(50, 100);
				break;

			case SV_DRAGON_RED:
				msgf("You breathe fire.");
				fire_ball(GF_FIRE, dir, 670, 2);
				o_ptr->timeout = rand_range(50, 100);
				break;

			case SV_DRAGON_GREEN:
				msgf("You breathe poison gas.");
				fire_ball(GF_POIS, dir, 500, 2);
				o_ptr->timeout = rand_range(50, 100);
				break;

				case SV_DRAGON_MULTIHUED:
					damtype = randint1(5);
					if (damtype == GF_ELEC)
						msgf("You breathe lightning.");
					else if (damtype == GF_ACID)
						msgf("You breathe acid.");
					else if (damtype == GF_COLD)
						msgf("You breathe frost.");
					else if (damtype == GF_FIRE)
						msgf("You breathe fire.");
					else /*damtype == GF_POISON*/
						msgf("You breathe poison gas.");
					fire_ball(damtype, dir, 840, 2);
					o_ptr->timeout = rand_range(25, 50);
					break;

				case SV_DRAGON_SHINING:
					damtype = GF_LITE;
					if (randint1(2) == 1)
					{
						damtype = GF_DARK;
						msgf("You breathe darkness.");
					}
					else
						msgf("You breathe light.");
					fire_ball(damtype, dir, 670, 2);
					o_ptr->timeout = rand_range(30, 60);
					break;

				case SV_DRAGON_LAW:
					damtype = GF_SOUND;
					if (randint1(2) == 1)
					{
						damtype = GF_SHARDS;
						msgf("You breathe shards.");
					}
					else
						msgf("You breathe sound.");
					fire_ball(damtype, dir, 750, 2);
					o_ptr->timeout = rand_range(30, 60);
					break;

				case SV_DRAGON_BRONZE:
					msgf("You breathe confusion.");
					fire_ball(GF_CONFUSION, dir, 400, 2);
					o_ptr->timeout = rand_range(50, 100);
					break;

				case SV_DRAGON_GOLD:
					msgf("You breathe sound.");
					fire_ball(GF_SOUND, dir, 430, 2);
					o_ptr->timeout = rand_range(50, 100);
					break;

				case SV_DRAGON_CHAOS:
					damtype = GF_CHAOS;
					if (randint1(2) == 1)
					{
						damtype = GF_DISENCHANT;
						msgf("You breathe disenchantment.");
					}
					else
						msgf("You breathe chaos.");
					fire_ball(damtype, dir, 740, 2);
					o_ptr->timeout = rand_range(30, 60);
					break;

				case SV_DRAGON_BALANCE:
					damtype = randint1(5);
					if (damtype == 1)
					{
						damtype = GF_SOUND;
						msgf("You breathe sound.");
					}
					else if (damtype == 2)
					{
						damtype = GF_SHARDS;
						msgf("You breathe shards.");
					}
					else if (damtype == 3)
					{
						damtype = GF_CHAOS;
						msgf("You breathe chaos.");
					}
					else /*damtype == 4*/
					{
						damtype = GF_DISENCHANT;
						msgf("You breathe disenchantment.");
					}
					fire_ball(damtype, dir, 840, 2);
					o_ptr->timeout = rand_range(30, 60);
					break;

				case SV_DRAGON_POWER:
					msgf("You breathe the elements.");
					fire_ball(GF_MISSILE, dir, 1000, 2);
					o_ptr->timeout = rand_range(30, 60);

			}
		}

	}


	/* Notice changes */
	notice_item();

	make_noise(3);

	return;
}


void do_cmd_activate(void)
{
	object_type *o_ptr;
	cptr q, s;

	/* Prepare the hook */
	item_tester_hook = item_tester_hook_activate;

	/* Get an item */
	q = "Activate which item? ";
	s = "You have nothing to activate.";

	o_ptr = get_item(q, s, (USE_EQUIP | USE_FLOOR));

	/* Not a valid item */
	if (!o_ptr) return;

	/* Activate the item */
	do_cmd_activate_aux(o_ptr);
}
