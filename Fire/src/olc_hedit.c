/*
 * RAM $Id: olc_hedit.c 67 2009-01-05 00:39:32Z quixadhal $
 */

#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "interp.h"
#include "tables.h"
#include "olc.h"
#include "db.h"
#include "strings.h"

#define HEDIT( fun )           bool fun(CHAR_DATA *ch, char*argument)
#define EDIT_HELP(ch, help)    ( help = (HELP_DATA *) ch->desc->pEdit )

const struct olc_cmd_type hedit_table[] = {
/*  {"command",function}, */

    {"keyword", hedit_keyword},
    {"text", hedit_text},
    {"new", hedit_new},
    {"level", hedit_level},
    {"commands", show_commands},
    {"delete", hedit_delete},
    {"list", hedit_list},
    {"show", hedit_show},
    {"?", show_help},

    {NULL, 0}
};

HELP_AREA              *get_help_area( HELP_DATA *help )
{
    HELP_AREA              *temp = NULL;
    HELP_DATA              *thelp = NULL;

    for ( temp = had_list; temp; temp = temp->next )
        for ( thelp = temp->first; thelp; thelp = thelp->next_area )
            if ( thelp == help )
                return temp;

    return NULL;
}

bool hedit_show( CHAR_DATA *ch, const char *argument )
{
    HELP_DATA              *help = NULL;

    EDIT_HELP( ch, help );

    ch_printf( ch, "Keyword : [%s]\r\n"
               "Level   : [%d]\r\n"
               "Text    :\r\n" "%s-END-\r\n", help->keyword, help->level, help->text );

    return false;
}

bool hedit_level( CHAR_DATA *ch, const char *argument )
{
    HELP_DATA              *help = NULL;
    int                     lev = 0;

    EDIT_HELP( ch, help );

    if ( IS_NULLSTR( argument ) || !is_number( argument ) )
    {
        ch_printf( ch, "Syntax : level [-1..MAX_LEVEL]\r\n" );
        return false;
    }

    lev = atoi( argument );

    if ( lev < -1 || lev > MAX_LEVEL )
    {
        ch_printf( ch, "HEdit : levels between -1 and %d only.\r\n", MAX_LEVEL );
        return false;
    }

    help->level = lev;
    ch_printf( ch, "Ok.\r\n" );
    return true;
}

bool hedit_keyword( CHAR_DATA *ch, const char *argument )
{
    HELP_DATA              *help = NULL;

    EDIT_HELP( ch, help );

    if ( IS_NULLSTR( argument ) )
    {
        ch_printf( ch, "Syntax : keyword [keywords]\r\n" );
        return false;
    }

    free_string( help->keyword );
    help->keyword = str_dup( argument );

    ch_printf( ch, "Ok.\r\n" );
    return true;
}

bool hedit_new( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    fullarg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    HELP_AREA              *had = NULL;
    HELP_DATA              *help = NULL;

    if ( IS_NULLSTR( argument ) )
    {
        ch_printf( ch, "Syntax : new [number]\r\n" );
        ch_printf( ch, "         new [area] [number]\r\n" );
        return false;
    }

    strcpy( fullarg, argument );
    argument = one_argument( argument, arg );

    if ( !( had = had_lookup( arg ) ) )
    {
        had = ch->in_room->area->helps;
        argument = fullarg;
    }

    if ( help_lookup( argument ) )
    {
        ch_printf( ch, "HEdit : Help exists.\r\n" );
        return false;
    }

    if ( !had )                                        /* The area has no helps */
    {
        had = new_had(  );
        had->filename = str_dup( ch->in_room->area->file_name );
        had->area = ch->in_room->area;
        had->first = NULL;
        had->last = NULL;
        had->changed = true;
        had->next = had_list;
        had_list = had;
        ch->in_room->area->helps = had;
        SET_BIT( ch->in_room->area->area_flags, AREA_CHANGED );
    }

    help = new_help(  );
    help->level = 0;
    help->keyword = str_dup( argument );
    help->text = str_dup( "" );

    if ( help_last )
        help_last->next = help;

    if ( help_first == NULL )
        help_first = help;

    help_last = help;
    help->next = NULL;

    if ( !had->first )
        had->first = help;
    if ( !had->last )
        had->last = help;

    had->last->next_area = help;
    had->last = help;
    help->next_area = NULL;

    ch->desc->pEdit = ( HELP_DATA * ) help;
    ch->desc->editor = ED_HELP;

    ch_printf( ch, "Ok.\r\n" );
    return true;
}

bool hedit_text( CHAR_DATA *ch, const char *argument )
{
    HELP_DATA              *help = NULL;

    EDIT_HELP( ch, help );

    if ( !IS_NULLSTR( argument ) )
    {
        ch_printf( ch, "Syntax : text\r\n" );
        return false;
    }

    string_append( ch, &help->text );

    return false;
}

void hedit( CHAR_DATA *ch, const char *argument )
{
    HELP_DATA              *pHelp = NULL;
    HELP_AREA              *had = NULL;
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    command[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    int                     cmd = 0;
    char                    local_argument[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    const char             *lap = local_argument;

    strcpy( local_argument, argument );
    smash_tilde( local_argument );

    strcpy( arg, lap );
    lap = one_argument( lap, command );

    EDIT_HELP( ch, pHelp );

    had = get_help_area( pHelp );

    if ( had == NULL )
    {
        log_error( "HEdit:  Help area for help '%s' NULL", pHelp->keyword );
        edit_done( ch );
        return;
    }

    if ( ch->pcdata->security < 9 )
    {
        ch_printf( ch, "HEdit:  Insufficient security for editing help.\r\n" );
        edit_done( ch );
        return;
    }

    if ( command[0] == '\0' )
    {
        hedit_show( ch, lap );
        return;
    }

    if ( !str_cmp( command, "done" ) )
    {
        edit_done( ch );
        return;
    }

    for ( cmd = 0; hedit_table[cmd].name != NULL; cmd++ )
    {
        if ( !str_prefix( command, hedit_table[cmd].name ) )
        {
            if ( ( *hedit_table[cmd].olc_fun ) ( ch, lap ) )
                had->changed = true;
            return;
        }
    }

    interpret( ch, arg );
    return;
}

void do_hedit( CHAR_DATA *ch, const char *argument )
{
    HELP_DATA              *pHelp = NULL;

    if ( IS_NPC( ch ) )
        return;

    if ( ( pHelp = help_lookup( argument ) ) == NULL )
    {
        ch_printf( ch, "HEdit:  Help does not exist.\r\n" );
        return;
    }

    ch->desc->pEdit = ( void * ) pHelp;
    ch->desc->editor = ED_HELP;

    return;
}

bool hedit_delete( CHAR_DATA *ch, const char *argument )
{
    HELP_DATA              *pHelp = NULL;
    HELP_DATA              *temp = NULL;
    HELP_AREA              *had = NULL;
    DESCRIPTOR_DATA        *d = NULL;
    bool                    found = false;

    EDIT_HELP( ch, pHelp );

    for ( d = descriptor_list; d; d = d->next )
        if ( d->editor == ED_HELP && pHelp == ( HELP_DATA * ) d->pEdit )
            edit_done( d->character );

    if ( help_first == pHelp )
        help_first = help_first->next;
    else
    {
        for ( temp = help_first; temp; temp = temp->next )
            if ( temp->next == pHelp )
                break;

        if ( !temp )
        {
            log_error( "hedit_delete : help %s not found in help_first", pHelp->keyword );
            return false;
        }

        temp->next = pHelp->next;
    }

    for ( had = had_list; had; had = had->next )
        if ( pHelp == had->first )
        {
            found = true;
            had->first = had->first->next_area;
        }
        else
        {
            for ( temp = had->first; temp; temp = temp->next_area )
                if ( temp->next_area == pHelp )
                    break;

            if ( temp )
            {
                temp->next_area = pHelp->next_area;
                found = true;
                break;
            }
        }

    if ( !found )
    {
        log_error( "hedit_delete : help %s not found in had_list", pHelp->keyword );
        return FALSE;
    }

    free_help( pHelp );

    ch_printf( ch, "Ok.\r\n" );
    return true;
}

bool hedit_list( CHAR_DATA *ch, const char *argument )
{
    char                    buf[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    int                     cnt = 0;
    HELP_DATA              *pHelp = NULL;
    BUFFER                 *buffer = NULL;

    EDIT_HELP( ch, pHelp );

    if ( !str_cmp( argument, "all" ) )
    {
        buffer = new_buf(  );

        for ( pHelp = help_first; pHelp; pHelp = pHelp->next )
        {
            sprintf( buf, "%3d. %-14.14s%s", cnt, pHelp->keyword,
                     cnt % 4 == 3 ? "\r\n" : " " );
            add_buf( buffer, buf );
            cnt++;
        }

        if ( cnt % 4 )
            add_buf( buffer, "\r\n" );

        page_to_char( buf_string( buffer ), ch );
        return false;
    }

    if ( !str_cmp( argument, "area" ) )
    {
        if ( ch->in_room->area->helps == NULL )
        {
            ch_printf( ch, "There are no helps in this area.\r\n" );
            return false;
        }

        buffer = new_buf(  );

        for ( pHelp = ch->in_room->area->helps->first; pHelp; pHelp = pHelp->next_area )
        {
            sprintf( buf, "%3d. %-14.14s%s", cnt, pHelp->keyword,
                     cnt % 4 == 3 ? "\r\n" : " " );
            add_buf( buffer, buf );
            cnt++;
        }

        if ( cnt % 4 )
            add_buf( buffer, "\r\n" );

        page_to_char( buf_string( buffer ), ch );
        return false;
    }

    if ( IS_NULLSTR( argument ) )
    {
        ch_printf( ch, "Syntax : list all\r\n" );
        ch_printf( ch, "         list area\r\n" );
        return false;
    }

    return false;
}
