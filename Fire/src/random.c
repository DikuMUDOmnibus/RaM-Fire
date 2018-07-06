/*
 * RAM $Id: random.c 43 2008-11-07 13:01:52Z quixadhal $
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
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "merc.h"
#include "random.h"

/*
 * I've gotten too many bad reports on OS-supplied random number generators.
 * This is the Mitchell-Moore algorithm from Knuth Volume II.
 * Best to leave the constants alone unless you've read Knuth.
 * -- Furey
 */

/* I noticed streaking with this random number generator, so I switched
   back to the system srandom call.  If this doesn't work for you, 
   define OLD_RAND to use the old system -- Alander */

#if defined (OLD_RAND)
static int              rgiState[2 + 55];

void init_random( void )
{
    int                    *piState = NULL;
    int                     iState = 0;

    piState = &rgiState[2];

    piState[-2] = 55 - 55;
    piState[-1] = 55 - 24;

    piState[0] = ( ( int ) current_time ) & ( ( 1 << 30 ) - 1 );
    piState[1] = 1;
    for ( iState = 2; iState < 55; iState++ )
    {
        piState[iState] = ( piState[iState - 1] + piState[iState - 2] )
            & ( ( 1 << 30 ) - 1 );
    }
    return;
}

int number_random( void )
{
    int                    *piState = NULL;
    int                     iState1 = 0;
    int                     iState2 = 0;
    int                     iRand = 0;

    piState = &rgiState[2];
    iState1 = piState[-2];
    iState2 = piState[-1];
    iRand = ( piState[iState1] + piState[iState2] ) & ( ( 1 << 30 ) - 1 );
    piState[iState1] = iRand;
    if ( ++iState1 == 55 )
        iState1 = 0;
    if ( ++iState2 == 55 )
        iState2 = 0;
    piState[-2] = iState1;
    piState[-1] = iState2;
    return iRand >> 6;
}

#else

void init_random( void )
{
    srandom( time( NULL ) ^ getpid(  ) );
}

int number_random( void )
{
    return random(  ) >> 6;
}

#endif

/*
 * Generate a random number.
 */
int number_range( int from, int to )
{
    int                     power = 0;
    int                     number = 0;

    if ( from == 0 && to == 0 )
        return 0;

    if ( ( to = to - from + 1 ) <= 1 )
        return from;

    for ( power = 2; power < to; power <<= 1 )
        ;

    while ( ( number = number_random(  ) & ( power - 1 ) ) >= to )
        ;

    return from + number;
}

/*
 * Return a random number within a bit limit?
 */
int number_bits( int width )
{
    return number_random(  ) & ( ( 1 << width ) - 1 );
}

/*
 * Stick a little fuzz on a number.
 */
int number_fuzzy( int number )
{
    switch ( number_bits( 2 ) )
    {
        case 0:
            number -= 1;
            break;
        case 3:
            number += 1;
            break;
    }

    return UMAX( 1, number );
}

/*
 * Generate a percentile roll.
 */
int number_percent( void )
{
    return number_range( 1, 100 );
}

/*
 * Generate a random door.
 */
int number_door( void )
{
    return number_range( 1, MAX_EXIT ) - 1;
}

/*
 * Roll some dice.
 */
int dice( int number, int size )
{
    int                     idice = 0;
    int                     sum = 0;

    switch ( size )
    {
        case 0:
            return 0;
        case 1:
            return number;
    }

    for ( idice = 0, sum = 0; idice < number; idice++ )
        sum += number_range( 1, size );

    return sum;
}

