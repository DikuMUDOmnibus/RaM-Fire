/*
 * RAM $Id: handler.c 70 2009-01-11 18:47:35Z quixadhal $
 */

/**************************************************************************r
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
 ***************************************************************************/

/***************************************************************************
*       ROM 2.4 is copyright 1993-1998 Russ Taylor                         *
*       ROM has been brought to you by the ROM consortium                  *
*           Russ Taylor (rtaylor@hypercube.org)                            *
*           Gabrielle Taylor (gtaylor@hypercube.org)                       *
*           Brian Moore (zump@rom.org)                                     *
*       By using this code, you have agreed to follow the terms of the     *
*       ROM license, in the file Rom24/doc/rom.license                     *
***************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "strings.h"
#include "random.h"
#include "tables.h"
#include "db.h"
#include "act.h"
#include "interp.h"
#include "magic.h"

/* friend stuff -- for NPC's mostly */
bool is_friend( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( is_same_group( ch, victim ) )
        return true;

    if ( !IS_NPC( ch ) )
        return false;

    if ( !IS_NPC( victim ) )
    {
        if ( IS_SET( ch->off_flags, ASSIST_PLAYERS ) )
            return true;
        else
            return false;
    }

    if ( IS_AFFECTED( ch, AFF_CHARM ) )
        return false;

    if ( IS_SET( ch->off_flags, ASSIST_ALL ) )
        return true;

    if ( ch->group && ch->group == victim->group )
        return true;

    if ( IS_SET( ch->off_flags, ASSIST_VNUM ) && ch->pIndexData == victim->pIndexData )
        return true;

    if ( IS_SET( ch->off_flags, ASSIST_RACE ) && ch->race == victim->race )
        return true;

    if ( IS_SET( ch->off_flags, ASSIST_ALIGN )
         && !IS_SET( ch->act, ACT_NOALIGN ) && !IS_SET( victim->act, ACT_NOALIGN )
         && ( ( IS_GOOD( ch ) && IS_GOOD( victim ) )
              || ( IS_EVIL( ch ) && IS_EVIL( victim ) )
              || ( IS_NEUTRAL( ch ) && IS_NEUTRAL( victim ) ) ) )
        return true;

    return false;
}

/* returns number of people on an object */
int count_users( OBJ_DATA *obj )
{
    CHAR_DATA              *fch = NULL;
    int                     count = 0;

    if ( obj->in_room == NULL )
        return 0;

    for ( fch = obj->in_room->people; fch != NULL; fch = fch->next_in_room )
        if ( fch->on == obj )
            count++;

    return count;
}

/* returns material number */
int material_lookup( const char *name )
{
    return 0;
}

int weapon_lookup( const char *name )
{
    int                     type = 0;

    for ( type = 0; weapon_table[type].name != NULL; type++ )
    {
        if ( LOWER( name[0] ) == LOWER( weapon_table[type].name[0] )
             && !str_prefix( name, weapon_table[type].name ) )
            return type;
    }

    return -1;
}

int weapon_type_lookup( const char *name )
{
    int                     type = 0;

    for ( type = 0; weapon_table[type].name != NULL; type++ )
    {
        if ( LOWER( name[0] ) == LOWER( weapon_table[type].name[0] )
             && !str_prefix( name, weapon_table[type].name ) )
            return weapon_table[type].type;
    }

    return WEAPON_EXOTIC;
}

const char             *item_name( int target_type )
{
    int                     type = 0;

    for ( type = 0; item_table[type].name != NULL; type++ )
        if ( target_type == item_table[type].type )
            return item_table[type].name;
    return "none";
}

const char             *weapon_name( int target_type )
{
    int                     type = 0;

    for ( type = 0; weapon_table[type].name != NULL; type++ )
        if ( target_type == weapon_table[type].type )
            return weapon_table[type].name;
    return "exotic";
}

int attack_lookup( const char *name )
{
    int                     att = 0;

    for ( att = 0; attack_table[att].name != NULL; att++ )
    {
        if ( LOWER( name[0] ) == LOWER( attack_table[att].name[0] )
             && !str_prefix( name, attack_table[att].name ) )
            return att;
    }

    return 0;
}

/* returns a flag for wiznet */
int wiznet_lookup( const char *name )
{
    int                     flag = 0;

    for ( flag = 0; wiznet_table[flag].name != NULL; flag++ )
    {
        if ( LOWER( name[0] ) == LOWER( wiznet_table[flag].name[0] )
             && !str_prefix( name, wiznet_table[flag].name ) )
            return flag;
    }

    return -1;
}

/* returns class number */
int class_lookup( const char *name )
{
    int                     iclass = 0;

    for ( iclass = 0; iclass < MAX_CLASS; iclass++ )
    {
        if ( LOWER( name[0] ) == LOWER( class_table[iclass].name[0] )
             && !str_prefix( name, class_table[iclass].name ) )
            return iclass;
    }

    return -1;
}

/* for immunity, vulnerabiltiy, and resistant
   the 'globals' (magic and weapons) may be overriden
   three other cases -- wood, silver, and iron -- are checked in fight.c */
int check_immune( CHAR_DATA *ch, int dam_type )
{
    int                     immune = -1;
    int                     def = IS_NORMAL;
    int                     bit = 0;

    if ( dam_type == DAM_NONE )
        return immune;

    if ( dam_type <= 3 )
    {
        if ( IS_SET( ch->imm_flags, IMM_WEAPON ) )
            def = IS_IMMUNE;
        else if ( IS_SET( ch->res_flags, RES_WEAPON ) )
            def = IS_RESISTANT;
        else if ( IS_SET( ch->vuln_flags, VULN_WEAPON ) )
            def = IS_VULNERABLE;
    }
    else                                               /* magical attack */
    {
        if ( IS_SET( ch->imm_flags, IMM_MAGIC ) )
            def = IS_IMMUNE;
        else if ( IS_SET( ch->res_flags, RES_MAGIC ) )
            def = IS_RESISTANT;
        else if ( IS_SET( ch->vuln_flags, VULN_MAGIC ) )
            def = IS_VULNERABLE;
    }

    /*
     * set bits to check -- VULN etc. must ALL be the same or this will fail 
     */
    switch ( dam_type )
    {
        case ( DAM_BASH ):
            bit = IMM_BASH;
            break;
        case ( DAM_PIERCE ):
            bit = IMM_PIERCE;
            break;
        case ( DAM_SLASH ):
            bit = IMM_SLASH;
            break;
        case ( DAM_FIRE ):
            bit = IMM_FIRE;
            break;
        case ( DAM_COLD ):
            bit = IMM_COLD;
            break;
        case ( DAM_LIGHTNING ):
            bit = IMM_LIGHTNING;
            break;
        case ( DAM_ACID ):
            bit = IMM_ACID;
            break;
        case ( DAM_POISON ):
            bit = IMM_POISON;
            break;
        case ( DAM_NEGATIVE ):
            bit = IMM_NEGATIVE;
            break;
        case ( DAM_HOLY ):
            bit = IMM_HOLY;
            break;
        case ( DAM_ENERGY ):
            bit = IMM_ENERGY;
            break;
        case ( DAM_MENTAL ):
            bit = IMM_MENTAL;
            break;
        case ( DAM_DISEASE ):
            bit = IMM_DISEASE;
            break;
        case ( DAM_DROWNING ):
            bit = IMM_DROWNING;
            break;
        case ( DAM_LIGHT ):
            bit = IMM_LIGHT;
            break;
        case ( DAM_CHARM ):
            bit = IMM_CHARM;
            break;
        case ( DAM_SOUND ):
            bit = IMM_SOUND;
            break;
        default:
            return def;
    }

    if ( IS_SET( ch->imm_flags, bit ) )
        immune = IS_IMMUNE;
    else if ( IS_SET( ch->res_flags, bit ) && immune != IS_IMMUNE )
        immune = IS_RESISTANT;
    else if ( IS_SET( ch->vuln_flags, bit ) )
    {
        if ( immune == IS_IMMUNE )
            immune = IS_RESISTANT;
        else if ( immune == IS_RESISTANT )
            immune = IS_NORMAL;
        else
            immune = IS_VULNERABLE;
    }

    if ( immune == -1 )
        return def;
    else
        return immune;
}

bool is_clan( CHAR_DATA *ch )
{
    return ch->clan;
}

bool is_same_clan( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( clan_table[ch->clan].independent )
        return false;
    else
        return ( ch->clan == victim->clan );
}

/* checks mob format */
bool is_old_mob( CHAR_DATA *ch )
{
    if ( ch->pIndexData == NULL )
        return false;
    else if ( ch->pIndexData->new_format )
        return false;
    return true;
}

/* for returning skill information */
int get_skill( const CHAR_DATA *ch, int sn )
{
    int                     skill = 0;

    if ( sn == -1 )                                    /* shorthand for level based
                                                        * skills */
    {
        skill = ch->level * 5 / 2;
    }

    else if ( sn < -1 || sn > MAX_SKILL )
    {
        log_error( "Bad skill number %d in get_skill", sn );
        skill = 0;
    }

    else if ( !IS_NPC( ch ) )
    {
        if ( ch->level < skill_table[sn].skill_level[ch->iclass] )
            skill = 0;
        else
            skill = ch->pcdata->learned[sn];
    }

    else                                               /* mobiles */
    {

        if ( skill_table[sn].spell_fun != spell_null )
            skill = 40 + 2 * ch->level;

        else if ( sn == skill_lookup( "sneak" ) || sn == skill_lookup( "hide" ) )
            skill = ch->level * 2 + 20;

        else if ( ( sn == skill_lookup( "dodge" ) && IS_SET( ch->off_flags, OFF_DODGE ) )
                  || ( sn == skill_lookup( "parry" )
                       && IS_SET( ch->off_flags, OFF_PARRY ) ) )
            skill = ch->level * 2;

        else if ( sn == skill_lookup( "shield block" ) )
            skill = 10 + 2 * ch->level;

        else if ( sn == skill_lookup( "second attack" )
                  && ( IS_SET( ch->act, ACT_WARRIOR ) || IS_SET( ch->act, ACT_THIEF ) ) )
            skill = 10 + 3 * ch->level;

        else if ( sn == skill_lookup( "third attack" ) && IS_SET( ch->act, ACT_WARRIOR ) )
            skill = 4 * ch->level - 40;

        else if ( sn == skill_lookup( "hand to hand" ) )
            skill = 40 + 2 * ch->level;

        else if ( sn == skill_lookup( "trip" ) && IS_SET( ch->off_flags, OFF_TRIP ) )
            skill = 10 + 3 * ch->level;

        else if ( sn == skill_lookup( "bash" ) && IS_SET( ch->off_flags, OFF_BASH ) )
            skill = 10 + 3 * ch->level;

        else if ( sn == skill_lookup( "disarm" )
                  && ( IS_SET( ch->off_flags, OFF_DISARM )
                       || IS_SET( ch->act, ACT_WARRIOR )
                       || IS_SET( ch->act, ACT_THIEF ) ) )
            skill = 20 + 3 * ch->level;

        else if ( sn == skill_lookup( "berserk" )
                  && IS_SET( ch->off_flags, OFF_BERSERK ) )
            skill = 3 * ch->level;

        else if ( sn == skill_lookup( "kick" ) )
            skill = 10 + 3 * ch->level;

        else if ( sn == skill_lookup( "backstab" ) && IS_SET( ch->act, ACT_THIEF ) )
            skill = 20 + 2 * ch->level;

        else if ( sn == skill_lookup( "rescue" ) )
            skill = 40 + ch->level;

        else if ( sn == skill_lookup( "recall" ) )
            skill = 40 + ch->level;

        else if ( sn == skill_lookup( "sword" )
                  || sn == skill_lookup( "dagger" )
                  || sn == skill_lookup( "spear" )
                  || sn == skill_lookup( "mace" )
                  || sn == skill_lookup( "axe" )
                  || sn == skill_lookup( "flail" )
                  || sn == skill_lookup( "whip" ) || sn == skill_lookup( "polearm" ) )
            skill = 40 + 5 * ch->level / 2;

        else
            skill = 0;
    }

    if ( ch->daze > 0 )
    {
        if ( skill_table[sn].spell_fun != spell_null )
            skill /= 2;
        else
            skill = 2 * skill / 3;
    }

    if ( !IS_NPC( ch ) && ch->pcdata->condition[COND_DRUNK] > 10 )
        skill = 9 * skill / 10;

    return URANGE( 0, skill, 100 );
}

int get_skill_byname( const CHAR_DATA *ch, const char *name )
{
    int                     skill = 0;
    int                     sn = -1;

    sn = skill_lookup( name );
    skill = get_skill( ch, sn );

    return skill;
}

/* for returning weapon information */
int get_weapon_sn( CHAR_DATA *ch )
{
    OBJ_DATA               *wield = NULL;
    int                     sn = -1;

    wield = get_eq_char( ch, WEAR_WIELD );
    if ( wield == NULL || wield->item_type != ITEM_WEAPON )
        sn = skill_lookup( "hand to hand" );
    else
        switch ( wield->value[0] )
        {
            default:
                sn = -1;
                break;
            case ( WEAPON_SWORD ):
                sn = skill_lookup( "sword" );
                break;
            case ( WEAPON_DAGGER ):
                sn = skill_lookup( "dagger" );
                break;
            case ( WEAPON_SPEAR ):
                sn = skill_lookup( "spear" );
                break;
            case ( WEAPON_MACE ):
                sn = skill_lookup( "mace" );
                break;
            case ( WEAPON_AXE ):
                sn = skill_lookup( "axe" );
                break;
            case ( WEAPON_FLAIL ):
                sn = skill_lookup( "flail" );
                break;
            case ( WEAPON_WHIP ):
                sn = skill_lookup( "whip" );
                break;
            case ( WEAPON_POLEARM ):
                sn = skill_lookup( "polearm" );
                break;
        }
    return sn;
}

int get_weapon_skill( CHAR_DATA *ch, int sn )
{
    int                     skill = 0;

    /*
     * -1 is exotic 
     */
    if ( IS_NPC( ch ) )
    {
        if ( sn == -1 )
            skill = 3 * ch->level;
        else if ( sn == skill_lookup( "hand to hand" ) )
            skill = 40 + 2 * ch->level;
        else
            skill = 40 + 5 * ch->level / 2;
    }

    else
    {
        if ( sn == -1 )
            skill = 3 * ch->level;
        else
            skill = ch->pcdata->learned[sn];
    }

    return URANGE( 0, skill, 100 );
}

/* used to de-screw characters */
void reset_char( CHAR_DATA *ch )
{
    int                     loc = 0;
    int                     mod = 0;
    int                     stat_index = 0;
    OBJ_DATA               *obj = NULL;
    AFFECT_DATA            *af = NULL;
    int                     i = 0;

    if ( IS_NPC( ch ) )
        return;

    if ( ch->pcdata->perm_hit == 0
         || ch->pcdata->perm_mana == 0
         || ch->pcdata->perm_move == 0 || ch->pcdata->last_level == 0 )
    {
        /*
         * do a FULL reset 
         */
        for ( loc = 0; loc < MAX_WEAR; loc++ )
        {
            obj = get_eq_char( ch, loc );
            if ( obj == NULL )
                continue;
            if ( !obj->enchanted )
                for ( af = obj->pIndexData->affected; af != NULL; af = af->next )
                {
                    mod = af->modifier;
                    switch ( af->location )
                    {
                        case APPLY_SEX:
                            ch->sex -= mod;
                            if ( ch->sex < 0 || ch->sex > 2 )
                                ch->sex = IS_NPC( ch ) ? 0 : ch->pcdata->true_sex;
                            break;
                        case APPLY_MANA:
                            ch->max_mana -= mod;
                            break;
                        case APPLY_HIT:
                            ch->max_hit -= mod;
                            break;
                        case APPLY_MOVE:
                            ch->max_move -= mod;
                            break;
                    }
                }

            for ( af = obj->affected; af != NULL; af = af->next )
            {
                mod = af->modifier;
                switch ( af->location )
                {
                    case APPLY_SEX:
                        ch->sex -= mod;
                        break;
                    case APPLY_MANA:
                        ch->max_mana -= mod;
                        break;
                    case APPLY_HIT:
                        ch->max_hit -= mod;
                        break;
                    case APPLY_MOVE:
                        ch->max_move -= mod;
                        break;
                }
            }
        }
        /*
         * now reset the permanent stats 
         */
        ch->pcdata->perm_hit = ch->max_hit;
        ch->pcdata->perm_mana = ch->max_mana;
        ch->pcdata->perm_move = ch->max_move;
        ch->pcdata->last_level = ch->played / 3600;
        if ( ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > 2 )
        {
            if ( ch->sex > 0 && ch->sex < 3 )
                ch->pcdata->true_sex = ch->sex;
            else
                ch->pcdata->true_sex = 0;
        }

    }

    /*
     * now restore the character to his/her true condition 
     */
    for ( stat_index = 0; stat_index < MAX_STATS; stat_index++ )
        ch->mod_stat[stat_index] = 0;

    if ( ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > 2 )
        ch->pcdata->true_sex = 0;
    ch->sex = ch->pcdata->true_sex;
    ch->max_hit = ch->pcdata->perm_hit;
    ch->max_mana = ch->pcdata->perm_mana;
    ch->max_move = ch->pcdata->perm_move;

    for ( i = 0; i < 4; i++ )
        ch->armor[i] = 100;

    ch->hitroll = 0;
    ch->damroll = 0;
    ch->saving_throw = 0;

    /*
     * now start adding back the effects 
     */
    for ( loc = 0; loc < MAX_WEAR; loc++ )
    {
        obj = get_eq_char( ch, loc );
        if ( obj == NULL )
            continue;
        for ( i = 0; i < 4; i++ )
            ch->armor[i] -= apply_ac( obj, loc, i );

        if ( !obj->enchanted )
            for ( af = obj->pIndexData->affected; af != NULL; af = af->next )
            {
                mod = af->modifier;
                switch ( af->location )
                {
                    case APPLY_STR:
                        ch->mod_stat[STAT_STR] += mod;
                        break;
                    case APPLY_DEX:
                        ch->mod_stat[STAT_DEX] += mod;
                        break;
                    case APPLY_INT:
                        ch->mod_stat[STAT_INT] += mod;
                        break;
                    case APPLY_WIS:
                        ch->mod_stat[STAT_WIS] += mod;
                        break;
                    case APPLY_CON:
                        ch->mod_stat[STAT_CON] += mod;
                        break;

                    case APPLY_SEX:
                        ch->sex += mod;
                        break;
                    case APPLY_MANA:
                        ch->max_mana += mod;
                        break;
                    case APPLY_HIT:
                        ch->max_hit += mod;
                        break;
                    case APPLY_MOVE:
                        ch->max_move += mod;
                        break;

                    case APPLY_AC:
                        for ( i = 0; i < 4; i++ )
                            ch->armor[i] += mod;
                        break;
                    case APPLY_HITROLL:
                        ch->hitroll += mod;
                        break;
                    case APPLY_DAMROLL:
                        ch->damroll += mod;
                        break;

                    case APPLY_SAVES:
                        ch->saving_throw += mod;
                        break;
                    case APPLY_SAVING_ROD:
                        ch->saving_throw += mod;
                        break;
                    case APPLY_SAVING_PETRI:
                        ch->saving_throw += mod;
                        break;
                    case APPLY_SAVING_BREATH:
                        ch->saving_throw += mod;
                        break;
                    case APPLY_SAVING_SPELL:
                        ch->saving_throw += mod;
                        break;
                }
            }

        for ( af = obj->affected; af != NULL; af = af->next )
        {
            mod = af->modifier;
            switch ( af->location )
            {
                case APPLY_STR:
                    ch->mod_stat[STAT_STR] += mod;
                    break;
                case APPLY_DEX:
                    ch->mod_stat[STAT_DEX] += mod;
                    break;
                case APPLY_INT:
                    ch->mod_stat[STAT_INT] += mod;
                    break;
                case APPLY_WIS:
                    ch->mod_stat[STAT_WIS] += mod;
                    break;
                case APPLY_CON:
                    ch->mod_stat[STAT_CON] += mod;
                    break;

                case APPLY_SEX:
                    ch->sex += mod;
                    break;
                case APPLY_MANA:
                    ch->max_mana += mod;
                    break;
                case APPLY_HIT:
                    ch->max_hit += mod;
                    break;
                case APPLY_MOVE:
                    ch->max_move += mod;
                    break;

                case APPLY_AC:
                    for ( i = 0; i < 4; i++ )
                        ch->armor[i] += mod;
                    break;
                case APPLY_HITROLL:
                    ch->hitroll += mod;
                    break;
                case APPLY_DAMROLL:
                    ch->damroll += mod;
                    break;

                case APPLY_SAVES:
                    ch->saving_throw += mod;
                    break;
                case APPLY_SAVING_ROD:
                    ch->saving_throw += mod;
                    break;
                case APPLY_SAVING_PETRI:
                    ch->saving_throw += mod;
                    break;
                case APPLY_SAVING_BREATH:
                    ch->saving_throw += mod;
                    break;
                case APPLY_SAVING_SPELL:
                    ch->saving_throw += mod;
                    break;
            }
        }
    }

    /*
     * now add back spell effects 
     */
    for ( af = ch->affected; af != NULL; af = af->next )
    {
        mod = af->modifier;
        switch ( af->location )
        {
            case APPLY_STR:
                ch->mod_stat[STAT_STR] += mod;
                break;
            case APPLY_DEX:
                ch->mod_stat[STAT_DEX] += mod;
                break;
            case APPLY_INT:
                ch->mod_stat[STAT_INT] += mod;
                break;
            case APPLY_WIS:
                ch->mod_stat[STAT_WIS] += mod;
                break;
            case APPLY_CON:
                ch->mod_stat[STAT_CON] += mod;
                break;

            case APPLY_SEX:
                ch->sex += mod;
                break;
            case APPLY_MANA:
                ch->max_mana += mod;
                break;
            case APPLY_HIT:
                ch->max_hit += mod;
                break;
            case APPLY_MOVE:
                ch->max_move += mod;
                break;

            case APPLY_AC:
                for ( i = 0; i < 4; i++ )
                    ch->armor[i] += mod;
                break;
            case APPLY_HITROLL:
                ch->hitroll += mod;
                break;
            case APPLY_DAMROLL:
                ch->damroll += mod;
                break;

            case APPLY_SAVES:
                ch->saving_throw += mod;
                break;
            case APPLY_SAVING_ROD:
                ch->saving_throw += mod;
                break;
            case APPLY_SAVING_PETRI:
                ch->saving_throw += mod;
                break;
            case APPLY_SAVING_BREATH:
                ch->saving_throw += mod;
                break;
            case APPLY_SAVING_SPELL:
                ch->saving_throw += mod;
                break;
        }
    }

    /*
     * make sure sex is RIGHT!!!! 
     */
    if ( ch->sex < 0 || ch->sex > 2 )
        ch->sex = ch->pcdata->true_sex;
}

/*
 * Retrieve a character's trusted level for permission checking.
 */
int get_trust( const CHAR_DATA *ch )
{
    if ( ch->desc != NULL && ch->desc->original != NULL )
        ch = ch->desc->original;

    if ( ch->trust )
        return ch->trust;

    if ( IS_NPC( ch ) && ch->level >= LEVEL_HERO )
        return LEVEL_HERO - 1;
    else
        return ch->level;
}

/*
 * Retrieve a character's age.
 */
int get_age( CHAR_DATA *ch )
{
    return 17 + ( ch->played + ( int ) ( current_time - ch->logon ) ) / 72000;
}

/* command for retrieving stats */
int get_curr_stat( const CHAR_DATA *ch, int stat_index )
{
    int                     max = 0;

    if ( IS_NPC( ch ) || ch->level > LEVEL_IMMORTAL )
        max = 25;

    else
    {
        max = pc_race_table[ch->race].max_stats[stat_index] + 4;

        if ( class_table[ch->iclass].attr_prime == stat_index )
            max += 2;

        if ( ch->race == race_lookup( "human" ) )
            max += 1;

        max = UMIN( max, 25 );
    }

    return URANGE( 3, ch->perm_stat[stat_index] + ch->mod_stat[stat_index], max );
}

/* command for returning max training score */
int get_max_train( CHAR_DATA *ch, int stat_index )
{
    int                     max = 0;

    if ( IS_NPC( ch ) || ch->level > LEVEL_IMMORTAL )
        return 25;

    max = pc_race_table[ch->race].max_stats[stat_index];
    if ( class_table[ch->iclass].attr_prime == stat_index )
    {
        if ( ch->race == race_lookup( "human" ) )
            max += 3;
        else
            max += 2;
    }

    return UMIN( max, 25 );
}

/*
 * Retrieve a character's carry capacity.
 */
int can_carry_n( CHAR_DATA *ch )
{
    if ( !IS_NPC( ch ) && ch->level >= LEVEL_IMMORTAL )
        return 1000;

    if ( IS_NPC( ch ) && IS_SET( ch->act, ACT_PET ) )
        return 0;

    return MAX_WEAR + 2 * get_curr_stat( ch, STAT_DEX ) + ch->level;
}

/*
 * Retrieve a character's carry capacity.
 */
int can_carry_w( CHAR_DATA *ch )
{
    if ( !IS_NPC( ch ) && ch->level >= LEVEL_IMMORTAL )
        return 10000000;

    if ( IS_NPC( ch ) && IS_SET( ch->act, ACT_PET ) )
        return 0;

    return str_app[get_curr_stat( ch, STAT_STR )].carry * 10 + ch->level * 25;
}

/*
 * See if a string is one of the names of an object.
 */
bool is_name( const char *str, const char *namelist )
{
    char                    name[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    part[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    const char             *list = NULL;
    const char             *string = NULL;

    /*
     * fix crash on NULL namelist 
     */
    if ( namelist == NULL || namelist[0] == '\0' )
        return false;

    /*
     * fixed to prevent is_name on "" returning true 
     */
    if ( str[0] == '\0' )
        return false;

    string = str;
    /*
     * we need ALL parts of string to match part of namelist 
     */
    for ( ;; )                                         /* start parsing string */
    {
        str = one_argument( str, part );

        if ( part[0] == '\0' )
            return true;

        /*
         * check to see if this is part of namelist 
         */
        list = namelist;
        for ( ;; )                                     /* start parsing namelist */
        {
            list = one_argument( list, name );
            if ( name[0] == '\0' )                     /* this name was not found */
                return false;

            if ( !str_prefix( string, name ) )
                return true;                           /* full pattern match */

            if ( !str_prefix( part, name ) )
                break;
        }
    }
}

bool is_exact_name( const char *str, const char *namelist )
{
    char                    name[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";

    if ( namelist == NULL )
        return false;

    for ( ;; )
    {
        namelist = one_argument( namelist, name );
        if ( name[0] == '\0' )
            return false;
        if ( !str_cmp( str, name ) )
            return true;
    }
}

/* enchanted stuff for eq */
void affect_enchant( OBJ_DATA *obj )
{
    /*
     * okay, move all the old flags into new vectors if we have to 
     */
    if ( !obj->enchanted )
    {
        AFFECT_DATA            *paf = NULL;
        AFFECT_DATA            *af_new = NULL;

        obj->enchanted = true;

        for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
        {
            af_new = new_affect(  );

            af_new->next = obj->affected;
            obj->affected = af_new;

            af_new->where = paf->where;
            af_new->type = UMAX( 0, paf->type );
            af_new->level = paf->level;
            af_new->duration = paf->duration;
            af_new->location = paf->location;
            af_new->modifier = paf->modifier;
            af_new->bitvector = paf->bitvector;
        }
    }
}

/*
 * Apply or remove an affect to a character.
 */
void affect_modify( CHAR_DATA *ch, AFFECT_DATA *paf, bool fAdd )
{
    OBJ_DATA               *wield = NULL;
    int                     mod = 0;
    int                     i = 0;

    mod = paf->modifier;

    if ( fAdd )
    {
        switch ( paf->where )
        {
            case TO_AFFECTS:
                SET_BIT( ch->affected_by, paf->bitvector );
                break;
            case TO_IMMUNE:
                SET_BIT( ch->imm_flags, paf->bitvector );
                break;
            case TO_RESIST:
                SET_BIT( ch->res_flags, paf->bitvector );
                break;
            case TO_VULN:
                SET_BIT( ch->vuln_flags, paf->bitvector );
                break;
        }
    }
    else
    {
        switch ( paf->where )
        {
            case TO_AFFECTS:
                REMOVE_BIT( ch->affected_by, paf->bitvector );
                break;
            case TO_IMMUNE:
                REMOVE_BIT( ch->imm_flags, paf->bitvector );
                break;
            case TO_RESIST:
                REMOVE_BIT( ch->res_flags, paf->bitvector );
                break;
            case TO_VULN:
                REMOVE_BIT( ch->vuln_flags, paf->bitvector );
                break;
        }
        mod = 0 - mod;
    }

    switch ( paf->location )
    {
        default:
            log_error( "Unknown affect location %d", paf->location );
            return;

        case APPLY_NONE:
            break;
        case APPLY_STR:
            ch->mod_stat[STAT_STR] += mod;
            break;
        case APPLY_DEX:
            ch->mod_stat[STAT_DEX] += mod;
            break;
        case APPLY_INT:
            ch->mod_stat[STAT_INT] += mod;
            break;
        case APPLY_WIS:
            ch->mod_stat[STAT_WIS] += mod;
            break;
        case APPLY_CON:
            ch->mod_stat[STAT_CON] += mod;
            break;
        case APPLY_SEX:
            ch->sex += mod;
            break;
        case APPLY_CLASS:
            break;
        case APPLY_LEVEL:
            break;
        case APPLY_AGE:
            break;
        case APPLY_HEIGHT:
            break;
        case APPLY_WEIGHT:
            break;
        case APPLY_MANA:
            ch->max_mana += mod;
            break;
        case APPLY_HIT:
            ch->max_hit += mod;
            break;
        case APPLY_MOVE:
            ch->max_move += mod;
            break;
        case APPLY_GOLD:
            break;
        case APPLY_EXP:
            break;
        case APPLY_AC:
            for ( i = 0; i < 4; i++ )
                ch->armor[i] += mod;
            break;
        case APPLY_HITROLL:
            ch->hitroll += mod;
            break;
        case APPLY_DAMROLL:
            ch->damroll += mod;
            break;
        case APPLY_SAVES:
            ch->saving_throw += mod;
            break;
        case APPLY_SAVING_ROD:
            ch->saving_throw += mod;
            break;
        case APPLY_SAVING_PETRI:
            ch->saving_throw += mod;
            break;
        case APPLY_SAVING_BREATH:
            ch->saving_throw += mod;
            break;
        case APPLY_SAVING_SPELL:
            ch->saving_throw += mod;
            break;
        case APPLY_SPELL_AFFECT:
            break;
    }

    /*
     * Check for weapon wielding.
     * Guard against recursion (for weapons with affects).
     */
    if ( !IS_NPC( ch ) && ( wield = get_eq_char( ch, WEAR_WIELD ) ) != NULL
         && get_obj_weight( wield ) >
         ( str_app[get_curr_stat( ch, STAT_STR )].wield * 10 ) )
    {
        static int              depth = 0;

        if ( depth == 0 )
        {
            depth++;
            act( "You drop $p.", ch, wield, NULL, TO_CHAR );
            act( "$n drops $p.", ch, wield, NULL, TO_ROOM );
            obj_from_char( wield );
            obj_to_room( wield, ch->in_room );
            depth--;
        }
    }

    return;
}

/* find an effect in an affect list */
AFFECT_DATA            *affect_find( AFFECT_DATA *paf, int sn )
{
    AFFECT_DATA            *paf_find = NULL;

    for ( paf_find = paf; paf_find != NULL; paf_find = paf_find->next )
    {
        if ( paf_find->type == sn )
            return paf_find;
    }

    return NULL;
}

/* fix object affects when removing one */
void affect_check( CHAR_DATA *ch, int where, int vector )
{
    AFFECT_DATA            *paf = NULL;
    OBJ_DATA               *obj = NULL;

    if ( where == TO_OBJECT || where == TO_WEAPON || vector == 0 )
        return;

    for ( paf = ch->affected; paf != NULL; paf = paf->next )
        if ( paf->where == where && paf->bitvector == vector )
        {
            switch ( where )
            {
                case TO_AFFECTS:
                    SET_BIT( ch->affected_by, vector );
                    break;
                case TO_IMMUNE:
                    SET_BIT( ch->imm_flags, vector );
                    break;
                case TO_RESIST:
                    SET_BIT( ch->res_flags, vector );
                    break;
                case TO_VULN:
                    SET_BIT( ch->vuln_flags, vector );
                    break;
            }
            return;
        }

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
        if ( obj->wear_loc == -1 )
            continue;

        for ( paf = obj->affected; paf != NULL; paf = paf->next )
            if ( paf->where == where && paf->bitvector == vector )
            {
                switch ( where )
                {
                    case TO_AFFECTS:
                        SET_BIT( ch->affected_by, vector );
                        break;
                    case TO_IMMUNE:
                        SET_BIT( ch->imm_flags, vector );
                        break;
                    case TO_RESIST:
                        SET_BIT( ch->res_flags, vector );
                        break;
                    case TO_VULN:
                        SET_BIT( ch->vuln_flags, vector );

                }
                return;
            }

        if ( obj->enchanted )
            continue;

        for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
            if ( paf->where == where && paf->bitvector == vector )
            {
                switch ( where )
                {
                    case TO_AFFECTS:
                        SET_BIT( ch->affected_by, vector );
                        break;
                    case TO_IMMUNE:
                        SET_BIT( ch->imm_flags, vector );
                        break;
                    case TO_RESIST:
                        SET_BIT( ch->res_flags, vector );
                        break;
                    case TO_VULN:
                        SET_BIT( ch->vuln_flags, vector );
                        break;
                }
                return;
            }
    }
}

/*
 * Give an affect to a char.
 */
void affect_to_char( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    AFFECT_DATA            *paf_new = NULL;

    paf_new = new_affect(  );
    *paf_new = *paf;
    VALIDATE( paf );                                   /* in case we missed it when we
                                                        * set up paf */
    paf_new->next = ch->affected;
    ch->affected = paf_new;

    affect_modify( ch, paf_new, true );
    return;
}

/* give an affect to an object */
void affect_to_obj( OBJ_DATA *obj, AFFECT_DATA *paf )
{
    AFFECT_DATA            *paf_new = NULL;

    paf_new = new_affect(  );
    *paf_new = *paf;
    VALIDATE( paf );                                   /* in case we missed it when we
                                                        * set up paf */
    paf_new->next = obj->affected;
    obj->affected = paf_new;

    /*
     * apply any affect vectors to the object's extra_flags 
     */
    if ( paf->bitvector )
        switch ( paf->where )
        {
            case TO_OBJECT:
                SET_BIT( obj->extra_flags, paf->bitvector );
                break;
            case TO_WEAPON:
                if ( obj->item_type == ITEM_WEAPON )
                    SET_BIT( obj->value[4], paf->bitvector );
                break;
        }

    return;
}

/*
 * Remove an affect from a char.
 */
void affect_remove( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    int                     where = 0;
    int                     vector = 0;

    if ( ch->affected == NULL )
    {
        log_error( "No affects to remove on %s", NAME( ch ) );
        return;
    }

    affect_modify( ch, paf, false );
    where = paf->where;
    vector = paf->bitvector;

    if ( paf == ch->affected )
    {
        ch->affected = paf->next;
    }
    else
    {
        AFFECT_DATA            *prev;

        for ( prev = ch->affected; prev != NULL; prev = prev->next )
        {
            if ( prev->next == paf )
            {
                prev->next = paf->next;
                break;
            }
        }

        if ( prev == NULL )
        {
            log_error( "Cannot find paf on %s", NAME( ch ) );
            return;
        }
    }

    free_affect( paf );

    affect_check( ch, where, vector );
    return;
}

void affect_remove_obj( OBJ_DATA *obj, AFFECT_DATA *paf )
{
    int                     where = 0;
    int                     vector = 0;

    if ( obj->affected == NULL )
    {
        log_error( "No affects to remove on %s", ONAME( obj ) );
        return;
    }

    if ( obj->carried_by != NULL && obj->wear_loc != -1 )
        affect_modify( obj->carried_by, paf, false );

    where = paf->where;
    vector = paf->bitvector;

    /*
     * remove flags from the object if needed 
     */
    if ( paf->bitvector )
        switch ( paf->where )
        {
            case TO_OBJECT:
                REMOVE_BIT( obj->extra_flags, paf->bitvector );
                break;
            case TO_WEAPON:
                if ( obj->item_type == ITEM_WEAPON )
                    REMOVE_BIT( obj->value[4], paf->bitvector );
                break;
        }

    if ( paf == obj->affected )
    {
        obj->affected = paf->next;
    }
    else
    {
        AFFECT_DATA            *prev;

        for ( prev = obj->affected; prev != NULL; prev = prev->next )
        {
            if ( prev->next == paf )
            {
                prev->next = paf->next;
                break;
            }
        }

        if ( prev == NULL )
        {
            log_error( "Cannot find paf on %s", ONAME( obj ) );
            return;
        }
    }

    free_affect( paf );

    if ( obj->carried_by != NULL && obj->wear_loc != -1 )
        affect_check( obj->carried_by, where, vector );
    return;
}

/*
 * Strip all affects of a given sn.
 */
void affect_strip( CHAR_DATA *ch, int sn )
{
    AFFECT_DATA            *paf = NULL;
    AFFECT_DATA            *paf_next = NULL;

    for ( paf = ch->affected; paf != NULL; paf = paf_next )
    {
        paf_next = paf->next;
        if ( paf->type == sn )
            affect_remove( ch, paf );
    }

    return;
}

/*
 * Return true if a char is affected by a spell.
 */
bool is_affected( CHAR_DATA *ch, int sn )
{
    AFFECT_DATA            *paf = NULL;

    for ( paf = ch->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->type == sn )
            return true;
    }

    return false;
}

/*
 * Add or enhance an affect.
 */
void affect_join( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    AFFECT_DATA            *paf_old = NULL;

    for ( paf_old = ch->affected; paf_old != NULL; paf_old = paf_old->next )
    {
        if ( paf_old->type == paf->type )
        {
            paf->level = ( paf->level += paf_old->level ) / 2;
            paf->duration += paf_old->duration;
            paf->modifier += paf_old->modifier;
            affect_remove( ch, paf_old );
            break;
        }
    }

    affect_to_char( ch, paf );
    return;
}

/*
 * Move a char out of a room.
 */
void char_from_room( CHAR_DATA *ch )
{
    OBJ_DATA               *obj = NULL;

    if ( ch->in_room == NULL )
    {
        log_error( "%s is in a NULL room", NAME( ch ) );
        return;
    }

    if ( !IS_NPC( ch ) )
        --ch->in_room->area->nplayer;

    if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) != NULL
         && obj->item_type == ITEM_LIGHT && obj->value[2] != 0 && ch->in_room->light > 0 )
        --ch->in_room->light;

    if ( ch == ch->in_room->people )
    {
        ch->in_room->people = ch->next_in_room;
    }
    else
    {
        CHAR_DATA              *prev = NULL;

        for ( prev = ch->in_room->people; prev; prev = prev->next_in_room )
        {
            if ( prev->next_in_room == ch )
            {
                prev->next_in_room = ch->next_in_room;
                break;
            }
        }

        if ( prev == NULL )
            log_error( "%s not found in room people list?", NAME( ch ) );
    }

    ch->in_room = NULL;
    ch->next_in_room = NULL;
    ch->on = NULL;                                     /* sanity check! */
    return;
}

/*
 * Move a char into a room.
 */
void char_to_room( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex )
{
    OBJ_DATA               *obj = NULL;

    if ( pRoomIndex == NULL )
    {
        ROOM_INDEX_DATA        *room = NULL;

        log_error( "%s tried to go to a NULL room", NAME( ch ) );

        if ( ( room = get_room_index( ROOM_VNUM_TEMPLE ) ) != NULL )
            char_to_room( ch, room );

        return;
    }

    ch->in_room = pRoomIndex;
    ch->next_in_room = pRoomIndex->people;
    pRoomIndex->people = ch;

    if ( !IS_NPC( ch ) )
    {
        if ( ch->in_room->area->empty )
        {
            ch->in_room->area->empty = false;
            ch->in_room->area->age = 0;
        }
        ++ch->in_room->area->nplayer;
    }

    if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) != NULL
         && obj->item_type == ITEM_LIGHT && obj->value[2] != 0 )
        ++ch->in_room->light;

    if ( IS_AFFECTED( ch, AFF_PLAGUE ) )
    {
        CHAR_DATA              *vch = NULL;
        AFFECT_DATA            *af = NULL;
        AFFECT_DATA             plague;
        int                     sn = -1;

        if ( ( sn = skill_lookup( "plague" ) ) == -1 )
        {
            log_error( "Can't find the \"%s\" skill in %s?", "plague", __FUNCTION__ );
            return;
        }

        for ( af = ch->affected; af != NULL; af = af->next )
        {
            if ( af->type == sn )
                break;
        }

        if ( af == NULL )
        {
            REMOVE_BIT( ch->affected_by, AFF_PLAGUE );
            return;
        }

        if ( af->level == 1 )
            return;

        plague.next = NULL;
        plague.valid = false;
        plague.where = TO_AFFECTS;
        plague.type = sn;
        plague.level = af->level - 1;
        plague.duration = number_range( 1, 2 * plague.level );
        plague.location = APPLY_STR;
        plague.modifier = -5;
        plague.bitvector = AFF_PLAGUE;

        for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room )
        {
            if ( !saves_spell( plague.level - 2, vch, DAM_DISEASE )
                 && !IS_IMMORTAL( vch ) &&
                 !IS_AFFECTED( vch, AFF_PLAGUE ) && number_bits( 6 ) == 0 )
            {
                ch_printf( vch, "You feel hot and feverish.\r\n" );
                act( "$n shivers and looks very ill.", vch, NULL, NULL, TO_ROOM );
                affect_join( vch, &plague );
            }
        }
    }

    return;
}

/*
 * Give an obj to a char.
 */
void obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch )
{
    obj->next_content = ch->carrying;
    ch->carrying = obj;
    obj->carried_by = ch;
    obj->in_room = NULL;
    obj->in_obj = NULL;
    ch->carry_number += get_obj_number( obj );
    ch->carry_weight += get_obj_weight( obj );
}

/*
 * Take an obj from its character.
 */
void obj_from_char( OBJ_DATA *obj )
{
    CHAR_DATA              *ch = NULL;

    if ( ( ch = obj->carried_by ) == NULL )
    {
        log_error( "Object %s not carried by anyone", ONAME( obj ) );
        return;
    }

    if ( obj->wear_loc != WEAR_NONE )
        unequip_char( ch, obj );

    if ( ch->carrying == obj )
    {
        ch->carrying = obj->next_content;
    }
    else
    {
        OBJ_DATA               *prev = NULL;

        for ( prev = ch->carrying; prev != NULL; prev = prev->next_content )
        {
            if ( prev->next_content == obj )
            {
                prev->next_content = obj->next_content;
                break;
            }
        }

        if ( prev == NULL )
            log_error( "Object %s not in %s carry list", ONAME( obj ), NAME( ch ) );
    }

    obj->carried_by = NULL;
    obj->next_content = NULL;
    ch->carry_number -= get_obj_number( obj );
    ch->carry_weight -= get_obj_weight( obj );
    return;
}

/*
 * Find the ac value of an obj, including position effect.
 */
int apply_ac( OBJ_DATA *obj, int iWear, int type )
{
    if ( obj->item_type != ITEM_ARMOR )
        return 0;

    switch ( iWear )
    {
        case WEAR_BODY:
            return 3 * obj->value[type];
        case WEAR_HEAD:
            return 2 * obj->value[type];
        case WEAR_LEGS:
            return 2 * obj->value[type];
        case WEAR_FEET:
            return obj->value[type];
        case WEAR_HANDS:
            return obj->value[type];
        case WEAR_ARMS:
            return obj->value[type];
        case WEAR_SHIELD:
            return obj->value[type];
        case WEAR_NECK_1:
            return obj->value[type];
        case WEAR_NECK_2:
            return obj->value[type];
        case WEAR_ABOUT:
            return 2 * obj->value[type];
        case WEAR_WAIST:
            return obj->value[type];
        case WEAR_WRIST_L:
            return obj->value[type];
        case WEAR_WRIST_R:
            return obj->value[type];
        case WEAR_HOLD:
            return obj->value[type];
    }

    return 0;
}

/*
 * Find a piece of eq on a character.
 */
OBJ_DATA               *get_eq_char( CHAR_DATA *ch, int iWear )
{
    OBJ_DATA               *obj = NULL;

    if ( ch == NULL )
        return NULL;

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
        if ( obj->wear_loc == iWear )
            return obj;
    }

    return NULL;
}

/*
 * Equip a char with an obj.
 */
void equip_char( CHAR_DATA *ch, OBJ_DATA *obj, int iWear )
{
    AFFECT_DATA            *paf = NULL;
    int                     i = 0;

    if ( get_eq_char( ch, iWear ) != NULL )
    {
        log_error( "Object %s is already equipped (%d) by %s", ONAME( obj ), iWear,
                   NAME( ch ) );
        return;
    }

    if ( ( IS_OBJ_STAT( obj, ITEM_ANTI_EVIL ) && IS_EVIL( ch ) )
         || ( IS_OBJ_STAT( obj, ITEM_ANTI_GOOD ) && IS_GOOD( ch ) )
         || ( IS_OBJ_STAT( obj, ITEM_ANTI_NEUTRAL ) && IS_NEUTRAL( ch ) ) )
    {
        /*
         * Thanks to Morgenes for the bug fix here!
         */
        act( "You are zapped by $p and drop it.", ch, obj, NULL, TO_CHAR );
        act( "$n is zapped by $p and drops it.", ch, obj, NULL, TO_ROOM );
        obj_from_char( obj );
        if ( IS_OBJ_STAT( obj, ITEM_NODROP ) )
        {
            obj_to_char( obj, ch );
        }
        else
        {
            obj_to_room( obj, ch->in_room );
            if ( IS_OBJ_STAT( obj, ITEM_MELT_DROP ) )
            {
                act( "$p dissolves into smoke.", ch, obj, NULL, TO_ROOM );
                act( "$p dissolves into smoke.", ch, obj, NULL, TO_CHAR );
                extract_obj( obj );
            }
        }
        return;
    }

    for ( i = 0; i < 4; i++ )
        ch->armor[i] -= apply_ac( obj, iWear, i );
    obj->wear_loc = iWear;

    if ( !obj->enchanted )
        for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
            if ( paf->location != APPLY_SPELL_AFFECT )
                affect_modify( ch, paf, true );
    for ( paf = obj->affected; paf != NULL; paf = paf->next )
        if ( paf->location == APPLY_SPELL_AFFECT )
            affect_to_char( ch, paf );
        else
            affect_modify( ch, paf, true );

    if ( obj->item_type == ITEM_LIGHT && obj->value[2] != 0 && ch->in_room != NULL )
        ++ch->in_room->light;

    return;
}

/*
 * Unequip a char with an obj.
 */
void unequip_char( CHAR_DATA *ch, OBJ_DATA *obj )
{
    AFFECT_DATA            *paf = NULL;
    AFFECT_DATA            *lpaf = NULL;
    AFFECT_DATA            *lpaf_next = NULL;
    int                     i = 0;

    if ( obj->wear_loc == WEAR_NONE )
    {
        log_error( "Object %s is not equipped by %s.", ONAME( obj ), NAME( ch ) );
        return;
    }

    for ( i = 0; i < 4; i++ )
        ch->armor[i] += apply_ac( obj, obj->wear_loc, i );
    obj->wear_loc = -1;

    if ( !obj->enchanted )
    {
        for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
        {
            if ( paf->location == APPLY_SPELL_AFFECT )
            {
                for ( lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next )
                {
                    lpaf_next = lpaf->next;
                    if ( ( lpaf->type == paf->type ) &&
                         ( lpaf->level == paf->level ) &&
                         ( lpaf->location == APPLY_SPELL_AFFECT ) )
                    {
                        affect_remove( ch, lpaf );
                        lpaf_next = NULL;
                    }
                }
            }
            else
            {
                affect_modify( ch, paf, false );
                affect_check( ch, paf->where, paf->bitvector );
            }
        }
    }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
        if ( paf->location == APPLY_SPELL_AFFECT )
        {
            log_error( "Norm-Apply: %d", paf->location );
            for ( lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next )
            {
                lpaf_next = lpaf->next;
                if ( ( lpaf->type == paf->type ) &&
                     ( lpaf->level == paf->level ) &&
                     ( lpaf->location == APPLY_SPELL_AFFECT ) )
                {
                    log_error( "location = %d", lpaf->location );
                    log_error( "type = %d", lpaf->type );
                    affect_remove( ch, lpaf );
                    lpaf_next = NULL;
                }
            }
        }
        else
        {
            affect_modify( ch, paf, false );
            affect_check( ch, paf->where, paf->bitvector );
        }

    if ( obj->item_type == ITEM_LIGHT
         && obj->value[2] != 0 && ch->in_room != NULL && ch->in_room->light > 0 )
        --ch->in_room->light;

    return;
}

/*
 * Count occurrences of an obj in a list.
 */
int count_obj_list( OBJ_INDEX_DATA *pObjIndex, OBJ_DATA *list )
{
    OBJ_DATA               *obj = NULL;
    int                     nMatch = 0;

    nMatch = 0;
    for ( obj = list; obj != NULL; obj = obj->next_content )
    {
        if ( obj->pIndexData == pObjIndex )
            nMatch++;
    }

    return nMatch;
}

/*
 * Move an obj out of a room.
 */
void obj_from_room( OBJ_DATA *obj )
{
    ROOM_INDEX_DATA        *in_room = NULL;
    CHAR_DATA              *ch = NULL;

    if ( ( in_room = obj->in_room ) == NULL )
    {
        log_error( "Object %s is in a NULL room", ONAME( obj ) );
        return;
    }

    for ( ch = in_room->people; ch != NULL; ch = ch->next_in_room )
        if ( ch->on == obj )
            ch->on = NULL;

    if ( obj == in_room->contents )
    {
        in_room->contents = obj->next_content;
    }
    else
    {
        OBJ_DATA               *prev;

        for ( prev = in_room->contents; prev; prev = prev->next_content )
        {
            if ( prev->next_content == obj )
            {
                prev->next_content = obj->next_content;
                break;
            }
        }

        if ( prev == NULL )
        {
            log_error( "Object %s not found in list", ONAME( obj ) );
            return;
        }
    }

    obj->in_room = NULL;
    obj->next_content = NULL;
    return;
}

/*
 * Move an obj into a room.
 */
void obj_to_room( OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex )
{
    obj->next_content = pRoomIndex->contents;
    pRoomIndex->contents = obj;
    obj->in_room = pRoomIndex;
    obj->carried_by = NULL;
    obj->in_obj = NULL;
    return;
}

/*
 * Move an object into an object.
 */
void obj_to_obj( OBJ_DATA *obj, OBJ_DATA *obj_to )
{
    obj->next_content = obj_to->contains;
    obj_to->contains = obj;
    obj->in_obj = obj_to;
    obj->in_room = NULL;
    obj->carried_by = NULL;
    if ( obj_to->pIndexData->vnum == OBJ_VNUM_PIT )
        obj->cost = 0;

    for ( ; obj_to != NULL; obj_to = obj_to->in_obj )
    {
        if ( obj_to->carried_by != NULL )
        {
            obj_to->carried_by->carry_number += get_obj_number( obj );
            obj_to->carried_by->carry_weight += get_obj_weight( obj )
                * WEIGHT_MULT( obj_to ) / 100;
        }
    }

    return;
}

/*
 * Move an object out of an object.
 */
void obj_from_obj( OBJ_DATA *obj )
{
    OBJ_DATA               *obj_from = NULL;

    if ( ( obj_from = obj->in_obj ) == NULL )
    {
        log_error( "Object %s has NULL obj_from", ONAME( obj ) );
        return;
    }

    if ( obj == obj_from->contains )
    {
        obj_from->contains = obj->next_content;
    }
    else
    {
        OBJ_DATA               *prev = NULL;

        for ( prev = obj_from->contains; prev; prev = prev->next_content )
        {
            if ( prev->next_content == obj )
            {
                prev->next_content = obj->next_content;
                break;
            }
        }

        if ( prev == NULL )
        {
            log_error( "Object %s not found in list", ONAME( obj ) );
            return;
        }
    }

    obj->next_content = NULL;
    obj->in_obj = NULL;

    for ( ; obj_from != NULL; obj_from = obj_from->in_obj )
    {
        if ( obj_from->carried_by != NULL )
        {
            obj_from->carried_by->carry_number -= get_obj_number( obj );
            obj_from->carried_by->carry_weight -= get_obj_weight( obj )
                * WEIGHT_MULT( obj_from ) / 100;
        }
    }

    return;
}

/*
 * Extract an obj from the world.
 */
void extract_obj( OBJ_DATA *obj )
{
    OBJ_DATA               *obj_content = NULL;
    OBJ_DATA               *obj_next = NULL;

    if ( obj->in_room != NULL )
        obj_from_room( obj );
    else if ( obj->carried_by != NULL )
        obj_from_char( obj );
    else if ( obj->in_obj != NULL )
        obj_from_obj( obj );

    for ( obj_content = obj->contains; obj_content; obj_content = obj_next )
    {
        obj_next = obj_content->next_content;
        extract_obj( obj_content );
    }

    if ( object_list == obj )
    {
        object_list = obj->next;
    }
    else
    {
        OBJ_DATA               *prev;

        for ( prev = object_list; prev != NULL; prev = prev->next )
        {
            if ( prev->next == obj )
            {
                prev->next = obj->next;
                break;
            }
        }

        if ( prev == NULL )
        {
            log_error( "Object %s (%d) not found in list", ONAME( obj ),
                       obj->pIndexData->vnum );
            return;
        }
    }

    --obj->pIndexData->count;
    free_obj( obj );
    return;
}

/*
 * Extract a char from the world.
 */
void extract_char( CHAR_DATA *ch, bool fPull )
{
    CHAR_DATA              *wch = NULL;
    OBJ_DATA               *obj = NULL;
    OBJ_DATA               *obj_next = NULL;

    /*
     * doesn't seem to be necessary
     */
#if 0
    if ( ch->in_room == NULL )
    {
        log_error( "%s is in a NULL room", NAME( ch ) );
        return;
    }
#endif

    nuke_pets( ch );
    ch->pet = NULL;                                    /* just in case */

    if ( fPull )

        die_follower( ch );

    stop_fighting( ch, true );

    for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    {
        obj_next = obj->next_content;
        extract_obj( obj );
    }

    if ( ch->in_room != NULL )
        char_from_room( ch );

    /*
     * Death room is set in the clan tabe now 
     */
    if ( !fPull )
    {
        char_to_room( ch, get_room_index( clan_table[ch->clan].hall ) );
        return;
    }

    if ( IS_NPC( ch ) )
        --ch->pIndexData->count;

    if ( ch->desc != NULL && ch->desc->original != NULL )
    {
        do_function( ch, &do_return, "" );
        ch->desc = NULL;
    }

    for ( wch = char_list; wch != NULL; wch = wch->next )
    {
        if ( wch->reply == ch )
            wch->reply = NULL;
        if ( ch->mprog_target == wch )
            wch->mprog_target = NULL;
    }

    if ( ch == char_list )
    {
        char_list = ch->next;
    }
    else
    {
        CHAR_DATA              *prev;

        for ( prev = char_list; prev != NULL; prev = prev->next )
        {
            if ( prev->next == ch )
            {
                prev->next = ch->next;
                break;
            }
        }

        if ( prev == NULL )
        {
            log_error( "%s not found in list", NAME( ch ) );
            return;
        }
    }

    if ( ch->desc != NULL )
        ch->desc->character = NULL;
    free_char( ch );
    return;
}

/*
 * Find a char in the room.
 */
CHAR_DATA              *get_char_room( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *rch = NULL;
    int                     number = 0;
    int                     count = 0;

    number = number_argument( argument, arg );
    count = 0;
    if ( !str_cmp( arg, "self" ) )
        return ch;
    for ( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room )
    {
        if ( !can_see( ch, rch ) || !is_name( arg, rch->name ) )
            continue;
        if ( ++count == number )
            return rch;
    }

    return NULL;
}

/*
 * Find a char in the world.
 */
CHAR_DATA              *get_char_world( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *wch = NULL;
    int                     number = 0;
    int                     count = 0;

    if ( ( wch = get_char_room( ch, argument ) ) != NULL )
        return wch;

    number = number_argument( argument, arg );
    count = 0;
    for ( wch = char_list; wch != NULL; wch = wch->next )
    {
        if ( wch->in_room == NULL || !can_see( ch, wch ) || !is_name( arg, wch->name ) )
            continue;
        if ( ++count == number )
            return wch;
    }

    return NULL;
}

/*
 * Find some object with a given index data.
 * Used by area-reset 'P' command.
 */
OBJ_DATA               *get_obj_type( OBJ_INDEX_DATA *pObjIndex )
{
    OBJ_DATA               *obj = NULL;

    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
        if ( obj->pIndexData == pObjIndex )
            return obj;
    }

    return NULL;
}

/*
 * Find an obj in a list.
 */
OBJ_DATA               *get_obj_list( CHAR_DATA *ch, const char *argument,
                                      OBJ_DATA *list )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;
    int                     number = 0;
    int                     count = 0;

    number = number_argument( argument, arg );
    count = 0;
    for ( obj = list; obj != NULL; obj = obj->next_content )
    {
        if ( can_see_obj( ch, obj ) && is_name( arg, obj->name ) )
        {
            if ( ++count == number )
                return obj;
        }
    }

    return NULL;
}

/*
 * Find an obj in player's inventory.
 */
OBJ_DATA               *get_obj_carry( const CHAR_DATA *ch, const char *argument,
                                       const CHAR_DATA *viewer )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;
    int                     number = 0;
    int                     count = 0;

    number = number_argument( argument, arg );
    count = 0;
    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
        if ( obj->wear_loc == WEAR_NONE
             && ( can_see_obj( viewer, obj ) ) && is_name( arg, obj->name ) )
        {
            if ( ++count == number )
                return obj;
        }
    }

    return NULL;
}

/*
 * Find an obj in player's equipment.
 */
OBJ_DATA               *get_obj_wear( const CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;
    int                     number = 0;
    int                     count = 0;

    number = number_argument( argument, arg );
    count = 0;
    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
        if ( obj->wear_loc != WEAR_NONE
             && can_see_obj( ch, obj ) && is_name( arg, obj->name ) )
        {
            if ( ++count == number )
                return obj;
        }
    }

    return NULL;
}

/*
 * Find an obj in the room or in inventory.
 */
OBJ_DATA               *get_obj_here( CHAR_DATA *ch, const char *argument )
{
    OBJ_DATA               *obj = NULL;

    obj = get_obj_list( ch, argument, ch->in_room->contents );
    if ( obj != NULL )
        return obj;

    if ( ( obj = get_obj_carry( ch, argument, ch ) ) != NULL )
        return obj;

    if ( ( obj = get_obj_wear( ch, argument ) ) != NULL )
        return obj;

    return NULL;
}

/*
 * Find an obj in the world.
 */
OBJ_DATA               *get_obj_world( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;
    int                     number = 0;
    int                     count = 0;

    if ( ( obj = get_obj_here( ch, argument ) ) != NULL )
        return obj;

    number = number_argument( argument, arg );
    count = 0;
    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
        if ( can_see_obj( ch, obj ) && is_name( arg, obj->name ) )
        {
            if ( ++count == number )
                return obj;
        }
    }

    return NULL;
}

/* deduct cost from a character */
void deduct_cost( CHAR_DATA *ch, int cost )
{
    int                     silver = 0;
    int                     gold = 0;

    silver = UMIN( ch->silver, cost );

    if ( silver < cost )
    {
        gold = ( ( cost - silver + 99 ) / 100 );
        silver = cost - 100 * gold;
    }

    ch->gold -= gold;
    ch->silver -= silver;

    if ( ch->gold < 0 )
    {
        log_balance( ch, "Negative gold? %d < 0", ch->gold );
        ch->gold = 0;
    }
    if ( ch->silver < 0 )
    {
        log_balance( ch, "Negative silver? %d < 0", ch->silver );
        ch->silver = 0;
    }
}

/*
 * Create a 'money' obj.
 */
OBJ_DATA               *create_money( int gold, int silver )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;

    if ( gold < 0 || silver < 0 || ( gold == 0 && silver == 0 ) )
    {
        log_error( "Zero or negative money object! (%d)", UMIN( gold, silver ) );
        gold = UMAX( 1, gold );
        silver = UMAX( 1, silver );
    }

    if ( gold == 0 && silver == 1 )
    {
        obj = create_object( get_obj_index( OBJ_VNUM_SILVER_ONE ), 0 );
    }
    else if ( gold == 1 && silver == 0 )
    {
        obj = create_object( get_obj_index( OBJ_VNUM_GOLD_ONE ), 0 );
    }
    else if ( silver == 0 )
    {
        obj = create_object( get_obj_index( OBJ_VNUM_GOLD_SOME ), 0 );
        sprintf( buf, obj->short_descr, gold );
        free_string( obj->short_descr );
        obj->short_descr = str_dup( buf );
        obj->value[1] = gold;
        obj->cost = gold;
        obj->weight = gold / 5;
    }
    else if ( gold == 0 )
    {
        obj = create_object( get_obj_index( OBJ_VNUM_SILVER_SOME ), 0 );
        sprintf( buf, obj->short_descr, silver );
        free_string( obj->short_descr );
        obj->short_descr = str_dup( buf );
        obj->value[0] = silver;
        obj->cost = silver;
        obj->weight = silver / 20;
    }

    else
    {
        obj = create_object( get_obj_index( OBJ_VNUM_COINS ), 0 );
        sprintf( buf, obj->short_descr, silver, gold );
        free_string( obj->short_descr );
        obj->short_descr = str_dup( buf );
        obj->value[0] = silver;
        obj->value[1] = gold;
        obj->cost = 100 * gold + silver;
        obj->weight = gold / 5 + silver / 20;
    }

    return obj;
}

/*
 * Return # of objects which an object counts as.
 * Thanks to Tony Chamberlain for the correct recursive code here.
 */
int get_obj_number( OBJ_DATA *obj )
{
    int                     number = 0;

    if ( obj->item_type == ITEM_CONTAINER || obj->item_type == ITEM_MONEY
         || obj->item_type == ITEM_GEM || obj->item_type == ITEM_JEWELRY )
        number = 0;
    else
        number = 1;

    for ( obj = obj->contains; obj != NULL; obj = obj->next_content )
        number += get_obj_number( obj );

    return number;
}

/*
 * Return weight of an object, including weight of contents.
 */
int get_obj_weight( OBJ_DATA *obj )
{
    int                     weight = 0;
    OBJ_DATA               *tobj = NULL;

    weight = obj->weight;
    for ( tobj = obj->contains; tobj != NULL; tobj = tobj->next_content )
        weight += get_obj_weight( tobj ) * WEIGHT_MULT( obj ) / 100;

    return weight;
}

int get_true_weight( OBJ_DATA *obj )
{
    int                     weight = 0;

    weight = obj->weight;
    for ( obj = obj->contains; obj != NULL; obj = obj->next_content )
        weight += get_obj_weight( obj );

    return weight;
}

/*
 * True if room is dark.
 */
bool room_is_dark( ROOM_INDEX_DATA *pRoomIndex )
{
    if ( pRoomIndex->light > 0 )
        return false;

    if ( IS_SET( pRoomIndex->room_flags, ROOM_DARK ) )
        return true;

    if ( pRoomIndex->sector_type == SECT_INSIDE || pRoomIndex->sector_type == SECT_CITY )
        return false;

    if ( weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK )
        return true;

    return false;
}

bool is_room_owner( CHAR_DATA *ch, ROOM_INDEX_DATA *room )
{
    if ( room->owner == NULL || room->owner[0] == '\0' )
        return false;

    return is_name( ch->name, room->owner );
}

/*
 * True if room is private.
 */
bool room_is_private( ROOM_INDEX_DATA *pRoomIndex )
{
    CHAR_DATA              *rch = NULL;
    int                     count = 0;

    if ( pRoomIndex->owner != NULL && pRoomIndex->owner[0] != '\0' )
        return true;

    count = 0;
    for ( rch = pRoomIndex->people; rch != NULL; rch = rch->next_in_room )
        count++;

    if ( IS_SET( pRoomIndex->room_flags, ROOM_PRIVATE ) && count >= 2 )
        return true;

    if ( IS_SET( pRoomIndex->room_flags, ROOM_SOLITARY ) && count >= 1 )
        return true;

    if ( IS_SET( pRoomIndex->room_flags, ROOM_IMP_ONLY ) )
        return true;

    return false;
}

/* visibility on a room -- for entering and exits */
bool can_see_room( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex )
{
    if ( IS_SET( pRoomIndex->room_flags, ROOM_IMP_ONLY ) && get_trust( ch ) < MAX_LEVEL )
        return false;

    if ( IS_SET( pRoomIndex->room_flags, ROOM_GODS_ONLY ) && !IS_IMMORTAL( ch ) )
        return false;

    if ( IS_SET( pRoomIndex->room_flags, ROOM_HEROES_ONLY ) && !IS_IMMORTAL( ch ) )
        return false;

    if ( IS_SET( pRoomIndex->room_flags, ROOM_NEWBIES_ONLY )
         && ch->level > 5 && !IS_IMMORTAL( ch ) )
        return false;

    if ( !IS_IMMORTAL( ch ) && pRoomIndex->clan && ch->clan != pRoomIndex->clan )
        return false;

    return true;
}

/*
 * True if char can see victim.
 */
bool can_see( const CHAR_DATA *ch, const CHAR_DATA *victim )
{
    if ( !ch || !victim )
        return false;

/* RT changed so that WIZ_INVIS has levels */
    if ( ch == victim )
        return true;

    if ( get_trust( ch ) < victim->invis_level )
        return false;

    if ( get_trust( ch ) < victim->incog_level && ch->in_room != victim->in_room )
        return false;

    if ( ( !IS_NPC( ch ) && IS_SET( ch->act, PLR_HOLYLIGHT ) )
         || ( IS_NPC( ch ) && IS_IMMORTAL( ch ) ) )
        return true;

    if ( IS_AFFECTED( ch, AFF_BLIND ) )
        return false;

    if ( room_is_dark( ch->in_room ) && !IS_AFFECTED( ch, AFF_INFRARED ) )
        return false;

    if ( IS_AFFECTED( victim, AFF_INVISIBLE ) && !IS_AFFECTED( ch, AFF_DETECT_INVISIBILITY ) )
        return false;

    /*
     * sneaking 
     */
    if ( IS_AFFECTED( victim, AFF_SNEAK )
         && !IS_AFFECTED( ch, AFF_DETECT_HIDDEN ) && victim->fighting == NULL )
    {
        int                     chance = 0;

        chance = get_skill( victim, skill_lookup( "sneak" ) );
        chance += get_curr_stat( victim, STAT_DEX ) * 3 / 2;
        chance -= get_curr_stat( ch, STAT_INT ) * 2;
        chance -= ch->level - victim->level * 3 / 2;

        if ( number_percent(  ) < chance )
            return false;
    }

    if ( IS_AFFECTED( victim, AFF_HIDE )
         && !IS_AFFECTED( ch, AFF_DETECT_HIDDEN ) && victim->fighting == NULL )
        return false;

    return true;
}

/*
 * True if char can see obj.
 */
bool can_see_obj( const CHAR_DATA *ch, const OBJ_DATA *obj )
{
    if ( !ch || !obj )
        return false;

    if ( !IS_NPC( ch ) && IS_SET( ch->act, PLR_HOLYLIGHT ) )
        return true;

    if ( IS_SET( obj->extra_flags, ITEM_VIS_DEATH ) )
        return false;

    if ( IS_AFFECTED( ch, AFF_BLIND ) && obj->item_type != ITEM_POTION )
        return false;

    if ( obj->item_type == ITEM_LIGHT && obj->value[2] != 0 )
        return true;

    if ( IS_SET( obj->extra_flags, ITEM_INVIS ) && !IS_AFFECTED( ch, AFF_DETECT_INVISIBILITY ) )
        return false;

    if ( IS_OBJ_STAT( obj, ITEM_GLOW ) )
        return true;

    if ( room_is_dark( ch->in_room ) && !IS_AFFECTED( ch, AFF_DARK_VISION ) )
        return false;

    return true;
}

/*
 * True if char can drop obj.
 */
bool can_drop_obj( CHAR_DATA *ch, OBJ_DATA *obj )
{
    if ( !IS_SET( obj->extra_flags, ITEM_NODROP ) )
        return true;

    if ( !IS_NPC( ch ) && ch->level >= LEVEL_IMMORTAL )
        return true;

    return false;
}

/*
 * Return ascii name of an affect location.
 */
const char             *affect_loc_name( int location )
{
    switch ( location )
    {
        case APPLY_NONE:
            return "none";
        case APPLY_STR:
            return "strength";
        case APPLY_DEX:
            return "dexterity";
        case APPLY_INT:
            return "intelligence";
        case APPLY_WIS:
            return "wisdom";
        case APPLY_CON:
            return "constitution";
        case APPLY_SEX:
            return "sex";
        case APPLY_CLASS:
            return "class";
        case APPLY_LEVEL:
            return "level";
        case APPLY_AGE:
            return "age";
        case APPLY_MANA:
            return "mana";
        case APPLY_HIT:
            return "hp";
        case APPLY_MOVE:
            return "moves";
        case APPLY_GOLD:
            return "gold";
        case APPLY_EXP:
            return "experience";
        case APPLY_AC:
            return "armor class";
        case APPLY_HITROLL:
            return "hit roll";
        case APPLY_DAMROLL:
            return "damage roll";
        case APPLY_SAVES:
            return "saves";
        case APPLY_SAVING_ROD:
            return "save vs rod";
        case APPLY_SAVING_PETRI:
            return "save vs petrification";
        case APPLY_SAVING_BREATH:
            return "save vs breath";
        case APPLY_SAVING_SPELL:
            return "save vs spell";
        case APPLY_SPELL_AFFECT:
            return "none";
    }

    log_error( "Unknown location %d", location );
    return "(unknown)";
}

/*
 * Return ascii name of an affect bit vector.
 */
const char             *affect_bit_name( int vector )
{
    static char             buf[512] = "\0\0\0\0\0\0\0";

    *buf = '\0';
    if ( vector & AFF_BLIND )
        strcat( buf, " blind" );
    if ( vector & AFF_INVISIBLE )
        strcat( buf, " invisible" );
    if ( vector & AFF_DETECT_EVIL )
        strcat( buf, " detect_evil" );
    if ( vector & AFF_DETECT_GOOD )
        strcat( buf, " detect_good" );
    if ( vector & AFF_DETECT_INVISIBILITY )
        strcat( buf, " detect_invisibility" );
    if ( vector & AFF_DETECT_MAGIC )
        strcat( buf, " detect_magic" );
    if ( vector & AFF_DETECT_HIDDEN )
        strcat( buf, " detect_hidden" );
    if ( vector & AFF_SANCTUARY )
        strcat( buf, " sanctuary" );
    if ( vector & AFF_FAERIE_FIRE )
        strcat( buf, " faerie_fire" );
    if ( vector & AFF_INFRARED )
        strcat( buf, " infrared" );
    if ( vector & AFF_CURSE )
        strcat( buf, " curse" );
    if ( vector & AFF_POISON )
        strcat( buf, " poison" );
    if ( vector & AFF_PROTECT_EVIL )
        strcat( buf, " prot_evil" );
    if ( vector & AFF_PROTECT_GOOD )
        strcat( buf, " prot_good" );
    if ( vector & AFF_SLEEP )
        strcat( buf, " sleep" );
    if ( vector & AFF_SNEAK )
        strcat( buf, " sneak" );
    if ( vector & AFF_HIDE )
        strcat( buf, " hide" );
    if ( vector & AFF_CHARM )
        strcat( buf, " charm" );
    if ( vector & AFF_FLYING )
        strcat( buf, " flying" );
    if ( vector & AFF_PASS_DOOR )
        strcat( buf, " pass_door" );
    if ( vector & AFF_BERSERK )
        strcat( buf, " berserk" );
    if ( vector & AFF_CALM )
        strcat( buf, " calm" );
    if ( vector & AFF_HASTE )
        strcat( buf, " haste" );
    if ( vector & AFF_SLOW )
        strcat( buf, " slow" );
    if ( vector & AFF_PLAGUE )
        strcat( buf, " plague" );
    if ( vector & AFF_DARK_VISION )
        strcat( buf, " dark_vision" );
    if ( !*buf )
        strcat( buf, " none" );

    return ( char * ) ( buf + 1 );
}

/*
 * Return ascii name of extra flags vector.
 */
const char             *extra_bit_name( int vector )
{
    static char             buf[512] = "\0\0\0\0\0\0\0";

    *buf = '\0';
    if ( vector & ITEM_GLOW )
        strcat( buf, " glow" );
    if ( vector & ITEM_HUM )
        strcat( buf, " hum" );
    if ( vector & ITEM_DARK )
        strcat( buf, " dark" );
    if ( vector & ITEM_LOCK )
        strcat( buf, " lock" );
    if ( vector & ITEM_EVIL )
        strcat( buf, " evil" );
    if ( vector & ITEM_INVIS )
        strcat( buf, " invis" );
    if ( vector & ITEM_MAGIC )
        strcat( buf, " magic" );
    if ( vector & ITEM_NODROP )
        strcat( buf, " nodrop" );
    if ( vector & ITEM_BLESS )
        strcat( buf, " bless" );
    if ( vector & ITEM_ANTI_GOOD )
        strcat( buf, " anti-good" );
    if ( vector & ITEM_ANTI_EVIL )
        strcat( buf, " anti-evil" );
    if ( vector & ITEM_ANTI_NEUTRAL )
        strcat( buf, " anti-neutral" );
    if ( vector & ITEM_NOREMOVE )
        strcat( buf, " noremove" );
    if ( vector & ITEM_INVENTORY )
        strcat( buf, " inventory" );
    if ( vector & ITEM_NOPURGE )
        strcat( buf, " nopurge" );
    if ( vector & ITEM_VIS_DEATH )
        strcat( buf, " vis_death" );
    if ( vector & ITEM_ROT_DEATH )
        strcat( buf, " rot_death" );
    if ( vector & ITEM_NOLOCATE )
        strcat( buf, " no_locate" );
    if ( vector & ITEM_SELL_EXTRACT )
        strcat( buf, " sell_extract" );
    if ( vector & ITEM_BURN_PROOF )
        strcat( buf, " burn_proof" );
    if ( vector & ITEM_NOUNCURSE )
        strcat( buf, " no_uncurse" );
    if ( !*buf )
        strcat( buf, " none" );

    return ( char * ) ( buf + 1 );
}

/* return ascii name of an act vector */
const char             *act_bit_name( int vector )
{
    static char             buf[512] = "\0\0\0\0\0\0\0";

    *buf = '\0';
    if ( IS_SET( vector, ACT_IS_NPC ) )
    {
        strcat( buf, " npc" );
        if ( vector & ACT_SENTINEL )
            strcat( buf, " sentinel" );
        if ( vector & ACT_SCAVENGER )
            strcat( buf, " scavenger" );
        if ( vector & ACT_AGGRESSIVE )
            strcat( buf, " aggressive" );
        if ( vector & ACT_STAY_AREA )
            strcat( buf, " stay_area" );
        if ( vector & ACT_WIMPY )
            strcat( buf, " wimpy" );
        if ( vector & ACT_PET )
            strcat( buf, " pet" );
        if ( vector & ACT_TRAIN )
            strcat( buf, " train" );
        if ( vector & ACT_PRACTICE )
            strcat( buf, " practice" );
        if ( vector & ACT_UNDEAD )
            strcat( buf, " undead" );
        if ( vector & ACT_CLERIC )
            strcat( buf, " cleric" );
        if ( vector & ACT_MAGE )
            strcat( buf, " mage" );
        if ( vector & ACT_THIEF )
            strcat( buf, " thief" );
        if ( vector & ACT_WARRIOR )
            strcat( buf, " warrior" );
        if ( vector & ACT_NOALIGN )
            strcat( buf, " no_align" );
        if ( vector & ACT_NOPURGE )
            strcat( buf, " no_purge" );
        if ( vector & ACT_IS_HEALER )
            strcat( buf, " healer" );
        if ( vector & ACT_IS_CHANGER )
            strcat( buf, " changer" );
        if ( vector & ACT_GAIN )
            strcat( buf, " skill_train" );
        if ( vector & ACT_UPDATE_ALWAYS )
            strcat( buf, " update_always" );
    }
    else
    {
        strcat( buf, " player" );
        if ( vector & PLR_AUTOASSIST )
            strcat( buf, " autoassist" );
        if ( vector & PLR_AUTOEXIT )
            strcat( buf, " autoexit" );
        if ( vector & PLR_AUTOLOOT )
            strcat( buf, " autoloot" );
        if ( vector & PLR_AUTOSAC )
            strcat( buf, " autosac" );
        if ( vector & PLR_AUTOGOLD )
            strcat( buf, " autogold" );
        if ( vector & PLR_AUTOSPLIT )
            strcat( buf, " autosplit" );
        if ( vector & PLR_HOLYLIGHT )
            strcat( buf, " holy_light" );
        if ( vector & PLR_CANLOOT )
            strcat( buf, " loot_corpse" );
        if ( vector & PLR_NOSUMMON )
            strcat( buf, " no_summon" );
        if ( vector & PLR_NOFOLLOW )
            strcat( buf, " no_follow" );
        if ( vector & PLR_FREEZE )
            strcat( buf, " frozen" );
        if ( vector & PLR_THIEF )
            strcat( buf, " thief" );
        if ( vector & PLR_KILLER )
            strcat( buf, " killer" );
    }
    if ( !*buf )
        strcat( buf, " none" );

    return ( char * ) ( buf + 1 );
}

const char             *comm_bit_name( int vector )
{
    static char             buf[512] = "\0\0\0\0\0\0\0";

    *buf = '\0';
    if ( vector & COMM_QUIET )
        strcat( buf, " quiet" );
    if ( vector & COMM_DEAF )
        strcat( buf, " deaf" );
    if ( vector & COMM_NOWIZ )
        strcat( buf, " no_wiz" );
    if ( vector & COMM_NOAUCTION )
        strcat( buf, " no_auction" );
    if ( vector & COMM_NOGOSSIP )
        strcat( buf, " no_gossip" );
    if ( vector & COMM_NOQUESTION )
        strcat( buf, " no_question" );
    /*
     * if ( vector & COMM_NOMUSIC ) strcat(buf, " no_music"); Legacy 
     */
    if ( vector & COMM_NOQUOTE )
        strcat( buf, " no_quote" );
    if ( vector & COMM_COMPACT )
        strcat( buf, " compact" );
    if ( vector & COMM_BRIEF )
        strcat( buf, " brief" );
    if ( vector & COMM_PROMPT )
        strcat( buf, " prompt" );
    if ( vector & COMM_COMBINE )
        strcat( buf, " combine" );
    if ( vector & COMM_NOEMOTE )
        strcat( buf, " no_emote" );
    if ( vector & COMM_NOSHOUT )
        strcat( buf, " no_shout" );
    if ( vector & COMM_NOTELL )
        strcat( buf, " no_tell" );
    if ( vector & COMM_NOCHANNELS )
        strcat( buf, " no_channels" );
    if ( !*buf )
        strcat( buf, " none" );

    return ( char * ) ( buf + 1 );
}

const char             *imm_bit_name( int vector )
{
    static char             buf[512] = "\0\0\0\0\0\0\0";

    *buf = '\0';
    if ( vector & IMM_SUMMON )
        strcat( buf, " summon" );
    if ( vector & IMM_CHARM )
        strcat( buf, " charm" );
    if ( vector & IMM_MAGIC )
        strcat( buf, " magic" );
    if ( vector & IMM_WEAPON )
        strcat( buf, " weapon" );
    if ( vector & IMM_BASH )
        strcat( buf, " blunt" );
    if ( vector & IMM_PIERCE )
        strcat( buf, " piercing" );
    if ( vector & IMM_SLASH )
        strcat( buf, " slashing" );
    if ( vector & IMM_FIRE )
        strcat( buf, " fire" );
    if ( vector & IMM_COLD )
        strcat( buf, " cold" );
    if ( vector & IMM_LIGHTNING )
        strcat( buf, " lightning" );
    if ( vector & IMM_ACID )
        strcat( buf, " acid" );
    if ( vector & IMM_POISON )
        strcat( buf, " poison" );
    if ( vector & IMM_NEGATIVE )
        strcat( buf, " negative" );
    if ( vector & IMM_HOLY )
        strcat( buf, " holy" );
    if ( vector & IMM_ENERGY )
        strcat( buf, " energy" );
    if ( vector & IMM_MENTAL )
        strcat( buf, " mental" );
    if ( vector & IMM_DISEASE )
        strcat( buf, " disease" );
    if ( vector & IMM_DROWNING )
        strcat( buf, " drowning" );
    if ( vector & IMM_LIGHT )
        strcat( buf, " light" );
    if ( vector & VULN_IRON )
        strcat( buf, " iron" );
    if ( vector & VULN_WOOD )
        strcat( buf, " wood" );
    if ( vector & VULN_SILVER )
        strcat( buf, " silver" );
    if ( !*buf )
        strcat( buf, " none" );

    return ( char * ) ( buf + 1 );
}

const char             *wear_bit_name( int vector )
{
    static char             buf[512] = "\0\0\0\0\0\0\0";

    *buf = '\0';
    if ( vector & ITEM_TAKE )
        strcat( buf, " take" );
    if ( vector & ITEM_WEAR_FINGER )
        strcat( buf, " finger" );
    if ( vector & ITEM_WEAR_NECK )
        strcat( buf, " neck" );
    if ( vector & ITEM_WEAR_BODY )
        strcat( buf, " torso" );
    if ( vector & ITEM_WEAR_HEAD )
        strcat( buf, " head" );
    if ( vector & ITEM_WEAR_LEGS )
        strcat( buf, " legs" );
    if ( vector & ITEM_WEAR_FEET )
        strcat( buf, " feet" );
    if ( vector & ITEM_WEAR_HANDS )
        strcat( buf, " hands" );
    if ( vector & ITEM_WEAR_ARMS )
        strcat( buf, " arms" );
    if ( vector & ITEM_WEAR_SHIELD )
        strcat( buf, " shield" );
    if ( vector & ITEM_WEAR_ABOUT )
        strcat( buf, " body" );
    if ( vector & ITEM_WEAR_WAIST )
        strcat( buf, " waist" );
    if ( vector & ITEM_WEAR_WRIST )
        strcat( buf, " wrist" );
    if ( vector & ITEM_WIELD )
        strcat( buf, " wield" );
    if ( vector & ITEM_HOLD )
        strcat( buf, " hold" );
    if ( vector & ITEM_NO_SAC )
        strcat( buf, " nosac" );
    if ( vector & ITEM_WEAR_FLOAT )
        strcat( buf, " float" );
    if ( !*buf )
        strcat( buf, " none" );

    return ( char * ) ( buf + 1 );
}

const char             *form_bit_name( int vector )
{
    static char             buf[512] = "\0\0\0\0\0\0\0";

    *buf = '\0';
    if ( vector & FORM_POISON )
        strcat( buf, " poison" );
    else if ( vector & FORM_EDIBLE )
        strcat( buf, " edible" );
    if ( vector & FORM_MAGICAL )
        strcat( buf, " magical" );
    if ( vector & FORM_INSTANT_DECAY )
        strcat( buf, " instant_rot" );
    if ( vector & FORM_OTHER )
        strcat( buf, " other" );
    if ( vector & FORM_ANIMAL )
        strcat( buf, " animal" );
    if ( vector & FORM_SENTIENT )
        strcat( buf, " sentient" );
    if ( vector & FORM_UNDEAD )
        strcat( buf, " undead" );
    if ( vector & FORM_CONSTRUCT )
        strcat( buf, " construct" );
    if ( vector & FORM_MIST )
        strcat( buf, " mist" );
    if ( vector & FORM_INTANGIBLE )
        strcat( buf, " intangible" );
    if ( vector & FORM_BIPED )
        strcat( buf, " biped" );
    if ( vector & FORM_CENTAUR )
        strcat( buf, " centaur" );
    if ( vector & FORM_INSECT )
        strcat( buf, " insect" );
    if ( vector & FORM_SPIDER )
        strcat( buf, " spider" );
    if ( vector & FORM_CRUSTACEAN )
        strcat( buf, " crustacean" );
    if ( vector & FORM_WORM )
        strcat( buf, " worm" );
    if ( vector & FORM_BLOB )
        strcat( buf, " blob" );
    if ( vector & FORM_MAMMAL )
        strcat( buf, " mammal" );
    if ( vector & FORM_BIRD )
        strcat( buf, " bird" );
    if ( vector & FORM_REPTILE )
        strcat( buf, " reptile" );
    if ( vector & FORM_SNAKE )
        strcat( buf, " snake" );
    if ( vector & FORM_DRAGON )
        strcat( buf, " dragon" );
    if ( vector & FORM_AMPHIBIAN )
        strcat( buf, " amphibian" );
    if ( vector & FORM_FISH )
        strcat( buf, " fish" );
    if ( vector & FORM_COLD_BLOOD )
        strcat( buf, " cold_blooded" );
    if ( !*buf )
        strcat( buf, " none" );

    return ( char * ) ( buf + 1 );
}

const char             *part_bit_name( int vector )
{
    static char             buf[512] = "\0\0\0\0\0\0\0";

    *buf = '\0';
    if ( vector & PART_HEAD )
        strcat( buf, " head" );
    if ( vector & PART_ARMS )
        strcat( buf, " arms" );
    if ( vector & PART_LEGS )
        strcat( buf, " legs" );
    if ( vector & PART_HEART )
        strcat( buf, " heart" );
    if ( vector & PART_BRAINS )
        strcat( buf, " brains" );
    if ( vector & PART_GUTS )
        strcat( buf, " guts" );
    if ( vector & PART_HANDS )
        strcat( buf, " hands" );
    if ( vector & PART_FEET )
        strcat( buf, " feet" );
    if ( vector & PART_FINGERS )
        strcat( buf, " fingers" );
    if ( vector & PART_EAR )
        strcat( buf, " ears" );
    if ( vector & PART_EYE )
        strcat( buf, " eyes" );
    if ( vector & PART_LONG_TONGUE )
        strcat( buf, " long_tongue" );
    if ( vector & PART_EYESTALKS )
        strcat( buf, " eyestalks" );
    if ( vector & PART_TENTACLES )
        strcat( buf, " tentacles" );
    if ( vector & PART_FINS )
        strcat( buf, " fins" );
    if ( vector & PART_WINGS )
        strcat( buf, " wings" );
    if ( vector & PART_TAIL )
        strcat( buf, " tail" );
    if ( vector & PART_CLAWS )
        strcat( buf, " claws" );
    if ( vector & PART_FANGS )
        strcat( buf, " fangs" );
    if ( vector & PART_HORNS )
        strcat( buf, " horns" );
    if ( vector & PART_SCALES )
        strcat( buf, " scales" );
    if ( !*buf )
        strcat( buf, " none" );

    return ( char * ) ( buf + 1 );
}

const char             *weapon_bit_name( int vector )
{
    static char             buf[512] = "\0\0\0\0\0\0\0";

    *buf = '\0';
    if ( vector & WEAPON_FLAMING )
        strcat( buf, " flaming" );
    if ( vector & WEAPON_FROST )
        strcat( buf, " frost" );
    if ( vector & WEAPON_VAMPIRIC )
        strcat( buf, " vampiric" );
    if ( vector & WEAPON_SHARP )
        strcat( buf, " sharp" );
    if ( vector & WEAPON_VORPAL )
        strcat( buf, " vorpal" );
    if ( vector & WEAPON_TWO_HANDS )
        strcat( buf, " two-handed" );
    if ( vector & WEAPON_SHOCKING )
        strcat( buf, " shocking" );
    if ( vector & WEAPON_POISON )
        strcat( buf, " poison" );
    if ( !*buf )
        strcat( buf, " none" );

    return ( char * ) ( buf + 1 );
}

const char             *cont_bit_name( int vector )
{
    static char             buf[512] = "\0\0\0\0\0\0\0";

    *buf = '\0';
    if ( vector & CONT_CLOSEABLE )
        strcat( buf, " closable" );
    if ( vector & CONT_PICKPROOF )
        strcat( buf, " pickproof" );
    if ( vector & CONT_CLOSED )
        strcat( buf, " closed" );
    if ( vector & CONT_LOCKED )
        strcat( buf, " locked" );
    if ( !*buf )
        strcat( buf, " none" );

    return ( char * ) ( buf + 1 );
}

const char             *off_bit_name( int vector )
{
    static char             buf[512] = "\0\0\0\0\0\0\0";

    *buf = '\0';
    if ( vector & OFF_AREA_ATTACK )
        strcat( buf, " area attack" );
    if ( vector & OFF_BACKSTAB )
        strcat( buf, " backstab" );
    if ( vector & OFF_BASH )
        strcat( buf, " bash" );
    if ( vector & OFF_BERSERK )
        strcat( buf, " berserk" );
    if ( vector & OFF_DISARM )
        strcat( buf, " disarm" );
    if ( vector & OFF_DODGE )
        strcat( buf, " dodge" );
    if ( vector & OFF_FADE )
        strcat( buf, " fade" );
    if ( vector & OFF_FAST )
        strcat( buf, " fast" );
    if ( vector & OFF_KICK )
        strcat( buf, " kick" );
    if ( vector & OFF_KICK_DIRT )
        strcat( buf, " kick_dirt" );
    if ( vector & OFF_PARRY )
        strcat( buf, " parry" );
    if ( vector & OFF_RESCUE )
        strcat( buf, " rescue" );
    if ( vector & OFF_TAIL )
        strcat( buf, " tail" );
    if ( vector & OFF_TRIP )
        strcat( buf, " trip" );
    if ( vector & OFF_CRUSH )
        strcat( buf, " crush" );
    if ( vector & ASSIST_ALL )
        strcat( buf, " assist_all" );
    if ( vector & ASSIST_ALIGN )
        strcat( buf, " assist_align" );
    if ( vector & ASSIST_RACE )
        strcat( buf, " assist_race" );
    if ( vector & ASSIST_PLAYERS )
        strcat( buf, " assist_players" );
    if ( vector & ASSIST_GUARD )
        strcat( buf, " assist_guard" );
    if ( vector & ASSIST_VNUM )
        strcat( buf, " assist_vnum" );
    if ( !*buf )
        strcat( buf, " none" );

    return ( char * ) ( buf + 1 );
}
