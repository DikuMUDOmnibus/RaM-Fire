/*
 * RAM $Id: strings.h 47 2008-11-07 22:50:47Z quixadhal $
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

/*
 * Memory management.
 * Increase MAX_STRING if you have too.
 * Tune the others only if you understand what you're doing.
 */
#define                        MAX_STRING        1413120
#define                        MAX_PERM_BLOCK    131072
#define                        MAX_MEM_LIST      11

/* Magic number for memory allocation */
#define                        MAGIC_NUM         52571214

/* stuff for providing a crash-proof buffer */
#define MAX_BUF         16384
#define MAX_BUF_LIST       10
#define BASE_BUF         1024

/* valid states */
#define BUFFER_SAFE         0
#define BUFFER_OVERFLOW     1
#define BUFFER_FREED        2

extern int              nAllocString;
extern size_t           sAllocString;
extern int              nAllocPerm;
extern size_t           sAllocPerm;

extern char            *string_space;
extern char            *top_string;
extern char             str_empty[1];

void                   *alloc_mem( size_t sMem );
void                    free_mem( void *pMem, size_t sMem );
void                   *alloc_perm( size_t sMem );
char                   *str_dup( const char *str );
void                    free_string( char *pstr );

bool                    str_cmp( const char *astr, const char *bstr );
bool                    str_prefix( const char *astr, const char *bstr );
bool                    str_infix( const char *astr, const char *bstr );
bool                    str_suffix( const char *astr, const char *bstr );
char                   *capitalize( const char *str );
void                    smash_tilde( char *str );

/* buffer procedures */
extern const int        buf_size[MAX_BUF_LIST];
int                     get_size( int val );
BUFFER                 *new_buf( void );
BUFFER                 *new_buf_size( int size );
void                    free_buf( BUFFER *buffer );
bool                    add_buf( BUFFER *buffer, const char *string );
void                    clear_buf( BUFFER *buffer );
char                   *buf_string( BUFFER *buffer );
