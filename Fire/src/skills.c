/*
 * RAM $Id: skills.c 64 2008-12-11 14:35:49Z quixadhal $
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

/* stuff for recycling gen_data */
GEN_DATA               *gen_data_free = NULL;

GEN_DATA               *new_gen_data( void )
{
    static GEN_DATA         gen_zero;
    GEN_DATA               *gen = NULL;

    if ( gen_data_free == NULL )
        gen = ( GEN_DATA * ) alloc_perm( sizeof( *gen ) );
    else
    {
        gen = gen_data_free;
        gen_data_free = gen_data_free->next;
    }
    *gen = gen_zero;
    VALIDATE( gen );
    return gen;
}

void free_gen_data( GEN_DATA *gen )
{
    if ( !IS_VALID( gen ) )
        return;

    INVALIDATE( gen );

    gen->next = gen_data_free;
    gen_data_free = gen;
}

/* used to get new skills */
void do_gain( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *trainer = NULL;
    int                     gn = -1;
    int                     sn = -1;

    if ( IS_NPC( ch ) )
        return;

    /*
     * find a trainer 
     */
    for ( trainer = ch->in_room->people;
          trainer != NULL; trainer = trainer->next_in_room )
        if ( IS_NPC( trainer ) && IS_SET( trainer->act, ACT_GAIN ) )
            break;

    if ( trainer == NULL || !can_see( ch, trainer ) )
    {
        ch_printf( ch, "You can't do that here.\r\n" );
        return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        do_function( trainer, &do_say, "Pardon me?" );
        return;
    }

    if ( !str_prefix( arg, "list" ) )
    {
        int                     col = 0;

        ch_printf( ch, "%-18s %-5s %-18s %-5s %-18s %-5s\r\n",
                   "group", "cost", "group", "cost", "group", "cost" );

        for ( gn = 0; gn < MAX_GROUP; gn++ )
        {
            if ( group_table[gn].name == NULL )
                break;

            if ( !ch->pcdata->group_known[gn] && group_table[gn].rating[ch->iclass] > 0 )
            {
                ch_printf( ch, "%-18s %-5d ",
                           group_table[gn].name, group_table[gn].rating[ch->iclass] );
                if ( ++col % 3 == 0 )
                    ch_printf( ch, "\r\n" );
            }
        }
        if ( col % 3 != 0 )
            ch_printf( ch, "\r\n" );

        ch_printf( ch, "\r\n" );

        col = 0;

        ch_printf( ch, "%-18s %-5s %-18s %-5s %-18s %-5s\r\n",
                   "skill", "cost", "skill", "cost", "skill", "cost" );

        for ( sn = 0; sn < MAX_SKILL; sn++ )
        {
            if ( skill_table[sn].name == NULL )
                break;

            if ( !ch->pcdata->learned[sn]
                 && skill_table[sn].rating[ch->iclass] > 0
                 && skill_table[sn].spell_fun == spell_null )
            {
                ch_printf( ch, "%-18s %-5d ",
                           skill_table[sn].name, skill_table[sn].rating[ch->iclass] );
                if ( ++col % 3 == 0 )
                    ch_printf( ch, "\r\n" );
            }
        }
        if ( col % 3 != 0 )
            ch_printf( ch, "\r\n" );
        return;
    }

    if ( !str_prefix( arg, "convert" ) )
    {
        if ( ch->practice < 10 )
        {
            act( "$N tells you 'You are not yet ready.'", ch, NULL, trainer, TO_CHAR );
            return;
        }

        act( "$N helps you apply your practice to training", ch, NULL, trainer, TO_CHAR );
        ch->practice -= 10;
        ch->train += 1;
        return;
    }

    if ( !str_prefix( arg, "points" ) )
    {
        if ( ch->train < 2 )
        {
            act( "$N tells you 'You are not yet ready.'", ch, NULL, trainer, TO_CHAR );
            return;
        }

        if ( ch->pcdata->points <= 40 )
        {
            act( "$N tells you 'There would be no point in that.'",
                 ch, NULL, trainer, TO_CHAR );
            return;
        }

        act( "$N trains you, and you feel more at ease with your skills.",
             ch, NULL, trainer, TO_CHAR );

        ch->train -= 2;
        ch->pcdata->points -= 1;
        ch->exp = exp_per_level( ch, ch->pcdata->points ) * ch->level;
        return;
    }

    /*
     * else add a group/skill 
     */

    gn = group_lookup( argument );
    if ( gn > 0 )
    {
        if ( ch->pcdata->group_known[gn] )
        {
            act( "$N tells you 'You already know that group!'",
                 ch, NULL, trainer, TO_CHAR );
            return;
        }

        if ( group_table[gn].rating[ch->iclass] <= 0 )
        {
            act( "$N tells you 'That group is beyond your powers.'",
                 ch, NULL, trainer, TO_CHAR );
            return;
        }

        if ( ch->train < group_table[gn].rating[ch->iclass] )
        {
            act( "$N tells you 'You are not yet ready for that group.'",
                 ch, NULL, trainer, TO_CHAR );
            return;
        }

        /*
         * add the group 
         */
        gn_add( ch, gn );
        act( "$N trains you in the art of $t",
             ch, group_table[gn].name, trainer, TO_CHAR );
        ch->train -= group_table[gn].rating[ch->iclass];
        return;
    }

    sn = skill_lookup( argument );
    if ( sn > -1 )
    {
        if ( skill_table[sn].spell_fun != spell_null )
        {
            act( "$N tells you 'You must learn the full group.'",
                 ch, NULL, trainer, TO_CHAR );
            return;
        }

        if ( ch->pcdata->learned[sn] )
        {
            act( "$N tells you 'You already know that skill!'",
                 ch, NULL, trainer, TO_CHAR );
            return;
        }

        if ( skill_table[sn].rating[ch->iclass] <= 0 )
        {
            act( "$N tells you 'That skill is beyond your powers.'",
                 ch, NULL, trainer, TO_CHAR );
            return;
        }

        if ( ch->train < skill_table[sn].rating[ch->iclass] )
        {
            act( "$N tells you 'You are not yet ready for that skill.'",
                 ch, NULL, trainer, TO_CHAR );
            return;
        }

        /*
         * add the skill 
         */
        ch->pcdata->learned[sn] = 1;
        act( "$N trains you in the art of $t",
             ch, skill_table[sn].name, trainer, TO_CHAR );
        ch->train -= skill_table[sn].rating[ch->iclass];
        return;
    }

    act( "$N tells you 'I do not understand...'", ch, NULL, trainer, TO_CHAR );
}

/* RT spells and skills show the players spells (or skills) */
void do_spells( CHAR_DATA *ch, const char *argument )
{
    BUFFER                 *buffer = NULL;
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    spell_list[LEVEL_HERO + 1][MAX_STRING_LENGTH];
    char                    spell_columns[LEVEL_HERO + 1];
    int                     sn = 0;
    int                     level = 0;
    int                     min_lev = 1;
    int                     max_lev = LEVEL_HERO;
    int                     mana = 0;
    bool                    fAll = false;
    bool                    found = false;
    char                    buf[MAX_STRING_LENGTH];

    if ( IS_NPC( ch ) )
        return;

    if ( argument[0] != '\0' )
    {
        fAll = true;

        if ( str_prefix( argument, "all" ) )
        {
            argument = one_argument( argument, arg );
            if ( !is_number( arg ) )
            {
                ch_printf( ch, "Arguments must be numerical or all.\r\n" );
                return;
            }
            max_lev = atoi( arg );

            if ( max_lev < 1 || max_lev > LEVEL_HERO )
            {
                ch_printf( ch, "Levels must be between 1 and %d.\r\n", LEVEL_HERO );
                return;
            }

            if ( argument[0] != '\0' )
            {
                argument = one_argument( argument, arg );
                if ( !is_number( arg ) )
                {
                    ch_printf( ch, "Arguments must be numerical or all.\r\n" );
                    return;
                }
                min_lev = max_lev;
                max_lev = atoi( arg );

                if ( max_lev < 1 || max_lev > LEVEL_HERO )
                {
                    ch_printf( ch, "Levels must be between 1 and %d.\r\n", LEVEL_HERO );
                    return;
                }

                if ( min_lev > max_lev )
                {
                    ch_printf( ch, "That would be silly.\r\n" );
                    return;
                }
            }
        }
    }

    /*
     * initialize data 
     */
    for ( level = 0; level < LEVEL_HERO + 1; level++ )
    {
        spell_columns[level] = 0;
        spell_list[level][0] = '\0';
    }

    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if ( skill_table[sn].name == NULL )
            break;

        if ( ( level = skill_table[sn].skill_level[ch->iclass] ) < LEVEL_HERO + 1
             && ( fAll || level <= ch->level )
             && level >= min_lev && level <= max_lev
             && skill_table[sn].spell_fun != spell_null && ch->pcdata->learned[sn] > 0 )
        {
            found = true;
            level = skill_table[sn].skill_level[ch->iclass];
            if ( ch->level < level )
                sprintf( buf, "%-18s n/a      ", skill_table[sn].name );
            else
            {
                mana = UMAX( skill_table[sn].min_mana, 100 / ( 2 + ch->level - level ) );
                sprintf( buf, "%-18s  %3d mana  ", skill_table[sn].name, mana );
            }

            if ( spell_list[level][0] == '\0' )
                sprintf( spell_list[level], "\r\nLevel %2d: %s", level, buf );
            else                                       /* append */
            {
                if ( ++spell_columns[level] % 2 == 0 )
                    strcat( spell_list[level], "\r\n          " );
                strcat( spell_list[level], buf );
            }
        }
    }

    /*
     * return results 
     */

    if ( !found )
    {
        ch_printf( ch, "No spells found.\r\n" );
        return;
    }

    buffer = new_buf(  );
    for ( level = 0; level < LEVEL_HERO + 1; level++ )
        if ( spell_list[level][0] != '\0' )
            add_buf( buffer, spell_list[level] );
    add_buf( buffer, "\r\n" );
    page_to_char( buf_string( buffer ), ch );
    free_buf( buffer );
}

void do_skills( CHAR_DATA *ch, const char *argument )
{
    BUFFER                 *buffer = NULL;
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    skill_list[LEVEL_HERO + 1][MAX_STRING_LENGTH];
    char                    skill_columns[LEVEL_HERO + 1];
    int                     sn = -1;
    int                     level = 0;
    int                     min_lev = 1;
    int                     max_lev = LEVEL_HERO;
    bool                    fAll = false;
    bool                    found = false;
    char                    buf[MAX_STRING_LENGTH];

    if ( IS_NPC( ch ) )
        return;

    if ( argument[0] != '\0' )
    {
        fAll = true;

        if ( str_prefix( argument, "all" ) )
        {
            argument = one_argument( argument, arg );
            if ( !is_number( arg ) )
            {
                ch_printf( ch, "Arguments must be numerical or all.\r\n" );
                return;
            }
            max_lev = atoi( arg );

            if ( max_lev < 1 || max_lev > LEVEL_HERO )
            {
                ch_printf( ch, "Levels must be between 1 and %d.\r\n", LEVEL_HERO );
                return;
            }

            if ( argument[0] != '\0' )
            {
                argument = one_argument( argument, arg );
                if ( !is_number( arg ) )
                {
                    ch_printf( ch, "Arguments must be numerical or all.\r\n" );
                    return;
                }
                min_lev = max_lev;
                max_lev = atoi( arg );

                if ( max_lev < 1 || max_lev > LEVEL_HERO )
                {
                    ch_printf( ch, "Levels must be between 1 and %d.\r\n", LEVEL_HERO );
                    return;
                }

                if ( min_lev > max_lev )
                {
                    ch_printf( ch, "That would be silly.\r\n" );
                    return;
                }
            }
        }
    }

    /*
     * initialize data 
     */
    for ( level = 0; level < LEVEL_HERO + 1; level++ )
    {
        skill_columns[level] = 0;
        skill_list[level][0] = '\0';
    }

    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if ( skill_table[sn].name == NULL )
            break;

        if ( ( level = skill_table[sn].skill_level[ch->iclass] ) < LEVEL_HERO + 1
             && ( fAll || level <= ch->level )
             && level >= min_lev && level <= max_lev
             && skill_table[sn].spell_fun == spell_null && ch->pcdata->learned[sn] > 0 )
        {
            found = true;
            level = skill_table[sn].skill_level[ch->iclass];
            if ( ch->level < level )
                sprintf( buf, "%-18s n/a      ", skill_table[sn].name );
            else
                sprintf( buf, "%-18s %3d%%      ", skill_table[sn].name,
                         ch->pcdata->learned[sn] );

            if ( skill_list[level][0] == '\0' )
                sprintf( skill_list[level], "\r\nLevel %2d: %s", level, buf );
            else                                       /* append */
            {
                if ( ++skill_columns[level] % 2 == 0 )
                    strcat( skill_list[level], "\r\n          " );
                strcat( skill_list[level], buf );
            }
        }
    }

    /*
     * return results 
     */

    if ( !found )
    {
        ch_printf( ch, "No skills found.\r\n" );
        return;
    }

    buffer = new_buf(  );
    for ( level = 0; level < LEVEL_HERO + 1; level++ )
        if ( skill_list[level][0] != '\0' )
            add_buf( buffer, skill_list[level] );
    add_buf( buffer, "\r\n" );
    page_to_char( buf_string( buffer ), ch );
    free_buf( buffer );
}

/* shows skills, groups and costs (only if not bought) */
void list_group_costs( CHAR_DATA *ch )
{
    int                     gn = -1;
    int                     sn = -1;
    int                     col = 0;

    if ( IS_NPC( ch ) )
        return;

    ch_printf( ch, "%-18s %-5s %-18s %-5s %-18s %-5s\r\n", "group", "cp", "group", "cp",
               "group", "cp" );

    for ( gn = 0; gn < MAX_GROUP; gn++ )
    {
        if ( group_table[gn].name == NULL )
            break;

        if ( !ch->gen_data->group_chosen[gn]
             && !ch->pcdata->group_known[gn] && group_table[gn].rating[ch->iclass] > 0 )
        {
            ch_printf( ch, "%-18s %-5d ", group_table[gn].name,
                       group_table[gn].rating[ch->iclass] );
            if ( ++col % 3 == 0 )
                ch_printf( ch, "\r\n" );
        }
    }
    if ( col % 3 != 0 )
        ch_printf( ch, "\r\n" );
    ch_printf( ch, "\r\n" );

    col = 0;

    ch_printf( ch, "%-18s %-5s %-18s %-5s %-18s %-5s\r\n", "skill", "cp", "skill", "cp",
               "skill", "cp" );

    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if ( skill_table[sn].name == NULL )
            break;

        if ( !ch->gen_data->skill_chosen[sn]
             && ch->pcdata->learned[sn] == 0
             && skill_table[sn].spell_fun == spell_null
             && skill_table[sn].rating[ch->iclass] > 0 )
        {
            ch_printf( ch, "%-18s %-5d ", skill_table[sn].name,
                       skill_table[sn].rating[ch->iclass] );
            if ( ++col % 3 == 0 )
                ch_printf( ch, "\r\n" );
        }
    }
    if ( col % 3 != 0 )
        ch_printf( ch, "\r\n" );
    ch_printf( ch, "\r\n" );

    ch_printf( ch, "Creation points: %d\r\n", ch->pcdata->points );
    ch_printf( ch, "Experience per level: %d\r\n",
               exp_per_level( ch, ch->gen_data->points_chosen ) );
    return;
}

void list_group_chosen( CHAR_DATA *ch )
{
    int                     gn = -1;
    int                     sn = -1;
    int                     col = 0;

    if ( IS_NPC( ch ) )
        return;

    ch_printf( ch, "%-18s %-5s %-18s %-5s %-18s %-5s", "group", "cp", "group", "cp",
               "group", "cp\r\n" );

    for ( gn = 0; gn < MAX_GROUP; gn++ )
    {
        if ( group_table[gn].name == NULL )
            break;

        if ( ch->gen_data->group_chosen[gn] && group_table[gn].rating[ch->iclass] > 0 )
        {
            ch_printf( ch, "%-18s %-5d ", group_table[gn].name,
                       group_table[gn].rating[ch->iclass] );
            if ( ++col % 3 == 0 )
                ch_printf( ch, "\r\n" );
        }
    }
    if ( col % 3 != 0 )
        ch_printf( ch, "\r\n" );
    ch_printf( ch, "\r\n" );

    col = 0;

    ch_printf( ch, "%-18s %-5s %-18s %-5s %-18s %-5s", "skill", "cp", "skill", "cp",
               "skill", "cp\r\n" );

    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if ( skill_table[sn].name == NULL )
            break;

        if ( ch->gen_data->skill_chosen[sn] && skill_table[sn].rating[ch->iclass] > 0 )
        {
            ch_printf( ch, "%-18s %-5d ", skill_table[sn].name,
                       skill_table[sn].rating[ch->iclass] );
            if ( ++col % 3 == 0 )
                ch_printf( ch, "\r\n" );
        }
    }
    if ( col % 3 != 0 )
        ch_printf( ch, "\r\n" );
    ch_printf( ch, "\r\n" );

    ch_printf( ch, "Creation points: %d\r\n", ch->gen_data->points_chosen );
    ch_printf( ch, "Experience per level: %d\r\n",
               exp_per_level( ch, ch->gen_data->points_chosen ) );
    return;
}

int exp_per_level( CHAR_DATA *ch, int points )
{
    int                     expl = 1000;
    int                     inc = 500;

    if ( IS_NPC( ch ) )
        return 1000;

    if ( points < 40 )
        return 1000 * ( pc_race_table[ch->race].class_mult[ch->iclass] ?
                        pc_race_table[ch->race].class_mult[ch->iclass] / 100 : 1 );

    /*
     * processing 
     */
    points -= 40;

    while ( points > 9 )
    {
        expl += inc;
        points -= 10;
        if ( points > 9 )
        {
            expl += inc;
            inc *= 2;
            points -= 10;
        }
    }

    expl += points * inc / 10;

    return expl * pc_race_table[ch->race].class_mult[ch->iclass] / 100;
}

/* this procedure handles the input parsing for the skill generator */
bool parse_gen_groups( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    int                     gn = -1;
    int                     sn = -1;
    int                     i = 0;

    if ( argument[0] == '\0' )
        return false;

    argument = one_argument( argument, arg );

    if ( !str_prefix( arg, "help" ) )
    {
        if ( argument[0] == '\0' )
        {
            do_function( ch, &do_help, "group help" );
            return true;
        }

        do_function( ch, &do_help, argument );
        return true;
    }

    if ( !str_prefix( arg, "add" ) )
    {
        if ( argument[0] == '\0' )
        {
            ch_printf( ch, "You must provide a skill name.\r\n" );
            return true;
        }

        gn = group_lookup( argument );
        if ( gn != -1 )
        {
            if ( ch->gen_data->group_chosen[gn] || ch->pcdata->group_known[gn] )
            {
                ch_printf( ch, "You already know that group!\r\n" );
                return true;
            }

            if ( group_table[gn].rating[ch->iclass] < 1 )
            {
                ch_printf( ch, "That group is not available.\r\n" );
                return true;
            }

            /*
             * Close security hole 
             */
            if ( ch->gen_data->points_chosen + group_table[gn].rating[ch->iclass] > 300 )
            {
                ch_printf( ch, "You cannot take more than 300 creation points.\r\n" );
                return true;
            }

            ch_printf( ch, "%s group added\r\n", group_table[gn].name );
            ch->gen_data->group_chosen[gn] = true;
            ch->gen_data->points_chosen += group_table[gn].rating[ch->iclass];
            gn_add( ch, gn );
            ch->pcdata->points += group_table[gn].rating[ch->iclass];
            return true;
        }

        sn = skill_lookup( argument );
        if ( sn != -1 )
        {
            if ( ch->gen_data->skill_chosen[sn] || ch->pcdata->learned[sn] > 0 )
            {
                ch_printf( ch, "You already know that skill!\r\n" );
                return true;
            }

            if ( skill_table[sn].rating[ch->iclass] < 1
                 || skill_table[sn].spell_fun != spell_null )
            {
                ch_printf( ch, "That skill is not available.\r\n" );
                return true;
            }

            /*
             * Close security hole 
             */
            if ( ch->gen_data->points_chosen + skill_table[sn].rating[ch->iclass] > 300 )
            {
                ch_printf( ch, "You cannot take more than 300 creation points.\r\n" );
                return true;
            }
            ch_printf( ch, "%s skill added\r\n", skill_table[sn].name );
            ch->gen_data->skill_chosen[sn] = true;
            ch->gen_data->points_chosen += skill_table[sn].rating[ch->iclass];
            ch->pcdata->learned[sn] = 1;
            ch->pcdata->points += skill_table[sn].rating[ch->iclass];
            return true;
        }

        ch_printf( ch, "No skills or groups by that name...\r\n" );
        return true;
    }

    if ( !strcmp( arg, "drop" ) )
    {
        if ( argument[0] == '\0' )
        {
            ch_printf( ch, "You must provide a skill to drop.\r\n" );
            return true;
        }

        gn = group_lookup( argument );
        if ( gn != -1 && ch->gen_data->group_chosen[gn] )
        {
            ch_printf( ch, "Group dropped.\r\n" );
            ch->gen_data->group_chosen[gn] = false;
            ch->gen_data->points_chosen -= group_table[gn].rating[ch->iclass];
            gn_remove( ch, gn );
            for ( i = 0; i < MAX_GROUP; i++ )
            {
                if ( ch->gen_data->group_chosen[gn] )
                    gn_add( ch, gn );
            }
            ch->pcdata->points -= group_table[gn].rating[ch->iclass];
            return true;
        }

        sn = skill_lookup( argument );
        if ( sn != -1 && ch->gen_data->skill_chosen[sn] )
        {
            ch_printf( ch, "Skill dropped.\r\n" );
            ch->gen_data->skill_chosen[sn] = false;
            ch->gen_data->points_chosen -= skill_table[sn].rating[ch->iclass];
            ch->pcdata->learned[sn] = 0;
            ch->pcdata->points -= skill_table[sn].rating[ch->iclass];
            return true;
        }

        ch_printf( ch, "You haven't bought any such skill or group.\r\n" );
        return true;
    }

    if ( !str_prefix( arg, "premise" ) )
    {
        do_function( ch, &do_help, "premise" );
        return true;
    }

    if ( !str_prefix( arg, "list" ) )
    {
        list_group_costs( ch );
        return true;
    }

    if ( !str_prefix( arg, "learned" ) )
    {
        list_group_chosen( ch );
        return true;
    }

    if ( !str_prefix( arg, "info" ) )
    {
        do_function( ch, &do_groups, argument );
        return true;
    }

    return false;
}

/* shows all groups, or the sub-members of a group */
void do_groups( CHAR_DATA *ch, const char *argument )
{
    int                     gn = -1;
    int                     sn = -1;
    int                     col = 0;

    if ( IS_NPC( ch ) )
        return;

    if ( argument[0] == '\0' )
    {                                                  /* show all groups */

        for ( gn = 0; gn < MAX_GROUP; gn++ )
        {
            if ( group_table[gn].name == NULL )
                break;
            if ( ch->pcdata->group_known[gn] )
            {
                ch_printf( ch, "%-20s ", group_table[gn].name );
                if ( ++col % 3 == 0 )
                    ch_printf( ch, "\r\n" );
            }
        }
        if ( col % 3 != 0 )
            ch_printf( ch, "\r\n" );
        ch_printf( ch, "Creation points: %d\r\n", ch->pcdata->points );
        return;
    }

    if ( !str_cmp( argument, "all" ) )                 /* show all groups */
    {
        for ( gn = 0; gn < MAX_GROUP; gn++ )
        {
            if ( group_table[gn].name == NULL )
                break;
            ch_printf( ch, "%-20s ", group_table[gn].name );
            if ( ++col % 3 == 0 )
                ch_printf( ch, "\r\n" );
        }
        if ( col % 3 != 0 )
            ch_printf( ch, "\r\n" );
        return;
    }

    /*
     * show the sub-members of a group 
     */
    gn = group_lookup( argument );
    if ( gn == -1 )
    {
        ch_printf( ch, "No group of that name exist.\r\n" );
        ch_printf( ch, "Type 'groups all' or 'info all' for a full listing.\r\n" );
        return;
    }

    for ( sn = 0; sn < MAX_IN_GROUP; sn++ )
    {
        if ( group_table[gn].spells[sn] == NULL )
            break;
        ch_printf( ch, "%-20s ", group_table[gn].spells[sn] );
        if ( ++col % 3 == 0 )
            ch_printf( ch, "\r\n" );
    }
    if ( col % 3 != 0 )
        ch_printf( ch, "\r\n" );
}

/* checks for skill improvement */
void check_improve( CHAR_DATA *ch, int sn, bool success, int multiplier )
{
    int                     chance = 0;

    if ( IS_NPC( ch ) )
        return;

    if ( ch->level < skill_table[sn].skill_level[ch->iclass]
         || skill_table[sn].rating[ch->iclass] == 0
         || ch->pcdata->learned[sn] == 0 || ch->pcdata->learned[sn] == 100 )
        return;                                        /* skill is not known */

    /*
     * check to see if the character has a chance to learn 
     */
    chance = 10 * int_app[get_curr_stat( ch, STAT_INT )].learn;
    chance /= ( multiplier * skill_table[sn].rating[ch->iclass] * 4 );
    chance += ch->level;

    if ( number_range( 1, 1000 ) > chance )
        return;

    /*
     * now that the character has a CHANCE to learn, see if they really have 
     */

    if ( success )
    {
        chance = URANGE( 5, 100 - ch->pcdata->learned[sn], 95 );
        if ( number_percent(  ) < chance )
        {
            ch_printf( ch, "You have become better at %s!\r\n", skill_table[sn].name );
            ch->pcdata->learned[sn]++;
            gain_exp( ch, 2 * skill_table[sn].rating[ch->iclass] );
        }
    }

    else
    {
        chance = URANGE( 5, ch->pcdata->learned[sn] / 2, 30 );
        if ( number_percent(  ) < chance )
        {
            ch_printf( ch,
                       "You learn from your mistakes, and your %s skill improves.\r\n",
                       skill_table[sn].name );
            ch->pcdata->learned[sn] += number_range( 1, 3 );
            ch->pcdata->learned[sn] = UMIN( ch->pcdata->learned[sn], 100 );
            gain_exp( ch, 2 * skill_table[sn].rating[ch->iclass] );
        }
    }
}

/* returns a group index number given the name */
int group_lookup( const char *name )
{
    int                     gn = -1;

    for ( gn = 0; gn < MAX_GROUP; gn++ )
    {
        if ( group_table[gn].name == NULL )
            break;
        if ( LOWER( name[0] ) == LOWER( group_table[gn].name[0] )
             && !str_prefix( name, group_table[gn].name ) )
            return gn;
    }

    return -1;
}

/* recursively adds a group given its number -- uses group_add */
void gn_add( CHAR_DATA *ch, int gn )
{
    int                     i = 0;

    ch->pcdata->group_known[gn] = true;
    for ( i = 0; i < MAX_IN_GROUP; i++ )
    {
        if ( group_table[gn].spells[i] == NULL )
            break;
        group_add( ch, group_table[gn].spells[i], false );
    }
}

/* recusively removes a group given its number -- uses group_remove */
void gn_remove( CHAR_DATA *ch, int gn )
{
    int                     i = 0;

    ch->pcdata->group_known[gn] = false;

    for ( i = 0; i < MAX_IN_GROUP; i++ )
    {
        if ( group_table[gn].spells[i] == NULL )
            break;
        group_remove( ch, group_table[gn].spells[i] );
    }
}

/* use for processing a skill or group for addition  */
void group_add( CHAR_DATA *ch, const char *name, bool deduct )
{
    int                     sn = -1;
    int                     gn = -1;

    if ( IS_NPC( ch ) )                                /* NPCs do not have skills */
        return;

    sn = skill_lookup( name );

    if ( sn != -1 )
    {
        if ( ch->pcdata->learned[sn] == 0 )            /* i.e. not known */
        {
            ch->pcdata->learned[sn] = 1;
            if ( deduct )
                ch->pcdata->points += skill_table[sn].rating[ch->iclass];
        }
        return;
    }

    /*
     * now check groups 
     */

    gn = group_lookup( name );

    if ( gn != -1 )
    {
        if ( ch->pcdata->group_known[gn] == false )
        {
            ch->pcdata->group_known[gn] = true;
            if ( deduct )
                ch->pcdata->points += group_table[gn].rating[ch->iclass];
        }
        gn_add( ch, gn );                              /* make sure all skills in the
                                                        * group are known */
    }
}

/* used for processing a skill or group for deletion -- no points back! */
void group_remove( CHAR_DATA *ch, const char *name )
{
    int                     sn = -1;
    int                     gn = -1;

    sn = skill_lookup( name );

    if ( sn != -1 )
    {
        ch->pcdata->learned[sn] = 0;
        return;
    }

    /*
     * now check groups 
     */

    gn = group_lookup( name );

    if ( gn != -1 && ch->pcdata->group_known[gn] == true )
    {
        ch->pcdata->group_known[gn] = false;
        gn_remove( ch, gn );                           /* be sure to call gn_add on all
                                                        * remaining groups */
    }
}
