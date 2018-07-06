/*
 * RAM $Id: act_obj.c 67 2009-01-05 00:39:32Z quixadhal $
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

/* RT part of the corpse looting code */

bool can_loot( CHAR_DATA *ch, OBJ_DATA *obj )
{
    CHAR_DATA              *owner = NULL;
    CHAR_DATA              *wch = NULL;

    if ( IS_IMMORTAL( ch ) )
        return true;

    if ( !obj->owner || obj->owner == NULL )
        return true;

    owner = NULL;
    for ( wch = char_list; wch != NULL; wch = wch->next )
        if ( !str_cmp( wch->name, obj->owner ) )
            owner = wch;

    if ( owner == NULL )
        return true;

    if ( !str_cmp( ch->name, owner->name ) )
        return true;

    if ( !IS_NPC( owner ) && IS_SET( owner->act, PLR_CANLOOT ) )
        return true;

    if ( is_same_group( ch, owner ) )
        return true;

    return false;
}

void get_obj( CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *container )
{
    /*
     * variables for AUTOSPLIT 
     */
    CHAR_DATA              *gch = NULL;
    int                     members = 0;
    char                    buffer[100] = "\0\0\0\0\0\0\0";

    if ( !CAN_WEAR( obj, ITEM_TAKE ) )
    {
        ch_printf( ch, "You can't take that.\r\n" );
        return;
    }

    if ( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) )
    {
        act( "$d: you can't carry that many items.", ch, NULL, obj->name, TO_CHAR );
        return;
    }

    if ( ( !obj->in_obj || obj->in_obj->carried_by != ch )
         && ( get_carry_weight( ch ) + get_obj_weight( obj ) > can_carry_w( ch ) ) )
    {
        act( "$d: you can't carry that much weight.", ch, NULL, obj->name, TO_CHAR );
        return;
    }

    if ( !can_loot( ch, obj ) )
    {
        act( "Corpse looting is not permitted.", ch, NULL, NULL, TO_CHAR );
        return;
    }

    if ( obj->in_room != NULL )
    {
        for ( gch = obj->in_room->people; gch != NULL; gch = gch->next_in_room )
            if ( gch->on == obj )
            {
                act( "$N appears to be using $p.", ch, obj, gch, TO_CHAR );
                return;
            }
    }

    if ( container != NULL )
    {
        if ( container->pIndexData->vnum == OBJ_VNUM_PIT && get_trust( ch ) < obj->level )
        {
            ch_printf( ch, "You are not powerful enough to use it.\r\n" );
            return;
        }

        if ( container->pIndexData->vnum == OBJ_VNUM_PIT
             && !CAN_WEAR( container, ITEM_TAKE ) && !IS_OBJ_STAT( obj, ITEM_HAD_TIMER ) )
            obj->timer = 0;
        act( "You get $p from $P.", ch, obj, container, TO_CHAR );
        act( "$n gets $p from $P.", ch, obj, container, TO_ROOM );
        REMOVE_BIT( obj->extra_flags, ITEM_HAD_TIMER );
        obj_from_obj( obj );
    }
    else
    {
        act( "You get $p.", ch, obj, container, TO_CHAR );
        act( "$n gets $p.", ch, obj, container, TO_ROOM );
        obj_from_room( obj );
    }

    if ( obj->item_type == ITEM_MONEY )
    {
        ch->silver += obj->value[0];
        ch->gold += obj->value[1];
        if ( IS_SET( ch->act, PLR_AUTOSPLIT ) )
        {                                              /* AUTOSPLIT code */
            for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
            {
                if ( !IS_AFFECTED( gch, AFF_CHARM ) && is_same_group( gch, ch ) )
                    members++;
            }

            if ( members > 1 && ( obj->value[0] > 1 || obj->value[1] ) )
            {
                sprintf( buffer, "%d %d", obj->value[0], obj->value[1] );
                do_function( ch, &do_split, buffer );
            }
        }

        extract_obj( obj );
    }
    else
    {
        obj_to_char( obj, ch );
    }

    return;
}

void do_get( CHAR_DATA *ch, const char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;
    OBJ_DATA               *obj_next = NULL;
    OBJ_DATA               *container = NULL;
    bool                    found = false;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( !str_cmp( arg2, "from" ) )
        argument = one_argument( argument, arg2 );

    /*
     * Get type. 
     */
    if ( arg1[0] == '\0' )
    {
        ch_printf( ch, "Get what?\r\n" );
        return;
    }

    if ( arg2[0] == '\0' )
    {
        if ( str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
        {
            /*
             * 'get obj' 
             */
            obj = get_obj_list( ch, arg1, ch->in_room->contents );
            if ( obj == NULL )
            {
                act( "I see no $T here.", ch, NULL, arg1, TO_CHAR );
                return;
            }

            get_obj( ch, obj, NULL );
        }
        else
        {
            /*
             * 'get all' or 'get all.obj' 
             */
            found = false;
            for ( obj = ch->in_room->contents; obj != NULL; obj = obj_next )
            {
                obj_next = obj->next_content;
                if ( ( arg1[3] == '\0' || is_name( &arg1[4], obj->name ) )
                     && can_see_obj( ch, obj ) )
                {
                    found = true;
                    get_obj( ch, obj, NULL );
                }
            }

            if ( !found )
            {
                if ( arg1[3] == '\0' )
                    ch_printf( ch, "I see nothing here.\r\n" );
                else
                    act( "I see no $T here.", ch, NULL, &arg1[4], TO_CHAR );
            }
        }
    }
    else
    {
        /*
         * 'get ... container' 
         */
        if ( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) )
        {
            ch_printf( ch, "You can't do that.\r\n" );
            return;
        }

        if ( ( container = get_obj_here( ch, arg2 ) ) == NULL )
        {
            act( "I see no $T here.", ch, NULL, arg2, TO_CHAR );
            return;
        }

        switch ( container->item_type )
        {
            default:
                ch_printf( ch, "That's not a container.\r\n" );
                return;

            case ITEM_CONTAINER:
            case ITEM_CORPSE_NPC:
                break;

            case ITEM_CORPSE_PC:
                {

                    if ( !can_loot( ch, container ) )
                    {
                        ch_printf( ch, "You can't do that.\r\n" );
                        return;
                    }
                }
        }

        if ( IS_SET( container->value[1], CONT_CLOSED ) )
        {
            act( "The $d is closed.", ch, NULL, container->name, TO_CHAR );
            return;
        }

        if ( str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
        {
            /*
             * 'get obj container' 
             */
            obj = get_obj_list( ch, arg1, container->contains );
            if ( obj == NULL )
            {
                act( "I see nothing like that in the $T.", ch, NULL, arg2, TO_CHAR );
                return;
            }
            get_obj( ch, obj, container );
        }
        else
        {
            /*
             * 'get all container' or 'get all.obj container' 
             */
            found = false;
            for ( obj = container->contains; obj != NULL; obj = obj_next )
            {
                obj_next = obj->next_content;
                if ( ( arg1[3] == '\0' || is_name( &arg1[4], obj->name ) )
                     && can_see_obj( ch, obj ) )
                {
                    found = true;
                    if ( container->pIndexData->vnum == OBJ_VNUM_PIT
                         && !IS_IMMORTAL( ch ) )
                    {
                        ch_printf( ch, "Don't be so greedy!\r\n" );
                        return;
                    }
                    get_obj( ch, obj, container );
                }
            }

            if ( !found )
            {
                if ( arg1[3] == '\0' )
                    act( "I see nothing in the $T.", ch, NULL, arg2, TO_CHAR );
                else
                    act( "I see nothing like that in the $T.", ch, NULL, arg2, TO_CHAR );
            }
        }
    }

    return;
}

void do_put( CHAR_DATA *ch, const char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *container = NULL;
    OBJ_DATA               *obj = NULL;
    OBJ_DATA               *obj_next = NULL;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( !str_cmp( arg2, "in" ) || !str_cmp( arg2, "on" ) )
        argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        ch_printf( ch, "Put what in what?\r\n" );
        return;
    }

    if ( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) )
    {
        ch_printf( ch, "You can't do that.\r\n" );
        return;
    }

    if ( ( container = get_obj_here( ch, arg2 ) ) == NULL )
    {
        act( "I see no $T here.", ch, NULL, arg2, TO_CHAR );
        return;
    }

    if ( container->item_type != ITEM_CONTAINER )
    {
        ch_printf( ch, "That's not a container.\r\n" );
        return;
    }

    if ( IS_SET( container->value[1], CONT_CLOSED ) )
    {
        act( "The $d is closed.", ch, NULL, container->name, TO_CHAR );
        return;
    }

    if ( str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
    {
        /*
         * 'put obj container' 
         */
        if ( ( obj = get_obj_carry( ch, arg1, ch ) ) == NULL )
        {
            ch_printf( ch, "You do not have that item.\r\n" );
            return;
        }

        if ( obj == container )
        {
            ch_printf( ch, "You can't fold it into itself.\r\n" );
            return;
        }

        if ( !can_drop_obj( ch, obj ) )
        {
            ch_printf( ch, "You can't let go of it.\r\n" );
            return;
        }

        if ( WEIGHT_MULT( obj ) != 100 )
        {
            ch_printf( ch, "You have a feeling that would be a bad idea.\r\n" );
            return;
        }

        if ( get_obj_weight( obj ) + get_true_weight( container )
             > ( container->value[0] * 10 )
             || get_obj_weight( obj ) > ( container->value[3] * 10 ) )
        {
            ch_printf( ch, "It won't fit.\r\n" );
            return;
        }

        if ( container->pIndexData->vnum == OBJ_VNUM_PIT
             && !CAN_WEAR( container, ITEM_TAKE ) )
        {
            if ( obj->timer )
                SET_BIT( obj->extra_flags, ITEM_HAD_TIMER );
            else
                obj->timer = number_range( 100, 200 );
        }
        obj_from_char( obj );
        obj_to_obj( obj, container );

        if ( IS_SET( container->value[1], CONT_PUT_ON ) )
        {
            act( "$n puts $p on $P.", ch, obj, container, TO_ROOM );
            act( "You put $p on $P.", ch, obj, container, TO_CHAR );
        }
        else
        {
            act( "$n puts $p in $P.", ch, obj, container, TO_ROOM );
            act( "You put $p in $P.", ch, obj, container, TO_CHAR );
        }
    }
    else
    {
        /*
         * 'put all container' or 'put all.obj container' 
         */
        for ( obj = ch->carrying; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;

            if ( ( arg1[3] == '\0' || is_name( &arg1[4], obj->name ) )
                 && can_see_obj( ch, obj )
                 && WEIGHT_MULT( obj ) == 100
                 && obj->wear_loc == WEAR_NONE
                 && obj != container
                 && can_drop_obj( ch, obj )
                 && get_obj_weight( obj ) + get_true_weight( container )
                 <= ( container->value[0] * 10 )
                 && get_obj_weight( obj ) <= ( container->value[3] * 10 ) )
            {
                if ( container->pIndexData->vnum == OBJ_VNUM_PIT
                     && !CAN_WEAR( obj, ITEM_TAKE ) )
                {
                    if ( obj->timer )
                        SET_BIT( obj->extra_flags, ITEM_HAD_TIMER );
                    else
                        obj->timer = number_range( 100, 200 );
                }
                obj_from_char( obj );
                obj_to_obj( obj, container );

                if ( IS_SET( container->value[1], CONT_PUT_ON ) )
                {
                    act( "$n puts $p on $P.", ch, obj, container, TO_ROOM );
                    act( "You put $p on $P.", ch, obj, container, TO_CHAR );
                }
                else
                {
                    act( "$n puts $p in $P.", ch, obj, container, TO_ROOM );
                    act( "You put $p in $P.", ch, obj, container, TO_CHAR );
                }
            }
        }
    }

    return;
}

void do_drop( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;
    OBJ_DATA               *obj_next = NULL;
    bool                    found = false;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Drop what?\r\n" );
        return;
    }

    if ( is_number( arg ) )
    {
        /*
         * 'drop NNNN coins' 
         */
        int                     amount = 0;
        int                     gold = 0;
        int                     silver = 0;

        amount = atoi( arg );
        argument = one_argument( argument, arg );
        if ( amount <= 0
             || ( str_cmp( arg, "coins" ) && str_cmp( arg, "coin" ) &&
                  str_cmp( arg, "gold" ) && str_cmp( arg, "silver" ) ) )
        {
            ch_printf( ch, "Sorry, you can't do that.\r\n" );
            return;
        }

        if ( !str_cmp( arg, "coins" ) || !str_cmp( arg, "coin" )
             || !str_cmp( arg, "silver" ) )
        {
            if ( ch->silver < amount )
            {
                ch_printf( ch, "You don't have that much silver.\r\n" );
                return;
            }

            ch->silver -= amount;
            silver = amount;
        }

        else
        {
            if ( ch->gold < amount )
            {
                ch_printf( ch, "You don't have that much gold.\r\n" );
                return;
            }

            ch->gold -= amount;
            gold = amount;
        }

        for ( obj = ch->in_room->contents; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;

            switch ( obj->pIndexData->vnum )
            {
                case OBJ_VNUM_SILVER_ONE:
                    silver += 1;
                    extract_obj( obj );
                    break;

                case OBJ_VNUM_GOLD_ONE:
                    gold += 1;
                    extract_obj( obj );
                    break;

                case OBJ_VNUM_SILVER_SOME:
                    silver += obj->value[0];
                    extract_obj( obj );
                    break;

                case OBJ_VNUM_GOLD_SOME:
                    gold += obj->value[1];
                    extract_obj( obj );
                    break;

                case OBJ_VNUM_COINS:
                    silver += obj->value[0];
                    gold += obj->value[1];
                    extract_obj( obj );
                    break;
            }
        }

        obj_to_room( create_money( gold, silver ), ch->in_room );
        act( "$n drops some coins.", ch, NULL, NULL, TO_ROOM );
        ch_printf( ch, "OK.\r\n" );
        return;
    }

    if ( str_cmp( arg, "all" ) && str_prefix( "all.", arg ) )
    {
        /*
         * 'drop obj' 
         */
        if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
        {
            ch_printf( ch, "You do not have that item.\r\n" );
            return;
        }

        if ( !can_drop_obj( ch, obj ) )
        {
            ch_printf( ch, "You can't let go of it.\r\n" );
            return;
        }

        obj_from_char( obj );
        obj_to_room( obj, ch->in_room );
        act( "$n drops $p.", ch, obj, NULL, TO_ROOM );
        act( "You drop $p.", ch, obj, NULL, TO_CHAR );
        if ( IS_OBJ_STAT( obj, ITEM_MELT_DROP ) )
        {
            act( "$p dissolves into smoke.", ch, obj, NULL, TO_ROOM );
            act( "$p dissolves into smoke.", ch, obj, NULL, TO_CHAR );
            extract_obj( obj );
        }
    }
    else
    {
        /*
         * 'drop all' or 'drop all.obj' 
         */
        found = false;
        for ( obj = ch->carrying; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;

            if ( ( arg[3] == '\0' || is_name( &arg[4], obj->name ) )
                 && can_see_obj( ch, obj )
                 && obj->wear_loc == WEAR_NONE && can_drop_obj( ch, obj ) )
            {
                found = true;
                obj_from_char( obj );
                obj_to_room( obj, ch->in_room );
                act( "$n drops $p.", ch, obj, NULL, TO_ROOM );
                act( "You drop $p.", ch, obj, NULL, TO_CHAR );
                if ( IS_OBJ_STAT( obj, ITEM_MELT_DROP ) )
                {
                    act( "$p dissolves into smoke.", ch, obj, NULL, TO_ROOM );
                    act( "$p dissolves into smoke.", ch, obj, NULL, TO_CHAR );
                    extract_obj( obj );
                }
            }
        }

        if ( !found )
        {
            if ( arg[3] == '\0' )
                act( "You are not carrying anything.", ch, NULL, arg, TO_CHAR );
            else
                act( "You are not carrying any $T.", ch, NULL, &arg[4], TO_CHAR );
        }
    }

    return;
}

void do_give( CHAR_DATA *ch, const char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;
    OBJ_DATA               *obj = NULL;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        ch_printf( ch, "Give what to whom?\r\n" );
        return;
    }

    if ( is_number( arg1 ) )
    {
        /*
         * 'give NNNN coins victim' 
         */
        int                     amount = 0;
        bool                    silver = false;

        amount = atoi( arg1 );
        if ( amount <= 0
             || ( str_cmp( arg2, "coins" ) && str_cmp( arg2, "coin" ) &&
                  str_cmp( arg2, "gold" ) && str_cmp( arg2, "silver" ) ) )
        {
            ch_printf( ch, "Sorry, you can't do that.\r\n" );
            return;
        }

        silver = str_cmp( arg2, "gold" );

        argument = one_argument( argument, arg2 );
        if ( arg2[0] == '\0' )
        {
            ch_printf( ch, "Give what to whom?\r\n" );
            return;
        }

        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
        {
            ch_printf( ch, "They aren't here.\r\n" );
            return;
        }

        if ( ( !silver && ch->gold < amount ) || ( silver && ch->silver < amount ) )
        {
            ch_printf( ch, "You haven't got that much.\r\n" );
            return;
        }

        if ( silver )
        {
            ch->silver -= amount;
            victim->silver += amount;
        }
        else
        {
            ch->gold -= amount;
            victim->gold += amount;
        }

        act_printf( "$n gives you %d %s.", ch, NULL, victim, TO_VICT, POS_RESTING, false,
                    amount, silver ? "silver" : "gold" );
        act( "$n gives $N some coins.", ch, NULL, victim, TO_NOTVICT );
        act_printf( "You give $N %d %s.", ch, NULL, victim, TO_CHAR, POS_RESTING, false,
                    amount, silver ? "silver" : "gold" );

        /*
         * Bribe trigger
         */
        if ( IS_NPC( victim ) && HAS_TRIGGER( victim, TRIG_BRIBE ) )
            mp_bribe_trigger( victim, ch, silver ? amount : amount * 100 );

        if ( IS_NPC( victim ) && IS_SET( victim->act, ACT_IS_CHANGER ) )
        {
            int                     change = 0;

            change = ( silver ? 95 * amount / 100 / 100 : 95 * amount );

            if ( !silver && change > victim->silver )
                victim->silver += change;

            if ( silver && change > victim->gold )
                victim->gold += change;

            if ( change < 1 && can_see( victim, ch ) )
            {
                act( "$n tells you 'I'm sorry, you did not give me enough to change.'",
                     victim, NULL, ch, TO_VICT );
                ch->reply = victim;
                sprintf( buf, "%d %s %s", amount, silver ? "silver" : "gold", ch->name );
                do_function( victim, &do_give, buf );
            }
            else if ( can_see( victim, ch ) )
            {
                sprintf( buf, "%d %s %s", change, silver ? "gold" : "silver", ch->name );
                do_function( victim, &do_give, buf );
                if ( silver )
                {
                    sprintf( buf, "%d silver %s",
                             ( 95 * amount / 100 - change * 100 ), ch->name );
                    do_function( victim, &do_give, buf );
                }
                act( "$n tells you 'Thank you, come again.'", victim, NULL, ch, TO_VICT );
                ch->reply = victim;
            }
        }
        return;
    }

    if ( ( obj = get_obj_carry( ch, arg1, ch ) ) == NULL )
    {
        ch_printf( ch, "You do not have that item.\r\n" );
        return;
    }

    if ( obj->wear_loc != WEAR_NONE )
    {
        ch_printf( ch, "You must remove it first.\r\n" );
        return;
    }

    if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
    {
        ch_printf( ch, "They aren't here.\r\n" );
        return;
    }

    if ( IS_NPC( victim ) && victim->pIndexData->pShop != NULL )
    {
        act( "$N tells you 'Sorry, you'll have to sell that.'",
             ch, NULL, victim, TO_CHAR );
        ch->reply = victim;
        return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
        ch_printf( ch, "You can't let go of it.\r\n" );
        return;
    }

    if ( victim->carry_number + get_obj_number( obj ) > can_carry_n( victim ) )
    {
        act( "$N has $S hands full.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( get_carry_weight( victim ) + get_obj_weight( obj ) > can_carry_w( victim ) )
    {
        act( "$N can't carry that much weight.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( !can_see_obj( victim, obj ) )
    {
        act( "$N can't see it.", ch, NULL, victim, TO_CHAR );
        return;
    }

    obj_from_char( obj );
    obj_to_char( obj, victim );
    MOBtrigger = false;
    act( "$n gives $p to $N.", ch, obj, victim, TO_NOTVICT );
    act( "$n gives you $p.", ch, obj, victim, TO_VICT );
    act( "You give $p to $N.", ch, obj, victim, TO_CHAR );
    MOBtrigger = true;

    /*
     * Give trigger
     */
    if ( IS_NPC( victim ) && HAS_TRIGGER( victim, TRIG_GIVE ) )
        mp_give_trigger( victim, ch, obj );

    return;
}

/* for poisoning weapons and food/drink */
void do_envenom( CHAR_DATA *ch, const char *argument )
{
    OBJ_DATA               *obj = NULL;
    AFFECT_DATA             af;
    int                     percent = 0;
    int                     skill = 0;
    int                     sn = -1;

    /*
     * find out what 
     */
    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Envenom what item?\r\n" );
        return;
    }

    obj = get_obj_list( ch, argument, ch->carrying );

    if ( obj == NULL )
    {
        ch_printf( ch, "You don't have that item.\r\n" );
        return;
    }

    if ( ( sn = skill_lookup( "envenom" ) ) == -1 )
    {
        log_error( "Can't find the \"%s\" skill in %s?", "envenom", __FUNCTION__ );
        return;
    }

    if ( ( skill = get_skill( ch, sn ) ) < 1 )
    {
        ch_printf( ch, "Are you crazy? You'd poison yourself!\r\n" );
        return;
    }

    if ( obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON )
    {
        if ( IS_OBJ_STAT( obj, ITEM_BLESS ) || IS_OBJ_STAT( obj, ITEM_BURN_PROOF ) )
        {
            act( "You fail to poison $p.", ch, obj, NULL, TO_CHAR );
            return;
        }

        if ( number_percent(  ) < skill )              /* success! */
        {
            act( "$n treats $p with deadly poison.", ch, obj, NULL, TO_ROOM );
            act( "You treat $p with deadly poison.", ch, obj, NULL, TO_CHAR );
            if ( !obj->value[3] )
            {
                obj->value[3] = 1;
                check_improve( ch, sn, true, 4 );
            }
            WAIT_STATE( ch, skill_table[sn].beats );
            return;
        }

        act( "You fail to poison $p.", ch, obj, NULL, TO_CHAR );
        if ( !obj->value[3] )
            check_improve( ch, sn, false, 4 );
        WAIT_STATE( ch, skill_table[sn].beats );
        return;
    }

    if ( obj->item_type == ITEM_WEAPON )
    {
        if ( IS_WEAPON_STAT( obj, WEAPON_FLAMING )
             || IS_WEAPON_STAT( obj, WEAPON_FROST )
             || IS_WEAPON_STAT( obj, WEAPON_VAMPIRIC )
             || IS_WEAPON_STAT( obj, WEAPON_SHARP )
             || IS_WEAPON_STAT( obj, WEAPON_VORPAL )
             || IS_WEAPON_STAT( obj, WEAPON_SHOCKING )
             || IS_OBJ_STAT( obj, ITEM_BLESS ) || IS_OBJ_STAT( obj, ITEM_BURN_PROOF ) )
        {
            act( "You can't seem to envenom $p.", ch, obj, NULL, TO_CHAR );
            return;
        }

        if ( obj->value[3] < 0 || attack_table[obj->value[3]].damage == DAM_BASH )
        {
            ch_printf( ch, "You can only envenom edged weapons.\r\n" );
            return;
        }

        if ( IS_WEAPON_STAT( obj, WEAPON_POISON ) )
        {
            act( "$p is already envenomed.", ch, obj, NULL, TO_CHAR );
            return;
        }

        percent = number_percent(  );
        if ( percent < skill )
        {

            af.where = TO_WEAPON;
            af.type = skill_lookup( "poison" );
            af.level = ch->level * percent / 100;
            af.duration = ch->level / 2 * percent / 100;
            af.location = 0;
            af.modifier = 0;
            af.bitvector = WEAPON_POISON;
            affect_to_obj( obj, &af );

            act( "$n coats $p with deadly venom.", ch, obj, NULL, TO_ROOM );
            act( "You coat $p with venom.", ch, obj, NULL, TO_CHAR );
            check_improve( ch, sn, true, 3 );
            WAIT_STATE( ch, skill_table[sn].beats );
            return;
        }
        else
        {
            act( "You fail to envenom $p.", ch, obj, NULL, TO_CHAR );
            check_improve( ch, sn, false, 3 );
            WAIT_STATE( ch, skill_table[sn].beats );
            return;
        }
    }

    act( "You can't poison $p.", ch, obj, NULL, TO_CHAR );
    return;
}

void do_fill( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;
    OBJ_DATA               *fountain = NULL;
    bool                    found = false;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Fill what?\r\n" );
        return;
    }

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
        ch_printf( ch, "You do not have that item.\r\n" );
        return;
    }

    found = false;
    for ( fountain = ch->in_room->contents; fountain != NULL;
          fountain = fountain->next_content )
    {
        if ( fountain->item_type == ITEM_FOUNTAIN )
        {
            found = true;
            break;
        }
    }

    if ( !found )
    {
        ch_printf( ch, "There is no fountain here!\r\n" );
        return;
    }

    if ( obj->item_type != ITEM_DRINK_CON )
    {
        ch_printf( ch, "You can't fill that.\r\n" );
        return;
    }

    if ( obj->value[1] != 0 && obj->value[2] != fountain->value[2] )
    {
        ch_printf( ch, "There is already another liquid in it.\r\n" );
        return;
    }

    if ( obj->value[1] >= obj->value[0] )
    {
        ch_printf( ch, "Your container is full.\r\n" );
        return;
    }

    act_printf( "You fill $p with %s from $P.",
                ch, obj, fountain, TO_CHAR, POS_RESTING, false,
                liq_table[fountain->value[2]].liq_name );
    act_printf( "$n fills $p with %s from $P.",
                ch, obj, fountain, TO_ROOM, POS_RESTING, false,
                liq_table[fountain->value[2]].liq_name );
    obj->value[2] = fountain->value[2];
    obj->value[1] = obj->value[0];
    return;
}

void do_pour( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *out = NULL;
    OBJ_DATA               *in = NULL;
    CHAR_DATA              *vch = NULL;
    int                     amount = 0;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
        ch_printf( ch, "Pour what into what?\r\n" );
        return;
    }

    if ( ( out = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
        ch_printf( ch, "You don't have that item.\r\n" );
        return;
    }

    if ( out->item_type != ITEM_DRINK_CON )
    {
        ch_printf( ch, "That's not a drink container.\r\n" );
        return;
    }

    if ( !str_cmp( argument, "out" ) )
    {
        if ( out->value[1] == 0 )
        {
            ch_printf( ch, "It's already empty.\r\n" );
            return;
        }

        out->value[1] = 0;
        out->value[3] = 0;
        act_printf( "You invert $p, spilling %s all over the ground.",
                    ch, out, NULL, TO_CHAR, POS_RESTING, false,
                    liq_table[out->value[2]].liq_name );

        act_printf( "$n inverts $p, spilling %s all over the ground.",
                    ch, out, NULL, TO_ROOM, POS_RESTING, false,
                    liq_table[out->value[2]].liq_name );
        return;
    }

    if ( ( in = get_obj_here( ch, argument ) ) == NULL )
    {
        vch = get_char_room( ch, argument );

        if ( vch == NULL )
        {
            ch_printf( ch, "Pour into what?\r\n" );
            return;
        }

        in = get_eq_char( vch, WEAR_HOLD );

        if ( in == NULL )
        {
            ch_printf( ch, "They aren't holding anything." );
            return;
        }
    }

    if ( in->item_type != ITEM_DRINK_CON )
    {
        ch_printf( ch, "You can only pour into other drink containers.\r\n" );
        return;
    }

    if ( in == out )
    {
        ch_printf( ch, "You cannot change the laws of physics!\r\n" );
        return;
    }

    if ( in->value[1] != 0 && in->value[2] != out->value[2] )
    {
        ch_printf( ch, "They don't hold the same liquid.\r\n" );
        return;
    }

    if ( out->value[1] == 0 )
    {
        act( "There's nothing in $p to pour.", ch, out, NULL, TO_CHAR );
        return;
    }

    if ( in->value[1] >= in->value[0] )
    {
        act( "$p is already filled to the top.", ch, in, NULL, TO_CHAR );
        return;
    }

    amount = UMIN( out->value[1], in->value[0] - in->value[1] );

    in->value[1] += amount;
    out->value[1] -= amount;
    in->value[2] = out->value[2];

    if ( vch == NULL )
    {
        act_printf( "You pour %s from $p into $P.", ch, out, in, TO_CHAR, POS_RESTING,
                    false, liq_table[out->value[2]].liq_name );
        act_printf( "$n pours %s from $p into $P.", ch, out, in, TO_ROOM, POS_RESTING,
                    false, liq_table[out->value[2]].liq_name );
    }
    else
    {
        act_printf( "You pour some %s for $N.", ch, NULL, vch, TO_CHAR, POS_RESTING,
                    false, liq_table[out->value[2]].liq_name );
        act_printf( "$n pours you some %s.", ch, NULL, vch, TO_VICT, POS_RESTING, false,
                    liq_table[out->value[2]].liq_name );
        act_printf( "$n pours some %s for $N.", ch, NULL, vch, TO_NOTVICT, POS_RESTING,
                    false, liq_table[out->value[2]].liq_name );
    }
}

void do_drink( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;
    int                     amount = 0;
    int                     liquid = 0;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        for ( obj = ch->in_room->contents; obj; obj = obj->next_content )
        {
            if ( obj->item_type == ITEM_FOUNTAIN )
                break;
        }

        if ( obj == NULL )
        {
            ch_printf( ch, "Drink what?\r\n" );
            return;
        }
    }
    else
    {
        if ( ( obj = get_obj_here( ch, arg ) ) == NULL )
        {
            ch_printf( ch, "You can't find it.\r\n" );
            return;
        }
    }

    if ( !IS_NPC( ch ) && ch->pcdata->condition[COND_DRUNK] > 10 )
    {
        ch_printf( ch, "You fail to reach your mouth.  *Hic*\r\n" );
        return;
    }

    switch ( obj->item_type )
    {
        default:
            ch_printf( ch, "You can't drink from that.\r\n" );
            return;

        case ITEM_FOUNTAIN:
            if ( ( liquid = obj->value[2] ) < 0 )
            {
                log_error( "Bad liquid number %d", liquid );
                liquid = obj->value[2] = 0;
            }
            amount = liq_table[liquid].liq_affect[4] * 3;
            break;

        case ITEM_DRINK_CON:
            if ( obj->value[1] <= 0 )
            {
                ch_printf( ch, "It is already empty.\r\n" );
                return;
            }

            if ( ( liquid = obj->value[2] ) < 0 )
            {
                log_error( "Bad liquid number %d", liquid );
                liquid = obj->value[2] = 0;
            }

            amount = liq_table[liquid].liq_affect[4];
            amount = UMIN( amount, obj->value[1] );
            break;
    }
    if ( !IS_NPC( ch ) && !IS_IMMORTAL( ch ) && ch->pcdata->condition[COND_FULL] > 45 )
    {
        ch_printf( ch, "You're too full to drink more.\r\n" );
        return;
    }

    act( "$n drinks $T from $p.", ch, obj, liq_table[liquid].liq_name, TO_ROOM );
    act( "You drink $T from $p.", ch, obj, liq_table[liquid].liq_name, TO_CHAR );

    gain_condition( ch, COND_DRUNK,
                    amount * liq_table[liquid].liq_affect[COND_DRUNK] / 36 );
    gain_condition( ch, COND_FULL, amount * liq_table[liquid].liq_affect[COND_FULL] / 4 );
    gain_condition( ch, COND_THIRST,
                    amount * liq_table[liquid].liq_affect[COND_THIRST] / 10 );
    gain_condition( ch, COND_HUNGER,
                    amount * liq_table[liquid].liq_affect[COND_HUNGER] / 2 );

    if ( !IS_NPC( ch ) && ch->pcdata->condition[COND_DRUNK] > 10 )
        ch_printf( ch, "You feel drunk.\r\n" );
    if ( !IS_NPC( ch ) && ch->pcdata->condition[COND_FULL] > 40 )
        ch_printf( ch, "You are full.\r\n" );
    if ( !IS_NPC( ch ) && ch->pcdata->condition[COND_THIRST] > 40 )
        ch_printf( ch, "Your thirst is quenched.\r\n" );

    if ( obj->value[3] != 0 )
    {
        /*
         * The drink was poisoned ! 
         */
        AFFECT_DATA             af;

        act( "$n chokes and gags.", ch, NULL, NULL, TO_ROOM );
        ch_printf( ch, "You choke and gag.\r\n" );
        af.where = TO_AFFECTS;
        af.type = skill_lookup( "poison" );
        af.level = number_fuzzy( amount );
        af.duration = 3 * amount;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = AFF_POISON;
        affect_join( ch, &af );
    }

    if ( obj->value[0] > 0 )
        obj->value[1] -= amount;

    return;
}

void do_eat( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Eat what?\r\n" );
        return;
    }

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
        ch_printf( ch, "You do not have that item.\r\n" );
        return;
    }

    if ( !IS_IMMORTAL( ch ) )
    {
        if ( obj->item_type != ITEM_FOOD && obj->item_type != ITEM_PILL )
        {
            ch_printf( ch, "That's not edible.\r\n" );
            return;
        }

        if ( !IS_NPC( ch ) && ch->pcdata->condition[COND_FULL] > 40 )
        {
            ch_printf( ch, "You are too full to eat more.\r\n" );
            return;
        }
    }

    act( "$n eats $p.", ch, obj, NULL, TO_ROOM );
    act( "You eat $p.", ch, obj, NULL, TO_CHAR );

    switch ( obj->item_type )
    {

        case ITEM_FOOD:
            if ( !IS_NPC( ch ) )
            {
                int                     condition = 0;

                condition = ch->pcdata->condition[COND_HUNGER];
                gain_condition( ch, COND_FULL, obj->value[0] );
                gain_condition( ch, COND_HUNGER, obj->value[1] );
                if ( condition == 0 && ch->pcdata->condition[COND_HUNGER] > 0 )
                    ch_printf( ch, "You are no longer hungry.\r\n" );
                else if ( ch->pcdata->condition[COND_FULL] > 40 )
                    ch_printf( ch, "You are full.\r\n" );
            }

            if ( obj->value[3] != 0 )
            {
                /*
                 * The food was poisoned! 
                 */
                AFFECT_DATA             af;

                act( "$n chokes and gags.", ch, 0, 0, TO_ROOM );
                ch_printf( ch, "You choke and gag.\r\n" );

                af.where = TO_AFFECTS;
                af.type = skill_lookup( "poison" );
                af.level = number_fuzzy( obj->value[0] );
                af.duration = 2 * obj->value[0];
                af.location = APPLY_NONE;
                af.modifier = 0;
                af.bitvector = AFF_POISON;
                affect_join( ch, &af );
            }
            break;

        case ITEM_PILL:
            obj_cast_spell( obj->value[1], obj->value[0], ch, ch, NULL );
            obj_cast_spell( obj->value[2], obj->value[0], ch, ch, NULL );
            obj_cast_spell( obj->value[3], obj->value[0], ch, ch, NULL );
            break;
    }

    extract_obj( obj );
    return;
}

/*
 * Remove an object.
 */
bool remove_obj( CHAR_DATA *ch, int iWear, bool fReplace )
{
    OBJ_DATA               *obj = NULL;

    if ( ( obj = get_eq_char( ch, iWear ) ) == NULL )
        return true;

    if ( !fReplace )
        return false;

    if ( IS_SET( obj->extra_flags, ITEM_NOREMOVE ) )
    {
        act( "You can't remove $p.", ch, obj, NULL, TO_CHAR );
        return false;
    }

    unequip_char( ch, obj );
    act( "$n stops using $p.", ch, obj, NULL, TO_ROOM );
    act( "You stop using $p.", ch, obj, NULL, TO_CHAR );
    return true;
}

/*
 * Wear one object.
 * Optional replacement of existing objects.
 * Big repetitive code, ick.
 */
void wear_obj( CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace )
{
    if ( ch->level < obj->level )
    {
        ch_printf( ch, "You must be level %d to use this object.\r\n", obj->level );
        act( "$n tries to use $p, but is too inexperienced.", ch, obj, NULL, TO_ROOM );
        return;
    }

    if ( obj->item_type == ITEM_LIGHT )
    {
        if ( !remove_obj( ch, WEAR_LIGHT, fReplace ) )
            return;
        act( "$n lights $p and holds it.", ch, obj, NULL, TO_ROOM );
        act( "You light $p and hold it.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_LIGHT );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_FINGER ) )
    {
        if ( get_eq_char( ch, WEAR_FINGER_L ) != NULL
             && get_eq_char( ch, WEAR_FINGER_R ) != NULL
             && !remove_obj( ch, WEAR_FINGER_L, fReplace )
             && !remove_obj( ch, WEAR_FINGER_R, fReplace ) )
            return;

        if ( get_eq_char( ch, WEAR_FINGER_L ) == NULL )
        {
            act( "$n wears $p on $s left finger.", ch, obj, NULL, TO_ROOM );
            act( "You wear $p on your left finger.", ch, obj, NULL, TO_CHAR );
            equip_char( ch, obj, WEAR_FINGER_L );
            return;
        }

        if ( get_eq_char( ch, WEAR_FINGER_R ) == NULL )
        {
            act( "$n wears $p on $s right finger.", ch, obj, NULL, TO_ROOM );
            act( "You wear $p on your right finger.", ch, obj, NULL, TO_CHAR );
            equip_char( ch, obj, WEAR_FINGER_R );
            return;
        }

        log_error( "%s attempting to wear a ring, but has no free finger slot",
                   NAME( ch ) );
        ch_printf( ch, "You already wear two rings.\r\n" );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_NECK ) )
    {
        if ( get_eq_char( ch, WEAR_NECK_1 ) != NULL
             && get_eq_char( ch, WEAR_NECK_2 ) != NULL
             && !remove_obj( ch, WEAR_NECK_1, fReplace )
             && !remove_obj( ch, WEAR_NECK_2, fReplace ) )
            return;

        if ( get_eq_char( ch, WEAR_NECK_1 ) == NULL )
        {
            act( "$n wears $p around $s neck.", ch, obj, NULL, TO_ROOM );
            act( "You wear $p around your neck.", ch, obj, NULL, TO_CHAR );
            equip_char( ch, obj, WEAR_NECK_1 );
            return;
        }

        if ( get_eq_char( ch, WEAR_NECK_2 ) == NULL )
        {
            act( "$n wears $p around $s neck.", ch, obj, NULL, TO_ROOM );
            act( "You wear $p around your neck.", ch, obj, NULL, TO_CHAR );
            equip_char( ch, obj, WEAR_NECK_2 );
            return;
        }

        log_error( "%s attempting to wear a necklace, but has no free neck slot",
                   NAME( ch ) );
        ch_printf( ch, "You already wear two neck items.\r\n" );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_BODY ) )
    {
        if ( !remove_obj( ch, WEAR_BODY, fReplace ) )
            return;
        act( "$n wears $p on $s torso.", ch, obj, NULL, TO_ROOM );
        act( "You wear $p on your torso.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_BODY );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_HEAD ) )
    {
        if ( !remove_obj( ch, WEAR_HEAD, fReplace ) )
            return;
        act( "$n wears $p on $s head.", ch, obj, NULL, TO_ROOM );
        act( "You wear $p on your head.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_HEAD );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_LEGS ) )
    {
        if ( !remove_obj( ch, WEAR_LEGS, fReplace ) )
            return;
        act( "$n wears $p on $s legs.", ch, obj, NULL, TO_ROOM );
        act( "You wear $p on your legs.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_LEGS );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_FEET ) )
    {
        if ( !remove_obj( ch, WEAR_FEET, fReplace ) )
            return;
        act( "$n wears $p on $s feet.", ch, obj, NULL, TO_ROOM );
        act( "You wear $p on your feet.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_FEET );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_HANDS ) )
    {
        if ( !remove_obj( ch, WEAR_HANDS, fReplace ) )
            return;
        act( "$n wears $p on $s hands.", ch, obj, NULL, TO_ROOM );
        act( "You wear $p on your hands.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_HANDS );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_ARMS ) )
    {
        if ( !remove_obj( ch, WEAR_ARMS, fReplace ) )
            return;
        act( "$n wears $p on $s arms.", ch, obj, NULL, TO_ROOM );
        act( "You wear $p on your arms.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_ARMS );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_ABOUT ) )
    {
        if ( !remove_obj( ch, WEAR_ABOUT, fReplace ) )
            return;
        act( "$n wears $p about $s torso.", ch, obj, NULL, TO_ROOM );
        act( "You wear $p about your torso.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_ABOUT );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_WAIST ) )
    {
        if ( !remove_obj( ch, WEAR_WAIST, fReplace ) )
            return;
        act( "$n wears $p about $s waist.", ch, obj, NULL, TO_ROOM );
        act( "You wear $p about your waist.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_WAIST );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_WRIST ) )
    {
        if ( get_eq_char( ch, WEAR_WRIST_L ) != NULL
             && get_eq_char( ch, WEAR_WRIST_R ) != NULL
             && !remove_obj( ch, WEAR_WRIST_L, fReplace )
             && !remove_obj( ch, WEAR_WRIST_R, fReplace ) )
            return;

        if ( get_eq_char( ch, WEAR_WRIST_L ) == NULL )
        {
            act( "$n wears $p around $s left wrist.", ch, obj, NULL, TO_ROOM );
            act( "You wear $p around your left wrist.", ch, obj, NULL, TO_CHAR );
            equip_char( ch, obj, WEAR_WRIST_L );
            return;
        }

        if ( get_eq_char( ch, WEAR_WRIST_R ) == NULL )
        {
            act( "$n wears $p around $s right wrist.", ch, obj, NULL, TO_ROOM );
            act( "You wear $p around your right wrist.", ch, obj, NULL, TO_CHAR );
            equip_char( ch, obj, WEAR_WRIST_R );
            return;
        }

        log_error( "%s attempting to wear a bracelet, but has no free wrist slot",
                   NAME( ch ) );
        ch_printf( ch, "You already wear two wrist items.\r\n" );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_SHIELD ) )
    {
        OBJ_DATA               *weapon;

        if ( !remove_obj( ch, WEAR_SHIELD, fReplace ) )
            return;

        weapon = get_eq_char( ch, WEAR_WIELD );
        if ( weapon != NULL && ch->size < SIZE_LARGE
             && IS_WEAPON_STAT( weapon, WEAPON_TWO_HANDS ) )
        {
            ch_printf( ch, "Your hands are tied up with your weapon!\r\n" );
            return;
        }

        act( "$n wears $p as a shield.", ch, obj, NULL, TO_ROOM );
        act( "You wear $p as a shield.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_SHIELD );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WIELD ) )
    {
        int                     sn = -1;
        int                     skill = 0;

        if ( !remove_obj( ch, WEAR_WIELD, fReplace ) )
            return;

        if ( !IS_NPC( ch )
             && get_obj_weight( obj ) > ( str_app[get_curr_stat( ch, STAT_STR )].wield
                                          * 10 ) )
        {
            ch_printf( ch, "It is too heavy for you to wield.\r\n" );
            return;
        }

        if ( !IS_NPC( ch ) && ch->size < SIZE_LARGE
             && IS_WEAPON_STAT( obj, WEAPON_TWO_HANDS )
             && get_eq_char( ch, WEAR_SHIELD ) != NULL )
        {
            ch_printf( ch, "You need two hands free for that weapon.\r\n" );
            return;
        }

        act( "$n wields $p.", ch, obj, NULL, TO_ROOM );
        act( "You wield $p.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_WIELD );

        sn = get_weapon_sn( ch );

        if ( sn == skill_lookup( "hand to hand" ) )
            return;

        skill = get_weapon_skill( ch, sn );

        if ( skill >= 100 )
            act( "$p feels like a part of you!", ch, obj, NULL, TO_CHAR );
        else if ( skill > 85 )
            act( "You feel quite confident with $p.", ch, obj, NULL, TO_CHAR );
        else if ( skill > 70 )
            act( "You are skilled with $p.", ch, obj, NULL, TO_CHAR );
        else if ( skill > 50 )
            act( "Your skill with $p is adequate.", ch, obj, NULL, TO_CHAR );
        else if ( skill > 25 )
            act( "$p feels a little clumsy in your hands.", ch, obj, NULL, TO_CHAR );
        else if ( skill > 1 )
            act( "You fumble and almost drop $p.", ch, obj, NULL, TO_CHAR );
        else
            act( "You don't even know which end is up on $p.", ch, obj, NULL, TO_CHAR );

        return;
    }

    if ( CAN_WEAR( obj, ITEM_HOLD ) )
    {
        if ( !remove_obj( ch, WEAR_HOLD, fReplace ) )
            return;
        act( "$n holds $p in $s hand.", ch, obj, NULL, TO_ROOM );
        act( "You hold $p in your hand.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_HOLD );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_FLOAT ) )
    {
        if ( !remove_obj( ch, WEAR_FLOAT, fReplace ) )
            return;
        act( "$n releases $p to float next to $m.", ch, obj, NULL, TO_ROOM );
        act( "You release $p and it floats next to you.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_FLOAT );
        return;
    }

    if ( fReplace )
        ch_printf( ch, "You can't wear, wield, or hold that.\r\n" );

    return;
}

void do_wear( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Wear, wield, or hold what?\r\n" );
        return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
        OBJ_DATA               *obj_next = NULL;

        for ( obj = ch->carrying; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;
            if ( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj ) )
                wear_obj( ch, obj, false );
        }
        return;
    }
    else
    {
        if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
        {
            ch_printf( ch, "You do not have that item.\r\n" );
            return;
        }

        wear_obj( ch, obj, true );
    }

    return;
}

void do_remove( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Remove what?\r\n" );
        return;
    }

    if ( ( obj = get_obj_wear( ch, arg ) ) == NULL )
    {
        ch_printf( ch, "You do not have that item.\r\n" );
        return;
    }

    remove_obj( ch, obj->wear_loc, true );
    return;
}

void do_sacrifice( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;
    int                     silver = 0;

    /*
     * variables for AUTOSPLIT 
     */
    CHAR_DATA              *gch = NULL;
    int                     members = 0;
    char                    buffer[100] = "\0\0\0\0\0\0\0";

    one_argument( argument, arg );

    if ( arg[0] == '\0' || !str_cmp( arg, ch->name ) )
    {
        act( "$n offers $mself to Mota, who graciously declines.",
             ch, NULL, NULL, TO_ROOM );
        ch_printf( ch, "Mota appreciates your offer and may accept it later.\r\n" );
        return;
    }

    obj = get_obj_list( ch, arg, ch->in_room->contents );
    if ( obj == NULL )
    {
        ch_printf( ch, "You can't find it.\r\n" );
        return;
    }

    if ( obj->item_type == ITEM_CORPSE_PC )
    {
        if ( obj->contains )
        {
            ch_printf( ch, "Mota wouldn't like that.\r\n" );
            return;
        }
    }

    if ( !CAN_WEAR( obj, ITEM_TAKE ) || CAN_WEAR( obj, ITEM_NO_SAC ) )
    {
        act( "$p is not an acceptable sacrifice.", ch, obj, 0, TO_CHAR );
        return;
    }

    if ( obj->in_room != NULL )
    {
        for ( gch = obj->in_room->people; gch != NULL; gch = gch->next_in_room )
            if ( gch->on == obj )
            {
                act( "$N appears to be using $p.", ch, obj, gch, TO_CHAR );
                return;
            }
    }

    silver = UMAX( 1, obj->level * 3 );

    if ( obj->item_type != ITEM_CORPSE_NPC && obj->item_type != ITEM_CORPSE_PC )
        silver = UMIN( silver, obj->cost );

    if ( silver == 1 )
        ch_printf( ch, "Mota gives you one silver coin for your sacrifice.\r\n" );
    else
    {
        ch_printf( ch, "Mota gives you %d silver coins for your sacrifice.\r\n", silver );
    }

    ch->silver += silver;

    if ( IS_SET( ch->act, PLR_AUTOSPLIT ) )
    {                                                  /* AUTOSPLIT code */
        members = 0;
        for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
        {
            if ( is_same_group( gch, ch ) )
                members++;
        }

        if ( members > 1 && silver > 1 )
        {
            sprintf( buffer, "%d", silver );
            do_function( ch, &do_split, buffer );
        }
    }

    act( "$n sacrifices $p to Mota.", ch, obj, NULL, TO_ROOM );
    wiznet( "$N sends up $p as a burnt offering.", ch, obj, WIZ_SACCING, 0, 0 );
    extract_obj( obj );
    return;
}

void do_quaff( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Quaff what?\r\n" );
        return;
    }

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
        ch_printf( ch, "You do not have that potion.\r\n" );
        return;
    }

    if ( obj->item_type != ITEM_POTION )
    {
        ch_printf( ch, "You can quaff only potions.\r\n" );
        return;
    }

    if ( ch->level < obj->level )
    {
        ch_printf( ch, "This liquid is too powerful for you to drink.\r\n" );
        return;
    }

    act( "$n quaffs $p.", ch, obj, NULL, TO_ROOM );
    act( "You quaff $p.", ch, obj, NULL, TO_CHAR );

    obj_cast_spell( obj->value[1], obj->value[0], ch, ch, NULL );
    obj_cast_spell( obj->value[2], obj->value[0], ch, ch, NULL );
    obj_cast_spell( obj->value[3], obj->value[0], ch, ch, NULL );

    extract_obj( obj );
    return;
}

void do_recite( CHAR_DATA *ch, const char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;
    OBJ_DATA               *scroll = NULL;
    OBJ_DATA               *obj = NULL;
    int                     sn = -1;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( ( scroll = get_obj_carry( ch, arg1, ch ) ) == NULL )
    {
        ch_printf( ch, "You do not have that scroll.\r\n" );
        return;
    }

    if ( scroll->item_type != ITEM_SCROLL )
    {
        ch_printf( ch, "You can recite only scrolls.\r\n" );
        return;
    }

    if ( ch->level < scroll->level )
    {
        ch_printf( ch, "This scroll is too complex for you to comprehend.\r\n" );
        return;
    }

    obj = NULL;
    if ( arg2[0] == '\0' )
    {
        victim = ch;
    }
    else
    {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL
             && ( obj = get_obj_here( ch, arg2 ) ) == NULL )
        {
            ch_printf( ch, "You can't find it.\r\n" );
            return;
        }
    }

    if ( ( sn = skill_lookup( "scrolls" ) ) == -1 )
    {
        log_error( "Can't find the \"%s\" skill in %s?", "scrolls", __FUNCTION__ );
        return;
    }

    act( "$n recites $p.", ch, scroll, NULL, TO_ROOM );
    act( "You recite $p.", ch, scroll, NULL, TO_CHAR );

    if ( number_percent(  ) >= 20 + get_skill( ch, sn ) * 4 / 5 )
    {
        ch_printf( ch, "You mispronounce a syllable.\r\n" );
        check_improve( ch, sn, false, 2 );
    }

    else
    {
        obj_cast_spell( scroll->value[1], scroll->value[0], ch, victim, obj );
        obj_cast_spell( scroll->value[2], scroll->value[0], ch, victim, obj );
        obj_cast_spell( scroll->value[3], scroll->value[0], ch, victim, obj );
        check_improve( ch, sn, true, 2 );
    }

    extract_obj( scroll );
    return;
}

void do_brandish( CHAR_DATA *ch, const char *argument )
{
    CHAR_DATA              *vch = NULL;
    CHAR_DATA              *vch_next = NULL;
    OBJ_DATA               *staff = NULL;
    int                     sn = -1;
    int                     staff_sn = -1;

    if ( ( staff = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
    {
        ch_printf( ch, "You hold nothing in your hand.\r\n" );
        return;
    }

    if ( staff->item_type != ITEM_STAFF )
    {
        ch_printf( ch, "You can brandish only with a staff.\r\n" );
        return;
    }

    if ( ( sn = staff->value[3] ) < 0
         || sn >= MAX_SKILL || skill_table[sn].spell_fun == 0 )
    {
        log_error( "Bad skill number %d", sn );
        return;
    }

    if ( ( staff_sn = skill_lookup( "staves" ) ) == -1 )
    {
        log_error( "Can't find the \"%s\" skill in %s?", "staves", __FUNCTION__ );
        return;
    }

    WAIT_STATE( ch, 2 * PULSE_VIOLENCE );

    if ( staff->value[2] > 0 )
    {
        act( "$n brandishes $p.", ch, staff, NULL, TO_ROOM );
        act( "You brandish $p.", ch, staff, NULL, TO_CHAR );
        if ( ch->level < staff->level
             || number_percent(  ) >= 20 + get_skill( ch, staff_sn ) * 4 / 5 )
        {
            act( "You fail to invoke $p.", ch, staff, NULL, TO_CHAR );
            act( "...and nothing happens.", ch, NULL, NULL, TO_ROOM );
            check_improve( ch, staff_sn, false, 2 );
        }

        else
            for ( vch = ch->in_room->people; vch; vch = vch_next )
            {
                vch_next = vch->next_in_room;

                switch ( skill_table[sn].target )
                {
                    default:
                        log_error( "Bad target entry for skill number %d", sn );
                        return;

                    case TAR_IGNORE:
                        if ( vch != ch )
                            continue;
                        break;

                    case TAR_CHAR_OFFENSIVE:
                        if ( IS_NPC( ch ) ? IS_NPC( vch ) : !IS_NPC( vch ) )
                            continue;
                        break;

                    case TAR_CHAR_DEFENSIVE:
                        if ( IS_NPC( ch ) ? !IS_NPC( vch ) : IS_NPC( vch ) )
                            continue;
                        break;

                    case TAR_CHAR_SELF:
                        if ( vch != ch )
                            continue;
                        break;
                }

                obj_cast_spell( staff->value[3], staff->value[0], ch, vch, NULL );
                check_improve( ch, staff_sn, true, 2 );
            }
    }

    if ( --staff->value[2] <= 0 )
    {
        act( "$n's $p blazes bright and is gone.", ch, staff, NULL, TO_ROOM );
        act( "Your $p blazes bright and is gone.", ch, staff, NULL, TO_CHAR );
        extract_obj( staff );
    }

    return;
}

void do_zap( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;
    OBJ_DATA               *wand = NULL;
    OBJ_DATA               *obj = NULL;
    int                     wand_sn = -1;

    one_argument( argument, arg );
    if ( arg[0] == '\0' && ch->fighting == NULL )
    {
        ch_printf( ch, "Zap whom or what?\r\n" );
        return;
    }

    if ( ( wand = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
    {
        ch_printf( ch, "You hold nothing in your hand.\r\n" );
        return;
    }

    if ( wand->item_type != ITEM_WAND )
    {
        ch_printf( ch, "You can zap only with a wand.\r\n" );
        return;
    }

    obj = NULL;
    if ( arg[0] == '\0' )
    {
        if ( ch->fighting != NULL )
        {
            victim = ch->fighting;
        }
        else
        {
            ch_printf( ch, "Zap whom or what?\r\n" );
            return;
        }
    }
    else
    {
        if ( ( victim = get_char_room( ch, arg ) ) == NULL
             && ( obj = get_obj_here( ch, arg ) ) == NULL )
        {
            ch_printf( ch, "You can't find it.\r\n" );
            return;
        }
    }

    if ( ( wand_sn = skill_lookup( "wands" ) ) == -1 )
    {
        log_error( "Can't find the \"%s\" skill in %s?", "wands", __FUNCTION__ );
        return;
    }

    WAIT_STATE( ch, 2 * PULSE_VIOLENCE );

    if ( wand->value[2] > 0 )
    {
        if ( victim != NULL )
        {
            act( "$n zaps $N with $p.", ch, wand, victim, TO_NOTVICT );
            act( "You zap $N with $p.", ch, wand, victim, TO_CHAR );
            act( "$n zaps you with $p.", ch, wand, victim, TO_VICT );
        }
        else
        {
            act( "$n zaps $P with $p.", ch, wand, obj, TO_ROOM );
            act( "You zap $P with $p.", ch, wand, obj, TO_CHAR );
        }

        if ( ch->level < wand->level
             || number_percent(  ) >= 20 + get_skill( ch, wand_sn ) * 4 / 5 )
        {
            act( "Your efforts with $p produce only smoke and sparks.",
                 ch, wand, NULL, TO_CHAR );
            act( "$n's efforts with $p produce only smoke and sparks.",
                 ch, wand, NULL, TO_ROOM );
            check_improve( ch, wand_sn, false, 2 );
        }
        else
        {
            obj_cast_spell( wand->value[3], wand->value[0], ch, victim, obj );
            check_improve( ch, wand_sn, true, 2 );
        }
    }

    if ( --wand->value[2] <= 0 )
    {
        act( "$n's $p explodes into fragments.", ch, wand, NULL, TO_ROOM );
        act( "Your $p explodes into fragments.", ch, wand, NULL, TO_CHAR );
        extract_obj( wand );
    }

    return;
}

void do_steal( CHAR_DATA *ch, const char *argument )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;
    OBJ_DATA               *obj = NULL;
    int                     percent = 0;
    int                     sn = -1;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        ch_printf( ch, "Steal what from whom?\r\n" );
        return;
    }

    if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
    {
        ch_printf( ch, "They aren't here.\r\n" );
        return;
    }

    if ( victim == ch )
    {
        ch_printf( ch, "That's pointless.\r\n" );
        return;
    }

    if ( is_safe( ch, victim ) )
        return;

    if ( IS_NPC( victim ) && victim->position == POS_FIGHTING )
    {
        ch_printf( ch, "Kill stealing is not permitted.\r\n"
                   "You'd better not -- you might get hit.\r\n" );
        return;
    }

    if ( ( sn = skill_lookup( "steal" ) ) == -1 )
    {
        log_error( "Can't find the \"%s\" skill in %s?", "steal", __FUNCTION__ );
        return;
    }

    WAIT_STATE( ch, skill_table[sn].beats );
    percent = number_percent(  );

    if ( !IS_AWAKE( victim ) )
        percent -= 10;
    else if ( !can_see( victim, ch ) )
        percent += 25;
    else
        percent += 50;

    if ( ( ( ch->level + 7 < victim->level || ch->level - 7 > victim->level )
           && !IS_NPC( victim ) && !IS_NPC( ch ) )
         || ( !IS_NPC( ch ) && percent > get_skill( ch, sn ) )
         || ( !IS_NPC( ch ) && !is_clan( ch ) ) )
    {
        /*
         * Failure.
         */
        ch_printf( ch, "Oops.\r\n" );
        affect_strip( ch, skill_lookup( "sneak" ) );
        REMOVE_BIT( ch->affected_by, AFF_SNEAK );

        act_printf( "%s tried to steal from you.\r\n", ch, NULL, victim, TO_VICT,
                    POS_RESTING, false, PERS( ch, victim ) );
        act_printf( "$n tried to steal from $N.\r\n", ch, NULL, victim, TO_NOTVICT,
                    POS_RESTING, true );
        switch ( number_range( 0, 3 ) )
        {
            case 0:
                sprintf( buf, "%s is a lousy thief!", PERS( ch, victim ) );
                break;
            case 1:
                sprintf( buf, "%s couldn't rob %s way out of a paper bag!",
                         PERS( ch, victim ), HISHER( ch ) );
                break;
            case 2:
                sprintf( buf, "%s tried to rob me!", PERS( ch, victim ) );
                break;
            case 3:
                sprintf( buf, "Keep your hands out of there, %s!", PERS( ch, victim ) );
                break;
        }
        if ( !IS_AWAKE( victim ) )
            do_function( victim, &do_wake, "" );
        if ( IS_AWAKE( victim ) )
            do_function( victim, &do_yell, buf );
        if ( !IS_NPC( ch ) )
        {
            if ( IS_NPC( victim ) )
            {
                check_improve( ch, sn, false, 2 );
                multi_hit( victim, ch, TYPE_UNDEFINED );
            }
            else
            {
                wiz_printf( ch, NULL, WIZ_FLAGS, 0, 0, "$N tried to steal from %s.",
                            NAME( victim ) );
                if ( !IS_SET( ch->act, PLR_THIEF ) )
                {
                    SET_BIT( ch->act, PLR_THIEF );
                    ch_printf( ch, "*** You are now a THIEF!! ***\r\n" );
                    save_char_obj( ch );
                }
            }
        }

        return;
    }

    if ( !str_cmp( arg1, "coin" )
         || !str_cmp( arg1, "coins" )
         || !str_cmp( arg1, "gold" ) || !str_cmp( arg1, "silver" ) )
    {
        int                     gold = 0;
        int                     silver = 0;

        gold = victim->gold * number_range( 1, ch->level ) / MAX_LEVEL;
        silver = victim->silver * number_range( 1, ch->level ) / MAX_LEVEL;
        if ( gold <= 0 && silver <= 0 )
        {
            ch_printf( ch, "You couldn't get any coins.\r\n" );
            return;
        }

        ch->gold += gold;
        ch->silver += silver;
        victim->silver -= silver;
        victim->gold -= gold;
        if ( silver <= 0 )
            ch_printf( ch, "Bingo!  You got %d gold coins.\r\n", gold );
        else if ( gold <= 0 )
            ch_printf( ch, "Bingo!  You got %d silver coins.\r\n", silver );
        else
            ch_printf( ch, "Bingo!  You got %d silver and %d gold coins.\r\n",
                       silver, gold );

        check_improve( ch, sn, true, 2 );
        return;
    }

    if ( ( obj = get_obj_carry( victim, arg1, ch ) ) == NULL )
    {
        ch_printf( ch, "You can't find it.\r\n" );
        return;
    }

    if ( !can_drop_obj( ch, obj )
         || IS_SET( obj->extra_flags, ITEM_INVENTORY ) || obj->level > ch->level )
    {
        ch_printf( ch, "You can't pry it away.\r\n" );
        return;
    }

    if ( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) )
    {
        ch_printf( ch, "You have your hands full.\r\n" );
        return;
    }

    if ( ch->carry_weight + get_obj_weight( obj ) > can_carry_w( ch ) )
    {
        ch_printf( ch, "You can't carry that much weight.\r\n" );
        return;
    }

    obj_from_char( obj );
    obj_to_char( obj, ch );
    act( "You pocket $p.", ch, obj, NULL, TO_CHAR );
    check_improve( ch, sn, true, 2 );
    ch_printf( ch, "Got it!\r\n" );
    return;
}

/*
 * Shopping commands.
 */
CHAR_DATA              *find_keeper( CHAR_DATA *ch )
{
    CHAR_DATA              *keeper = NULL;
    SHOP_DATA              *pShop = NULL;

    for ( keeper = ch->in_room->people; keeper; keeper = keeper->next_in_room )
    {
        if ( IS_NPC( keeper ) && ( pShop = keeper->pIndexData->pShop ) != NULL )
            break;
    }

    if ( pShop == NULL )
    {
        ch_printf( ch, "You can't do that here.\r\n" );
        return NULL;
    }

    /*
     * Undesirables.
     *
     if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_KILLER) )
     {
     do_function(keeper, &do_say, "Killers are not welcome!");
     sprintf(buf, "%s the KILLER is over here!\r\n", ch->name);
     do_function(keeper, &do_yell, buf );
     return NULL;
     }
     
     if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_THIEF) )
     {
     do_function(keeper, &do_say, "Thieves are not welcome!");
     sprintf(buf, "%s the THIEF is over here!\r\n", ch->name);
     do_function(keeper, &do_yell, buf );
     return NULL;
     }
     */
    /*
     * Shop hours.
     */
    if ( time_info.hour < pShop->open_hour )
    {
        do_function( keeper, &do_say, "Sorry, I am closed. Come back later." );
        return NULL;
    }

    if ( time_info.hour > pShop->close_hour )
    {
        do_function( keeper, &do_say, "Sorry, I am closed. Come back tomorrow." );
        return NULL;
    }

    /*
     * Invisible or hidden people.
     */
    if ( !can_see( keeper, ch ) )
    {
        do_function( keeper, &do_say, "I don't trade with folks I can't see." );
        return NULL;
    }

    return keeper;
}

/* insert an object at the right spot for the keeper */
void obj_to_keeper( OBJ_DATA *obj, CHAR_DATA *ch )
{
    OBJ_DATA               *t_obj = NULL;
    OBJ_DATA               *t_obj_next = NULL;

    /*
     * see if any duplicates are found 
     */
    for ( t_obj = ch->carrying; t_obj != NULL; t_obj = t_obj_next )
    {
        t_obj_next = t_obj->next_content;

        if ( obj->pIndexData == t_obj->pIndexData
             && !str_cmp( obj->short_descr, t_obj->short_descr ) )
        {
            /*
             * if this is an unlimited item, destroy the new one 
             */
            if ( IS_OBJ_STAT( t_obj, ITEM_INVENTORY ) )
            {
                extract_obj( obj );
                return;
            }
            obj->cost = t_obj->cost;                   /* keep it standard */
            break;
        }
    }

    if ( t_obj == NULL )
    {
        obj->next_content = ch->carrying;
        ch->carrying = obj;
    }
    else
    {
        obj->next_content = t_obj->next_content;
        t_obj->next_content = obj;
    }

    obj->carried_by = ch;
    obj->in_room = NULL;
    obj->in_obj = NULL;
    ch->carry_number += get_obj_number( obj );
    ch->carry_weight += get_obj_weight( obj );
}

/* get an object from a shopkeeper's list */
OBJ_DATA               *get_obj_keeper( CHAR_DATA *ch, CHAR_DATA *keeper, char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    OBJ_DATA               *obj = NULL;
    int                     number = 0;
    int                     count = 0;

    number = number_argument( argument, arg );
    for ( obj = keeper->carrying; obj != NULL; obj = obj->next_content )
    {
        if ( obj->wear_loc == WEAR_NONE
             && can_see_obj( keeper, obj )
             && can_see_obj( ch, obj ) && is_name( arg, obj->name ) )
        {
            if ( ++count == number )
                return obj;

            /*
             * skip other objects of the same name 
             */
            while ( obj->next_content != NULL
                    && obj->pIndexData == obj->next_content->pIndexData
                    && !str_cmp( obj->short_descr, obj->next_content->short_descr ) )
                obj = obj->next_content;
        }
    }

    return NULL;
}

int get_cost( CHAR_DATA *keeper, OBJ_DATA *obj, bool fBuy )
{
    SHOP_DATA              *pShop = NULL;
    int                     cost = 0;

    if ( obj == NULL || ( pShop = keeper->pIndexData->pShop ) == NULL )
        return 0;

    if ( fBuy )
    {
        cost = obj->cost * pShop->profit_buy / 100;
    }
    else
    {
        OBJ_DATA               *obj2 = NULL;
        int                     itype = 0;

        cost = 0;
        for ( itype = 0; itype < MAX_TRADE; itype++ )
        {
            if ( obj->item_type == pShop->buy_type[itype] )
            {
                cost = obj->cost * pShop->profit_sell / 100;
                break;
            }
        }

        if ( !IS_OBJ_STAT( obj, ITEM_SELL_EXTRACT ) )
            for ( obj2 = keeper->carrying; obj2; obj2 = obj2->next_content )
            {
                if ( obj->pIndexData == obj2->pIndexData
                     && !str_cmp( obj->short_descr, obj2->short_descr ) )
                {
                    if ( IS_OBJ_STAT( obj2, ITEM_INVENTORY ) )
                        cost /= 2;
                    else
                        cost = cost * 3 / 4;
                }
            }
    }

    if ( obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND )
    {
        if ( obj->value[1] == 0 )
            cost /= 4;
        else
            cost = cost * obj->value[2] / obj->value[1];
    }

    return cost;
}

void do_buy( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    int                     cost = 0;
    int                     roll = 0;
    int                     sn = -1;

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Buy what?\r\n" );
        return;
    }

    if ( ( sn = skill_lookup( "haggle" ) ) == -1 )
    {
        log_error( "Can't find the \"%s\" skill in %s?", "haggle", __FUNCTION__ );
        return;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_PET_SHOP ) )
    {
        CHAR_DATA              *pet = NULL;
        ROOM_INDEX_DATA        *pRoomIndexNext = NULL;
        ROOM_INDEX_DATA        *in_room = NULL;
        char                    local_argument[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
        const char             *lap = local_argument;

        strcpy( local_argument, argument );
        smash_tilde( local_argument );

        if ( IS_NPC( ch ) )
            return;

        lap = one_argument( local_argument, arg );

        /*
         * hack to make new thalos pets work 
         */
        if ( ch->in_room->vnum == 9621 )
            pRoomIndexNext = get_room_index( 9706 );
        else
            pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 );
        if ( pRoomIndexNext == NULL )
        {
            log_error( "Bad pet shop at vnum %d", ch->in_room->vnum );
            ch_printf( ch, "Sorry, you can't buy that here.\r\n" );
            return;
        }

        in_room = ch->in_room;
        ch->in_room = pRoomIndexNext;
        pet = get_char_room( ch, arg );
        ch->in_room = in_room;

        if ( pet == NULL || !IS_SET( pet->act, ACT_PET ) )
        {
            ch_printf( ch, "Sorry, you can't buy that here.\r\n" );
            return;
        }

        if ( ch->pet != NULL )
        {
            ch_printf( ch, "You already own a pet.\r\n" );
            return;
        }

        cost = 10 * pet->level * pet->level;

        if ( ( ch->silver + 100 * ch->gold ) < cost )
        {
            ch_printf( ch, "You can't afford it.\r\n" );
            return;
        }

        if ( ch->level < pet->level )
        {
            ch_printf( ch, "You're not powerful enough to master this pet.\r\n" );
            return;
        }

        /*
         * haggle 
         */
        roll = number_percent(  );
        if ( roll < get_skill( ch, sn ) )
        {
            cost -= cost / 2 * roll / 100;
            ch_printf( ch, "You haggle the price down to %d coins.\r\n", cost );
            check_improve( ch, sn, true, 4 );

        }

        deduct_cost( ch, cost );
        pet = create_mobile( pet->pIndexData );
        SET_BIT( pet->act, ACT_PET );
        SET_BIT( pet->affected_by, AFF_CHARM );
        pet->comm = COMM_NOTELL | COMM_NOSHOUT | COMM_NOCHANNELS;

        lap = one_argument( lap, arg );
        if ( arg[0] != '\0' )
        {
            sprintf( buf, "%s %s", pet->name, arg );
            free_string( pet->name );
            pet->name = str_dup( buf );
        }

        sprintf( buf, "%sA neck tag says 'I belong to %s'.\r\n",
                 pet->description, ch->name );
        free_string( pet->description );
        pet->description = str_dup( buf );

        char_to_room( pet, ch->in_room );
        add_follower( pet, ch );
        pet->leader = ch;
        ch->pet = pet;
        ch_printf( ch, "Enjoy your pet.\r\n" );
        act( "$n bought $N as a pet.", ch, NULL, pet, TO_ROOM );
        return;
    }
    else
    {
        CHAR_DATA              *keeper = NULL;
        OBJ_DATA               *obj = NULL;
        OBJ_DATA               *t_obj = NULL;
        int                     number = 0;
        int                     count = 1;

        if ( ( keeper = find_keeper( ch ) ) == NULL )
            return;

        number = mult_argument( argument, arg );
        obj = get_obj_keeper( ch, keeper, arg );
        cost = get_cost( keeper, obj, true );

        if ( number < 1 || number > 99 )
        {
            act( "$n tells you 'Get real!", keeper, NULL, ch, TO_VICT );
            return;
        }

        if ( cost <= 0 || !can_see_obj( ch, obj ) )
        {
            act( "$n tells you 'I don't sell that -- try 'list''.",
                 keeper, NULL, ch, TO_VICT );
            ch->reply = keeper;
            return;
        }

        if ( !IS_OBJ_STAT( obj, ITEM_INVENTORY ) )
        {
            for ( t_obj = obj->next_content;
                  count < number && t_obj != NULL; t_obj = t_obj->next_content )
            {
                if ( t_obj->pIndexData == obj->pIndexData
                     && !str_cmp( t_obj->short_descr, obj->short_descr ) )
                    count++;
                else
                    break;
            }

            if ( count < number )
            {
                act( "$n tells you 'I don't have that many in stock.",
                     keeper, NULL, ch, TO_VICT );
                ch->reply = keeper;
                return;
            }
        }

        if ( ( ch->silver + ch->gold * 100 ) < cost * number )
        {
            if ( number > 1 )
                act( "$n tells you 'You can't afford to buy that many.",
                     keeper, obj, ch, TO_VICT );
            else
                act( "$n tells you 'You can't afford to buy $p'.",
                     keeper, obj, ch, TO_VICT );
            ch->reply = keeper;
            return;
        }

        if ( obj->level > ch->level )
        {
            act( "$n tells you 'You can't use $p yet'.", keeper, obj, ch, TO_VICT );
            ch->reply = keeper;
            return;
        }

        if ( ch->carry_number + number * get_obj_number( obj ) > can_carry_n( ch ) )
        {
            ch_printf( ch, "You can't carry that many items.\r\n" );
            return;
        }

        if ( ch->carry_weight + number * get_obj_weight( obj ) > can_carry_w( ch ) )
        {
            ch_printf( ch, "You can't carry that much weight.\r\n" );
            return;
        }

        /*
         * haggle 
         */
        roll = number_percent(  );
        if ( !IS_OBJ_STAT( obj, ITEM_SELL_EXTRACT ) && roll < get_skill( ch, sn ) )
        {
            cost -= obj->cost / 2 * roll / 100;
            act( "You haggle with $N.", ch, NULL, keeper, TO_CHAR );
            check_improve( ch, sn, true, 4 );
        }

        if ( number > 1 )
        {
            act_printf( "$n buys $p[%d].", ch, obj, NULL, TO_ROOM, POS_RESTING, false,
                        number );
            act_printf( "You buy $p[%d] for %d silver.", ch, obj, NULL, TO_CHAR,
                        POS_RESTING, false, number, cost * number );
        }
        else
        {
            act( "$n buys $p.", ch, obj, NULL, TO_ROOM );
            act_printf( "You buy $p for %d silver.", ch, obj, NULL, TO_CHAR, POS_RESTING,
                        false, cost );
        }
        deduct_cost( ch, cost * number );
        keeper->gold += cost * number / 100;
        keeper->silver += cost * number - ( cost * number / 100 ) * 100;

        for ( count = 0; count < number; count++ )
        {
            if ( IS_SET( obj->extra_flags, ITEM_INVENTORY ) )
                t_obj = create_object( obj->pIndexData, obj->level );
            else
            {
                t_obj = obj;
                obj = obj->next_content;
                obj_from_char( t_obj );
            }

            if ( t_obj->timer > 0 && !IS_OBJ_STAT( t_obj, ITEM_HAD_TIMER ) )
                t_obj->timer = 0;
            REMOVE_BIT( t_obj->extra_flags, ITEM_HAD_TIMER );
            obj_to_char( t_obj, ch );
            if ( cost < t_obj->cost )
                t_obj->cost = cost;
        }
    }
}

void do_list( CHAR_DATA *ch, const char *argument )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    if ( IS_SET( ch->in_room->room_flags, ROOM_PET_SHOP ) )
    {
        ROOM_INDEX_DATA        *pRoomIndexNext = NULL;
        CHAR_DATA              *pet = NULL;
        bool                    found = false;

        /*
         * hack to make new thalos pets work 
         */
        if ( ch->in_room->vnum == 9621 )
            pRoomIndexNext = get_room_index( 9706 );
        else
            pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 );

        if ( pRoomIndexNext == NULL )
        {
            log_error( "Bad pet shop at vnum %d", ch->in_room->vnum );
            ch_printf( ch, "You can't do that here.\r\n" );
            return;
        }

        found = false;
        for ( pet = pRoomIndexNext->people; pet; pet = pet->next_in_room )
        {
            if ( IS_SET( pet->act, ACT_PET ) )
            {
                if ( !found )
                {
                    found = true;
                    ch_printf( ch, "Pets for sale:\r\n" );
                }
                ch_printf( ch, "[%2d] %8d - %s\r\n",
                           pet->level, 10 * pet->level * pet->level, pet->short_descr );
            }
        }
        if ( !found )
            ch_printf( ch, "Sorry, we're out of pets right now.\r\n" );
        return;
    }
    else
    {
        CHAR_DATA              *keeper = NULL;
        OBJ_DATA               *obj = NULL;
        int                     cost = 0;
        int                     count = 0;
        bool                    found = false;
        char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";

        if ( ( keeper = find_keeper( ch ) ) == NULL )
            return;
        one_argument( argument, arg );

        found = false;
        for ( obj = keeper->carrying; obj; obj = obj->next_content )
        {
            if ( obj->wear_loc == WEAR_NONE
                 && can_see_obj( ch, obj )
                 && ( cost = get_cost( keeper, obj, true ) ) > 0
                 && ( arg[0] == '\0' || is_name( arg, obj->name ) ) )
            {
                if ( !found )
                {
                    found = true;
                    ch_printf( ch, "[Lv Price Qty] Item\r\n" );
                }

                if ( IS_OBJ_STAT( obj, ITEM_INVENTORY ) )
                    sprintf( buf, "[%2d %5d -- ] %s\r\n",
                             obj->level, cost, obj->short_descr );
                else
                {
                    count = 1;

                    while ( obj->next_content != NULL
                            && obj->pIndexData == obj->next_content->pIndexData
                            && !str_cmp( obj->short_descr,
                                         obj->next_content->short_descr ) )
                    {
                        obj = obj->next_content;
                        count++;
                    }
                    sprintf( buf, "[%2d %5d %2d ] %s\r\n",
                             obj->level, cost, count, obj->short_descr );
                }
                send_to_char( buf, ch );
            }
        }

        if ( !found )
            ch_printf( ch, "You can't buy anything here.\r\n" );
        return;
    }
}

void do_sell( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *keeper = NULL;
    OBJ_DATA               *obj = NULL;
    int                     cost = 0;
    int                     roll = 0;
    int                     sn = -1;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Sell what?\r\n" );
        return;
    }

    if ( ( keeper = find_keeper( ch ) ) == NULL )
        return;

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
        act( "$n tells you 'You don't have that item'.", keeper, NULL, ch, TO_VICT );
        ch->reply = keeper;
        return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
        ch_printf( ch, "You can't let go of it.\r\n" );
        return;
    }

    if ( !can_see_obj( keeper, obj ) )
    {
        act( "$n doesn't see what you are offering.", keeper, NULL, ch, TO_VICT );
        return;
    }

    if ( ( cost = get_cost( keeper, obj, false ) ) <= 0 )
    {
        act( "$n looks uninterested in $p.", keeper, obj, ch, TO_VICT );
        return;
    }

    if ( ( sn = skill_lookup( "haggle" ) ) == -1 )
    {
        log_error( "Can't find the \"%s\" skill in %s?", "haggle", __FUNCTION__ );
        return;
    }

    if ( cost > ( keeper->silver + 100 * keeper->gold ) )
    {
        act( "$n tells you 'I'm afraid I don't have enough wealth to buy $p.",
             keeper, obj, ch, TO_VICT );
        return;
    }

    act( "$n sells $p.", ch, obj, NULL, TO_ROOM );
    /*
     * haggle 
     */
    roll = number_percent(  );
    if ( !IS_OBJ_STAT( obj, ITEM_SELL_EXTRACT ) && roll < get_skill( ch, sn ) )
    {
        ch_printf( ch, "You haggle with the shopkeeper.\r\n" );
        cost += obj->cost / 2 * roll / 100;
        cost = UMIN( cost, 95 * get_cost( keeper, obj, true ) / 100 );
        cost = UMIN( cost, ( keeper->silver + 100 * keeper->gold ) );
        check_improve( ch, sn, true, 4 );
    }
    act_printf( "You sell $p for %d silver and %d gold piece%s.", ch, obj, NULL, TO_CHAR,
                POS_RESTING, false, cost - ( cost / 100 ) * 100, cost / 100,
                cost == 1 ? "" : "s" );
    ch->gold += cost / 100;
    ch->silver += cost - ( cost / 100 ) * 100;
    deduct_cost( keeper, cost );
    if ( keeper->gold < 0 )
        keeper->gold = 0;
    if ( keeper->silver < 0 )
        keeper->silver = 0;

    if ( obj->item_type == ITEM_TRASH || IS_OBJ_STAT( obj, ITEM_SELL_EXTRACT ) )
    {
        extract_obj( obj );
    }
    else
    {
        obj_from_char( obj );
        if ( obj->timer )
            SET_BIT( obj->extra_flags, ITEM_HAD_TIMER );
        else
            obj->timer = number_range( 50, 100 );
        obj_to_keeper( obj, keeper );
    }

    return;
}

void do_value( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *keeper = NULL;
    OBJ_DATA               *obj = NULL;
    int                     cost = 0;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Value what?\r\n" );
        return;
    }

    if ( ( keeper = find_keeper( ch ) ) == NULL )
        return;

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
        act( "$n tells you 'You don't have that item'.", keeper, NULL, ch, TO_VICT );
        ch->reply = keeper;
        return;
    }

    if ( !can_see_obj( keeper, obj ) )
    {
        act( "$n doesn't see what you are offering.", keeper, NULL, ch, TO_VICT );
        return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
        ch_printf( ch, "You can't let go of it.\r\n" );
        return;
    }

    if ( ( cost = get_cost( keeper, obj, false ) ) <= 0 )
    {
        act( "$n looks uninterested in $p.", keeper, obj, ch, TO_VICT );
        return;
    }

    act_printf( "$n tells you 'I'll give you %d silver and %d gold coins for $p'.",
                keeper, obj, ch, TO_VICT, POS_RESTING, false,
                cost - ( cost / 100 ) * 100, cost / 100 );
    ch->reply = keeper;

    return;
}
