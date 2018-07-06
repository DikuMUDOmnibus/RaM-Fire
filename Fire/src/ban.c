/*
 * RAM $Id: ban.c 69 2009-01-11 18:13:26Z quixadhal $
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
#include <unistd.h>
#include <errno.h>

#include <string>

#include "merc.h"
#include "strings.h"
#include "db.h"
#include "interp.h"
#include "ban.h"

BAN_DATA               *ban_list;
BAN_DATA               *ban_free = NULL;

BAN_DATA               *new_ban( void )
{
    static BAN_DATA         ban_zero;
    BAN_DATA               *ban = NULL;

    if ( ban_free == NULL )
        ban = ( BAN_DATA * ) alloc_perm( sizeof( *ban ) );
    else
    {
        ban = ban_free;
        ban_free = ban_free->next;
    }

    *ban = ban_zero;
    VALIDATE( ban );
    ban->name.erase();
    return ban;
}

void free_ban( BAN_DATA *ban )
{
    if ( !IS_VALID( ban ) )
        return;

    ban->name.erase();
    INVALIDATE( ban );

    ban->next = ban_free;
    ban_free = ban;
}

void save_bans( void )
{
    BAN_DATA               *pban = NULL;
    FILE                   *fp = NULL;
    bool                    found = false;

    if ( ( fp = fopen( BAN_FILE, "w" ) ) == NULL )
    {
        char                   *e = strerror( errno );

        log_error( "fopen: %s: %s", BAN_FILE, e );
    }

    for ( pban = ban_list; pban != NULL; pban = pban->next )
    {
        if ( IS_SET( pban->ban_flags, BAN_PERMANENT ) )
        {
            found = true;
            fprintf( fp, "%-20s %-2d %s\n", pban->name.c_str(), pban->level,
                     print_flags( pban->ban_flags ) );
        }
    }

    fclose( fp );
    if ( !found )
        unlink( BAN_FILE );
}

void load_bans( void )
{
    FILE                   *fp = NULL;
    BAN_DATA               *ban_last = NULL;

    if ( ( fp = fopen( BAN_FILE, "r" ) ) == NULL )
        return;

    ban_last = NULL;
    for ( ;; )
    {
        BAN_DATA               *pban = NULL;

        if ( feof( fp ) )
        {
            fclose( fp );
            return;
        }

        pban = new_ban(  );

        pban->name = fread_word( fp );
        pban->level = fread_number( fp );
        pban->ban_flags = fread_flag( fp );
        fread_to_eol( fp );

        if ( ban_list == NULL )
            ban_list = pban;
        else
            ban_last->next = pban;
        ban_last = pban;
    }
}

bool check_ban( char *site, int type )
{
    BAN_DATA               *pban = NULL;
    char                    host[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    strcpy( host, capitalize( site ) );
    host[0] = LOWER( host[0] );

    for ( pban = ban_list; pban != NULL; pban = pban->next )
    {
        if ( !IS_SET( pban->ban_flags, type ) )
            continue;

        if ( IS_SET( pban->ban_flags, BAN_PREFIX )
             && IS_SET( pban->ban_flags, BAN_SUFFIX )
             && pban->name.find(host) != std::string::npos)
            return true;

        if ( IS_SET( pban->ban_flags, BAN_PREFIX ) && !str_suffix( pban->name.c_str(), host ) )
            return true;

        if ( IS_SET( pban->ban_flags, BAN_SUFFIX ) && !str_prefix( pban->name.c_str(), host ) )
            return true;
    }

    return false;
}

void ban_site( CHAR_DATA *ch, const char *argument, bool fPerm )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    buf2[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                   *name = NULL;
    BUFFER                 *buffer = NULL;
    BAN_DATA               *pban = NULL;
    BAN_DATA               *prev = NULL;
    bool                    prefix = false;
    bool                    suffix = false;
    int                     type = 0;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' )
    {
        if ( ban_list == NULL )
        {
            ch_printf( ch, "No sites banned at this time.\r\n" );
            return;
        }
        buffer = new_buf(  );

        add_buf( buffer, "Banned sites  level  type     status\r\n" );
        for ( pban = ban_list; pban != NULL; pban = pban->next )
        {
            sprintf( buf2, "%s%s%s",
                     IS_SET( pban->ban_flags, BAN_PREFIX ) ? "*" : "",
                     pban->name.c_str(), IS_SET( pban->ban_flags, BAN_SUFFIX ) ? "*" : "" );
            sprintf( buf, "%-12s    %-3d  %-7s  %s\r\n",
                     buf2, pban->level,
                     IS_SET( pban->ban_flags, BAN_NEWBIES ) ? "newbies" :
                     IS_SET( pban->ban_flags, BAN_PERMIT ) ? "permit" :
                     IS_SET( pban->ban_flags, BAN_ALL ) ? "all" : "",
                     IS_SET( pban->ban_flags, BAN_PERMANENT ) ? "perm" : "temp" );
            add_buf( buffer, buf );
        }

        page_to_char( buf_string( buffer ), ch );
        free_buf( buffer );
        return;
    }

    /*
     * find out what type of ban 
     */
    if ( arg2[0] == '\0' || !str_prefix( arg2, "all" ) )
        type = BAN_ALL;
    else if ( !str_prefix( arg2, "newbies" ) )
        type = BAN_NEWBIES;
    else if ( !str_prefix( arg2, "permit" ) )
        type = BAN_PERMIT;
    else
    {
        ch_printf( ch, "Acceptable ban types are all, newbies, and permit.\r\n" );
        return;
    }

    name = arg1;

    if ( name[0] == '*' )
    {
        prefix = true;
        name++;
    }

    if ( name[strlen( name ) - 1] == '*' )
    {
        suffix = true;
        name[strlen( name ) - 1] = '\0';
    }

    if ( strlen( name ) == 0 )
    {
        ch_printf( ch, "You have to ban SOMETHING.\r\n" );
        return;
    }

    prev = NULL;
    for ( pban = ban_list; pban != NULL; prev = pban, pban = pban->next )
    {
        if ( !str_cmp( name, pban->name.c_str() ) )
        {
            if ( pban->level > get_trust( ch ) )
            {
                ch_printf( ch, "That ban was set by a higher power.\r\n" );
                return;
            }
            else
            {
                if ( prev == NULL )
                    ban_list = pban->next;
                else
                    prev->next = pban->next;
                free_ban( pban );
            }
        }
    }

    pban = new_ban(  );
    pban->name = str_dup( name );
    pban->level = get_trust( ch );

    /*
     * set ban type 
     */
    pban->ban_flags = type;

    if ( prefix )
        SET_BIT( pban->ban_flags, BAN_PREFIX );
    if ( suffix )
        SET_BIT( pban->ban_flags, BAN_SUFFIX );
    if ( fPerm )
        SET_BIT( pban->ban_flags, BAN_PERMANENT );

    pban->next = ban_list;
    ban_list = pban;
    save_bans(  );
    ch_printf( ch, "%s has been banned.\r\n", pban->name.c_str() );
    return;
}

void do_ban( CHAR_DATA *ch, const char *argument )
{
    ban_site( ch, argument, false );
}

void do_permban( CHAR_DATA *ch, const char *argument )
{
    ban_site( ch, argument, true );
}

void do_allow( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    BAN_DATA               *prev = NULL;
    BAN_DATA               *curr = NULL;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Remove which site from the ban list?\r\n" );
        return;
    }

    prev = NULL;
    for ( curr = ban_list; curr != NULL; prev = curr, curr = curr->next )
    {
        if ( !str_cmp( arg, curr->name.c_str() ) )
        {
            if ( curr->level > get_trust( ch ) )
            {
                ch_printf( ch, "You are not powerful enough to lift that ban.\r\n" );
                return;
            }
            if ( prev == NULL )
                ban_list = ban_list->next;
            else
                prev->next = curr->next;

            free_ban( curr );
            ch_printf( ch, "Ban on %s lifted.\r\n", arg );
            save_bans(  );
            return;
        }
    }

    ch_printf( ch, "Site is not banned.\r\n" );
    return;
}
