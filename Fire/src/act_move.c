/*
 * RAM $Id: act_move.c 67 2009-01-05 00:39:32Z quixadhal $
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
#include "strings.h"
#include "random.h"
#include "db.h"
#include "interp.h"
#include "magic.h"
#include "tables.h"
#include "act.h"

const char             *dir_name[] = {
    "north", "east", "south", "west", "up", "down"
};
const char             *dir_abbrev[] = { "N", "E", "S", "W", "U", "D" };

const int               rev_dir[] = {
    DIR_SOUTH,
    DIR_WEST,
    DIR_NORTH,
    DIR_EAST,
    DIR_DOWN,
    DIR_UP
};

const int               movement_loss[SECT_MAX] = {
    1,                                                 /* SECT_INSIDE */
    2,                                                 /* SECT_CITY */
    2,                                                 /* SECT_FIELD */
    3,                                                 /* SECT_FOREST */
    4,                                                 /* SECT_HILLS */
    6,                                                 /* SECT_MOUNTAIN */
    4,                                                 /* SECT_WATER_SWIM */
    1,                                                 /* SECT_WATER_NOSWIM */
    6,                                                 /* SECT_UNUSED */
    10,                                                /* SECT_AIR */
    6                                                  /* SECT_DESERT */
};

void move_char( CHAR_DATA *ch, int door, bool follow )
{
    CHAR_DATA              *fch = NULL;
    CHAR_DATA              *fch_next = NULL;
    ROOM_INDEX_DATA        *in_room = NULL;
    ROOM_INDEX_DATA        *to_room = NULL;
    EXIT_DATA              *pexit = NULL;

    if ( door < 0 || door >= MAX_EXIT )
    {
        log_error( "%s attempting to use bad door %d", NAME( ch ), door );
        return;
    }

    /*
     * This should never trigger for PC's, so if it does, fail now!
     * Exit trigger, if activated, bail out. Only PCs are triggered.
     */
    if ( !IS_NPC( ch ) && mp_exit_trigger( ch, door ) )
    {
        log_error( "%s hit an mp_exit_trigger on door %d!", NAME( ch ), door );
        return;
    }

    in_room = ch->in_room;
    if ( ( pexit = in_room->exit[door] ) == NULL
         || ( to_room = pexit->u1.to_room ) == NULL
         || !can_see_room( ch, pexit->u1.to_room ) )
    {
        ch_printf( ch, "Alas, you cannot go that way.\r\n" );
        return;
    }

    if ( IS_SET( pexit->exit_info, EX_CLOSED )
         && ( !IS_AFFECTED( ch, AFF_PASS_DOOR ) || IS_SET( pexit->exit_info, EX_NOPASS ) )
         && !IS_TRUSTED( ch, ANGEL ) )
    {
        act( "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_CHARM )
         && ch->master != NULL && in_room == ch->master->in_room )
    {
        ch_printf( ch, "What?  And leave your beloved master?\r\n" );
        return;
    }

    if ( !is_room_owner( ch, to_room ) && room_is_private( to_room ) )
    {
        ch_printf( ch, "That room is private right now.\r\n" );
        return;
    }

    if ( !IS_NPC( ch ) )
    {
        int                     iClass = 0;
        int                     iGuild = 0;
        int                     move = 0;

        for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
        {
            for ( iGuild = 0; iGuild < MAX_GUILD; iGuild++ )
            {
                if ( !IS_IMMORTAL( ch ) && iClass != ch->iclass
                     && to_room->vnum == class_table[iClass].guild[iGuild] )
                {
                    ch_printf( ch, "You aren't allowed in there.\r\n" );
                    return;
                }
            }
        }

        if ( in_room->sector_type == SECT_AIR || to_room->sector_type == SECT_AIR )
        {
            if ( !IS_AFFECTED( ch, AFF_FLYING ) && !IS_IMMORTAL( ch ) )
            {
                ch_printf( ch, "You can't fly.\r\n" );
                return;
            }
        }

        if ( ( in_room->sector_type == SECT_WATER_NOSWIM
               || to_room->sector_type == SECT_WATER_NOSWIM )
             && !IS_AFFECTED( ch, AFF_FLYING ) )
        {
            OBJ_DATA               *obj = NULL;
            bool                    found = false;

            /*
             * Look for a boat.
             */
            if ( IS_IMMORTAL( ch ) )
                found = true;

            for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
            {
                if ( obj->item_type == ITEM_BOAT )
                {
                    found = true;
                    break;
                }
            }
            if ( !found )
            {
                ch_printf( ch, "You need a boat to go there.\r\n" );
                return;
            }
        }

        move = movement_loss[UMIN( SECT_MAX - 1, in_room->sector_type )]
            + movement_loss[UMIN( SECT_MAX - 1, to_room->sector_type )];

        move /= 2;                                     /* i.e. the average */

        /*
         * conditional effects 
         */
        if ( IS_AFFECTED( ch, AFF_FLYING ) || IS_AFFECTED( ch, AFF_HASTE ) )
            move /= 2;

        if ( IS_AFFECTED( ch, AFF_SLOW ) )
            move *= 2;

        if ( ch->move < move )
        {
            ch_printf( ch, "You are too exhausted.\r\n" );
            return;
        }

        WAIT_STATE( ch, 1 );
        ch->move -= move;
    }

    if ( !IS_AFFECTED( ch, AFF_SNEAK ) && ch->invis_level < LEVEL_HERO )
        act( "$n leaves $T.", ch, NULL, dir_name[door], TO_ROOM );

    char_from_room( ch );
    char_to_room( ch, to_room );
    if ( !IS_AFFECTED( ch, AFF_SNEAK ) && ch->invis_level < LEVEL_HERO )
        act( "$n has arrived.", ch, NULL, NULL, TO_ROOM );

    do_function( ch, &do_look, "auto" );

    if ( in_room == to_room )                          /* no circular follows */
        return;

    for ( fch = in_room->people; fch != NULL; fch = fch_next )
    {
        fch_next = fch->next_in_room;

        if ( fch->master == ch && IS_AFFECTED( fch, AFF_CHARM )
             && fch->position < POS_STANDING )
            do_function( fch, &do_stand, "" );

        if ( fch->master == ch && fch->position == POS_STANDING
             && can_see_room( fch, to_room ) )
        {

            if ( IS_SET( ch->in_room->room_flags, ROOM_LAW )
                 && ( IS_NPC( fch ) && IS_SET( fch->act, ACT_AGGRESSIVE ) ) )
            {
                act( "You can't bring $N into the city.", ch, NULL, fch, TO_CHAR );
                act( "You aren't allowed in the city.", fch, NULL, NULL, TO_CHAR );
                continue;
            }

            act( "You follow $N.", fch, NULL, ch, TO_CHAR );
            move_char( fch, door, true );
        }
    }

    /*
     * If someone is following the char, these triggers get activated
     * for the followers before the char, but it's safer this way...
     */
    if ( IS_NPC( ch ) && HAS_TRIGGER( ch, TRIG_ENTRY ) )
        mp_percent_trigger( ch, NULL, NULL, NULL, TRIG_ENTRY );
    if ( !IS_NPC( ch ) )
        mp_greet_trigger( ch );

    return;
}

void do_north( CHAR_DATA *ch, const char *argument )
{
    move_char( ch, DIR_NORTH, false );
    return;
}

void do_east( CHAR_DATA *ch, const char *argument )
{
    move_char( ch, DIR_EAST, false );
    return;
}

void do_south( CHAR_DATA *ch, const char *argument )
{
    move_char( ch, DIR_SOUTH, false );
    return;
}

void do_west( CHAR_DATA *ch, const char *argument )
{
    move_char( ch, DIR_WEST, false );
    return;
}

void do_up( CHAR_DATA *ch, const char *argument )
{
    move_char( ch, DIR_UP, false );
    return;
}

void do_down( CHAR_DATA *ch, const char *argument )
{
    move_char( ch, DIR_DOWN, false );
    return;
}

int find_door( CHAR_DATA *ch, char *arg )
{
    EXIT_DATA              *pexit = NULL;
    int                     door = 0;

    if ( !str_cmp( arg, "n" ) || !str_cmp( arg, "north" ) )
        door = DIR_NORTH;
    else if ( !str_cmp( arg, "e" ) || !str_cmp( arg, "east" ) )
        door = DIR_EAST;
    else if ( !str_cmp( arg, "s" ) || !str_cmp( arg, "south" ) )
        door = DIR_SOUTH;
    else if ( !str_cmp( arg, "w" ) || !str_cmp( arg, "west" ) )
        door = DIR_WEST;
    else if ( !str_cmp( arg, "u" ) || !str_cmp( arg, "up" ) )
        door = DIR_UP;
    else if ( !str_cmp( arg, "d" ) || !str_cmp( arg, "down" ) )
        door = DIR_DOWN;
    else
    {
        for ( door = 0; door < MAX_EXIT; door++ )
        {
            if ( ( pexit = ch->in_room->exit[door] ) != NULL
                 && IS_SET( pexit->exit_info, EX_ISDOOR )
                 && pexit->keyword != NULL && is_name( arg, pexit->keyword ) )
                return door;
        }
        act( "I see no $T here.", ch, NULL, arg, TO_CHAR );
        return -1;
    }

    if ( ( pexit = ch->in_room->exit[door] ) == NULL )
    {
        act( "I see no door $T here.", ch, NULL, arg, TO_CHAR );
        return -1;
    }

    if ( !IS_SET( pexit->exit_info, EX_ISDOOR ) )
    {
        ch_printf( ch, "You can't do that.\r\n" );
        return -1;
    }

    return door;
}

void do_open( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;
    int                     door = 0;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Open what?\r\n" );
        return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
        /*
         * open portal 
         */
        if ( obj->item_type == ITEM_PORTAL )
        {
            if ( !IS_SET( obj->value[1], EX_ISDOOR ) )
            {
                ch_printf( ch, "You can't do that.\r\n" );
                return;
            }

            if ( !IS_SET( obj->value[1], EX_CLOSED ) )
            {
                ch_printf( ch, "It's already open.\r\n" );
                return;
            }

            if ( IS_SET( obj->value[1], EX_LOCKED ) )
            {
                ch_printf( ch, "It's locked.\r\n" );
                return;
            }

            REMOVE_BIT( obj->value[1], EX_CLOSED );
            act( "You open $p.", ch, obj, NULL, TO_CHAR );
            act( "$n opens $p.", ch, obj, NULL, TO_ROOM );
            return;
        }

        /*
         * 'open object' 
         */
        if ( obj->item_type != ITEM_CONTAINER )
        {
            ch_printf( ch, "That's not a container.\r\n" );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_CLOSED ) )
        {
            ch_printf( ch, "It's already open.\r\n" );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_CLOSEABLE ) )
        {
            ch_printf( ch, "You can't do that.\r\n" );
            return;
        }
        if ( IS_SET( obj->value[1], CONT_LOCKED ) )
        {
            ch_printf( ch, "It's locked.\r\n" );
            return;
        }

        REMOVE_BIT( obj->value[1], CONT_CLOSED );
        act( "You open $p.", ch, obj, NULL, TO_CHAR );
        act( "$n opens $p.", ch, obj, NULL, TO_ROOM );
        return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
        /*
         * 'open door' 
         */
        ROOM_INDEX_DATA        *to_room;
        EXIT_DATA              *pexit;
        EXIT_DATA              *pexit_rev;

        pexit = ch->in_room->exit[door];
        if ( !IS_SET( pexit->exit_info, EX_CLOSED ) )
        {
            ch_printf( ch, "It's already open.\r\n" );
            return;
        }
        if ( IS_SET( pexit->exit_info, EX_LOCKED ) )
        {
            ch_printf( ch, "It's locked.\r\n" );
            return;
        }

        REMOVE_BIT( pexit->exit_info, EX_CLOSED );
        act( "$n opens the $d.", ch, NULL, pexit->keyword, TO_ROOM );
        ch_printf( ch, "Ok.\r\n" );

        /*
         * open the other side 
         */
        if ( ( to_room = pexit->u1.to_room ) != NULL
             && ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
             && pexit_rev->u1.to_room == ch->in_room )
        {
            CHAR_DATA              *rch;

            REMOVE_BIT( pexit_rev->exit_info, EX_CLOSED );
            for ( rch = to_room->people; rch != NULL; rch = rch->next_in_room )
                act( "The $d opens.", rch, NULL, pexit_rev->keyword, TO_CHAR );
        }
    }

    return;
}

void do_close( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;
    int                     door = 0;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Close what?\r\n" );
        return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
        /*
         * portal stuff 
         */
        if ( obj->item_type == ITEM_PORTAL )
        {

            if ( !IS_SET( obj->value[1], EX_ISDOOR )
                 || IS_SET( obj->value[1], EX_NOCLOSE ) )
            {
                ch_printf( ch, "You can't do that.\r\n" );
                return;
            }

            if ( IS_SET( obj->value[1], EX_CLOSED ) )
            {
                ch_printf( ch, "It's already closed.\r\n" );
                return;
            }

            SET_BIT( obj->value[1], EX_CLOSED );
            act( "You close $p.", ch, obj, NULL, TO_CHAR );
            act( "$n closes $p.", ch, obj, NULL, TO_ROOM );
            return;
        }

        /*
         * 'close object' 
         */
        if ( obj->item_type != ITEM_CONTAINER )
        {
            ch_printf( ch, "That's not a container.\r\n" );
            return;
        }
        if ( IS_SET( obj->value[1], CONT_CLOSED ) )
        {
            ch_printf( ch, "It's already closed.\r\n" );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_CLOSEABLE ) )
        {
            ch_printf( ch, "You can't do that.\r\n" );
            return;
        }

        SET_BIT( obj->value[1], CONT_CLOSED );
        act( "You close $p.", ch, obj, NULL, TO_CHAR );
        act( "$n closes $p.", ch, obj, NULL, TO_ROOM );
        return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
        /*
         * 'close door' 
         */
        ROOM_INDEX_DATA        *to_room = NULL;
        EXIT_DATA              *pexit = NULL;
        EXIT_DATA              *pexit_rev = NULL;

        pexit = ch->in_room->exit[door];
        if ( IS_SET( pexit->exit_info, EX_CLOSED ) )
        {
            ch_printf( ch, "It's already closed.\r\n" );
            return;
        }

        SET_BIT( pexit->exit_info, EX_CLOSED );
        act( "$n closes the $d.", ch, NULL, pexit->keyword, TO_ROOM );
        ch_printf( ch, "Ok.\r\n" );

        /*
         * close the other side 
         */
        if ( ( to_room = pexit->u1.to_room ) != NULL
             && ( pexit_rev = to_room->exit[rev_dir[door]] ) != 0
             && pexit_rev->u1.to_room == ch->in_room )
        {
            CHAR_DATA              *rch;

            SET_BIT( pexit_rev->exit_info, EX_CLOSED );
            for ( rch = to_room->people; rch != NULL; rch = rch->next_in_room )
                act( "The $d closes.", rch, NULL, pexit_rev->keyword, TO_CHAR );
        }
    }

    return;
}

bool has_key( CHAR_DATA *ch, int key )
{
    OBJ_DATA               *obj = NULL;

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
        if ( obj->pIndexData->vnum == key )
            return true;
    }

    return false;
}

void do_lock( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;
    int                     door = 0;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Lock what?\r\n" );
        return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
        /*
         * portal stuff 
         */
        if ( obj->item_type == ITEM_PORTAL )
        {
            if ( !IS_SET( obj->value[1], EX_ISDOOR )
                 || IS_SET( obj->value[1], EX_NOCLOSE ) )
            {
                ch_printf( ch, "You can't do that.\r\n" );
                return;
            }
            if ( !IS_SET( obj->value[1], EX_CLOSED ) )
            {
                ch_printf( ch, "It's not closed.\r\n" );
                return;
            }

            if ( obj->value[4] < 0 || IS_SET( obj->value[1], EX_NOLOCK ) )
            {
                ch_printf( ch, "It can't be locked.\r\n" );
                return;
            }

            if ( !has_key( ch, obj->value[4] ) )
            {
                ch_printf( ch, "You lack the key.\r\n" );
                return;
            }

            if ( IS_SET( obj->value[1], EX_LOCKED ) )
            {
                ch_printf( ch, "It's already locked.\r\n" );
                return;
            }

            SET_BIT( obj->value[1], EX_LOCKED );
            act( "You lock $p.", ch, obj, NULL, TO_CHAR );
            act( "$n locks $p.", ch, obj, NULL, TO_ROOM );
            return;
        }

        /*
         * 'lock object' 
         */
        if ( obj->item_type != ITEM_CONTAINER )
        {
            ch_printf( ch, "That's not a container.\r\n" );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_CLOSED ) )
        {
            ch_printf( ch, "It's not closed.\r\n" );
            return;
        }
        if ( obj->value[2] < 0 )
        {
            ch_printf( ch, "It can't be locked.\r\n" );
            return;
        }
        if ( !has_key( ch, obj->value[2] ) )
        {
            ch_printf( ch, "You lack the key.\r\n" );
            return;
        }
        if ( IS_SET( obj->value[1], CONT_LOCKED ) )
        {
            ch_printf( ch, "It's already locked.\r\n" );
            return;
        }

        SET_BIT( obj->value[1], CONT_LOCKED );
        act( "You lock $p.", ch, obj, NULL, TO_CHAR );
        act( "$n locks $p.", ch, obj, NULL, TO_ROOM );
        return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
        /*
         * 'lock door' 
         */
        ROOM_INDEX_DATA        *to_room = NULL;
        EXIT_DATA              *pexit = NULL;
        EXIT_DATA              *pexit_rev = NULL;

        pexit = ch->in_room->exit[door];
        if ( !IS_SET( pexit->exit_info, EX_CLOSED ) )
        {
            ch_printf( ch, "It's not closed.\r\n" );
            return;
        }
        if ( pexit->key < 0 )
        {
            ch_printf( ch, "It can't be locked.\r\n" );
            return;
        }
        if ( !has_key( ch, pexit->key ) )
        {
            ch_printf( ch, "You lack the key.\r\n" );
            return;
        }
        if ( IS_SET( pexit->exit_info, EX_LOCKED ) )
        {
            ch_printf( ch, "It's already locked.\r\n" );
            return;
        }

        SET_BIT( pexit->exit_info, EX_LOCKED );
        ch_printf( ch, "*Click*\r\n" );
        act( "$n locks the $d.", ch, NULL, pexit->keyword, TO_ROOM );

        /*
         * lock the other side 
         */
        if ( ( to_room = pexit->u1.to_room ) != NULL
             && ( pexit_rev = to_room->exit[rev_dir[door]] ) != 0
             && pexit_rev->u1.to_room == ch->in_room )
        {
            SET_BIT( pexit_rev->exit_info, EX_LOCKED );
        }
    }

    return;
}

void do_unlock( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;
    int                     door = 0;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Unlock what?\r\n" );
        return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
        /*
         * portal stuff 
         */
        if ( obj->item_type == ITEM_PORTAL )
        {
            if ( !IS_SET( obj->value[1], EX_ISDOOR ) )
            {
                ch_printf( ch, "You can't do that.\r\n" );
                return;
            }

            if ( !IS_SET( obj->value[1], EX_CLOSED ) )
            {
                ch_printf( ch, "It's not closed.\r\n" );
                return;
            }

            if ( obj->value[4] < 0 )
            {
                ch_printf( ch, "It can't be unlocked.\r\n" );
                return;
            }

            if ( !has_key( ch, obj->value[4] ) )
            {
                ch_printf( ch, "You lack the key.\r\n" );
                return;
            }

            if ( !IS_SET( obj->value[1], EX_LOCKED ) )
            {
                ch_printf( ch, "It's already unlocked.\r\n" );
                return;
            }

            REMOVE_BIT( obj->value[1], EX_LOCKED );
            act( "You unlock $p.", ch, obj, NULL, TO_CHAR );
            act( "$n unlocks $p.", ch, obj, NULL, TO_ROOM );
            return;
        }

        /*
         * 'unlock object' 
         */
        if ( obj->item_type != ITEM_CONTAINER )
        {
            ch_printf( ch, "That's not a container.\r\n" );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_CLOSED ) )
        {
            ch_printf( ch, "It's not closed.\r\n" );
            return;
        }
        if ( obj->value[2] < 0 )
        {
            ch_printf( ch, "It can't be unlocked.\r\n" );
            return;
        }
        if ( !has_key( ch, obj->value[2] ) )
        {
            ch_printf( ch, "You lack the key.\r\n" );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_LOCKED ) )
        {
            ch_printf( ch, "It's already unlocked.\r\n" );
            return;
        }

        REMOVE_BIT( obj->value[1], CONT_LOCKED );
        act( "You unlock $p.", ch, obj, NULL, TO_CHAR );
        act( "$n unlocks $p.", ch, obj, NULL, TO_ROOM );
        return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
        /*
         * 'unlock door' 
         */
        ROOM_INDEX_DATA        *to_room = NULL;
        EXIT_DATA              *pexit = NULL;
        EXIT_DATA              *pexit_rev = NULL;

        pexit = ch->in_room->exit[door];
        if ( !IS_SET( pexit->exit_info, EX_CLOSED ) )
        {
            ch_printf( ch, "It's not closed.\r\n" );
            return;
        }
        if ( pexit->key < 0 )
        {
            ch_printf( ch, "It can't be unlocked.\r\n" );
            return;
        }
        if ( !has_key( ch, pexit->key ) )
        {
            ch_printf( ch, "You lack the key.\r\n" );
            return;
        }
        if ( !IS_SET( pexit->exit_info, EX_LOCKED ) )
        {
            ch_printf( ch, "It's already unlocked.\r\n" );
            return;
        }

        REMOVE_BIT( pexit->exit_info, EX_LOCKED );
        ch_printf( ch, "*Click*\r\n" );
        act( "$n unlocks the $d.", ch, NULL, pexit->keyword, TO_ROOM );

        /*
         * unlock the other side 
         */
        if ( ( to_room = pexit->u1.to_room ) != NULL
             && ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
             && pexit_rev->u1.to_room == ch->in_room )
        {
            REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
        }
    }

    return;
}

void do_pick( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *gch = NULL;
    OBJ_DATA               *obj = NULL;
    int                     door = 0;
    int                     sn = -1;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Pick what?\r\n" );
        return;
    }

    if ( ( sn = skill_lookup( "pick lock" ) ) == -1 )
    {
        log_error( "Can't find the \"%s\" skill in %s?", "pick lock", __FUNCTION__ );
        return;
    }

    WAIT_STATE( ch, skill_table[sn].beats );

    /*
     * look for guards 
     */
    for ( gch = ch->in_room->people; gch; gch = gch->next_in_room )
    {
        if ( IS_NPC( gch ) && IS_AWAKE( gch ) && ch->level + 5 < gch->level )
        {
            act( "$N is standing too close to the lock.", ch, NULL, gch, TO_CHAR );
            return;
        }
    }

    if ( !IS_NPC( ch ) && number_percent(  ) > get_skill( ch, sn ) )
    {
        ch_printf( ch, "You failed.\r\n" );
        check_improve( ch, sn, false, 2 );
        return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
        /*
         * portal stuff 
         */
        if ( obj->item_type == ITEM_PORTAL )
        {
            if ( !IS_SET( obj->value[1], EX_ISDOOR ) )
            {
                ch_printf( ch, "You can't do that.\r\n" );
                return;
            }

            if ( !IS_SET( obj->value[1], EX_CLOSED ) )
            {
                ch_printf( ch, "It's not closed.\r\n" );
                return;
            }

            if ( obj->value[4] < 0 )
            {
                ch_printf( ch, "It can't be unlocked.\r\n" );
                return;
            }

            if ( IS_SET( obj->value[1], EX_PICKPROOF ) )
            {
                ch_printf( ch, "You failed.\r\n" );
                return;
            }

            REMOVE_BIT( obj->value[1], EX_LOCKED );
            act( "You pick the lock on $p.", ch, obj, NULL, TO_CHAR );
            act( "$n picks the lock on $p.", ch, obj, NULL, TO_ROOM );
            check_improve( ch, sn, true, 2 );
            return;
        }

        /*
         * 'pick object' 
         */
        if ( obj->item_type != ITEM_CONTAINER )
        {
            ch_printf( ch, "That's not a container.\r\n" );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_CLOSED ) )
        {
            ch_printf( ch, "It's not closed.\r\n" );
            return;
        }
        if ( obj->value[2] < 0 )
        {
            ch_printf( ch, "It can't be unlocked.\r\n" );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_LOCKED ) )
        {
            ch_printf( ch, "It's already unlocked.\r\n" );
            return;
        }
        if ( IS_SET( obj->value[1], CONT_PICKPROOF ) )
        {
            ch_printf( ch, "You failed.\r\n" );
            return;
        }

        REMOVE_BIT( obj->value[1], CONT_LOCKED );
        act( "You pick the lock on $p.", ch, obj, NULL, TO_CHAR );
        act( "$n picks the lock on $p.", ch, obj, NULL, TO_ROOM );
        check_improve( ch, sn, true, 2 );
        return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
        /*
         * 'pick door' 
         */
        ROOM_INDEX_DATA        *to_room = NULL;
        EXIT_DATA              *pexit = NULL;
        EXIT_DATA              *pexit_rev = NULL;

        pexit = ch->in_room->exit[door];
        if ( !IS_SET( pexit->exit_info, EX_CLOSED ) && !IS_IMMORTAL( ch ) )
        {
            ch_printf( ch, "It's not closed.\r\n" );
            return;
        }
        if ( pexit->key < 0 && !IS_IMMORTAL( ch ) )
        {
            ch_printf( ch, "It can't be picked.\r\n" );
            return;
        }
        if ( !IS_SET( pexit->exit_info, EX_LOCKED ) )
        {
            ch_printf( ch, "It's already unlocked.\r\n" );
            return;
        }
        if ( IS_SET( pexit->exit_info, EX_PICKPROOF ) && !IS_IMMORTAL( ch ) )
        {
            ch_printf( ch, "You failed.\r\n" );
            return;
        }

        REMOVE_BIT( pexit->exit_info, EX_LOCKED );
        ch_printf( ch, "*Click*\r\n" );
        act( "$n picks the $d.", ch, NULL, pexit->keyword, TO_ROOM );
        check_improve( ch, sn, true, 2 );

        /*
         * pick the other side 
         */
        if ( ( to_room = pexit->u1.to_room ) != NULL
             && ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
             && pexit_rev->u1.to_room == ch->in_room )
        {
            REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
        }
    }

    return;
}

void do_stand( CHAR_DATA *ch, const char *argument )
{
    OBJ_DATA               *obj = NULL;

    if ( argument[0] != '\0' )
    {
        if ( ch->position == POS_FIGHTING )
        {
            ch_printf( ch, "Maybe you should finish fighting first?\r\n" );
            return;
        }
        obj = get_obj_list( ch, argument, ch->in_room->contents );
        if ( obj == NULL )
        {
            ch_printf( ch, "You don't see that here.\r\n" );
            return;
        }
        if ( obj->item_type != ITEM_FURNITURE
             || ( !IS_SET( obj->value[2], STAND_AT )
                  && !IS_SET( obj->value[2], STAND_ON )
                  && !IS_SET( obj->value[2], STAND_IN ) ) )
        {
            ch_printf( ch, "You can't seem to find a place to stand.\r\n" );
            return;
        }
        if ( ch->on != obj && count_users( obj ) >= obj->value[0] )
        {
            act_new( "There's no room to stand on $p.",
                     ch, obj, NULL, TO_CHAR, POS_DEAD );
            return;
        }
        ch->on = obj;
    }

    switch ( ch->position )
    {
        case POS_SLEEPING:
            if ( IS_AFFECTED( ch, AFF_SLEEP ) )
            {
                ch_printf( ch, "You can't wake up!\r\n" );
                return;
            }

            if ( obj == NULL )
            {
                ch_printf( ch, "You wake and stand up.\r\n" );
                act( "$n wakes and stands up.", ch, NULL, NULL, TO_ROOM );
                ch->on = NULL;
            }
            else if ( IS_SET( obj->value[2], STAND_AT ) )
            {
                act_new( "You wake and stand at $p.", ch, obj, NULL, TO_CHAR, POS_DEAD );
                act( "$n wakes and stands at $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], STAND_ON ) )
            {
                act_new( "You wake and stand on $p.", ch, obj, NULL, TO_CHAR, POS_DEAD );
                act( "$n wakes and stands on $p.", ch, obj, NULL, TO_ROOM );
            }
            else
            {
                act_new( "You wake and stand in $p.", ch, obj, NULL, TO_CHAR, POS_DEAD );
                act( "$n wakes and stands in $p.", ch, obj, NULL, TO_ROOM );
            }
            ch->position = POS_STANDING;
            do_function( ch, &do_look, "auto" );
            break;

        case POS_RESTING:
        case POS_SITTING:
            if ( obj == NULL )
            {
                ch_printf( ch, "You stand up.\r\n" );
                act( "$n stands up.", ch, NULL, NULL, TO_ROOM );
                ch->on = NULL;
            }
            else if ( IS_SET( obj->value[2], STAND_AT ) )
            {
                act( "You stand at $p.", ch, obj, NULL, TO_CHAR );
                act( "$n stands at $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], STAND_ON ) )
            {
                act( "You stand on $p.", ch, obj, NULL, TO_CHAR );
                act( "$n stands on $p.", ch, obj, NULL, TO_ROOM );
            }
            else
            {
                act( "You stand in $p.", ch, obj, NULL, TO_CHAR );
                act( "$n stands on $p.", ch, obj, NULL, TO_ROOM );
            }
            ch->position = POS_STANDING;
            break;

        case POS_STANDING:
            ch_printf( ch, "You are already standing.\r\n" );
            break;

        case POS_FIGHTING:
            ch_printf( ch, "You are already fighting!\r\n" );
            break;
    }

    return;
}

void do_rest( CHAR_DATA *ch, const char *argument )
{
    OBJ_DATA               *obj = NULL;

    if ( ch->position == POS_FIGHTING )
    {
        ch_printf( ch, "You are already fighting!\r\n" );
        return;
    }

    /*
     * okay, now that we know we can rest, find an object to rest on 
     */
    if ( argument[0] != '\0' )
    {
        obj = get_obj_list( ch, argument, ch->in_room->contents );
        if ( obj == NULL )
        {
            ch_printf( ch, "You don't see that here.\r\n" );
            return;
        }
    }
    else
        obj = ch->on;

    if ( obj != NULL )
    {
        if ( obj->item_type != ITEM_FURNITURE
             || ( !IS_SET( obj->value[2], REST_ON )
                  && !IS_SET( obj->value[2], REST_IN )
                  && !IS_SET( obj->value[2], REST_AT ) ) )
        {
            ch_printf( ch, "You can't rest on that.\r\n" );
            return;
        }

        if ( obj != NULL && ch->on != obj && count_users( obj ) >= obj->value[0] )
        {
            act_new( "There's no more room on $p.", ch, obj, NULL, TO_CHAR, POS_DEAD );
            return;
        }

        ch->on = obj;
    }

    switch ( ch->position )
    {
        case POS_SLEEPING:
            if ( IS_AFFECTED( ch, AFF_SLEEP ) )
            {
                ch_printf( ch, "You can't wake up!\r\n" );
                return;
            }

            if ( obj == NULL )
            {
                ch_printf( ch, "You wake up and start resting.\r\n" );
                act( "$n wakes up and starts resting.", ch, NULL, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], REST_AT ) )
            {
                act_new( "You wake up and rest at $p.",
                         ch, obj, NULL, TO_CHAR, POS_SLEEPING );
                act( "$n wakes up and rests at $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], REST_ON ) )
            {
                act_new( "You wake up and rest on $p.",
                         ch, obj, NULL, TO_CHAR, POS_SLEEPING );
                act( "$n wakes up and rests on $p.", ch, obj, NULL, TO_ROOM );
            }
            else
            {
                act_new( "You wake up and rest in $p.",
                         ch, obj, NULL, TO_CHAR, POS_SLEEPING );
                act( "$n wakes up and rests in $p.", ch, obj, NULL, TO_ROOM );
            }
            ch->position = POS_RESTING;
            break;

        case POS_RESTING:
            ch_printf( ch, "You are already resting.\r\n" );
            break;

        case POS_STANDING:
            if ( obj == NULL )
            {
                ch_printf( ch, "You rest.\r\n" );
                act( "$n sits down and rests.", ch, NULL, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], REST_AT ) )
            {
                act( "You sit down at $p and rest.", ch, obj, NULL, TO_CHAR );
                act( "$n sits down at $p and rests.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], REST_ON ) )
            {
                act( "You sit on $p and rest.", ch, obj, NULL, TO_CHAR );
                act( "$n sits on $p and rests.", ch, obj, NULL, TO_ROOM );
            }
            else
            {
                act( "You rest in $p.", ch, obj, NULL, TO_CHAR );
                act( "$n rests in $p.", ch, obj, NULL, TO_ROOM );
            }
            ch->position = POS_RESTING;
            break;

        case POS_SITTING:
            if ( obj == NULL )
            {
                ch_printf( ch, "You rest.\r\n" );
                act( "$n rests.", ch, NULL, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], REST_AT ) )
            {
                act( "You rest at $p.", ch, obj, NULL, TO_CHAR );
                act( "$n rests at $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], REST_ON ) )
            {
                act( "You rest on $p.", ch, obj, NULL, TO_CHAR );
                act( "$n rests on $p.", ch, obj, NULL, TO_ROOM );
            }
            else
            {
                act( "You rest in $p.", ch, obj, NULL, TO_CHAR );
                act( "$n rests in $p.", ch, obj, NULL, TO_ROOM );
            }
            ch->position = POS_RESTING;
            break;
    }

    return;
}

void do_sit( CHAR_DATA *ch, const char *argument )
{
    OBJ_DATA               *obj = NULL;

    if ( ch->position == POS_FIGHTING )
    {
        ch_printf( ch, "Maybe you should finish this fight first?\r\n" );
        return;
    }

    /*
     * okay, now that we know we can sit, find an object to sit on 
     */
    if ( argument[0] != '\0' )
    {
        obj = get_obj_list( ch, argument, ch->in_room->contents );
        if ( obj == NULL )
        {
            ch_printf( ch, "You don't see that here.\r\n" );
            return;
        }
    }
    else
        obj = ch->on;

    if ( obj != NULL )
    {
        if ( obj->item_type != ITEM_FURNITURE
             || ( !IS_SET( obj->value[2], SIT_ON )
                  && !IS_SET( obj->value[2], SIT_IN )
                  && !IS_SET( obj->value[2], SIT_AT ) ) )
        {
            ch_printf( ch, "You can't sit on that.\r\n" );
            return;
        }

        if ( obj != NULL && ch->on != obj && count_users( obj ) >= obj->value[0] )
        {
            act_new( "There's no more room on $p.", ch, obj, NULL, TO_CHAR, POS_DEAD );
            return;
        }

        ch->on = obj;
    }
    switch ( ch->position )
    {
        case POS_SLEEPING:
            if ( IS_AFFECTED( ch, AFF_SLEEP ) )
            {
                ch_printf( ch, "You can't wake up!\r\n" );
                return;
            }

            if ( obj == NULL )
            {
                ch_printf( ch, "You wake and sit up.\r\n" );
                act( "$n wakes and sits up.", ch, NULL, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SIT_AT ) )
            {
                act_new( "You wake and sit at $p.", ch, obj, NULL, TO_CHAR, POS_DEAD );
                act( "$n wakes and sits at $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SIT_ON ) )
            {
                act_new( "You wake and sit on $p.", ch, obj, NULL, TO_CHAR, POS_DEAD );
                act( "$n wakes and sits at $p.", ch, obj, NULL, TO_ROOM );
            }
            else
            {
                act_new( "You wake and sit in $p.", ch, obj, NULL, TO_CHAR, POS_DEAD );
                act( "$n wakes and sits in $p.", ch, obj, NULL, TO_ROOM );
            }

            ch->position = POS_SITTING;
            break;
        case POS_RESTING:
            if ( obj == NULL )
                ch_printf( ch, "You stop resting.\r\n" );
            else if ( IS_SET( obj->value[2], SIT_AT ) )
            {
                act( "You sit at $p.", ch, obj, NULL, TO_CHAR );
                act( "$n sits at $p.", ch, obj, NULL, TO_ROOM );
            }

            else if ( IS_SET( obj->value[2], SIT_ON ) )
            {
                act( "You sit on $p.", ch, obj, NULL, TO_CHAR );
                act( "$n sits on $p.", ch, obj, NULL, TO_ROOM );
            }
            ch->position = POS_SITTING;
            break;
        case POS_SITTING:
            ch_printf( ch, "You are already sitting down.\r\n" );
            break;
        case POS_STANDING:
            if ( obj == NULL )
            {
                ch_printf( ch, "You sit down.\r\n" );
                act( "$n sits down on the ground.", ch, NULL, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SIT_AT ) )
            {
                act( "You sit down at $p.", ch, obj, NULL, TO_CHAR );
                act( "$n sits down at $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SIT_ON ) )
            {
                act( "You sit on $p.", ch, obj, NULL, TO_CHAR );
                act( "$n sits on $p.", ch, obj, NULL, TO_ROOM );
            }
            else
            {
                act( "You sit down in $p.", ch, obj, NULL, TO_CHAR );
                act( "$n sits down in $p.", ch, obj, NULL, TO_ROOM );
            }
            ch->position = POS_SITTING;
            break;
    }
    return;
}

void do_sleep( CHAR_DATA *ch, const char *argument )
{
    OBJ_DATA               *obj = NULL;

    switch ( ch->position )
    {
        case POS_SLEEPING:
            ch_printf( ch, "You are already sleeping.\r\n" );
            break;

        case POS_RESTING:
        case POS_SITTING:
        case POS_STANDING:
            if ( argument[0] == '\0' && ch->on == NULL )
            {
                ch_printf( ch, "You go to sleep.\r\n" );
                act( "$n goes to sleep.", ch, NULL, NULL, TO_ROOM );
                ch->position = POS_SLEEPING;
            }
            else
            {
                /*
                 * find an object and sleep on it 
                 */
                if ( argument[0] == '\0' )
                    obj = ch->on;
                else
                    obj = get_obj_list( ch, argument, ch->in_room->contents );

                if ( obj == NULL )
                {
                    ch_printf( ch, "You don't see that here.\r\n" );
                    return;
                }
                if ( obj->item_type != ITEM_FURNITURE
                     || ( !IS_SET( obj->value[2], SLEEP_ON )
                          && !IS_SET( obj->value[2], SLEEP_IN )
                          && !IS_SET( obj->value[2], SLEEP_AT ) ) )
                {
                    ch_printf( ch, "You can't sleep on that!\r\n" );
                    return;
                }

                if ( ch->on != obj && count_users( obj ) >= obj->value[0] )
                {
                    act_new( "There is no room on $p for you.",
                             ch, obj, NULL, TO_CHAR, POS_DEAD );
                    return;
                }

                ch->on = obj;
                if ( IS_SET( obj->value[2], SLEEP_AT ) )
                {
                    act( "You go to sleep at $p.", ch, obj, NULL, TO_CHAR );
                    act( "$n goes to sleep at $p.", ch, obj, NULL, TO_ROOM );
                }
                else if ( IS_SET( obj->value[2], SLEEP_ON ) )
                {
                    act( "You go to sleep on $p.", ch, obj, NULL, TO_CHAR );
                    act( "$n goes to sleep on $p.", ch, obj, NULL, TO_ROOM );
                }
                else
                {
                    act( "You go to sleep in $p.", ch, obj, NULL, TO_CHAR );
                    act( "$n goes to sleep in $p.", ch, obj, NULL, TO_ROOM );
                }
                ch->position = POS_SLEEPING;
            }
            break;

        case POS_FIGHTING:
            ch_printf( ch, "You are already fighting!\r\n" );
            break;
    }

    return;
}

void do_wake( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        do_function( ch, &do_stand, "" );
        return;
    }

    if ( !IS_AWAKE( ch ) )
    {
        ch_printf( ch, "You are asleep yourself!\r\n" );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        ch_printf( ch, "They aren't here.\r\n" );
        return;
    }

    if ( IS_AWAKE( victim ) )
    {
        act( "$N is already awake.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( IS_AFFECTED( victim, AFF_SLEEP ) )
    {
        act( "You can't wake $M!", ch, NULL, victim, TO_CHAR );
        return;
    }

    act_new( "$n wakes you.", ch, NULL, victim, TO_VICT, POS_SLEEPING );
    do_function( ch, &do_stand, "" );
    return;
}

void do_sneak( CHAR_DATA *ch, const char *argument )
{
    AFFECT_DATA             af;
    int                     sn = -1;

    if ( ( sn = skill_lookup( "sneak" ) ) == -1 )
    {
        log_error( "Can't find the \"%s\" skill in %s?", "sneak", __FUNCTION__ );
        return;
    }

    ch_printf( ch, "You attempt to move silently.\r\n" );
    affect_strip( ch, sn );

    if ( IS_AFFECTED( ch, AFF_SNEAK ) )
        return;

    if ( number_percent(  ) < get_skill( ch, sn ) )
    {
        check_improve( ch, sn, true, 3 );
        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = ch->level;
        af.duration = ch->level;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = AFF_SNEAK;
        affect_to_char( ch, &af );
    }
    else
        check_improve( ch, sn, false, 3 );

    return;
}

void do_hide( CHAR_DATA *ch, const char *argument )
{
    int                     sn = -1;

    if ( ( sn = skill_lookup( "hide" ) ) == -1 )
    {
        log_error( "Can't find the \"%s\" skill in %s?", "hide", __FUNCTION__ );
        return;
    }

    ch_printf( ch, "You attempt to hide.\r\n" );

    if ( IS_AFFECTED( ch, AFF_HIDE ) )
        REMOVE_BIT( ch->affected_by, AFF_HIDE );

    if ( number_percent(  ) < get_skill( ch, sn ) )
    {
        SET_BIT( ch->affected_by, AFF_HIDE );
        check_improve( ch, sn, true, 3 );
    }
    else
        check_improve( ch, sn, false, 3 );

    return;
}

/*
 * Contributed by Alander.
 */
void do_visible( CHAR_DATA *ch, const char *argument )
{
    affect_strip( ch, skill_lookup( "invisibility" ) );
    affect_strip( ch, skill_lookup( "mass invis" ) );
    affect_strip( ch, skill_lookup( "sneak" ) );
    REMOVE_BIT( ch->affected_by, AFF_HIDE );
    REMOVE_BIT( ch->affected_by, AFF_INVISIBLE );
    REMOVE_BIT( ch->affected_by, AFF_SNEAK );
    ch_printf( ch, "Ok.\r\n" );
    return;
}

void do_recall( CHAR_DATA *ch, const char *argument )
{
    CHAR_DATA              *victim = NULL;
    ROOM_INDEX_DATA        *location = NULL;

    if ( IS_NPC( ch ) && !IS_SET( ch->act, ACT_PET ) )
    {
        ch_printf( ch, "Only players can recall.\r\n" );
        return;
    }

    act( "$n prays for transportation!", ch, 0, 0, TO_ROOM );

    if ( ( location = get_room_index( ROOM_VNUM_TEMPLE ) ) == NULL )
    {
        ch_printf( ch, "You are completely lost.\r\n" );
        return;
    }

    if ( ch->in_room == location )
        return;

    if ( IS_SET( ch->in_room->room_flags, ROOM_NO_RECALL )
         || IS_AFFECTED( ch, AFF_CURSE ) )
    {
        ch_printf( ch, "Mota has forsaken you.\r\n" );
        return;
    }

    if ( ( victim = ch->fighting ) != NULL )
    {
        int                     lose = 0;
        int                     skill = 0;
        int                     sn = -1;

        if ( ( sn = skill_lookup( "recall" ) ) == -1 )
        {
            log_error( "Can't find the \"%s\" skill in %s?", "recall", __FUNCTION__ );
            return;
        }

        skill = get_skill( ch, sn );

        if ( number_percent(  ) < 80 * skill / 100 )
        {
            check_improve( ch, sn, false, 6 );
            WAIT_STATE( ch, 4 );
            ch_printf( ch, "You failed!.\r\n" );
            return;
        }

        lose = ( ch->desc != NULL ) ? 25 : 50;
        gain_exp( ch, 0 - lose );
        check_improve( ch, sn, true, 4 );
        ch_printf( ch, "You recall from combat!  You lose %d exps.\r\n", lose );
        stop_fighting( ch, true );
    }

    ch->move /= 2;
    act( "$n disappears.", ch, NULL, NULL, TO_ROOM );
    char_from_room( ch );
    char_to_room( ch, location );
    act( "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
    do_function( ch, &do_look, "auto" );

    if ( ch->pet != NULL )
        do_function( ch->pet, &do_recall, "" );

    return;
}

void do_train( CHAR_DATA *ch, const char *argument )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *mob = NULL;
    int                     stat = -1;
    const char             *pOutput = NULL;
    int                     cost = 0;

    if ( IS_NPC( ch ) )
        return;

    /*
     * Check for trainer.
     */
    for ( mob = ch->in_room->people; mob; mob = mob->next_in_room )
    {
        if ( IS_NPC( mob ) && IS_SET( mob->act, ACT_TRAIN ) )
            break;
    }

    if ( mob == NULL )
    {
        ch_printf( ch, "You can't do that here.\r\n" );
        return;
    }

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "You have %d training sessions.\r\n", ch->train );
        argument = "foo";
    }

    cost = 1;

    if ( !str_cmp( argument, "str" ) )
    {
        if ( class_table[ch->iclass].attr_prime == STAT_STR )
            cost = 1;
        stat = STAT_STR;
        pOutput = "strength";
    }

    else if ( !str_cmp( argument, "int" ) )
    {
        if ( class_table[ch->iclass].attr_prime == STAT_INT )
            cost = 1;
        stat = STAT_INT;
        pOutput = "intelligence";
    }

    else if ( !str_cmp( argument, "wis" ) )
    {
        if ( class_table[ch->iclass].attr_prime == STAT_WIS )
            cost = 1;
        stat = STAT_WIS;
        pOutput = "wisdom";
    }

    else if ( !str_cmp( argument, "dex" ) )
    {
        if ( class_table[ch->iclass].attr_prime == STAT_DEX )
            cost = 1;
        stat = STAT_DEX;
        pOutput = "dexterity";
    }

    else if ( !str_cmp( argument, "con" ) )
    {
        if ( class_table[ch->iclass].attr_prime == STAT_CON )
            cost = 1;
        stat = STAT_CON;
        pOutput = "constitution";
    }

    else if ( !str_cmp( argument, "hp" ) )
        cost = 1;

    else if ( !str_cmp( argument, "mana" ) )
        cost = 1;

    else
    {
        strcpy( buf, "You can train:" );
        if ( ch->perm_stat[STAT_STR] < get_max_train( ch, STAT_STR ) )
            strcat( buf, " str" );
        if ( ch->perm_stat[STAT_INT] < get_max_train( ch, STAT_INT ) )
            strcat( buf, " int" );
        if ( ch->perm_stat[STAT_WIS] < get_max_train( ch, STAT_WIS ) )
            strcat( buf, " wis" );
        if ( ch->perm_stat[STAT_DEX] < get_max_train( ch, STAT_DEX ) )
            strcat( buf, " dex" );
        if ( ch->perm_stat[STAT_CON] < get_max_train( ch, STAT_CON ) )
            strcat( buf, " con" );
        strcat( buf, " hp mana" );

        if ( buf[strlen( buf ) - 1] != ':' )
        {
            ch_printf( ch, "%s.\r\n", buf );
        }
        else
        {
            /*
             * This message dedicated to Jordan ... you big stud!
             */
            act( "You have nothing left to train, you $T!",
                 ch, NULL,
                 ch->sex == SEX_MALE ? "big stud" :
                 ch->sex == SEX_FEMALE ? "hot babe" : "wild thing", TO_CHAR );
        }

        return;
    }

    if ( !str_cmp( "hp", argument ) )
    {
        if ( cost > ch->train )
        {
            ch_printf( ch, "You don't have enough training sessions.\r\n" );
            return;
        }

        ch->train -= cost;
        ch->pcdata->perm_hit += 10;
        ch->max_hit += 10;
        ch->hit += 10;
        act( "Your durability increases!", ch, NULL, NULL, TO_CHAR );
        act( "$n's durability increases!", ch, NULL, NULL, TO_ROOM );
        return;
    }

    if ( !str_cmp( "mana", argument ) )
    {
        if ( cost > ch->train )
        {
            ch_printf( ch, "You don't have enough training sessions.\r\n" );
            return;
        }

        ch->train -= cost;
        ch->pcdata->perm_mana += 10;
        ch->max_mana += 10;
        ch->mana += 10;
        act( "Your power increases!", ch, NULL, NULL, TO_CHAR );
        act( "$n's power increases!", ch, NULL, NULL, TO_ROOM );
        return;
    }

    if ( ch->perm_stat[stat] >= get_max_train( ch, stat ) )
    {
        act( "Your $T is already at maximum.", ch, NULL, pOutput, TO_CHAR );
        return;
    }

    if ( cost > ch->train )
    {
        ch_printf( ch, "You don't have enough training sessions.\r\n" );
        return;
    }

    ch->train -= cost;

    ch->perm_stat[stat] += 1;
    act( "Your $T increases!", ch, NULL, pOutput, TO_CHAR );
    act( "$n's $T increases!", ch, NULL, pOutput, TO_ROOM );
    return;
}

/* random room selection procedure */
ROOM_INDEX_DATA        *get_random_room( CHAR_DATA *ch )
{
    /*
     * top_room is the number of rooms loaded. room_index_hash[MAX_KEY_HASH] is the set
     * of rooms. Each room is stored in rih[vnum % MAX_KEY_HASH]. 
     */

    ROOM_INDEX_DATA       **pRooms = NULL;
    ROOM_INDEX_DATA        *pRoomIndex = NULL;
    int                     iHash = 0;
    int                     room_count = 0;
    int                     target_room = 0;

    if ( !
         ( pRooms =
           ( ROOM_INDEX_DATA ** ) calloc( top_room, sizeof( ROOM_INDEX_DATA * ) ) ) )
    {
        proper_exit( MUD_HALT,
                     "get_random_room: can't alloc %d pointers for room pointer table.",
                     top_room );
    }

    /*
     * First, we need to filter out rooms that aren't valid choices 
     */
    for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for ( pRoomIndex = room_index_hash[iHash];
              pRoomIndex != NULL; pRoomIndex = pRoomIndex->next )
        {
            /*
             * Skip private/safe rooms, no hiding in there! 
             */
            if ( IS_SET( pRoomIndex->room_flags, ROOM_PRIVATE )
                 || IS_SET( pRoomIndex->room_flags, ROOM_SOLITARY )
                 || IS_SET( pRoomIndex->room_flags, ROOM_SAFE ) )
                continue;

            /*
             * Skip rooms the target can't "see" 
             */
            if ( !can_see_room( ch, pRoomIndex ) )
                continue;

            /*
             * Skip rooms that are considered private beyond the flags 
             */
            if ( room_is_private( pRoomIndex ) )
                continue;

            /*
             * Skip rooms that are LAW if the target is a non-PK player 
             */
            if ( IS_SET( pRoomIndex->room_flags, ROOM_LAW )
                 && !IS_NPC( ch ) && !IS_SET( ch->act, ACT_AGGRESSIVE ) )
                continue;

            pRooms[room_count++] = pRoomIndex;
        }
    }

    /*
     * Now, we pick a random number and grab the room pointer 
     */
    target_room = number_range( 0, room_count );
    pRoomIndex = pRooms[target_room];
    free( pRooms );

    return pRoomIndex;
}

/* RT Enter portals */
void do_enter( CHAR_DATA *ch, const char *argument )
{
    ROOM_INDEX_DATA        *location = NULL;

    if ( ch->fighting != NULL )
        return;

    /*
     * nifty portal stuff 
     */
    if ( argument[0] != '\0' )
    {
        ROOM_INDEX_DATA        *old_room = NULL;
        OBJ_DATA               *portal = NULL;
        CHAR_DATA              *fch = NULL;
        CHAR_DATA              *fch_next = NULL;

        old_room = ch->in_room;
        portal = get_obj_list( ch, argument, ch->in_room->contents );

        if ( portal == NULL )
        {
            ch_printf( ch, "You don't see that here.\r\n" );
            return;
        }

        if ( portal->item_type != ITEM_PORTAL
             || ( IS_SET( portal->value[1], EX_CLOSED ) && !IS_TRUSTED( ch, ANGEL ) ) )
        {
            ch_printf( ch, "You can't seem to find a way in.\r\n" );
            return;
        }

        if ( !IS_TRUSTED( ch, ANGEL ) && !IS_SET( portal->value[2], GATE_NOCURSE )
             && ( IS_AFFECTED( ch, AFF_CURSE )
                  || IS_SET( old_room->room_flags, ROOM_NO_RECALL ) ) )
        {
            ch_printf( ch, "Something prevents you from leaving...\r\n" );
            return;
        }

        if ( IS_SET( portal->value[2], GATE_RANDOM ) || portal->value[3] == -1 )
        {
            location = get_random_room( ch );
            portal->value[3] = location->vnum;         /* for record keeping :) */
        }
        else if ( IS_SET( portal->value[2], GATE_BUGGY ) && ( number_percent(  ) < 5 ) )
            location = get_random_room( ch );
        else
            location = get_room_index( portal->value[3] );

        if ( location == NULL
             || location == old_room
             || !can_see_room( ch, location )
             || ( room_is_private( location ) && !IS_TRUSTED( ch, IMPLEMENTOR ) ) )
        {
            act( "$p doesn't seem to go anywhere.", ch, portal, NULL, TO_CHAR );
            return;
        }

        if ( IS_NPC( ch ) && IS_SET( ch->act, ACT_AGGRESSIVE )
             && IS_SET( location->room_flags, ROOM_LAW ) )
        {
            ch_printf( ch, "Something prevents you from leaving...\r\n" );
            return;
        }

        act( "$n steps into $p.", ch, portal, NULL, TO_ROOM );

        if ( IS_SET( portal->value[2], GATE_NORMAL_EXIT ) )
            act( "You enter $p.", ch, portal, NULL, TO_CHAR );
        else
            act( "You walk through $p and find yourself somewhere else...",
                 ch, portal, NULL, TO_CHAR );

        char_from_room( ch );
        char_to_room( ch, location );

        if ( IS_SET( portal->value[2], GATE_GOWITH ) ) /* take the gate along */
        {
            obj_from_room( portal );
            obj_to_room( portal, location );
        }

        if ( IS_SET( portal->value[2], GATE_NORMAL_EXIT ) )
            act( "$n has arrived.", ch, portal, NULL, TO_ROOM );
        else
            act( "$n has arrived through $p.", ch, portal, NULL, TO_ROOM );

        do_function( ch, &do_look, "auto" );

        /*
         * charges 
         */
        if ( portal->value[0] > 0 )
        {
            portal->value[0]--;
            if ( portal->value[0] == 0 )
                portal->value[0] = -1;
        }

        /*
         * protect against circular follows 
         */
        if ( old_room == location )
            return;

        for ( fch = old_room->people; fch != NULL; fch = fch_next )
        {
            fch_next = fch->next_in_room;

            if ( portal == NULL || portal->value[0] == -1 )
                /*
                 * no following through dead portals 
                 */
                continue;

            if ( fch->master == ch && IS_AFFECTED( fch, AFF_CHARM )
                 && fch->position < POS_STANDING )
                do_function( fch, &do_stand, "" );

            if ( fch->master == ch && fch->position == POS_STANDING )
            {

                if ( IS_SET( ch->in_room->room_flags, ROOM_LAW )
                     && ( IS_NPC( fch ) && IS_SET( fch->act, ACT_AGGRESSIVE ) ) )
                {
                    act( "You can't bring $N into the city.", ch, NULL, fch, TO_CHAR );
                    act( "You aren't allowed in the city.", fch, NULL, NULL, TO_CHAR );
                    continue;
                }

                act( "You follow $N.", fch, NULL, ch, TO_CHAR );
                do_function( fch, &do_enter, argument );
            }
        }

        if ( portal != NULL && portal->value[0] == -1 )
        {
            act( "$p fades out of existence.", ch, portal, NULL, TO_CHAR );
            if ( ch->in_room == old_room )
                act( "$p fades out of existence.", ch, portal, NULL, TO_ROOM );
            else if ( old_room->people != NULL )
            {
                act( "$p fades out of existence.",
                     old_room->people, portal, NULL, TO_CHAR );
                act( "$p fades out of existence.",
                     old_room->people, portal, NULL, TO_ROOM );
            }
            extract_obj( portal );
        }
        /*
         * If someone is following the char, these triggers get activated
         * for the followers before the char, but it's safer this way...
         */
        if ( IS_NPC( ch ) && HAS_TRIGGER( ch, TRIG_ENTRY ) )
            mp_percent_trigger( ch, NULL, NULL, NULL, TRIG_ENTRY );
        if ( !IS_NPC( ch ) )
            mp_greet_trigger( ch );

        return;
    }

    ch_printf( ch, "Nope, can't do it.\r\n" );
    return;
}
