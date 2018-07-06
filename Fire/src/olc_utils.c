/*
 * RAM $Id: olc_utils.c 85 2009-02-08 17:09:18Z ram $
 */

/***************************************************************************
 *  File: string.c                                                         *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "db.h"
#include "interp.h"
#include "tables.h"
#include "strings.h"
#include "olc.h"

/*****************************************************************************
 Name:           string_edit
 Purpose:        Clears string and puts player into editing mode.
 Called by:      none
 ****************************************************************************/
void string_edit( CHAR_DATA *ch, char **pString )
{
    ch_printf( ch, "-========- Entering EDIT Mode -=========-\r\n" );
    ch_printf( ch, "    Type .h on a new line for help\r\n" );
    ch_printf( ch, " Terminate with a ~ or @ on a blank line.\r\n" );
    ch_printf( ch, "-=======================================-\r\n" );

    if ( *pString == NULL )
    {
        *pString = str_dup( "" );
    }
    else
    {
        **pString = '\0';
    }

    ch->desc->pString = pString;

    return;
}

/*****************************************************************************
 Name:           string_append
 Purpose:        Puts player into append mode for given string.
 Called by:      (many)olc_act.c
 ****************************************************************************/
void string_append( CHAR_DATA *ch, char **pString )
{
    ch_printf( ch, "-=======- Entering APPEND Mode -========-\r\n" );
    ch_printf( ch, "    Type .h on a new line for help\r\n" );
    ch_printf( ch, " Terminate with a ~ or @ on a blank line.\r\n" );
    ch_printf( ch, "-=======================================-\r\n" );

    if ( *pString == NULL )
    {
        *pString = str_dup( "" );
    }
    ch_printf( ch, "%s", numlineas( *pString ) );

    ch->desc->pString = pString;

    return;
}

/*****************************************************************************
 Name:           string_replace
 Purpose:        Substitutes one string for another.
 Called by:      string_add(string.c) (aedit_builder)olc_act.c.
 ****************************************************************************/
char                   *string_replace( char *orig, const char *pOld, const char *pNew )
{
    char                    xbuf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    int                     i = 0;

    strcpy( xbuf, orig );
    if ( strstr( orig, pOld ) != NULL )
    {
        i = strlen( orig ) - strlen( strstr( orig, pOld ) );
        xbuf[i] = '\0';
        strcat( xbuf, pNew );
        strcat( xbuf, &orig[i + strlen( pOld )] );
        free_string( orig );
    }

    return str_dup( xbuf );
}

/*****************************************************************************
 Name:           string_add
 Purpose:        Interpreter for string editing.
 Called by:      game_loop_xxxx(comm.c).
 ****************************************************************************/
void string_add( CHAR_DATA *ch, const char *argument )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    local_argument[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    const char             *lap = local_argument;

    /*
     * Thanks to James Seng
     */
    strcpy( local_argument, argument );
    smash_tilde( local_argument );

    if ( *lap == '.' )
    {
        char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
        char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
        char                    arg3[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
        char                    tmparg3[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";

        lap = one_argument( lap, arg1 );
        lap = first_arg( lap, arg2, FALSE );
        strcpy( tmparg3, lap );
        lap = first_arg( lap, arg3, FALSE );

        if ( !str_cmp( arg1, ".c" ) )
        {
            ch_printf( ch, "String cleared.\r\n" );
            free_string( *ch->desc->pString );
            *ch->desc->pString = str_dup( "" );
            return;
        }

        if ( !str_cmp( arg1, ".s" ) )
        {
            ch_printf( ch, "String so far:\r\n" );
            ch_printf( ch, "%s", numlineas( *ch->desc->pString ) );
            return;
        }

        if ( !str_cmp( arg1, ".r" ) )
        {
            if ( arg2[0] == '\0' )
            {
                ch_printf( ch, "usage:  .r \"old string\" \"new string\"\r\n" );
                return;
            }

            *ch->desc->pString = string_replace( *ch->desc->pString, arg2, arg3 );
            ch_printf( ch, "'%s' replaced with '%s'.\r\n", arg2, arg3 );
            return;
        }

        if ( !str_cmp( arg1, ".f" ) )
        {
            *ch->desc->pString = format_string( *ch->desc->pString );
            ch_printf( ch, "String formatted.\r\n" );
            return;
        }

        if ( !str_cmp( arg1, ".ld" ) )
        {
            *ch->desc->pString = string_linedel( *ch->desc->pString, atoi( arg2 ) );
            ch_printf( ch, "Line deleted.\r\n" );
            return;
        }

        if ( !str_cmp( arg1, ".li" ) )
        {
            *ch->desc->pString =
                string_lineadd( *ch->desc->pString, tmparg3, atoi( arg2 ) );
            ch_printf( ch, "Line inserted.\r\n" );
            return;
        }

        if ( !str_cmp( arg1, ".lr" ) )
        {
            *ch->desc->pString = string_linedel( *ch->desc->pString, atoi( arg2 ) );
            *ch->desc->pString =
                string_lineadd( *ch->desc->pString, tmparg3, atoi( arg2 ) );
            ch_printf( ch, "Line replaced.\r\n" );
            return;
        }

        if ( !str_cmp( arg1, ".h" ) )
        {
            ch_printf( ch, "Sedit help (commands on blank line):   \r\n" );
            ch_printf( ch, ".r 'old' 'new'   - replace a substring \r\n" );
            ch_printf( ch, "                   (requires '', \"\") \r\n" );
            ch_printf( ch, ".h               - get help (this info)\r\n" );
            ch_printf( ch, ".s               - show string so far  \r\n" );
            ch_printf( ch, ".f               - (word wrap) string  \r\n" );
            ch_printf( ch, ".c               - clear string so far \r\n" );
            ch_printf( ch, ".ld <num>        - delete line number <num>\r\n" );
            ch_printf( ch, ".li <num> <str>  - insert <str> at line <num>\r\n" );
            ch_printf( ch, ".lr <num> <str>  - replace line <num> with <str>\r\n" );
            ch_printf( ch, "@                - end string          \r\n" );
            return;
        }

        ch_printf( ch, "SEdit:  Invalid dot command.\r\n" );
        return;
    }

    if ( *argument == '~' || *argument == '@' )
    {
        if ( ch->desc->editor == ED_MPCODE )           /* for mobprogs */
        {
            MOB_INDEX_DATA         *mob = NULL;
            int                     hash = 0;
            MPROG_LIST             *mpl = NULL;
            MPROG_CODE             *mpc = NULL;

            EDIT_MPCODE( ch, mpc );

            if ( mpc != NULL )
                for ( hash = 0; hash < MAX_KEY_HASH; hash++ )
                    for ( mob = mob_index_hash[hash]; mob; mob = mob->next )
                        for ( mpl = mob->mprogs; mpl; mpl = mpl->next )
                            if ( mpl->vnum == mpc->vnum )
                            {
                                ch_printf( ch, "Fixing mob %d.\r\n", mob->vnum );
                                mpl->code = mpc->code;
                            }
        }

        ch->desc->pString = NULL;
        return;
    }

    strcpy( buf, *ch->desc->pString );

    /*
     * Truncate strings to MAX_STRING_LENGTH.
     * --------------------------------------
     */
    if ( strlen( buf ) + strlen( lap ) >= ( MAX_STRING_LENGTH - 4 ) )
    {
        ch_printf( ch, "String too long, last line skipped.\r\n" );

        /*
         * Force character out of editing mode. 
         */
        ch->desc->pString = NULL;
        return;
    }

    /*
     * Ensure no tilde's inside string.
     * --------------------------------
     */
    smash_tilde( local_argument );
    smash_tilde( buf );

    strcat( buf, lap );
    strcat( buf, "\r\n" );
    free_string( *ch->desc->pString );
    *ch->desc->pString = str_dup( buf );
    return;
}

/*
 * Thanks to Kalgen for the new procedure (no more bug!)
 * Original wordwrap() written by Surreality.
 */
/*****************************************************************************
 Name:           format_string
 Purpose:        Special string formating and word-wrapping.
 Called by:      string_add(string.c) (many)olc_act.c
 ****************************************************************************/
char                   *format_string( char *oldstring /* , bool fSpace */  )
{
    char                    xbuf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    xbuf2[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                   *rdesc = NULL;
    int                     i = 0;
    bool                    cap = true;

    for ( rdesc = oldstring; *rdesc; rdesc++ )
    {
        if ( *rdesc == '\n' )
        {
            if ( xbuf[i - 1] != ' ' )
            {
                xbuf[i] = ' ';
                i++;
            }
        }
        else if ( *rdesc == '\r' );
        else if ( *rdesc == ' ' )
        {
            if ( xbuf[i - 1] != ' ' )
            {
                xbuf[i] = ' ';
                i++;
            }
        }
        else if ( *rdesc == ')' )
        {
            if ( xbuf[i - 1] == ' ' && xbuf[i - 2] == ' ' &&
                 ( xbuf[i - 3] == '.' || xbuf[i - 3] == '?' || xbuf[i - 3] == '!' ) )
            {
                xbuf[i - 2] = *rdesc;
                xbuf[i - 1] = ' ';
                xbuf[i] = ' ';
                i++;
            }
            else
            {
                xbuf[i] = *rdesc;
                i++;
            }
        }
        else if ( *rdesc == '.' || *rdesc == '?' || *rdesc == '!' )
        {
            if ( xbuf[i - 1] == ' ' && xbuf[i - 2] == ' ' &&
                 ( xbuf[i - 3] == '.' || xbuf[i - 3] == '?' || xbuf[i - 3] == '!' ) )
            {
                xbuf[i - 2] = *rdesc;
                if ( *( rdesc + 1 ) != '\"' )
                {
                    xbuf[i - 1] = ' ';
                    xbuf[i] = ' ';
                    i++;
                }
                else
                {
                    xbuf[i - 1] = '\"';
                    xbuf[i] = ' ';
                    xbuf[i + 1] = ' ';
                    i += 2;
                    rdesc++;
                }
            }
            else
            {
                xbuf[i] = *rdesc;
                if ( *( rdesc + 1 ) != '\"' )
                {
                    xbuf[i + 1] = ' ';
                    xbuf[i + 2] = ' ';
                    i += 3;
                }
                else
                {
                    xbuf[i + 1] = '\"';
                    xbuf[i + 2] = ' ';
                    xbuf[i + 3] = ' ';
                    i += 4;
                    rdesc++;
                }
            }
            cap = true;
        }
        else
        {
            xbuf[i] = *rdesc;
            if ( cap )
            {
                cap = false;
                xbuf[i] = UPPER( xbuf[i] );
            }
            i++;
        }
    }
    xbuf[i] = 0;
    strcpy( xbuf2, xbuf );

    rdesc = xbuf2;

    xbuf[0] = 0;

    for ( ;; )
    {
        for ( i = 0; i < 77; i++ )
        {
            if ( !*( rdesc + i ) )
                break;
        }
        if ( i < 77 )
        {
            break;
        }
        for ( i = ( xbuf[0] ? 76 : 73 ); i; i-- )
        {
            if ( *( rdesc + i ) == ' ' )
                break;
        }
        if ( i )
        {
            *( rdesc + i ) = 0;
            strcat( xbuf, rdesc );
            strcat( xbuf, "\r\n" );
            rdesc += i + 1;
            while ( *rdesc == ' ' )
                rdesc++;
        }
        else
        {
            log_error( "No spaces" );
            *( rdesc + 75 ) = 0;
            strcat( xbuf, rdesc );
            strcat( xbuf, "-\r\n" );
            rdesc += 76;
        }
    }
    while ( *( rdesc + i ) && ( *( rdesc + i ) == ' ' ||
                                *( rdesc + i ) == '\n' || *( rdesc + i ) == '\r' ) )
        i--;
    *( rdesc + i + 1 ) = 0;
    strcat( xbuf, rdesc );
    if ( xbuf[strlen( xbuf ) - 2] != '\n' )
        strcat( xbuf, "\r\n" );

    free_string( oldstring );
    return ( str_dup( xbuf ) );
}

/*
 * Used above in string_add.  Because this function does not
 * modify case if fCase is FALSE and because it understands
 * parenthesis, it would probably make a nice replacement
 * for one_argument.
 */
/*****************************************************************************
 Name:           first_arg
 Purpose:        Pick off one argument from a string and return the rest.
                 Understands quates, parenthesis (barring ) ('s) and
                 percentages.
 Called by:      string_add(string.c)
 ****************************************************************************/
const char             *first_arg( const char *argument, char *arg_first, bool fCase )
{
    char                    cEnd = '\0';

    while ( *argument == ' ' )
        argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' || *argument == '%' || *argument == '(' )
    {
        if ( *argument == '(' )
        {
            cEnd = ')';
            argument++;
        }
        else
            cEnd = *argument++;
    }

    while ( *argument != '\0' )
    {
        if ( *argument == cEnd )
        {
            argument++;
            break;
        }
        if ( fCase )
            *arg_first = LOWER( *argument );
        else
            *arg_first = *argument;
        arg_first++;
        argument++;
    }
    *arg_first = '\0';

    while ( *argument == ' ' )
        argument++;

    return argument;
}

/*
 * Used in olc_act.c for aedit_builders.
 */
char                   *string_unpad( char *argument )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                   *s = argument;

    while ( *s == ' ' )
        s++;

    strcpy( buf, s );
    s = buf;

    if ( *s != '\0' )
    {
        while ( *s != '\0' )
            s++;
        s--;

        while ( *s == ' ' )
            s--;
        s++;
        *s = '\0';
    }

    free_string( argument );
    return str_dup( buf );
}

/*
 * Same as capitalize but changes the pointer's data.
 * Used in olc_act.c in aedit_builder.
 */
char                   *string_proper( char *argument )
{
    char                   *s = argument;

    while ( *s != '\0' )
    {
        if ( *s != ' ' )
        {
            *s = UPPER( *s );
            while ( *s != ' ' && *s != '\0' )
                s++;
        }
        else
        {
            s++;
        }
    }

    return argument;
}

char                   *string_linedel( char *string, int line )
{
    char                   *strtmp = string;
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    int                     cnt = 1;
    int                     tmp = 0;

    for ( ; *strtmp != '\0'; strtmp++ )
    {
        if ( cnt != line )
            buf[tmp++] = *strtmp;

        if ( *strtmp == '\n' )
        {
            if ( *( strtmp + 1 ) == '\r' )
            {
                if ( cnt != line )
                    buf[tmp++] = *( ++strtmp );
                else
                    ++strtmp;
            }

            cnt++;
        }
    }

    buf[tmp] = '\0';

    free_string( string );
    return str_dup( buf );
}

char                   *string_lineadd( char *string, char *newstr, int line )
{
    char                   *strtmp = string;
    int                     cnt = 1;
    int                     tmp = 0;
    bool                    done = false;
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    for ( ; *strtmp != '\0' || ( !done && cnt == line ); strtmp++ )
    {
        if ( cnt == line && !done )
        {
            strcat( buf, newstr );
            strcat( buf, "\r\n" );
            tmp += strlen( newstr ) + 2;
            cnt++;
            done = true;
        }

        buf[tmp++] = *strtmp;

        if ( done && *strtmp == '\0' )
            break;

        if ( *strtmp == '\n' )
        {
            if ( *( strtmp + 1 ) == '\r' )
                buf[tmp++] = *( ++strtmp );

            cnt++;
        }

        buf[tmp] = '\0';
    }

    free_string( string );
    return str_dup( buf );
}

/* buf queda con la linea sin \r\n */
char                   *getline( char *str, char *buf )
{
    int                     tmp = 0;
    bool                    found = false;

    while ( *str )
    {
        if ( *str == '\n' )
        {
            found = true;
            break;
        }

        buf[tmp++] = *( str++ );
    }

    if ( found )
    {
        if ( *( str + 1 ) == '\r' )
            str += 2;
        else
            str += 1;
    }                                                  /* para que quedemos en el inicio
                                                        * de la prox linea */

    buf[tmp] = '\0';

    return str;
}

char                   *numlineas( char *string )
{
    int                     cnt = 1;
    static char             buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    tmp_line[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    buf[0] = '\0';                                     /* keep me, static */

    while ( *string )
    {
        int buf_left = 0;
        int size_wanted = 0;

        string = getline( string, tmp_line );
        buf_left = MAX_STRING_LENGTH - strlen( buf ) - 7; /* See format string below */
        size_wanted = snprintf( buf + strlen( buf ), buf_left, "%2d. %s\r\n", cnt++, tmp_line );
        if ( size_wanted >= buf_left )
            log_error( "STRING OVERFLOW! wanted %d, had %d", size_wanted, buf_left );
    }

    return buf;
}
