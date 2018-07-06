/*
 * RAM $Id: olc_mpcode.c 67 2009-01-05 00:39:32Z quixadhal $
 */

/* The following code is based on ILAB OLC by Jason Dinkel */
/* Mobprogram code by Lordrom for Nevermore Mud */

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "interp.h"
#include "tables.h"
#include "strings.h"
#include "olc.h"
#include "db.h"

#define MPEDIT( fun )           bool fun(CHAR_DATA *ch, char*argument)

const struct olc_cmd_type mpedit_table[] = {
/* {"command",function}, */

    {"commands", show_commands},
    {"create", mpedit_create},
    {"code", mpedit_code},
    {"show", mpedit_show},
    {"list", mpedit_list},
    {"?", show_help},

    {NULL, 0}
};

void mpedit( CHAR_DATA *ch, const char *argument )
{
    MPROG_CODE             *pMcode = NULL;
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    command[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    local_argument[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    const char             *lap = local_argument;
    int                     cmd = 0;
    AREA_DATA              *ad = NULL;

    smash_tilde( local_argument );
    strcpy( arg, argument );
    lap = one_argument( lap, command );

    EDIT_MPCODE( ch, pMcode );

    if ( pMcode )
    {
        ad = get_vnum_area( pMcode->vnum );

        if ( ad == NULL )                              /* ??? */
        {
            edit_done( ch );
            return;
        }

        if ( !IS_BUILDER( ch, ad ) )
        {
            ch_printf( ch, "MPEdit:  Insufficient security to modify code.\r\n" );
            edit_done( ch );
            return;
        }
    }

    if ( command[0] == '\0' )
    {
        mpedit_show( ch, lap );
        return;
    }

    if ( !str_cmp( command, "done" ) )
    {
        edit_done( ch );
        return;
    }

    for ( cmd = 0; mpedit_table[cmd].name != NULL; cmd++ )
    {
        if ( !str_prefix( command, mpedit_table[cmd].name ) )
        {
            if ( ( *mpedit_table[cmd].olc_fun ) ( ch, lap ) && pMcode )
                if ( ( ad = get_vnum_area( pMcode->vnum ) ) != NULL )
                    SET_BIT( ad->area_flags, AREA_CHANGED );
            return;
        }
    }

    interpret( ch, arg );

    return;
}

void do_mpedit( CHAR_DATA *ch, const char *argument )
{
    MPROG_CODE             *pMcode = NULL;
    char                    command[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";

    argument = one_argument( argument, command );

    if ( is_number( command ) )
    {
        int                     vnum = atoi( command );
        AREA_DATA              *ad = NULL;

        if ( ( pMcode = get_mprog_index( vnum ) ) == NULL )
        {
            ch_printf( ch, "MPEdit:  That vnum does not exist.\r\n" );
            return;
        }

        ad = get_vnum_area( vnum );

        if ( ad == NULL )
        {
            ch_printf( ch, "MPEdit:  VNUM not assigned to any area.\r\n" );
            return;
        }

        if ( !IS_BUILDER( ch, ad ) )
        {
            ch_printf( ch, "MPEdit:  Insufficient security to modify area.\r\n" );
            return;
        }

        ch->desc->pEdit = ( void * ) pMcode;
        ch->desc->editor = ED_MPCODE;

        return;
    }

    if ( !str_cmp( command, "create" ) )
    {
        if ( argument[0] == '\0' )
        {
            ch_printf( ch, "Syntax:  mpedit create [vnum]\r\n" );
            return;
        }

        mpedit_create( ch, argument );
        return;
    }

    ch_printf( ch, "Syntax:  mpedit [vnum]\r\n" );
    ch_printf( ch, "         mpedit create [vnum]\r\n" );

    return;
}

bool mpedit_create( CHAR_DATA *ch, const char *argument )
{
    MPROG_CODE             *pMcode = NULL;
    int                     value = atoi( argument );
    AREA_DATA              *ad = NULL;

    if ( IS_NULLSTR( argument ) || value < 1 )
    {
        ch_printf( ch, "Syntax:  mpedit create [vnum]\r\n" );
        return false;
    }

    ad = get_vnum_area( value );

    if ( ad == NULL )
    {
        ch_printf( ch, "MPEdit:  VNUM not assigned to any area.\r\n" );
        return false;
    }

    if ( !IS_BUILDER( ch, ad ) )
    {
        ch_printf( ch, "MPEdit:  Insufficient security to modify area.\r\n" );
        return false;
    }

    if ( get_mprog_index( value ) )
    {
        ch_printf( ch, "MPEdit: Code vnum already exists.\r\n" );
        return false;
    }

    pMcode = new_mpcode(  );
    pMcode->vnum = value;
    pMcode->next = mprog_list;
    mprog_list = pMcode;
    ch->desc->pEdit = ( void * ) pMcode;
    ch->desc->editor = ED_MPCODE;

    ch_printf( ch, "MobProgram Code Created.\r\n" );

    return true;
}

bool mpedit_show( CHAR_DATA *ch, const char *argument )
{
    MPROG_CODE             *pMcode = NULL;

    EDIT_MPCODE( ch, pMcode );

    ch_printf( ch, "Vnum:       [%d]\r\n" "Code:\r\n%s\r\n", pMcode->vnum, pMcode->code );

    return false;
}

bool mpedit_code( CHAR_DATA *ch, const char *argument )
{
    MPROG_CODE             *pMcode = NULL;

    EDIT_MPCODE( ch, pMcode );

    if ( argument[0] == '\0' )
    {
        string_append( ch, &pMcode->code );
        return true;
    }

    ch_printf( ch, "Syntax: code\r\n" );
    return false;
}

bool mpedit_list( CHAR_DATA *ch, const char *argument )
{
    int                     count = 1;
    MPROG_CODE             *mprg = NULL;
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    BUFFER                 *buffer = NULL;
    bool                    fAll = !str_cmp( argument, "all" );
    char                    blah = '\0';
    AREA_DATA              *ad = NULL;

    buffer = new_buf(  );

    for ( mprg = mprog_list; mprg != NULL; mprg = mprg->next )
        if ( fAll
             || ENTRE( ch->in_room->area->min_vnum, mprg->vnum,
                       ch->in_room->area->max_vnum ) )
        {
            ad = get_vnum_area( mprg->vnum );

            if ( ad == NULL )
                blah = '?';
            else if ( IS_BUILDER( ch, ad ) )
                blah = '*';
            else
                blah = ' ';

            sprintf( buf, "[%3d] (%c) %5d\r\n", count, blah, mprg->vnum );
            add_buf( buffer, buf );

            count++;
        }

    if ( count == 1 )
    {
        if ( fAll )
            add_buf( buffer, "No existing MobPrograms.\r\n" );
        else
            add_buf( buffer, "No existing MobPrograms in this area.\r\n" );
    }

    page_to_char( buf_string( buffer ), ch );
    free_buf( buffer );

    return false;
}
