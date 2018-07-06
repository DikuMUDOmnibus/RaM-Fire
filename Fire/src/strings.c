/*
 * RAM $Id: strings.c 47 2008-11-07 22:50:47Z quixadhal $
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
#include <string.h>
#include <errno.h>

#include "merc.h"
#include "strings.h"

void                   *rgFreeList[MAX_MEM_LIST];
const size_t            rgSizeList[MAX_MEM_LIST] = {
    16, 32, 64, 128, 256, 1024, 2048, 4096, 8192, 16384, 32768 - 64
};

int                     nAllocString = 0;
size_t                  sAllocString = 0;
int                     nAllocPerm = 0;
size_t                  sAllocPerm = 0;

char                   *string_space = NULL;
char                   *top_string = NULL;
char                    str_empty[1];

BUFFER                 *buf_free = NULL;

/* buffer sizes */
const int               buf_size[MAX_BUF_LIST] = {
    16, 32, 64, 128, 256, 1024, 2048, 4096, 8192, 16384
};

void *alloc_mem( size_t sMem )
{
    void                   *pMem = NULL;
    size_t                 *magic = NULL;
    int                     iList = 0;

    sMem += sizeof( *magic );

    for ( iList = 0; iList < MAX_MEM_LIST; iList++ )
    {
        if ( sMem <= rgSizeList[iList] )
            break;
    }

    if ( iList == MAX_MEM_LIST )
    {
        proper_exit( MUD_HALT, "Alloc_mem: size %zd too large.", sMem );
    }

    if ( rgFreeList[iList] == NULL )
    {
        pMem = alloc_perm( rgSizeList[iList] );
    }
    else
    {
        pMem = rgFreeList[iList];
        rgFreeList[iList] = *( ( void ** ) rgFreeList[iList] );
    }

    magic = ( size_t * ) pMem;
    *magic = MAGIC_NUM;
    pMem = ( void * ) ( ( size_t ) pMem + ( size_t ) ( sizeof( *magic ) ) );

    return pMem;
}

/*
 * Free some memory.
 * Recycle it back onto the free list for blocks of that size.
 */
void free_mem( void *pMem, size_t sMem )
{
    int                     iList = 0;
    size_t                 *magic = NULL;

    pMem = ( void * ) ( ( size_t ) pMem - ( size_t ) sizeof( *magic ) );
    magic = ( size_t * ) pMem;

    if ( *magic != MAGIC_NUM )
    {
        log_error( "Attempt to recyle invalid memory of size %zd \"%s\"", sMem,
                   ( ( char * ) pMem + sizeof( *magic ) ) );
        return;
    }

    *magic = 0;
    sMem += sizeof( *magic );

    for ( iList = 0; iList < MAX_MEM_LIST; iList++ )
    {
        if ( sMem <= rgSizeList[iList] )
            break;
    }

    if ( iList == MAX_MEM_LIST )
    {
        proper_exit( MUD_HALT, "Free_mem: size %zd too large.", sMem );
    }

    *( ( void ** ) pMem ) = rgFreeList[iList];
    rgFreeList[iList] = pMem;

    return;
}

/*
 * Allocate some permanent memory.
 * Permanent memory is never freed,
 *   pointers into it may be copied safely.
 */
void *alloc_perm( size_t sMem )
{
    static char            *pMemPerm = NULL;
    static size_t           iMemPerm = 0;
    void                   *pMem = NULL;

    while ( sMem % sizeof( int ) != 0 )
        sMem++;
    if ( sMem > MAX_PERM_BLOCK )
    {
        proper_exit( MUD_HALT, "Alloc_perm: %zd too large.", sMem );
    }

    if ( pMemPerm == NULL || iMemPerm + sMem > MAX_PERM_BLOCK )
    {
        iMemPerm = 0;
        if ( ( pMemPerm = ( char * ) calloc( 1, MAX_PERM_BLOCK ) ) == NULL )
        {
            char                   *e = strerror( errno );

            proper_exit( MUD_HALT, "Alloc_perm: %s\n", e );
        }
    }

    pMem = pMemPerm + iMemPerm;
    iMemPerm += sMem;
    nAllocPerm += 1;
    sAllocPerm += sMem;
    return pMem;
}

/*
 * Duplicate a string into dynamic memory.
 * Fread_strings are read-only and shared.
 */
char *str_dup( const char *str )
{
    char                   *str_new = NULL;

    if ( str[0] == '\0' )
        return &str_empty[0];

    if ( str >= string_space && str < top_string )
    {
        /*
         * return str; 
         */
        return ( char * ) ( size_t ) str;
        /*
         * I can't believe I just did that cast... EVIL! 
         */
    }

    str_new = ( char * ) alloc_mem( strlen( str ) + 1 );
    strcpy( str_new, str );
    return str_new;
}

/*
 * Free a string.
 * Null is legal here to simplify callers.
 * Read-only shared strings are not touched.
 */
void free_string( char *pstr )
{
    if ( pstr == NULL
         || pstr == &str_empty[0] || ( pstr >= string_space && pstr < top_string ) )
        return;

    free_mem( pstr, strlen( pstr ) + 1 );
    return;
}

/*
 * Compare strings, case insensitive.
 * Return true if different
 *   (compatibility with historical functions).
 */
bool str_cmp( const char *astr, const char *bstr )
{
    if ( astr == NULL )
    {
        log_error( "%s", "NULL left side of str_cmp" );
        return true;
    }

    if ( bstr == NULL )
    {
        log_error( "%s", "NULL right side of str_cmp" );
        return true;
    }

    for ( ; *astr || *bstr; astr++, bstr++ )
    {
        if ( LOWER( *astr ) != LOWER( *bstr ) )
            return true;
    }

    return false;
}

/*
 * Compare strings, case insensitive, for prefix matching.
 * Return true if astr not a prefix of bstr
 *   (compatibility with historical functions).
 */
bool str_prefix( const char *astr, const char *bstr )
{
    if ( astr == NULL )
    {
        log_error( "%s", "NULL left side of str_cmp" );
        return true;
    }

    if ( bstr == NULL )
    {
        log_error( "%s", "NULL right side of str_cmp" );
        return true;
    }

    for ( ; *astr; astr++, bstr++ )
    {
        if ( LOWER( *astr ) != LOWER( *bstr ) )
            return true;
    }

    return false;
}

/*
 * Compare strings, case insensitive, for match anywhere.
 * Returns true is astr not part of bstr.
 *   (compatibility with historical functions).
 */
bool str_infix( const char *astr, const char *bstr )
{
    int                     sstr1 = 0;
    int                     sstr2 = 0;
    int                     ichar = 0;
    char                    c0 = '\0';

    if ( ( c0 = LOWER( astr[0] ) ) == '\0' )
        return false;

    sstr1 = strlen( astr );
    sstr2 = strlen( bstr );

    for ( ichar = 0; ichar <= sstr2 - sstr1; ichar++ )
    {
        if ( c0 == LOWER( bstr[ichar] ) && !str_prefix( astr, bstr + ichar ) )
            return false;
    }

    return true;
}

/*
 * Compare strings, case insensitive, for suffix matching.
 * Return true if astr not a suffix of bstr
 *   (compatibility with historical functions).
 */
bool str_suffix( const char *astr, const char *bstr )
{
    int                     sstr1 = 0;
    int                     sstr2 = 0;

    sstr1 = strlen( astr );
    sstr2 = strlen( bstr );
    if ( sstr1 <= sstr2 && !str_cmp( astr, bstr + sstr2 - sstr1 ) )
        return false;
    else
        return true;
}

/*
 * Returns an initial-capped string.
 */
char                   *capitalize( const char *str )
{
    static char             strcap[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    int                     i = 0;

    for ( i = 0; str[i] != '\0'; i++ )
        strcap[i] = LOWER( str[i] );
    strcap[i] = '\0';
    strcap[0] = UPPER( strcap[0] );
    return strcap;
}

/*
 * Removes the tildes from a string.
 * Used for player-entered strings that go into disk files.
 */
void smash_tilde( char *str )
{
    for ( ; *str != '\0'; str++ )
    {
        if ( *str == '~' )
            *str = '-';
    }

    return;
}

/* procedures and constants needed for buffering */

/* local procedure for finding the next acceptable size */
/* -1 indicates out-of-boundary error */
int get_size( int val )
{
    int                     i = 0;

    for ( i = 0; i < MAX_BUF_LIST; i++ )
        if ( buf_size[i] >= val )
        {
            return buf_size[i];
        }

    return -1;
}

BUFFER                 *new_buf( void )
{
    BUFFER                 *buffer = NULL;

    if ( buf_free == NULL )
        buffer = ( BUFFER * ) alloc_perm( sizeof( *buffer ) );
    else
    {
        buffer = buf_free;
        buf_free = buf_free->next;
    }

    buffer->next = NULL;
    buffer->state = BUFFER_SAFE;
    buffer->size = get_size( BASE_BUF );

    buffer->string = ( char * ) alloc_mem( buffer->size );
    buffer->string[0] = '\0';
    VALIDATE( buffer );

    return buffer;
}

BUFFER                 *new_buf_size( int size )
{
    BUFFER                 *buffer = NULL;

    if ( buf_free == NULL )
        buffer = ( BUFFER * ) alloc_perm( sizeof( *buffer ) );
    else
    {
        buffer = buf_free;
        buf_free = buf_free->next;
    }

    buffer->next = NULL;
    buffer->state = BUFFER_SAFE;
    buffer->size = get_size( size );
    if ( buffer->size == -1 )
    {
        proper_exit( MUD_HALT, "new_buf: buffer size %d too large.", size );
    }
    buffer->string = ( char * ) alloc_mem( buffer->size );
    buffer->string[0] = '\0';
    VALIDATE( buffer );

    return buffer;
}

void free_buf( BUFFER *buffer )
{
    if ( !IS_VALID( buffer ) )
        return;

    free_mem( buffer->string, buffer->size );
    buffer->string = NULL;
    buffer->size = 0;
    buffer->state = BUFFER_FREED;
    INVALIDATE( buffer );

    buffer->next = buf_free;
    buf_free = buffer;
}

bool add_buf( BUFFER *buffer, const char *string )
{
    int                     len = 0;
    char                   *oldstr = NULL;
    int                     oldsize = 0;

    oldstr = buffer->string;
    oldsize = buffer->size;

    if ( buffer->state == BUFFER_OVERFLOW )            /* don't waste time on bad
                                                        * strings! */
        return false;

    len = strlen( buffer->string ) + strlen( string ) + 1;

    while ( len >= buffer->size )                      /* increase the buffer size */
    {
        buffer->size = get_size( buffer->size + 1 );
        {
            if ( buffer->size == -1 )                  /* overflow */
            {
                buffer->size = oldsize;
                buffer->state = BUFFER_OVERFLOW;
                log_error( "Buffer overflow past size %d", buffer->size );
                return false;
            }
        }
    }

    if ( buffer->size != oldsize )
    {
        buffer->string = ( char * ) alloc_mem( buffer->size );

        strcpy( buffer->string, oldstr );
        free_mem( oldstr, oldsize );
    }

    strcat( buffer->string, string );
    return true;
}

void clear_buf( BUFFER *buffer )
{
    buffer->string[0] = '\0';
    buffer->state = BUFFER_SAFE;
}

char                   *buf_string( BUFFER *buffer )
{
    return buffer->string;
}

