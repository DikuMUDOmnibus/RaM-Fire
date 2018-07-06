/*
 * RAM $Id: comm.c 69 2009-01-11 18:13:26Z quixadhal $
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
 *  Thanks to abaddon for proof-reading our comm.c and pointing out bugs.  *
 *  Any remaining bugs are, of course, our work, not his.  :)              *
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

/*
 * This file contains all of the OS-dependent stuff:
 *   startup, signals, BSD sockets for tcp/ip, i/o, timing.
 *
 * The data flow for input is:
 *    Game_loop ---> Read_from_descriptor ---> Read
 *    Game_loop ---> Read_from_buffer
 *
 * The data flow for output is:
 *    Game_loop ---> Process_Output ---> Write_to_descriptor -> Write
 *
 * The OS-dependent functions are Read_from_descriptor and Write_to_descriptor.
 * -- Furey  26 Jan 1993
 */

#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/telnet.h>
#include <stdarg.h>

#include <string>

#if !defined(NOCRYPT)
#include "sha256.h"
#endif

#include "merc.h"
#include "tables.h"
#include "strings.h"
#include "db.h"
#include "act.h"
#include "interp.h"
#include "magic.h"
#include "olc.h"
#include "ban.h"

/*
 * Socket and TCP/IP stuff.
 */
const char              echo_off_str[] = { IAC, WILL, TELOPT_ECHO, '\0' };
const char              echo_on_str[] = { IAC, WONT, TELOPT_ECHO, '\0' };
const char              go_ahead_str[] = { IAC, GA, '\0' };

/*
 * Global variables.
 */
DESCRIPTOR_DATA        *descriptor_list = NULL;        /* All open descriptors */
DESCRIPTOR_DATA        *descriptor_free = NULL;
bool                    need_god = false;              /* New Game, First login is IMP! */
bool                    merc_down = false;             /* Shutdown */
bool                    wizlock = false;               /* Game is wizlocked */
bool                    newlock = false;               /* Game is newlocked */
char                    str_boot_time[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
time_t                  current_time = 0;              /* time of this pulse */

DESCRIPTOR_DATA        *new_descriptor( void )
{
    static DESCRIPTOR_DATA  d_zero;
    DESCRIPTOR_DATA        *d = NULL;

    if ( descriptor_free == NULL )
        d = ( DESCRIPTOR_DATA * ) alloc_perm( sizeof( *d ) );
    else
    {
        d = descriptor_free;
        descriptor_free = descriptor_free->next;
    }

    *d = d_zero;
    VALIDATE( d );
    return d;
}

void free_descriptor( DESCRIPTOR_DATA *d )
{
    if ( !IS_VALID( d ) )
        return;

    free_string( d->host );
    free_mem( d->outbuf, d->outsize );
    INVALIDATE( d );
    d->next = descriptor_free;
    descriptor_free = d;
}

int main( int argc, char **argv )
{
    struct timeval          now_time;
    int                     port = DEFAULT_PORT;
    int                     control = 0;
    int                     pos = 1;
    char                   *dir = NULL;
    char                   *pidfile = NULL;
    char                   *logfile = NULL;

    /*
     * Init time.
     */
    gettimeofday( &now_time, NULL );
    current_time = ( time_t ) now_time.tv_sec;
    strcpy( str_boot_time, ctime( &current_time ) );

    /*
     * Parse the flag arguments.
     */
    while ( ( pos < argc ) && ( *( argv[pos] ) == '-' ) )
    {
        switch ( *( argv[pos] + 1 ) )
        {
            case 'h':
                proper_exit( MUD_HALT,
                             "Usage: %s [-n][-w][-g] [-d pathname] [-p pidfile] [-l logfile] [ port # ]\n",
                             argv[0] );
                break;
            case 'g':
                need_god = true;
                log_boot( "GOD MODE is SET!" );
                break;
            case 'n':
                newlock = true;
                log_boot( "NewLock is SET." );
                break;
            case 'w':
                wizlock = true;
                newlock = true;
                log_boot( "WizLock is SET." );
                break;
            case 'd':
                if ( *( argv[pos] + 2 ) )
                    dir = argv[pos] + 2;
                else if ( ++pos < argc )
                    dir = argv[pos];
                else
                    proper_exit( MUD_HALT, "Directory arg expected after option -d." );
                break;
            case 'l':
                if ( *( argv[pos] + 2 ) )
                    logfile = argv[pos] + 2;
                else if ( ++pos < argc )
                    logfile = argv[pos];
                else
                    proper_exit( MUD_HALT, "LOG filename expected after option -l." );
#if defined(__APPLE__)
                proper_exit( MUD_HALT, "LOG file redirection is not supported under OS X." );
#endif
                break;
            case 'p':
                if ( *( argv[pos] + 2 ) )
                    pidfile = argv[pos] + 2;
                else if ( ++pos < argc )
                    pidfile = argv[pos];
                else
                    proper_exit( MUD_HALT, "PID filename expected after option -p." );
                break;
            default:
                log_info( "Unknown option -%c in argument string.", *( argv[pos] + 1 ) );
                break;
        }
        pos++;
    }
    /*
     * Get the port, default if not specified.
     */
    if ( pos < argc )
    {
        if ( !is_number( argv[pos] ) )
            proper_exit( MUD_HALT,
                         "Usage: %s [-n][-w][-g] [-d pathname] [-p pidfile] [-l logfile] [ port # ]\n",
                         argv[0] );
        else if ( ( port = atoi( argv[pos] ) ) <= 1024 )
            proper_exit( MUD_HALT, "Illegal port # %d", port );
    }

    /*
     * Change to data directory.
     */
    if ( dir )
    {
        if ( chdir( dir ) < 0 )
            proper_exit( MUD_HALT, "Cannot change directory to %s", dir );
        log_boot( "Using %s as data directory.", dir );
    }

    /*
     * If using a PID file, write our PID out there now
     */
    if ( pidfile )
    {
        FILE                   *pidfp = NULL;

        if ( !( pidfp = fopen( pidfile, "w" ) ) )
            proper_exit( MUD_HALT, "Cannot open PID file %s", pidfile );
        fprintf( pidfp, "%d\n", getpid(  ) );
        fclose( pidfp );
        log_boot( "PID written to %s", pidfile );
    }

#if !defined(__APPLE__)
    /*
     * If we want stderr to go to a filename, now's the time!
     * Unless we're on OS X -- Ghasatta.
     */
    if ( logfile )
    {
        log_boot( "Switching to %s as stderr.", logfile );
        stderr = freopen( logfile, "a", stderr );
        if ( !stderr )
            proper_exit( MUD_HALT, "Cannot reopen stderr!" );
        close( fileno( stdout ) );
        dup2( fileno( stderr ), fileno( stdout ) );
        log_boot( "Switch to %s completed.", logfile );
    }
#endif

    /*
     * Run the game.
     */
    boot_db(  );
    control = init_listening_socket( port );
    log_boot( "RaM is ready to rock on port %d.", port );
    game_loop( control );
    close( control );

    /*
     * That's all, folks.
     */
    proper_exit( MUD_REBOOT, "Normal termination of game." );
    return MUD_REBOOT;
}

int init_listening_socket( int port )
{
    static struct sockaddr_in sa_zero;
    struct sockaddr_in      sa;
    int                     x = 1;
    int                     fd = -1;

    if ( ( fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
    {
        char                   *e = strerror( errno );

        proper_exit( MUD_HALT, "Init_socket: socket: %s\n", e );
    }

    if ( setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, ( char * ) &x, sizeof( x ) ) < 0 )
    {
        char                   *e = strerror( errno );

        close( fd );
        proper_exit( MUD_HALT, "Init_socket: SO_REUSEADDR: %s\n", e );
    }

#if defined(SO_DONTLINGER) && !defined(SYSV)
    {
        struct linger           ld;

        ld.l_onoff = 1;
        ld.l_linger = 1000;

        if ( setsockopt( fd, SOL_SOCKET, SO_DONTLINGER,
                         ( char * ) &ld, sizeof( ld ) ) < 0 )
        {
            char                   *e = strerror( errno );

            close( fd );
            proper_exit( MUD_HALT, "Init_socket: SO_DONTLINGER: %s\n", e );
        }
    }
#endif

    sa = sa_zero;
    sa.sin_family = AF_INET;
    sa.sin_port = htons( port );

    if ( bind( fd, ( struct sockaddr * ) &sa, sizeof( sa ) ) < 0 )
    {
        char                   *e = strerror( errno );

        close( fd );
        proper_exit( MUD_HALT, "Init socket: bind: %s\n", e );
    }

    if ( listen( fd, 3 ) < 0 )
    {
        char                   *e = strerror( errno );

        close( fd );
        proper_exit( MUD_HALT, "Init socket: listen: %s\n", e );
    }

    return fd;
}

void game_loop( int control )
{
    static struct timeval   null_time;
    struct timeval          last_time;
    DESCRIPTOR_DATA        *d_next = NULL;

    log_boot( "Setting up signal handling." );
    signal( SIGPIPE, SIG_IGN );
    signal( SIGINT, shutdown_request );
    signal( SIGTERM, shutdown_request );

    gettimeofday( &last_time, NULL );
    current_time = ( time_t ) last_time.tv_sec;

    /*
     * Main loop 
     */
    while ( !merc_down )
    {
        fd_set                  in_set;
        fd_set                  out_set;
        fd_set                  exc_set;
        DESCRIPTOR_DATA        *d = NULL;
        int                     maxsock = -1;

        /*
         * Poll all active descriptors.
         */
        FD_ZERO( &in_set );
        FD_ZERO( &out_set );
        FD_ZERO( &exc_set );
        FD_SET( control, &in_set );
        maxsock = control;
        for ( d = descriptor_list; d; d = d->next )
        {
            maxsock = UMAX( maxsock, d->socket );
            FD_SET( d->socket, &in_set );
            FD_SET( d->socket, &out_set );
            FD_SET( d->socket, &exc_set );
        }

        if ( select( maxsock + 1, &in_set, &out_set, &exc_set, &null_time ) < 0 )
        {
            char                   *e = strerror( errno );

            proper_exit( MUD_HALT, "Game_loop: select: poll: %s\n", e );
        }

        /*
         * New connection?
         */
        if ( FD_ISSET( control, &in_set ) )
            init_descriptor( control );

        /*
         * Kick out the freaky folks.
         */
        for ( d = descriptor_list; d != NULL; d = d_next )
        {
            d_next = d->next;
            if ( FD_ISSET( d->socket, &exc_set ) )
            {
                FD_CLR( d->socket, &in_set );
                FD_CLR( d->socket, &out_set );
                if ( d->character && d->connected == CON_PLAYING )
                    save_char_obj( d->character );
                d->outtop = 0;
                close_descriptor( d );
            }
        }

        /*
         * Process input.
         */
        for ( d = descriptor_list; d != NULL; d = d_next )
        {
            d_next = d->next;
            d->fcommand = false;

            if ( FD_ISSET( d->socket, &in_set ) )
            {
                if ( d->character != NULL )
                    d->character->timer = 0;
                if ( !read_from_descriptor( d ) )
                {
                    FD_CLR( d->socket, &out_set );
                    if ( d->character != NULL && d->connected == CON_PLAYING )
                        save_char_obj( d->character );
                    d->outtop = 0;
                    close_descriptor( d );
                    continue;
                }
            }

            if ( d->character != NULL && d->character->daze > 0 )
                --d->character->daze;

            if ( d->character != NULL && d->character->wait > 0 )
            {
                --d->character->wait;
                continue;
            }

            read_from_buffer( d );
            if ( d->incomm[0] != '\0' )
            {
                d->fcommand = true;
                stop_idling( d->character );

                if ( d->showstr_point )
                    show_string( d, d->incomm );
                else if ( d->pString )
                    string_add( d->character, d->incomm );
                else 
                {
                    if ( d->connected == CON_PLAYING )
                    {
                        if ( !run_olc_editor( d ) )
                            substitute_alias( d, d->incomm );
                    }
                    else
                        nanny( d, d->incomm );
                }

                d->incomm[0] = '\0';
            }
        }

        /*
         * Autonomous game motion.
         */
        update_handler(  );

        /*
         * Output.
         */
        for ( d = descriptor_list; d != NULL; d = d_next )
        {
            d_next = d->next;

            if ( ( d->fcommand || d->outtop > 0 ) && FD_ISSET( d->socket, &out_set ) )
            {
                if ( !process_output( d, true ) )
                {
                    if ( d->character != NULL && d->connected == CON_PLAYING )
                        save_char_obj( d->character );
                    d->outtop = 0;
                    close_descriptor( d );
                }
            }
        }

        /*
         * Synchronize to a clock.
         * Sleep( last_time + 1/PULSE_PER_SECOND - now ).
         * Careful here of signed versus unsigned arithmetic.
         */
        {
            struct timeval          now_time;
            int                     secDelta = 0;
            int                     usecDelta = 0;

            gettimeofday( &now_time, NULL );
            usecDelta = ( ( int ) last_time.tv_usec ) - ( ( int ) now_time.tv_usec )
                + 1000000 / PULSE_PER_SECOND;
            secDelta = ( ( int ) last_time.tv_sec ) - ( ( int ) now_time.tv_sec );
            while ( usecDelta < 0 )
            {
                usecDelta += 1000000;
                secDelta -= 1;
            }

            while ( usecDelta >= 1000000 )
            {
                usecDelta -= 1000000;
                secDelta += 1;
            }

            if ( secDelta > 0 || ( secDelta == 0 && usecDelta > 0 ) )
            {
                struct timeval          stall_time;

                stall_time.tv_usec = usecDelta;
                stall_time.tv_sec = secDelta;
                if ( select( 0, NULL, NULL, NULL, &stall_time ) < 0 )
                {
                    char                   *e = strerror( errno );

                    proper_exit( MUD_HALT, "Game_loop: select: stall: %s\n", e );
                }
            }
        }

        gettimeofday( &last_time, NULL );
        current_time = ( time_t ) last_time.tv_sec;
    }
    dump_player_list(  );
    return;
}

void init_descriptor( int control )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    DESCRIPTOR_DATA        *dnew = NULL;
    struct sockaddr_in      sock_in;
    struct hostent         *from = NULL;
    int                     sock = 0;
    socklen_t               size = sizeof( sock );

    getsockname( control, ( struct sockaddr * ) &sock_in, &size );
    if ( ( sock = accept( control, ( struct sockaddr * ) &sock_in, &size ) ) < 0 )
    {
        char                   *e = strerror( errno );

        log_error( "accept: %s", e );
        return;
    }

#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

    if ( fcntl( sock, F_SETFL, FNDELAY ) == -1 )
    {
        char                   *e = strerror( errno );

        log_error( "fcntl: FNDELAY: %s", e );
        return;
    }

    /*
     * Cons a new descriptor.
     */
    dnew = new_descriptor(  );

    dnew->socket = sock;
    dnew->connected = CON_GET_NAME;
    dnew->showstr_head = NULL;
    dnew->showstr_point = NULL;
    dnew->outsize = 2000;
    dnew->pEdit = NULL;                                /* OLC */
    dnew->pString = NULL;                              /* OLC */
    dnew->editor = 0;                                  /* OLC */
    dnew->outbuf = ( char * ) alloc_mem( dnew->outsize );

    size = sizeof( sock_in );
    if ( getpeername( sock, ( struct sockaddr * ) &sock_in, &size ) < 0 )
    {
        char                   *e = strerror( errno );

        log_error( "getpeername: %s", e );
        dnew->host = str_dup( "(unknown)" );
    }
    else
    {
        /*
         * Would be nice to use inet_ntoa here but it takes a struct arg,
         * which ain't very compatible between gcc and system libraries.
         */
        int                     addr = 0;

        addr = ntohl( sock_in.sin_addr.s_addr );
        sprintf( buf, "%d.%d.%d.%d",
                 ( addr >> 24 ) & 0xFF, ( addr >> 16 ) & 0xFF,
                 ( addr >> 8 ) & 0xFF, ( addr ) & 0xFF );
        log_auth( NULL, "sock_in.sinaddr:  %s", buf );
        from = gethostbyaddr( ( char * ) &sock_in.sin_addr,
                              sizeof( sock_in.sin_addr ), AF_INET );
        dnew->host = str_dup( from ? from->h_name : buf );
    }

    /*
     * Swiftest: I added the following to ban sites.  I don't
     * endorse banning of sites, but Copper has few descriptors now
     * and some people from certain sites keep abusing access by
     * using automated 'autodialers' and leaving connections hanging.
     *
     * Furey: added suffix check by request of Nickel of HiddenWorlds.
     */
    if ( check_ban( dnew->host, BAN_ALL ) )
    {
        write_to_socket( sock, "Your site has been banned from this mud.\r\n", 0 );
        close( sock );
        free_descriptor( dnew );
        return;
    }
    /*
     * Init descriptor data.
     */
    dnew->next = descriptor_list;
    descriptor_list = dnew;

    /*
     * Send the greeting.
     */
    {
        if ( help_greeting[0] == '.' )
            write_to_buffer( dnew, help_greeting + 1, 0 );
        else
            write_to_buffer( dnew, help_greeting, 0 );
    }

    return;
}

void close_descriptor( DESCRIPTOR_DATA *dclose )
{
    CHAR_DATA              *ch = NULL;

    if ( dclose->outtop > 0 )
        process_output( dclose, false );

    if ( dclose->snoop_by != NULL )
    {
        desc_printf( dclose->snoop_by, "Your victim has left the game.\r\n" );
    }

    {
        DESCRIPTOR_DATA        *d = NULL;

        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( d->snoop_by == dclose )
                d->snoop_by = NULL;
        }
    }

    if ( ( ch = dclose->character ) != NULL )
    {
        log_auth( ch, "Closing link to %s.", NAME( ch ) );
        update_player_list( ch, false );

        /*
         * A bugfix for people who log in with a pet, but then
         * abort their login (failed pasword), in which case the
         * pet becomes orphaned and has a NULL room pointer.
         */
        if ( ch->pet && ch->pet->in_room == NULL )
        {
            char_to_room( ch->pet, get_room_index( ROOM_VNUM_LIMBO ) );
            extract_char( ch->pet, true );
        }

        /*
         * cut down on wiznet spam when rebooting 
         */
        if ( dclose->connected == CON_PLAYING && !merc_down )
        {
            act( "$n has lost $s link.", ch, NULL, NULL, TO_ROOM );
            wiznet( "Net death has claimed $N.", ch, NULL, WIZ_LINKS, 0, 0 );
            ch->desc = NULL;
        }
        else
        {
            free_char( dclose->original ? dclose->original : dclose->character );
        }
    }

    /*
     * if ( d_next == dclose ) d_next = d_next->next; 
     */

    if ( dclose == descriptor_list )
    {
        descriptor_list = descriptor_list->next;
    }
    else
    {
        DESCRIPTOR_DATA        *d = NULL;

        for ( d = descriptor_list; d && d->next != dclose; d = d->next )
            ;
        if ( d != NULL )
            d->next = dclose->next;
        else
            log_error( "%s", "Descriptor at end of descriptor_list is NULL" );
    }

    close( dclose->socket );
    free_descriptor( dclose );
    return;
}

bool read_from_descriptor( DESCRIPTOR_DATA *d )
{
    size_t                  iStart = 0;

    /*
     * Hold horses if pending command already. 
     */
    if ( d->incomm[0] != '\0' )
        return true;

    /*
     * Check for overflow. 
     */
    iStart = strlen( d->inbuf );
    if ( iStart >= sizeof( d->inbuf ) - 10 )
    {
        log_auth( NULL, "%s input overflow!", d->host );
        write_to_socket( d->socket, "\r\n*** PUT A LID ON IT!!! ***\r\n", 0 );
        return false;
    }

    /*
     * Snarf input. 
     */
    for ( ;; )
    {
        int                     nRead = 0;

        nRead = read( ( int ) d->socket, ( void * ) ( d->inbuf + iStart ),
                      sizeof( d->inbuf ) - 10 - iStart );
        if ( nRead > 0 )
        {
            iStart += nRead;
            if ( d->inbuf[iStart - 1] == '\r' || d->inbuf[iStart - 1] == '\n' )
                break;
        }
        else if ( nRead == 0 )
        {
            log_error( "%s", "EOF encountered on socket read" );
            return false;
        }
        else if ( errno == EWOULDBLOCK )
            break;
        else
        {
            char                   *e = strerror( errno );

            log_error( "Socket error: %s", e );
            return false;
        }
    }

    d->inbuf[iStart] = '\0';
    return true;
}

/*
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer( DESCRIPTOR_DATA *d )
{
    int                     i = 0;
    int                     j = 0;
    int                     k = 0;

    /*
     * Hold horses if pending command already.
     */
    if ( d->incomm[0] != '\0' )
        return;

    /*
     * Look for at least one new line.
     */
    for ( i = 0; d->inbuf[i] != '\r' && d->inbuf[i] != '\n'; i++ )
    {
        if ( d->inbuf[i] == '\0' )
            return;
    }

    /*
     * Canonical input processing.
     */
    for ( i = 0, k = 0; d->inbuf[i] != '\r' && d->inbuf[i] != '\n'; i++ )
    {
        if ( k >= MAX_INPUT_LENGTH - 2 )
        {
            write_to_socket( d->socket, "Line too long.\r\n", 0 );

            /*
             * skip the rest of the line 
             */
            for ( ; d->inbuf[i] != '\0'; i++ )
            {
                if ( d->inbuf[i] == '\r' || d->inbuf[i] == '\n' )
                    break;
            }
            d->inbuf[i] = '\n';
            d->inbuf[i + 1] = '\0';
            break;
        }

        if ( d->inbuf[i] == '\b' && k > 0 )
            --k;
        else if ( isascii( d->inbuf[i] ) && isprint( d->inbuf[i] ) )
            d->incomm[k++] = d->inbuf[i];
    }

    /*
     * Finish off the line.
     */
    if ( k == 0 )
        d->incomm[k++] = ' ';
    d->incomm[k] = '\0';

    /*
     * Deal with bozos with #repeat 1000 ...
     */

    if ( k > 1 || d->incomm[0] == '!' )
    {
        if ( d->incomm[0] != '!' && strcmp( d->incomm, d->inlast ) )
        {
            d->repeat = 0;
        }
        else
        {
            if ( ++d->repeat >= 25 && d->character && d->connected == CON_PLAYING )
            {
                log_auth( NULL, "%s input spamming!", d->host );
                wiznet( "Spam spam spam $N spam spam spam spam spam!",
                        d->character, NULL, WIZ_SPAM, 0, get_trust( d->character ) );
                if ( d->incomm[0] == '!' )
                    wiznet( d->inlast, d->character, NULL, WIZ_SPAM, 0,
                            get_trust( d->character ) );
                else
                    wiznet( d->incomm, d->character, NULL, WIZ_SPAM, 0,
                            get_trust( d->character ) );

                d->repeat = 0;
/*
                write_to_socket( d->socket,
                    "\r\n*** PUT A LID ON IT!!! ***\r\n", 0 );
                strcpy( d->incomm, "quit" );
*/
            }
        }
    }

    /*
     * Do '!' substitution.
     */
    if ( d->incomm[0] == '!' )
        strcpy( d->incomm, d->inlast );
    else
        strcpy( d->inlast, d->incomm );

    /*
     * Shift the input buffer.
     */
    while ( d->inbuf[i] == '\r' || d->inbuf[i] == '\n' )
        i++;
    for ( j = 0; ( d->inbuf[j] = d->inbuf[i + j] ) != '\0'; j++ )
        ;
    return;
}

/*
 * Low level output function.
 */
bool process_output( DESCRIPTOR_DATA *d, bool fPrompt )
{
    /*
     * Bust a prompt.
     */
    if ( !merc_down && d->showstr_point )
        desc_printf( d, "[Hit Return to continue]\r\n" );
    else if ( fPrompt && !merc_down && d->connected == CON_PLAYING && d->pString )
        desc_printf( d, "> " );
    else if ( fPrompt && !merc_down && d->connected == CON_PLAYING )
    {
        CHAR_DATA              *ch = NULL;
        CHAR_DATA              *victim = NULL;

        ch = d->character;

        /*
         * battle prompt 
         */
        if ( ( victim = ch->fighting ) != NULL && can_see( ch, victim ) )
        {
            int                     percent = 0;

            if ( victim->max_hit > 0 )
                percent = victim->hit * 100 / victim->max_hit;
            else
                percent = -1;

            if ( percent >= 100 )
                desc_printf( d, "%c%s %s\r\n", UPPER( *NAME( victim ) ),
                             ( NAME( victim ) + 1 ), "is in excellent condition." );
            else if ( percent >= 90 )
                desc_printf( d, "%c%s %s\r\n", UPPER( *NAME( victim ) ),
                             ( NAME( victim ) + 1 ), "has a few scratches." );
            else if ( percent >= 75 )
                desc_printf( d, "%c%s %s\r\n", UPPER( *NAME( victim ) ),
                             ( NAME( victim ) + 1 ),
                             "has some small wounds and bruises." );
            else if ( percent >= 50 )
                desc_printf( d, "%c%s %s\r\n", UPPER( *NAME( victim ) ),
                             ( NAME( victim ) + 1 ), "has quite a few wounds." );
            else if ( percent >= 30 )
                desc_printf( d, "%c%s %s\r\n", UPPER( *NAME( victim ) ),
                             ( NAME( victim ) + 1 ),
                             "has some big nasty wounds and scratches." );
            else if ( percent >= 15 )
                desc_printf( d, "%c%s %s\r\n", UPPER( *NAME( victim ) ),
                             ( NAME( victim ) + 1 ), "looks pretty hurt." );
            else if ( percent >= 0 )
                desc_printf( d, "%c%s %s\r\n", UPPER( *NAME( victim ) ),
                             ( NAME( victim ) + 1 ), "is in awful condition." );
            else
                desc_printf( d, "%c%s %s\r\n", UPPER( *NAME( victim ) ),
                             ( NAME( victim ) + 1 ), "is bleeding to death." );
        }

        ch = d->original ? d->original : d->character;
        if ( !IS_SET( ch->comm, COMM_COMPACT ) )
            desc_printf( d, "\r\n" );

        if ( IS_SET( ch->comm, COMM_PROMPT ) )
            bust_a_prompt( d->character );

        if ( IS_SET( ch->comm, COMM_TELNET_GA ) )
            write_to_buffer( d, go_ahead_str, 0 );
    }

    /*
     * Short-circuit if nothing to write.
     */
    if ( d->outtop == 0 )
        return true;

    /*
     * Snoop-o-rama.
     */
    if ( d->snoop_by != NULL )
    {
        if ( d->character != NULL )
            write_to_buffer( d->snoop_by, d->character->name, 0 );
        write_to_buffer( d->snoop_by, "> ", 2 );
        write_to_buffer( d->snoop_by, d->outbuf, d->outtop );
    }

    /*
     * OS-dependent output.
     */
    if ( !write_to_socket( d->socket, d->outbuf, d->outtop ) )
    {
        d->outtop = 0;
        return false;
    }
    else
    {
        d->outtop = 0;
        return true;
    }
}

/*
 * Deal with sockets that haven't logged in yet.
 */
void nanny( DESCRIPTOR_DATA *d, const char *argument )
{
    DESCRIPTOR_DATA        *d_old = NULL;
    DESCRIPTOR_DATA        *d_next = NULL;
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *ch = NULL;
    const char             *pwdnew = NULL;
    const char             *p = NULL;
    int                     iClass = 0;
    int                     race = 0;
    int                     i = 0;
    int                     weapon = 0;
    bool                    fOld = false;
    char                    local_argument[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    const char             *lap = local_argument;

    while ( isspace( *argument ) )
        argument++;

    ch = d->character;

    switch ( d->connected )
    {

        default:
            log_error( "Bad d->connected state %d", d->connected );
            close_descriptor( d );
            return;

        case CON_GET_NAME:
            if ( argument[0] == '\0' )
            {
                close_descriptor( d );
                return;
            }

            strcpy( local_argument, argument );
            local_argument[0] = UPPER( local_argument[0] );
            if ( !check_parse_name( lap ) )
            {
                desc_printf( d, "Illegal name, try another.\r\nName: " );
                return;
            }

            fOld = load_char_obj( d, lap );
            ch = d->character;

            if ( IS_SET( ch->act, PLR_DENY ) )
            {
                log_auth( NULL, "Denying access to %s@%s.", lap, d->host );
                desc_printf( d, "You are denied access.\r\n" );
                close_descriptor( d );
                return;
            }

            if ( check_ban( d->host, BAN_PERMIT ) && !IS_SET( ch->act, PLR_PERMIT ) )
            {
                desc_printf( d, "Your site has been banned from this mud.\r\n" );
                close_descriptor( d );
                return;
            }

            if ( check_reconnect( d, lap, false ) )
            {
                fOld = true;
            }
            else
            {
                if ( wizlock && !IS_IMMORTAL( ch ) )
                {
                    desc_printf( d, "The game is wizlocked.\r\n" );
                    close_descriptor( d );
                    return;
                }
            }

            if ( fOld )
            {
                /*
                 * Old player 
                 */
                desc_printf( d, "Password: %s", echo_off_str );
                d->connected = CON_GET_OLD_PASSWORD;
                return;
            }
            else
            {
                /*
                 * New player 
                 */
                if ( newlock )
                {
                    desc_printf( d, "The game is newlocked.\r\n" );
                    close_descriptor( d );
                    return;
                }

                if ( check_ban( d->host, BAN_NEWBIES ) )
                {
                    desc_printf( d, "New players are not allowed from your site.\r\n" );
                    close_descriptor( d );
                    return;
                }

                desc_printf( d, "Did I get that right, %s (Y/N)? ", lap );
                d->connected = CON_CONFIRM_NEW_NAME;
                return;
            }
            break;

        case CON_GET_OLD_PASSWORD:
            desc_printf( d, "\r\n%s", echo_on_str );

            if ( strcmp( CRYPT( argument ), ch->pcdata->pwd ) )
            {
                desc_printf( d, "Wrong password.\r\n" );
                close_descriptor( d );
                return;
            }

            if ( check_playing( d, ch->name ) )
                return;

            if ( check_reconnect( d, ch->name, true ) )
                return;

            log_auth( ch, "%s@%s has connected.", NAME( ch ), d->host );
            wiznet( log_buf, NULL, NULL, WIZ_SITES, 0, get_trust( ch ) );

            if ( IS_IMMORTAL( ch ) )
            {
                do_function( ch, &do_help, "imotd" );
                d->connected = CON_READ_IMOTD;
            }
            else
            {
                do_function( ch, &do_help, "motd" );
                d->connected = CON_READ_MOTD;
            }
            break;

/* RT code for breaking link */

        case CON_BREAK_CONNECT:
            switch ( *argument )
            {
                case 'y':
                case 'Y':
                    for ( d_old = descriptor_list; d_old != NULL; d_old = d_next )
                    {
                        d_next = d_old->next;
                        if ( d_old == d || d_old->character == NULL )
                            continue;

                        if ( str_cmp( ch->name, d_old->original ?
                                      d_old->original->name : d_old->character->name ) )
                            continue;

                        close_descriptor( d_old );
                    }
                    if ( check_reconnect( d, ch->name, true ) )
                        return;
                    desc_printf( d, "Reconnect attempt failed.\r\nName: " );
                    if ( d->character != NULL )
                    {
                        free_char( d->character );
                        d->character = NULL;
                    }
                    d->connected = CON_GET_NAME;
                    break;

                case 'n':
                case 'N':
                    desc_printf( d, "Name: " );
                    if ( d->character != NULL )
                    {
                        free_char( d->character );
                        d->character = NULL;
                    }
                    d->connected = CON_GET_NAME;
                    break;

                default:
                    desc_printf( d, "Please type Y or N? " );
                    break;
            }
            break;

        case CON_CONFIRM_NEW_NAME:
            switch ( *argument )
            {
                case 'y':
                case 'Y':
                    desc_printf( d, "New character.\r\nGive me a password for %s: %s",
                                 NAME( ch ), echo_off_str );
                    d->connected = CON_GET_NEW_PASSWORD;
                    break;

                case 'n':
                case 'N':
                    desc_printf( d, "Ok, what IS it, then? " );
                    free_char( d->character );
                    d->character = NULL;
                    d->connected = CON_GET_NAME;
                    break;

                default:
                    desc_printf( d, "Please type Yes or No? " );
                    break;
            }
            break;

        case CON_GET_NEW_PASSWORD:
            desc_printf( d, "\r\n" );

            if ( strlen( argument ) < 5 )
            {
                desc_printf( d,
                             "Password must be at least five characters long.\r\nPassword: " );
                return;
            }

            pwdnew = CRYPT( argument );
            for ( p = pwdnew; *p != '\0'; p++ )
            {
                if ( *p == '~' )
                {
                    desc_printf( d,
                                 "New password not acceptable, try again.\r\nPassword: " );
                    return;
                }
            }

            free_string( ch->pcdata->pwd );
            ch->pcdata->pwd = str_dup( pwdnew );
            desc_printf( d, "Please retype password: " );
            d->connected = CON_CONFIRM_NEW_PASSWORD;
            break;

        case CON_CONFIRM_NEW_PASSWORD:
            desc_printf( d, "\r\n" );

            if ( strcmp( CRYPT( argument ), ch->pcdata->pwd ) )
            {
                desc_printf( d, "Passwords don't match.\r\nRetype password: " );
                d->connected = CON_GET_NEW_PASSWORD;
                return;
            }

            desc_printf( d, "%sThe following races are available:\r\n  ", echo_on_str );
            for ( race = 1; race_table[race].name != NULL; race++ )
            {
                if ( !race_table[race].pc_race )
                    break;
                desc_printf( d, "%s ", race_table[race].name );
            }
            desc_printf( d, "\r\nWhat is your race (help for more information)? " );
            d->connected = CON_GET_NEW_RACE;
            break;

        case CON_GET_NEW_RACE:
            one_argument( argument, arg );

            if ( !strcmp( arg, "help" ) )
            {
                argument = one_argument( argument, arg );
                if ( argument[0] == '\0' )
                    do_function( ch, &do_help, "race help" );
                else
                    do_function( ch, &do_help, argument );
                desc_printf( d, "What is your race (help for more information)? " );
                break;
            }

            race = race_lookup( argument );

            if ( race == 0 || !race_table[race].pc_race )
            {
                desc_printf( d, "That is not a valid race.\r\n" );
                desc_printf( d, "The following races are available:\r\n  " );
                for ( race = 1; race_table[race].name != NULL; race++ )
                {
                    if ( !race_table[race].pc_race )
                        break;
                    desc_printf( d, "%s ", race_table[race].name );
                }
                desc_printf( d, "\r\nWhat is your race? (help for more information) " );
                break;
            }

            ch->race = race;
            /*
             * initialize stats 
             */
            for ( i = 0; i < MAX_STATS; i++ )
                ch->perm_stat[i] = pc_race_table[race].stats[i];
            ch->affected_by = ch->affected_by | race_table[race].aff;
            ch->imm_flags = ch->imm_flags | race_table[race].imm;
            ch->res_flags = ch->res_flags | race_table[race].res;
            ch->vuln_flags = ch->vuln_flags | race_table[race].vuln;
            ch->form = race_table[race].form;
            ch->parts = race_table[race].parts;

            /*
             * add skills 
             */
            for ( i = 0; i < 5; i++ )
            {
                if ( pc_race_table[race].skills[i] == NULL )
                    break;
                group_add( ch, pc_race_table[race].skills[i], false );
            }
            /*
             * add cost 
             */
            ch->pcdata->points = pc_race_table[race].points;
            ch->size = pc_race_table[race].size;

            desc_printf( d, "What is your sex (M/F)? " );
            d->connected = CON_GET_NEW_SEX;
            break;

        case CON_GET_NEW_SEX:
            switch ( argument[0] )
            {
                case 'm':
                case 'M':
                    ch->sex = SEX_MALE;
                    ch->pcdata->true_sex = SEX_MALE;
                    break;
                case 'f':
                case 'F':
                    ch->sex = SEX_FEMALE;
                    ch->pcdata->true_sex = SEX_FEMALE;
                    break;
                default:
                    desc_printf( d, "That's not a sex.\r\nWhat IS your sex? " );
                    return;
            }

            desc_printf( d, "Select a class [" );
            for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
            {
                desc_printf( d, "%s%s", iClass == 0 ? "" : " ",
                             class_table[iClass].name );
            }
            desc_printf( d, "]: " );
            d->connected = CON_GET_NEW_CLASS;
            break;

        case CON_GET_NEW_CLASS:
            iClass = class_lookup( argument );

            if ( iClass == -1 )
            {
                desc_printf( d, "That's not a class.\r\nWhat IS your class? " );
                return;
            }

            ch->iclass = iClass;

            log_auth( ch, "%s@%s is a new player.", NAME( ch ), d->host );
            wiznet( "Newbie alert!  $N sighted.", ch, NULL, WIZ_NEWBIE, 0, 0 );
            wiznet( log_buf, NULL, NULL, WIZ_SITES, 0, get_trust( ch ) );

            desc_printf( d, "\r\nYou may be good, neutral, or evil.\r\n" );
            desc_printf( d, "Which alignment (G/N/E)? " );
            d->connected = CON_GET_ALIGNMENT;
            break;

        case CON_GET_ALIGNMENT:
            switch ( argument[0] )
            {
                case 'g':
                case 'G':
                    ch->alignment = 750;
                    break;
                case 'n':
                case 'N':
                    ch->alignment = 0;
                    break;
                case 'e':
                case 'E':
                    ch->alignment = -750;
                    break;
                default:
                    desc_printf( d, "That's not a valid alignment.\r\n" );
                    desc_printf( d, "Which alignment (G/N/E)? " );
                    return;
            }

            group_add( ch, "rom basics", false );
            group_add( ch, class_table[ch->iclass].base_group, false );
            ch->pcdata->learned[skill_lookup( "recall" )] = 50;
            desc_printf( d, "\r\nDo you wish to customize this character?\r\n" );
            desc_printf( d,
                         "Customization takes time, but allows a wider range of skills and abilities.\r\n" );
            desc_printf( d, "Customize (Y/N)? " );
            d->connected = CON_DEFAULT_CHOICE;
            break;

        case CON_DEFAULT_CHOICE:
            desc_printf( d, "\r\n" );
            switch ( argument[0] )
            {
                case 'y':
                case 'Y':
                    ch->gen_data = new_gen_data(  );
                    ch->gen_data->points_chosen = ch->pcdata->points;
                    do_function( ch, &do_help, "group header" );
                    list_group_costs( ch );
                    desc_printf( d, "You already have the following skills:\r\n" );
                    do_function( ch, &do_skills, "" );
                    do_function( ch, &do_help, "menu choice" );
                    d->connected = CON_GEN_GROUPS;
                    break;
                case 'n':
                case 'N':
                    group_add( ch, class_table[ch->iclass].default_group, true );
                    desc_printf( d,
                                 "\r\nPlease pick a weapon from the following choices:\r\n" );
                    for ( i = 0; weapon_table[i].name != NULL; i++ )
                        if ( ch->pcdata->learned[skill_lookup( weapon_table[i].skill )] >
                             0 )
                        {
                            desc_printf( d, "%s ", weapon_table[i].name );
                        }
                    desc_printf( d, "\r\nYour choice? " );
                    d->connected = CON_PICK_WEAPON;
                    break;
                default:
                    desc_printf( d, "Please answer (Y/N)? " );
                    return;
            }
            break;

        case CON_PICK_WEAPON:
            desc_printf( d, "\r\n" );
            weapon = weapon_lookup( argument );
            if ( weapon == -1
                 || ch->pcdata->learned[skill_lookup( weapon_table[weapon].skill )] <= 0 )
            {
                desc_printf( d, "That's not a valid selection. Choices are:\r\n" );
                for ( i = 0; weapon_table[i].name != NULL; i++ )
                    if ( ch->pcdata->learned[skill_lookup( weapon_table[i].skill )] > 0 )
                    {
                        desc_printf( d, "%s ", weapon_table[i].name );
                    }
                desc_printf( d, "\r\nYour choice? " );
                return;
            }

            ch->pcdata->learned[skill_lookup( weapon_table[weapon].skill )] = 40;
            desc_printf( d, "\r\n" );
            do_function( ch, &do_help, "motd" );
            d->connected = CON_READ_MOTD;
            break;

        case CON_GEN_GROUPS:
            ch_printf( ch, "\r\n" );

            if ( !str_cmp( argument, "done" ) )
            {
                if ( ch->pcdata->points == pc_race_table[ch->race].points )
                {
                    ch_printf( ch, "You didn't pick anything.\r\n" );
                    break;
                }

                if ( ch->pcdata->points <= 40 + pc_race_table[ch->race].points )
                {
                    ch_printf( ch,
                               "You must take at least %d points of skills and groups",
                               40 + pc_race_table[ch->race].points );
                    break;
                }

                ch_printf( ch, "Creation points: %d\r\n", ch->pcdata->points );
                ch_printf( ch, "Experience per level: %d\r\n",
                           exp_per_level( ch, ch->gen_data->points_chosen ) );
                if ( ch->pcdata->points < 40 )
                    ch->train = ( 40 - ch->pcdata->points + 1 ) / 2;
                free_gen_data( ch->gen_data );
                ch->gen_data = NULL;
                desc_printf( d,
                             "\r\nPlease pick a weapon from the following choices:\r\n" );
                for ( i = 0; weapon_table[i].name != NULL; i++ )
                    if ( ch->pcdata->learned[skill_lookup( weapon_table[i].skill )] > 0 )
                    {
                        desc_printf( d, "%s ", weapon_table[i].name );
                    }
                desc_printf( d, "\r\nYour choice? " );
                d->connected = CON_PICK_WEAPON;
                break;
            }

            if ( !parse_gen_groups( ch, argument ) )
                ch_printf( ch,
                           "Choices are: list,learned,premise,add,drop,info,help, and done.\r\n" );

            do_function( ch, &do_help, "menu choice" );
            break;

        case CON_READ_IMOTD:
            desc_printf( d, "\r\n" );
            do_function( ch, &do_help, "motd" );
            d->connected = CON_READ_MOTD;
            break;

        case CON_READ_MOTD:
            if ( ch->pcdata == NULL || ch->pcdata->pwd[0] == '\0' )
            {
                desc_printf( d, "Warning! Null password!\r\n" );
                desc_printf( d, "Please report old password with bug.\r\n" );
                desc_printf( d, "Type 'password null <new password>' to fix.\r\n" );
            }

            desc_printf( d,
                         "\r\nWelcome to ROM 2.4.  Please do not feed the mobiles.\r\n" );
            ch->next = char_list;
            char_list = ch;
            d->connected = CON_PLAYING;
            reset_char( ch );

            if ( ch->level == 0 )
            {

                ch->perm_stat[class_table[ch->iclass].attr_prime] += 3;

                ch->level = 1;
                ch->pcdata->security = 0;
                ch->exp = exp_per_level( ch, ch->pcdata->points );
                ch->hit = ch->max_hit;
                ch->mana = ch->max_mana;
                ch->move = ch->max_move;
                ch->train = 3;
                ch->practice = 5;
                set_title( ch, "the %s",
                           title_table[ch->iclass][ch->level]
                           [ch->sex == SEX_FEMALE ? 1 : 0] );

                do_function( ch, &do_outfit, "" );
                obj_to_char( create_object( get_obj_index( OBJ_VNUM_MAP ), 0 ), ch );

                char_to_room( ch, get_room_index( ROOM_VNUM_SCHOOL ) );
                ch_printf( ch, "\r\n" );
                do_function( ch, &do_help, "newbie info" );
                ch_printf( ch, "\r\n" );
            }
            else if ( ch->in_room != NULL )
            {
                char_to_room( ch, ch->in_room );
            }
            else if ( IS_IMMORTAL( ch ) )
            {
                char_to_room( ch, get_room_index( ROOM_VNUM_CHAT ) );
            }
            else
            {
                char_to_room( ch, get_room_index( ROOM_VNUM_TEMPLE ) );
            }

            act( "$n has entered the game.", ch, NULL, NULL, TO_ROOM );
            do_function( ch, &do_look, "auto" );

            wiznet( "$N has left real life behind.", ch, NULL,
                    WIZ_LOGINS, WIZ_SITES, get_trust( ch ) );

            if ( ch->pet != NULL )
            {
                char_to_room( ch->pet, ch->in_room );
                act( "$n has entered the game.", ch->pet, NULL, NULL, TO_ROOM );
            }

            do_function( ch, &do_unread, "" );
            save_char_obj( ch );                       /* It can't hurt to save, can it? */

            if ( need_god )
            {
                ch_printf( ch, "\r\nCongratulations!\r\n"
                           "As the first new inhabitant of this realm, "
                           "you are now its ruler!\r\n" );
                while ( ch->level < IMPLEMENTOR )
                {
                    ch->level++;
                    advance_level( ch, true );
                }
                ch->exp = exp_per_level( ch, ch->pcdata->points ) * UMAX( 1, ch->level );
                ch->trust = ch->level;
                ch->pcdata->security = 9;
                save_char_obj( ch );
                log_auth( ch, "%s@%s is now an IMPLEMENTOR!", NAME( ch ), d->host );
                need_god = false;
            }
            update_player_list( ch, true );
            break;
    }
    return;
}

/*
 * Parse a name for acceptability.
 */
bool check_parse_name( const char *name )
{
    int                     clan = 0;

    /*
     * Reserved words.
     */
    if ( is_exact_name
         ( name, "all auto immortal self someone something the you loner none guest" ) )
    {
        return false;
    }

    /*
     * check clans 
     */
    for ( clan = 0; clan < MAX_CLAN; clan++ )
    {
        if ( LOWER( name[0] ) == LOWER( clan_table[clan].name[0] )
             && !str_cmp( name, clan_table[clan].name ) )
            return false;
    }

    if ( str_cmp( capitalize( name ), "Alander" ) && ( !str_prefix( "Alan", name )
                                                       || !str_suffix( "Alander",
                                                                       name ) ) )
        return false;

    /*
     * Length restrictions.
     */

    if ( strlen( name ) < 2 )
        return false;

    if ( strlen( name ) > 12 )
        return false;

    /*
     * Alphanumerics only.
     * Lock out IllIll twits.
     */
    {
        const char             *pc = NULL;
        bool                    fIll = true;
        bool                    adjcaps = false;
        bool                    cleancaps = false;
        size_t                  total_caps = 0;

        for ( pc = name; *pc != '\0'; pc++ )
        {
            if ( !isalpha( *pc ) )
                return false;

            if ( isupper( *pc ) )                      /* ugly anti-caps hack */
            {
                if ( adjcaps )
                    cleancaps = true;
                total_caps++;
                adjcaps = true;
            }
            else
                adjcaps = false;

            if ( LOWER( *pc ) != 'i' && LOWER( *pc ) != 'l' )
                fIll = false;
        }

        if ( fIll )
            return false;

        if ( cleancaps || ( total_caps > ( strlen( name ) ) / 2 && strlen( name ) < 3 ) )
            return false;
    }

    /*
     * Prevent players from naming themselves after mobs.
     */
    {
        MOB_INDEX_DATA         *pMobIndex = NULL;
        int                     iHash = 0;

        for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
        {
            for ( pMobIndex = mob_index_hash[iHash];
                  pMobIndex != NULL; pMobIndex = pMobIndex->next )
            {
                if ( is_name( name, pMobIndex->player_name ) )
                    return false;
            }
        }
    }

    return true;
}

/*
 * Look for link-dead player to reconnect.
 */
bool check_reconnect( DESCRIPTOR_DATA *d, const char *name, bool fConn )
{
    CHAR_DATA              *ch = NULL;

    for ( ch = char_list; ch != NULL; ch = ch->next )
    {
        if ( !IS_NPC( ch )
             && ( !fConn || ch->desc == NULL )
             && !str_cmp( d->character->name, ch->name ) )
        {
            if ( fConn == false )
            {
                free_string( d->character->pcdata->pwd );
                d->character->pcdata->pwd = str_dup( ch->pcdata->pwd );
            }
            else
            {
                free_char( d->character );
                d->character = ch;
                ch->desc = d;
                ch->timer = 0;
                ch_printf( ch, "Reconnecting. Type replay to see missed tells.\r\n" );
                act( "$n has reconnected.", ch, NULL, NULL, TO_ROOM );

                log_auth( ch, "%s@%s has reconnected.", NAME( ch ), d->host );
                wiznet( "$N groks the fullness of $S link.", ch, NULL, WIZ_LINKS, 0, 0 );
                d->connected = CON_PLAYING;
            }
            return true;
        }
    }

    return false;
}

/*
 * Check if already playing.
 */
bool check_playing( DESCRIPTOR_DATA *d, const char *name )
{
    DESCRIPTOR_DATA        *dold = NULL;

    for ( dold = descriptor_list; dold; dold = dold->next )
    {
        if ( dold != d
             && dold->character != NULL
             && dold->connected != CON_GET_NAME
             && dold->connected != CON_GET_OLD_PASSWORD
             && !str_cmp( name, dold->original
                          ? dold->original->name : dold->character->name ) )
        {
            desc_printf( d, "That character is already playing.\r\n" );
            desc_printf( d, "Do you wish to connect anyway (Y/N)?" );
            d->connected = CON_BREAK_CONNECT;
            return true;
        }
    }

    return false;
}

void stop_idling( CHAR_DATA *ch )
{
    if ( ch == NULL
         || ch->desc == NULL
         || ch->desc->connected != CON_PLAYING
         || ch->was_in_room == NULL || ch->in_room != get_room_index( ROOM_VNUM_LIMBO ) )
        return;

    ch->timer = 0;
    char_from_room( ch );
    char_to_room( ch, ch->was_in_room );
    ch->was_in_room = NULL;
    act( "$n has returned from the void.", ch, NULL, NULL, TO_ROOM );
    return;
}

/*
 * This function logs a last error message (if Str is not NULL)
 * and then calls exit with the code passed in.
 */
void proper_exit( int code, const char *Str, ... )
{
    va_list                 arg;
    char                    Temp[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    if ( Str && *Str )
    {
        va_start( arg, Str );
        vsnprintf( Temp, MAX_STRING_LENGTH, Str, arg );
        va_end( arg );
        log_fatal( "%s", Temp );
    }
    exit( code );
}

/*
 * Signal handler for game shutdown.
 * Might want to call do_shutdown() or a subset of it.
 */
void shutdown_request( int a )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    DESCRIPTOR_DATA        *d = NULL;
    DESCRIPTOR_DATA        *d_next = NULL;
    CHAR_DATA              *vch = NULL;

    sprintf( buf, "Shutdown by %s.", "SYSTEM -- Received SIGINT or SIGTERM" );
    append_file( NULL, SHUTDOWN_FILE, buf );
    strcat( buf, "\r\n" );
    merc_down = true;
    for ( d = descriptor_list; d != NULL; d = d_next )
    {
        d_next = d->next;
        write_to_socket( d->socket, buf, 0 );
        vch = d->original ? d->original : d->character;
        if ( vch != NULL )
            save_char_obj( vch );
        close_descriptor( d );
    }
    dump_player_list(  );
    proper_exit( MUD_HALT, "Shutdown by %s.", "SYSTEM -- Received SIGINT or SIGTERM" );
}

/* string pager */
void show_string( struct descriptor_data *d, const char *input )
{
    char                    buffer[4 * MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    buf[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                   *scan = NULL;
    char                   *chk = NULL;
    int                     lines = 0;
    bool                    newline = false;
    int                     show_lines = 0;

    one_argument( input, buf );
    if ( buf[0] != '\0' )
    {
        if ( d->showstr_head )
        {
            free_mem( d->showstr_head, strlen( d->showstr_head ) + 1 );
            d->showstr_head = 0;
        }
        d->showstr_point = 0;
        return;
    }

    if ( d->character )
        show_lines = d->character->lines;
    else
        show_lines = 0;

    for ( scan = buffer;; scan++, d->showstr_point++ )
    {
        if ( ( *scan = *d->showstr_point ) == '\r' || *scan == '\n' )
        {
            if ( newline == false )
            {
                newline = true;
                lines++;
            }
            else
            {
                newline = false;
            }
        }
        else if ( !*scan || ( show_lines > 0 && lines >= show_lines ) )
        {
            *scan = '\0';
            write_to_buffer( d, buffer, strlen( buffer ) );
            for ( chk = d->showstr_point; isspace( *chk ); chk++ );
            {
                if ( !*chk )
                {
                    if ( d->showstr_head )
                    {
                        free_mem( d->showstr_head, strlen( d->showstr_head ) + 1 );
                        d->showstr_head = 0;
                    }
                    d->showstr_point = 0;
                }
            }
            return;
        }
        else
        {
            newline = false;
        }
    }
    return;
}

/*
 * Bust a prompt (player settable prompt)
 * coded by Morgenes for Aldara Mud
 */
void bust_a_prompt( CHAR_DATA *ch )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    buf2[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    const char             *str = NULL;
    const char             *i = NULL;
    char                   *point = NULL;
    char                    doors[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    EXIT_DATA              *pexit = NULL;
    bool                    found = false;
    int                     door = -1;

    point = buf;
    str = ch->prompt;
    if ( str == NULL || str[0] == '\0' )
    {
        ch_printf( ch, "<%dhp %dm %dmv> %s", ch->hit, ch->mana, ch->move, ch->prefix );
        return;
    }

    if ( IS_SET( ch->comm, COMM_AFK ) )
    {
        ch_printf( ch, "<AFK> " );
        return;
    }

    while ( *str != '\0' )
    {
        if ( *str != '%' )
        {
            *point++ = *str++;
            continue;
        }
        ++str;
        switch ( *str )
        {
            default:
                i = " ";
                break;
            case 'e':
                found = false;
                doors[0] = '\0';
                for ( door = 0; door < MAX_EXIT; door++ )
                {
                    if ( ( pexit = ch->in_room->exit[door] ) != NULL
                         && pexit->u1.to_room != NULL
                         && ( can_see_room( ch, pexit->u1.to_room )
                              || ( IS_AFFECTED( ch, AFF_INFRARED )
                                   && !IS_AFFECTED( ch, AFF_BLIND ) ) )
                         && !IS_SET( pexit->exit_info, EX_CLOSED ) )
                    {
                        found = true;
                        strcat( doors, dir_abbrev[door] );
                    }
                }
                if ( !found )
                    strcat( buf, "none" );
                sprintf( buf2, "%s", doors );
                i = buf2;
                break;
            case 'c':
                sprintf( buf2, "%s", "\r\n" );
                i = buf2;
                break;
            case 'h':
                sprintf( buf2, "%d", ch->hit );
                i = buf2;
                break;
            case 'H':
                sprintf( buf2, "%d", ch->max_hit );
                i = buf2;
                break;
            case 'm':
                sprintf( buf2, "%d", ch->mana );
                i = buf2;
                break;
            case 'M':
                sprintf( buf2, "%d", ch->max_mana );
                i = buf2;
                break;
            case 'v':
                sprintf( buf2, "%d", ch->move );
                i = buf2;
                break;
            case 'V':
                sprintf( buf2, "%d", ch->max_move );
                i = buf2;
                break;
            case 'x':
                sprintf( buf2, "%d", ch->exp );
                i = buf2;
                break;
            case 'X':
                sprintf( buf2, "%d", IS_NPC( ch ) ? 0 :
                         ( ch->level + 1 ) * exp_per_level( ch,
                                                            ch->pcdata->points ) -
                         ch->exp );
                i = buf2;
                break;
            case 'g':
                sprintf( buf2, "%d", ch->gold );
                i = buf2;
                break;
            case 's':
                sprintf( buf2, "%d", ch->silver );
                i = buf2;
                break;
            case 'a':
                if ( ch->level > 9 )
                    sprintf( buf2, "%d", ch->alignment );
                else
                    sprintf( buf2, "%s", IS_GOOD( ch ) ? "good" : IS_EVIL( ch ) ?
                             "evil" : "neutral" );
                i = buf2;
                break;
            case 'r':
                if ( ch->in_room != NULL )
                    sprintf( buf2, "%s",
                             ( ( !IS_NPC( ch ) && IS_SET( ch->act, PLR_HOLYLIGHT ) ) ||
                               ( !IS_AFFECTED( ch, AFF_BLIND )
                                 && !room_is_dark( ch->in_room ) ) ) ? ch->
                             in_room->name : "darkness" );
                else
                    sprintf( buf2, " " );
                i = buf2;
                break;
            case 'R':
                if ( IS_IMMORTAL( ch ) && ch->in_room != NULL )
                    sprintf( buf2, "%d", ch->in_room->vnum );
                else
                    sprintf( buf2, " " );
                i = buf2;
                break;
            case 'z':
                if ( IS_IMMORTAL( ch ) && ch->in_room != NULL )
                    sprintf( buf2, "%s", ch->in_room->area->name );
                else
                    sprintf( buf2, " " );
                i = buf2;
                break;
            case '%':
                sprintf( buf2, "%%" );
                i = buf2;
                break;
            case 'o':
                sprintf( buf2, "%s", olc_ed_name( ch ) );
                i = buf2;
                break;
            case 'O':
                sprintf( buf2, "%s", olc_ed_vnum( ch ) );
                i = buf2;
                break;
        }
        ++str;
        while ( ( *point = *i ) != '\0' )
            ++point, ++i;
    }
    write_to_buffer( ch->desc, buf, point - buf );

    if ( ch->prefix[0] != '\0' )
        write_to_buffer( ch->desc, ch->prefix, 0 );
    return;
}

/*
 * Lowest level output function.
 * Write a block of text to the file descriptor.
 * If this gives errors on very long blocks (like 'ofind all'),
 *   try lowering the max block size.
 */
bool write_to_socket( int sock, const char *txt, int length )
{
    int                     iStart = 0;
    int                     nWrite = 0;
    size_t                  nBlock = 0;

    if ( length <= 0 )
        length = strlen( txt );

    for ( iStart = 0; iStart < length; iStart += nWrite )
    {
        nBlock = UMIN( length - iStart, 4096 );
        if ( ( nWrite =
               write( ( int ) sock, ( const void * ) ( txt + iStart ), nBlock ) ) < 0 )
        {
            char                   *e = strerror( errno );

            log_error( "socket write size mismatch: %s", e );
            return false;
        }
    }

    return true;
}

/*
 * Append onto an output buffer.
 */
void write_to_buffer( DESCRIPTOR_DATA *d, const char *txt, int length )
{
    /*
     * Find length in case caller didn't.
     */
    if ( length <= 0 )
        length = strlen( txt );

    /*
     * Initial \r\n if needed.
     */
    if ( d->outtop == 0 && !d->fcommand )
    {
        d->outbuf[0] = '\r';
        d->outbuf[1] = '\n';
        d->outtop = 2;
    }

    /*
     * Expand the buffer as needed.
     */
    while ( d->outtop + length >= d->outsize )
    {
        char                   *outbuf = NULL;

        if ( d->outsize >= 32000 )
        {
            log_error( "%s", "Buffer overflow, closing socket" );
            close_descriptor( d );
            return;
        }
        outbuf = ( char * ) alloc_mem( 2 * d->outsize );
        strncpy( outbuf, d->outbuf, d->outtop );
        free_mem( d->outbuf, d->outsize );
        d->outbuf = outbuf;
        d->outsize *= 2;
    }

    /*
     * Copy.
     */
    strncpy( d->outbuf + d->outtop, txt, length );
    d->outtop += length;
    return;
}

void desc_printf( DESCRIPTOR_DATA *d, const char *format, ... )
{
    va_list                 arg;
    char                    Str[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    if ( !format || !*format || !d )
        return;

    va_start( arg, format );
    vsnprintf( Str, MAX_STRING_LENGTH, format, arg );
    va_end( arg );

    write_to_buffer( d, Str, strlen( Str ) );
    return;
}

/*
 * Write to one char.
 */
void send_to_char( const char *format, CHAR_DATA *ch )
{
    if ( !format || !*format || !ch || !ch->desc )
        return;

    write_to_buffer( ch->desc, format, strlen( format ) );
    return;
}

void ch_printf( CHAR_DATA *ch, const char *format, ... )
{
    va_list                 arg;
    char                    Str[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    if ( !format || !*format || !ch || !ch->desc )
        return;

    va_start( arg, format );
    vsnprintf( Str, MAX_STRING_LENGTH, format, arg );
    va_end( arg );

    write_to_buffer( ch->desc, Str, strlen( Str ) );
    return;
}

/*
 * Send a page to one char.
 */
void page_to_char( const char *format, CHAR_DATA *ch )
{
    if ( !format || !*format || !ch || !ch->desc )
        return;

    page_printf( ch, "%s", format );
    return;
}

void page_printf( CHAR_DATA *ch, const char *format, ... )
{
    va_list                 arg;
    char                    Str[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    if ( !format || !*format || !ch || !ch->desc )
        return;

    va_start( arg, format );
    vsnprintf( Str, MAX_STRING_LENGTH, format, arg );
    va_end( arg );

    if ( ch->lines == 0 )
    {
        write_to_buffer( ch->desc, Str, strlen( Str ) );
        return;
    }

    if ( ch->desc->showstr_head &&
         ( strlen( Str ) + strlen( ch->desc->showstr_head ) + 1 ) < 32000 )
    {
        char                   *tmp = NULL;

        tmp =
            ( char * ) alloc_mem( strlen( Str ) + strlen( ch->desc->showstr_head ) + 1 );
        strcpy( tmp, ch->desc->showstr_head );
        strcat( tmp, Str );
        ch->desc->showstr_point = tmp +
            ( ch->desc->showstr_point - ch->desc->showstr_head );
        free_mem( ch->desc->showstr_head, strlen( ch->desc->showstr_head ) + 1 );
        ch->desc->showstr_head = tmp;
    }
    else
    {
        if ( ch->desc->showstr_head )
            free_mem( ch->desc->showstr_head, strlen( ch->desc->showstr_head ) + 1 );
        ch->desc->showstr_head = ( char * ) alloc_mem( strlen( Str ) + 1 );
        strcpy( ch->desc->showstr_head, Str );
        ch->desc->showstr_point = ch->desc->showstr_head;
        show_string( ch->desc, "" );
    }
    return;
}

void act_new( const char *format, CHAR_DATA *ch, const void *arg1,
              const void *arg2, int type, int min_pos )
{
    if ( format == NULL || format[0] == '\0' )
        return;

    act_printf( "%s", ch, arg1, arg2, type, min_pos, false, format );
    return;
}

void act_printf( const char *format, CHAR_DATA *ch, const void *arg1,
                 const void *arg2, int type, int min_pos, bool hide_invis, ... )
{
    va_list                 arg;
    char                    Str[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    fname[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                   *strp = NULL;
    char                   *point = NULL;
    const char             *i = NULL;
    CHAR_DATA              *to = NULL;
    const CHAR_DATA        *vch = ( const CHAR_DATA * ) arg2;
    const OBJ_DATA         *obj1 = ( const OBJ_DATA * ) arg1;
    const OBJ_DATA         *obj2 = ( const OBJ_DATA * ) arg2;

    /*
     * Discard null and zero-length messages.
     */
    if ( format == NULL || format[0] == '\0' )
        return;

    /*
     * discard null rooms and chars 
     */
    if ( ch == NULL || ch->in_room == NULL )
        return;

    /*
     * Figure out which "to" we want...
     */
    to = ch->in_room->people;
    if ( type == TO_VICT )
    {
        if ( vch == NULL || vch->in_room == NULL )
        {
            log_error( "TO_VICT type, but %s is NULL", vch ? "vch->in_room" : "vch" );
            return;
        }
        to = vch->in_room->people;
    }

    /*
     * If nobody is in the room (at all), just stop here
     */
    if ( to == NULL )
        return;

    /*
     * Ok, someone can hear us... parse the varargs!
     */
    va_start( arg, hide_invis );
    vsnprintf( Str, MAX_STRING_LENGTH, format, arg );
    va_end( arg );

    /*
     * This loop goes through everyone in the target room
     */
    for ( ; to != NULL; to = to->next_in_room )
    {
        /*
         * If anyone can't here us, skip them 
         */
        if ( !IS_NPC( to ) && to->desc == NULL )
            continue;
        if ( IS_NPC( to ) && !HAS_TRIGGER( to, TRIG_ACT ) )
            continue;
        if ( to->position < min_pos )
            continue;
        if ( !IS_AWAKE( to ) )
            continue;
        if ( !can_see( to, ch ) && hide_invis )
            continue;
        if ( type == TO_CHAR && to != ch )
            continue;
        if ( type == TO_VICT && ( to != vch || to == ch ) )
            continue;
        if ( type == TO_ROOM && to == ch )
            continue;
        if ( type == TO_NOTVICT && ( to == ch || to == vch ) )
            continue;

        /*
         * Now parse the act-tokens for the current recipient 
         */
        for ( strp = Str, point = buf;; )
        {
            if ( *strp == '$' )
            {
                switch ( *( ++strp ) )
                {
                    default:
                        log_error( "Illegal $-code to act(): $%c in %s", *strp, Str );
                        i = "$?";
                        break;
                    case '$':
                        i = "$";
                        break;
                    case 't':
                        if ( arg1 == NULL )
                        {
                            log_error( "NULL arg1 to act(): $%c in %s", *strp, Str );
                            i = "$?";
                        }
                        else
                            i = ( const char * ) arg1;
                        break;
                    case 'T':
                        if ( arg2 == NULL )
                        {
                            log_error( "NULL arg2 to act(): $%c in %s", *strp, Str );
                            i = "$?";
                        }
                        else
                            i = ( const char * ) arg2;
                        break;
                    case 'n':
                        i = PERS( ch, to );
                        break;
                    case 'N':
                        i = PERS( vch, to );
                        break;
                    case 'e':
                        i = HESHE( ch );
                        break;
                    case 'E':
                        i = HESHE( vch );
                        break;
                    case 'm':
                        i = HIMHER( ch );
                        break;
                    case 'M':
                        i = HIMHER( vch );
                        break;
                    case 's':
                        i = HISHER( ch );
                        break;
                    case 'S':
                        i = HISHER( vch );
                        break;

                    case 'p':
                        i = OPERS( obj1, to );
                        break;
                    case 'P':
                        i = OPERS( obj2, to );
                        break;

                    case 'a':
                        i = ANA( obj1->name );
                        break;
                    case 'A':
                        i = ANA( obj2->name );
                        break;

                    case 'd':
                        if ( arg2 == NULL || ( ( const char * ) arg2 )[0] == '\0' )
                        {
                            i = "door";
                        }
                        else
                        {
                            one_argument( ( const char * ) arg2, fname );
                            i = fname;
                        }
                        break;
                }
                while ( ( *point = *( i++ ) ) )
                    ++point;
                ++strp;
            }
            else if ( !( *( point++ ) = *( strp++ ) ) )
                break;
        }

        *( --point ) = '\r';
        *( ++point ) = '\n';
        *( ++point ) = '\0';

        buf[0] = UPPER( buf[0] );
        if ( to->desc != NULL )
            write_to_buffer( to->desc, buf, point - buf );
        else if ( MOBtrigger )
            mp_act_trigger( buf, to, ch, arg1, arg2, TRIG_ACT );

        if ( ( type == TO_VICT ) || ( type == TO_CHAR ) )
            return;
    }
    return;
}

void wiznet( const char *format, CHAR_DATA *ch, OBJ_DATA *obj,
             int flag, int flag_skip, int min_level )
{
    if ( format == NULL || format[0] == '\0' )
        return;

    wiz_printf( ch, obj, flag, flag_skip, min_level, "%s", format );
    return;
}

void wiz_printf( CHAR_DATA *ch, OBJ_DATA *obj, int flag, int flag_skip,
                 int min_level, const char *format, ... )
{
    va_list                 arg;
    char                    Str[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    DESCRIPTOR_DATA        *d = NULL;

    if ( format == NULL || format[0] == '\0' )
        return;

    va_start( arg, format );
    vsnprintf( Str, MAX_STRING_LENGTH, format, arg );
    va_end( arg );

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ( d->connected == CON_PLAYING
             && IS_IMMORTAL( d->character )
             && IS_SET( d->character->wiznet, WIZ_ON )
             && ( !flag || IS_SET( d->character->wiznet, flag ) )
             && ( !flag_skip || !IS_SET( d->character->wiznet, flag_skip ) )
             && get_trust( d->character ) >= min_level && d->character != ch )
        {
            act_printf( "%s%s", d->character, obj, ch, TO_CHAR, POS_DEAD, false,
                        IS_SET( d->character->wiznet, WIZ_PREFIX ) ? "--> " : "", Str );
        }
    }
    return;
}
