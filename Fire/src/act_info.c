/*
 * RAM $Id: act_info.c 67 2009-01-05 00:39:32Z quixadhal $
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
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>

#if !defined(NOCRYPT)
#include "sha256.h"
#endif

#include "merc.h"
#include "strings.h"
#include "random.h"
#include "tables.h"
#include "db.h"
#include "interp.h"
#include "magic.h"
#include "act.h"

const char             *where_name[] = {
    "<used as light>     ",
    "<worn on finger>    ",
    "<worn on finger>    ",
    "<worn around neck>  ",
    "<worn around neck>  ",
    "<worn on torso>     ",
    "<worn on head>      ",
    "<worn on legs>      ",
    "<worn on feet>      ",
    "<worn on hands>     ",
    "<worn on arms>      ",
    "<worn as shield>    ",
    "<worn about body>   ",
    "<worn about waist>  ",
    "<worn around wrist> ",
    "<worn around wrist> ",
    "<wielded>           ",
    "<held>              ",
    "<floating nearby>   ",
};

/* for  keeping track of the player count */
int                     max_on = 0;

char                   *format_obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch, bool fShort )
{
    static char             buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    buf[0] = '\0';                                     /* This is actually needed,
                                                        * because buf is static! */

    if ( ( fShort && ( obj->short_descr == NULL || obj->short_descr[0] == '\0' ) )
         || ( obj->description == NULL || obj->description[0] == '\0' ) )
        return buf;

    if ( IS_OBJ_STAT( obj, ITEM_INVIS ) )
        strcat( buf, "(Invis) " );
    if ( IS_AFFECTED( ch, AFF_DETECT_EVIL ) && IS_OBJ_STAT( obj, ITEM_EVIL ) )
        strcat( buf, "(Red Aura) " );
    if ( IS_AFFECTED( ch, AFF_DETECT_GOOD ) && IS_OBJ_STAT( obj, ITEM_BLESS ) )
        strcat( buf, "(Blue Aura) " );
    if ( IS_AFFECTED( ch, AFF_DETECT_MAGIC ) && IS_OBJ_STAT( obj, ITEM_MAGIC ) )
        strcat( buf, "(Magical) " );
    if ( IS_OBJ_STAT( obj, ITEM_GLOW ) )
        strcat( buf, "(Glowing) " );
    if ( IS_OBJ_STAT( obj, ITEM_HUM ) )
        strcat( buf, "(Humming) " );

    if ( fShort )
    {
        if ( obj->short_descr != NULL )
            strcat( buf, obj->short_descr );
    }
    else
    {
        if ( obj->description != NULL )
            strcat( buf, obj->description );
    }

    return buf;
}

/*
 * Show a list to a character.
 * Can coalesce duplicated items.
 */
void show_list_to_char( OBJ_DATA *list, CHAR_DATA *ch, bool fShort, bool fShowNothing )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    BUFFER                 *output = NULL;
    char                  **prgpstrShow = NULL;
    int                    *prgnShow = NULL;
    char                   *pstrShow = NULL;
    OBJ_DATA               *obj = NULL;
    int                     nShow = 0;
    int                     iShow = 0;
    size_t                  count = 0;
    bool                    fCombine = false;

    if ( ch->desc == NULL )
        return;

    /*
     * Alloc space for output lines.
     */
    output = new_buf(  );

    count = 0;
    for ( obj = list; obj != NULL; obj = obj->next_content )
        count++;
    prgpstrShow = ( char ** ) alloc_mem( count * sizeof( char * ) );
    prgnShow = ( int * ) alloc_mem( count * sizeof( int ) );
    nShow = 0;

    /*
     * Format the list of objects.
     */
    for ( obj = list; obj != NULL; obj = obj->next_content )
    {
        if ( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj ) )
        {
            pstrShow = format_obj_to_char( obj, ch, fShort );

            fCombine = false;

            if ( IS_NPC( ch ) || IS_SET( ch->comm, COMM_COMBINE ) )
            {
                /*
                 * Look for duplicates, case sensitive.
                 * Matches tend to be near end so run loop backwords.
                 */
                for ( iShow = nShow - 1; iShow >= 0; iShow-- )
                {
                    if ( !strcmp( prgpstrShow[iShow], pstrShow ) )
                    {
                        prgnShow[iShow]++;
                        fCombine = true;
                        break;
                    }
                }
            }

            /*
             * Couldn't combine, or didn't want to.
             */
            if ( !fCombine )
            {
                prgpstrShow[nShow] = str_dup( pstrShow );
                prgnShow[nShow] = 1;
                nShow++;
            }
        }
    }

    /*
     * Output the formatted list.
     */
    for ( iShow = 0; iShow < nShow; iShow++ )
    {
        if ( prgpstrShow[iShow][0] == '\0' )
        {
            free_string( prgpstrShow[iShow] );
            continue;
        }

        if ( IS_NPC( ch ) || IS_SET( ch->comm, COMM_COMBINE ) )
        {
            if ( prgnShow[iShow] != 1 )
            {
                sprintf( buf, "(%2d) ", prgnShow[iShow] );
                add_buf( output, buf );
            }
            else
            {
                add_buf( output, "     " );
            }
        }
        add_buf( output, prgpstrShow[iShow] );
        add_buf( output, "\r\n" );
        free_string( prgpstrShow[iShow] );
    }

    if ( fShowNothing && nShow == 0 )
    {
        if ( IS_NPC( ch ) || IS_SET( ch->comm, COMM_COMBINE ) )
            ch_printf( ch, "     " );
        ch_printf( ch, "Nothing.\r\n" );
    }
    page_to_char( buf_string( output ), ch );

    /*
     * Clean up.
     */
    free_buf( output );
    free_mem( prgpstrShow, count * sizeof( char * ) );
    free_mem( prgnShow, count * sizeof( int ) );

    return;
}

void show_char_to_char_0( CHAR_DATA *victim, CHAR_DATA *ch )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    message[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    if ( IS_SET( victim->comm, COMM_AFK ) )
        strcat( buf, "[AFK] " );
    if ( IS_AFFECTED( victim, AFF_INVISIBLE ) )
        strcat( buf, "(Invis) " );
    if ( victim->invis_level >= LEVEL_HERO )
        strcat( buf, "(Wizi) " );
    if ( IS_AFFECTED( victim, AFF_HIDE ) )
        strcat( buf, "(Hide) " );
    if ( IS_AFFECTED( victim, AFF_CHARM ) )
        strcat( buf, "(Charmed) " );
    if ( IS_AFFECTED( victim, AFF_PASS_DOOR ) )
        strcat( buf, "(Translucent) " );
    if ( IS_AFFECTED( victim, AFF_FAERIE_FIRE ) )
        strcat( buf, "(Pink Aura) " );
    if ( IS_EVIL( victim ) && IS_AFFECTED( ch, AFF_DETECT_EVIL ) )
        strcat( buf, "(Red Aura) " );
    if ( IS_GOOD( victim ) && IS_AFFECTED( ch, AFF_DETECT_GOOD ) )
        strcat( buf, "(Golden Aura) " );
    if ( IS_AFFECTED( victim, AFF_SANCTUARY ) )
        strcat( buf, "(White Aura) " );
    if ( !IS_NPC( victim ) && IS_SET( victim->act, PLR_KILLER ) )
        strcat( buf, "(KILLER) " );
    if ( !IS_NPC( victim ) && IS_SET( victim->act, PLR_THIEF ) )
        strcat( buf, "(THIEF) " );
    if ( victim->position == victim->start_pos && victim->long_descr[0] != '\0' )
    {
        strcat( buf, victim->long_descr );
        send_to_char( buf, ch );
        return;
    }

    strcat( buf, PERS( victim, ch ) );
    if ( !IS_NPC( victim ) && !IS_SET( ch->comm, COMM_BRIEF )
         && victim->position == POS_STANDING && ch->on == NULL )
        strcat( buf, victim->pcdata->title );

    switch ( victim->position )
    {
        case POS_DEAD:
            strcat( buf, " is DEAD!!" );
            break;
        case POS_MORTAL:
            strcat( buf, " is mortally wounded." );
            break;
        case POS_INCAP:
            strcat( buf, " is incapacitated." );
            break;
        case POS_STUNNED:
            strcat( buf, " is lying here stunned." );
            break;
        case POS_SLEEPING:
            if ( victim->on != NULL )
            {
                if ( IS_SET( victim->on->value[2], SLEEP_AT ) )
                {
                    sprintf( message, " is sleeping at %s.", victim->on->short_descr );
                    strcat( buf, message );
                }
                else if ( IS_SET( victim->on->value[2], SLEEP_ON ) )
                {
                    sprintf( message, " is sleeping on %s.", victim->on->short_descr );
                    strcat( buf, message );
                }
                else
                {
                    sprintf( message, " is sleeping in %s.", victim->on->short_descr );
                    strcat( buf, message );
                }
            }
            else
                strcat( buf, " is sleeping here." );
            break;
        case POS_RESTING:
            if ( victim->on != NULL )
            {
                if ( IS_SET( victim->on->value[2], REST_AT ) )
                {
                    sprintf( message, " is resting at %s.", victim->on->short_descr );
                    strcat( buf, message );
                }
                else if ( IS_SET( victim->on->value[2], REST_ON ) )
                {
                    sprintf( message, " is resting on %s.", victim->on->short_descr );
                    strcat( buf, message );
                }
                else
                {
                    sprintf( message, " is resting in %s.", victim->on->short_descr );
                    strcat( buf, message );
                }
            }
            else
                strcat( buf, " is resting here." );
            break;
        case POS_SITTING:
            if ( victim->on != NULL )
            {
                if ( IS_SET( victim->on->value[2], SIT_AT ) )
                {
                    sprintf( message, " is sitting at %s.", victim->on->short_descr );
                    strcat( buf, message );
                }
                else if ( IS_SET( victim->on->value[2], SIT_ON ) )
                {
                    sprintf( message, " is sitting on %s.", victim->on->short_descr );
                    strcat( buf, message );
                }
                else
                {
                    sprintf( message, " is sitting in %s.", victim->on->short_descr );
                    strcat( buf, message );
                }
            }
            else
                strcat( buf, " is sitting here." );
            break;
        case POS_STANDING:
            if ( victim->on != NULL )
            {
                if ( IS_SET( victim->on->value[2], STAND_AT ) )
                {
                    sprintf( message, " is standing at %s.", victim->on->short_descr );
                    strcat( buf, message );
                }
                else if ( IS_SET( victim->on->value[2], STAND_ON ) )
                {
                    sprintf( message, " is standing on %s.", victim->on->short_descr );
                    strcat( buf, message );
                }
                else
                {
                    sprintf( message, " is standing in %s.", victim->on->short_descr );
                    strcat( buf, message );
                }
            }
            else
                strcat( buf, " is here." );
            break;
        case POS_FIGHTING:
            strcat( buf, " is here, fighting " );
            if ( victim->fighting == NULL )
                strcat( buf, "thin air??" );
            else if ( victim->fighting == ch )
                strcat( buf, "YOU!" );
            else if ( victim->in_room == victim->fighting->in_room )
            {
                strcat( buf, PERS( victim->fighting, ch ) );
                strcat( buf, "." );
            }
            else
                strcat( buf, "someone who left??" );
            break;
    }

    strcat( buf, "\r\n" );
    buf[0] = UPPER( buf[0] );
    send_to_char( buf, ch );
    return;
}

void show_char_to_char_1( CHAR_DATA *victim, CHAR_DATA *ch )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;
    int                     iWear = 0;
    int                     percent = 0;
    bool                    found = false;
    int                     sn = -1;

    if ( can_see( victim, ch ) )
    {
        if ( ch == victim )
            act( "$n looks at $mself.", ch, NULL, NULL, TO_ROOM );
        else
        {
            act( "$n looks at you.", ch, NULL, victim, TO_VICT );
            act( "$n looks at $N.", ch, NULL, victim, TO_NOTVICT );
        }
    }

    if ( victim->description[0] != '\0' )
    {
        ch_printf( ch, "%s", victim->description );
    }
    else
    {
        act( "You see nothing special about $M.", ch, NULL, victim, TO_CHAR );
    }

    if ( victim->max_hit > 0 )
        percent = ( 100 * victim->hit ) / victim->max_hit;
    else
        percent = -1;

    strcpy( buf, PERS( victim, ch ) );

    if ( percent >= 100 )
        strcat( buf, " is in excellent condition.\r\n" );
    else if ( percent >= 90 )
        strcat( buf, " has a few scratches.\r\n" );
    else if ( percent >= 75 )
        strcat( buf, " has some small wounds and bruises.\r\n" );
    else if ( percent >= 50 )
        strcat( buf, " has quite a few wounds.\r\n" );
    else if ( percent >= 30 )
        strcat( buf, " has some big nasty wounds and scratches.\r\n" );
    else if ( percent >= 15 )
        strcat( buf, " looks pretty hurt.\r\n" );
    else if ( percent >= 0 )
        strcat( buf, " is in awful condition.\r\n" );
    else
        strcat( buf, " is bleeding to death.\r\n" );

    buf[0] = UPPER( buf[0] );
    send_to_char( buf, ch );

    found = false;
    for ( iWear = 0; iWear < MAX_WEAR; iWear++ )
    {
        if ( ( obj = get_eq_char( victim, iWear ) ) != NULL && can_see_obj( ch, obj ) )
        {
            if ( !found )
            {
                ch_printf( ch, "\r\n" );
                act( "$N is using:", ch, NULL, victim, TO_CHAR );
                found = true;
            }
            ch_printf( ch, "%s%s\r\n", where_name[iWear],
                       format_obj_to_char( obj, ch, true ) );
        }
    }

    if ( ( sn = skill_lookup( "peek" ) ) == -1 )
    {
        log_error( "Can't find the \"%s\" skill in %s?", "peek", __FUNCTION__ );
        return;
    }

    if ( victim != ch && !IS_NPC( ch ) && number_percent(  ) < get_skill( ch, sn ) )
    {
        ch_printf( ch, "\r\nYou peek at the inventory:\r\n" );
        check_improve( ch, sn, true, 4 );
        show_list_to_char( victim->carrying, ch, true, true );
    }

    return;
}

void show_char_to_char( CHAR_DATA *list, CHAR_DATA *ch )
{
    CHAR_DATA              *rch = NULL;

    for ( rch = list; rch != NULL; rch = rch->next_in_room )
    {
        if ( rch == ch )
            continue;

        if ( get_trust( ch ) < rch->invis_level )
            continue;

        if ( can_see( ch, rch ) )
        {
            show_char_to_char_0( rch, ch );
        }
        else if ( room_is_dark( ch->in_room ) && IS_AFFECTED( rch, AFF_INFRARED ) )
        {
            ch_printf( ch, "You see glowing red eyes watching YOU!\r\n" );
        }
    }

    return;
}

bool check_blind( CHAR_DATA *ch )
{
    if ( !IS_NPC( ch ) && IS_SET( ch->act, PLR_HOLYLIGHT ) )
        return true;

    if ( IS_AFFECTED( ch, AFF_BLIND ) )
    {
        ch_printf( ch, "You can't see a thing!\r\n" );
        return false;
    }

    return true;
}

/* changes your scroll */
void do_scroll( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    int                     lines = 0;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        if ( ch->lines == 0 )
            ch_printf( ch, "You do not page long messages.\r\n" );
        else
        {
            ch_printf( ch, "You currently display %d lines per page.\r\n",
                       ch->lines + 2 );
        }
        return;
    }

    if ( !is_number( arg ) )
    {
        ch_printf( ch, "You must provide a number.\r\n" );
        return;
    }

    lines = atoi( arg );

    if ( lines == 0 )
    {
        ch_printf( ch, "Paging disabled.\r\n" );
        ch->lines = 0;
        return;
    }

    if ( lines < 10 || lines > 100 )
    {
        ch_printf( ch, "You must provide a reasonable number.\r\n" );
        return;
    }

    ch_printf( ch, "Scroll set to %d lines.\r\n", lines );
    ch->lines = lines - 2;
}

/* RT does socials */
void do_socials( CHAR_DATA *ch, const char *argument )
{
    int                     iSocial = 0;
    int                     col = 0;

    for ( iSocial = 0; social_table[iSocial].name[0] != '\0'; iSocial++ )
    {
        ch_printf( ch, "%-12s", social_table[iSocial].name );
        if ( ++col % 6 == 0 )
            ch_printf( ch, "\r\n" );
    }

    if ( col % 6 != 0 )
        ch_printf( ch, "\r\n" );
    return;
}

/* RT Commands to replace news, motd, imotd, etc from ROM */
void do_motd( CHAR_DATA *ch, const char *argument )
{
    do_function( ch, &do_help, "motd" );
}

void do_imotd( CHAR_DATA *ch, const char *argument )
{
    do_function( ch, &do_help, "imotd" );
}

void do_rules( CHAR_DATA *ch, const char *argument )
{
    do_function( ch, &do_help, "rules" );
}

void do_story( CHAR_DATA *ch, const char *argument )
{
    do_function( ch, &do_help, "story" );
}

void do_wizlist( CHAR_DATA *ch, const char *argument )
{
    do_function( ch, &do_help, "wizlist" );
}

/* RT this following section holds all the auto commands from ROM, as well as
   replacements for config */

void do_autolist( CHAR_DATA *ch, const char *argument )
{
    /*
     * lists most player flags 
     */
    if ( IS_NPC( ch ) )
        return;

    ch_printf( ch, "   action     status\r\n" );
    ch_printf( ch, "---------------------\r\n" );

    ch_printf( ch, "autoassist     " );
    if ( IS_SET( ch->act, PLR_AUTOASSIST ) )
        ch_printf( ch, "ON\r\n" );
    else
        ch_printf( ch, "OFF\r\n" );

    ch_printf( ch, "autoexit       " );
    if ( IS_SET( ch->act, PLR_AUTOEXIT ) )
        ch_printf( ch, "ON\r\n" );
    else
        ch_printf( ch, "OFF\r\n" );

    ch_printf( ch, "autogold       " );
    if ( IS_SET( ch->act, PLR_AUTOGOLD ) )
        ch_printf( ch, "ON\r\n" );
    else
        ch_printf( ch, "OFF\r\n" );

    ch_printf( ch, "autoloot       " );
    if ( IS_SET( ch->act, PLR_AUTOLOOT ) )
        ch_printf( ch, "ON\r\n" );
    else
        ch_printf( ch, "OFF\r\n" );

    ch_printf( ch, "autosac        " );
    if ( IS_SET( ch->act, PLR_AUTOSAC ) )
        ch_printf( ch, "ON\r\n" );
    else
        ch_printf( ch, "OFF\r\n" );

    ch_printf( ch, "autosplit      " );
    if ( IS_SET( ch->act, PLR_AUTOSPLIT ) )
        ch_printf( ch, "ON\r\n" );
    else
        ch_printf( ch, "OFF\r\n" );

    ch_printf( ch, "compact mode   " );
    if ( IS_SET( ch->comm, COMM_COMPACT ) )
        ch_printf( ch, "ON\r\n" );
    else
        ch_printf( ch, "OFF\r\n" );

    ch_printf( ch, "prompt         " );
    if ( IS_SET( ch->comm, COMM_PROMPT ) )
        ch_printf( ch, "ON\r\n" );
    else
        ch_printf( ch, "OFF\r\n" );

    ch_printf( ch, "combine items  " );
    if ( IS_SET( ch->comm, COMM_COMBINE ) )
        ch_printf( ch, "ON\r\n" );
    else
        ch_printf( ch, "OFF\r\n" );

    if ( !IS_SET( ch->act, PLR_CANLOOT ) )
        ch_printf( ch, "Your corpse is safe from thieves.\r\n" );
    else
        ch_printf( ch, "Your corpse may be looted.\r\n" );

    if ( IS_SET( ch->act, PLR_NOSUMMON ) )
        ch_printf( ch, "You cannot be summoned.\r\n" );
    else
        ch_printf( ch, "You can be summoned.\r\n" );

    if ( IS_SET( ch->act, PLR_NOFOLLOW ) )
        ch_printf( ch, "You do not welcome followers.\r\n" );
    else
        ch_printf( ch, "You accept followers.\r\n" );
}

void do_autoassist( CHAR_DATA *ch, const char *argument )
{
    if ( IS_NPC( ch ) )
        return;

    if ( IS_SET( ch->act, PLR_AUTOASSIST ) )
    {
        ch_printf( ch, "Autoassist removed.\r\n" );
        REMOVE_BIT( ch->act, PLR_AUTOASSIST );
    }
    else
    {
        ch_printf( ch, "You will now assist when needed.\r\n" );
        SET_BIT( ch->act, PLR_AUTOASSIST );
    }
}

void do_autoexit( CHAR_DATA *ch, const char *argument )
{
    if ( IS_NPC( ch ) )
        return;

    if ( IS_SET( ch->act, PLR_AUTOEXIT ) )
    {
        ch_printf( ch, "Exits will no longer be displayed.\r\n" );
        REMOVE_BIT( ch->act, PLR_AUTOEXIT );
    }
    else
    {
        ch_printf( ch, "Exits will now be displayed.\r\n" );
        SET_BIT( ch->act, PLR_AUTOEXIT );
    }
}

void do_autogold( CHAR_DATA *ch, const char *argument )
{
    if ( IS_NPC( ch ) )
        return;

    if ( IS_SET( ch->act, PLR_AUTOGOLD ) )
    {
        ch_printf( ch, "Autogold removed.\r\n" );
        REMOVE_BIT( ch->act, PLR_AUTOGOLD );
    }
    else
    {
        ch_printf( ch, "Automatic gold looting set.\r\n" );
        SET_BIT( ch->act, PLR_AUTOGOLD );
    }
}

void do_autoloot( CHAR_DATA *ch, const char *argument )
{
    if ( IS_NPC( ch ) )
        return;

    if ( IS_SET( ch->act, PLR_AUTOLOOT ) )
    {
        ch_printf( ch, "Autolooting removed.\r\n" );
        REMOVE_BIT( ch->act, PLR_AUTOLOOT );
    }
    else
    {
        ch_printf( ch, "Automatic corpse looting set.\r\n" );
        SET_BIT( ch->act, PLR_AUTOLOOT );
    }
}

void do_autosac( CHAR_DATA *ch, const char *argument )
{
    if ( IS_NPC( ch ) )
        return;

    if ( IS_SET( ch->act, PLR_AUTOSAC ) )
    {
        ch_printf( ch, "Autosacrificing removed.\r\n" );
        REMOVE_BIT( ch->act, PLR_AUTOSAC );
    }
    else
    {
        ch_printf( ch, "Automatic corpse sacrificing set.\r\n" );
        SET_BIT( ch->act, PLR_AUTOSAC );
    }
}

void do_autosplit( CHAR_DATA *ch, const char *argument )
{
    if ( IS_NPC( ch ) )
        return;

    if ( IS_SET( ch->act, PLR_AUTOSPLIT ) )
    {
        ch_printf( ch, "Autosplitting removed.\r\n" );
        REMOVE_BIT( ch->act, PLR_AUTOSPLIT );
    }
    else
    {
        ch_printf( ch, "Automatic gold splitting set.\r\n" );
        SET_BIT( ch->act, PLR_AUTOSPLIT );
    }
}

void do_brief( CHAR_DATA *ch, const char *argument )
{
    if ( IS_SET( ch->comm, COMM_BRIEF ) )
    {
        ch_printf( ch, "Full descriptions activated.\r\n" );
        REMOVE_BIT( ch->comm, COMM_BRIEF );
    }
    else
    {
        ch_printf( ch, "Short descriptions activated.\r\n" );
        SET_BIT( ch->comm, COMM_BRIEF );
    }
}

void do_compact( CHAR_DATA *ch, const char *argument )
{
    if ( IS_SET( ch->comm, COMM_COMPACT ) )
    {
        ch_printf( ch, "Compact mode removed.\r\n" );
        REMOVE_BIT( ch->comm, COMM_COMPACT );
    }
    else
    {
        ch_printf( ch, "Compact mode set.\r\n" );
        SET_BIT( ch->comm, COMM_COMPACT );
    }
}

void do_show( CHAR_DATA *ch, const char *argument )
{
    if ( IS_SET( ch->comm, COMM_SHOW_AFFECTS ) )
    {
        ch_printf( ch, "Affects will no longer be shown in score.\r\n" );
        REMOVE_BIT( ch->comm, COMM_SHOW_AFFECTS );
    }
    else
    {
        ch_printf( ch, "Affects will now be shown in score.\r\n" );
        SET_BIT( ch->comm, COMM_SHOW_AFFECTS );
    }
}

void do_prompt( CHAR_DATA *ch, const char *argument )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    if ( argument[0] == '\0' )
    {
        if ( IS_SET( ch->comm, COMM_PROMPT ) )
        {
            ch_printf( ch, "You will no longer see prompts.\r\n" );
            REMOVE_BIT( ch->comm, COMM_PROMPT );
        }
        else
        {
            ch_printf( ch, "You will now see prompts.\r\n" );
            SET_BIT( ch->comm, COMM_PROMPT );
        }
        return;
    }

    if ( !strcmp( argument, "all" ) )
        strcpy( buf, "<%hhp %mm %vmv> " );
    else
    {
        strncpy( buf, argument, 50 );
        buf[50] = '\0';
        smash_tilde( buf );
        if ( str_suffix( "%c", buf ) )
            strcat( buf, " " );

    }

    free_string( ch->prompt );
    ch->prompt = str_dup( buf );
    ch_printf( ch, "Prompt set to %s\r\n", ch->prompt );
    return;
}

void do_combine( CHAR_DATA *ch, const char *argument )
{
    if ( IS_SET( ch->comm, COMM_COMBINE ) )
    {
        ch_printf( ch, "Long inventory selected.\r\n" );
        REMOVE_BIT( ch->comm, COMM_COMBINE );
    }
    else
    {
        ch_printf( ch, "Combined inventory selected.\r\n" );
        SET_BIT( ch->comm, COMM_COMBINE );
    }
}

void do_noloot( CHAR_DATA *ch, const char *argument )
{
    if ( IS_NPC( ch ) )
        return;

    if ( IS_SET( ch->act, PLR_CANLOOT ) )
    {
        ch_printf( ch, "Your corpse is now safe from thieves.\r\n" );
        REMOVE_BIT( ch->act, PLR_CANLOOT );
    }
    else
    {
        ch_printf( ch, "Your corpse may now be looted.\r\n" );
        SET_BIT( ch->act, PLR_CANLOOT );
    }
}

void do_nofollow( CHAR_DATA *ch, const char *argument )
{
    if ( IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
        return;

    if ( IS_SET( ch->act, PLR_NOFOLLOW ) )
    {
        ch_printf( ch, "You now accept followers.\r\n" );
        REMOVE_BIT( ch->act, PLR_NOFOLLOW );
    }
    else
    {
        ch_printf( ch, "You no longer accept followers.\r\n" );
        SET_BIT( ch->act, PLR_NOFOLLOW );
        die_follower( ch );
    }
}

void do_nosummon( CHAR_DATA *ch, const char *argument )
{
    if ( IS_NPC( ch ) )
    {
        if ( IS_SET( ch->imm_flags, IMM_SUMMON ) )
        {
            ch_printf( ch, "You are no longer immune to summon.\r\n" );
            REMOVE_BIT( ch->imm_flags, IMM_SUMMON );
        }
        else
        {
            ch_printf( ch, "You are now immune to summoning.\r\n" );
            SET_BIT( ch->imm_flags, IMM_SUMMON );
        }
    }
    else
    {
        if ( IS_SET( ch->act, PLR_NOSUMMON ) )
        {
            ch_printf( ch, "You are no longer immune to summon.\r\n" );
            REMOVE_BIT( ch->act, PLR_NOSUMMON );
        }
        else
        {
            ch_printf( ch, "You are now immune to summoning.\r\n" );
            SET_BIT( ch->act, PLR_NOSUMMON );
        }
    }
}

void do_look( CHAR_DATA *ch, const char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg3[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    EXIT_DATA              *pexit = NULL;
    CHAR_DATA              *victim = NULL;
    OBJ_DATA               *obj = NULL;
    char                   *pdesc = NULL;
    int                     door = 0;
    int                     number = 0;
    int                     count = 0;

    if ( ch->desc == NULL )
        return;

    if ( ch->position < POS_SLEEPING )
    {
        ch_printf( ch, "You can't see anything but stars!\r\n" );
        return;
    }

    if ( ch->position == POS_SLEEPING )
    {
        ch_printf( ch, "You can't see anything, you're sleeping!\r\n" );
        return;
    }

    if ( !check_blind( ch ) )
        return;

    if ( !IS_NPC( ch )
         && !IS_SET( ch->act, PLR_HOLYLIGHT ) && room_is_dark( ch->in_room ) )
    {
        ch_printf( ch, "It is pitch black ... \r\n" );
        show_char_to_char( ch->in_room->people, ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    number = number_argument( arg1, arg3 );
    count = 0;

    if ( arg1[0] == '\0' || !str_cmp( arg1, "auto" ) )
    {
        /*
         * 'look' or 'look auto' 
         */
        ch_printf( ch, "%s", ch->in_room->name );

        if ( ( IS_IMMORTAL( ch ) || IS_BUILDER( ch, ch->in_room->area ) )
             && ( IS_NPC( ch ) || IS_SET( ch->act, PLR_HOLYLIGHT ) ) )
        {
            ch_printf( ch, " [Room %d]", ch->in_room->vnum );
        }

        ch_printf( ch, "\r\n" );

        if ( arg1[0] == '\0' || ( !IS_NPC( ch ) && !IS_SET( ch->comm, COMM_BRIEF ) ) )
        {
            ch_printf( ch, " %s", ch->in_room->description );
        }

        if ( !IS_NPC( ch ) && IS_SET( ch->act, PLR_AUTOEXIT ) )
        {
            ch_printf( ch, "\r\n" );
            do_function( ch, &do_exits, "auto" );
        }

        show_list_to_char( ch->in_room->contents, ch, false, false );
        show_char_to_char( ch->in_room->people, ch );
        return;
    }

    if ( !str_cmp( arg1, "i" ) || !str_cmp( arg1, "in" ) || !str_cmp( arg1, "on" ) )
    {
        /*
         * 'look in' 
         */
        if ( arg2[0] == '\0' )
        {
            ch_printf( ch, "Look in what?\r\n" );
            return;
        }

        if ( ( obj = get_obj_here( ch, arg2 ) ) == NULL )
        {
            ch_printf( ch, "You do not see that here.\r\n" );
            return;
        }

        switch ( obj->item_type )
        {
            default:
                ch_printf( ch, "That is not a container.\r\n" );
                break;

            case ITEM_DRINK_CON:
                if ( obj->value[1] <= 0 )
                {
                    ch_printf( ch, "It is empty.\r\n" );
                    break;
                }

                ch_printf( ch, "It's %sfilled with  a %s liquid.\r\n",
                           obj->value[1] < obj->value[0] / 4
                           ? "less than half-" :
                           obj->value[1] < 3 * obj->value[0] / 4
                           ? "about half-" : "more than half-",
                           liq_table[obj->value[2]].liq_color );
                break;

            case ITEM_CONTAINER:
            case ITEM_CORPSE_NPC:
            case ITEM_CORPSE_PC:
                if ( IS_SET( obj->value[1], CONT_CLOSED ) )
                {
                    ch_printf( ch, "It is closed.\r\n" );
                    break;
                }

                act( "$p holds:", ch, obj, NULL, TO_CHAR );
                show_list_to_char( obj->contains, ch, true, true );
                break;
        }
        return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) != NULL )
    {
        show_char_to_char_1( victim, ch );
        return;
    }

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
        if ( can_see_obj( ch, obj ) )
        {                                              /* player can see object */
            pdesc = get_extra_descr( arg3, obj->extra_descr );
            if ( pdesc != NULL )
            {
                if ( ++count == number )
                {
                    send_to_char( pdesc, ch );
                    return;
                }
                else
                    continue;
            }
            pdesc = get_extra_descr( arg3, obj->pIndexData->extra_descr );
            if ( pdesc != NULL )
            {
                if ( ++count == number )
                {
                    send_to_char( pdesc, ch );
                    return;
                }
                else
                    continue;
            }
            if ( is_name( arg3, obj->name ) )
                if ( ++count == number )
                {
                    ch_printf( ch, "%s\r\n", obj->description );
                    return;
                }
        }
    }

    for ( obj = ch->in_room->contents; obj != NULL; obj = obj->next_content )
    {
        if ( can_see_obj( ch, obj ) )
        {
            pdesc = get_extra_descr( arg3, obj->extra_descr );
            if ( pdesc != NULL )
                if ( ++count == number )
                {
                    send_to_char( pdesc, ch );
                    return;
                }

            pdesc = get_extra_descr( arg3, obj->pIndexData->extra_descr );
            if ( pdesc != NULL )
                if ( ++count == number )
                {
                    send_to_char( pdesc, ch );
                    return;
                }

            if ( is_name( arg3, obj->name ) )
                if ( ++count == number )
                {
                    ch_printf( ch, "%s\r\n", obj->description );
                    return;
                }
        }
    }

    pdesc = get_extra_descr( arg3, ch->in_room->extra_descr );
    if ( pdesc != NULL )
    {
        if ( ++count == number )
        {
            send_to_char( pdesc, ch );
            return;
        }
    }

    if ( count > 0 && count != number )
    {
        if ( count == 1 )
            ch_printf( ch, "You only see one %s here.\r\n", arg3 );
        else
            ch_printf( ch, "You only see %d of those here.\r\n", count );
        return;
    }

    if ( !str_cmp( arg1, "n" ) || !str_cmp( arg1, "north" ) )
        door = 0;
    else if ( !str_cmp( arg1, "e" ) || !str_cmp( arg1, "east" ) )
        door = 1;
    else if ( !str_cmp( arg1, "s" ) || !str_cmp( arg1, "south" ) )
        door = 2;
    else if ( !str_cmp( arg1, "w" ) || !str_cmp( arg1, "west" ) )
        door = 3;
    else if ( !str_cmp( arg1, "u" ) || !str_cmp( arg1, "up" ) )
        door = 4;
    else if ( !str_cmp( arg1, "d" ) || !str_cmp( arg1, "down" ) )
        door = 5;
    else
    {
        ch_printf( ch, "You do not see that here.\r\n" );
        return;
    }

    /*
     * 'look direction' 
     */
    if ( ( pexit = ch->in_room->exit[door] ) == NULL )
    {
        ch_printf( ch, "Nothing special there.\r\n" );
        return;
    }

    if ( pexit->description != NULL && pexit->description[0] != '\0' )
        ch_printf( ch, "%s", pexit->description );
    else
        ch_printf( ch, "Nothing special there.\r\n" );

    if ( pexit->keyword != NULL && pexit->keyword[0] != '\0' && pexit->keyword[0] != ' ' )
    {
        if ( IS_SET( pexit->exit_info, EX_CLOSED ) )
        {
            act( "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
        }
        else if ( IS_SET( pexit->exit_info, EX_ISDOOR ) )
        {
            act( "The $d is open.", ch, NULL, pexit->keyword, TO_CHAR );
        }
    }

    return;
}

/* RT added back for the hell of it */
void do_read( CHAR_DATA *ch, const char *argument )
{
    do_function( ch, &do_look, argument );
}

void do_examine( CHAR_DATA *ch, const char *argument )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Examine what?\r\n" );
        return;
    }

    do_function( ch, &do_look, arg );

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
        switch ( obj->item_type )
        {
            default:
                break;

            case ITEM_MONEY:
                if ( obj->value[0] == 0 )
                {
                    if ( obj->value[1] == 0 )
                        sprintf( buf, "Odd...there's no coins in the pile.\r\n" );
                    else if ( obj->value[1] == 1 )
                        sprintf( buf, "Wow. One gold coin.\r\n" );
                    else
                        sprintf( buf, "There are %d gold coins in the pile.\r\n",
                                 obj->value[1] );
                }
                else if ( obj->value[1] == 0 )
                {
                    if ( obj->value[0] == 1 )
                        sprintf( buf, "Wow. One silver coin.\r\n" );
                    else
                        sprintf( buf, "There are %d silver coins in the pile.\r\n",
                                 obj->value[0] );
                }
                else
                    sprintf( buf,
                             "There are %d gold and %d silver coins in the pile.\r\n",
                             obj->value[1], obj->value[0] );
                send_to_char( buf, ch );
                break;

            case ITEM_DRINK_CON:
            case ITEM_CONTAINER:
            case ITEM_CORPSE_NPC:
            case ITEM_CORPSE_PC:
                sprintf( buf, "in %s", argument );
                do_function( ch, &do_look, buf );
        }
    }

    return;
}

/*
 * Thanks to Zrin for auto-exit part.
 */
void do_exits( CHAR_DATA *ch, const char *argument )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    EXIT_DATA              *pexit = NULL;
    bool                    found = false;
    bool                    fAuto = false;
    int                     door = 0;

    fAuto = !str_cmp( argument, "auto" );

    if ( !check_blind( ch ) )
        return;

    if ( fAuto )
        sprintf( buf, "[Exits:" );
    else if ( IS_IMMORTAL( ch ) )
        sprintf( buf, "Obvious exits from room %d:\r\n", ch->in_room->vnum );
    else
        sprintf( buf, "Obvious exits:\r\n" );

    found = false;
    for ( door = 0; door < MAX_EXIT; door++ )
    {
        if ( ( pexit = ch->in_room->exit[door] ) != NULL
             && pexit->u1.to_room != NULL
             && can_see_room( ch, pexit->u1.to_room )
             && !IS_SET( pexit->exit_info, EX_CLOSED ) )
        {
            found = true;
            if ( fAuto )
            {
                strcat( buf, " " );
                strcat( buf, dir_name[door] );
            }
            else
            {
                sprintf( buf + strlen( buf ), "%-5s - %s",
                         capitalize( dir_name[door] ),
                         room_is_dark( pexit->u1.to_room )
                         ? "Too dark to tell" : pexit->u1.to_room->name );
                if ( IS_IMMORTAL( ch ) )
                    sprintf( buf + strlen( buf ),
                             " (room %d)\r\n", pexit->u1.to_room->vnum );
                else
                    sprintf( buf + strlen( buf ), "\r\n" );
            }
        }
    }

    if ( !found )
        strcat( buf, fAuto ? " none" : "None.\r\n" );

    if ( fAuto )
        strcat( buf, "]\r\n" );

    send_to_char( buf, ch );
    return;
}

void do_worth( CHAR_DATA *ch, const char *argument )
{
    if ( IS_NPC( ch ) )
    {
        ch_printf( ch, "You have %d gold and %d silver.\r\n", ch->gold, ch->silver );
        return;
    }

    ch_printf( ch,
               "You have %d gold, %d silver, and %d experience (%d exp to level).\r\n",
               ch->gold, ch->silver, ch->exp,
               ( ch->level + 1 ) * exp_per_level( ch, ch->pcdata->points ) - ch->exp );
    return;
}

void do_score( CHAR_DATA *ch, const char *argument )
{
    int                     i = 0;

    ch_printf( ch,
               "You are %s%s, level %d, %d years old (%d hours).\r\n",
               ch->name,
               IS_NPC( ch ) ? "" : ch->pcdata->title,
               ch->level, get_age( ch ),
               ( ch->played + ( int ) ( current_time - ch->logon ) ) / 3600 );

    if ( get_trust( ch ) != ch->level )
    {
        ch_printf( ch, "You are trusted at level %d.\r\n", get_trust( ch ) );
    }

    ch_printf( ch, "Race: %s  Sex: %s  Class: %s\r\n",
               race_table[ch->race].name,
               ch->sex == 0 ? "sexless" : ch->sex == 1 ? "male" : "female",
               IS_NPC( ch ) ? "mobile" : class_table[ch->iclass].name );
    ch_printf( ch,
               "You have %d/%d hit, %d/%d mana, %d/%d movement.\r\n",
               ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move );
    ch_printf( ch,
               "You have %d practices and %d training sessions.\r\n",
               ch->practice, ch->train );
    ch_printf( ch,
               "You are carrying %d/%d items with weight %d/%d pounds.\r\n",
               ch->carry_number, can_carry_n( ch ),
               get_carry_weight( ch ) / 10, can_carry_w( ch ) / 10 );
    ch_printf( ch,
               "Str: %d(%d)  Int: %d(%d)  Wis: %d(%d)  Dex: %d(%d)  Con: %d(%d)\r\n",
               ch->perm_stat[STAT_STR],
               get_curr_stat( ch, STAT_STR ),
               ch->perm_stat[STAT_INT],
               get_curr_stat( ch, STAT_INT ),
               ch->perm_stat[STAT_WIS],
               get_curr_stat( ch, STAT_WIS ),
               ch->perm_stat[STAT_DEX],
               get_curr_stat( ch, STAT_DEX ),
               ch->perm_stat[STAT_CON], get_curr_stat( ch, STAT_CON ) );
    ch_printf( ch,
               "You have scored %d exp, and have %d gold and %d silver coins.\r\n",
               ch->exp, ch->gold, ch->silver );

    /*
     * RT shows exp to level 
     */
    if ( !IS_NPC( ch ) && ch->level < LEVEL_HERO )
    {
        ch_printf( ch,
                   "You need %d exp to level.\r\n",
                   ( ( ch->level + 1 ) * exp_per_level( ch,
                                                        ch->pcdata->points ) -
                     ch->exp ) );
    }

    ch_printf( ch, "Wimpy set to %d hit points.\r\n", ch->wimpy );

    if ( !IS_NPC( ch ) && ch->pcdata->condition[COND_DRUNK] > 10 )
        ch_printf( ch, "You are drunk.\r\n" );
    if ( !IS_NPC( ch ) && ch->pcdata->condition[COND_THIRST] == 0 )
        ch_printf( ch, "You are thirsty.\r\n" );
    if ( !IS_NPC( ch ) && ch->pcdata->condition[COND_HUNGER] == 0 )
        ch_printf( ch, "You are hungry.\r\n" );

    switch ( ch->position )
    {
        case POS_DEAD:
            ch_printf( ch, "You are DEAD!!\r\n" );
            break;
        case POS_MORTAL:
            ch_printf( ch, "You are mortally wounded.\r\n" );
            break;
        case POS_INCAP:
            ch_printf( ch, "You are incapacitated.\r\n" );
            break;
        case POS_STUNNED:
            ch_printf( ch, "You are stunned.\r\n" );
            break;
        case POS_SLEEPING:
            ch_printf( ch, "You are sleeping.\r\n" );
            break;
        case POS_RESTING:
            ch_printf( ch, "You are resting.\r\n" );
            break;
        case POS_SITTING:
            ch_printf( ch, "You are sitting.\r\n" );
            break;
        case POS_STANDING:
            ch_printf( ch, "You are standing.\r\n" );
            break;
        case POS_FIGHTING:
            ch_printf( ch, "You are fighting.\r\n" );
            break;
    }

    /*
     * print AC values 
     */
    if ( ch->level >= 25 )
    {
        ch_printf( ch, "Armor: pierce: %d  bash: %d  slash: %d  magic: %d\r\n",
                   GET_AC( ch, AC_PIERCE ),
                   GET_AC( ch, AC_BASH ), GET_AC( ch, AC_SLASH ), GET_AC( ch,
                                                                          AC_EXOTIC ) );
    }

    for ( i = 0; i < 4; i++ )
    {
        const char             *temp = NULL;

        switch ( i )
        {
            case ( AC_PIERCE ):
                temp = "piercing";
                break;
            case ( AC_BASH ):
                temp = "bashing";
                break;
            case ( AC_SLASH ):
                temp = "slashing";
                break;
            case ( AC_EXOTIC ):
                temp = "magic";
                break;
            default:
                temp = "error";
                break;
        }

        if ( GET_AC( ch, i ) >= 101 )
            ch_printf( ch, "You are hopelessly vulnerable to %s.\r\n", temp );
        else if ( GET_AC( ch, i ) >= 80 )
            ch_printf( ch, "You are defenseless against %s.\r\n", temp );
        else if ( GET_AC( ch, i ) >= 60 )
            ch_printf( ch, "You are barely protected from %s.\r\n", temp );
        else if ( GET_AC( ch, i ) >= 40 )
            ch_printf( ch, "You are slightly armored against %s.\r\n", temp );
        else if ( GET_AC( ch, i ) >= 20 )
            ch_printf( ch, "You are somewhat armored against %s.\r\n", temp );
        else if ( GET_AC( ch, i ) >= 0 )
            ch_printf( ch, "You are armored against %s.\r\n", temp );
        else if ( GET_AC( ch, i ) >= -20 )
            ch_printf( ch, "You are well-armored against %s.\r\n", temp );
        else if ( GET_AC( ch, i ) >= -40 )
            ch_printf( ch, "You are very well-armored against %s.\r\n", temp );
        else if ( GET_AC( ch, i ) >= -60 )
            ch_printf( ch, "You are heavily armored against %s.\r\n", temp );
        else if ( GET_AC( ch, i ) >= -80 )
            ch_printf( ch, "You are superbly armored against %s.\r\n", temp );
        else if ( GET_AC( ch, i ) >= -100 )
            ch_printf( ch, "You are almost invulnerable to %s.\r\n", temp );
        else
            ch_printf( ch, "You are divinely armored against %s.\r\n", temp );
    }

    /*
     * RT wizinvis and holy light 
     */
    if ( IS_IMMORTAL( ch ) )
    {
        ch_printf( ch, "Holy Light: " );
        if ( IS_SET( ch->act, PLR_HOLYLIGHT ) )
            ch_printf( ch, "on" );
        else
            ch_printf( ch, "off" );

        if ( ch->invis_level )
        {
            ch_printf( ch, "  Invisible: level %d", ch->invis_level );
        }

        if ( ch->incog_level )
        {
            ch_printf( ch, "  Incognito: level %d", ch->incog_level );
        }
        ch_printf( ch, "\r\n" );
    }

    if ( ch->level >= 15 )
    {
        ch_printf( ch, "Hitroll: %d  Damroll: %d.\r\n",
                   GET_HITROLL( ch ), GET_DAMROLL( ch ) );
    }

    if ( ch->level >= 10 )
    {
        ch_printf( ch, "Alignment: %d.  ", ch->alignment );
    }

    ch_printf( ch, "You are " );
    if ( ch->alignment > 900 )
        ch_printf( ch, "angelic.\r\n" );
    else if ( ch->alignment > 700 )
        ch_printf( ch, "saintly.\r\n" );
    else if ( ch->alignment > 350 )
        ch_printf( ch, "good.\r\n" );
    else if ( ch->alignment > 100 )
        ch_printf( ch, "kind.\r\n" );
    else if ( ch->alignment > -100 )
        ch_printf( ch, "neutral.\r\n" );
    else if ( ch->alignment > -350 )
        ch_printf( ch, "mean.\r\n" );
    else if ( ch->alignment > -700 )
        ch_printf( ch, "evil.\r\n" );
    else if ( ch->alignment > -900 )
        ch_printf( ch, "demonic.\r\n" );
    else
        ch_printf( ch, "satanic.\r\n" );

    if ( IS_SET( ch->comm, COMM_SHOW_AFFECTS ) )
        do_function( ch, &do_affects, "" );
}

void do_affects( CHAR_DATA *ch, const char *argument )
{
    AFFECT_DATA            *paf = NULL;
    AFFECT_DATA            *paf_last = NULL;
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    if ( ch->affected != NULL )
    {
        ch_printf( ch, "You are affected by the following spells:\r\n" );
        for ( paf = ch->affected; paf != NULL; paf = paf->next )
        {
            if ( paf_last != NULL && paf->type == paf_last->type )
                if ( ch->level >= 20 )
                    sprintf( buf, "                      " );
                else
                    continue;
            else
                sprintf( buf, "Spell: %-15s", skill_table[paf->type].name );

            send_to_char( buf, ch );

            if ( ch->level >= 20 )
            {
                ch_printf( ch,
                           ": modifies %s by %d ",
                           affect_loc_name( paf->location ), paf->modifier );
                if ( paf->duration == -1 )
                    ch_printf( ch, "permanently" );
                else
                    ch_printf( ch, "for %d hours", paf->duration );
            }

            ch_printf( ch, "\r\n" );
            paf_last = paf;
        }
    }
    else
        ch_printf( ch, "You are not affected by any spells.\r\n" );

    return;
}

const char             *day_name[] = {
    "the Moon", "the Bull", "Deception", "Thunder", "Freedom",
    "the Great Gods", "the Sun"
};

const char             *month_name[] = {
    "Winter", "the Winter Wolf", "the Frost Giant", "the Old Forces",
    "the Grand Struggle", "the Spring", "Nature", "Futility", "the Dragon",
    "the Sun", "the Heat", "the Battle", "the Dark Shades", "the Shadows",
    "the Long Shadows", "the Ancient Darkness", "the Great Evil"
};

void do_time( CHAR_DATA *ch, const char *argument )
{
    const char             *suf = NULL;
    int                     day = 0;

    day = time_info.day + 1;

    if ( day > 4 && day < 20 )
        suf = "th";
    else if ( day % 10 == 1 )
        suf = "st";
    else if ( day % 10 == 2 )
        suf = "nd";
    else if ( day % 10 == 3 )
        suf = "rd";
    else
        suf = "th";

    ch_printf( ch,
               "It is %d o'clock %s, Day of %s, %d%s the Month of %s.\r\n",
               ( time_info.hour % 12 == 0 ) ? 12 : time_info.hour % 12,
               time_info.hour >= 12 ? "pm" : "am",
               day_name[day % 7], day, suf, month_name[time_info.month] );
    ch_printf( ch, "ROM started up at %s\r\nThe system time is %s.\r\n",
               str_boot_time, ( char * ) ctime( &current_time ) );

    return;
}

void do_weather( CHAR_DATA *ch, const char *argument )
{
    static const char      *sky_look[4] = {
        "cloudless",
        "cloudy",
        "rainy",
        "lit by flashes of lightning"
    };

    if ( !IS_OUTSIDE( ch ) )
    {
        ch_printf( ch, "You can't see the weather indoors.\r\n" );
        return;
    }

    ch_printf( ch, "The sky is %s and %s.\r\n",
               sky_look[weather_info.sky],
               weather_info.change >= 0
               ? "a warm southerly breeze blows" : "a cold northern gust blows" );

    return;
}

void do_help( CHAR_DATA *ch, const char *argument )
{
    HELP_DATA              *pHelp = NULL;
    BUFFER                 *output = NULL;
    bool                    found = false;
    char                    argall[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    argone[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    int                     level = 0;

    output = new_buf(  );

    if ( argument[0] == '\0' )
        argument = "summary";

    /*
     * this parts handles help a b so that it returns help 'a b' 
     */
    argall[0] = '\0';
    while ( argument[0] != '\0' )
    {
        argument = one_argument( argument, argone );
        if ( argall[0] != '\0' )
            strcat( argall, " " );
        strcat( argall, argone );
    }

    for ( pHelp = help_first; pHelp != NULL; pHelp = pHelp->next )
    {
        level = ( pHelp->level < 0 ) ? -1 * pHelp->level - 1 : pHelp->level;

        if ( level > get_trust( ch ) )
            continue;

        if ( is_name( argall, pHelp->keyword ) )
        {
            /*
             * add seperator if found 
             */
            if ( found )
                add_buf( output,
                         "\r\n============================================================\r\n\r\n" );
            if ( pHelp->level >= 0 && str_cmp( argall, "imotd" ) )
            {
                add_buf( output, pHelp->keyword );
                add_buf( output, "\r\n" );
            }

            /*
             * Strip leading '.' to allow initial blanks.
             */
            if ( pHelp->text[0] == '.' )
                add_buf( output, pHelp->text + 1 );
            else
                add_buf( output, pHelp->text );
            found = true;
            /*
             * small hack :) 
             */
            if ( ch->desc != NULL && ch->desc->connected != CON_PLAYING
                 && ch->desc->connected != CON_GEN_GROUPS )
                break;
        }
    }

    if ( !found )
        ch_printf( ch, "No help on that word.\r\n" );
    else
        page_to_char( buf_string( output ), ch );
    free_buf( output );
}

/* whois command */
void do_whois( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    BUFFER                 *output = NULL;
    DESCRIPTOR_DATA        *d = NULL;
    bool                    found = false;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "You must provide a name.\r\n" );
        return;
    }

    output = new_buf(  );

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        CHAR_DATA              *wch = NULL;
        char const             *iclass = NULL;

        if ( d->connected != CON_PLAYING || !can_see( ch, d->character ) )
            continue;

        wch = ( d->original != NULL ) ? d->original : d->character;

        if ( !can_see( ch, wch ) )
            continue;

        if ( !str_prefix( arg, wch->name ) )
        {
            found = true;

            /*
             * work out the printing 
             */
            iclass = class_table[wch->iclass].who_name;
            switch ( wch->level )
            {
                case MAX_LEVEL - 0:
                    iclass = "IMP";
                    break;
                case MAX_LEVEL - 1:
                    iclass = "CRE";
                    break;
                case MAX_LEVEL - 2:
                    iclass = "SUP";
                    break;
                case MAX_LEVEL - 3:
                    iclass = "DEI";
                    break;
                case MAX_LEVEL - 4:
                    iclass = "GOD";
                    break;
                case MAX_LEVEL - 5:
                    iclass = "IMM";
                    break;
                case MAX_LEVEL - 6:
                    iclass = "DEM";
                    break;
                case MAX_LEVEL - 7:
                    iclass = "ANG";
                    break;
                case MAX_LEVEL - 8:
                    iclass = "AVA";
                    break;
            }

            /*
             * a little formatting 
             */
            sprintf( buf, "[%2d %6s %s] %s%s%s%s%s%s%s%s\r\n",
                     wch->level,
                     wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name
                     : "     ",
                     iclass,
                     wch->incog_level >= LEVEL_HERO ? "(Incog) " : "",
                     wch->invis_level >= LEVEL_HERO ? "(Wizi) " : "",
                     clan_table[wch->clan].who_name,
                     IS_SET( wch->comm, COMM_AFK ) ? "[AFK] " : "",
                     IS_SET( wch->act, PLR_KILLER ) ? "(KILLER) " : "",
                     IS_SET( wch->act, PLR_THIEF ) ? "(THIEF) " : "",
                     wch->name, IS_NPC( wch ) ? "" : wch->pcdata->title );
            add_buf( output, buf );
        }
    }

    if ( !found )
    {
        ch_printf( ch, "No one of that name is playing.\r\n" );
        return;
    }

    page_to_char( buf_string( output ), ch );
    free_buf( output );
}

/*
 * New 'who' command originally by Alander of Rivers of Mud.
 */
void do_who( CHAR_DATA *ch, const char *argument )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    buf2[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    BUFFER                 *output = NULL;
    DESCRIPTOR_DATA        *d = NULL;
    int                     iClass = 0;
    int                     iRace = 0;
    int                     iClan = 0;
    int                     iLevelLower = 0;
    int                     iLevelUpper = MAX_LEVEL;
    int                     nNumber = 0;
    int                     nMatch = 0;
    bool                    rgfClass[MAX_CLASS];
    bool                    rgfRace[MAX_PC_RACE];
    bool                    rgfClan[MAX_CLAN];
    bool                    fClassRestrict = false;
    bool                    fClanRestrict = false;
    bool                    fClan = false;
    bool                    fRaceRestrict = false;
    bool                    fImmortalOnly = false;

    /*
     * Set default arguments.
     */
    for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
        rgfClass[iClass] = false;
    for ( iRace = 0; iRace < MAX_PC_RACE; iRace++ )
        rgfRace[iRace] = false;
    for ( iClan = 0; iClan < MAX_CLAN; iClan++ )
        rgfClan[iClan] = false;

    /*
     * Parse arguments.
     */
    nNumber = 0;
    for ( ;; )
    {
        char                    arg[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

        argument = one_argument( argument, arg );
        if ( arg[0] == '\0' )
            break;

        if ( is_number( arg ) )
        {
            switch ( ++nNumber )
            {
                case 1:
                    iLevelLower = atoi( arg );
                    break;
                case 2:
                    iLevelUpper = atoi( arg );
                    break;
                default:
                    ch_printf( ch, "Only two level numbers allowed.\r\n" );
                    return;
            }
        }
        else
        {
            /*
             * Look for classes to turn on.
             */
            if ( !str_prefix( arg, "immortals" ) )
            {
                fImmortalOnly = true;
            }
            else
            {
                iClass = class_lookup( arg );
                if ( iClass == -1 )
                {
                    iRace = race_lookup( arg );

                    if ( iRace == 0 || iRace >= MAX_PC_RACE )
                    {
                        if ( !str_prefix( arg, "clan" ) )
                            fClan = true;
                        else
                        {
                            iClan = clan_lookup( arg );
                            if ( iClan )
                            {
                                fClanRestrict = true;
                                rgfClan[iClan] = true;
                            }
                            else
                            {
                                ch_printf( ch,
                                           "That's not a valid race, class, or clan.\r\n" );
                                return;
                            }
                        }
                    }
                    else
                    {
                        fRaceRestrict = true;
                        rgfRace[iRace] = true;
                    }
                }
                else
                {
                    fClassRestrict = true;
                    rgfClass[iClass] = true;
                }
            }
        }
    }

    /*
     * Now show matching chars.
     */
    nMatch = 0;
    buf[0] = '\0';
    output = new_buf(  );
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        CHAR_DATA              *wch = NULL;
        char const             *iclass = NULL;

        /*
         * Check for match against restrictions.
         * Don't use trust as that exposes trusted mortals.
         */
        if ( d->connected != CON_PLAYING || !can_see( ch, d->character ) )
            continue;

        wch = ( d->original != NULL ) ? d->original : d->character;

        if ( !can_see( ch, wch ) )
            continue;

        if ( wch->level < iLevelLower
             || wch->level > iLevelUpper
             || ( fImmortalOnly && wch->level < LEVEL_IMMORTAL )
             || ( fClassRestrict && !rgfClass[wch->iclass] )
             || ( fRaceRestrict && !rgfRace[wch->race] )
             || ( fClan && !is_clan( wch ) ) || ( fClanRestrict && !rgfClan[wch->clan] ) )
            continue;

        nMatch++;

        /*
         * Figure out what to print for class.
         */
        iclass = class_table[wch->iclass].who_name;
        switch ( wch->level )
        {
            default:
                break;
                {
            case MAX_LEVEL - 0:
                    iclass = "IMP";
                    break;
            case MAX_LEVEL - 1:
                    iclass = "CRE";
                    break;
            case MAX_LEVEL - 2:
                    iclass = "SUP";
                    break;
            case MAX_LEVEL - 3:
                    iclass = "DEI";
                    break;
            case MAX_LEVEL - 4:
                    iclass = "GOD";
                    break;
            case MAX_LEVEL - 5:
                    iclass = "IMM";
                    break;
            case MAX_LEVEL - 6:
                    iclass = "DEM";
                    break;
            case MAX_LEVEL - 7:
                    iclass = "ANG";
                    break;
            case MAX_LEVEL - 8:
                    iclass = "AVA";
                    break;
                }
        }
        /*
         * Format it up.
         */
        sprintf( buf, "[%2d %6s %s] %s%s%s%s%s%s%s%s\r\n",
                 wch->level,
                 wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name
                 : "     ",
                 iclass,
                 wch->incog_level >= LEVEL_HERO ? "(Incog) " : "",
                 wch->invis_level >= LEVEL_HERO ? "(Wizi) " : "",
                 clan_table[wch->clan].who_name,
                 IS_SET( wch->comm, COMM_AFK ) ? "[AFK] " : "",
                 IS_SET( wch->act, PLR_KILLER ) ? "(KILLER) " : "",
                 IS_SET( wch->act, PLR_THIEF ) ? "(THIEF) " : "",
                 wch->name, IS_NPC( wch ) ? "" : wch->pcdata->title );
        add_buf( output, buf );
    }

    sprintf( buf2, "\r\nPlayers found: %d\r\n", nMatch );
    add_buf( output, buf2 );
    page_to_char( buf_string( output ), ch );
    free_buf( output );
    return;
}

void do_count( CHAR_DATA *ch, const char *argument )
{
    int                     count = 0;
    DESCRIPTOR_DATA        *d = NULL;

    for ( d = descriptor_list; d != NULL; d = d->next )
        if ( d->connected == CON_PLAYING && can_see( ch, d->character ) )
            count++;

    max_on = UMAX( count, max_on );

    if ( max_on == count )
        ch_printf( ch, "There are %d characters on, the most so far today.\r\n", count );
    else
        ch_printf( ch, "There are %d characters on, the most on today was %d.\r\n",
                   count, max_on );
}

void do_inventory( CHAR_DATA *ch, const char *argument )
{
    ch_printf( ch, "You are carrying:\r\n" );
    show_list_to_char( ch->carrying, ch, true, true );
    return;
}

void do_equipment( CHAR_DATA *ch, const char *argument )
{
    OBJ_DATA               *obj = NULL;
    int                     iWear = 0;
    bool                    found = false;

    ch_printf( ch, "You are using:\r\n" );
    for ( iWear = 0; iWear < MAX_WEAR; iWear++ )
    {
        if ( ( obj = get_eq_char( ch, iWear ) ) == NULL )
            continue;

        ch_printf( ch, "%s", where_name[iWear] );
        if ( can_see_obj( ch, obj ) )
        {
            ch_printf( ch, "%s\r\n", format_obj_to_char( obj, ch, true ) );
        }
        else
        {
            ch_printf( ch, "something.\r\n" );
        }
        found = true;
    }

    if ( !found )
        ch_printf( ch, "Nothing.\r\n" );

    return;
}

void do_compare( CHAR_DATA *ch, const char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    const char             *msg = NULL;
    OBJ_DATA               *obj1 = NULL;
    OBJ_DATA               *obj2 = NULL;
    int                     value1 = 0;
    int                     value2 = 0;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' )
    {
        ch_printf( ch, "Compare what to what?\r\n" );
        return;
    }

    if ( ( obj1 = get_obj_carry( ch, arg1, ch ) ) == NULL )
    {
        ch_printf( ch, "You do not have that item.\r\n" );
        return;
    }

    if ( arg2[0] == '\0' )
    {
        for ( obj2 = ch->carrying; obj2 != NULL; obj2 = obj2->next_content )
        {
            if ( obj2->wear_loc != WEAR_NONE
                 && can_see_obj( ch, obj2 )
                 && obj1->item_type == obj2->item_type
                 && ( obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE ) != 0 )
                break;
        }

        if ( obj2 == NULL )
        {
            ch_printf( ch, "You aren't wearing anything comparable.\r\n" );
            return;
        }
    }

    else if ( ( obj2 = get_obj_carry( ch, arg2, ch ) ) == NULL )
    {
        ch_printf( ch, "You do not have that item.\r\n" );
        return;
    }

    msg = NULL;
    value1 = 0;
    value2 = 0;

    if ( obj1 == obj2 )
    {
        msg = "You compare $p to itself.  It looks about the same.";
    }
    else if ( obj1->item_type != obj2->item_type )
    {
        msg = "You can't compare $p and $P.";
    }
    else
    {
        switch ( obj1->item_type )
        {
            default:
                msg = "You can't compare $p and $P.";
                break;

            case ITEM_ARMOR:
                value1 = obj1->value[0] + obj1->value[1] + obj1->value[2];
                value2 = obj2->value[0] + obj2->value[1] + obj2->value[2];
                break;

            case ITEM_WEAPON:
                if ( obj1->pIndexData->new_format )
                    value1 = ( 1 + obj1->value[2] ) * obj1->value[1];
                else
                    value1 = obj1->value[1] + obj1->value[2];

                if ( obj2->pIndexData->new_format )
                    value2 = ( 1 + obj2->value[2] ) * obj2->value[1];
                else
                    value2 = obj2->value[1] + obj2->value[2];
                break;
        }
    }

    if ( msg == NULL )
    {
        if ( value1 == value2 )
            msg = "$p and $P look about the same.";
        else if ( value1 > value2 )
            msg = "$p looks better than $P.";
        else
            msg = "$p looks worse than $P.";
    }

    act( msg, ch, obj1, obj2, TO_CHAR );
    return;
}

void do_credits( CHAR_DATA *ch, const char *argument )
{
    do_function( ch, &do_help, "diku" );
    return;
}

void do_where( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;
    DESCRIPTOR_DATA        *d = NULL;
    bool                    found = false;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Players near you:\r\n" );
        found = false;
        for ( d = descriptor_list; d; d = d->next )
        {
            if ( d->connected == CON_PLAYING
                 && ( victim = d->character ) != NULL
                 && !IS_NPC( victim )
                 && victim->in_room != NULL
                 && !IS_SET( victim->in_room->room_flags, ROOM_NOWHERE )
                 && ( is_room_owner( ch, victim->in_room )
                      || !room_is_private( victim->in_room ) )
                 && victim->in_room->area == ch->in_room->area && can_see( ch, victim ) )
            {
                found = true;
                ch_printf( ch, "%-28s %s\r\n", victim->name, victim->in_room->name );
            }
        }
        if ( !found )
            ch_printf( ch, "None\r\n" );
    }
    else
    {
        found = false;
        for ( victim = char_list; victim != NULL; victim = victim->next )
        {
            if ( victim->in_room != NULL
                 && victim->in_room->area == ch->in_room->area
                 && !IS_AFFECTED( victim, AFF_HIDE )
                 && !IS_AFFECTED( victim, AFF_SNEAK )
                 && can_see( ch, victim ) && is_name( arg, victim->name ) )
            {
                found = true;
                ch_printf( ch, "%-28s %s\r\n", PERS( victim, ch ),
                           victim->in_room->name );
                break;
            }
        }
        if ( !found )
            act( "You didn't find any $T.", ch, NULL, arg, TO_CHAR );
    }

    return;
}

void do_consider( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;
    const char             *msg = NULL;
    int                     diff = 0;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Consider killing whom?\r\n" );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        ch_printf( ch, "They're not here.\r\n" );
        return;
    }

    if ( is_safe( ch, victim ) )
    {
        ch_printf( ch, "Don't even think about it.\r\n" );
        return;
    }

    diff = victim->level - ch->level;

    if ( diff <= -10 )
        msg = "You can kill $N naked and weaponless.";
    else if ( diff <= -5 )
        msg = "$N is no match for you.";
    else if ( diff <= -2 )
        msg = "$N looks like an easy kill.";
    else if ( diff <= 1 )
        msg = "The perfect match!";
    else if ( diff <= 4 )
        msg = "$N says 'Do you feel lucky, punk?'.";
    else if ( diff <= 9 )
        msg = "$N laughs at you mercilessly.";
    else
        msg = "Death will thank you for your gift.";

    act( msg, ch, NULL, victim, TO_CHAR );
    return;
}

void set_title( CHAR_DATA *ch, const char *title, ... )
{
    va_list                 arg;
    char                    tmp[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    if ( IS_NPC( ch ) )
    {
        log_error( "Attempting to set title on NPC %s", NAME( ch ) );
        return;
    }

    if ( title == NULL )
        return;

    va_start( arg, title );
    vsnprintf( tmp, MAX_STRING_LENGTH, title, arg );
    va_end( arg );

    if ( tmp[0] != '.' && tmp[0] != ',' && tmp[0] != '!' && tmp[0] != '?' )
    {
        buf[0] = ' ';
        strcpy( buf + 1, tmp );
    }
    else
    {
        strcpy( buf, tmp );
    }

    free_string( ch->pcdata->title );
    ch->pcdata->title = str_dup( buf );
    return;
}

void do_title( CHAR_DATA *ch, const char *argument )
{
    char                    buf[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";

    if ( IS_NPC( ch ) )
        return;

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Change your title to what?\r\n" );
        return;
    }

    strncpy( buf, argument, 45 );
    buf[45] = '\0';

    smash_tilde( buf );
    set_title( ch, "%s", buf );
    ch_printf( ch, "Ok.\r\n" );
}

void do_description( CHAR_DATA *ch, const char *argument )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    tmp[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                   *tp = tmp;

    if ( argument[0] != '\0' )
    {
        strcpy( tp, argument );
        buf[0] = '\0';
        smash_tilde( tp );

        if ( tp[0] == '-' )
        {
            int                     len = 0;
            bool                    found = false;

            if ( ch->description == NULL || ch->description[0] == '\0' )
            {
                ch_printf( ch, "No lines left to remove.\r\n" );
                return;
            }

            strcpy( buf, ch->description );

            for ( len = strlen( buf ); len > 0; len-- )
            {
                if ( buf[len] == '\r' )
                {
                    if ( !found )                      /* back it up */
                    {
                        if ( len > 0 )
                            len--;
                        found = true;
                    }
                    else                               /* found the second one */
                    {
                        buf[len + 1] = '\0';
                        free_string( ch->description );
                        ch->description = str_dup( buf );
                        ch_printf( ch, "Your description is:\r\n%s",
                                   ch->description ? ch->description : "(None).\r\n" );
                        return;
                    }
                }
            }
            buf[0] = '\0';
            free_string( ch->description );
            ch->description = str_dup( buf );
            ch_printf( ch, "Description cleared.\r\n" );
            return;
        }
        if ( tp[0] == '+' )
        {
            if ( ch->description != NULL )
                strcat( buf, ch->description );
            tp++;
            while ( isspace( *tp ) )
                tp++;
        }

        if ( strlen( buf ) >= 1024 )
        {
            ch_printf( ch, "Description too long.\r\n" );
            return;
        }

        strcat( buf, tp );
        strcat( buf, "\r\n" );
        free_string( ch->description );
        ch->description = str_dup( buf );
    }

    ch_printf( ch, "Your description is:\r\n%s",
               ch->description ? ch->description : "(None).\r\n" );
    return;
}

void do_report( CHAR_DATA *ch, const char *argument )
{
    ch_printf( ch,
               "You say 'I have %d/%d hp %d/%d mana %d/%d mv %d xp.'\r\n",
               ch->hit, ch->max_hit,
               ch->mana, ch->max_mana, ch->move, ch->max_move, ch->exp );
    act_printf( "$n says 'I have %d/%d hp %d/%d mana %d/%d mv %d xp.'",
                ch, NULL, NULL, TO_ROOM, POS_RESTING, false,
                ch->hit, ch->max_hit,
                ch->mana, ch->max_mana, ch->move, ch->max_move, ch->exp );
    return;
}

void do_practice( CHAR_DATA *ch, const char *argument )
{
    int                     sn = 0;

    if ( IS_NPC( ch ) )
        return;

    if ( argument[0] == '\0' )
    {
        int                     col = 0;

        col = 0;
        for ( sn = 0; sn < MAX_SKILL; sn++ )
        {
            if ( skill_table[sn].name == NULL )
                break;
            /*
             * skill is not known 
             */
            if ( ch->level < skill_table[sn].skill_level[ch->iclass]
                 || ch->pcdata->learned[sn] < 1 )
                continue;

            ch_printf( ch, "%-18s %3d%%  ",
                       skill_table[sn].name, ch->pcdata->learned[sn] );
            if ( ++col % 3 == 0 )
                ch_printf( ch, "\r\n" );
        }

        if ( col % 3 != 0 )
            ch_printf( ch, "\r\n" );

        ch_printf( ch, "You have %d practice sessions left.\r\n", ch->practice );
    }
    else
    {
        CHAR_DATA              *mob = NULL;
        int                     adept = 0;

        if ( !IS_AWAKE( ch ) )
        {
            ch_printf( ch, "In your dreams, or what?\r\n" );
            return;
        }

        for ( mob = ch->in_room->people; mob != NULL; mob = mob->next_in_room )
        {
            if ( IS_NPC( mob ) && IS_SET( mob->act, ACT_PRACTICE ) )
                break;
        }

        if ( mob == NULL )
        {
            ch_printf( ch, "You can't do that here.\r\n" );
            return;
        }

        if ( ch->practice <= 0 )
        {
            ch_printf( ch, "You have no practice sessions left.\r\n" );
            return;
        }
        /*
         * skill is not known 
         */
        if ( ( sn = find_spell( ch, argument ) ) < 0
             || ( !IS_NPC( ch ) && ( ch->level < skill_table[sn].skill_level[ch->iclass]
                                     || ch->pcdata->learned[sn] < 1
                                     || skill_table[sn].rating[ch->iclass] == 0 ) ) )
        {
            ch_printf( ch, "You can't practice that.\r\n" );
            return;
        }

        adept = IS_NPC( ch ) ? 100 : class_table[ch->iclass].skill_adept;

        if ( ch->pcdata->learned[sn] >= adept )
        {
            ch_printf( ch, "You are already learned at %s.\r\n", skill_table[sn].name );
        }
        else
        {
            ch->practice--;
            ch->pcdata->learned[sn] +=
                int_app[get_curr_stat( ch, STAT_INT )].learn /
                skill_table[sn].rating[ch->iclass];
            if ( ch->pcdata->learned[sn] < adept )
            {
                act( "You practice $T.", ch, NULL, skill_table[sn].name, TO_CHAR );
                act( "$n practices $T.", ch, NULL, skill_table[sn].name, TO_ROOM );
            }
            else
            {
                ch->pcdata->learned[sn] = adept;
                act( "You are now learned at $T.",
                     ch, NULL, skill_table[sn].name, TO_CHAR );
                act( "$n is now learned at $T.",
                     ch, NULL, skill_table[sn].name, TO_ROOM );
            }
        }
    }
    return;
}

/*
 * 'Wimpy' originally by Dionysos.
 */
void do_wimpy( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    int                     wimpy = 0;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
        wimpy = ch->max_hit / 5;
    else
        wimpy = atoi( arg );

    if ( wimpy < 0 )
    {
        ch_printf( ch, "Your courage exceeds your wisdom.\r\n" );
        return;
    }

    if ( wimpy > ch->max_hit / 2 )
    {
        ch_printf( ch, "Such cowardice ill becomes you.\r\n" );
        return;
    }

    ch->wimpy = wimpy;
    ch_printf( ch, "Wimpy set to %d hit points.\r\n", wimpy );
    return;
}

void do_password( CHAR_DATA *ch, const char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                   *pArg = NULL;
    const char             *pwdnew = NULL;
    const char             *p = NULL;
    char                    cEnd = '\0';

    if ( IS_NPC( ch ) )
        return;

    /*
     * Can't use one_argument here because it smashes case.
     * So we just steal all its code.  Bleagh.
     */
    pArg = arg1;
    while ( isspace( *argument ) )
        argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
        cEnd = *argument++;

    while ( *argument != '\0' )
    {
        if ( *argument == cEnd )
        {
            argument++;
            break;
        }
        *pArg++ = *argument++;
    }
    *pArg = '\0';

    pArg = arg2;
    while ( isspace( *argument ) )
        argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
        cEnd = *argument++;

    while ( *argument != '\0' )
    {
        if ( *argument == cEnd )
        {
            argument++;
            break;
        }
        *pArg++ = *argument++;
    }
    *pArg = '\0';

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        ch_printf( ch, "Syntax: password <old> <new>.\r\n" );
        return;
    }

    if ( strcmp( CRYPT( arg1 ), ch->pcdata->pwd ) )
    {
        WAIT_STATE( ch, 40 );
        ch_printf( ch, "Wrong password.  Wait 10 seconds.\r\n" );
        return;
    }

    if ( strlen( arg2 ) < 5 )
    {
        ch_printf( ch, "New password must be at least five characters long.\r\n" );
        return;
    }

    /*
     * No tilde allowed because of player file format.
     */
    pwdnew = CRYPT( arg2 );
    for ( p = pwdnew; *p != '\0'; p++ )
    {
        if ( *p == '~' )
        {
            ch_printf( ch, "New password not acceptable, try again.\r\n" );
            return;
        }
    }

    free_string( ch->pcdata->pwd );
    ch->pcdata->pwd = str_dup( pwdnew );
    save_char_obj( ch );
    ch_printf( ch, "Ok.\r\n" );
    return;
}

const char             *distance[4] = {
    "right here.", "nearby to the %s.", "not far %s.", "off in the distance %s."
};

void do_scan( CHAR_DATA *ch, const char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    buf[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    ROOM_INDEX_DATA        *scan_room = NULL;
    EXIT_DATA              *pExit = NULL;
    int                     door = 0;
    int                     depth = 0;

    argument = one_argument( argument, arg1 );

    if ( arg1[0] == '\0' )
    {
        act( "$n looks all around.", ch, NULL, NULL, TO_ROOM );
        ch_printf( ch, "Looking around you see:\r\n" );
        scan_list( ch->in_room, ch, 0, -1 );

        for ( door = 0; door < MAX_EXIT; door++ )
        {
            if ( ( pExit = ch->in_room->exit[door] ) != NULL )
                scan_list( pExit->u1.to_room, ch, 1, door );
        }
        return;
    }
    else if ( !str_cmp( arg1, "n" ) || !str_cmp( arg1, "north" ) )
        door = 0;
    else if ( !str_cmp( arg1, "e" ) || !str_cmp( arg1, "east" ) )
        door = 1;
    else if ( !str_cmp( arg1, "s" ) || !str_cmp( arg1, "south" ) )
        door = 2;
    else if ( !str_cmp( arg1, "w" ) || !str_cmp( arg1, "west" ) )
        door = 3;
    else if ( !str_cmp( arg1, "u" ) || !str_cmp( arg1, "up" ) )
        door = 4;
    else if ( !str_cmp( arg1, "d" ) || !str_cmp( arg1, "down" ) )
        door = 5;
    else
    {
        ch_printf( ch, "Which way do you want to scan?\r\n" );
        return;
    }

    act( "You peer intently $T.", ch, NULL, dir_name[door], TO_CHAR );
    act( "$n peers intently $T.", ch, NULL, dir_name[door], TO_ROOM );
    sprintf( buf, "Looking %s you see:\r\n", dir_name[door] );

    scan_room = ch->in_room;

    for ( depth = 1; depth < 4; depth++ )
    {
        if ( ( pExit = scan_room->exit[door] ) != NULL )
        {
            scan_room = pExit->u1.to_room;
            scan_list( pExit->u1.to_room, ch, depth, door );
        }
    }
    return;
}

void scan_list( ROOM_INDEX_DATA *scan_room, CHAR_DATA *ch, int depth, int door )
{
    CHAR_DATA              *rch = NULL;

    if ( scan_room == NULL )
        return;

    if ( !ch || !ch->in_room )
        return;

    /*
     * I think we also need to check that the door we're trying
     * to look THROUGH is not closed.
     */
    if ( door != -1 && ch->in_room->exit[door] != NULL
         && IS_SET( ch->in_room->exit[door]->exit_info, EX_CLOSED ) )
        return;

    /*
     * this used to cause a mysterious crash here, finally realized it was
     * 'door' being -1, and rev_dir seems to have a problem with that...
     * only acted up when it was done in a room with "extra" exits - Mull
     */
    if ( door != -1 && scan_room->exit[rev_dir[door]] != NULL
         && scan_room->exit[rev_dir[door]]->u1.to_room == ch->in_room
         && IS_SET( scan_room->exit[rev_dir[door]]->exit_info, EX_CLOSED ) )
        return;

    for ( rch = scan_room->people; rch != NULL; rch = rch->next_in_room )
    {
        if ( rch == ch )
            continue;
        if ( !IS_NPC( rch ) && rch->invis_level > get_trust( ch ) )
            continue;
        if ( can_see( ch, rch ) )
            scan_char( rch, ch, depth, door );
    }
    return;
}

void scan_char( CHAR_DATA *victim, CHAR_DATA *ch, int depth, int door )
{
    ch_printf( ch, "%s, ", PERS( victim, ch ) );
    ch_printf( ch, distance[depth], dir_name[door] );
    ch_printf( ch, "\r\n" );
    return;
}
