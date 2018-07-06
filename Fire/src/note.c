/*
 * RAM $Id: note.c 47 2008-11-07 22:50:47Z quixadhal $
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
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "merc.h"
#include "strings.h"
#include "db.h"
#include "interp.h"
#include "tables.h"

NOTE_DATA              *note_list = NULL;
NOTE_DATA              *idea_list = NULL;
NOTE_DATA              *penalty_list = NULL;
NOTE_DATA              *news_list = NULL;
NOTE_DATA              *changes_list = NULL;
NOTE_DATA              *note_free = NULL;

NOTE_DATA              *new_note( void )
{
    NOTE_DATA              *note = NULL;

    if ( note_free == NULL )
        note = ( NOTE_DATA * ) alloc_perm( sizeof( *note ) );
    else
    {
        note = note_free;
        note_free = note_free->next;
    }
    VALIDATE( note );
    return note;
}

void free_note( NOTE_DATA *note )
{
    if ( !IS_VALID( note ) )
        return;

    free_string( note->text );
    free_string( note->subject );
    free_string( note->to_list );
    free_string( note->date );
    free_string( note->sender );
    INVALIDATE( note );

    note->next = note_free;
    note_free = note;
}

int count_spool( CHAR_DATA *ch, NOTE_DATA *spool )
{
    int                     count = 0;
    NOTE_DATA              *pnote = NULL;

    for ( pnote = spool; pnote != NULL; pnote = pnote->next )
        if ( !hide_note( ch, pnote ) )
            count++;

    return count;
}

void do_unread( CHAR_DATA *ch, const char *argument )
{
    int                     count = 0;
    bool                    found = false;

    if ( IS_NPC( ch ) )
        return;

    if ( ( count = count_spool( ch, news_list ) ) > 0 )
    {
        found = true;
        ch_printf( ch, "There %s %d new news article%s waiting.\r\n",
                   count > 1 ? "are" : "is", count, count > 1 ? "s" : "" );
    }
    if ( ( count = count_spool( ch, changes_list ) ) > 0 )
    {
        found = true;
        ch_printf( ch, "There %s %d change%s waiting to be read.\r\n",
                   count > 1 ? "are" : "is", count, count > 1 ? "s" : "" );
    }
    if ( ( count = count_spool( ch, note_list ) ) > 0 )
    {
        found = true;
        ch_printf( ch, "You have %d new note%s waiting.\r\n",
                   count, count > 1 ? "s" : "" );
    }
    if ( ( count = count_spool( ch, idea_list ) ) > 0 )
    {
        found = true;
        ch_printf( ch, "You have %d unread idea%s to peruse.\r\n",
                   count, count > 1 ? "s" : "" );
    }
    if ( IS_TRUSTED( ch, ANGEL ) && ( count = count_spool( ch, penalty_list ) ) > 0 )
    {
        found = true;
        ch_printf( ch, "%d %s been added.\r\n",
                   count, count > 1 ? "penalties have" : "penalty has" );
    }

    if ( !found )
        ch_printf( ch, "You have no unread notes.\r\n" );
}

void do_note( CHAR_DATA *ch, const char *argument )
{
    parse_note( ch, argument, NOTE_NOTE );
}

void do_idea( CHAR_DATA *ch, const char *argument )
{
    parse_note( ch, argument, NOTE_IDEA );
}

void do_penalty( CHAR_DATA *ch, const char *argument )
{
    parse_note( ch, argument, NOTE_PENALTY );
}

void do_news( CHAR_DATA *ch, const char *argument )
{
    parse_note( ch, argument, NOTE_NEWS );
}

void do_changes( CHAR_DATA *ch, const char *argument )
{
    parse_note( ch, argument, NOTE_CHANGES );
}

void save_notes( int type )
{
    FILE                   *fp = NULL;
    const char             *name = NULL;
    NOTE_DATA              *pnote = NULL;

    switch ( type )
    {
        default:
            return;
        case NOTE_NOTE:
            name = NOTE_FILE;
            pnote = note_list;
            break;
        case NOTE_IDEA:
            name = IDEA_FILE;
            pnote = idea_list;
            break;
        case NOTE_PENALTY:
            name = PENALTY_FILE;
            pnote = penalty_list;
            break;
        case NOTE_NEWS:
            name = NEWS_FILE;
            pnote = news_list;
            break;
        case NOTE_CHANGES:
            name = CHANGES_FILE;
            pnote = changes_list;
            break;
    }

    if ( ( fp = fopen( name, "w" ) ) == NULL )
    {
        char                   *e = strerror( errno );

        log_error( "fopen: %s: %s", name, e );
    }
    else
    {
        for ( ; pnote != NULL; pnote = pnote->next )
        {
            fprintf( fp, "Sender  %s~\n", pnote->sender );
            fprintf( fp, "Date    %s~\n", pnote->date );
            fprintf( fp, "Stamp   %ld\n", pnote->date_stamp );
            fprintf( fp, "To      %s~\n", pnote->to_list );
            fprintf( fp, "Subject %s~\n", pnote->subject );
            fprintf( fp, "Text\n%s~\n", pnote->text );
        }
        fclose( fp );
        return;
    }
}

void load_notes( void )
{
    load_thread( NOTE_FILE, &note_list, NOTE_NOTE, 14 * 24 * 60 * 60 );
    load_thread( IDEA_FILE, &idea_list, NOTE_IDEA, 28 * 24 * 60 * 60 );
    load_thread( PENALTY_FILE, &penalty_list, NOTE_PENALTY, 0 );
    load_thread( NEWS_FILE, &news_list, NOTE_NEWS, 0 );
    load_thread( CHANGES_FILE, &changes_list, NOTE_CHANGES, 0 );
}

void load_thread( const char *name, NOTE_DATA **list, int type, time_t free_time )
{
    FILE                   *fp = NULL;
    NOTE_DATA              *pnotelast = NULL;

    if ( ( fp = fopen( name, "r" ) ) == NULL )
        return;

    pnotelast = NULL;
    for ( ;; )
    {
        NOTE_DATA              *pnote;
        char                    letter;

        do
        {
            letter = getc( fp );
            if ( feof( fp ) )
            {
                fclose( fp );
                return;
            }
        }
        while ( isspace( letter ) );
        ungetc( letter, fp );

        pnote = ( NOTE_DATA * ) alloc_perm( sizeof( *pnote ) );

        if ( str_cmp( fread_word( fp ), "sender" ) )
            break;
        pnote->sender = fread_string( fp );

        if ( str_cmp( fread_word( fp ), "date" ) )
            break;
        pnote->date = fread_string( fp );

        if ( str_cmp( fread_word( fp ), "stamp" ) )
            break;
        pnote->date_stamp = fread_number( fp );

        if ( str_cmp( fread_word( fp ), "to" ) )
            break;
        pnote->to_list = fread_string( fp );

        if ( str_cmp( fread_word( fp ), "subject" ) )
            break;
        pnote->subject = fread_string( fp );

        if ( str_cmp( fread_word( fp ), "text" ) )
            break;
        pnote->text = fread_string( fp );

        if ( free_time && pnote->date_stamp < current_time - free_time )
        {
            free_note( pnote );
            continue;
        }

        pnote->type = type;

        if ( *list == NULL )
            *list = pnote;
        else
            pnotelast->next = pnote;

        pnotelast = pnote;
    }

    strcpy( strArea, NOTE_FILE );
    fpArea = fp;
    proper_exit( MUD_HALT, "Load_notes: bad key word." );
    return;
}

void append_note( NOTE_DATA *pnote )
{
    FILE                   *fp = NULL;
    const char             *name = NULL;
    NOTE_DATA             **list = NULL;
    NOTE_DATA              *last = NULL;

    switch ( pnote->type )
    {
        default:
            return;
        case NOTE_NOTE:
            name = NOTE_FILE;
            list = &note_list;
            break;
        case NOTE_IDEA:
            name = IDEA_FILE;
            list = &idea_list;
            break;
        case NOTE_PENALTY:
            name = PENALTY_FILE;
            list = &penalty_list;
            break;
        case NOTE_NEWS:
            name = NEWS_FILE;
            list = &news_list;
            break;
        case NOTE_CHANGES:
            name = CHANGES_FILE;
            list = &changes_list;
            break;
    }

    if ( *list == NULL )
        *list = pnote;
    else
    {
        for ( last = *list; last->next != NULL; last = last->next );
        last->next = pnote;
    }

    if ( ( fp = fopen( name, "a" ) ) == NULL )
    {
        char                   *e = strerror( errno );

        log_error( "fopen: %s: %s", name, e );
    }
    else
    {
        fprintf( fp, "Sender  %s~\n", pnote->sender );
        fprintf( fp, "Date    %s~\n", pnote->date );
        fprintf( fp, "Stamp   %ld\n", pnote->date_stamp );
        fprintf( fp, "To      %s~\n", pnote->to_list );
        fprintf( fp, "Subject %s~\n", pnote->subject );
        fprintf( fp, "Text\n%s~\n", pnote->text );
        fclose( fp );
    }
}

bool is_note_to( CHAR_DATA *ch, NOTE_DATA *pnote )
{
    if ( !str_cmp( ch->name, pnote->sender ) )
        return true;

    if ( is_exact_name( "all", pnote->to_list ) )
        return true;

    if ( IS_IMMORTAL( ch ) && is_exact_name( "immortal", pnote->to_list ) )
        return true;

    if ( ch->clan && is_exact_name( clan_table[ch->clan].name, pnote->to_list ) )
        return true;

    if ( is_exact_name( ch->name, pnote->to_list ) )
        return true;

    return false;
}

void note_attach( CHAR_DATA *ch, int type )
{
    NOTE_DATA              *pnote = NULL;

    if ( ch->pnote != NULL )
        return;

    pnote = new_note(  );

    pnote->next = NULL;
    pnote->sender = str_dup( ch->name );
    pnote->date = str_dup( "" );
    pnote->to_list = str_dup( "" );
    pnote->subject = str_dup( "" );
    pnote->text = str_dup( "" );
    pnote->type = type;
    ch->pnote = pnote;
    return;
}

void note_remove( CHAR_DATA *ch, NOTE_DATA *pnote, bool idelete )
{
    char                    to_new[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    to_one[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    NOTE_DATA              *prev = NULL;
    NOTE_DATA             **list = NULL;
    const char             *to_list = NULL;

    if ( !idelete )
    {
        /*
         * make a new list 
         */
        to_new[0] = '\0';
        to_list = pnote->to_list;
        while ( *to_list != '\0' )
        {
            to_list = one_argument( to_list, to_one );
            if ( to_one[0] != '\0' && str_cmp( ch->name, to_one ) )
            {
                strcat( to_new, " " );
                strcat( to_new, to_one );
            }
        }
        /*
         * Just a simple recipient removal? 
         */
        if ( str_cmp( ch->name, pnote->sender ) && to_new[0] != '\0' )
        {
            free_string( pnote->to_list );
            pnote->to_list = str_dup( to_new + 1 );
            return;
        }
    }
    /*
     * nuke the whole note 
     */

    switch ( pnote->type )
    {
        default:
            return;
        case NOTE_NOTE:
            list = &note_list;
            break;
        case NOTE_IDEA:
            list = &idea_list;
            break;
        case NOTE_PENALTY:
            list = &penalty_list;
            break;
        case NOTE_NEWS:
            list = &news_list;
            break;
        case NOTE_CHANGES:
            list = &changes_list;
            break;
    }

    /*
     * Remove note from linked list.
     */
    if ( pnote == *list )
    {
        *list = pnote->next;
    }
    else
    {
        for ( prev = *list; prev != NULL; prev = prev->next )
        {
            if ( prev->next == pnote )
                break;
        }

        if ( prev == NULL )
        {
            log_error( "%s", "Pnote not found" );
            return;
        }

        prev->next = pnote->next;
    }

    save_notes( pnote->type );
    free_note( pnote );
    return;
}

bool hide_note( CHAR_DATA *ch, NOTE_DATA *pnote )
{
    time_t                  last_read = 0;

    if ( IS_NPC( ch ) )
        return true;

    switch ( pnote->type )
    {
        default:
            return true;
        case NOTE_NOTE:
            last_read = ch->pcdata->last_note;
            break;
        case NOTE_IDEA:
            last_read = ch->pcdata->last_idea;
            break;
        case NOTE_PENALTY:
            last_read = ch->pcdata->last_penalty;
            break;
        case NOTE_NEWS:
            last_read = ch->pcdata->last_news;
            break;
        case NOTE_CHANGES:
            last_read = ch->pcdata->last_changes;
            break;
    }

    if ( pnote->date_stamp <= last_read )
        return true;

    if ( !str_cmp( ch->name, pnote->sender ) )
        return true;

    if ( !is_note_to( ch, pnote ) )
        return true;

    return false;
}

void update_read( CHAR_DATA *ch, NOTE_DATA *pnote )
{
    time_t                  stamp = 0;

    if ( IS_NPC( ch ) )
        return;

    stamp = pnote->date_stamp;

    switch ( pnote->type )
    {
        default:
            return;
        case NOTE_NOTE:
            ch->pcdata->last_note = UMAX( ch->pcdata->last_note, stamp );
            break;
        case NOTE_IDEA:
            ch->pcdata->last_idea = UMAX( ch->pcdata->last_idea, stamp );
            break;
        case NOTE_PENALTY:
            ch->pcdata->last_penalty = UMAX( ch->pcdata->last_penalty, stamp );
            break;
        case NOTE_NEWS:
            ch->pcdata->last_news = UMAX( ch->pcdata->last_news, stamp );
            break;
        case NOTE_CHANGES:
            ch->pcdata->last_changes = UMAX( ch->pcdata->last_changes, stamp );
            break;
    }
}

void parse_note( CHAR_DATA *ch, const char *argument, int type )
{
    BUFFER                 *buffer = NULL;
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    NOTE_DATA              *pnote = NULL;
    NOTE_DATA             **list = NULL;
    const char             *list_name = NULL;
    int                     vnum = 0;
    int                     anum = 0;
    char                    local_argument[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    const char             *lap = local_argument;

    if ( IS_NPC( ch ) )
        return;

    switch ( type )
    {
        default:
            return;
        case NOTE_NOTE:
            list = &note_list;
            list_name = "notes";
            break;
        case NOTE_IDEA:
            list = &idea_list;
            list_name = "ideas";
            break;
        case NOTE_PENALTY:
            list = &penalty_list;
            list_name = "penalties";
            break;
        case NOTE_NEWS:
            list = &news_list;
            list_name = "news";
            break;
        case NOTE_CHANGES:
            list = &changes_list;
            list_name = "changes";
            break;
    }

    strcpy( local_argument, argument );
    smash_tilde( local_argument );                     /* I moved smash_tilde above the
                                                        * call to one_argument */
    lap = one_argument( lap, arg );

    if ( arg[0] == '\0' || !str_prefix( arg, "read" ) )
    {
        bool                    fAll;

        if ( !str_cmp( lap, "all" ) )
        {
            fAll = true;
            anum = 0;
        }

        else if ( lap[0] == '\0' || !str_prefix( lap, "next" ) )
            /*
             * read next unread note 
             */
        {
            vnum = 0;
            for ( pnote = *list; pnote != NULL; pnote = pnote->next )
            {
                if ( !hide_note( ch, pnote ) )
                {
                    page_printf( ch, "[%3d] %s: %s\r\n%s\r\nTo: %s\r\n%s",
                                 vnum, pnote->sender, pnote->subject, pnote->date,
                                 pnote->to_list, pnote->text );
                    update_read( ch, pnote );
                    return;
                }
                else if ( is_note_to( ch, pnote ) )
                    vnum++;
            }
            ch_printf( ch, "You have no unread %s.\r\n", list_name );
            return;
        }

        else if ( is_number( lap ) )
        {
            fAll = false;
            anum = atoi( lap );
        }
        else
        {
            ch_printf( ch, "Read which number?\r\n" );
            return;
        }

        vnum = 0;
        for ( pnote = *list; pnote != NULL; pnote = pnote->next )
        {
            if ( is_note_to( ch, pnote ) && ( vnum++ == anum || fAll ) )
            {
                page_printf( ch, "[%3d] %s: %s\r\n%s\r\nTo: %s\r\n%s",
                             vnum - 1, pnote->sender, pnote->subject, pnote->date,
                             pnote->to_list, pnote->text );
                update_read( ch, pnote );
                return;
            }
        }

        ch_printf( ch, "There aren't that many %s.\r\n", list_name );
        return;
    }

    if ( !str_prefix( arg, "list" ) )
    {
        vnum = 0;
        for ( pnote = *list; pnote != NULL; pnote = pnote->next )
        {
            if ( is_note_to( ch, pnote ) )
            {
                ch_printf( ch, "[%3d%s] %s: %s\r\n",
                           vnum, hide_note( ch, pnote ) ? " " : "N",
                           pnote->sender, pnote->subject );
                vnum++;
            }
        }
        if ( !vnum )
        {
            switch ( type )
            {
                case NOTE_NOTE:
                    ch_printf( ch, "There are no notes for you.\r\n" );
                    break;
                case NOTE_IDEA:
                    ch_printf( ch, "There are no ideas for you.\r\n" );
                    break;
                case NOTE_PENALTY:
                    ch_printf( ch, "There are no penalties for you.\r\n" );
                    break;
                case NOTE_NEWS:
                    ch_printf( ch, "There is no news for you.\r\n" );
                    break;
                case NOTE_CHANGES:
                    ch_printf( ch, "There are no changes for you.\r\n" );
                    break;
            }
        }
        return;
    }

    if ( !str_prefix( arg, "remove" ) )
    {
        if ( !is_number( lap ) )
        {
            ch_printf( ch, "Note remove which number?\r\n" );
            return;
        }

        anum = atoi( lap );
        vnum = 0;
        for ( pnote = *list; pnote != NULL; pnote = pnote->next )
        {
            if ( is_note_to( ch, pnote ) && vnum++ == anum )
            {
                note_remove( ch, pnote, false );
                ch_printf( ch, "Ok.\r\n" );
                return;
            }
        }

        ch_printf( ch, "There aren't that many %s.", list_name );
        return;
    }

    if ( !str_prefix( arg, "delete" ) && get_trust( ch ) >= MAX_LEVEL - 1 )
    {
        if ( !is_number( lap ) )
        {
            ch_printf( ch, "Note delete which number?\r\n" );
            return;
        }

        anum = atoi( lap );
        vnum = 0;
        for ( pnote = *list; pnote != NULL; pnote = pnote->next )
        {
            if ( is_note_to( ch, pnote ) && vnum++ == anum )
            {
                note_remove( ch, pnote, true );
                ch_printf( ch, "Ok.\r\n" );
                return;
            }
        }

        ch_printf( ch, "There aren't that many %s.", list_name );
        return;
    }

    if ( !str_prefix( arg, "catchup" ) )
    {
        switch ( type )
        {
            case NOTE_NOTE:
                ch->pcdata->last_note = current_time;
                break;
            case NOTE_IDEA:
                ch->pcdata->last_idea = current_time;
                break;
            case NOTE_PENALTY:
                ch->pcdata->last_penalty = current_time;
                break;
            case NOTE_NEWS:
                ch->pcdata->last_news = current_time;
                break;
            case NOTE_CHANGES:
                ch->pcdata->last_changes = current_time;
                break;
        }
        return;
    }

    /*
     * below this point only certain people can edit notes 
     */
    if ( ( type == NOTE_NEWS && !IS_TRUSTED( ch, ANGEL ) )
         || ( type == NOTE_CHANGES && !IS_TRUSTED( ch, CREATOR ) ) )
    {
        ch_printf( ch, "You aren't high enough level to write %s.", list_name );
        return;
    }

    if ( !str_cmp( arg, "+" ) )
    {
        note_attach( ch, type );
        if ( ch->pnote->type != type )
        {
            ch_printf( ch, "You already have a different note in progress.\r\n" );
            return;
        }

        if ( strlen( ch->pnote->text ) + strlen( lap ) >= 4096 )
        {
            ch_printf( ch, "Note too long.\r\n" );
            return;
        }

        buffer = new_buf(  );

        add_buf( buffer, ch->pnote->text );
        add_buf( buffer, lap );
        add_buf( buffer, "\r\n" );
        free_string( ch->pnote->text );
        ch->pnote->text = str_dup( buf_string( buffer ) );
        free_buf( buffer );
        ch_printf( ch, "Ok.\r\n" );
        return;
    }

    if ( !str_cmp( arg, "-" ) )
    {
        int                     len;
        bool                    found = false;

        note_attach( ch, type );
        if ( ch->pnote->type != type )
        {
            ch_printf( ch, "You already have a different note in progress.\r\n" );
            return;
        }

        if ( ch->pnote->text == NULL || ch->pnote->text[0] == '\0' )
        {
            ch_printf( ch, "No lines left to remove.\r\n" );
            return;
        }

        strcpy( buf, ch->pnote->text );

        for ( len = strlen( buf ); len > 0; len-- )
        {
            if ( buf[len] == '\r' )
            {
                if ( !found )                          /* back it up */
                {
                    if ( len > 0 )
                        len--;
                    found = true;
                }
                else                                   /* found the second one */
                {
                    buf[len + 1] = '\0';
                    free_string( ch->pnote->text );
                    ch->pnote->text = str_dup( buf );
                    return;
                }
            }
        }
        buf[0] = '\0';
        free_string( ch->pnote->text );
        ch->pnote->text = str_dup( buf );
        return;
    }

    if ( !str_prefix( arg, "subject" ) )
    {
        note_attach( ch, type );
        if ( ch->pnote->type != type )
        {
            ch_printf( ch, "You already have a different note in progress.\r\n" );
            return;
        }

        free_string( ch->pnote->subject );
        ch->pnote->subject = str_dup( lap );
        ch_printf( ch, "Ok.\r\n" );
        return;
    }

    if ( !str_prefix( arg, "to" ) )
    {
        note_attach( ch, type );
        if ( ch->pnote->type != type )
        {
            ch_printf( ch, "You already have a different note in progress.\r\n" );
            return;
        }
        free_string( ch->pnote->to_list );
        ch->pnote->to_list = str_dup( lap );
        ch_printf( ch, "Ok.\r\n" );
        return;
    }

    if ( !str_prefix( arg, "clear" ) )
    {
        if ( ch->pnote != NULL )
        {
            free_note( ch->pnote );
            ch->pnote = NULL;
        }

        ch_printf( ch, "Ok.\r\n" );
        return;
    }

    if ( !str_prefix( arg, "show" ) )
    {
        if ( ch->pnote == NULL )
        {
            ch_printf( ch, "You have no note in progress.\r\n" );
            return;
        }

        if ( ch->pnote->type != type )
        {
            ch_printf( ch, "You aren't working on that kind of note.\r\n" );
            return;
        }

        ch_printf( ch, "%s: %s\r\nTo: %s\r\n%s",
                   ch->pnote->sender, ch->pnote->subject, ch->pnote->to_list,
                   ch->pnote->text );
        return;
    }

    if ( !str_prefix( arg, "post" ) || !str_prefix( arg, "send" ) )
    {
        char                   *strtime;

        if ( ch->pnote == NULL )
        {
            ch_printf( ch, "You have no note in progress.\r\n" );
            return;
        }

        if ( ch->pnote->type != type )
        {
            ch_printf( ch, "You aren't working on that kind of note.\r\n" );
            return;
        }

        if ( !str_cmp( ch->pnote->to_list, "" ) )
        {
            ch_printf( ch,
                       "You need to provide a recipient (name, all, or immortal).\r\n" );
            return;
        }

        if ( !str_cmp( ch->pnote->subject, "" ) )
        {
            ch_printf( ch, "You need to provide a subject.\r\n" );
            return;
        }

        ch->pnote->next = NULL;
        strtime = ctime( &current_time );
        strtime[strlen( strtime ) - 1] = '\0';
        ch->pnote->date = str_dup( strtime );
        ch->pnote->date_stamp = current_time;

        append_note( ch->pnote );
        ch->pnote = NULL;
        return;
    }

    ch_printf( ch, "You can't do that.\r\n" );
    return;
}

