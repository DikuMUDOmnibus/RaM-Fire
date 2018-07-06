/*
 * RAM $Id: playerlist.c 59 2008-11-17 04:12:42Z quixadhal $
 */

/***************************************************************************
*  RaM 0.1, copyright (C) 2008 by The RaM Team                             *
*                                                                          *
*  While RaM is a derivative of ROM, and therefore you are bound by the    *
*  ROM, Merc, and Diku Mud licenses, some files are developed entirely     *
*  by us, and those may be used freely, provided this copyright notice     *
*  remains intact.                                                         *
***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#include "merc.h"
#include "strings.h"
#include "db.h"

struct player_list     *all_players = NULL;
int                     top_player = -1;

#ifdef PLAYER_LIST

static int qcmp_player_list( const void *left, const void *right )
{
    const struct player_list *a = left;
    const struct player_list *b = right;

    if ( !a && !b )
        return 0;
    if ( a && !b )
        return -1;
    if ( !a && b )
        return 1;

    return strcasecmp( a->name, b->name );
}

static struct player_list * bsearch_player_list( const char *name, int first, int top )
{
    int                     i = 0;

    for ( ;; )
    {
        i = ( first + top ) / 2;

        if ( i < 0 || i > top_player )
            return NULL;
        if ( !strcasecmp( name, all_players[i].name ) )
            return &all_players[i];
        if ( first >= top )
            return NULL;
        if ( strcasecmp( name, all_players[i].name ) < 0 )
            top = i - 1;
        else
            first = i + 1;
    }
}

void dump_player_list( void )
{
    FILE *fp = NULL;

    if ( all_players == NULL )
        return;

    if ( ( fp = fopen( PLAYER_FILE, "w" ) ) )
    {
        int i = 0;

        fprintf( fp, "%d\n", top_player + 1 );
        for( i = 0; i <= top_player; i++ )
        {
            if ( all_players[i].ch != NULL )
            {
                all_players[i].time_played += ( time( NULL ) - all_players[i].last_login );
                all_players[i].last_login = time( NULL );
                all_players[i].level = all_players[i].ch->level;
                all_players[i].iRace = all_players[i].ch->race;
                all_players[i].iClass = all_players[i].ch->iclass;
            }
            fprintf( fp, "%s\t%ld\t%ld\t%ld\t%d\t%d\t%d\t%d\n",
                         all_players[i].name, all_players[i].date_created,
                         all_players[i].last_login, all_players[i].time_played,
                         all_players[i].login_count, all_players[i].level,
                         all_players[i].iRace, all_players[i].iClass );
        }
        fclose( fp );
    }
    else
    {
        log_error( "Can't dump player list!" );
    }
}

void load_player_list( void )
{
    FILE *fp = NULL;

    if ( ( fp = fopen( PLAYER_FILE, "r" ) ) )
    {
        int i = 0;

        need_god = false;
        top_player = fread_number(fp);
        top_player--;
        if ( top_player < 0 )
        {
            log_error( "Bad data in %s.", PLAYER_FILE );
            all_players = NULL;
            top_player = -1;
            return;
        }
        CREATE( all_players, struct player_list, top_player + 1 );
        for( i = 0; i <= top_player; i++ )
        {
            all_players[i].name = str_dup( fread_word( fp ) );
            all_players[i].date_created = fread_number( fp );
            all_players[i].last_login = fread_number( fp );
            all_players[i].time_played = fread_number( fp );
            all_players[i].login_count = fread_number( fp );
            all_players[i].level = fread_number( fp );
            all_players[i].iRace = fread_number( fp );
            all_players[i].iClass = fread_number( fp );
            all_players[i].ch = NULL;
        }
        fclose( fp );
        qsort( ( void * ) all_players, top_player + 1, sizeof( struct player_list ),
               ( int ( * )( const void *, const void * ) ) qcmp_player_list );
    }
    else
    {
        need_god = true;
        all_players = NULL;
        top_player = -1;
    }
}

void update_player_list( CHAR_DATA *ch, bool online )
{
    struct player_list *pl = NULL;

    if ( ( pl = find_player_in_list( ch->name ) ) )
    {
        // Update to existing player.
        pl->last_login = time( NULL );
        pl->login_count++;
        pl->level = ch->level;
        pl->iRace = ch->race;
        pl->iClass = ch->iclass;
        pl->ch = online ? ch : NULL;
    }
    else
    {
        if ( all_players == NULL )
        {
            top_player = 0;
            CREATE( all_players, struct player_list, top_player + 1 );
            pl = &all_players[0];
            pl->name = str_dup( ch->name );
            pl->date_created = time( NULL );
            pl->last_login = time( NULL );
            pl->time_played = 0;
            pl->login_count = 1;
            pl->level = ch->level;
            pl->iRace = ch->race;
            pl->iClass = ch->iclass;
            pl->ch = online ? ch : NULL;
        }
        else
        {
            top_player++;
            RECREATE( all_players, struct player_list, top_player + 1 );
            pl = &all_players[top_player];
            pl->name = str_dup( ch->name );
            pl->date_created = time( NULL );
            pl->last_login = time( NULL );
            pl->time_played = 0;
            pl->login_count = 1;
            pl->level = ch->level;
            pl->iRace = ch->race;
            pl->iClass = ch->iclass;
            pl->ch = online ? ch : NULL;
            qsort( ( void * ) all_players, top_player + 1, sizeof( struct player_list ),
                   ( int ( * )( const void *, const void * ) ) qcmp_player_list );
        }
    }
}

struct player_list * find_player_in_list( const char *name )
{
    if ( all_players == NULL )
        return NULL;
    else
        return bsearch_player_list( name, 0, top_player );
}

#else

void dump_player_list( void )
{
    return;
}

void load_player_list( void )
{
    return;
}

void update_player_list( CHAR_DATA *ch, bool online )
{
    return;
}

struct player_list * find_player_in_list( const char *name )
{
    return NULL;
}

#endif
