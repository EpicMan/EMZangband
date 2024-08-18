/* File: script.c */

#include "angband.h"

#include "script.h"

/*
 * Apply an object trigger, a small lua script which can be attached to an
 * object type or a specific item (usually an ego-item or artifact).
 *
 * Currently defined triggers, and their normal arguments, include:
 *
 * TRIGGER_USE - for activating a wearable item or using any other item.
 * Wearable items neither take nor return values. Other items may or may not
 * have a 'dir' value, depending on type, and may return 'result' and 'ident' 
 * which indicate if the action used a charge and if it should identify the
 * object, respectively.
 *
 * TRIGGER_MAKE - called once near the end of object generation. Takes one
 * argument, 'lev', which is the level the object is being generated at for
 * non-artifacts and the level of the artifact for artifacts.
 *
 * TRIGGER_BONUS - called on worn items during calc_bonuses(). No arguments.
 *
 * TRIGGER_SMASH - called for potions when they break.
 *
 * TRIGGER_DESC - called to get an activation/use description for an item.
 * Returns a string describing the activation or use.
 */
void apply_object_trigger(int trigger_id, object_type *o_ptr, cptr format, ...)
{
	va_list vp;
	
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	cptr script = NULL;
	
	void *q_ptr = NULL;
	
	bool success = FALSE;
#if 0	
	if (o_ptr->trigger[trigger_id])
		script = quark_str(o_ptr->trigger[trigger_id]);
	else if (k_ptr->trigger[trigger_id])
		script = k_text + k_ptr->trigger[trigger_id];
	else
		return;
	
	/* Save parameter so recursion works. */
	lua_getglobal (L, "object");
	if (tolua_istype(L, -1, tolua_tag(L, "object_type"), 0))
	{
		q_ptr = tolua_getuserdata(L, -1, NULL);
	}
	lua_pop(L,1);

	/* Set parameters (really global) */
	tolua_pushusertype(L, (void*)o_ptr, tolua_tag(L, "object_type"));
	lua_setglobal(L, "object");
	
	/* Begin the Varargs Stuff */
	va_start(vp, format);
	
	success = call_lua_hook(script, format, vp);

	/* End the Varargs Stuff */
	va_end(vp);
	
	/* Restore global so recursion works*/
	tolua_pushusertype(L, q_ptr, tolua_tag(L,"object_type"));
	lua_setglobal(L, "object");
#endif
	/* Paranoia */
	if (!success)
	{
		msgf("Script for object: %v failed.", OBJECT_STORE_FMT(o_ptr, FALSE, 3));
	}
}
#if 0
/*
 * Callback for using an object
 *
 * If the object is a scroll of identify this function may have a side effect.
 * The side effect is that the o_ptr no longer points at the scroll of identity
 * because of the sorting done by the identify_spell.
 * This side effect is countered in do_cmd_read_scroll_aux, the only place where
 * it matters.
 */
bool use_object(object_type *o_ptr, bool *id_return, int dir)
{
	bool result = TRUE, ident = *id_return;
	/*
	apply_object_trigger(TRIGGER_USE, o_ptr, "i:bb",
			LUA_VAR(dir), LUA_RETURN(result), LUA_RETURN(ident));
			*/
	*id_return = ident;
	return result;
}
#endif
static bool field_delete = FALSE;

/*
 * Delete current field when finish processing.
 *
 * This is a horrible name...
 */
void deleteme(void)
{
	field_delete = TRUE;
}

/*
 * Apply an field trigger, a small lua script which does
 * what the old field action functions did.
 */
bool apply_field_trigger(cptr script, field_type *f_ptr, cptr format, va_list vp)
{
	field_thaum *t_ptr = &t_info[f_ptr->t_idx];

	void *q_ptr = NULL;
	
	bool success = FALSE;
	bool delete_save, delete = FALSE;
#if 0
	/* Save parameter so recursion works. */
	lua_getglobal (L, "field");
	if (tolua_istype(L, -1, tolua_tag(L, "field_type"), 0))
	{
		q_ptr = tolua_getuserdata(L, -1, NULL);
	}
	lua_pop(L,1);
	delete_save = field_delete;
	
	/* Default to no deletion */
	field_delete = FALSE;

	/* Set parameters (really global) */
	tolua_pushusertype(L, (void*)f_ptr, tolua_tag(L, "field_type"));
	lua_setglobal(L, "field");
	
	success = call_lua_hook(script, format, vp);
	
	/* Restore global so recursion works*/
	tolua_pushusertype(L, q_ptr, tolua_tag(L,"field_type"));
	lua_setglobal(L, "field");
	delete = field_delete;
	field_delete = delete_save;
	
	/* Paranoia */
	if (!success)
	{
		msgf("Script for field: %s failed.", t_ptr->name);
	}
#endif
	/* Does the field want to be deleted? */
	return (delete);
}


/*
 * Apply an field trigger, a small lua script which does
 * what the old field action functions did.
 *
 * This version doesn't modify the field, but uses a copy instead.
 * This allows const versions of field hooks.
 *
 * The field cannot be deleted.
 */
void const_field_trigger(cptr script, const field_type *f_ptr, cptr format, va_list vp)
{
	/* Structure copy to get local working version */
	field_type temp_field = *f_ptr;
	
	(void) apply_field_trigger(script, &temp_field, format, vp);
}
