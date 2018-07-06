/*
 * RAM $Id: bug.c 26 2008-11-02 07:01:25Z ram $
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include "merc.h"
#include "db.h"

/*
 * Reports a bug.
 */
void bug( const char *str, int param )
{
    event_logger( LOG_ERROR, NULL, NULL, NULL, 0, NULL, NULL, GOD, str, param );
}

/*
 * Writes a string to the log.
 */
void log_string( const char *str )
{
    event_logger( LOG_INFO, NULL, NULL, NULL, 0, NULL, NULL, GOD, str, 0 );
}

/*
 * These should NOT be longer than 8 characters
 */
static const char      *LogNames[] = {
    "INFO",
    "ERROR",
    "FATAL",
    "BOOT",
    "AUTH",
    "KILL",
    "BALANCE",
    "RESET"
};

/*
 * Things we want to have when logging events.....
 *
 * Type is the type of error, typically things like
 * LOG_INFO, LOG_ERROR, LOG_FATAL, LOG_BOOT, LOG_AUTH
 *
 * LogFile is the filename you want to log to, NULL means stderr
 *
 * File, Func, Line can all be provided by the compiler as
 * __FILE__, __PRETTY_FUNCTION__, and __LINE__
 *
 * ch is the character structure that caused the log,
 * victim is the target of the action, if any.
 *
 * Level is the minimum character level which will see the
 * bug if they're logged in.
 *
 * Str is, of course, the message, and it gets printed
 * using varargs, so you can have this be a printf type
 * set of macros.
 */
void event_logger( unsigned int Type, const char *LogFile,
                   const char *File, const char *Func, int Line,
                   struct char_data *ch, struct char_data *victim,
                   int Level, const char *Str, ... )
{
    va_list                 arg;
    char                    Result[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    Temp[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    FILE                   *fp = NULL;
    struct timeval          right_now;
    struct tm              *now_part = NULL;
    struct descriptor_data *d = NULL;

    /*
     * Cat got your tongue? 
     */
    if ( !Str || !*Str )
        return;

    /*
     * Get the time we logged the error 
     */
    gettimeofday( &right_now, NULL );

    /*
     * Parse the varargs 
     */
    va_start( arg, Str );
    vsnprintf( Temp, MAX_STRING_LENGTH, Str, arg );
    va_end( arg );

    /*
     * Construct a message and send to everyone 
     * This should eventually have bits to disable certain
     * types of messages.
     */
    sprintf( Result, "LOG_%s> %s\r\n", LogNames[Type], Temp );

    for ( d = descriptor_list; d; d = d->next )
        if ( d->connected == CON_PLAYING && get_trust( d->character ) >= Level )
            switch ( Type )
            {
                default:
                    write_to_buffer( d, Result, 0 );
                    break;
                case LOG_BOOT:
                    break;
                case LOG_RESET:
                    if ( IS_SET( d->character->wiznet, WIZ_RESETS ) )
                        write_to_buffer( d, Result, 0 );
                    break;
                case LOG_KILL:
                    if ( IS_SET( d->character->wiznet, WIZ_DEATHS )
                         || IS_SET( d->character->wiznet, WIZ_MOBDEATHS ) )
                        write_to_buffer( d, Result, 0 );
                    break;
            }

    /*
     * Format the data for the logs now, not the whizzards 
     */
    now_part = localtime( ( const time_t * ) &( right_now.tv_sec ) );
    snprintf( Result, MAX_STRING_LENGTH, "<: %04d-%02d-%02d %02d:%02d:%02d.%03d : %-8.8s",
              now_part->tm_year + 1900, now_part->tm_mon + 1, now_part->tm_mday,
              now_part->tm_hour, now_part->tm_min, now_part->tm_sec,
              ( int ) ( right_now.tv_usec / 1000 ), LogNames[Type] );

    /*
     * If we're booting, we want the area file information 
     */
    if ( fpArea )
    {
        int                     iLine = 0;
        long                    iChar = 0;
        int                     c = 0;

        if ( fpArea == stdin )
        {
            iLine = 0;
        }
        else
        {
            iChar = ftell( fpArea );
            fseek( fpArea, 0, SEEK_SET );
            for ( iLine = 0; ( ftell( fpArea ) < iChar ) && !feof( fpArea ); iLine++ )
            {
                while ( ( c = fgetc( fpArea ) ) != '\n' )
                    if ( c == EOF )
                        break;
            }
            fseek( fpArea, iChar, SEEK_SET );
        }

        snprintf( Result + strlen( Result ), MAX_STRING_LENGTH - strlen( Result ),
                  " [ FILE: %s, LINE: %d ]", strArea, iLine );
    }

    /*
     * Now, hook us up with file/function/line from the bug macros 
     */
    if ( File || Func || Line )
    {
        strncat( Result, " (", MAX_STRING_LENGTH - strlen( Result ) - 1 );
        if ( File && *File )
        {
            strncat( Result, File, MAX_STRING_LENGTH - strlen( Result ) - 1 );
        }
        if ( Func && *Func )
            snprintf( Result + strlen( Result ), MAX_STRING_LENGTH - strlen( Result ),
                      ";%s", Func );
        if ( Line )
            snprintf( Result + strlen( Result ), MAX_STRING_LENGTH - strlen( Result ),
                      ",%d)", Line );
        else
            strncat( Result, ")", MAX_STRING_LENGTH - strlen( Result ) - 1 );
    }

    /*
     * And then let us know who did it, and where they were, and whom they were with 
     */
    if ( ch || victim )
    {
        if ( ch )
            snprintf( Result + strlen( Result ), MAX_STRING_LENGTH - strlen( Result ),
                      " ch \"%s\" [#%d]", NAME( ch ),
                      ch->in_room ? ch->in_room->vnum : -1 );
        if ( victim )
            snprintf( Result + strlen( Result ), MAX_STRING_LENGTH - strlen( Result ),
                      " victim \"%s\" [#%d]", NAME( victim ),
                      victim->in_room ? victim->in_room->vnum : -1 );
        strncat( Result, "\n", MAX_STRING_LENGTH - strlen( Result ) - 1 );
    }
    else
    {
        if ( File || Func || Line || fpArea )
            strncat( Result, "\n", MAX_STRING_LENGTH - strlen( Result ) - 1 );
    }

    /*
     * Finally, slap on the actual message 
     */
    strncat( Result, " : ", MAX_STRING_LENGTH - strlen( Result ) - 1 );
    strncat( Result, Temp, MAX_STRING_LENGTH - strlen( Result ) - 1 );

    /*
     * Shoot it to a file, if desired 
     */
    if ( LogFile && *LogFile )
    {
        if ( !( fp = fopen( LogFile, "a" ) ) )
        {
            char                   *e = strerror( errno );

            fprintf( stderr, "bug_logger: %s: %s\n", LogFile, e );
            if ( ch )
                ch_printf( ch, "Could not open the file!\r\n" );
        }
        else
        {
            fprintf( fp, "%s\n", Result );
            fclose( fp );
        }
    }
    else if ( stderr )
    {
        fprintf( stderr, "%s\n", Result );
        fflush( stderr );
    }
}
