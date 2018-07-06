/*
 * RAM $Id: act_wiz.c 85 2009-02-08 17:09:18Z ram $
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

#include "merc.h"
#include "tables.h"
#include "strings.h"
#include "random.h"
#include "db.h"
#include "interp.h"
#include "magic.h"
#include "special.h"
#include "act.h"

void do_wiznet( CHAR_DATA *ch, const char *argument )
{
    int                     flag = 0;
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    if ( argument[0] == '\0' )
    {
        if ( IS_SET( ch->wiznet, WIZ_ON ) )
        {
            ch_printf( ch, "Signing off of Wiznet.\r\n" );
            REMOVE_BIT( ch->wiznet, WIZ_ON );
        }
        else
        {
            ch_printf( ch, "Welcome to Wiznet!\r\n" );
            SET_BIT( ch->wiznet, WIZ_ON );
        }
        return;
    }

    if ( !str_prefix( argument, "on" ) )
    {
        ch_printf( ch, "Welcome to Wiznet!\r\n" );
        SET_BIT( ch->wiznet, WIZ_ON );
        return;
    }

    if ( !str_prefix( argument, "off" ) )
    {
        ch_printf( ch, "Signing off of Wiznet.\r\n" );
        REMOVE_BIT( ch->wiznet, WIZ_ON );
        return;
    }

    /*
     * show wiznet status 
     */
    if ( !str_prefix( argument, "status" ) )
    {
        buf[0] = '\0';

        if ( !IS_SET( ch->wiznet, WIZ_ON ) )
            strcat( buf, "off " );

        for ( flag = 0; wiznet_table[flag].name != NULL; flag++ )
            if ( IS_SET( ch->wiznet, wiznet_table[flag].flag ) )
            {
                strcat( buf, wiznet_table[flag].name );
                strcat( buf, " " );
            }

        strcat( buf, "\r\n" );

        ch_printf( ch, "Wiznet status:\r\n" );
        return;
    }

    if ( !str_prefix( argument, "show" ) )
        /*
         * list of all wiznet options 
         */
    {
        buf[0] = '\0';

        for ( flag = 0; wiznet_table[flag].name != NULL; flag++ )
        {
            if ( wiznet_table[flag].level <= get_trust( ch ) )
            {
                strcat( buf, wiznet_table[flag].name );
                strcat( buf, " " );
            }
        }

        strcat( buf, "\r\n" );

        ch_printf( ch, "Wiznet options available to you are:\r\n" );
        return;
    }

    flag = wiznet_lookup( argument );

    if ( flag == -1 || get_trust( ch ) < wiznet_table[flag].level )
    {
        ch_printf( ch, "No such option.\r\n" );
        return;
    }

    if ( IS_SET( ch->wiznet, wiznet_table[flag].flag ) )
    {
        ch_printf( ch, "You will no longer see %s on wiznet.\r\n",
                   wiznet_table[flag].name );
        REMOVE_BIT( ch->wiznet, wiznet_table[flag].flag );
        return;
    }
    else
    {
        ch_printf( ch, "You will now see %s on wiznet.\r\n", wiznet_table[flag].name );
        SET_BIT( ch->wiznet, wiznet_table[flag].flag );
        return;
    }

}

void do_guild( CHAR_DATA *ch, const char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;
    int                     clan = 0;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        ch_printf( ch, "Syntax: guild <char> <cln name>\r\n" );
        return;
    }
    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        ch_printf( ch, "They aren't playing.\r\n" );
        return;
    }

    if ( !str_prefix( arg2, "none" ) )
    {
        ch_printf( ch, "They are now clanless.\r\n" );
        ch_printf( victim, "You are now a member of no clan!\r\n" );
        victim->clan = 0;
        return;
    }

    if ( ( clan = clan_lookup( arg2 ) ) == 0 )
    {
        ch_printf( ch, "No such clan exists.\r\n" );
        return;
    }

    if ( clan_table[clan].independent )
    {
        ch_printf( ch, "They are now a %s.\r\n", clan_table[clan].name );
        ch_printf( victim, "You are now a %s.\r\n", clan_table[clan].name );
    }
    else
    {
        ch_printf( ch, "They are now a member of clan %s.\r\n",
                   capitalize( clan_table[clan].name ) );
        ch_printf( victim, "You are now a member of clan %s.\r\n",
                   capitalize( clan_table[clan].name ) );
    }

    victim->clan = clan;
}

/* equips a character */
void do_outfit( CHAR_DATA *ch, const char *argument )
{
    OBJ_DATA               *obj = NULL;
    int                     i = 0;
    int                     sn = -1;
    int                     vnum = -1;

    if ( ch->level > 5 || IS_NPC( ch ) )
    {
        ch_printf( ch, "Find it yourself!\r\n" );
        return;
    }

    if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) == NULL )
    {
        obj = create_object( get_obj_index( OBJ_VNUM_SCHOOL_BANNER ), 0 );
        obj->cost = 0;
        obj_to_char( obj, ch );
        equip_char( ch, obj, WEAR_LIGHT );
    }

    if ( ( obj = get_eq_char( ch, WEAR_BODY ) ) == NULL )
    {
        obj = create_object( get_obj_index( OBJ_VNUM_SCHOOL_VEST ), 0 );
        obj->cost = 0;
        obj_to_char( obj, ch );
        equip_char( ch, obj, WEAR_BODY );
    }

    /*
     * do the weapon thing 
     */
    if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL )
    {
        sn = 0;
        vnum = OBJ_VNUM_SCHOOL_SWORD;                  /* just in case! */

        for ( i = 0; weapon_table[i].name != NULL; i++ )
        {
            if ( ch->pcdata->learned[sn] <
                 ch->pcdata->learned[skill_lookup( weapon_table[i].skill )] )
            {
                sn = skill_lookup( weapon_table[i].skill );
                vnum = weapon_table[i].vnum;
            }
        }

        obj = create_object( get_obj_index( vnum ), 0 );
        obj_to_char( obj, ch );
        equip_char( ch, obj, WEAR_WIELD );
    }

    if ( ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL
           || !IS_WEAPON_STAT( obj, WEAPON_TWO_HANDS ) )
         && ( obj = get_eq_char( ch, WEAR_SHIELD ) ) == NULL )
    {
        obj = create_object( get_obj_index( OBJ_VNUM_SCHOOL_SHIELD ), 0 );
        obj->cost = 0;
        obj_to_char( obj, ch );
        equip_char( ch, obj, WEAR_SHIELD );
    }

    ch_printf( ch, "You have been equipped by Mota.\r\n" );
}

/* RT nochannels command, for those spammers */
void do_nochannels( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Nochannel whom?" );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        ch_printf( ch, "They aren't here.\r\n" );
        return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
        ch_printf( ch, "You failed.\r\n" );
        return;
    }

    if ( IS_SET( victim->comm, COMM_NOCHANNELS ) )
    {
        REMOVE_BIT( victim->comm, COMM_NOCHANNELS );
        ch_printf( victim, "The gods have restored your channel priviliges.\r\n" );
        ch_printf( ch, "NOCHANNELS removed.\r\n" );
        wiz_printf( ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0,
                    "$N restores channels to %s", NAME( victim ) );
    }
    else
    {
        SET_BIT( victim->comm, COMM_NOCHANNELS );
        ch_printf( victim, "The gods have revoked your channel priviliges.\r\n" );
        ch_printf( ch, "NOCHANNELS set.\r\n" );
        wiz_printf( ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0,
                    "$N revokes %s's channels.", NAME( victim ) );
    }

    return;
}

void do_smote( CHAR_DATA *ch, const char *argument )
{
    CHAR_DATA              *vch = NULL;
    char                   *letter = NULL;
    char                   *name = NULL;
    char                    last[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    temp[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    size_t                  matches = 0;

    if ( !IS_NPC( ch ) && IS_SET( ch->comm, COMM_NOEMOTE ) )
    {
        ch_printf( ch, "You can't show your emotions.\r\n" );
        return;
    }

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Emote what?\r\n" );
        return;
    }

    if ( strstr( argument, ch->name ) == NULL )
    {
        ch_printf( ch, "You must include your name in an smote.\r\n" );
        return;
    }

    ch_printf( ch, "%s\r\n", argument );

    for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room )
    {
        if ( vch->desc == NULL || vch == ch )
            continue;

        if ( ( letter = strstr( argument, vch->name ) ) == NULL )
        {
            ch_printf( vch, "%s\r\n", argument );
            continue;
        }

        strcpy( temp, argument );
        temp[strlen( argument ) - strlen( letter )] = '\0';
        last[0] = '\0';
        name = vch->name;

        for ( ; *letter != '\0'; letter++ )
        {
            if ( *letter == '\'' && matches == strlen( vch->name ) )
            {
                strcat( temp, "r" );
                continue;
            }

            if ( *letter == 's' && matches == strlen( vch->name ) )
            {
                matches = 0;
                continue;
            }

            if ( matches == strlen( vch->name ) )
            {
                matches = 0;
            }

            if ( *letter == *name )
            {
                matches++;
                name++;
                if ( matches == strlen( vch->name ) )
                {
                    strcat( temp, "you" );
                    last[0] = '\0';
                    name = vch->name;
                    continue;
                }
                strncat( last, letter, 1 );
                continue;
            }

            matches = 0;
            strcat( temp, last );
            strncat( temp, letter, 1 );
            last[0] = '\0';
            name = vch->name;
        }
        ch_printf( vch, "%s\r\n", temp );
    }

    return;
}

void do_bamfin( CHAR_DATA *ch, const char *argument )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    if ( !IS_NPC( ch ) )
    {
        if ( argument[0] == '\0' )
        {
            ch_printf( ch, "Your poofin is %s\r\n", ch->pcdata->bamfin );
            return;
        }

        strcpy( buf, argument );
        smash_tilde( buf );

        if ( strstr( buf, ch->name ) == NULL )
        {
            ch_printf( ch, "You must include your name.\r\n" );
            return;
        }

        free_string( ch->pcdata->bamfin );
        ch->pcdata->bamfin = str_dup( buf );

        ch_printf( ch, "Your poofin is now %s\r\n", ch->pcdata->bamfin );
    }
    return;
}

void do_bamfout( CHAR_DATA *ch, const char *argument )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    if ( !IS_NPC( ch ) )
    {
        if ( argument[0] == '\0' )
        {
            ch_printf( ch, "Your poofout is %s\r\n", ch->pcdata->bamfout );
            return;
        }

        strcpy( buf, argument );
        smash_tilde( buf );

        if ( strstr( buf, ch->name ) == NULL )
        {
            ch_printf( ch, "You must include your name.\r\n" );
            return;
        }

        free_string( ch->pcdata->bamfout );
        ch->pcdata->bamfout = str_dup( buf );

        ch_printf( ch, "Your poofout is now %s\r\n", ch->pcdata->bamfout );
    }
    return;
}

void do_deny( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Deny whom?\r\n" );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        ch_printf( ch, "They aren't here.\r\n" );
        return;
    }

    if ( IS_NPC( victim ) )
    {
        ch_printf( ch, "Not on NPC's.\r\n" );
        return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
        ch_printf( ch, "You failed.\r\n" );
        return;
    }

    SET_BIT( victim->act, PLR_DENY );
    ch_printf( victim, "You are denied access!\r\n" );
    wiz_printf( ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0,
                "$N denies access to %s", NAME( victim ) );
    ch_printf( ch, "OK.\r\n" );
    save_char_obj( victim );
    stop_fighting( victim, true );
    do_function( victim, &do_quit, "" );

    return;
}

void do_disconnect( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    DESCRIPTOR_DATA        *d = NULL;
    CHAR_DATA              *victim = NULL;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Disconnect whom?\r\n" );
        return;
    }

    if ( is_number( arg ) )
    {
        int                     sock = 0;

        sock = atoi( arg );
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( d->socket == sock )
            {
                close_descriptor( d );
                ch_printf( ch, "Ok.\r\n" );
                return;
            }
        }
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        ch_printf( ch, "They aren't here.\r\n" );
        return;
    }

    if ( victim->desc == NULL )
    {
        act( "$N doesn't have a descriptor.", ch, NULL, victim, TO_CHAR );
        return;
    }

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ( d == victim->desc )
        {
            close_descriptor( d );
            ch_printf( ch, "Ok.\r\n" );
            return;
        }
    }

    log_error( "%s attempted to disconnect a non-existant descriptor!", NAME( ch ) );
    ch_printf( ch, "Descriptor not found!\r\n" );
    return;
}

void do_pardon( CHAR_DATA *ch, const char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        ch_printf( ch, "Syntax: pardon <character> <killer|thief>.\r\n" );
        return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        ch_printf( ch, "They aren't here.\r\n" );
        return;
    }

    if ( IS_NPC( victim ) )
    {
        ch_printf( ch, "Not on NPC's.\r\n" );
        return;
    }

    if ( !str_cmp( arg2, "killer" ) )
    {
        if ( IS_SET( victim->act, PLR_KILLER ) )
        {
            REMOVE_BIT( victim->act, PLR_KILLER );
            ch_printf( ch, "Killer flag removed.\r\n" );
            ch_printf( victim, "You are no longer a KILLER.\r\n" );
        }
        return;
    }

    if ( !str_cmp( arg2, "thief" ) )
    {
        if ( IS_SET( victim->act, PLR_THIEF ) )
        {
            REMOVE_BIT( victim->act, PLR_THIEF );
            ch_printf( ch, "Thief flag removed.\r\n" );
            ch_printf( victim, "You are no longer a THIEF.\r\n" );
        }
        return;
    }

    ch_printf( ch, "Syntax: pardon <character> <killer|thief>.\r\n" );
    return;
}

void do_echo( CHAR_DATA *ch, const char *argument )
{
    DESCRIPTOR_DATA        *d = NULL;

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Global echo what?\r\n" );
        return;
    }

    for ( d = descriptor_list; d; d = d->next )
    {
        if ( d->connected == CON_PLAYING )
        {
            if ( get_trust( d->character ) >= get_trust( ch ) )
                ch_printf( d->character, "global> " );
            ch_printf( d->character, "%s\r\n", argument );
        }
    }

    return;
}

void do_recho( CHAR_DATA *ch, const char *argument )
{
    DESCRIPTOR_DATA        *d = NULL;

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Local echo what?\r\n" );

        return;
    }

    for ( d = descriptor_list; d; d = d->next )
    {
        if ( d->connected == CON_PLAYING && d->character->in_room == ch->in_room )
        {
            if ( get_trust( d->character ) >= get_trust( ch ) )
                ch_printf( d->character, "local> " );
            ch_printf( d->character, "%s\r\n", argument );
        }
    }

    return;
}

void do_zecho( CHAR_DATA *ch, const char *argument )
{
    DESCRIPTOR_DATA        *d = NULL;

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Zone echo what?\r\n" );
        return;
    }

    for ( d = descriptor_list; d; d = d->next )
    {
        if ( d->connected == CON_PLAYING
             && d->character->in_room != NULL && ch->in_room != NULL
             && d->character->in_room->area == ch->in_room->area )
        {
            if ( get_trust( d->character ) >= get_trust( ch ) )
                ch_printf( d->character, "zone> " );
            ch_printf( d->character, "%s\r\n", argument );
        }
    }
}

void do_pecho( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;

    argument = one_argument( argument, arg );

    if ( argument[0] == '\0' || arg[0] == '\0' )
    {
        ch_printf( ch, "Personal echo what?\r\n" );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        ch_printf( ch, "Target not found.\r\n" );
        return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) && get_trust( ch ) != MAX_LEVEL )
        ch_printf( victim, "personal> " );

    ch_printf( victim, "%s\r\n", argument );
    ch_printf( ch, "personal> " );
    ch_printf( ch, "%s\r\n", argument );
}

ROOM_INDEX_DATA        *find_location( CHAR_DATA *ch, const char *arg )
{
    CHAR_DATA              *victim = NULL;
    OBJ_DATA               *obj = NULL;

    if ( is_number( arg ) )
        return get_room_index( atoi( arg ) );

    if ( ( victim = get_char_world( ch, arg ) ) != NULL )
        return victim->in_room;

    if ( ( obj = get_obj_world( ch, arg ) ) != NULL )
        return obj->in_room;

    return NULL;
}

void do_transfer( CHAR_DATA *ch, const char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    ROOM_INDEX_DATA        *location = NULL;
    DESCRIPTOR_DATA        *d = NULL;
    CHAR_DATA              *victim = NULL;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' )
    {
        ch_printf( ch, "Transfer whom (and where)?\r\n" );
        return;
    }

    if ( !str_cmp( arg1, "all" ) )
    {
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( d->connected == CON_PLAYING
                 && d->character != ch
                 && d->character->in_room != NULL && can_see( ch, d->character ) )
            {
                char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

                sprintf( buf, "%s %s", d->character->name, arg2 );
                do_function( ch, &do_transfer, buf );
            }
        }
        return;
    }

    /*
     * Thanks to Grodyn for the optional location parameter.
     */
    if ( arg2[0] == '\0' )
    {
        location = ch->in_room;
    }
    else
    {
        if ( ( location = find_location( ch, arg2 ) ) == NULL )
        {
            ch_printf( ch, "No such location.\r\n" );
            return;
        }

        if ( !is_room_owner( ch, location ) && room_is_private( location )
             && get_trust( ch ) < MAX_LEVEL )
        {
            ch_printf( ch, "That room is private right now.\r\n" );
            return;
        }
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        ch_printf( ch, "They aren't here.\r\n" );
        return;
    }

    if ( victim->in_room == NULL )
    {
        ch_printf( ch, "They are in limbo.\r\n" );
        return;
    }

    if ( victim->fighting != NULL )
        stop_fighting( victim, true );
    act( "$n disappears in a mushroom cloud.", victim, NULL, NULL, TO_ROOM );
    char_from_room( victim );
    char_to_room( victim, location );
    act( "$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM );
    if ( ch != victim )
        act( "$n has transferred you.", ch, NULL, victim, TO_VICT );
    do_function( victim, &do_look, "auto" );
    ch_printf( ch, "Ok.\r\n" );
}

void do_at( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    ROOM_INDEX_DATA        *location = NULL;
    ROOM_INDEX_DATA        *original = NULL;
    OBJ_DATA               *on = NULL;
    CHAR_DATA              *wch = NULL;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
        ch_printf( ch, "At where what?\r\n" );
        return;
    }

    if ( ( location = find_location( ch, arg ) ) == NULL )
    {
        ch_printf( ch, "No such location.\r\n" );
        return;
    }

    if ( !is_room_owner( ch, location ) && room_is_private( location )
         && get_trust( ch ) < MAX_LEVEL )
    {
        ch_printf( ch, "That room is private right now.\r\n" );
        return;
    }

    original = ch->in_room;
    on = ch->on;
    char_from_room( ch );
    char_to_room( ch, location );
    interpret( ch, argument );

    /*
     * See if 'ch' still exists before continuing!
     * Handles 'at XXXX quit' case.
     */
    for ( wch = char_list; wch != NULL; wch = wch->next )
    {
        if ( wch == ch )
        {
            OBJ_DATA               *obj = NULL;

            char_from_room( ch );
            char_to_room( ch, original );

            /*
             * See if on still exists before setting ch->on back to it. 
             */
            for ( obj = original->contents; obj; obj = obj->next_content )
            {
                if ( obj == on )
                {
                    ch->on = on;
                    break;
                }
            }
            break;
        }
    }
    return;
}

void do_goto( CHAR_DATA *ch, const char *argument )
{
    ROOM_INDEX_DATA        *location = NULL;
    CHAR_DATA              *rch = NULL;
    int                     count = 0;

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Goto where?\r\n" );
        return;
    }

    if ( ( location = find_location( ch, argument ) ) == NULL )
    {
        ch_printf( ch, "No such location.\r\n" );
        return;
    }

    count = 0;
    for ( rch = location->people; rch != NULL; rch = rch->next_in_room )
        count++;

    if ( !is_room_owner( ch, location ) && room_is_private( location )
         && ( count > 1 || get_trust( ch ) < MAX_LEVEL ) )
    {
        ch_printf( ch, "That room is private right now.\r\n" );
        return;
    }

    if ( ch->fighting != NULL )
        stop_fighting( ch, true );

    for ( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room )
    {
        if ( get_trust( rch ) >= ch->invis_level )
        {
            if ( ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0' )
                act( "$t", ch, ch->pcdata->bamfout, rch, TO_VICT );
            else
                act( "$n leaves in a swirling mist.", ch, NULL, rch, TO_VICT );
        }
    }

    char_from_room( ch );
    char_to_room( ch, location );

    for ( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room )
    {
        if ( get_trust( rch ) >= ch->invis_level )
        {
            if ( ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0' )
                act( "$t", ch, ch->pcdata->bamfin, rch, TO_VICT );
            else
                act( "$n appears in a swirling mist.", ch, NULL, rch, TO_VICT );
        }
    }

    do_function( ch, &do_look, "auto" );
    return;
}

void do_violate( CHAR_DATA *ch, const char *argument )
{
    ROOM_INDEX_DATA        *location = NULL;
    CHAR_DATA              *rch = NULL;

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Goto where?\r\n" );
        return;
    }

    if ( ( location = find_location( ch, argument ) ) == NULL )
    {
        ch_printf( ch, "No such location.\r\n" );
        return;
    }

    if ( !room_is_private( location ) )
    {
        ch_printf( ch, "That room isn't private, use goto.\r\n" );
        return;
    }

    if ( ch->fighting != NULL )
        stop_fighting( ch, true );

    for ( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room )
    {
        if ( get_trust( rch ) >= ch->invis_level )
        {
            if ( ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0' )
                act( "$t", ch, ch->pcdata->bamfout, rch, TO_VICT );
            else
                act( "$n leaves in a swirling mist.", ch, NULL, rch, TO_VICT );
        }
    }

    char_from_room( ch );
    char_to_room( ch, location );

    for ( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room )
    {
        if ( get_trust( rch ) >= ch->invis_level )
        {
            if ( ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0' )
                act( "$t", ch, ch->pcdata->bamfin, rch, TO_VICT );
            else
                act( "$n appears in a swirling mist.", ch, NULL, rch, TO_VICT );
        }
    }

    do_function( ch, &do_look, "auto" );
    return;
}

/* RT to replace the 3 stat commands */
void do_stat( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    const char             *string = NULL;
    OBJ_DATA               *obj = NULL;
    ROOM_INDEX_DATA        *location = NULL;
    CHAR_DATA              *victim = NULL;

    string = one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Syntax:\r\n" );
        ch_printf( ch, "  stat <name>\r\n" );
        ch_printf( ch, "  stat obj <name>\r\n" );
        ch_printf( ch, "  stat mob <name>\r\n" );
        ch_printf( ch, "  stat room <number>\r\n" );
        return;
    }

    if ( !str_cmp( arg, "room" ) )
    {
        do_function( ch, &do_rstat, string );
        return;
    }

    if ( !str_cmp( arg, "obj" ) )
    {
        do_function( ch, &do_ostat, string );
        return;
    }

    if ( !str_cmp( arg, "char" ) || !str_cmp( arg, "mob" ) )
    {
        do_function( ch, &do_mstat, string );
        return;
    }

    /*
     * do it the old way 
     */

    obj = get_obj_world( ch, argument );
    if ( obj != NULL )
    {
        do_function( ch, &do_ostat, argument );
        return;
    }

    victim = get_char_world( ch, argument );
    if ( victim != NULL )
    {
        do_function( ch, &do_mstat, argument );
        return;
    }

    location = find_location( ch, argument );
    if ( location != NULL )
    {
        do_function( ch, &do_rstat, argument );
        return;
    }

    ch_printf( ch, "Nothing by that name found anywhere.\r\n" );
}

void do_rstat( CHAR_DATA *ch, const char *argument )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    ROOM_INDEX_DATA        *location = NULL;
    OBJ_DATA               *obj = NULL;
    CHAR_DATA              *rch = NULL;
    int                     door = 0;

    one_argument( argument, arg );
    location = ( arg[0] == '\0' ) ? ch->in_room : find_location( ch, arg );
    if ( location == NULL )
    {
        ch_printf( ch, "No such location.\r\n" );
        return;
    }

    if ( !is_room_owner( ch, location ) && ch->in_room != location
         && room_is_private( location ) && !IS_TRUSTED( ch, IMPLEMENTOR ) )
    {
        ch_printf( ch, "That room is private right now.\r\n" );
        return;
    }

    ch_printf( ch, "Name: '%s'\r\nArea: '%s'\r\n", location->name, location->area->name );

    ch_printf( ch,
               "Vnum: %d  Sector: %d  Light: %d  Healing: %d  Mana: %d\r\n",
               location->vnum,
               location->sector_type,
               location->light, location->heal_rate, location->mana_rate );

    ch_printf( ch,
               "Room flags: %d.\r\nDescription:\r\n%s",
               location->room_flags, location->description );

    if ( location->extra_descr != NULL )
    {
        EXTRA_DESCR_DATA       *ed = NULL;

        ch_printf( ch, "Extra description keywords: '" );
        for ( ed = location->extra_descr; ed; ed = ed->next )
        {
            ch_printf( ch, "%s", ed->keyword );
            if ( ed->next != NULL )
                ch_printf( ch, " " );
        }
        ch_printf( ch, "'.\r\n" );
    }

    ch_printf( ch, "Characters:" );
    for ( rch = location->people; rch; rch = rch->next_in_room )
    {
        if ( can_see( ch, rch ) )
        {
            one_argument( rch->name, buf );
            ch_printf( ch, " %s", buf );
        }
    }

    ch_printf( ch, ".\r\nObjects:   " );
    for ( obj = location->contents; obj; obj = obj->next_content )
    {
        one_argument( obj->name, buf );
        ch_printf( ch, " %s", buf );
    }
    ch_printf( ch, ".\r\n" );

    for ( door = 0; door < MAX_EXIT; door++ )
    {
        EXIT_DATA              *pexit = NULL;

        if ( ( pexit = location->exit[door] ) != NULL )
        {
            ch_printf( ch,
                       "Door: %d.  To: %d.  Key: %d.  Exit flags: %d.\r\nKeyword: '%s'.  Description: %s",
                       door,
                       ( pexit->u1.to_room == NULL ? -1 : pexit->u1.to_room->vnum ),
                       pexit->key,
                       pexit->exit_info,
                       pexit->keyword,
                       pexit->description[0] !=
                       '\0' ? pexit->description : "(none).\r\n" );
        }
    }

    return;
}

void do_ostat( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    AFFECT_DATA            *paf = NULL;
    OBJ_DATA               *obj = NULL;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Stat what?\r\n" );
        return;
    }

    if ( ( obj = get_obj_world( ch, argument ) ) == NULL )
    {
        ch_printf( ch, "Nothing like that in hell, earth, or heaven.\r\n" );
        return;
    }

    ch_printf( ch, "Name(s): %s\r\n", obj->name );
    ch_printf( ch, "Vnum: %d  Format: %s  Type: %s  Resets: %d\r\n",
               obj->pIndexData->vnum, obj->pIndexData->new_format ? "new" : "old",
               item_name( obj->item_type ), obj->pIndexData->reset_num );
    ch_printf( ch, "Short description: %s\r\nLong description: %s\r\n",
               obj->short_descr, obj->description );
    ch_printf( ch, "Wear bits: %s\r\nExtra bits: %s\r\n",
               wear_bit_name( obj->wear_flags ), extra_bit_name( obj->extra_flags ) );
    ch_printf( ch, "Number: %d/%d  Weight: %d/%d/%d (10th pounds)\r\n",
               1, get_obj_number( obj ),
               obj->weight, get_obj_weight( obj ), get_true_weight( obj ) );
    ch_printf( ch, "Level: %d  Cost: %d  Condition: %d  Timer: %d\r\n",
               obj->level, obj->cost, obj->condition, obj->timer );
    ch_printf( ch,
               "In room: %d  In object: %s  Carried by: %s  Wear_loc: %d\r\n",
               obj->in_room == NULL ? 0 : obj->in_room->vnum,
               obj->in_obj == NULL ? "(none)" : obj->in_obj->short_descr,
               obj->carried_by == NULL ? "(none)" :
               can_see( ch, obj->carried_by ) ? obj->carried_by->name
               : "someone", obj->wear_loc );
    ch_printf( ch, "Values: %d %d %d %d %d\r\n",
               obj->value[0], obj->value[1], obj->value[2], obj->value[3],
               obj->value[4] );

    /*
     * now give out vital statistics as per identify 
     */

    switch ( obj->item_type )
    {
        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_PILL:
            ch_printf( ch, "Level %d spells of:", obj->value[0] );

            if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL )
            {
                ch_printf( ch, " '%s'", skill_table[obj->value[1]].name );
            }

            if ( obj->value[2] >= 0 && obj->value[2] < MAX_SKILL )
            {
                ch_printf( ch, " '%s'", skill_table[obj->value[2]].name );
            }

            if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
            {
                ch_printf( ch, " '%s'", skill_table[obj->value[3]].name );
            }

            if ( obj->value[4] >= 0 && obj->value[4] < MAX_SKILL )
            {
                ch_printf( ch, " '%s'", skill_table[obj->value[4]].name );
            }

            ch_printf( ch, ".\r\n" );
            break;

        case ITEM_WAND:
        case ITEM_STAFF:
            ch_printf( ch, "Has %d(%d) charges of level %d",
                       obj->value[1], obj->value[2], obj->value[0] );

            if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
            {
                ch_printf( ch, " '%s'", skill_table[obj->value[3]].name );
            }

            ch_printf( ch, ".\r\n" );
            break;

        case ITEM_DRINK_CON:
            ch_printf( ch, "It holds %s-colored %s.\r\n",
                       liq_table[obj->value[2]].liq_color,
                       liq_table[obj->value[2]].liq_name );
            break;

        case ITEM_WEAPON:
            ch_printf( ch, "Weapon type is " );
            switch ( obj->value[0] )
            {
                case ( WEAPON_EXOTIC ):
                    ch_printf( ch, "exotic\r\n" );
                    break;
                case ( WEAPON_SWORD ):
                    ch_printf( ch, "sword\r\n" );
                    break;
                case ( WEAPON_DAGGER ):
                    ch_printf( ch, "dagger\r\n" );
                    break;
                case ( WEAPON_SPEAR ):
                    ch_printf( ch, "spear/staff\r\n" );
                    break;
                case ( WEAPON_MACE ):
                    ch_printf( ch, "mace/club\r\n" );
                    break;
                case ( WEAPON_AXE ):
                    ch_printf( ch, "axe\r\n" );
                    break;
                case ( WEAPON_FLAIL ):
                    ch_printf( ch, "flail\r\n" );
                    break;
                case ( WEAPON_WHIP ):
                    ch_printf( ch, "whip\r\n" );
                    break;
                case ( WEAPON_POLEARM ):
                    ch_printf( ch, "polearm\r\n" );
                    break;
                default:
                    ch_printf( ch, "unknown\r\n" );
                    break;
            }
            if ( obj->pIndexData->new_format )
                ch_printf( ch, "Damage is %dd%d (average %d)\r\n",
                           obj->value[1], obj->value[2],
                           ( 1 + obj->value[2] ) * obj->value[1] / 2 );
            else
                ch_printf( ch, "Damage is %d to %d (average %d)\r\n",
                           obj->value[1], obj->value[2],
                           ( obj->value[1] + obj->value[2] ) / 2 );

            ch_printf( ch, "Damage noun is %s.\r\n",
                       ( obj->value[3] > 0 && obj->value[3] < MAX_DAMAGE_MESSAGE ) ?
                       attack_table[obj->value[3]].noun : "undefined" );

            if ( obj->value[4] )                       /* weapon flags */
            {
                ch_printf( ch, "Weapons flags: %s\r\n",
                           weapon_bit_name( obj->value[4] ) );
            }
            break;

        case ITEM_ARMOR:
            ch_printf( ch,
                       "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic\r\n",
                       obj->value[0], obj->value[1], obj->value[2], obj->value[3] );
            break;

        case ITEM_CONTAINER:
            ch_printf( ch, "Capacity: %d#  Maximum weight: %d#  flags: %s\r\n",
                       obj->value[0], obj->value[3], cont_bit_name( obj->value[1] ) );
            if ( obj->value[4] != 100 )
            {
                ch_printf( ch, "Weight multiplier: %d%%\r\n", obj->value[4] );
            }
            break;
    }

    if ( obj->extra_descr != NULL || obj->pIndexData->extra_descr != NULL )
    {
        EXTRA_DESCR_DATA       *ed = NULL;

        ch_printf( ch, "Extra description keywords: '" );

        for ( ed = obj->extra_descr; ed != NULL; ed = ed->next )
        {
            ch_printf( ch, "%s%s", ed->keyword, ed->next == NULL ? "" : " " );
        }

        for ( ed = obj->pIndexData->extra_descr; ed != NULL; ed = ed->next )
        {
            ch_printf( ch, "%s%s", ed->keyword, ed->next == NULL ? "" : " " );
        }

        ch_printf( ch, "'\r\n" );
    }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
        ch_printf( ch, "Affects %s by %d, level %d",
                   affect_loc_name( paf->location ), paf->modifier, paf->level );
        if ( paf->duration > -1 )
            ch_printf( ch, ", %d hours.\r\n", paf->duration );
        else
            ch_printf( ch, ".\r\n" );
        if ( paf->bitvector )
        {
            switch ( paf->where )
            {
                case TO_AFFECTS:
                    ch_printf( ch, "Adds %s affect.\n",
                               affect_bit_name( paf->bitvector ) );
                    break;
                case TO_WEAPON:
                    ch_printf( ch, "Adds %s weapon flags.\n",
                               weapon_bit_name( paf->bitvector ) );
                    break;
                case TO_OBJECT:
                    ch_printf( ch, "Adds %s object flag.\n",
                               extra_bit_name( paf->bitvector ) );
                    break;
                case TO_IMMUNE:
                    ch_printf( ch, "Adds immunity to %s.\n",
                               imm_bit_name( paf->bitvector ) );
                    break;
                case TO_RESIST:
                    ch_printf( ch, "Adds resistance to %s.\r\n",
                               imm_bit_name( paf->bitvector ) );
                    break;
                case TO_VULN:
                    ch_printf( ch, "Adds vulnerability to %s.\r\n",
                               imm_bit_name( paf->bitvector ) );
                    break;
                default:
                    ch_printf( ch, "Unknown bit %d: %d\r\n", paf->where, paf->bitvector );
                    break;
            }
        }
    }

    if ( !obj->enchanted )
        for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
        {
            ch_printf( ch, "Affects %s by %d, level %d.\r\n",
                       affect_loc_name( paf->location ), paf->modifier, paf->level );
            if ( paf->bitvector )
            {
                switch ( paf->where )
                {
                    case TO_AFFECTS:
                        ch_printf( ch, "Adds %s affect.\n",
                                   affect_bit_name( paf->bitvector ) );
                        break;
                    case TO_OBJECT:
                        ch_printf( ch, "Adds %s object flag.\n",
                                   extra_bit_name( paf->bitvector ) );
                        break;
                    case TO_IMMUNE:
                        ch_printf( ch, "Adds immunity to %s.\n",
                                   imm_bit_name( paf->bitvector ) );
                        break;
                    case TO_RESIST:
                        ch_printf( ch, "Adds resistance to %s.\r\n",
                                   imm_bit_name( paf->bitvector ) );
                        break;
                    case TO_VULN:
                        ch_printf( ch, "Adds vulnerability to %s.\r\n",
                                   imm_bit_name( paf->bitvector ) );
                        break;
                    default:
                        ch_printf( ch, "Unknown bit %d: %d\r\n",
                                   paf->where, paf->bitvector );
                        break;
                }
            }
        }

    return;
}

void do_mstat( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    AFFECT_DATA            *paf = NULL;
    CHAR_DATA              *victim = NULL;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Stat whom?\r\n" );
        return;
    }

    if ( ( victim = get_char_world( ch, argument ) ) == NULL )
    {
        ch_printf( ch, "They aren't here.\r\n" );
        return;
    }

    ch_printf( ch, "Name: %s\r\n", victim->name );
    ch_printf( ch,
               "Vnum: %d  Format: %s  Race: %s  Group: %d  Sex: %s  Room: %d\r\n",
               IS_NPC( victim ) ? victim->pIndexData->vnum : 0,
               IS_NPC( victim ) ? victim->pIndexData->new_format ? "new" : "old" : "pc",
               race_table[victim->race].name,
               IS_NPC( victim ) ? victim->group : 0, sex_table[victim->sex].name,
               victim->in_room == NULL ? 0 : victim->in_room->vnum );

    if ( IS_NPC( victim ) )
    {
        ch_printf( ch, "Count: %d  Killed: %d\r\n",
                   victim->pIndexData->count, victim->pIndexData->killed );
    }

    ch_printf( ch,
               "Str: %d(%d)  Int: %d(%d)  Wis: %d(%d)  Dex: %d(%d)  Con: %d(%d)\r\n",
               victim->perm_stat[STAT_STR],
               get_curr_stat( victim, STAT_STR ),
               victim->perm_stat[STAT_INT],
               get_curr_stat( victim, STAT_INT ),
               victim->perm_stat[STAT_WIS],
               get_curr_stat( victim, STAT_WIS ),
               victim->perm_stat[STAT_DEX],
               get_curr_stat( victim, STAT_DEX ),
               victim->perm_stat[STAT_CON], get_curr_stat( victim, STAT_CON ) );

    ch_printf( ch, "Hp: %d/%d  Mana: %d/%d  Move: %d/%d  Practices: %d\r\n",
               victim->hit, victim->max_hit,
               victim->mana, victim->max_mana,
               victim->move, victim->max_move, IS_NPC( victim ) ? 0 : victim->practice );

    ch_printf( ch,
               "Lv: %d  Class: %s  Align: %d  Gold: %d  Silver: %d  Exp: %d\r\n",
               victim->level,
               IS_NPC( victim ) ? "mobile" : class_table[victim->iclass].name,
               victim->alignment, victim->gold, victim->silver, victim->exp );

    ch_printf( ch, "Armor: pierce: %d  bash: %d  slash: %d  magic: %d\r\n",
               GET_AC( victim, AC_PIERCE ), GET_AC( victim, AC_BASH ),
               GET_AC( victim, AC_SLASH ), GET_AC( victim, AC_EXOTIC ) );

    ch_printf( ch,
               "Hit: %d  Dam: %d  Saves: %d  Size: %s  Position: %s  Wimpy: %d\r\n",
               GET_HITROLL( victim ), GET_DAMROLL( victim ), victim->saving_throw,
               size_table[victim->size].name, position_table[victim->position].name,
               victim->wimpy );

    if ( IS_NPC( victim ) && victim->pIndexData->new_format )
    {
        ch_printf( ch, "Damage: %dd%d  Message:  %s\r\n",
                   victim->damage[DICE_NUMBER], victim->damage[DICE_TYPE],
                   attack_table[victim->dam_type].noun );
    }

    ch_printf( ch, "Fighting: %s\r\n",
               victim->fighting ? victim->fighting->name : "(none)" );

    if ( !IS_NPC( victim ) )
    {
        ch_printf( ch,
                   "Thirst: %d  Hunger: %d  Full: %d  Drunk: %d\r\n",
                   victim->pcdata->condition[COND_THIRST],
                   victim->pcdata->condition[COND_HUNGER],
                   victim->pcdata->condition[COND_FULL],
                   victim->pcdata->condition[COND_DRUNK] );
    }

    ch_printf( ch, "Carry number: %d  Carry weight: %d\r\n",
               victim->carry_number, get_carry_weight( victim ) / 10 );

    if ( !IS_NPC( victim ) )
    {
        ch_printf( ch,
                   "Age: %d  Played: %d  Last Level: %d  Timer: %d\r\n",
                   get_age( victim ),
                   ( int ) ( victim->played + current_time - victim->logon ) / 3600,
                   victim->pcdata->last_level, victim->timer );
    }

    ch_printf( ch, "Act: %s\r\n", act_bit_name( victim->act ) );

    if ( victim->comm )
    {
        ch_printf( ch, "Comm: %s\r\n", comm_bit_name( victim->comm ) );
    }

    if ( IS_NPC( victim ) && victim->off_flags )
    {
        ch_printf( ch, "Offense: %s\r\n", off_bit_name( victim->off_flags ) );
    }

    if ( victim->imm_flags )
    {
        ch_printf( ch, "Immune: %s\r\n", imm_bit_name( victim->imm_flags ) );
    }

    if ( victim->res_flags )
    {
        ch_printf( ch, "Resist: %s\r\n", imm_bit_name( victim->res_flags ) );
    }

    if ( victim->vuln_flags )
    {
        ch_printf( ch, "Vulnerable: %s\r\n", imm_bit_name( victim->vuln_flags ) );
    }

    ch_printf( ch, "Form: %s\r\nParts: %s\r\n",
               form_bit_name( victim->form ), part_bit_name( victim->parts ) );

    if ( victim->affected_by )
    {
        ch_printf( ch, "Affected by %s\r\n", affect_bit_name( victim->affected_by ) );
    }

    ch_printf( ch, "Master: %s  Leader: %s  Pet: %s\r\n",
               victim->master ? victim->master->name : "(none)",
               victim->leader ? victim->leader->name : "(none)",
               victim->pet ? victim->pet->name : "(none)" );

    ch_printf( ch, "Short description: %s\r\nLong  description: %s",
               victim->short_descr,
               victim->long_descr[0] != '\0' ? victim->long_descr : "(none)\r\n" );

    if ( !IS_NPC( victim ) )
    {
        ch_printf( ch, "Security: %d.\r\n", victim->pcdata->security ); /* OLC */
    }

    if ( IS_NPC( victim ) && victim->spec_fun != 0 )
    {
        ch_printf( ch, "Mobile has special procedure %s.\r\n",
                   spec_name( victim->spec_fun ) );
    }

    for ( paf = victim->affected; paf != NULL; paf = paf->next )
    {
        ch_printf( ch,
                   "Spell: '%s' modifies %s by %d for %d hours with bits %s, level %d.\r\n",
                   skill_table[( int ) paf->type].name,
                   affect_loc_name( paf->location ),
                   paf->modifier,
                   paf->duration, affect_bit_name( paf->bitvector ), paf->level );
    }

    return;
}

/* ofind and mfind replaced with vnum, vnum skill also added */
void do_vnum( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    const char             *string = NULL;

    string = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Syntax:\r\n" );
        ch_printf( ch, "  vnum obj <name>\r\n" );
        ch_printf( ch, "  vnum mob <name>\r\n" );
        ch_printf( ch, "  vnum skill <skill or spell>\r\n" );
        return;
    }

    if ( !str_cmp( arg, "obj" ) )
    {
        do_function( ch, &do_ofind, string );
        return;
    }

    if ( !str_cmp( arg, "mob" ) || !str_cmp( arg, "char" ) )
    {
        do_function( ch, &do_mfind, string );
        return;
    }

    if ( !str_cmp( arg, "skill" ) || !str_cmp( arg, "spell" ) )
    {
        do_function( ch, &do_slookup, string );
        return;
    }
    /*
     * do both 
     */
    do_function( ch, &do_mfind, argument );
    do_function( ch, &do_ofind, argument );
}

void do_mfind( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    MOB_INDEX_DATA         *pMobIndex = NULL;
    int                     vnum = 0;
    int                     nMatch = 0;
    bool                    fAll = false;
    bool                    found = false;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Find whom?\r\n" );
        return;
    }

    fAll = false;                                      /* !str_cmp( arg, "all" ); */
    found = false;
    nMatch = 0;

    /*
     * Yeah, so iterating over all vnum's takes 10,000 loops.
     * Get_mob_index is fast, and I don't feel like threading another link.
     * Do you?
     * -- Furey
     */
    for ( vnum = 0; nMatch < top_mob_index; vnum++ )
    {
        if ( ( pMobIndex = get_mob_index( vnum ) ) != NULL )
        {
            nMatch++;
            if ( fAll || is_name( argument, pMobIndex->player_name ) )
            {
                found = true;
                ch_printf( ch, "[%5d] %s\r\n", pMobIndex->vnum, pMobIndex->short_descr );
            }
        }
    }

    if ( !found )
        ch_printf( ch, "No mobiles by that name.\r\n" );

    return;
}

void do_ofind( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_INDEX_DATA         *pObjIndex = NULL;
    int                     vnum = 0;
    int                     nMatch = 0;
    bool                    fAll = false;
    bool                    found = false;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Find what?\r\n" );
        return;
    }

    fAll = false;                                      /* !str_cmp( arg, "all" ); */
    found = false;
    nMatch = 0;

    /*
     * Yeah, so iterating over all vnum's takes 10,000 loops.
     * Get_obj_index is fast, and I don't feel like threading another link.
     * Do you?
     * -- Furey
     */
    for ( vnum = 0; nMatch < top_obj_index; vnum++ )
    {
        if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL )
        {
            nMatch++;
            if ( fAll || is_name( argument, pObjIndex->name ) )
            {
                found = true;
                ch_printf( ch, "[%5d] %s\r\n", pObjIndex->vnum, pObjIndex->short_descr );
            }
        }
    }

    if ( !found )
        ch_printf( ch, "No objects by that name.\r\n" );

    return;
}

void do_owhere( CHAR_DATA *ch, const char *argument )
{
    char                    buf[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    BUFFER                 *buffer = NULL;
    OBJ_DATA               *obj = NULL;
    OBJ_DATA               *in_obj = NULL;
    bool                    found = false;
    int                     number = 0;
    int                     max_found = 200;

    buffer = new_buf(  );

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Find what?\r\n" );
        return;
    }

    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
        if ( !can_see_obj( ch, obj ) || !is_name( argument, obj->name )
             || ch->level < obj->level )
            continue;

        found = true;
        number++;

        for ( in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj )
            ;

        if ( in_obj->carried_by != NULL && can_see( ch, in_obj->carried_by )
             && in_obj->carried_by->in_room != NULL )
            sprintf( buf, "%3d) %s is carried by %s [Room %d]\r\n",
                     number, obj->short_descr, PERS( in_obj->carried_by, ch ),
                     in_obj->carried_by->in_room->vnum );
        else if ( in_obj->in_room != NULL && can_see_room( ch, in_obj->in_room ) )
            sprintf( buf, "%3d) %s is in %s [Room %d]\r\n",
                     number, obj->short_descr, in_obj->in_room->name,
                     in_obj->in_room->vnum );
        else
            sprintf( buf, "%3d) %s is somewhere\r\n", number, obj->short_descr );

        buf[0] = UPPER( buf[0] );
        add_buf( buffer, buf );

        if ( number >= max_found )
            break;
    }

    if ( !found )
        ch_printf( ch, "Nothing like that in heaven or earth.\r\n" );
    else
        page_to_char( buf_string( buffer ), ch );

    free_buf( buffer );
}

void do_mwhere( CHAR_DATA *ch, const char *argument )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    BUFFER                 *buffer = NULL;
    CHAR_DATA              *victim = NULL;
    bool                    found = false;
    int                     count = 0;

    if ( argument[0] == '\0' )
    {
        DESCRIPTOR_DATA        *d;

        /*
         * show characters logged 
         */

        buffer = new_buf(  );
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( d->character != NULL && d->connected == CON_PLAYING
                 && d->character->in_room != NULL && can_see( ch, d->character )
                 && can_see_room( ch, d->character->in_room ) )
            {
                victim = d->character;
                count++;
                if ( d->original != NULL )
                    sprintf( buf, "%3d) %s (in the body of %s) is in %s [%d]\r\n",
                             count, d->original->name, victim->short_descr,
                             victim->in_room->name, victim->in_room->vnum );
                else
                    sprintf( buf, "%3d) %s is in %s [%d]\r\n",
                             count, victim->name, victim->in_room->name,
                             victim->in_room->vnum );
                add_buf( buffer, buf );
            }
        }

        page_to_char( buf_string( buffer ), ch );
        free_buf( buffer );
        return;
    }

    found = false;
    buffer = new_buf(  );
    for ( victim = char_list; victim != NULL; victim = victim->next )
    {
        if ( victim->in_room != NULL && is_name( argument, victim->name ) )
        {
            found = true;
            count++;
            sprintf( buf, "%3d) [%5d] %-28s [%5d] %s\r\n", count,
                     IS_NPC( victim ) ? victim->pIndexData->vnum : 0,
                     IS_NPC( victim ) ? victim->short_descr : victim->name,
                     victim->in_room->vnum, victim->in_room->name );
            add_buf( buffer, buf );
        }
    }

    if ( !found )
        act( "You didn't find any $T.", ch, NULL, argument, TO_CHAR );
    else
        page_to_char( buf_string( buffer ), ch );

    free_buf( buffer );

    return;
}

void do_reboo( CHAR_DATA *ch, const char *argument )
{
    ch_printf( ch, "If you want to REBOOT, spell it out.\r\n" );
    return;
}

void do_reboot( CHAR_DATA *ch, const char *argument )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    DESCRIPTOR_DATA        *d = NULL;
    DESCRIPTOR_DATA        *d_next = NULL;
    CHAR_DATA              *vch = NULL;

    if ( ch->invis_level < LEVEL_HERO )
    {
        sprintf( buf, "Reboot by %s.", ch->name );
        do_function( ch, &do_echo, buf );
    }

    merc_down = true;
    for ( d = descriptor_list; d != NULL; d = d_next )
    {
        d_next = d->next;
        vch = d->original ? d->original : d->character;
        if ( vch != NULL )
            save_char_obj( vch );
        close_descriptor( d );
    }

    return;
}

void do_shutdow( CHAR_DATA *ch, const char *argument )
{
    ch_printf( ch, "If you want to SHUTDOWN, spell it out.\r\n" );
    return;
}

void do_shutdown( CHAR_DATA *ch, const char *argument )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    DESCRIPTOR_DATA        *d = NULL;
    DESCRIPTOR_DATA        *d_next = NULL;
    CHAR_DATA              *vch = NULL;

    if ( ch->invis_level < LEVEL_HERO )
        sprintf( buf, "Shutdown by %s.", ch->name );
    append_file( ch, SHUTDOWN_FILE, buf );
    strcat( buf, "\r\n" );
    if ( ch->invis_level < LEVEL_HERO )
    {
        do_function( ch, &do_echo, buf );
    }
    merc_down = true;
    for ( d = descriptor_list; d != NULL; d = d_next )
    {
        d_next = d->next;
        vch = d->original ? d->original : d->character;
        if ( vch != NULL )
            save_char_obj( vch );
        close_descriptor( d );
    }
    return;
}

void do_protect( CHAR_DATA *ch, const char *argument )
{
    CHAR_DATA              *victim = NULL;

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Protect whom from snooping?\r\n" );
        return;
    }

    if ( ( victim = get_char_world( ch, argument ) ) == NULL )
    {
        ch_printf( ch, "You can't find them.\r\n" );
        return;
    }

    if ( IS_SET( victim->comm, COMM_SNOOP_PROOF ) )
    {
        act_new( "$N is no longer snoop-proof.", ch, NULL, victim, TO_CHAR, POS_DEAD );
        ch_printf( victim, "Your snoop-proofing was just removed.\r\n" );
        REMOVE_BIT( victim->comm, COMM_SNOOP_PROOF );
    }
    else
    {
        act_new( "$N is now snoop-proof.", ch, NULL, victim, TO_CHAR, POS_DEAD );
        ch_printf( victim, "You are now immune to snooping.\r\n" );
        SET_BIT( victim->comm, COMM_SNOOP_PROOF );
    }
}

void do_snoop( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    DESCRIPTOR_DATA        *d = NULL;
    CHAR_DATA              *victim = NULL;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Snoop whom?\r\n" );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        ch_printf( ch, "They aren't here.\r\n" );
        return;
    }

    if ( victim->desc == NULL )
    {
        ch_printf( ch, "No descriptor to snoop.\r\n" );
        return;
    }

    if ( victim == ch )
    {
        ch_printf( ch, "Cancelling all snoops.\r\n" );
        wiznet( "$N stops being such a snoop.",
                ch, NULL, WIZ_SNOOPS, WIZ_SECURE, get_trust( ch ) );
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( d->snoop_by == ch->desc )
                d->snoop_by = NULL;
        }
        return;
    }

    if ( victim->desc->snoop_by != NULL )
    {
        ch_printf( ch, "Busy already.\r\n" );
        return;
    }

    if ( !is_room_owner( ch, victim->in_room ) && ch->in_room != victim->in_room
         && room_is_private( victim->in_room ) && !IS_TRUSTED( ch, IMPLEMENTOR ) )
    {
        ch_printf( ch, "That character is in a private room.\r\n" );
        return;
    }

    if ( get_trust( victim ) >= get_trust( ch )
         || IS_SET( victim->comm, COMM_SNOOP_PROOF ) )
    {
        ch_printf( ch, "You failed.\r\n" );
        return;
    }

    if ( ch->desc != NULL )
    {
        for ( d = ch->desc->snoop_by; d != NULL; d = d->snoop_by )
        {
            if ( d->character == victim || d->original == victim )
            {
                ch_printf( ch, "No snoop loops.\r\n" );
                return;
            }
        }
    }

    victim->desc->snoop_by = ch->desc;
    wiz_printf( ch, NULL, WIZ_SNOOPS, WIZ_SECURE, get_trust( ch ),
                "$N starts snooping on %s", NAME( victim ) );
    ch_printf( ch, "Ok.\r\n" );
    return;
}

void do_switch( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Switch into whom?\r\n" );
        return;
    }

    if ( ch->desc == NULL )
        return;

    if ( ch->desc->original != NULL )
    {
        ch_printf( ch, "You are already switched.\r\n" );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        ch_printf( ch, "They aren't here.\r\n" );
        return;
    }

    if ( victim == ch )
    {
        ch_printf( ch, "Ok.\r\n" );
        return;
    }

    if ( !IS_NPC( victim ) )
    {
        ch_printf( ch, "You can only switch into mobiles.\r\n" );
        return;
    }

    if ( !is_room_owner( ch, victim->in_room ) && ch->in_room != victim->in_room
         && room_is_private( victim->in_room ) && !IS_TRUSTED( ch, IMPLEMENTOR ) )
    {
        ch_printf( ch, "That character is in a private room.\r\n" );
        return;
    }

    if ( victim->desc != NULL )
    {
        ch_printf( ch, "Character in use.\r\n" );
        return;
    }

    wiz_printf( ch, NULL, WIZ_SWITCHES, WIZ_SECURE, get_trust( ch ),
                "$N switches into %s", NAME( victim ) );

    ch->desc->character = victim;
    ch->desc->original = ch;
    victim->desc = ch->desc;
    ch->desc = NULL;
    /*
     * change communications to match 
     */
    if ( ch->prompt != NULL )
        victim->prompt = str_dup( ch->prompt );
    victim->comm = ch->comm;
    victim->lines = ch->lines;
    ch_printf( victim, "Ok.\r\n" );
    return;
}

void do_return( CHAR_DATA *ch, const char *argument )
{
    if ( ch->desc == NULL )
        return;

    if ( ch->desc->original == NULL )
    {
        ch_printf( ch, "You aren't switched.\r\n" );
        return;
    }

    ch_printf( ch,
               "You return to your original body. Type replay to see any missed tells.\r\n" );
    if ( ch->prompt != NULL )
    {
        free_string( ch->prompt );
        ch->prompt = NULL;
    }

    wiz_printf( ch->desc->original, 0, WIZ_SWITCHES, WIZ_SECURE, get_trust( ch ),
                "$N returns from %s.", NAME( ch ) );
    ch->desc->character = ch->desc->original;
    ch->desc->original = NULL;
    ch->desc->character->desc = ch->desc;
    ch->desc = NULL;
    return;
}

/* trust levels for load and clone */
bool obj_check( CHAR_DATA *ch, OBJ_DATA *obj )
{
    if ( IS_TRUSTED( ch, GOD )
         || ( IS_TRUSTED( ch, IMMORTAL ) && obj->level <= 20 && obj->cost <= 1000 )
         || ( IS_TRUSTED( ch, DEMI ) && obj->level <= 10 && obj->cost <= 500 )
         || ( IS_TRUSTED( ch, ANGEL ) && obj->level <= 5 && obj->cost <= 250 )
         || ( IS_TRUSTED( ch, AVATAR ) && obj->level == 0 && obj->cost <= 100 ) )
        return true;
    else
        return false;
}

/* for clone, to insure that cloning goes many levels deep */
void recursive_clone( CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *clone )
{
    OBJ_DATA               *c_obj = NULL;
    OBJ_DATA               *t_obj = NULL;

    for ( c_obj = obj->contains; c_obj != NULL; c_obj = c_obj->next_content )
    {
        if ( obj_check( ch, c_obj ) )
        {
            t_obj = create_object( c_obj->pIndexData, 0 );
            clone_object( c_obj, t_obj );
            obj_to_obj( t_obj, clone );
            recursive_clone( ch, c_obj, t_obj );
        }
    }
}

/* command that is similar to load */
void do_clone( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    const char             *rest = NULL;
    CHAR_DATA              *mob = NULL;
    OBJ_DATA               *obj = NULL;

    rest = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Clone what?\r\n" );
        return;
    }

    if ( !str_prefix( arg, "object" ) )
    {
        mob = NULL;
        obj = get_obj_here( ch, rest );
        if ( obj == NULL )
        {
            ch_printf( ch, "You don't see that here.\r\n" );
            return;
        }
    }
    else if ( !str_prefix( arg, "mobile" ) || !str_prefix( arg, "character" ) )
    {
        obj = NULL;
        mob = get_char_room( ch, rest );
        if ( mob == NULL )
        {
            ch_printf( ch, "You don't see that here.\r\n" );
            return;
        }
    }
    else                                               /* find both */
    {
        mob = get_char_room( ch, argument );
        obj = get_obj_here( ch, argument );
        if ( mob == NULL && obj == NULL )
        {
            ch_printf( ch, "You don't see that here.\r\n" );
            return;
        }
    }

    /*
     * clone an object 
     */
    if ( obj != NULL )
    {
        OBJ_DATA               *clone = NULL;

        if ( !obj_check( ch, obj ) )
        {
            ch_printf( ch, "Your powers are not great enough for such a task.\r\n" );
            return;
        }

        clone = create_object( obj->pIndexData, 0 );
        clone_object( obj, clone );
        if ( obj->carried_by != NULL )
            obj_to_char( clone, ch );
        else
            obj_to_room( clone, ch->in_room );
        recursive_clone( ch, obj, clone );

        act( "$n has created $p.", ch, clone, NULL, TO_ROOM );
        act( "You clone $p.", ch, clone, NULL, TO_CHAR );
        wiznet( "$N clones $p.", ch, clone, WIZ_LOAD, WIZ_SECURE, get_trust( ch ) );
        return;
    }
    else if ( mob != NULL )
    {
        CHAR_DATA              *clone = NULL;
        OBJ_DATA               *cloned_obj = NULL;

        if ( !IS_NPC( mob ) )
        {
            ch_printf( ch, "You can only clone mobiles.\r\n" );
            return;
        }

        if ( ( mob->level > 20 && !IS_TRUSTED( ch, GOD ) )
             || ( mob->level > 10 && !IS_TRUSTED( ch, IMMORTAL ) )
             || ( mob->level > 5 && !IS_TRUSTED( ch, DEMI ) )
             || ( mob->level > 0 && !IS_TRUSTED( ch, ANGEL ) )
             || !IS_TRUSTED( ch, AVATAR ) )
        {
            ch_printf( ch, "Your powers are not great enough for such a task.\r\n" );
            return;
        }

        clone = create_mobile( mob->pIndexData );
        clone_mobile( mob, clone );

        for ( obj = mob->carrying; obj != NULL; obj = obj->next_content )
        {
            if ( obj_check( ch, obj ) )
            {
                cloned_obj = create_object( obj->pIndexData, 0 );
                clone_object( obj, cloned_obj );
                recursive_clone( ch, obj, cloned_obj );
                obj_to_char( cloned_obj, clone );
                cloned_obj->wear_loc = obj->wear_loc;
            }
        }
        char_to_room( clone, ch->in_room );
        act( "$n has created $N.", ch, NULL, clone, TO_ROOM );
        act( "You clone $N.", ch, NULL, clone, TO_CHAR );
        wiz_printf( ch, NULL, WIZ_LOAD, WIZ_SECURE, get_trust( ch ),
                    "$N clones %s.", NAME( clone ) );
        return;
    }
}

/* RT to replace the two load commands */
void do_load( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Syntax:\r\n" );
        ch_printf( ch, "  load mob <vnum>\r\n" );
        ch_printf( ch, "  load obj <vnum> <level>\r\n" );
        return;
    }

    if ( !str_cmp( arg, "mob" ) || !str_cmp( arg, "char" ) )
    {
        do_function( ch, &do_mload, argument );
        return;
    }

    if ( !str_cmp( arg, "obj" ) )
    {
        do_function( ch, &do_oload, argument );
        return;
    }
    /*
     * echo syntax 
     */
    do_function( ch, &do_load, "" );
}

void do_mload( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    MOB_INDEX_DATA         *pMobIndex = NULL;
    CHAR_DATA              *victim = NULL;

    one_argument( argument, arg );

    if ( arg[0] == '\0' || !is_number( arg ) )
    {
        ch_printf( ch, "Syntax: load mob <vnum>.\r\n" );
        return;
    }

    if ( ( pMobIndex = get_mob_index( atoi( arg ) ) ) == NULL )
    {
        ch_printf( ch, "No mob has that vnum.\r\n" );
        return;
    }

    victim = create_mobile( pMobIndex );
    char_to_room( victim, ch->in_room );
    act( "$n has created $N!", ch, NULL, victim, TO_ROOM );
    wiz_printf( ch, NULL, WIZ_LOAD, WIZ_SECURE, get_trust( ch ),
                "$N loads %s.", NAME( victim ) );
    ch_printf( ch, "Ok.\r\n" );
    return;
}

void do_oload( CHAR_DATA *ch, const char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_INDEX_DATA         *pObjIndex = NULL;
    OBJ_DATA               *obj = NULL;
    int                     level = 0;

    argument = one_argument( argument, arg1 );
    one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || !is_number( arg1 ) )
    {
        ch_printf( ch, "Syntax: load obj <vnum> <level>.\r\n" );
        return;
    }

    level = get_trust( ch );                           /* default */

    if ( arg2[0] != '\0' )                             /* load with a level */
    {
        if ( !is_number( arg2 ) )
        {
            ch_printf( ch, "Syntax: oload <vnum> <level>.\r\n" );
            return;
        }
        level = atoi( arg2 );
        if ( level < 0 || level > get_trust( ch ) )
        {
            ch_printf( ch, "Level must be be between 0 and your level.\r\n" );
            return;
        }
    }

    if ( ( pObjIndex = get_obj_index( atoi( arg1 ) ) ) == NULL )
    {
        ch_printf( ch, "No object has that vnum.\r\n" );
        return;
    }

    obj = create_object( pObjIndex, level );
    if ( CAN_WEAR( obj, ITEM_TAKE ) )
        obj_to_char( obj, ch );
    else
        obj_to_room( obj, ch->in_room );
    act( "$n has created $p!", ch, obj, NULL, TO_ROOM );
    wiznet( "$N loads $p.", ch, obj, WIZ_LOAD, WIZ_SECURE, get_trust( ch ) );
    ch_printf( ch, "Ok.\r\n" );
    return;
}

void do_purge( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;
    OBJ_DATA               *obj = NULL;
    DESCRIPTOR_DATA        *d = NULL;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        /*
         * 'purge' 
         */
        CHAR_DATA              *vnext = NULL;
        OBJ_DATA               *obj_next = NULL;

        for ( victim = ch->in_room->people; victim != NULL; victim = vnext )
        {
            vnext = victim->next_in_room;
            if ( IS_NPC( victim ) && !IS_SET( victim->act, ACT_NOPURGE ) && victim != ch        /* safety 
                                                                                                 * precaution 
                                                                                                 */  )
                extract_char( victim, true );
        }

        for ( obj = ch->in_room->contents; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;
            if ( !IS_OBJ_STAT( obj, ITEM_NOPURGE ) )
                extract_obj( obj );
        }

        act( "$n purges the room!", ch, NULL, NULL, TO_ROOM );
        ch_printf( ch, "Ok.\r\n" );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        ch_printf( ch, "They aren't here.\r\n" );
        return;
    }

    if ( !IS_NPC( victim ) )
    {

        if ( ch == victim )
        {
            ch_printf( ch, "Ho ho ho.\r\n" );
            return;
        }

        if ( get_trust( ch ) <= get_trust( victim ) )
        {
            ch_printf( ch, "Maybe that wasn't a good idea...\r\n" );
            ch_printf( ch, "%s tried to purge you!\r\n", ch->name );
            return;
        }

        act( "$n disintegrates $N.", ch, 0, victim, TO_NOTVICT );

        if ( victim->level > 1 )
            save_char_obj( victim );
        d = victim->desc;
        update_player_list( victim, false );
        extract_char( victim, true );
        if ( d != NULL )
            close_descriptor( d );

        return;
    }

    act( "$n purges $N.", ch, NULL, victim, TO_NOTVICT );
    extract_char( victim, true );
    return;
}

void do_advance( CHAR_DATA *ch, const char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;
    int                     level = 0;
    int                     iLevel = 0;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || !is_number( arg2 ) )
    {
        ch_printf( ch, "Syntax: advance <char> <level>.\r\n" );
        return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        ch_printf( ch, "That player is not here.\r\n" );
        return;
    }

    if ( IS_NPC( victim ) )
    {
        ch_printf( ch, "Not on NPC's.\r\n" );
        return;
    }

    if ( ( level = atoi( arg2 ) ) < 1 || level > MAX_LEVEL )
    {
        ch_printf( ch, "Level must be 1 to %d.\r\n", MAX_LEVEL );
        return;
    }

    if ( level > get_trust( ch ) )
    {
        ch_printf( ch, "Limited to your trust level.\r\n" );
        return;
    }

    /*
     * Lower level:
     *   Reset to level 1.
     *   Then raise again.
     *   Currently, an imp can lower another imp.
     *   -- Swiftest
     */
    if ( level <= victim->level )
    {
        int                     temp_prac;

        ch_printf( ch, "Lowering a player's level!\r\n" );
        ch_printf( victim, "**** OOOOHHHHHHHHHH  NNNNOOOO ****\r\n" );
        temp_prac = victim->practice;
        victim->level = 1;
        victim->exp = exp_per_level( victim, victim->pcdata->points );
        victim->max_hit = 10;
        victim->max_mana = 100;
        victim->max_move = 100;
        victim->practice = 0;
        victim->hit = victim->max_hit;
        victim->mana = victim->max_mana;
        victim->move = victim->max_move;
        advance_level( victim, true );
        victim->practice = temp_prac;
    }
    else
    {
        ch_printf( ch, "Raising a player's level!\r\n" );
        ch_printf( victim, "**** OOOOHHHHHHHHHH  YYYYEEEESSS ****\r\n" );
    }

    for ( iLevel = victim->level; iLevel < level; iLevel++ )
    {
        victim->level += 1;
        advance_level( victim, true );
    }
    ch_printf( victim, "You are now level %d.\r\n", victim->level );
    victim->exp = exp_per_level( victim, victim->pcdata->points )
        * UMAX( 1, victim->level );
    victim->trust = 0;
    save_char_obj( victim );
    return;
}

void do_trust( CHAR_DATA *ch, const char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;
    int                     level = 0;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || !is_number( arg2 ) )
    {
        ch_printf( ch, "Syntax: trust <char> <level>.\r\n" );
        return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        ch_printf( ch, "That player is not here.\r\n" );
        return;
    }

    if ( ( level = atoi( arg2 ) ) < 0 || level > MAX_LEVEL )
    {
        ch_printf( ch, "Level must be 0 (reset) or 1 to %d.\r\n", MAX_LEVEL );
        return;
    }

    if ( level > get_trust( ch ) )
    {
        ch_printf( ch, "Limited to your trust.\r\n" );
        return;
    }

    victim->trust = level;
    return;
}

void do_restore( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;
    CHAR_DATA              *vch = NULL;
    DESCRIPTOR_DATA        *d = NULL;

    one_argument( argument, arg );
    if ( arg[0] == '\0' || !str_cmp( arg, "room" ) )
    {
        /*
         * cure room 
         */

        for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room )
        {
            affect_strip( vch, skill_lookup( "plague" ) );
            affect_strip( vch, skill_lookup( "poison" ) );
            affect_strip( vch, skill_lookup( "blindness" ) );
            affect_strip( vch, skill_lookup( "sleep" ) );
            affect_strip( vch, skill_lookup( "curse" ) );

            vch->hit = vch->max_hit;
            vch->mana = vch->max_mana;
            vch->move = vch->max_move;
            update_pos( vch );
            act( "$n has restored you.", ch, NULL, vch, TO_VICT );
        }

        wiz_printf( ch, NULL, WIZ_RESTORE, WIZ_SECURE, get_trust( ch ),
                    "$N restored room %d.", ch->in_room->vnum );

        ch_printf( ch, "Room restored.\r\n" );
        return;

    }

    if ( get_trust( ch ) >= MAX_LEVEL - 1 && !str_cmp( arg, "all" ) )
    {
        /*
         * cure all 
         */

        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            victim = d->character;

            if ( victim == NULL || IS_NPC( victim ) )
                continue;

            affect_strip( victim, skill_lookup( "plague" ) );
            affect_strip( victim, skill_lookup( "poison" ) );
            affect_strip( victim, skill_lookup( "blindness" ) );
            affect_strip( victim, skill_lookup( "sleep" ) );
            affect_strip( victim, skill_lookup( "curse" ) );

            victim->hit = victim->max_hit;
            victim->mana = victim->max_mana;
            victim->move = victim->max_move;
            update_pos( victim );
            if ( victim->in_room != NULL )
                act( "$n has restored you.", ch, NULL, victim, TO_VICT );
        }
        ch_printf( ch, "All active players restored.\r\n" );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        ch_printf( ch, "They aren't here.\r\n" );
        return;
    }

    affect_strip( victim, skill_lookup( "plague" ) );
    affect_strip( victim, skill_lookup( "poison" ) );
    affect_strip( victim, skill_lookup( "blindness" ) );
    affect_strip( victim, skill_lookup( "sleep" ) );
    affect_strip( victim, skill_lookup( "curse" ) );
    victim->hit = victim->max_hit;
    victim->mana = victim->max_mana;
    victim->move = victim->max_move;
    update_pos( victim );
    act( "$n has restored you.", ch, NULL, victim, TO_VICT );
    wiz_printf( ch, NULL, WIZ_RESTORE, WIZ_SECURE, get_trust( ch ),
                "$N restored %s", NAME( victim ) );
    ch_printf( ch, "Ok.\r\n" );
    return;
}

void do_freeze( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Freeze whom?\r\n" );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        ch_printf( ch, "They aren't here.\r\n" );
        return;
    }

    if ( IS_NPC( victim ) )
    {
        ch_printf( ch, "Not on NPC's.\r\n" );
        return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
        ch_printf( ch, "You failed.\r\n" );
        return;
    }

    if ( IS_SET( victim->act, PLR_FREEZE ) )
    {
        REMOVE_BIT( victim->act, PLR_FREEZE );
        ch_printf( victim, "You can play again.\r\n" );
        ch_printf( ch, "FREEZE removed.\r\n" );
        wiz_printf( ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0,
                    "$N thaws %s.", NAME( victim ) );
    }
    else
    {
        SET_BIT( victim->act, PLR_FREEZE );
        ch_printf( victim, "You can't do ANYthing!\r\n" );
        ch_printf( ch, "FREEZE set.\r\n" );
        wiz_printf( ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0,
                    "$N puts %s in the deep freeze.", NAME( victim ) );
    }

    save_char_obj( victim );

    return;
}

void do_log( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Log whom?\r\n" );
        return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
        if ( fLogAll )
        {
            fLogAll = false;
            ch_printf( ch, "Log ALL off.\r\n" );
        }
        else
        {
            fLogAll = true;
            ch_printf( ch, "Log ALL on.\r\n" );
        }
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        ch_printf( ch, "They aren't here.\r\n" );
        return;
    }

    if ( IS_NPC( victim ) )
    {
        ch_printf( ch, "Not on NPC's.\r\n" );
        return;
    }

    /*
     * No level check, gods can log anyone.
     */
    if ( IS_SET( victim->act, PLR_LOG ) )
    {
        REMOVE_BIT( victim->act, PLR_LOG );
        ch_printf( ch, "LOG removed.\r\n" );
    }
    else
    {
        SET_BIT( victim->act, PLR_LOG );
        ch_printf( ch, "LOG set.\r\n" );
    }

    return;
}

void do_noemote( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Noemote whom?\r\n" );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        ch_printf( ch, "They aren't here.\r\n" );
        return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
        ch_printf( ch, "You failed.\r\n" );
        return;
    }

    if ( IS_SET( victim->comm, COMM_NOEMOTE ) )
    {
        REMOVE_BIT( victim->comm, COMM_NOEMOTE );
        ch_printf( victim, "You can emote again.\r\n" );
        ch_printf( ch, "NOEMOTE removed.\r\n" );
        wiz_printf( ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0,
                    "$N restores emotes to %s.", NAME( victim ) );
    }
    else
    {
        SET_BIT( victim->comm, COMM_NOEMOTE );
        ch_printf( victim, "You can't emote!\r\n" );
        ch_printf( ch, "NOEMOTE set.\r\n" );
        wiz_printf( ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0,
                    "$N revokes %s's emotes.", NAME( victim ) );
    }

    return;
}

void do_noshout( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Noshout whom?\r\n" );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        ch_printf( ch, "They aren't here.\r\n" );
        return;
    }

    if ( IS_NPC( victim ) )
    {
        ch_printf( ch, "Not on NPC's.\r\n" );
        return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
        ch_printf( ch, "You failed.\r\n" );
        return;
    }

    if ( IS_SET( victim->comm, COMM_NOSHOUT ) )
    {
        REMOVE_BIT( victim->comm, COMM_NOSHOUT );
        ch_printf( victim, "You can shout again.\r\n" );
        ch_printf( ch, "NOSHOUT removed.\r\n" );
        wiz_printf( ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0,
                    "$N restores shouts to %s.", NAME( victim ) );
    }
    else
    {
        SET_BIT( victim->comm, COMM_NOSHOUT );
        ch_printf( victim, "You can't shout!\r\n" );
        ch_printf( ch, "NOSHOUT set.\r\n" );
        wiz_printf( ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0,
                    "$N revokes %s's shouts.", NAME( victim ) );
    }

    return;
}

void do_notell( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Notell whom?" );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        ch_printf( ch, "They aren't here.\r\n" );
        return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
        ch_printf( ch, "You failed.\r\n" );
        return;
    }

    if ( IS_SET( victim->comm, COMM_NOTELL ) )
    {
        REMOVE_BIT( victim->comm, COMM_NOTELL );
        ch_printf( victim, "You can tell again.\r\n" );
        ch_printf( ch, "NOTELL removed.\r\n" );
        wiz_printf( ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0,
                    "$N restores tells to %s.", NAME( victim ) );
    }
    else
    {
        SET_BIT( victim->comm, COMM_NOTELL );
        ch_printf( victim, "You can't tell!\r\n" );
        ch_printf( ch, "NOTELL set.\r\n" );
        wiz_printf( ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0,
                    "$N revokes %s's tells.", NAME( victim ) );
    }

    return;
}

void do_peace( CHAR_DATA *ch, const char *argument )
{
    CHAR_DATA              *rch = NULL;

    for ( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room )
    {
        if ( rch->fighting != NULL )
            stop_fighting( rch, true );
        if ( IS_NPC( rch ) && IS_SET( rch->act, ACT_AGGRESSIVE ) )
            REMOVE_BIT( rch->act, ACT_AGGRESSIVE );
    }

    ch_printf( ch, "Ok.\r\n" );
    return;
}

void do_wizlock( CHAR_DATA *ch, const char *argument )
{
    wizlock = !wizlock;

    if ( wizlock )
    {
        wiznet( "$N has wizlocked the game.", ch, NULL, 0, 0, 0 );
        ch_printf( ch, "Game wizlocked.\r\n" );
    }
    else
    {
        wiznet( "$N removes wizlock.", ch, NULL, 0, 0, 0 );
        ch_printf( ch, "Game un-wizlocked.\r\n" );
    }

    return;
}

/* RT anti-newbie code */
void do_newlock( CHAR_DATA *ch, const char *argument )
{
    newlock = !newlock;

    if ( newlock )
    {
        wiznet( "$N locks out new characters.", ch, NULL, 0, 0, 0 );
        ch_printf( ch, "New characters have been locked out.\r\n" );
    }
    else
    {
        wiznet( "$N allows new characters back in.", ch, NULL, 0, 0, 0 );
        ch_printf( ch, "Newlock removed.\r\n" );
    }

    return;
}

void do_slookup( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    int                     sn = -1;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Lookup which skill or spell?\r\n" );
        return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
        for ( sn = 0; sn < MAX_SKILL; sn++ )
        {
            if ( skill_table[sn].name == NULL )
                break;
            ch_printf( ch, "Sn: %3d  Slot: %3d  Skill/spell: '%s'\r\n",
                       sn, skill_table[sn].slot, skill_table[sn].name );
        }
    }
    else
    {
        if ( ( sn = skill_lookup( arg ) ) < 0 )
        {
            ch_printf( ch, "No such skill or spell.\r\n" );
            return;
        }

        ch_printf( ch, "Sn: %3d  Slot: %3d  Skill/spell: '%s'\r\n",
                   sn, skill_table[sn].slot, skill_table[sn].name );
    }

    return;
}

/* RT set replaces sset, mset, oset, and rset */
void do_set( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Syntax:\r\n" );
        ch_printf( ch, "  set mob   <name> <field> <value>\r\n" );
        ch_printf( ch, "  set obj   <name> <field> <value>\r\n" );
        ch_printf( ch, "  set room  <room> <field> <value>\r\n" );
        ch_printf( ch, "  set skill <name> <spell or skill> <value>\r\n" );
        return;
    }

    if ( !str_prefix( arg, "mobile" ) || !str_prefix( arg, "character" ) )
    {
        do_function( ch, &do_mset, argument );
        return;
    }

    if ( !str_prefix( arg, "skill" ) || !str_prefix( arg, "spell" ) )
    {
        do_function( ch, &do_sset, argument );
        return;
    }

    if ( !str_prefix( arg, "object" ) )
    {
        do_function( ch, &do_oset, argument );
        return;
    }

    if ( !str_prefix( arg, "room" ) )
    {
        do_function( ch, &do_rset, argument );
        return;
    }
    /*
     * echo syntax 
     */
    do_function( ch, &do_set, "" );
}

void do_sset( CHAR_DATA *ch, const char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg3[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;
    int                     value = 0;
    int                     sn = -1;
    bool                    fAll = false;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
        ch_printf( ch, "Syntax:\r\n" );
        ch_printf( ch, "  set skill <name> <spell or skill> <value>\r\n" );
        ch_printf( ch, "  set skill <name> all <value>\r\n" );
        ch_printf( ch, "   (use the name of the skill, not the number)\r\n" );
        return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        ch_printf( ch, "They aren't here.\r\n" );
        return;
    }

    if ( IS_NPC( victim ) )
    {
        ch_printf( ch, "Not on NPC's.\r\n" );
        return;
    }

    fAll = !str_cmp( arg2, "all" );
    sn = 0;
    if ( !fAll && ( sn = skill_lookup( arg2 ) ) < 0 )
    {
        ch_printf( ch, "No such skill or spell.\r\n" );
        return;
    }

    /*
     * Snarf the value.
     */
    if ( !is_number( arg3 ) )
    {
        ch_printf( ch, "Value must be numeric.\r\n" );
        return;
    }

    value = atoi( arg3 );
    if ( value < 0 || value > 100 )
    {
        ch_printf( ch, "Value range is 0 to 100.\r\n" );
        return;
    }

    if ( fAll )
    {
        for ( sn = 0; sn < MAX_SKILL; sn++ )
        {
            if ( skill_table[sn].name != NULL )
                victim->pcdata->learned[sn] = value;
        }
    }
    else
    {
        victim->pcdata->learned[sn] = value;
    }

    return;
}

void do_mset( CHAR_DATA *ch, const char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg3[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;
    int                     value = 0;
    char                    local_argument[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    const char             *lap = local_argument;

    strcpy( local_argument, argument );
    smash_tilde( local_argument );
    lap = one_argument( lap, arg1 );
    lap = one_argument( lap, arg2 );
    strcpy( arg3, lap );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
        ch_printf( ch, "Syntax:\r\n" );
        ch_printf( ch, "  set char <name> <field> <value>\r\n" );
        ch_printf( ch, "  Field being one of:\r\n" );
        ch_printf( ch, "    str int wis dex con sex class level\r\n" );
        ch_printf( ch, "    race group gold silver hp mana move prac\r\n" );
        ch_printf( ch, "    align train thirst hunger drunk full\r\n" );
        ch_printf( ch, "    security\r\n" );
        return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        ch_printf( ch, "They aren't here.\r\n" );
        return;
    }

    /*
     * clear zones for mobs 
     */
    victim->zone = NULL;

    /*
     * Snarf the value (which need not be numeric).
     */
    value = is_number( arg3 ) ? atoi( arg3 ) : -1;

    /*
     * Set something.
     */
    if ( !str_cmp( arg2, "str" ) )
    {
        if ( value < 3 || value > get_max_train( victim, STAT_STR ) )
        {
            ch_printf( ch,
                       "Strength range is 3 to %d\r\n.",
                       get_max_train( victim, STAT_STR ) );
            return;
        }

        victim->perm_stat[STAT_STR] = value;
        return;
    }

    if ( !str_cmp( arg2, "int" ) )
    {
        if ( value < 3 || value > get_max_train( victim, STAT_INT ) )
        {
            ch_printf( ch,
                       "Intelligence range is 3 to %d.\r\n",
                       get_max_train( victim, STAT_INT ) );
            return;
        }

        victim->perm_stat[STAT_INT] = value;
        return;
    }

    if ( !str_cmp( arg2, "wis" ) )
    {
        if ( value < 3 || value > get_max_train( victim, STAT_WIS ) )
        {
            ch_printf( ch,
                       "Wisdom range is 3 to %d.\r\n", get_max_train( victim,
                                                                      STAT_WIS ) );
            return;
        }

        victim->perm_stat[STAT_WIS] = value;
        return;
    }

    if ( !str_cmp( arg2, "dex" ) )
    {
        if ( value < 3 || value > get_max_train( victim, STAT_DEX ) )
        {
            ch_printf( ch,
                       "Dexterity range is 3 to %d.\r\n",
                       get_max_train( victim, STAT_DEX ) );
            return;
        }

        victim->perm_stat[STAT_DEX] = value;
        return;
    }

    if ( !str_cmp( arg2, "con" ) )
    {
        if ( value < 3 || value > get_max_train( victim, STAT_CON ) )
        {
            ch_printf( ch,
                       "Constitution range is 3 to %d.\r\n",
                       get_max_train( victim, STAT_CON ) );
            return;
        }

        victim->perm_stat[STAT_CON] = value;
        return;
    }

    if ( !str_prefix( arg2, "sex" ) )
    {
        if ( value < 0 || value > 2 )
        {
            ch_printf( ch, "Sex range is 0 to 2.\r\n" );
            return;
        }
        victim->sex = value;
        if ( !IS_NPC( victim ) )
            victim->pcdata->true_sex = value;
        return;
    }

    if ( !str_prefix( arg2, "class" ) )
    {
        int                     iclass = 0;

        if ( IS_NPC( victim ) )
        {
            ch_printf( ch, "Mobiles have no class.\r\n" );
            return;
        }

        iclass = class_lookup( arg3 );
        if ( iclass == -1 )
        {
            ch_printf( ch, "Possible classes are: " );
            for ( iclass = 0; iclass < MAX_CLASS; iclass++ )
            {
                ch_printf( ch, "%s%s", iclass > 0 ? " " : "", class_table[iclass].name );
            }
            ch_printf( ch, ".\r\n" );
            return;
        }

        victim->iclass = iclass;
        return;
    }

    if ( !str_prefix( arg2, "level" ) )
    {
        if ( !IS_NPC( victim ) )
        {
            ch_printf( ch, "Not on PC's.\r\n" );
            return;
        }

        if ( value < 0 || value > MAX_LEVEL )
        {
            ch_printf( ch, "Level range is 0 to %d.\r\n", MAX_LEVEL );
            return;
        }
        victim->level = value;
        return;
    }

    if ( !str_prefix( arg2, "gold" ) )
    {
        victim->gold = value;
        return;
    }

    if ( !str_prefix( arg2, "silver" ) )
    {
        victim->silver = value;
        return;
    }

    if ( !str_prefix( arg2, "hp" ) )
    {
        if ( value < -10 || value > 30000 )
        {
            ch_printf( ch, "Hp range is -10 to 30,000 hit points.\r\n" );
            return;
        }
        victim->max_hit = value;
        if ( !IS_NPC( victim ) )
            victim->pcdata->perm_hit = value;
        return;
    }

    if ( !str_prefix( arg2, "mana" ) )
    {
        if ( value < 0 || value > 30000 )
        {
            ch_printf( ch, "Mana range is 0 to 30,000 mana points.\r\n" );
            return;
        }
        victim->max_mana = value;
        if ( !IS_NPC( victim ) )
            victim->pcdata->perm_mana = value;
        return;
    }

    if ( !str_prefix( arg2, "move" ) )
    {
        if ( value < 0 || value > 30000 )
        {
            ch_printf( ch, "Move range is 0 to 30,000 move points.\r\n" );
            return;
        }
        victim->max_move = value;
        if ( !IS_NPC( victim ) )
            victim->pcdata->perm_move = value;
        return;
    }

    if ( !str_prefix( arg2, "practice" ) )
    {
        if ( value < 0 || value > 250 )
        {
            ch_printf( ch, "Practice range is 0 to 250 sessions.\r\n" );
            return;
        }
        victim->practice = value;
        return;
    }

    if ( !str_prefix( arg2, "train" ) )
    {
        if ( value < 0 || value > 50 )
        {
            ch_printf( ch, "Training session range is 0 to 50 sessions.\r\n" );
            return;
        }
        victim->train = value;
        return;
    }

    if ( !str_prefix( arg2, "align" ) )
    {
        if ( value < -1000 || value > 1000 )
        {
            ch_printf( ch, "Alignment range is -1000 to 1000.\r\n" );
            return;
        }
        victim->alignment = value;
        return;
    }

    if ( !str_prefix( arg2, "thirst" ) )
    {
        if ( IS_NPC( victim ) )
        {
            ch_printf( ch, "Not on NPC's.\r\n" );
            return;
        }

        if ( value < -1 || value > 100 )
        {
            ch_printf( ch, "Thirst range is -1 to 100.\r\n" );
            return;
        }

        victim->pcdata->condition[COND_THIRST] = value;
        return;
    }

    if ( !str_prefix( arg2, "drunk" ) )
    {
        if ( IS_NPC( victim ) )
        {
            ch_printf( ch, "Not on NPC's.\r\n" );
            return;
        }

        if ( value < -1 || value > 100 )
        {
            ch_printf( ch, "Drunk range is -1 to 100.\r\n" );
            return;
        }

        victim->pcdata->condition[COND_DRUNK] = value;
        return;
    }

    if ( !str_prefix( arg2, "full" ) )
    {
        if ( IS_NPC( victim ) )
        {
            ch_printf( ch, "Not on NPC's.\r\n" );
            return;
        }

        if ( value < -1 || value > 100 )
        {
            ch_printf( ch, "Full range is -1 to 100.\r\n" );
            return;
        }

        victim->pcdata->condition[COND_FULL] = value;
        return;
    }

    if ( !str_prefix( arg2, "hunger" ) )
    {
        if ( IS_NPC( victim ) )
        {
            ch_printf( ch, "Not on NPC's.\r\n" );
            return;
        }

        if ( value < -1 || value > 100 )
        {
            ch_printf( ch, "Full range is -1 to 100.\r\n" );
            return;
        }

        victim->pcdata->condition[COND_HUNGER] = value;
        return;
    }

    if ( !str_prefix( arg2, "race" ) )
    {
        int                     race;

        race = race_lookup( arg3 );

        if ( race == 0 )
        {
            ch_printf( ch, "That is not a valid race.\r\n" );
            return;
        }

        if ( !IS_NPC( victim ) && !race_table[race].pc_race )
        {
            ch_printf( ch, "That is not a valid player race.\r\n" );
            return;
        }

        victim->race = race;
        return;
    }

    if ( !str_prefix( arg2, "group" ) )
    {
        if ( !IS_NPC( victim ) )
        {
            ch_printf( ch, "Only on NPCs.\r\n" );
            return;
        }
        victim->group = value;
        return;
    }

    if ( !str_cmp( arg2, "security" ) )                /* OLC */
    {
        if ( IS_NPC( ch ) )
        {
            ch_printf( ch, "Yeah, sure.\r\n" );
            return;
        }

        if ( IS_NPC( victim ) )
        {
            ch_printf( ch, "Not on NPC's.\r\n" );
            return;
        }

        if ( value > ch->pcdata->security || value < 0 )
        {
            if ( ch->pcdata->security != 0 )
            {
                ch_printf( ch, "Valid security is 0-%d.\r\n", ch->pcdata->security );
            }
            else
            {
                ch_printf( ch, "Valid security is 0 only.\r\n" );
            }
            return;
        }
        victim->pcdata->security = value;
        return;
    }

    /*
     * Generate usage message.
     */
    do_function( ch, &do_mset, "" );
    return;
}

void do_string( CHAR_DATA *ch, const char *argument )
{
    char                    type[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg3[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;
    OBJ_DATA               *obj = NULL;
    char                    local_argument[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    const char             *lap = local_argument;

    strcpy( local_argument, argument );
    smash_tilde( local_argument );
    lap = one_argument( lap, type );
    lap = one_argument( lap, arg1 );
    lap = one_argument( lap, arg2 );
    strcpy( arg3, lap );

    if ( type[0] == '\0' || arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
        ch_printf( ch, "Syntax:\r\n" );
        ch_printf( ch, "  string char <name> <field> <string>\r\n" );
        ch_printf( ch, "    fields: name short long desc title spec\r\n" );
        ch_printf( ch, "  string obj  <name> <field> <string>\r\n" );
        ch_printf( ch, "    fields: name short long extended\r\n" );
        return;
    }

    if ( !str_prefix( type, "character" ) || !str_prefix( type, "mobile" ) )
    {
        if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
        {
            ch_printf( ch, "They aren't here.\r\n" );
            return;
        }

        /*
         * clear zone for mobs 
         */
        victim->zone = NULL;

        /*
         * string something 
         */

        if ( !str_prefix( arg2, "name" ) )
        {
            if ( !IS_NPC( victim ) )
            {
                ch_printf( ch, "Not on PC's.\r\n" );
                return;
            }
            free_string( victim->name );
            victim->name = str_dup( arg3 );
            return;
        }

        if ( !str_prefix( arg2, "description" ) )
        {
            free_string( victim->description );
            victim->description = str_dup( arg3 );
            return;
        }

        if ( !str_prefix( arg2, "short" ) )
        {
            free_string( victim->short_descr );
            victim->short_descr = str_dup( arg3 );
            return;
        }

        if ( !str_prefix( arg2, "long" ) )
        {
            free_string( victim->long_descr );
            strcat( arg3, "\r\n" );
            victim->long_descr = str_dup( arg3 );
            return;
        }

        if ( !str_prefix( arg2, "title" ) )
        {
            if ( IS_NPC( victim ) )
            {
                ch_printf( ch, "Not on NPC's.\r\n" );
                return;
            }

            set_title( victim, "%s", arg3 );
            return;
        }

        if ( !str_prefix( arg2, "spec" ) )
        {
            if ( !IS_NPC( victim ) )
            {
                ch_printf( ch, "Not on PC's.\r\n" );
                return;
            }

            if ( ( victim->spec_fun = spec_lookup( arg3 ) ) == 0 )
            {
                ch_printf( ch, "No such spec fun.\r\n" );
                return;
            }

            return;
        }
    }

    if ( !str_prefix( type, "object" ) )
    {
        /*
         * string an obj 
         */

        if ( ( obj = get_obj_world( ch, arg1 ) ) == NULL )
        {
            ch_printf( ch, "Nothing like that in heaven or earth.\r\n" );
            return;
        }

        if ( !str_prefix( arg2, "name" ) )
        {
            free_string( obj->name );
            obj->name = str_dup( arg3 );
            return;
        }

        if ( !str_prefix( arg2, "short" ) )
        {
            free_string( obj->short_descr );
            obj->short_descr = str_dup( arg3 );
            return;
        }

        if ( !str_prefix( arg2, "long" ) )
        {
            free_string( obj->description );
            obj->description = str_dup( arg3 );
            return;
        }

        if ( !str_prefix( arg2, "ed" ) || !str_prefix( arg2, "extended" ) )
        {
            EXTRA_DESCR_DATA       *ed;

            lap = one_argument( lap, arg3 );
            if ( lap == NULL )
            {
                ch_printf( ch, "Syntax: oset <object> ed <keyword> <string>\r\n" );
                return;
            }

            strcat( local_argument, "\r\n" );          /* This should still work, since
                                                        * lap is read only */

            ed = new_extra_descr(  );

            ed->keyword = str_dup( arg3 );
            ed->description = str_dup( lap );
            ed->next = obj->extra_descr;
            obj->extra_descr = ed;
            return;
        }
    }

    /*
     * echo bad use message 
     */
    do_function( ch, &do_string, "" );
}

void do_oset( CHAR_DATA *ch, const char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg3[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;
    int                     value = 0;
    char                    local_argument[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    const char             *lap = local_argument;

    strcpy( local_argument, argument );
    smash_tilde( local_argument );
    lap = one_argument( lap, arg1 );
    lap = one_argument( lap, arg2 );
    strcpy( arg3, lap );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
        ch_printf( ch, "Syntax:\r\n" );
        ch_printf( ch, "  set obj <object> <field> <value>\r\n" );
        ch_printf( ch, "  Field being one of:\r\n" );
        ch_printf( ch, "    value0 value1 value2 value3 value4 (v1-v4)\r\n" );
        ch_printf( ch, "    extra wear level weight cost timer\r\n" );
        return;
    }

    if ( ( obj = get_obj_world( ch, arg1 ) ) == NULL )
    {
        ch_printf( ch, "Nothing like that in heaven or earth.\r\n" );
        return;
    }

    /*
     * Snarf the value (which need not be numeric).
     */
    value = atoi( arg3 );

    /*
     * Set something.
     */
    if ( !str_cmp( arg2, "value0" ) || !str_cmp( arg2, "v0" ) )
    {
        obj->value[0] = UMIN( 50, value );
        return;
    }

    if ( !str_cmp( arg2, "value1" ) || !str_cmp( arg2, "v1" ) )
    {
        obj->value[1] = value;
        return;
    }

    if ( !str_cmp( arg2, "value2" ) || !str_cmp( arg2, "v2" ) )
    {
        obj->value[2] = value;
        return;
    }

    if ( !str_cmp( arg2, "value3" ) || !str_cmp( arg2, "v3" ) )
    {
        obj->value[3] = value;
        return;
    }

    if ( !str_cmp( arg2, "value4" ) || !str_cmp( arg2, "v4" ) )
    {
        obj->value[4] = value;
        return;
    }

    if ( !str_prefix( arg2, "extra" ) )
    {
        obj->extra_flags = value;
        return;
    }

    if ( !str_prefix( arg2, "wear" ) )
    {
        obj->wear_flags = value;
        return;
    }

    if ( !str_prefix( arg2, "level" ) )
    {
        obj->level = value;
        return;
    }

    if ( !str_prefix( arg2, "weight" ) )
    {
        obj->weight = value;
        return;
    }

    if ( !str_prefix( arg2, "cost" ) )
    {
        obj->cost = value;
        return;
    }

    if ( !str_prefix( arg2, "timer" ) )
    {
        obj->timer = value;
        return;
    }

    /*
     * Generate usage message.
     */
    do_function( ch, &do_oset, "" );
    return;
}

void do_rset( CHAR_DATA *ch, const char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg3[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    ROOM_INDEX_DATA        *location = NULL;
    int                     value = 0;
    char                    local_argument[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    const char             *lap = local_argument;

    strcpy( local_argument, argument );
    smash_tilde( local_argument );
    lap = one_argument( lap, arg1 );
    lap = one_argument( lap, arg2 );
    strcpy( arg3, lap );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
        ch_printf( ch, "Syntax:\r\n" );
        ch_printf( ch, "  set room <location> <field> <value>\r\n" );
        ch_printf( ch, "  Field being one of:\r\n" );
        ch_printf( ch, "    flags sector\r\n" );
        return;
    }

    if ( ( location = find_location( ch, arg1 ) ) == NULL )
    {
        ch_printf( ch, "No such location.\r\n" );
        return;
    }

    if ( !is_room_owner( ch, location ) && ch->in_room != location
         && room_is_private( location ) && !IS_TRUSTED( ch, IMPLEMENTOR ) )
    {
        ch_printf( ch, "That room is private right now.\r\n" );
        return;
    }

    /*
     * Snarf the value.
     */
    if ( !is_number( arg3 ) )
    {
        ch_printf( ch, "Value must be numeric.\r\n" );
        return;
    }
    value = atoi( arg3 );

    /*
     * Set something.
     */
    if ( !str_prefix( arg2, "flags" ) )
    {
        location->room_flags = value;
        return;
    }

    if ( !str_prefix( arg2, "sector" ) )
    {
        location->sector_type = value;
        return;
    }

    /*
     * Generate usage message.
     */
    do_function( ch, &do_rset, "" );
    return;
}

void do_sockets( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    DESCRIPTOR_DATA        *d = NULL;
    int                     count = 0;

    one_argument( argument, arg );
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ( d->character != NULL && can_see( ch, d->character )
             && ( arg[0] == '\0' || is_name( arg, d->character->name )
                  || ( d->original && is_name( arg, d->original->name ) ) ) )
        {
            count++;
            page_printf( ch, "[%3d %2d] %s@%s\r\n",
                         d->socket,
                         d->connected,
                         d->original ? d->original->name :
                         d->character ? d->character->name : "(none)",
                         d->host );
        }
    }
    if ( count == 0 )
    {
        ch_printf( ch, "No one by that name is connected.\r\n" );
        return;
    }

    page_printf( ch, "%d user%s\r\n", count, count == 1 ? "" : "s" );
    return;
}

/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
void do_force( CHAR_DATA *ch, const char *argument )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
        ch_printf( ch, "Force whom to do what?\r\n" );
        return;
    }

    one_argument( argument, arg2 );

    if ( !str_cmp( arg2, "delete" ) || !str_cmp( arg2, "mob" ) )
    {
        ch_printf( ch, "That will NOT be done.\r\n" );
        return;
    }

    sprintf( buf, "$n forces you to '%s'.", argument );

    if ( !str_cmp( arg, "all" ) )
    {
        CHAR_DATA              *vch = NULL;
        CHAR_DATA              *vch_next = NULL;

        if ( get_trust( ch ) < MAX_LEVEL - 3 )
        {
            ch_printf( ch, "Not at your level!\r\n" );
            return;
        }

        for ( vch = char_list; vch != NULL; vch = vch_next )
        {
            vch_next = vch->next;

            if ( !IS_NPC( vch ) && get_trust( vch ) < get_trust( ch ) )
            {
                act( buf, ch, NULL, vch, TO_VICT );
                interpret( vch, argument );
            }
        }
    }
    else if ( !str_cmp( arg, "players" ) )
    {
        CHAR_DATA              *vch = NULL;
        CHAR_DATA              *vch_next = NULL;

        if ( get_trust( ch ) < MAX_LEVEL - 2 )
        {
            ch_printf( ch, "Not at your level!\r\n" );
            return;
        }

        for ( vch = char_list; vch != NULL; vch = vch_next )
        {
            vch_next = vch->next;

            if ( !IS_NPC( vch ) && get_trust( vch ) < get_trust( ch )
                 && vch->level < LEVEL_HERO )
            {
                act( buf, ch, NULL, vch, TO_VICT );
                interpret( vch, argument );
            }
        }
    }
    else if ( !str_cmp( arg, "gods" ) )
    {
        CHAR_DATA              *vch = NULL;
        CHAR_DATA              *vch_next = NULL;

        if ( get_trust( ch ) < MAX_LEVEL - 2 )
        {
            ch_printf( ch, "Not at your level!\r\n" );
            return;
        }

        for ( vch = char_list; vch != NULL; vch = vch_next )
        {
            vch_next = vch->next;

            if ( !IS_NPC( vch ) && get_trust( vch ) < get_trust( ch )
                 && vch->level >= LEVEL_HERO )
            {
                act( buf, ch, NULL, vch, TO_VICT );
                interpret( vch, argument );
            }
        }
    }
    else
    {
        CHAR_DATA              *victim = NULL;

        if ( ( victim = get_char_world( ch, arg ) ) == NULL )
        {
            ch_printf( ch, "They aren't here.\r\n" );
            return;
        }

        if ( victim == ch )
        {
            ch_printf( ch, "Aye aye, right away!\r\n" );
            return;
        }

        if ( !is_room_owner( ch, victim->in_room )
             && ch->in_room != victim->in_room
             && room_is_private( victim->in_room ) && !IS_TRUSTED( ch, IMPLEMENTOR ) )
        {
            ch_printf( ch, "That character is in a private room.\r\n" );
            return;
        }

        if ( get_trust( victim ) >= get_trust( ch ) )
        {
            ch_printf( ch, "Do it yourself!\r\n" );
            return;
        }

        if ( !IS_NPC( victim ) && get_trust( ch ) < MAX_LEVEL - 3 )
        {
            ch_printf( ch, "Not at your level!\r\n" );
            return;
        }

        act( buf, ch, NULL, victim, TO_VICT );
        interpret( victim, argument );
    }

    ch_printf( ch, "Ok.\r\n" );
    return;
}

/*
 * New routines by Dionysos.
 */
void do_invis( CHAR_DATA *ch, const char *argument )
{
    int                     level = 0;
    char                    arg[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    /*
     * RT code for taking a level argument 
     */
    one_argument( argument, arg );

    if ( arg[0] == '\0' )
        /*
         * take the default path 
         */

        if ( ch->invis_level )
        {
            ch->invis_level = 0;
            act( "$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM );
            ch_printf( ch, "You slowly fade back into existence.\r\n" );
        }
        else
        {
            ch->invis_level = get_trust( ch );
            act( "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
            ch_printf( ch, "You slowly vanish into thin air.\r\n" );
        }
    else
        /*
         * do the level thing 
         */
    {
        level = atoi( arg );
        if ( level < 2 || level > get_trust( ch ) )
        {
            ch_printf( ch, "Invis level must be between 2 and your level.\r\n" );
            return;
        }
        else
        {
            ch->reply = NULL;
            ch->invis_level = level;
            act( "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
            ch_printf( ch, "You slowly vanish into thin air.\r\n" );
        }
    }

    return;
}

void do_incognito( CHAR_DATA *ch, const char *argument )
{
    int                     level = 0;
    char                    arg[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    /*
     * RT code for taking a level argument 
     */
    one_argument( argument, arg );

    if ( arg[0] == '\0' )
        /*
         * take the default path 
         */

        if ( ch->incog_level )
        {
            ch->incog_level = 0;
            act( "$n is no longer cloaked.", ch, NULL, NULL, TO_ROOM );
            ch_printf( ch, "You are no longer cloaked.\r\n" );
        }
        else
        {
            ch->incog_level = get_trust( ch );
            act( "$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM );
            ch_printf( ch, "You cloak your presence.\r\n" );
        }
    else
        /*
         * do the level thing 
         */
    {
        level = atoi( arg );
        if ( level < 2 || level > get_trust( ch ) )
        {
            ch_printf( ch, "Incog level must be between 2 and your level.\r\n" );
            return;
        }
        else
        {
            ch->reply = NULL;
            ch->incog_level = level;
            act( "$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM );
            ch_printf( ch, "You cloak your presence.\r\n" );
        }
    }

    return;
}

void do_holylight( CHAR_DATA *ch, const char *argument )
{
    if ( IS_NPC( ch ) )
        return;

    if ( IS_SET( ch->act, PLR_HOLYLIGHT ) )
    {
        REMOVE_BIT( ch->act, PLR_HOLYLIGHT );
        ch_printf( ch, "Holy light mode off.\r\n" );
    }
    else
    {
        SET_BIT( ch->act, PLR_HOLYLIGHT );
        ch_printf( ch, "Holy light mode on.\r\n" );
    }

    return;
}

/* prefix command: it will put the string typed on each line typed */
void do_prefi( CHAR_DATA *ch, const char *argument )
{
    ch_printf( ch, "You cannot abbreviate the prefix command.\r\n" );
    return;
}

void do_prefix( CHAR_DATA *ch, const char *argument )
{
    char                    buf[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";

    if ( argument[0] == '\0' )
    {
        if ( ch->prefix[0] == '\0' )
        {
            ch_printf( ch, "You have no prefix to clear.\r\n" );
            return;
        }

        ch_printf( ch, "Prefix removed.\r\n" );
        free_string( ch->prefix );
        ch->prefix = str_dup( "" );
        return;
    }

    if ( ch->prefix[0] != '\0' )
    {
        sprintf( buf, "Prefix changed to %s.\r\n", argument );
        free_string( ch->prefix );
    }
    else
    {
        sprintf( buf, "Prefix set to %s.\r\n", argument );
    }

    ch->prefix = str_dup( argument );
}
