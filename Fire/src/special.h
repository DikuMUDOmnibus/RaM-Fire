/*
 * RAM $Id: special.h 16 2008-10-22 04:42:21Z ram $
 */

/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 **************************************************************************/

/***************************************************************************
*       ROM 2.4 is copyright 1993-1998 Russ Taylor                         *
*       ROM has been brought to you by the ROM consortium                  *
*           Russ Taylor (rtaylor@hypercube.org)                            *
*           Gabrielle Taylor (gtaylor@hypercube.org)                       *
*           Brian Moore (zump@rom.org)                                     *
*       By using this code, you have agreed to follow the terms of the     *
*       ROM license, in the file Rom24/doc/rom.license                     *
***************************************************************************/

struct spec_type
{
    const char             *name;                      /* special function name */
    SPEC_FUN               *function;                  /* the function */
};

extern const struct spec_type spec_table[];

SPEC_FUN               *spec_lookup( const char *name );
const char             *spec_name( SPEC_FUN *function );
bool                    dragon( CHAR_DATA *ch, const char *spell_name );

/*
 * All special procedures should be declared down here!
 */
bool                    spec_breath_any( CHAR_DATA *ch );
bool                    spec_breath_acid( CHAR_DATA *ch );
bool                    spec_breath_fire( CHAR_DATA *ch );
bool                    spec_breath_frost( CHAR_DATA *ch );
bool                    spec_breath_gas( CHAR_DATA *ch );
bool                    spec_breath_lightning( CHAR_DATA *ch );
bool                    spec_cast_adept( CHAR_DATA *ch );
bool                    spec_cast_cleric( CHAR_DATA *ch );
bool                    spec_cast_judge( CHAR_DATA *ch );
bool                    spec_cast_mage( CHAR_DATA *ch );
bool                    spec_cast_undead( CHAR_DATA *ch );
bool                    spec_executioner( CHAR_DATA *ch );
bool                    spec_fido( CHAR_DATA *ch );
bool                    spec_guard( CHAR_DATA *ch );
bool                    spec_janitor( CHAR_DATA *ch );
bool                    spec_mayor( CHAR_DATA *ch );
bool                    spec_poison( CHAR_DATA *ch );
bool                    spec_thief( CHAR_DATA *ch );
bool                    spec_nasty( CHAR_DATA *ch );
bool                    spec_troll_member( CHAR_DATA *ch );
bool                    spec_ogre_member( CHAR_DATA *ch );
bool                    spec_patrolman( CHAR_DATA *ch );
