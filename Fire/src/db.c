/*
 * RAM $Id: db.c 70 2009-01-11 18:47:35Z quixadhal $
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

/***************************************************************************
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>

#include <string>

#include "merc.h"
#include "strings.h"
#include "random.h"
#include "tables.h"
#include "act.h"
#include "db.h"
#include "interp.h"
#include "magic.h"
#include "special.h"
#include "olc.h"
#include "ban.h"

/*
 * Globals.
 */
HELP_DATA              *help_first = NULL;
HELP_DATA              *help_last = NULL;
HELP_AREA              *had_list = NULL;

SHOP_DATA              *shop_first = NULL;
SHOP_DATA              *shop_last = NULL;

AREA_DATA              *area_first = NULL;
AREA_DATA              *area_last = NULL;
AREA_DATA              *current_area = NULL;

MPROG_CODE             *mprog_list = NULL;
MPROG_CODE             *mpcode_free = NULL;

char                    bug_buf[2 * MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
char                    log_buf[2 * MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";

CHAR_DATA              *char_list = NULL;
char                   *help_greeting = NULL;
KILL_DATA               kill_table[MAX_LEVEL];
OBJ_DATA               *object_list = NULL;
TIME_INFO_DATA          time_info;
WEATHER_DATA            weather_info;
bool                    MOBtrigger = true;             /* act() switch */

/*
 * Locals.
 */
MOB_INDEX_DATA         *mob_index_hash[MAX_KEY_HASH];
OBJ_INDEX_DATA         *obj_index_hash[MAX_KEY_HASH];
ROOM_INDEX_DATA        *room_index_hash[MAX_KEY_HASH];
char                   *string_hash[MAX_KEY_HASH];

int                     top_affect = 0;
int                     top_area = 0;
int                     top_ed = 0;
int                     top_exit = 0;
int                     top_help = 0;
int                     top_mob_index = 0;
int                     top_obj_index = 0;
int                     top_reset = 0;
int                     top_room = 0;
int                     top_shop = 0;
int                     top_vnum_room = 0;             /* OLC */
int                     top_vnum_mob = 0;              /* OLC */
int                     top_vnum_obj = 0;              /* OLC */
int                     top_mprog_index = 0;           /* OLC */
int                     mobile_count = 0;
int                     newmobs = 0;
int                     newobjs = 0;

/*
 * Semi-locals.
 */
bool                    fBootDb = false;
FILE                   *fpArea = NULL;
char                    strArea[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
struct social_type      social_table[MAX_SOCIALS];
int                     social_count = 0;

/* stuff for recycling extended descs */
EXTRA_DESCR_DATA       *extra_descr_free = NULL;

EXTRA_DESCR_DATA       *new_extra_descr( void )
{
    EXTRA_DESCR_DATA       *ed = NULL;

    if ( extra_descr_free == NULL )
        ed = ( EXTRA_DESCR_DATA * ) alloc_perm( sizeof( *ed ) );
    else
    {
        ed = extra_descr_free;
        extra_descr_free = extra_descr_free->next;
    }

    ed->keyword = &str_empty[0];
    ed->description = &str_empty[0];
    VALIDATE( ed );
    return ed;
}

void free_extra_descr( EXTRA_DESCR_DATA *ed )
{
    if ( !IS_VALID( ed ) )
        return;

    free_string( ed->keyword );
    free_string( ed->description );
    INVALIDATE( ed );

    ed->next = extra_descr_free;
    extra_descr_free = ed;
}

/* stuff for recycling objects */
OBJ_DATA               *obj_free = NULL;

OBJ_DATA               *new_obj( void )
{
    static OBJ_DATA         obj_zero;
    OBJ_DATA               *obj = NULL;

    if ( obj_free == NULL )
        obj = ( OBJ_DATA * ) alloc_perm( sizeof( *obj ) );
    else
    {
        obj = obj_free;
        obj_free = obj_free->next;
    }
    *obj = obj_zero;
    VALIDATE( obj );

    return obj;
}

void free_obj( OBJ_DATA *obj )
{
    AFFECT_DATA            *paf = NULL;
    AFFECT_DATA            *paf_next = NULL;
    EXTRA_DESCR_DATA       *ed = NULL;
    EXTRA_DESCR_DATA       *ed_next = NULL;

    if ( !IS_VALID( obj ) )
        return;

    for ( paf = obj->affected; paf != NULL; paf = paf_next )
    {
        paf_next = paf->next;
        free_affect( paf );
    }
    obj->affected = NULL;

    for ( ed = obj->extra_descr; ed != NULL; ed = ed_next )
    {
        ed_next = ed->next;
        free_extra_descr( ed );
    }
    obj->extra_descr = NULL;

    free_string( obj->name );
    free_string( obj->description );
    free_string( obj->short_descr );
    free_string( obj->owner );
    INVALIDATE( obj );

    obj->next = obj_free;
    obj_free = obj;
}

/* stuff for recyling characters */
CHAR_DATA              *char_free = NULL;

CHAR_DATA              *new_char( void )
{
    static CHAR_DATA        ch_zero;
    CHAR_DATA              *ch = NULL;
    int                     i = 0;

    if ( char_free == NULL )
        ch = ( CHAR_DATA * ) alloc_perm( sizeof( *ch ) );
    else
    {
        ch = char_free;
        char_free = char_free->next;
    }

    *ch = ch_zero;
    VALIDATE( ch );
    ch->name = &str_empty[0];
    ch->short_descr = &str_empty[0];
    ch->long_descr = &str_empty[0];
    ch->description = &str_empty[0];
    ch->prompt = &str_empty[0];
    ch->prefix = &str_empty[0];
    ch->logon = current_time;
    ch->lines = PAGELEN;
    for ( i = 0; i < 4; i++ )
        ch->armor[i] = 100;
    ch->position = POS_STANDING;
    ch->hit = 20;
    ch->max_hit = 20;
    ch->mana = 100;
    ch->max_mana = 100;
    ch->move = 100;
    ch->max_move = 100;
    for ( i = 0; i < MAX_STATS; i++ )
    {
        ch->perm_stat[i] = 13;
        ch->mod_stat[i] = 0;
    }

    return ch;
}

void free_char( CHAR_DATA *ch )
{
    OBJ_DATA               *obj = NULL;
    OBJ_DATA               *obj_next = NULL;
    AFFECT_DATA            *paf = NULL;
    AFFECT_DATA            *paf_next = NULL;

    if ( !IS_VALID( ch ) )
        return;

    if ( IS_NPC( ch ) )
        mobile_count--;

    for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    {
        obj_next = obj->next_content;
        extract_obj( obj );
    }

    for ( paf = ch->affected; paf != NULL; paf = paf_next )
    {
        paf_next = paf->next;
        affect_remove( ch, paf );
    }

    free_string( ch->name );
    free_string( ch->short_descr );
    free_string( ch->long_descr );
    free_string( ch->description );
    free_string( ch->prompt );
    free_string( ch->prefix );
    free_note( ch->pnote );
    free_pcdata( ch->pcdata );

    ch->next = char_free;
    char_free = ch;

    INVALIDATE( ch );
    return;
}

PC_DATA                *pcdata_free = NULL;

PC_DATA                *new_pcdata( void )
{
    int                     alias = 0;
    static PC_DATA          pcdata_zero;
    PC_DATA                *pcdata = NULL;

    if ( pcdata_free == NULL )
        pcdata = ( PC_DATA * ) alloc_perm( sizeof( *pcdata ) );
    else
    {
        pcdata = pcdata_free;
        pcdata_free = pcdata_free->next;
    }

    *pcdata = pcdata_zero;

    for ( alias = 0; alias < MAX_ALIAS; alias++ )
    {
        pcdata->alias[alias] = NULL;
        pcdata->alias_sub[alias] = NULL;
    }

    pcdata->buffer = new_buf(  );

    VALIDATE( pcdata );
    return pcdata;
}

void free_pcdata( PC_DATA *pcdata )
{
    int                     alias = 0;

    if ( !IS_VALID( pcdata ) )
        return;

    free_string( pcdata->pwd );
    free_string( pcdata->bamfin );
    free_string( pcdata->bamfout );
    free_string( pcdata->title );
    free_buf( pcdata->buffer );

    for ( alias = 0; alias < MAX_ALIAS; alias++ )
    {
        free_string( pcdata->alias[alias] );
        free_string( pcdata->alias_sub[alias] );
    }
    INVALIDATE( pcdata );
    pcdata->next = pcdata_free;
    pcdata_free = pcdata;

    return;
}

/* stuff for setting ids */
int                     last_pc_id = 0;
int                     last_mob_id = 0;

int get_pc_id( void )
{
    int                     val = 0;

    val = ( current_time <= last_pc_id ) ? last_pc_id + 1 : current_time;
    last_pc_id = val;
    return val;
}

int get_mob_id( void )
{
    last_mob_id++;
    return last_mob_id;
}

MEM_DATA               *mem_data_free = NULL;

MEM_DATA               *new_mem_data( void )
{
    MEM_DATA               *memory = NULL;

    if ( mem_data_free == NULL )
        memory = ( MEM_DATA * ) alloc_mem( sizeof( *memory ) );
    else
    {
        memory = mem_data_free;
        mem_data_free = mem_data_free->next;
    }

    memory->next = NULL;
    memory->id = 0;
    memory->reaction = 0;
    memory->when = 0;
    VALIDATE( memory );

    return memory;
}

void free_mem_data( MEM_DATA *memory )
{
    if ( !IS_VALID( memory ) )
        return;

    memory->next = mem_data_free;
    mem_data_free = memory;
    INVALIDATE( memory );
}

/* This was added for OLC */

RESET_DATA             *reset_free = NULL;

RESET_DATA             *new_reset_data( void )
{
    RESET_DATA             *pReset = NULL;

    if ( !reset_free )
    {
        pReset = ( RESET_DATA * ) alloc_perm( sizeof( *pReset ) );
        top_reset++;
    }
    else
    {
        pReset = reset_free;
        reset_free = reset_free->next;
    }

    pReset->next = NULL;
    pReset->command = 'X';
    pReset->arg1 = 0;
    pReset->arg2 = 0;
    pReset->arg3 = 0;
    pReset->arg4 = 0;

    return pReset;
}

void free_reset_data( RESET_DATA *pReset )
{
    pReset->next = reset_free;
    reset_free = pReset;
    return;
}

AREA_DATA              *area_free = NULL;

AREA_DATA              *new_area( void )
{
    AREA_DATA              *pArea = NULL;
    char                    buf[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";

    if ( !area_free )
    {
        pArea = ( AREA_DATA * ) alloc_perm( sizeof( *pArea ) );
        top_area++;
    }
    else
    {
        pArea = area_free;
        area_free = area_free->next;
    }

    pArea->next = NULL;
    pArea->name = str_dup( "New area" );
/*    pArea->recall           =   ROOM_VNUM_TEMPLE;      ROM OLC */
    pArea->area_flags = AREA_ADDED;
    pArea->security = 1;
    pArea->builders = str_dup( "None" );
    pArea->min_vnum = 0;
    pArea->max_vnum = 0;
    pArea->age = 0;
    pArea->nplayer = 0;
    pArea->empty = true;                               /* ROM patch */
    sprintf( buf, "area%d.are", pArea->vnum );
    pArea->file_name = str_dup( buf );
    pArea->vnum = top_area - 1;

    return pArea;
}

void free_area( AREA_DATA *pArea )
{
    free_string( pArea->name );
    free_string( pArea->file_name );
    free_string( pArea->builders );
    free_string( pArea->credits );

    pArea->next = area_free->next;
    area_free = pArea;
    return;
}

EXIT_DATA              *exit_free = NULL;

EXIT_DATA              *new_exit( void )
{
    EXIT_DATA              *pExit = NULL;

    if ( !exit_free )
    {
        pExit = ( EXIT_DATA * ) alloc_perm( sizeof( *pExit ) );
        top_exit++;
    }
    else
    {
        pExit = exit_free;
        exit_free = exit_free->next;
    }

    pExit->u1.to_room = NULL;                          /* ROM OLC */
    pExit->next = NULL;
/*  pExit->vnum         =   0;                        ROM OLC */
    pExit->exit_info = 0;
    pExit->key = 0;
    pExit->keyword = &str_empty[0];
    pExit->description = &str_empty[0];
    pExit->rs_flags = 0;

    return pExit;
}

void free_exit( EXIT_DATA *pExit )
{
    free_string( pExit->keyword );
    free_string( pExit->description );

    pExit->next = exit_free;
    exit_free = pExit;
    return;
}

ROOM_INDEX_DATA        *room_index_free = NULL;

ROOM_INDEX_DATA        *new_room_index( void )
{
    ROOM_INDEX_DATA        *pRoom = NULL;
    int                     door = -1;

    if ( !room_index_free )
    {
        pRoom = ( ROOM_INDEX_DATA * ) alloc_perm( sizeof( *pRoom ) );
        top_room++;
    }
    else
    {
        pRoom = room_index_free;
        room_index_free = room_index_free->next;
    }

    pRoom->next = NULL;
    pRoom->people = NULL;
    pRoom->contents = NULL;
    pRoom->extra_descr = NULL;
    pRoom->area = NULL;

    for ( door = 0; door < MAX_DIR; door++ )
        pRoom->exit[door] = NULL;

    pRoom->name = &str_empty[0];
    pRoom->description = &str_empty[0];
    pRoom->owner = &str_empty[0];
    pRoom->vnum = 0;
    pRoom->room_flags = 0;
    pRoom->light = 0;
    pRoom->sector_type = 0;
    pRoom->clan = 0;
    pRoom->heal_rate = 100;
    pRoom->mana_rate = 100;

    return pRoom;
}

void free_room_index( ROOM_INDEX_DATA *pRoom )
{
    int                     door = -1;
    EXTRA_DESCR_DATA       *pExtra = NULL;
    RESET_DATA             *pReset = NULL;

    free_string( pRoom->name );
    free_string( pRoom->description );
    free_string( pRoom->owner );

    for ( door = 0; door < MAX_DIR; door++ )
    {
        if ( pRoom->exit[door] )
            free_exit( pRoom->exit[door] );
    }

    for ( pExtra = pRoom->extra_descr; pExtra; pExtra = pExtra->next )
    {
        free_extra_descr( pExtra );
    }

    for ( pReset = pRoom->reset_first; pReset; pReset = pReset->next )
    {
        free_reset_data( pReset );
    }

    pRoom->next = room_index_free;
    room_index_free = pRoom;
    return;
}

SHOP_DATA              *shop_free = NULL;

SHOP_DATA              *new_shop( void )
{
    SHOP_DATA              *pShop = NULL;
    int                     buy = 0;

    if ( !shop_free )
    {
        pShop = ( SHOP_DATA * ) alloc_perm( sizeof( *pShop ) );
        top_shop++;
    }
    else
    {
        pShop = shop_free;
        shop_free = shop_free->next;
    }

    pShop->next = NULL;
    pShop->keeper = 0;

    for ( buy = 0; buy < MAX_TRADE; buy++ )
        pShop->buy_type[buy] = 0;

    pShop->profit_buy = 100;
    pShop->profit_sell = 100;
    pShop->open_hour = 0;
    pShop->close_hour = 23;

    return pShop;
}

void free_shop( SHOP_DATA *pShop )
{
    pShop->next = shop_free;
    shop_free = pShop;
    return;
}

OBJ_INDEX_DATA         *obj_index_free = NULL;

OBJ_INDEX_DATA         *new_obj_index( void )
{
    OBJ_INDEX_DATA         *pObj = NULL;
    int                     value = 0;

    if ( !obj_index_free )
    {
        pObj = ( OBJ_INDEX_DATA * ) alloc_perm( sizeof( *pObj ) );
        top_obj_index++;
    }
    else
    {
        pObj = obj_index_free;
        obj_index_free = obj_index_free->next;
    }

    pObj->next = NULL;
    pObj->extra_descr = NULL;
    pObj->affected = NULL;
    pObj->area = NULL;
    pObj->name = str_dup( "no name" );
    pObj->short_descr = str_dup( "(no short description)" );
    pObj->description = str_dup( "(no description)" );
    pObj->vnum = 0;
    pObj->item_type = ITEM_TRASH;
    pObj->extra_flags = 0;
    pObj->wear_flags = 0;
    pObj->count = 0;
    pObj->weight = 0;
    pObj->cost = 0;
    pObj->material = str_dup( "unknown" );             /* ROM */
    pObj->condition = 100;                             /* ROM */
    for ( value = 0; value < 5; value++ )              /* 5 - ROM */
        pObj->value[value] = 0;

    pObj->new_format = true;                           /* ROM */

    return pObj;
}

void free_obj_index( OBJ_INDEX_DATA *pObj )
{
    EXTRA_DESCR_DATA       *pExtra = NULL;
    AFFECT_DATA            *pAf = NULL;

    free_string( pObj->name );
    free_string( pObj->short_descr );
    free_string( pObj->description );

    for ( pAf = pObj->affected; pAf; pAf = pAf->next )
    {
        free_affect( pAf );
    }

    for ( pExtra = pObj->extra_descr; pExtra; pExtra = pExtra->next )
    {
        free_extra_descr( pExtra );
    }

    pObj->next = obj_index_free;
    obj_index_free = pObj;
    return;
}

MOB_INDEX_DATA         *mob_index_free = NULL;

MOB_INDEX_DATA         *new_mob_index( void )
{
    MOB_INDEX_DATA         *pMob = NULL;

    if ( !mob_index_free )
    {
        pMob = ( MOB_INDEX_DATA * ) alloc_perm( sizeof( *pMob ) );
        top_mob_index++;
    }
    else
    {
        pMob = mob_index_free;
        mob_index_free = mob_index_free->next;
    }

    pMob->next = NULL;
    pMob->spec_fun = NULL;
    pMob->pShop = NULL;
    pMob->area = NULL;
    pMob->player_name = str_dup( "no name" );
    pMob->short_descr = str_dup( "(no short description)" );
    pMob->long_descr = str_dup( "(no long description)\r\n" );
    pMob->description = &str_empty[0];
    pMob->vnum = 0;
    pMob->count = 0;
    pMob->killed = 0;
    pMob->sex = 0;
    pMob->level = 0;
    pMob->act = ACT_IS_NPC;
    pMob->affected_by = 0;
    pMob->alignment = 0;
    pMob->hitroll = 0;
    pMob->race = race_lookup( "human" );               /* - Hugin */
    pMob->form = 0;                                    /* ROM patch -- Hugin */
    pMob->parts = 0;                                   /* ROM patch -- Hugin */
    pMob->imm_flags = 0;                               /* ROM patch -- Hugin */
    pMob->res_flags = 0;                               /* ROM patch -- Hugin */
    pMob->vuln_flags = 0;                              /* ROM patch -- Hugin */
    pMob->material = str_dup( "unknown" );             /* -- Hugin */
    pMob->off_flags = 0;                               /* ROM patch -- Hugin */
    pMob->size = SIZE_MEDIUM;                          /* ROM patch -- Hugin */
    pMob->ac[AC_PIERCE] = 0;                           /* ROM patch -- Hugin */
    pMob->ac[AC_BASH] = 0;                             /* ROM patch -- Hugin */
    pMob->ac[AC_SLASH] = 0;                            /* ROM patch -- Hugin */
    pMob->ac[AC_EXOTIC] = 0;                           /* ROM patch -- Hugin */
    pMob->hit[DICE_NUMBER] = 0;                        /* ROM patch -- Hugin */
    pMob->hit[DICE_TYPE] = 0;                          /* ROM patch -- Hugin */
    pMob->hit[DICE_BONUS] = 0;                         /* ROM patch -- Hugin */
    pMob->mana[DICE_NUMBER] = 0;                       /* ROM patch -- Hugin */
    pMob->mana[DICE_TYPE] = 0;                         /* ROM patch -- Hugin */
    pMob->mana[DICE_BONUS] = 0;                        /* ROM patch -- Hugin */
    pMob->damage[DICE_NUMBER] = 0;                     /* ROM patch -- Hugin */
    pMob->damage[DICE_TYPE] = 0;                       /* ROM patch -- Hugin */
    pMob->damage[DICE_NUMBER] = 0;                     /* ROM patch -- Hugin */
    pMob->start_pos = POS_STANDING;                    /* -- Hugin */
    pMob->default_pos = POS_STANDING;                  /* -- Hugin */
    pMob->wealth = 0;

    pMob->new_format = true;                           /* ROM */

    return pMob;
}

void free_mob_index( MOB_INDEX_DATA *pMob )
{
    free_string( pMob->player_name );
    free_string( pMob->short_descr );
    free_string( pMob->long_descr );
    free_string( pMob->description );
    free_mprog( pMob->mprogs );

    free_shop( pMob->pShop );

    pMob->next = mob_index_free;
    mob_index_free = pMob;
    return;
}

MPROG_CODE             *new_mpcode( void )
{
    MPROG_CODE             *NewCode = NULL;

    if ( !mpcode_free )
    {
        NewCode = ( MPROG_CODE * ) alloc_perm( sizeof( *NewCode ) );
        top_mprog_index++;
    }
    else
    {
        NewCode = mpcode_free;
        mpcode_free = mpcode_free->next;
    }

    NewCode->vnum = 0;
    NewCode->code = str_dup( "" );
    NewCode->next = NULL;

    return NewCode;
}

void free_mpcode( MPROG_CODE *pMcode )
{
    free_string( pMcode->code );
    pMcode->next = mpcode_free;
    mpcode_free = pMcode;
    return;
}

/*
 * Big mama top level function.
 */
void boot_db( void )
{
    /*
     * Init some data space stuff.
     */
    {
        log_boot( "Allocating %d bytes of static string space", MAX_STRING );
        if ( ( string_space = ( char * ) calloc( 1, MAX_STRING ) ) == NULL )
        {
            proper_exit( MUD_HALT, "Boot_db: can't alloc %d string space.", MAX_STRING );
        }
        top_string = string_space;
        fBootDb = true;
    }

    /*
     * Init random number generator.
     */
    {
        log_boot( "Seeding random number generator" );
        init_random(  );
    }

    /*
     * Set time and weather.
     */
    {
        int                     lhour = 0;
        int                     lday = 0;
        int                     lmonth = 0;

        log_boot( "Updating game time and weather" );

        lhour = ( current_time - 650336715 ) / ( PULSE_TICK / PULSE_PER_SECOND );
        time_info.hour = lhour % 24;
        lday = lhour / 24;
        time_info.day = lday % 35;
        lmonth = lday / 35;
        time_info.month = lmonth % 17;
        time_info.year = lmonth / 17;

        if ( time_info.hour < 5 )
            weather_info.sunlight = SUN_DARK;
        else if ( time_info.hour < 6 )
            weather_info.sunlight = SUN_RISE;
        else if ( time_info.hour < 19 )
            weather_info.sunlight = SUN_LIGHT;
        else if ( time_info.hour < 20 )
            weather_info.sunlight = SUN_SET;
        else
            weather_info.sunlight = SUN_DARK;

        weather_info.change = 0;
        weather_info.mmhg = 960;
        if ( time_info.month >= 7 && time_info.month <= 12 )
            weather_info.mmhg += number_range( 1, 50 );
        else
            weather_info.mmhg += number_range( 1, 80 );

        if ( weather_info.mmhg <= 980 )
            weather_info.sky = SKY_LIGHTNING;
        else if ( weather_info.mmhg <= 1000 )
            weather_info.sky = SKY_RAINING;
        else if ( weather_info.mmhg <= 1020 )
            weather_info.sky = SKY_CLOUDY;
        else
            weather_info.sky = SKY_CLOUDLESS;

    }

    sort_skill_table(  );

    /*
     * Read in all the area files.
     */
    {
        FILE                   *fpList = NULL;

        log_boot( "Loading areas..." );
        if ( ( fpList = fopen( AREA_LIST, "r" ) ) == NULL )
        {
            char                   *e = strerror( errno );

            proper_exit( MUD_HALT, "%s: %s\n", AREA_LIST, e );
        }

        for ( ;; )
        {
            strcpy( strArea, fread_word( fpList ) );
            if ( strArea[0] == '$' )
                break;

            log_boot( "Loading %s", strArea );
            if ( strArea[0] == '-' )
            {
                fpArea = stdin;
            }
            else
            {
                if ( ( fpArea = fopen( strArea, "r" ) ) == NULL )
                {
                    char                   *e = strerror( errno );

                    proper_exit( MUD_HALT, "%s: %s\n", strArea, e );
                }
            }

            load_area_file( fpArea );

            if ( fpArea != stdin )
                fclose( fpArea );
            fpArea = NULL;
        }
        fclose( fpList );
    }

    /*
     * Fix up exits.
     * Declare db booting over.
     * Reset all areas once.
     * Load up the notes and ban files.
     */
    {
        log_boot( "Fixing exits" );
        fix_exits(  );
        fix_mobprogs(  );
        fBootDb = false;
        convert_objects(  );                           /* ROM OLC */
        area_update(  );
        load_notes(  );
        load_bans(  );
        load_player_list(  );
    }
    return;
}

/*
 * Figures out which area format is in use.
 */
void load_area_file( FILE * fp )
{
    char                   *word = NULL;
    int                     version = 0;

    if ( fp == stdin )
    {
        /*
         * We can't fseek here, so assume it's OLD format!
         */
        load_rom_area_file( fp );
        return;
    }

    if ( fread_letter( fpArea ) != '#' )
    {
        proper_exit( MUD_HALT, "Boot_db: # not found." );
    }

    word = fread_word( fpArea );

    if ( str_cmp( word, "VERSION" ) )
    {
        /*
         * If the first line isn't #VERSION, this is an old format file.
         */
        rewind( fp );
        load_rom_area_file( fp );
        return;
    }

    version = fread_number( fp );
    load_ram_area_file( fp, version );
}

/*
 * Loads an entire area file, RaM format.
 */
void load_ram_area_file( FILE * fp, int version )
{
    load_rom_area_file( fp );
}

/*
 * Loads an entire area file, ROM format.
 */
void load_rom_area_file( FILE * fp )
{
    current_area = NULL;

    for ( ;; )
    {
        char                   *word = NULL;

        if ( fread_letter( fp ) != '#' )
        {
            proper_exit( MUD_HALT, "Boot_db: # not found." );
        }

        word = fread_word( fp );

        if ( word[0] == '$' )
            break;
        else if ( !str_cmp( word, "AREA" ) )
            load_area( fp );
        else if ( !str_cmp( word, "AREADATA" ) )
            new_load_area( fp );
        else if ( !str_cmp( word, "HELPS" ) )
            load_helps( fp, strArea );
        else if ( !str_cmp( word, "MOBOLD" ) )
            load_old_mob( fp );
        else if ( !str_cmp( word, "MOBILES" ) )
            load_mobiles( fp );
        else if ( !str_cmp( word, "MOBPROGS" ) )
            load_mobprogs( fp );
        else if ( !str_cmp( word, "OBJOLD" ) )
            load_old_obj( fp );
        else if ( !str_cmp( word, "OBJECTS" ) )
            load_objects( fp );
        else if ( !str_cmp( word, "RESETS" ) )
            load_resets( fp );
        else if ( !str_cmp( word, "ROOMS" ) )
            load_rooms( fp );
        else if ( !str_cmp( word, "SHOPS" ) )
            load_shops( fp );
        else if ( !str_cmp( word, "SOCIALS" ) )
            load_socials( fp );
        else if ( !str_cmp( word, "SPECIALS" ) )
            load_specials( fp );
        else
        {
            proper_exit( MUD_HALT, "Boot_db: bad section name." );
        }
    }
}

/*
 * Snarf an 'area' header line.
 */
void load_area( FILE * fp )
{
    AREA_DATA              *pArea = NULL;

    pArea = ( AREA_DATA * ) alloc_perm( sizeof( *pArea ) );
    pArea->reset_first = NULL;
    pArea->reset_last = NULL;
    pArea->file_name = fread_string( fp );
    pArea->area_flags = AREA_LOADING;                  /* OLC */
    pArea->security = 9;                               /* OLC - 9 -- Hugin */
    pArea->builders = str_dup( "None" );               /* OLC */
    pArea->vnum = top_area;                            /* OLC */
    pArea->name = fread_string( fp );
    pArea->credits = fread_string( fp );
    pArea->min_vnum = fread_number( fp );
    pArea->max_vnum = fread_number( fp );
    pArea->age = 15;
    pArea->nplayer = 0;
    pArea->empty = false;

    if ( area_first == NULL )
        area_first = pArea;
    if ( area_last != NULL )
    {
        area_last->next = pArea;
        REMOVE_BIT( area_last->area_flags, AREA_LOADING );      /* OLC */
    }
    area_last = pArea;
    pArea->next = NULL;
    current_area = pArea;

    top_area++;
    return;
}

/* OLC
 * Snarf an 'area' header line.   Check this format.  MUCH better.  Add fields
 * too.
 *
 * #AREAFILE
 * Name   { All } Locke    Newbie School~
 * Repop  A teacher pops in the room and says, 'Repop coming!'~
 * Recall 3001
 * End
 */
void new_load_area( FILE * fp )
{
    AREA_DATA              *pArea = NULL;
    const char             *word = NULL;
    bool                    fMatch = false;

    pArea = ( AREA_DATA * ) alloc_perm( sizeof( *pArea ) );
    pArea->age = 15;
    pArea->nplayer = 0;
    pArea->file_name = str_dup( strArea );
    pArea->vnum = top_area;
    pArea->name = str_dup( "New Area" );
    pArea->builders = str_dup( "" );
    pArea->security = 9;                               /* 9 -- Hugin */
    pArea->min_vnum = 0;
    pArea->max_vnum = 0;
    pArea->area_flags = 0;
/*  pArea->recall       = ROOM_VNUM_TEMPLE;        ROM OLC */

    for ( ;; )
    {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = false;

        switch ( UPPER( word[0] ) )
        {
            case 'N':
                KEYS( "Name", pArea->name, fread_string( fp ) );
                break;
            case 'S':
                KEY( "Security", pArea->security, fread_number( fp ) );
                break;
            case 'V':
                if ( !str_cmp( word, "VNUMs" ) )
                {
                    pArea->min_vnum = fread_number( fp );
                    pArea->max_vnum = fread_number( fp );
                }
                break;
            case 'E':
                if ( !str_cmp( word, "End" ) )
                {
                    fMatch = true;
                    if ( area_first == NULL )
                        area_first = pArea;
                    if ( area_last != NULL )
                        area_last->next = pArea;
                    area_last = pArea;
                    pArea->next = NULL;
                    current_area = pArea;
                    top_area++;

                    return;
                }
                break;
            case 'B':
                KEYS( "Builders", pArea->builders, fread_string( fp ) );
                break;
            case 'C':
                KEYS( "Credits", pArea->credits, fread_string( fp ) );
                break;
        }
    }
}

/*
 * Sets vnum range for area using OLC protection features.
 */
void assign_area_vnum( int vnum )
{
    if ( area_last->min_vnum == 0 || area_last->max_vnum == 0 )
        area_last->min_vnum = area_last->max_vnum = vnum;
    if ( vnum != URANGE( area_last->min_vnum, vnum, area_last->max_vnum ) )
    {
        if ( vnum < area_last->min_vnum )
            area_last->min_vnum = vnum;
        else
            area_last->max_vnum = vnum;
    }
    return;
}

#if 0
/*
 * Snarf an old help section.
 */
void old_load_helps( FILE * fp )
{
    HELP_DATA              *pHelp = NULL;

    for ( ;; )
    {
        pHelp = ( HELP_DATA * ) alloc_perm( sizeof( *pHelp ) );
        pHelp->level = fread_number( fp );
        pHelp->keyword = fread_string( fp );
        if ( pHelp->keyword[0] == '$' )
            break;
        pHelp->text = fread_string( fp );

        if ( !str_cmp( pHelp->keyword, "greeting" ) )
            help_greeting = pHelp->text;

        if ( help_first == NULL )
            help_first = pHelp;
        if ( help_last != NULL )
            help_last->next = pHelp;

        help_last = pHelp;
        pHelp->next = NULL;
        top_help++;
    }

    return;
}
#endif

/*
 * Snarf a help section.
 */
void load_helps( FILE * fp, const char *fname )
{
    HELP_DATA              *pHelp = NULL;
    char                   *keyword = NULL;
    int                     level = 0;

    for ( ;; )
    {
        HELP_AREA              *had = NULL;

        level = fread_number( fp );
        keyword = fread_string( fp );

        if ( keyword[0] == '$' )
            break;

        if ( !had_list )
        {
            had = new_had(  );
            had->filename = str_dup( fname );
            had->area = current_area;
            if ( current_area )
                current_area->helps = had;
            had_list = had;
        }
        else if ( str_cmp( fname, had_list->filename ) )
        {
            had = new_had(  );
            had->filename = str_dup( fname );
            had->area = current_area;
            if ( current_area )
                current_area->helps = had;
            had->next = had_list;
            had_list = had;
        }
        else
            had = had_list;

        pHelp = new_help(  );
        pHelp->level = level;
        pHelp->keyword = keyword;
        pHelp->text = fread_string( fp );

        if ( !str_cmp( pHelp->keyword, "greeting" ) )
            help_greeting = pHelp->text;

        if ( help_first == NULL )
            help_first = pHelp;
        if ( help_last != NULL )
            help_last->next = pHelp;

        help_last = pHelp;
        pHelp->next = NULL;

        if ( !had->first )
            had->first = pHelp;
        if ( !had->last )
            had->last = pHelp;

        had->last->next_area = pHelp;
        had->last = pHelp;
        pHelp->next_area = NULL;

        top_help++;
    }

    return;
}

/*
 * Snarf a mob section.  old style 
 */
void load_old_mob( FILE * fp )
{
    MOB_INDEX_DATA         *pMobIndex = NULL;

    /*
     * for race updating 
     */
    int                     race = 0;
    char                    name[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    if ( !area_last )                                  /* OLC */
    {
        proper_exit( MUD_HALT, "OLD Load_mobiles: no #AREA seen yet." );
    }

    for ( ;; )
    {
        int                     vnum = 0;
        char                    letter = '\0';
        int                     iHash = 0;

        letter = fread_letter( fp );
        if ( letter != '#' )
        {
            proper_exit( MUD_HALT, "Load_mobiles: # not found." );
        }

        vnum = fread_number( fp );
        if ( vnum == 0 )
            break;

        fBootDb = false;
        if ( get_mob_index( vnum ) != NULL )
        {
            proper_exit( MUD_HALT, "Load_mobiles: vnum %d duplicated.", vnum );
        }
        fBootDb = true;

        pMobIndex = ( MOB_INDEX_DATA * ) alloc_perm( sizeof( *pMobIndex ) );
        pMobIndex->vnum = vnum;
        pMobIndex->area = area_last;                   /* OLC */
        pMobIndex->new_format = false;
        pMobIndex->player_name = fread_string( fp );
        pMobIndex->short_descr = fread_string( fp );
        pMobIndex->long_descr = fread_string( fp );
        pMobIndex->description = fread_string( fp );

        pMobIndex->long_descr[0] = UPPER( pMobIndex->long_descr[0] );
        pMobIndex->description[0] = UPPER( pMobIndex->description[0] );

        pMobIndex->act = fread_flag( fp ) | ACT_IS_NPC;
        pMobIndex->affected_by = fread_flag( fp );
        pMobIndex->pShop = NULL;
        pMobIndex->alignment = fread_number( fp );
        letter = fread_letter( fp );
        pMobIndex->level = fread_number( fp );

        /*
         * The unused stuff is for imps who want to use the old-style
         * stats-in-files method.
         */
        fread_number( fp );                            /* Unused */
        fread_number( fp );                            /* Unused */
        fread_number( fp );                            /* Unused */
        fread_letter( fp );                            /* 'd' */
        fread_number( fp );                            /* Unused */
        fread_letter( fp );                            /* '+' */
        fread_number( fp );                            /* Unused */
        fread_number( fp );                            /* Unused */
        fread_letter( fp );                            /* 'd' */
        fread_number( fp );                            /* Unused */
        fread_letter( fp );                            /* '+' */
        fread_number( fp );                            /* Unused */

        pMobIndex->wealth = fread_number( fp ) / 20;

        fread_number( fp );                            /* xp can't be used! */

        pMobIndex->start_pos = fread_number( fp );     /* Unused */
        pMobIndex->default_pos = fread_number( fp );   /* Unused */

        if ( pMobIndex->start_pos < POS_SLEEPING )
            pMobIndex->start_pos = POS_STANDING;
        if ( pMobIndex->default_pos < POS_SLEEPING )
            pMobIndex->default_pos = POS_STANDING;

        /*
         * Back to meaningful values.
         */
        pMobIndex->sex = fread_number( fp );

        /*
         * compute the race BS 
         */
        one_argument( pMobIndex->player_name, name );

        if ( name[0] == '\0' || ( race = race_lookup( name ) ) == 0 )
        {
            /*
             * fill in with blanks 
             */
            pMobIndex->race = race_lookup( "human" );
            pMobIndex->off_flags = OFF_DODGE | OFF_DISARM | OFF_TRIP | ASSIST_VNUM;
            pMobIndex->imm_flags = 0;
            pMobIndex->res_flags = 0;
            pMobIndex->vuln_flags = 0;
            pMobIndex->form = FORM_EDIBLE | FORM_SENTIENT | FORM_BIPED | FORM_MAMMAL;
            pMobIndex->parts = PART_HEAD | PART_ARMS | PART_LEGS | PART_HEART |
                PART_BRAINS | PART_GUTS;
        }
        else
        {
            pMobIndex->race = race;
            pMobIndex->off_flags = OFF_DODGE | OFF_DISARM | OFF_TRIP | ASSIST_RACE |
                race_table[race].off;
            pMobIndex->imm_flags = race_table[race].imm;
            pMobIndex->res_flags = race_table[race].res;
            pMobIndex->vuln_flags = race_table[race].vuln;
            pMobIndex->form = race_table[race].form;
            pMobIndex->parts = race_table[race].parts;
        }

        if ( letter != 'S' )
        {
            proper_exit( MUD_HALT, "Load_mobiles: vnum %d non-S.", vnum );
        }

        convert_mobile( pMobIndex );                   /* ROM OLC */

        iHash = vnum % MAX_KEY_HASH;
        pMobIndex->next = mob_index_hash[iHash];
        mob_index_hash[iHash] = pMobIndex;
        top_mob_index++;
        top_vnum_mob = top_vnum_mob < vnum ? vnum : top_vnum_mob;       /* OLC */
        assign_area_vnum( vnum );                      /* OLC */
        kill_table[URANGE( 0, pMobIndex->level, MAX_LEVEL - 1 )].number++;
    }
    return;
}

/*
 * Snarf an obj section.  old style 
 */
void load_old_obj( FILE * fp )
{
    OBJ_INDEX_DATA         *pObjIndex = NULL;

    if ( !area_last )                                  /* OLC */
    {
        proper_exit( MUD_HALT, "OLD Load_objects: no #AREA seen yet." );
    }

    for ( ;; )
    {
        int                     vnum = 0;
        char                    letter = '\0';
        int                     iHash = 0;

        letter = fread_letter( fp );
        if ( letter != '#' )
        {
            proper_exit( MUD_HALT, "Load_objects: # not found." );
        }

        vnum = fread_number( fp );
        if ( vnum == 0 )
            break;

        fBootDb = false;
        if ( get_obj_index( vnum ) != NULL )
        {
            proper_exit( MUD_HALT, "Load_objects: vnum %d duplicated.", vnum );
        }
        fBootDb = true;

        pObjIndex = ( OBJ_INDEX_DATA * ) alloc_perm( sizeof( *pObjIndex ) );
        pObjIndex->vnum = vnum;
        pObjIndex->area = area_last;                   /* OLC */
        pObjIndex->new_format = false;
        pObjIndex->reset_num = 0;
        pObjIndex->name = fread_string( fp );
        pObjIndex->short_descr = fread_string( fp );
        pObjIndex->description = fread_string( fp );
        /*
         * Action description 
         */ fread_string( fp );

        pObjIndex->short_descr[0] = LOWER( pObjIndex->short_descr[0] );
        pObjIndex->description[0] = UPPER( pObjIndex->description[0] );
        pObjIndex->material = str_dup( "" );

        pObjIndex->item_type = fread_number( fp );
        pObjIndex->extra_flags = fread_flag( fp );
        pObjIndex->wear_flags = fread_flag( fp );
        pObjIndex->value[0] = fread_number( fp );
        pObjIndex->value[1] = fread_number( fp );
        pObjIndex->value[2] = fread_number( fp );
        pObjIndex->value[3] = fread_number( fp );
        pObjIndex->value[4] = 0;
        pObjIndex->level = 0;
        pObjIndex->condition = 100;
        pObjIndex->weight = fread_number( fp );
        pObjIndex->cost = fread_number( fp );          /* Unused */
        /*
         * Cost per day 
         */ fread_number( fp );

        if ( pObjIndex->item_type == ITEM_WEAPON )
        {
            if ( is_name( "two", pObjIndex->name )
                 || is_name( "two-handed", pObjIndex->name )
                 || is_name( "claymore", pObjIndex->name ) )
                SET_BIT( pObjIndex->value[4], WEAPON_TWO_HANDS );
        }

        for ( ;; )
        {
            char                    another_letter = '\0';

            another_letter = fread_letter( fp );

            if ( another_letter == 'A' )
            {
                AFFECT_DATA            *paf;

                paf = ( AFFECT_DATA * ) alloc_perm( sizeof( *paf ) );
                paf->where = TO_OBJECT;
                paf->type = -1;
                paf->level = 20;                       /* RT temp fix */
                paf->duration = -1;
                paf->location = fread_number( fp );
                paf->modifier = fread_number( fp );
                paf->bitvector = 0;
                paf->next = pObjIndex->affected;
                pObjIndex->affected = paf;
                top_affect++;
            }

            else if ( another_letter == 'E' )
            {
                EXTRA_DESCR_DATA       *ed = NULL;

                ed = ( EXTRA_DESCR_DATA * ) alloc_perm( sizeof( *ed ) );
                ed->keyword = fread_string( fp );
                ed->description = fread_string( fp );
                ed->next = pObjIndex->extra_descr;
                pObjIndex->extra_descr = ed;
                top_ed++;
            }

            else
            {
                ungetc( another_letter, fp );
                break;
            }
        }

        /*
         * fix armors 
         */
        if ( pObjIndex->item_type == ITEM_ARMOR )
        {
            pObjIndex->value[1] = pObjIndex->value[0];
            pObjIndex->value[2] = pObjIndex->value[1];
        }

        /*
         * Translate spell "slot numbers" to internal "skill numbers."
         */
        switch ( pObjIndex->item_type )
        {
            case ITEM_PILL:
            case ITEM_POTION:
            case ITEM_SCROLL:
                pObjIndex->value[1] = slot_lookup( pObjIndex->value[1] );
                pObjIndex->value[2] = slot_lookup( pObjIndex->value[2] );
                pObjIndex->value[3] = slot_lookup( pObjIndex->value[3] );
                pObjIndex->value[4] = slot_lookup( pObjIndex->value[4] );
                break;

            case ITEM_STAFF:
            case ITEM_WAND:
                pObjIndex->value[3] = slot_lookup( pObjIndex->value[3] );
                break;
        }

        iHash = vnum % MAX_KEY_HASH;
        pObjIndex->next = obj_index_hash[iHash];
        obj_index_hash[iHash] = pObjIndex;
        top_obj_index++;
        top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;       /* OLC */
        assign_area_vnum( vnum );                      /* OLC */
    }
    return;
}

/*
 * Adds a reset to a room.  OLC
 * Similar to add_reset in olc.c
 */
void new_reset( ROOM_INDEX_DATA *pR, RESET_DATA *pReset )
{
    RESET_DATA             *pr = NULL;

    if ( !pR )
        return;

    pr = pR->reset_last;
    if ( !pr )
    {
        pR->reset_first = pReset;
        pR->reset_last = pReset;
    }
    else
    {
        pR->reset_last->next = pReset;
        pR->reset_last = pReset;
        pR->reset_last->next = NULL;
    }
    return;
}

#if 0
/*
 * Snarf a reset section.
 */
void old_load_resets( FILE * fp )
{
    RESET_DATA             *pReset = NULL;

    if ( area_last == NULL )
    {
        proper_exit( MUD_HALT, "Load_resets: no #AREA seen yet." );
    }

    for ( ;; )
    {
        ROOM_INDEX_DATA        *pRoomIndex = NULL;
        EXIT_DATA              *pexit = NULL;
        char                    letter = '\0';
        OBJ_INDEX_DATA         *temp_index = NULL;

        if ( ( letter = fread_letter( fp ) ) == 'S' )
            break;

        if ( letter == '*' )
        {
            fread_to_eol( fp );
            continue;
        }

        pReset = ( RESET_DATA * ) alloc_perm( sizeof( *pReset ) );
        pReset->command = letter;
        /*
         * if_flag 
         */ fread_number( fp );
        pReset->arg1 = fread_number( fp );
        pReset->arg2 = fread_number( fp );
        pReset->arg3 = ( letter == 'G' || letter == 'R' ) ? 0 : fread_number( fp );
        pReset->arg4 = ( letter == 'P' || letter == 'M' ) ? fread_number( fp ) : 0;
        fread_to_eol( fp );

        /*
         * Validate parameters.
         * We're calling the index functions for the side effect.
         */
        switch ( letter )
        {
            default:
                proper_exit( MUD_HALT, "Load_resets: bad command '%c'.", letter );
                break;

            case 'M':
                get_mob_index( pReset->arg1 );
                get_room_index( pReset->arg3 );
                break;

            case 'O':
                temp_index = get_obj_index( pReset->arg1 );
                temp_index->reset_num++;
                get_room_index( pReset->arg3 );
                break;

            case 'P':
                temp_index = get_obj_index( pReset->arg1 );
                temp_index->reset_num++;
                get_obj_index( pReset->arg3 );
                break;

            case 'G':
            case 'E':
                temp_index = get_obj_index( pReset->arg1 );
                temp_index->reset_num++;
                break;

            case 'D':
                pRoomIndex = get_room_index( pReset->arg1 );

                if ( pReset->arg2 < 0
                     || pReset->arg2 > 5
                     || ( pexit = pRoomIndex->exit[pReset->arg2] ) == NULL
                     || !IS_SET( pexit->exit_info, EX_ISDOOR ) )
                {
                    proper_exit( MUD_HALT, "Load_resets: 'D': exit %d not door.",
                                 pReset->arg2 );
                }

                if ( pReset->arg3 < 0 || pReset->arg3 > 2 )
                {
                    proper_exit( MUD_HALT, "Load_resets: 'D': bad 'locks': %d.",
                                 pReset->arg3 );
                }

                break;

            case 'R':
                pRoomIndex = get_room_index( pReset->arg1 );

                if ( pReset->arg2 < 0 || pReset->arg2 > 6 )
                {
                    proper_exit( MUD_HALT, "Load_resets: 'R': bad exit %d.",
                                 pReset->arg2 );
                }

                break;
        }

        if ( area_last->reset_first == NULL )
            area_last->reset_first = pReset;
        if ( area_last->reset_last != NULL )
            area_last->reset_last->next = pReset;

        area_last->reset_last = pReset;
        pReset->next = NULL;
        top_reset++;
    }

    return;
}
#endif

/*
 * Snarf a reset section.
 */
void load_resets( FILE * fp )
{
    RESET_DATA             *pReset = NULL;
    EXIT_DATA              *pexit = NULL;
    ROOM_INDEX_DATA        *pRoomIndex = NULL;
    int                     rVnum = -1;

    if ( area_last == NULL )
    {
        proper_exit( MUD_HALT, "Load_resets: no #AREA seen yet." );
    }

    for ( ;; )
    {
        char                    letter = '\0';

        if ( ( letter = fread_letter( fp ) ) == 'S' )
            break;

        if ( letter == '*' )
        {
            fread_to_eol( fp );
            continue;
        }

        pReset = new_reset_data(  );
        pReset->command = letter;
        fread_number( fp );                            /* if_flag */
        pReset->arg1 = fread_number( fp );
        pReset->arg2 = fread_number( fp );
        pReset->arg3 = ( letter == 'G' || letter == 'R' ) ? 0 : fread_number( fp );
        pReset->arg4 = ( letter == 'P' || letter == 'M' ) ? fread_number( fp ) : 0;
        fread_to_eol( fp );

        /*
         * Validate parameters.
         * We're calling the index functions for the side effect.
         */
        switch ( pReset->command )
        {
            case 'M':
            case 'O':
                rVnum = pReset->arg3;
                break;
            case 'P':
            case 'G':
            case 'E':
                break;
            case 'D':
                pRoomIndex = get_room_index( ( rVnum = pReset->arg1 ) );
                if ( pReset->arg2 < 0
                     || pReset->arg2 >= MAX_DIR
                     || !pRoomIndex
                     || !( pexit = pRoomIndex->exit[pReset->arg2] )
                     || !IS_SET( pexit->rs_flags, EX_ISDOOR ) )
                {
                    proper_exit( MUD_HALT,
                                 "Load_resets: 'D': exit %d, room %d not door.",
                                 pReset->arg2, pReset->arg1 );
                }
                switch ( pReset->arg3 )
                {
                    default:
                        log_error( "Load_resets: 'D': bad 'locks': %d.", pReset->arg3 );
                        break;
                    case 0:
                        break;
                    case 1:
                        SET_BIT( pexit->rs_flags, EX_CLOSED );
                        SET_BIT( pexit->exit_info, EX_CLOSED );
                        break;
                    case 2:
                        SET_BIT( pexit->rs_flags, EX_CLOSED | EX_LOCKED );
                        SET_BIT( pexit->exit_info, EX_CLOSED | EX_LOCKED );
                        break;
                }
                break;
            case 'R':
                rVnum = pReset->arg1;
                break;
        }

        if ( rVnum == -1 )
            proper_exit( MUD_HALT, "load_resets : rVnum == -1" );

        if ( pReset->command != 'D' )
            new_reset( get_room_index( rVnum ), pReset );
        else
            free_reset_data( pReset );
    }
    return;
}

/*
 * Snarf a room section.
 */
void load_rooms( FILE * fp )
{
    ROOM_INDEX_DATA        *pRoomIndex = NULL;

    if ( area_last == NULL )
    {
        proper_exit( MUD_HALT, "Load_resets: no #AREA seen yet." );
    }

    for ( ;; )
    {
        int                     vnum = 0;
        char                    letter = '\0';
        int                     door = 0;
        int                     iHash = 0;

        letter = fread_letter( fp );
        if ( letter != '#' )
        {
            proper_exit( MUD_HALT, "Load_rooms: # not found." );
        }

        vnum = fread_number( fp );
        if ( vnum == 0 )
            break;

        fBootDb = false;
        if ( get_room_index( vnum ) != NULL )
        {
            proper_exit( MUD_HALT, "Load_rooms: vnum %d duplicated.", vnum );
        }
        fBootDb = true;

        pRoomIndex = ( ROOM_INDEX_DATA * ) alloc_perm( sizeof( *pRoomIndex ) );
        pRoomIndex->owner = str_dup( "" );
        pRoomIndex->people = NULL;
        pRoomIndex->contents = NULL;
        pRoomIndex->extra_descr = NULL;
        pRoomIndex->area = area_last;
        pRoomIndex->vnum = vnum;
        pRoomIndex->name = fread_string( fp );
        pRoomIndex->description = fread_string( fp );
        /*
         * Area number 
         */ fread_number( fp );
        pRoomIndex->room_flags = fread_flag( fp );
        pRoomIndex->sector_type = fread_number( fp );
        pRoomIndex->light = 0;
        for ( door = 0; door < MAX_EXIT; door++ )
            pRoomIndex->exit[door] = NULL;

        /*
         * defaults 
         */
        pRoomIndex->heal_rate = 100;
        pRoomIndex->mana_rate = 100;

        for ( ;; )
        {
            letter = fread_letter( fp );

            if ( letter == 'S' )
                break;

            if ( letter == 'H' )                       /* healing room */
                pRoomIndex->heal_rate = fread_number( fp );

            else if ( letter == 'M' )                  /* mana room */
                pRoomIndex->mana_rate = fread_number( fp );

            else if ( letter == 'C' )                  /* clan */
            {
                if ( pRoomIndex->clan )
                {
                    proper_exit( MUD_HALT, "Load_rooms: duplicate clan fields." );
                }
                pRoomIndex->clan = clan_lookup( fread_string( fp ) );
            }

            else if ( letter == 'D' )
            {
                EXIT_DATA              *pexit;
                int                     locks;

                door = fread_number( fp );
                if ( door < 0 || door >= MAX_EXIT )
                {
                    proper_exit( MUD_HALT, "Fread_rooms: vnum %d has bad door number.",
                                 vnum );
                }

                pexit = ( EXIT_DATA * ) alloc_perm( sizeof( *pexit ) );
                pexit->description = fread_string( fp );
                pexit->keyword = fread_string( fp );
                pexit->exit_info = 0;
                pexit->rs_flags = 0;                   /* OLC */
                locks = fread_number( fp );
                pexit->key = fread_number( fp );
                pexit->u1.vnum = fread_number( fp );
                pexit->orig_door = door;               /* OLC */

                switch ( locks )
                {
                    case 1:
                        pexit->exit_info = EX_ISDOOR;
                        pexit->rs_flags = EX_ISDOOR;
                        break;
                    case 2:
                        pexit->exit_info = EX_ISDOOR | EX_PICKPROOF;
                        pexit->rs_flags = EX_ISDOOR | EX_PICKPROOF;
                        break;
                    case 3:
                        pexit->exit_info = EX_ISDOOR | EX_NOPASS;
                        pexit->rs_flags = EX_ISDOOR | EX_NOPASS;
                        break;
                    case 4:
                        pexit->exit_info = EX_ISDOOR | EX_NOPASS | EX_PICKPROOF;
                        pexit->rs_flags = EX_ISDOOR | EX_NOPASS | EX_PICKPROOF;
                        break;
                }

                pRoomIndex->exit[door] = pexit;
                top_exit++;
            }
            else if ( letter == 'E' )
            {
                EXTRA_DESCR_DATA       *ed = NULL;

                ed = ( EXTRA_DESCR_DATA * ) alloc_perm( sizeof( *ed ) );
                ed->keyword = fread_string( fp );
                ed->description = fread_string( fp );
                ed->next = pRoomIndex->extra_descr;
                pRoomIndex->extra_descr = ed;
                top_ed++;
            }

            else if ( letter == 'O' )
            {
                if ( pRoomIndex->owner[0] != '\0' )
                {
                    proper_exit( MUD_HALT, "Load_rooms: duplicate owner." );
                }

                pRoomIndex->owner = fread_string( fp );
            }

            else
            {
                proper_exit( MUD_HALT, "Load_rooms: vnum %d has flag not 'DES'.", vnum );
            }
        }

        iHash = vnum % MAX_KEY_HASH;
        pRoomIndex->next = room_index_hash[iHash];
        room_index_hash[iHash] = pRoomIndex;
        top_room++;
        top_vnum_room = top_vnum_room < vnum ? vnum : top_vnum_room;    /* OLC */
        assign_area_vnum( vnum );                      /* OLC */
    }
    return;
}

/*
 * Snarf a shop section.
 */
void load_shops( FILE * fp )
{
    SHOP_DATA              *pShop = NULL;

    for ( ;; )
    {
        MOB_INDEX_DATA         *pMobIndex = NULL;
        int                     iTrade = 0;

        pShop = ( SHOP_DATA * ) alloc_perm( sizeof( *pShop ) );
        pShop->keeper = fread_number( fp );
        if ( pShop->keeper == 0 )
            break;
        for ( iTrade = 0; iTrade < MAX_TRADE; iTrade++ )
            pShop->buy_type[iTrade] = fread_number( fp );
        pShop->profit_buy = fread_number( fp );
        pShop->profit_sell = fread_number( fp );
        pShop->open_hour = fread_number( fp );
        pShop->close_hour = fread_number( fp );
        fread_to_eol( fp );
        pMobIndex = get_mob_index( pShop->keeper );
        pMobIndex->pShop = pShop;

        if ( shop_first == NULL )
            shop_first = pShop;
        if ( shop_last != NULL )
            shop_last->next = pShop;

        shop_last = pShop;
        pShop->next = NULL;
        top_shop++;
    }

    return;
}

/*
 * Snarf spec proc declarations.
 */
void load_specials( FILE * fp )
{
    for ( ;; )
    {
        MOB_INDEX_DATA         *pMobIndex = NULL;
        char                    letter = '\0';

        switch ( letter = fread_letter( fp ) )
        {
            default:
                proper_exit( MUD_HALT, "Load_specials: letter '%c' not *MS.", letter );

            case 'S':
                return;

            case '*':
                break;

            case 'M':
                pMobIndex = get_mob_index( fread_number( fp ) );
                pMobIndex->spec_fun = spec_lookup( fread_word( fp ) );
                if ( pMobIndex->spec_fun == 0 )
                {
                    proper_exit( MUD_HALT, "Load_specials: 'M': vnum %d.",
                                 pMobIndex->vnum );
                }
                break;
        }

        fread_to_eol( fp );
    }
}

/*
 * Translate all room exits from virtual to real.
 * Has to be done after all rooms are read in.
 * Check for bad reverse exits.
 */
void fix_exits( void )
{
    ROOM_INDEX_DATA        *pRoomIndex = NULL;
    ROOM_INDEX_DATA        *to_room = NULL;
    EXIT_DATA              *pexit = NULL;
    EXIT_DATA              *pexit_rev = NULL;
    RESET_DATA             *pReset = NULL;
    ROOM_INDEX_DATA        *iLastRoom = NULL;
    ROOM_INDEX_DATA        *iLastObj = NULL;
    int                     iHash = 0;
    int                     door = -1;

    for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for ( pRoomIndex = room_index_hash[iHash];
              pRoomIndex != NULL; pRoomIndex = pRoomIndex->next )
        {
            bool                    fexit = false;

            iLastRoom = iLastObj = NULL;

            /*
             * OLC : new check of resets 
             */
            for ( pReset = pRoomIndex->reset_first; pReset; pReset = pReset->next )
            {
                switch ( pReset->command )
                {
                    default:
                        proper_exit( MUD_HALT,
                                     "fix_exits : room %d with reset cmd %c",
                                     pRoomIndex->vnum, pReset->command );
                        break;

                    case 'M':
                        get_mob_index( pReset->arg1 );
                        iLastRoom = get_room_index( pReset->arg3 );
                        break;

                    case 'O':
                        get_obj_index( pReset->arg1 );
                        iLastObj = get_room_index( pReset->arg3 );
                        break;

                    case 'P':
                        get_obj_index( pReset->arg1 );
                        if ( iLastObj == NULL )
                        {
                            proper_exit( MUD_HALT,
                                         "fix_exits : reset in room %d with iLastObj NULL",
                                         pRoomIndex->vnum );
                        }
                        break;

                    case 'G':
                    case 'E':
                        get_obj_index( pReset->arg1 );
                        if ( iLastRoom == NULL )
                        {
                            proper_exit( MUD_HALT,
                                         "fix_exits : reset in room %d with iLastRoom NULL",
                                         pRoomIndex->vnum );
                        }
                        iLastObj = iLastRoom;
                        break;

                    case 'D':
                        log_error( "???" );
                        break;

                    case 'R':
                        get_room_index( pReset->arg1 );
                        if ( pReset->arg2 < 0 || pReset->arg2 > MAX_DIR )
                        {
                            proper_exit( MUD_HALT,
                                         "fix_exits : reset in room %d with arg2 %d >= MAX_DIR",
                                         pRoomIndex->vnum, pReset->arg2 );
                        }
                        break;
                }                                      /* switch */
            }                                          /* for */

            for ( door = 0; door < MAX_EXIT; door++ )
            {
                if ( ( pexit = pRoomIndex->exit[door] ) != NULL )
                {
                    if ( pexit->u1.vnum <= 0 || get_room_index( pexit->u1.vnum ) == NULL )
                        pexit->u1.to_room = NULL;
                    else
                    {
                        fexit = true;
                        pexit->u1.to_room = get_room_index( pexit->u1.vnum );
                    }
                }
            }
            if ( !fexit )
                SET_BIT( pRoomIndex->room_flags, ROOM_NO_MOB );
        }
    }

    for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for ( pRoomIndex = room_index_hash[iHash];
              pRoomIndex != NULL; pRoomIndex = pRoomIndex->next )
        {
            for ( door = 0; door < MAX_EXIT; door++ )
            {
                if ( ( pexit = pRoomIndex->exit[door] ) != NULL
                     && ( to_room = pexit->u1.to_room ) != NULL
                     && ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
                     && pexit_rev->u1.to_room != pRoomIndex )
                {
                    log_error( "Non-euclidean exit: %d:%d -> %d:%d but -> %d",
                               pRoomIndex->vnum, door,
                               to_room->vnum, rev_dir[door],
                               ( pexit_rev->u1.to_room == NULL )
                               ? 0 : pexit_rev->u1.to_room->vnum );
                }
            }
        }
    }
    return;
}

/*
 * Load mobprogs section
 */
void load_mobprogs( FILE * fp )
{
    MPROG_CODE             *pMprog = NULL;

    if ( area_last == NULL )
    {
        proper_exit( MUD_HALT, "Load_mobprogs: no #AREA seen yet." );
    }

    for ( ;; )
    {
        int                     vnum = 0;
        char                    letter = '\0';

        letter = fread_letter( fp );
        if ( letter != '#' )
        {
            proper_exit( MUD_HALT, "Load_mobprogs: # not found." );
        }

        vnum = fread_number( fp );
        if ( vnum == 0 )
            break;

        fBootDb = false;
        if ( get_mprog_index( vnum ) != NULL )
        {
            proper_exit( MUD_HALT, "Load_mobprogs: vnum %d duplicated.", vnum );
        }
        fBootDb = true;

        pMprog = ( MPROG_CODE * ) alloc_perm( sizeof( *pMprog ) );
        pMprog->vnum = vnum;
        pMprog->code = fread_string( fp );
        if ( mprog_list == NULL )
            mprog_list = pMprog;
        else
        {
            pMprog->next = mprog_list;
            mprog_list = pMprog;
        }
        top_mprog_index++;
    }
    return;
}

/*
 *  Translate mobprog vnums pointers to real code
 */
void fix_mobprogs( void )
{
    MOB_INDEX_DATA         *pMobIndex;
    MPROG_LIST             *list;
    MPROG_CODE             *prog;
    int                     iHash;

    for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for ( pMobIndex = mob_index_hash[iHash];
              pMobIndex != NULL; pMobIndex = pMobIndex->next )
        {
            for ( list = pMobIndex->mprogs; list != NULL; list = list->next )
            {
                if ( ( prog = get_mprog_index( list->vnum ) ) != NULL )
                    list->code = prog->code;
                else
                {
                    proper_exit( MUD_HALT,
                                 "Fix_mobprogs: code vnum %d not found.", list->vnum );
                }
            }
        }
    }
}

/*
 * Repopulate areas periodically.
 */
void area_update( void )
{
    AREA_DATA              *pArea = NULL;

    for ( pArea = area_first; pArea != NULL; pArea = pArea->next )
    {

        if ( ++pArea->age < 3 )
            continue;

        /*
         * Check age and reset.
         * Note: Mud School resets every 3 minutes (not 15).
         */
        if ( ( !pArea->empty && ( pArea->nplayer == 0 || pArea->age >= 15 ) )
             || pArea->age >= 31 )
        {
            ROOM_INDEX_DATA        *pRoomIndex = NULL;

            reset_area( pArea );
            log_reset( "%s reset", pArea->name );
            wiz_printf( NULL, NULL, WIZ_RESETS, 0, 0, "%s reset", pArea->name );

            pArea->age = number_range( 0, 3 );
            pRoomIndex = get_room_index( ROOM_VNUM_SCHOOL );
            if ( pRoomIndex != NULL && pArea == pRoomIndex->area )
                pArea->age = 15 - 2;
            else if ( pArea->nplayer == 0 )
                pArea->empty = true;
        }
    }

    return;
}

#if 0
/*
 * Reset one area.
 */
void old_reset_area( AREA_DATA *pArea )
{
    RESET_DATA             *pReset = NULL;
    CHAR_DATA              *mob = NULL;
    bool                    last = true;
    int                     level = 0;

    for ( pReset = pArea->reset_first; pReset != NULL; pReset = pReset->next )
    {
        ROOM_INDEX_DATA        *pRoomIndex = NULL;
        MOB_INDEX_DATA         *pMobIndex = NULL;
        OBJ_INDEX_DATA         *pObjIndex = NULL;
        OBJ_INDEX_DATA         *pObjToIndex = NULL;
        EXIT_DATA              *pexit = NULL;
        OBJ_DATA               *obj = NULL;
        OBJ_DATA               *obj_to = NULL;
        int                     count = 0;
        int                     limit = 0;

        switch ( pReset->command )
        {
            default:
                log_error( "Bad reset command %c", pReset->command );
                break;

            case 'M':
                if ( ( pMobIndex = get_mob_index( pReset->arg1 ) ) == NULL )
                {
                    log_error( "'M': bad vnum %d", pReset->arg1 );
                    continue;
                }

                if ( ( pRoomIndex = get_room_index( pReset->arg3 ) ) == NULL )
                {
                    log_error( "'R': bad vnum %d", pReset->arg3 );
                    continue;
                }

                if ( pMobIndex->count >= pReset->arg2 )
                {
                    last = false;
                    break;
                }

                count = 0;
                for ( mob = pRoomIndex->people; mob != NULL; mob = mob->next_in_room )
                    if ( mob->pIndexData == pMobIndex )
                    {
                        count++;
                        if ( count >= pReset->arg4 )
                        {
                            last = false;
                            break;
                        }
                    }

                if ( count >= pReset->arg4 )
                    break;

                mob = create_mobile( pMobIndex );

                /*
                 * Check for pet shop.
                 */
                {
                    ROOM_INDEX_DATA        *pRoomIndexPrev = NULL;

                    pRoomIndexPrev = get_room_index( pRoomIndex->vnum - 1 );
                    if ( pRoomIndexPrev != NULL
                         && IS_SET( pRoomIndexPrev->room_flags, ROOM_PET_SHOP ) )
                        SET_BIT( mob->act, ACT_PET );
                }

                /*
                 * set area 
                 */
                mob->zone = pRoomIndex->area;

                char_to_room( mob, pRoomIndex );
                level = URANGE( 0, mob->level - 2, LEVEL_HERO - 1 );
                last = true;
                break;

            case 'O':
                if ( ( pObjIndex = get_obj_index( pReset->arg1 ) ) == NULL )
                {
                    log_error( "'O': bad vnum %d", pReset->arg1 );
                    continue;
                }

                if ( ( pRoomIndex = get_room_index( pReset->arg3 ) ) == NULL )
                {
                    log_error( "'R': bad vnum %d", pReset->arg3 );
                    continue;
                }

                if ( pArea->nplayer > 0
                     || count_obj_list( pObjIndex, pRoomIndex->contents ) > 0 )
                {
                    last = false;
                    break;
                }

                obj = create_object( pObjIndex, UMIN( number_fuzzy( level ),
                                                      LEVEL_HERO - 1 ) );
                obj->cost = 0;
                obj_to_room( obj, pRoomIndex );
                last = true;
                break;

            case 'P':
                if ( ( pObjIndex = get_obj_index( pReset->arg1 ) ) == NULL )
                {
                    log_error( "'P': bad vnum %d", pReset->arg1 );
                    continue;
                }

                if ( ( pObjToIndex = get_obj_index( pReset->arg3 ) ) == NULL )
                {
                    log_error( "'P': bad vnum %d", pReset->arg3 );
                    continue;
                }

                if ( pReset->arg2 > 50 )               /* old format */
                    limit = 6;
                else if ( pReset->arg2 == -1 )         /* no limit */
                    limit = 999;
                else
                    limit = pReset->arg2;

                if ( pArea->nplayer > 0
                     || ( obj_to = get_obj_type( pObjToIndex ) ) == NULL
                     || ( obj_to->in_room == NULL && !last )
                     || ( pObjIndex->count >= limit && number_range( 0, 4 ) != 0 )
                     || ( count = count_obj_list( pObjIndex, obj_to->contains ) )
                     > pReset->arg4 )
                {
                    last = false;
                    break;
                }

                while ( count < pReset->arg4 )
                {
                    obj = create_object( pObjIndex, number_fuzzy( obj_to->level ) );
                    obj_to_obj( obj, obj_to );
                    count++;
                    if ( pObjIndex->count >= limit )
                        break;
                }
                /*
                 * fix object lock state! 
                 */
                obj_to->value[1] = obj_to->pIndexData->value[1];
                last = true;
                break;

            case 'G':
            case 'E':
                if ( ( pObjIndex = get_obj_index( pReset->arg1 ) ) == NULL )
                {
                    log_error( "'E' or 'G': bad vnum %d", pReset->arg1 );
                    continue;
                }

                if ( !last )
                    break;

                if ( mob == NULL )
                {
                    log_error( "'E' or 'G': NULL mob for vnum %d", pReset->arg1 );
                    last = false;
                    break;
                }

                if ( mob->pIndexData->pShop != NULL )
                {
                    int                     olevel = 0;
                    int                     i = 0;
                    int                     j = 0;

                    if ( !pObjIndex->new_format )
                        switch ( pObjIndex->item_type )
                        {
                            case ITEM_PILL:
                            case ITEM_POTION:
                            case ITEM_SCROLL:
                                olevel = 53;
                                for ( i = 1; i < 5; i++ )
                                {
                                    if ( pObjIndex->value[i] > 0 )
                                    {
                                        for ( j = 0; j < MAX_CLASS; j++ )
                                        {
                                            olevel = UMIN( olevel,
                                                           skill_table[pObjIndex->
                                                                       value[i]].
                                                           skill_level[j] );
                                        }
                                    }
                                }

                                olevel = UMAX( 0, ( olevel * 3 / 4 ) - 2 );
                                break;
                            case ITEM_WAND:
                                olevel = number_range( 10, 20 );
                                break;
                            case ITEM_STAFF:
                                olevel = number_range( 15, 25 );
                                break;
                            case ITEM_ARMOR:
                                olevel = number_range( 5, 15 );
                                break;
                            case ITEM_WEAPON:
                                olevel = number_range( 5, 15 );
                                break;
                            case ITEM_TREASURE:
                                olevel = number_range( 10, 20 );
                                break;
                        }

                    obj = create_object( pObjIndex, olevel );
                    SET_BIT( obj->extra_flags, ITEM_INVENTORY );
                }

                else
                {
                    if ( pReset->arg2 > 50 )           /* old format */
                        limit = 6;
                    else if ( pReset->arg2 == -1 )     /* no limit */
                        limit = 999;
                    else
                        limit = pReset->arg2;

                    if ( pObjIndex->count < limit || number_range( 0, 4 ) == 0 )
                    {
                        obj = create_object( pObjIndex, UMIN( number_fuzzy( level ),
                                                              LEVEL_HERO - 1 ) );
                        /*
                         * error message if it is too high 
                         */
                        if ( obj->item_type != ITEM_WEAPON
                             && obj->level > mob->level + 3 )
                            log_balance( mob,
                                         "Level %d object \"%s\"(#%d), owned by level %d mob \"%s\"(#%d)",
                                         obj->level, obj->short_descr,
                                         obj->pIndexData->vnum, mob->level,
                                         mob->short_descr, mob->pIndexData->vnum );

                        if ( obj->item_type == ITEM_WEAPON && pReset->command == 'E'
                             && obj->level < mob->level - 5 && obj->level < 45 )
                            log_balance( mob,
                                         "Level %d weapon \"%s\"(#%d), wielded by level %d mob \"%s\"(#%d)",
                                         obj->level, obj->short_descr,
                                         obj->pIndexData->vnum, mob->level,
                                         mob->short_descr, mob->pIndexData->vnum );
                    }
                    else
                        break;
                }
                obj_to_char( obj, mob );
                if ( pReset->command == 'E' )
                    equip_char( mob, obj, pReset->arg3 );
                last = true;
                break;

            case 'D':
                if ( ( pRoomIndex = get_room_index( pReset->arg1 ) ) == NULL )
                {
                    log_error( "'D': bad vnum %d", pReset->arg1 );
                    continue;
                }

                if ( ( pexit = pRoomIndex->exit[pReset->arg2] ) == NULL )
                    break;

                switch ( pReset->arg3 )
                {
                    case 0:
                        REMOVE_BIT( pexit->exit_info, EX_CLOSED );
                        REMOVE_BIT( pexit->exit_info, EX_LOCKED );
                        break;

                    case 1:
                        SET_BIT( pexit->exit_info, EX_CLOSED );
                        REMOVE_BIT( pexit->exit_info, EX_LOCKED );
                        break;

                    case 2:
                        SET_BIT( pexit->exit_info, EX_CLOSED );
                        SET_BIT( pexit->exit_info, EX_LOCKED );
                        break;
                }

                last = true;
                break;

            case 'R':
                if ( ( pRoomIndex = get_room_index( pReset->arg1 ) ) == NULL )
                {
                    log_error( "'R': bad vnum %d", pReset->arg1 );
                    continue;
                }

                {
                    int                     d0 = 0;
                    int                     d1 = 0;

                    for ( d0 = 0; d0 < pReset->arg2 - 1; d0++ )
                    {
                        d1 = number_range( d0, pReset->arg2 - 1 );
                        pexit = pRoomIndex->exit[d0];
                        pRoomIndex->exit[d0] = pRoomIndex->exit[d1];
                        pRoomIndex->exit[d1] = pexit;
                    }
                }
                break;
        }
    }

    return;
}
#endif

/*
 * OLC
 * Reset one room.  Called by reset_area and olc.
 */
void reset_room( ROOM_INDEX_DATA *pRoom )
{
    RESET_DATA             *pReset = NULL;
    CHAR_DATA              *pMob = NULL;
    CHAR_DATA              *mob = NULL;
    OBJ_DATA               *pObj = NULL;
    CHAR_DATA              *LastMob = NULL;
    OBJ_DATA               *LastObj = NULL;
    int                     iExit;
    int                     level = 0;
    bool                    last = true;

    if ( !pRoom )
        return;

    last = false;

    for ( iExit = 0; iExit < MAX_DIR; iExit++ )
    {
        EXIT_DATA              *pExit;

        if ( ( pExit = pRoom->exit[iExit] )
             /*
              * && !IS_SET( pExit->exit_info, EX_BASHED ) ROM OLC 
              */  )
        {
            pExit->exit_info = pExit->rs_flags;
            if ( ( pExit->u1.to_room != NULL )
                 && ( ( pExit = pExit->u1.to_room->exit[rev_dir[iExit]] ) ) )
            {
                /*
                 * nail the other side 
                 */
                pExit->exit_info = pExit->rs_flags;
            }
        }
    }

    for ( pReset = pRoom->reset_first; pReset != NULL; pReset = pReset->next )
    {
        MOB_INDEX_DATA         *pMobIndex;
        OBJ_INDEX_DATA         *pObjIndex;
        OBJ_INDEX_DATA         *pObjToIndex;
        ROOM_INDEX_DATA        *pRoomIndex;
        int                     count,
                                limit = 0;

        switch ( pReset->command )
        {
            default:
                log_error( "Reset_room: bad command %c.", pReset->command );
                break;

            case 'M':
                if ( !( pMobIndex = get_mob_index( pReset->arg1 ) ) )
                {
                    log_error( "Reset_room: 'M': bad vnum %d.", pReset->arg1 );
                    continue;
                }

                if ( ( pRoomIndex = get_room_index( pReset->arg3 ) ) == NULL )
                {
                    log_error( "'R': bad vnum %d", pReset->arg3 );
                    continue;
                }

                if ( pMobIndex->count >= pReset->arg2 )
                {
                    last = false;
                    break;
                }

                count = 0;
                for ( mob = pRoomIndex->people; mob != NULL; mob = mob->next_in_room )
                    if ( mob->pIndexData == pMobIndex )
                    {
                        count++;
                        if ( count >= pReset->arg4 )
                        {
                            last = false;
                            break;
                        }
                    }

                if ( count >= pReset->arg4 )
                    break;

                pMob = create_mobile( pMobIndex );

                /*
                 * Some more hard coding.
                 */
                if ( room_is_dark( pRoom ) )
                    SET_BIT( pMob->affected_by, AFF_INFRARED );

                /*
                 * Pet shop mobiles get ACT_PET set.
                 */
                {
                    ROOM_INDEX_DATA        *pRoomIndexPrev = NULL;

                    pRoomIndexPrev = get_room_index( pRoom->vnum - 1 );
                    if ( pRoomIndexPrev
                         && IS_SET( pRoomIndexPrev->room_flags, ROOM_PET_SHOP ) )
                        SET_BIT( pMob->act, ACT_PET );
                }

                char_to_room( pMob, pRoom );

                LastMob = pMob;
                level = URANGE( 0, pMob->level - 2, LEVEL_HERO - 1 );   /* -1 ROM */
                last = true;
                break;

            case 'O':
                if ( !( pObjIndex = get_obj_index( pReset->arg1 ) ) )
                {
                    log_error( "Reset_room: 'O' 1 : bad vnum %d", pReset->arg1 );
                    log_error( "%d %d %d %d", pReset->arg1, pReset->arg2,
                               pReset->arg3, pReset->arg4 );
                    continue;
                }

                if ( !( pRoomIndex = get_room_index( pReset->arg3 ) ) )
                {
                    log_error( "Reset_room: 'O' 2 : bad vnum %d.", pReset->arg3 );
                    log_error( "%d %d %d %d", pReset->arg1, pReset->arg2,
                               pReset->arg3, pReset->arg4 );
                    continue;
                }

                if ( pRoom->area->nplayer > 0
                     || count_obj_list( pObjIndex, pRoom->contents ) > 0 )
                {
                    last = false;
                    break;
                }

                pObj = create_object( pObjIndex,       /* UMIN - ROM OLC */
                                      UMIN( number_fuzzy( level ), LEVEL_HERO - 1 ) );
                pObj->cost = 0;
                obj_to_room( pObj, pRoom );
                last = true;
                break;

            case 'P':
                if ( !( pObjIndex = get_obj_index( pReset->arg1 ) ) )
                {
                    log_error( "Reset_room: 'P': bad vnum %d.", pReset->arg1 );
                    continue;
                }

                if ( !( pObjToIndex = get_obj_index( pReset->arg3 ) ) )
                {
                    log_error( "Reset_room: 'P': bad vnum %d.", pReset->arg3 );
                    continue;
                }

                if ( pReset->arg2 > 50 )               /* old format */
                    limit = 6;
                else if ( pReset->arg2 == -1 )         /* no limit */
                    limit = 999;
                else
                    limit = pReset->arg2;

                if ( pRoom->area->nplayer > 0 || ( LastObj = get_obj_type( pObjToIndex ) ) == NULL || ( LastObj->in_room == NULL && !last ) || ( pObjIndex->count >= limit      /* && 
                                                                                                                                                                                 * number_range( 
                                                                                                                                                                                 * 0, 
                                                                                                                                                                                 * 4 
                                                                                                                                                                                 * ) 
                                                                                                                                                                                 * != 
                                                                                                                                                                                 * 0 
                                                                                                                                                                                 */
                      )
                     || ( count =
                          count_obj_list( pObjIndex,
                                          LastObj->contains ) ) > pReset->arg4 )
                {
                    last = false;
                    break;
                }

                while ( count < pReset->arg4 )
                {
                    pObj = create_object( pObjIndex, number_fuzzy( LastObj->level ) );
                    obj_to_obj( pObj, LastObj );
                    count++;
                    if ( pObjIndex->count >= limit )
                        break;
                }
                /*
                 * fix object lock state! 
                 */
                LastObj->value[1] = LastObj->pIndexData->value[1];
                last = true;
                break;

            case 'G':
            case 'E':
                if ( !( pObjIndex = get_obj_index( pReset->arg1 ) ) )
                {
                    log_error( "Reset_room: 'E' or 'G': bad vnum %d.", pReset->arg1 );
                    continue;
                }

                if ( !last )
                    break;

                if ( !LastMob )
                {
                    log_error( "Reset_room: 'E' or 'G': null mob for vnum %d.",
                               pReset->arg1 );
                    last = false;
                    break;
                }

                if ( LastMob->pIndexData->pShop )      /* Shop-keeper? */
                {
                    int                     olevel = 0;
                    int                     i = 0;
                    int                     j = 0;

                    if ( !pObjIndex->new_format )
                        switch ( pObjIndex->item_type )
                        {
                            default:
                                olevel = 0;
                                break;
                            case ITEM_PILL:
                            case ITEM_POTION:
                            case ITEM_SCROLL:
                                olevel = 53;
                                for ( i = 1; i < 5; i++ )
                                {
                                    if ( pObjIndex->value[i] > 0 )
                                    {
                                        for ( j = 0; j < MAX_CLASS; j++ )
                                        {
                                            olevel = UMIN( olevel,
                                                           skill_table[pObjIndex->
                                                                       value[i]].
                                                           skill_level[j] );
                                        }
                                    }
                                }

                                olevel = UMAX( 0, ( olevel * 3 / 4 ) - 2 );
                                break;
                            case ITEM_WAND:
                                olevel = number_range( 10, 20 );
                                break;
                            case ITEM_STAFF:
                                olevel = number_range( 15, 25 );
                                break;
                            case ITEM_ARMOR:
                                olevel = number_range( 5, 15 );
                                break;
                            case ITEM_WEAPON:
                                olevel = number_range( 5, 15 );
                                break;
                            case ITEM_TREASURE:
                                olevel = number_range( 10, 20 );
                                break;
                        }

                    pObj = create_object( pObjIndex, olevel );
                    SET_BIT( pObj->extra_flags, ITEM_INVENTORY );       /* ROM OLC */
                }
                else                                   /* ROM OLC else version */
                {
                    if ( pReset->arg2 > 50 )           /* old format */
                        limit = 6;
                    else if ( pReset->arg2 == -1 || pReset->arg2 == 0 )
                        limit = 999;
                    else
                        limit = pReset->arg2;

                    if ( pObjIndex->count < limit || number_range( 0, 4 ) == 0 )
                    {
                        pObj = create_object( pObjIndex,
                                              UMIN( number_fuzzy( level ),
                                                    LEVEL_HERO - 1 ) );
                        /*
                         * error message if it is too high 
                         */
                        if ( pObj->item_type != ITEM_WEAPON
                             && pObj->level > LastMob->level + 3 )
                            log_balance( LastMob,
                                         "Level %d object \"%s\"(#%d), owned by level %d mob \"%s\"(#%d)",
                                         pObj->level, pObj->short_descr,
                                         pObj->pIndexData->vnum, LastMob->level,
                                         LastMob->short_descr,
                                         LastMob->pIndexData->vnum );

                        if ( pObj->item_type == ITEM_WEAPON && pReset->command == 'E'
                             && pObj->level < LastMob->level - 5 && pObj->level < 45 )
                            log_balance( LastMob,
                                         "Level %d weapon \"%s\"(#%d), wielded by level %d mob \"%s\"(#%d)",
                                         pObj->level, pObj->short_descr,
                                         pObj->pIndexData->vnum, LastMob->level,
                                         LastMob->short_descr,
                                         LastMob->pIndexData->vnum );
                    }
                    else
                        break;
                }
                obj_to_char( pObj, LastMob );
                if ( pReset->command == 'E' )
                    equip_char( LastMob, pObj, pReset->arg3 );
                last = true;
                break;

            case 'D':
                break;

            case 'R':
                if ( !( pRoomIndex = get_room_index( pReset->arg1 ) ) )
                {
                    log_error( "Reset_room: 'R': bad vnum %d.", pReset->arg1 );
                    continue;
                }

                {
                    EXIT_DATA              *pExit;
                    int                     d0;
                    int                     d1;

                    for ( d0 = 0; d0 < pReset->arg2 - 1; d0++ )
                    {
                        d1 = number_range( d0, pReset->arg2 - 1 );
                        pExit = pRoomIndex->exit[d0];
                        pRoomIndex->exit[d0] = pRoomIndex->exit[d1];
                        pRoomIndex->exit[d1] = pExit;
                    }
                }
                break;
        }
    }
    return;
}

/* OLC
 * Reset one area.
 */
void reset_area( AREA_DATA *pArea )
{
    ROOM_INDEX_DATA        *pRoom;
    int                     vnum;

    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ )
    {
        if ( ( pRoom = get_room_index( vnum ) ) )
            reset_room( pRoom );
    }
    return;
}

/*
 * Create an instance of a mobile.
 */
CHAR_DATA              *create_mobile( MOB_INDEX_DATA *pMobIndex )
{
    CHAR_DATA              *mob = NULL;
    int                     i = 0;
    AFFECT_DATA             af;

    mobile_count++;

    if ( pMobIndex == NULL )
    {
        proper_exit( MUD_HALT, "Create_mobile: NULL pMobIndex." );
    }

    mob = new_char(  );

    mob->pIndexData = pMobIndex;

    mob->name = str_dup( pMobIndex->player_name );     /* OLC */
    mob->short_descr = str_dup( pMobIndex->short_descr );       /* OLC */
    mob->long_descr = str_dup( pMobIndex->long_descr ); /* OLC */
    mob->description = str_dup( pMobIndex->description );       /* OLC */
    mob->id = get_mob_id(  );
    mob->spec_fun = pMobIndex->spec_fun;
    mob->prompt = NULL;
    mob->mprog_target = NULL;

    if ( pMobIndex->wealth == 0 )
    {
        mob->silver = 0;
        mob->gold = 0;
    }
    else
    {
        int                     wealth = 0;

        wealth = number_range( pMobIndex->wealth / 2, 3 * pMobIndex->wealth / 2 );
        mob->gold = number_range( wealth / 200, wealth / 100 );
        mob->silver = wealth - ( mob->gold * 100 );
    }

    if ( pMobIndex->new_format )
        /*
         * load in new style 
         */
    {
        /*
         * read from prototype 
         */
        mob->group = pMobIndex->group;
        mob->act = pMobIndex->act;
        mob->comm = COMM_NOCHANNELS | COMM_NOSHOUT | COMM_NOTELL;
        mob->affected_by = pMobIndex->affected_by;
        mob->alignment = pMobIndex->alignment;
        mob->level = pMobIndex->level;
        mob->hitroll = pMobIndex->hitroll;
        mob->damroll = pMobIndex->damage[DICE_BONUS];
        mob->max_hit = dice( pMobIndex->hit[DICE_NUMBER],
                             pMobIndex->hit[DICE_TYPE] ) + pMobIndex->hit[DICE_BONUS];
        mob->hit = mob->max_hit;
        mob->max_mana = dice( pMobIndex->mana[DICE_NUMBER],
                              pMobIndex->mana[DICE_TYPE] ) + pMobIndex->mana[DICE_BONUS];
        mob->mana = mob->max_mana;
        mob->damage[DICE_NUMBER] = pMobIndex->damage[DICE_NUMBER];
        mob->damage[DICE_TYPE] = pMobIndex->damage[DICE_TYPE];
        mob->dam_type = pMobIndex->dam_type;
        if ( mob->dam_type == 0 )
            switch ( number_range( 1, 3 ) )
            {
                case ( 1 ):
                    mob->dam_type = 3;
                    break;                             /* slash */
                case ( 2 ):
                    mob->dam_type = 7;
                    break;                             /* pound */
                case ( 3 ):
                    mob->dam_type = 11;
                    break;                             /* pierce */
            }
        for ( i = 0; i < 4; i++ )
            mob->armor[i] = pMobIndex->ac[i];
        mob->off_flags = pMobIndex->off_flags;
        mob->imm_flags = pMobIndex->imm_flags;
        mob->res_flags = pMobIndex->res_flags;
        mob->vuln_flags = pMobIndex->vuln_flags;
        mob->start_pos = pMobIndex->start_pos;
        mob->default_pos = pMobIndex->default_pos;
        mob->sex = pMobIndex->sex;
        if ( mob->sex == 3 )                           /* random sex */
            mob->sex = number_range( 1, 2 );
        mob->race = pMobIndex->race;
        mob->form = pMobIndex->form;
        mob->parts = pMobIndex->parts;
        mob->size = pMobIndex->size;
        mob->material = str_dup( pMobIndex->material );

        /*
         * computed on the spot 
         */

        for ( i = 0; i < MAX_STATS; i++ )
            mob->perm_stat[i] = UMIN( 25, 11 + mob->level / 4 );

        if ( IS_SET( mob->act, ACT_WARRIOR ) )
        {
            mob->perm_stat[STAT_STR] += 3;
            mob->perm_stat[STAT_INT] -= 1;
            mob->perm_stat[STAT_CON] += 2;
        }

        if ( IS_SET( mob->act, ACT_THIEF ) )
        {
            mob->perm_stat[STAT_DEX] += 3;
            mob->perm_stat[STAT_INT] += 1;
            mob->perm_stat[STAT_WIS] -= 1;
        }

        if ( IS_SET( mob->act, ACT_CLERIC ) )
        {
            mob->perm_stat[STAT_WIS] += 3;
            mob->perm_stat[STAT_DEX] -= 1;
            mob->perm_stat[STAT_STR] += 1;
        }

        if ( IS_SET( mob->act, ACT_MAGE ) )
        {
            mob->perm_stat[STAT_INT] += 3;
            mob->perm_stat[STAT_STR] -= 1;
            mob->perm_stat[STAT_DEX] += 1;
        }

        if ( IS_SET( mob->off_flags, OFF_FAST ) )
            mob->perm_stat[STAT_DEX] += 2;

        mob->perm_stat[STAT_STR] += mob->size - SIZE_MEDIUM;
        mob->perm_stat[STAT_CON] += ( mob->size - SIZE_MEDIUM ) / 2;

        /*
         * let's get some spell action 
         */
        if ( IS_AFFECTED( mob, AFF_SANCTUARY ) )
        {
            af.where = TO_AFFECTS;
            af.type = skill_lookup( "sanctuary" );
            af.level = mob->level;
            af.duration = -1;
            af.location = APPLY_NONE;
            af.modifier = 0;
            af.bitvector = AFF_SANCTUARY;
            affect_to_char( mob, &af );
        }

        if ( IS_AFFECTED( mob, AFF_HASTE ) )
        {
            af.where = TO_AFFECTS;
            af.type = skill_lookup( "haste" );
            af.level = mob->level;
            af.duration = -1;
            af.location = APPLY_DEX;
            af.modifier = 1 + ( mob->level >= 18 ) + ( mob->level >= 25 ) +
                ( mob->level >= 32 );
            af.bitvector = AFF_HASTE;
            affect_to_char( mob, &af );
        }

        if ( IS_AFFECTED( mob, AFF_PROTECT_EVIL ) )
        {
            af.where = TO_AFFECTS;
            af.type = skill_lookup( "protection evil" );
            af.level = mob->level;
            af.duration = -1;
            af.location = APPLY_SAVES;
            af.modifier = -1;
            af.bitvector = AFF_PROTECT_EVIL;
            affect_to_char( mob, &af );
        }

        if ( IS_AFFECTED( mob, AFF_PROTECT_GOOD ) )
        {
            af.where = TO_AFFECTS;
            af.type = skill_lookup( "protection good" );
            af.level = mob->level;
            af.duration = -1;
            af.location = APPLY_SAVES;
            af.modifier = -1;
            af.bitvector = AFF_PROTECT_GOOD;
            affect_to_char( mob, &af );
        }
    }
    else
    {
        /*
         * read in old format and convert 
         */
        mob->act = pMobIndex->act;
        mob->affected_by = pMobIndex->affected_by;
        mob->alignment = pMobIndex->alignment;
        mob->level = pMobIndex->level;
        mob->hitroll = pMobIndex->hitroll;
        mob->damroll = 0;
        mob->max_hit = mob->level * 8 + number_range( mob->level * mob->level / 4,
                                                      mob->level * mob->level );
        mob->max_hit *= 10 / 9;
        mob->hit = mob->max_hit;
        mob->max_mana = 100 + dice( mob->level, 10 );
        mob->mana = mob->max_mana;
        switch ( number_range( 1, 3 ) )
        {
            case ( 1 ):
                mob->dam_type = 3;
                break;                                 /* slash */
            case ( 2 ):
                mob->dam_type = 7;
                break;                                 /* pound */
            case ( 3 ):
                mob->dam_type = 11;
                break;                                 /* pierce */
        }
        for ( i = 0; i < 3; i++ )
            mob->armor[i] = interpolate( mob->level, 100, -100 );
        mob->armor[3] = interpolate( mob->level, 100, 0 );
        mob->race = pMobIndex->race;
        mob->off_flags = pMobIndex->off_flags;
        mob->imm_flags = pMobIndex->imm_flags;
        mob->res_flags = pMobIndex->res_flags;
        mob->vuln_flags = pMobIndex->vuln_flags;
        mob->start_pos = pMobIndex->start_pos;
        mob->default_pos = pMobIndex->default_pos;
        mob->sex = pMobIndex->sex;
        mob->form = pMobIndex->form;
        mob->parts = pMobIndex->parts;
        mob->size = SIZE_MEDIUM;
        mob->material = str_dup( "" );                 /* This should be ok as a shared
                                                        * string */

        for ( i = 0; i < MAX_STATS; i++ )
            mob->perm_stat[i] = 11 + mob->level / 4;
    }

    mob->position = mob->start_pos;

    /*
     * link the mob to the world list 
     */
    mob->next = char_list;
    char_list = mob;
    pMobIndex->count++;
    return mob;
}

/* duplicate a mobile exactly -- except inventory */
void clone_mobile( CHAR_DATA *parent, CHAR_DATA *clone )
{
    int                     i = 0;
    AFFECT_DATA            *paf = NULL;

    if ( parent == NULL || clone == NULL || !IS_NPC( parent ) )
        return;

    /*
     * start fixing values 
     */
    clone->name = str_dup( parent->name );
    clone->version = parent->version;
    clone->short_descr = str_dup( parent->short_descr );
    clone->long_descr = str_dup( parent->long_descr );
    clone->description = str_dup( parent->description );
    clone->group = parent->group;
    clone->sex = parent->sex;
    clone->iclass = parent->iclass;
    clone->race = parent->race;
    clone->level = parent->level;
    clone->trust = 0;
    clone->timer = parent->timer;
    clone->wait = parent->wait;
    clone->hit = parent->hit;
    clone->max_hit = parent->max_hit;
    clone->mana = parent->mana;
    clone->max_mana = parent->max_mana;
    clone->move = parent->move;
    clone->max_move = parent->max_move;
    clone->gold = parent->gold;
    clone->silver = parent->silver;
    clone->exp = parent->exp;
    clone->act = parent->act;
    clone->comm = parent->comm;
    clone->imm_flags = parent->imm_flags;
    clone->res_flags = parent->res_flags;
    clone->vuln_flags = parent->vuln_flags;
    clone->invis_level = parent->invis_level;
    clone->affected_by = parent->affected_by;
    clone->position = parent->position;
    clone->practice = parent->practice;
    clone->train = parent->train;
    clone->saving_throw = parent->saving_throw;
    clone->alignment = parent->alignment;
    clone->hitroll = parent->hitroll;
    clone->damroll = parent->damroll;
    clone->wimpy = parent->wimpy;
    clone->form = parent->form;
    clone->parts = parent->parts;
    clone->size = parent->size;
    clone->material = str_dup( parent->material );
    clone->off_flags = parent->off_flags;
    clone->dam_type = parent->dam_type;
    clone->start_pos = parent->start_pos;
    clone->default_pos = parent->default_pos;
    clone->spec_fun = parent->spec_fun;

    for ( i = 0; i < 4; i++ )
        clone->armor[i] = parent->armor[i];

    for ( i = 0; i < MAX_STATS; i++ )
    {
        clone->perm_stat[i] = parent->perm_stat[i];
        clone->mod_stat[i] = parent->mod_stat[i];
    }

    for ( i = 0; i < 3; i++ )
        clone->damage[i] = parent->damage[i];

    /*
     * now add the affects 
     */
    for ( paf = parent->affected; paf != NULL; paf = paf->next )
        affect_to_char( clone, paf );

}

/*
 * Create an instance of an object.
 */
OBJ_DATA               *create_object( OBJ_INDEX_DATA *pObjIndex, int level )
{
    AFFECT_DATA            *paf = NULL;
    OBJ_DATA               *obj = NULL;

    if ( pObjIndex == NULL )
    {
        proper_exit( MUD_HALT, "Create_object: NULL pObjIndex." );
    }

    obj = new_obj(  );

    obj->pIndexData = pObjIndex;
    obj->in_room = NULL;
    obj->enchanted = false;

    if ( pObjIndex->new_format )
        obj->level = pObjIndex->level;
    else
        obj->level = UMAX( 0, level );
    obj->wear_loc = -1;

    obj->name = str_dup( pObjIndex->name );            /* OLC */
    obj->short_descr = str_dup( pObjIndex->short_descr );       /* OLC */
    obj->description = str_dup( pObjIndex->description );       /* OLC */
    obj->material = str_dup( pObjIndex->material );
    obj->item_type = pObjIndex->item_type;
    obj->extra_flags = pObjIndex->extra_flags;
    obj->wear_flags = pObjIndex->wear_flags;
    obj->value[0] = pObjIndex->value[0];
    obj->value[1] = pObjIndex->value[1];
    obj->value[2] = pObjIndex->value[2];
    obj->value[3] = pObjIndex->value[3];
    obj->value[4] = pObjIndex->value[4];
    obj->weight = pObjIndex->weight;

    if ( level == -1 || pObjIndex->new_format )
        obj->cost = pObjIndex->cost;
    else
        obj->cost = number_fuzzy( 10 ) * number_fuzzy( level ) * number_fuzzy( level );

    /*
     * Mess with object properties.
     */
    switch ( obj->item_type )
    {
        default:
            log_error( "Object VNUM %d is of unknown type", pObjIndex->vnum );
            break;

        case ITEM_LIGHT:
            if ( obj->value[2] == 999 )
                obj->value[2] = -1;
            break;

        case ITEM_FURNITURE:
        case ITEM_TRASH:
        case ITEM_CONTAINER:
        case ITEM_DRINK_CON:
        case ITEM_KEY:
        case ITEM_FOOD:
        case ITEM_BOAT:
        case ITEM_CORPSE_NPC:
        case ITEM_CORPSE_PC:
        case ITEM_FOUNTAIN:
        case ITEM_MAP:
        case ITEM_CLOTHING:
        case ITEM_PORTAL:
            if ( !pObjIndex->new_format )
                obj->cost /= 5;
            break;

        case ITEM_TREASURE:
        case ITEM_WARP_STONE:
        case ITEM_ROOM_KEY:
        case ITEM_GEM:
        case ITEM_JEWELRY:
            break;

        case ITEM_SCROLL:
            if ( level != -1 && !pObjIndex->new_format )
                obj->value[0] = number_fuzzy( obj->value[0] );
            break;

        case ITEM_WAND:
        case ITEM_STAFF:
            if ( level != -1 && !pObjIndex->new_format )
            {
                obj->value[0] = number_fuzzy( obj->value[0] );
                obj->value[1] = number_fuzzy( obj->value[1] );
                obj->value[2] = obj->value[1];
            }
            if ( !pObjIndex->new_format )
                obj->cost *= 2;
            break;

        case ITEM_WEAPON:
            if ( level != -1 && !pObjIndex->new_format )
            {
                obj->value[1] = number_fuzzy( number_fuzzy( 1 * level / 4 + 2 ) );
                obj->value[2] = number_fuzzy( number_fuzzy( 3 * level / 4 + 6 ) );
            }
            break;

        case ITEM_ARMOR:
            if ( level != -1 && !pObjIndex->new_format )
            {
                obj->value[0] = number_fuzzy( level / 5 + 3 );
                obj->value[1] = number_fuzzy( level / 5 + 3 );
                obj->value[2] = number_fuzzy( level / 5 + 3 );
            }
            break;

        case ITEM_POTION:
        case ITEM_PILL:
            if ( level != -1 && !pObjIndex->new_format )
                obj->value[0] = number_fuzzy( number_fuzzy( obj->value[0] ) );
            break;

        case ITEM_MONEY:
            if ( !pObjIndex->new_format )
                obj->value[0] = obj->cost;
            break;
    }

    for ( paf = pObjIndex->affected; paf != NULL; paf = paf->next )
        if ( paf->location == APPLY_SPELL_AFFECT )
            affect_to_obj( obj, paf );

    obj->next = object_list;
    object_list = obj;
    pObjIndex->count++;

    return obj;
}

/* duplicate an object exactly -- except contents */
void clone_object( OBJ_DATA *parent, OBJ_DATA *clone )
{
    int                     i = 0;
    AFFECT_DATA            *paf = NULL;
    EXTRA_DESCR_DATA       *ed = NULL;
    EXTRA_DESCR_DATA       *ed_new = NULL;

    if ( parent == NULL || clone == NULL )
        return;

    /*
     * start fixing the object 
     */
    clone->name = str_dup( parent->name );
    clone->short_descr = str_dup( parent->short_descr );
    clone->description = str_dup( parent->description );
    clone->item_type = parent->item_type;
    clone->extra_flags = parent->extra_flags;
    clone->wear_flags = parent->wear_flags;
    clone->weight = parent->weight;
    clone->cost = parent->cost;
    clone->level = parent->level;
    clone->condition = parent->condition;
    clone->material = str_dup( parent->material );
    clone->timer = parent->timer;

    for ( i = 0; i < 5; i++ )
        clone->value[i] = parent->value[i];

    /*
     * affects 
     */
    clone->enchanted = parent->enchanted;

    for ( paf = parent->affected; paf != NULL; paf = paf->next )
        affect_to_obj( clone, paf );

    /*
     * extended desc 
     */
    for ( ed = parent->extra_descr; ed != NULL; ed = ed->next )
    {
        ed_new = new_extra_descr(  );
        ed_new->keyword = str_dup( ed->keyword );
        ed_new->description = str_dup( ed->description );
        ed_new->next = clone->extra_descr;
        clone->extra_descr = ed_new;
    }

}

/*
 * Clear a new character.
 */
void clear_char( CHAR_DATA *ch )
{
    static CHAR_DATA        ch_zero;
    int                     i = 0;

    *ch = ch_zero;
    ch->name = &str_empty[0];
    ch->short_descr = &str_empty[0];
    ch->long_descr = &str_empty[0];
    ch->description = &str_empty[0];
    ch->prompt = &str_empty[0];
    ch->logon = current_time;
    ch->lines = PAGELEN;
    for ( i = 0; i < 4; i++ )
        ch->armor[i] = 100;
    ch->position = POS_STANDING;
    ch->hit = 20;
    ch->max_hit = 20;
    ch->mana = 100;
    ch->max_mana = 100;
    ch->move = 100;
    ch->max_move = 100;
    ch->on = NULL;
    for ( i = 0; i < MAX_STATS; i++ )
    {
        ch->perm_stat[i] = 13;
        ch->mod_stat[i] = 0;
    }
    return;
}

/*
 * Get an extra description from a list.
 */
char                   *get_extra_descr( char *name, EXTRA_DESCR_DATA *ed )
{
    for ( ; ed != NULL; ed = ed->next )
    {
        if ( is_name( name, ed->keyword ) )
            return ed->description;
    }
    return NULL;
}

/*
 * Translates mob virtual number to its mob index struct.
 * Hash table lookup.
 */
MOB_INDEX_DATA         *get_mob_index( int vnum )
{
    MOB_INDEX_DATA         *pMobIndex = NULL;

    for ( pMobIndex = mob_index_hash[vnum % MAX_KEY_HASH];
          pMobIndex != NULL; pMobIndex = pMobIndex->next )
    {
        if ( pMobIndex->vnum == vnum )
            return pMobIndex;
    }

    if ( fBootDb )
    {
        proper_exit( MUD_HALT, "Get_mob_index: bad vnum %d.", vnum );
    }

    return NULL;
}

/*
 * Translates mob virtual number to its obj index struct.
 * Hash table lookup.
 */
OBJ_INDEX_DATA         *get_obj_index( int vnum )
{
    OBJ_INDEX_DATA         *pObjIndex = NULL;

    for ( pObjIndex = obj_index_hash[vnum % MAX_KEY_HASH];
          pObjIndex != NULL; pObjIndex = pObjIndex->next )
    {
        if ( pObjIndex->vnum == vnum )
            return pObjIndex;
    }

    if ( fBootDb )
    {
        proper_exit( MUD_HALT, "Get_obj_index: bad vnum %d.", vnum );
    }

    return NULL;
}

/*
 * Translates mob virtual number to its room index struct.
 * Hash table lookup.
 */
ROOM_INDEX_DATA        *get_room_index( int vnum )
{
    ROOM_INDEX_DATA        *pRoomIndex = NULL;

    for ( pRoomIndex = room_index_hash[vnum % MAX_KEY_HASH];
          pRoomIndex != NULL; pRoomIndex = pRoomIndex->next )
    {
        if ( pRoomIndex->vnum == vnum )
            return pRoomIndex;
    }

    if ( fBootDb )
    {
        proper_exit( MUD_HALT, "Get_room_index: bad vnum %d.", vnum );
    }

    return NULL;
}

/*
 * Read a letter from a file.
 */
char fread_letter( FILE * fp )
{
    char                    c = '\0';

    do
    {
        c = getc( fp );
    }
    while ( isspace( c ) );

    return c;
}

/*
 * Read a number from a file.
 */
int fread_number( FILE * fp )
{
    int                     number = 0;
    bool                    sign = false;
    char                    c = '\0';

    do
    {
        c = getc( fp );
    }
    while ( isspace( c ) );

    number = 0;

    sign = false;
    if ( c == '+' )
    {
        c = getc( fp );
    }
    else if ( c == '-' )
    {
        sign = true;
        c = getc( fp );
    }

    if ( !isdigit( c ) )
    {
        proper_exit( MUD_HALT, "Fread_number: bad format." );
    }

    while ( isdigit( c ) )
    {
        number = number * 10 + c - '0';
        c = getc( fp );
    }

    if ( sign )
        number = 0 - number;

    if ( c == '|' )
        number += fread_number( fp );
    else if ( c != ' ' )
        ungetc( c, fp );

    return number;
}

int fread_flag( FILE * fp )
{
    int                     number = 0;
    char                    c = '\0';
    bool                    negative = false;

    do
    {
        c = getc( fp );
    }
    while ( isspace( c ) );

    if ( c == '-' )
    {
        negative = true;
        c = getc( fp );
    }

    number = 0;

    if ( !isdigit( c ) )
    {
        while ( ( 'A' <= c && c <= 'Z' ) || ( 'a' <= c && c <= 'z' ) )
        {
            number += flag_convert( c );
            c = getc( fp );
        }
    }

    while ( isdigit( c ) )
    {
        number = number * 10 + c - '0';
        c = getc( fp );
    }

    if ( c == '|' )
        number += fread_flag( fp );

    else if ( c != ' ' )
        ungetc( c, fp );

    if ( negative )
        return -1 * number;

    return number;
}

int flag_convert( char letter )
{
    int                     bitsum = 0;
    char                    i = '\0';

    if ( 'A' <= letter && letter <= 'Z' )
    {
        bitsum = 1;
        for ( i = letter; i > 'A'; i-- )
            bitsum *= 2;
    }
    else if ( 'a' <= letter && letter <= 'z' )
    {
        bitsum = 67108864;                             /* 2^26 */
        for ( i = letter; i > 'a'; i-- )
            bitsum *= 2;
    }

    return bitsum;
}

/*
 * Read and allocate space for a string from a file.
 * These strings are read-only and shared.
 * Strings are hashed:
 *   each string prepended with hash pointer to prev string,
 *   hash code is simply the string length.
 *   this function takes 40% to 50% of boot-up time.
 */
char                   *fread_string( FILE * fp )
{
    char                   *plast = NULL;
    char                    c = '\0';

    plast = top_string + sizeof( char * );
    if ( plast > &string_space[MAX_STRING - MAX_STRING_LENGTH] )
    {
        proper_exit( MUD_HALT, "Fread_string: MAX_STRING %d exceeded.", MAX_STRING );
    }

    /*
     * Skip blanks.
     * Read first char.
     */
    do
    {
        c = getc( fp );
    }
    while ( isspace( c ) );

    if ( ( *plast++ = c ) == '~' )
        return &str_empty[0];

    for ( ;; )
    {
        /*
         * Back off the char type lookup,
         *   it was too dirty for portability.
         *   -- Furey
         */

        switch ( *plast = getc( fp ) )
        {
            default:
                plast++;
                break;

            case EOF:
                /*
                 * temp fix 
                 */
                log_error( "%s", "EOF in fread_string" );
                return NULL;
                /*
                 * exit( 1 ); 
                 */
                break;

            case '\n':
                plast++;
                *plast++ = '\r';
                break;

            case '\r':
                break;

            case '~':
                plast++;
                {
                    union
                    {
                        char                   *pc;
                        char                    rgc[sizeof( char * )];
                    } u1;
                    size_t                  ic = 0;
                    int                     iHash = 0;
                    char                   *pHash = NULL;
                    char                   *pHashPrev = NULL;
                    char                   *pString = NULL;

                    plast[-1] = '\0';
                    iHash = UMIN( MAX_KEY_HASH - 1, plast - 1 - top_string );
                    for ( pHash = string_hash[iHash]; pHash; pHash = pHashPrev )
                    {
                        for ( ic = 0; ic < sizeof( char * ); ic++ )
                            u1.rgc[ic] = pHash[ic];
                        pHashPrev = u1.pc;
                        pHash += sizeof( char * );

                        if ( top_string[sizeof( char * )] == pHash[0]
                             && !strcmp( top_string + sizeof( char * ) + 1, pHash + 1 ) )
                            return pHash;
                    }

                    if ( fBootDb )
                    {
                        pString = top_string;
                        top_string = plast;
                        u1.pc = string_hash[iHash];
                        for ( ic = 0; ic < sizeof( char * ); ic++ )
                            pString[ic] = u1.rgc[ic];
                        string_hash[iHash] = pString;

                        nAllocString += 1;
                        sAllocString += top_string - pString;
                        return pString + sizeof( char * );
                    }
                    else
                    {
                        return str_dup( top_string + sizeof( char * ) );
                    }
                }
        }
    }
}

char                   *fread_string_eol( FILE * fp )
{
    static bool             char_special[256 - EOF];
    char                   *plast = NULL;
    char                    c = '\0';

    if ( char_special[EOF - EOF] != true )
    {
        char_special[EOF - EOF] = true;
        char_special['\n' - EOF] = true;
        char_special['\r' - EOF] = true;
    }

    plast = top_string + sizeof( char * );
    if ( plast > &string_space[MAX_STRING - MAX_STRING_LENGTH] )
    {
        proper_exit( MUD_HALT, "Fread_string: MAX_STRING %d exceeded.", MAX_STRING );
    }

    /*
     * Skip blanks.
     * Read first char.
     */
    do
    {
        c = getc( fp );
    }
    while ( isspace( c ) );

    if ( ( *plast++ = c ) == '\n' )
        return &str_empty[0];

    for ( ;; )
    {
        if ( !char_special[( *plast++ = getc( fp ) ) - EOF] )
            continue;

        switch ( plast[-1] )
        {
            default:
                break;

            case EOF:
                /*
                 * temp fix 
                 */
                log_error( "%s", "EOF in fread_string_eol" );
                return NULL;
                /*
                 * exit( 1 ); 
                 */
                break;

            case '\n':
            case '\r':
                {
                    union
                    {
                        char                   *pc;
                        char                    rgc[sizeof( char * )];
                    } u1;
                    size_t                  ic = 0;
                    int                     iHash = 0;
                    char                   *pHash = NULL;
                    char                   *pHashPrev = NULL;
                    char                   *pString = NULL;

                    plast[-1] = '\0';
                    iHash = UMIN( MAX_KEY_HASH - 1, plast - 1 - top_string );
                    for ( pHash = string_hash[iHash]; pHash; pHash = pHashPrev )
                    {
                        for ( ic = 0; ic < sizeof( char * ); ic++ )
                            u1.rgc[ic] = pHash[ic];
                        pHashPrev = u1.pc;
                        pHash += sizeof( char * );

                        if ( top_string[sizeof( char * )] == pHash[0]
                             && !strcmp( top_string + sizeof( char * ) + 1, pHash + 1 ) )
                            return pHash;
                    }

                    if ( fBootDb )
                    {
                        pString = top_string;
                        top_string = plast;
                        u1.pc = string_hash[iHash];
                        for ( ic = 0; ic < sizeof( char * ); ic++ )
                            pString[ic] = u1.rgc[ic];
                        string_hash[iHash] = pString;

                        nAllocString += 1;
                        sAllocString += top_string - pString;
                        return pString + sizeof( char * );
                    }
                    else
                    {
                        return str_dup( top_string + sizeof( char * ) );
                    }
                }
        }
    }
}

/*
 * Read to end of line (for comments).
 */
void fread_to_eol( FILE * fp )
{
    char                    c = '\0';

    do
    {
        c = getc( fp );
    }
    while ( c != '\n' && c != '\r' );

    do
    {
        c = getc( fp );
    }
    while ( c == '\n' || c == '\r' );

    ungetc( c, fp );
    return;
}

/*
 * Read one word (into static buffer).
 */
char                   *fread_word( FILE * fp )
{
    static char             word[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                   *pword = NULL;
    char                    cEnd = '\0';

    do
    {
        cEnd = getc( fp );
    }
    while ( isspace( cEnd ) );

    if ( cEnd == '\'' || cEnd == '"' )
    {
        pword = word;
    }
    else
    {
        word[0] = cEnd;
        pword = word + 1;
        cEnd = ' ';
    }

    for ( ; pword < word + MAX_INPUT_LENGTH; pword++ )
    {
        *pword = getc( fp );
        if ( cEnd == ' ' ? isspace( *pword ) : *pword == cEnd )
        {
            if ( cEnd == ' ' )
                ungetc( *pword, fp );
            *pword = '\0';
            return word;
        }
    }

    proper_exit( MUD_HALT, "Fread_word: word too long." );
    return NULL;
}

void do_areas( CHAR_DATA *ch, const char *argument )
{
    AREA_DATA              *pArea1 = NULL;
    AREA_DATA              *pArea2 = NULL;
    int                     iArea = 0;
    int                     iAreaHalf = 0;

    if ( argument[0] != '\0' )
    {
        ch_printf( ch, "No argument is used with this command.\r\n" );
        return;
    }

    iAreaHalf = ( top_area + 1 ) / 2;
    pArea1 = area_first;
    pArea2 = area_first;
    for ( iArea = 0; iArea < iAreaHalf; iArea++ )
        pArea2 = pArea2->next;

    for ( iArea = 0; iArea < iAreaHalf; iArea++ )
    {
        ch_printf( ch, "%-39s%-39s\r\n",
                   pArea1->credits, ( pArea2 != NULL ) ? pArea2->credits : "" );
        pArea1 = pArea1->next;
        if ( pArea2 != NULL )
            pArea2 = pArea2->next;
    }

    return;
}

void do_memory( CHAR_DATA *ch, const char *argument )
{
    ch_printf( ch, "Affects %5d\r\n", top_affect );
    ch_printf( ch, "Areas   %5d\r\n", top_area );
    ch_printf( ch, "ExDes   %5d\r\n", top_ed );
    ch_printf( ch, "Exits   %5d\r\n", top_exit );
    ch_printf( ch, "Helps   %5d\r\n", top_help );
    ch_printf( ch, "Socials %5d\r\n", social_count );
    ch_printf( ch, "Mobs    %5d(%d new format)\r\n", top_mob_index, newmobs );
    ch_printf( ch, "(in use)%5d\r\n", mobile_count );
    ch_printf( ch, "Objs    %5d(%d new format)\r\n", top_obj_index, newobjs );
    ch_printf( ch, "Resets  %5d\r\n", top_reset );
    ch_printf( ch, "Rooms   %5d\r\n", top_room );
    ch_printf( ch, "Shops   %5d\r\n", top_shop );
    ch_printf( ch, "Strings %5d strings of %7zd bytes (max %d).\r\n",
               nAllocString, sAllocString, MAX_STRING );
    ch_printf( ch, "Perms   %5d blocks  of %7zd bytes.\r\n", nAllocPerm, sAllocPerm );

    return;
}

void do_dump( CHAR_DATA *ch, const char *argument )
{
    int                     count = 0;
    int                     count2 = 0;
    int                     num_pcs = 0;
    int                     aff_count = 0;
    CHAR_DATA              *fch = NULL;
    MOB_INDEX_DATA         *pMobIndex = NULL;
    PC_DATA                *pc = NULL;
    OBJ_DATA               *obj = NULL;
    OBJ_INDEX_DATA         *pObjIndex = NULL;
    ROOM_INDEX_DATA        *room = NULL;
    EXIT_DATA              *pExit = NULL;
    DESCRIPTOR_DATA        *d = NULL;
    AFFECT_DATA            *af = NULL;
    FILE                   *fp = NULL;
    int                     vnum = 0;
    int                     nMatch = 0;

    /*
     * open file 
     */
    fp = fopen( "mem.dmp", "w" );

    /*
     * report use of data structures 
     */

    num_pcs = 0;
    aff_count = 0;

    /*
     * mobile prototypes 
     */
    fprintf( fp, "MobProt        %4d (%8zd bytes)\n",
             top_mob_index, top_mob_index * ( sizeof( *pMobIndex ) ) );

    /*
     * mobs 
     */
    count = 0;
    count2 = 0;
    for ( fch = char_list; fch != NULL; fch = fch->next )
    {
        count++;
        if ( fch->pcdata != NULL )
            num_pcs++;
        for ( af = fch->affected; af != NULL; af = af->next )
            aff_count++;
    }
    for ( fch = char_free; fch != NULL; fch = fch->next )
        count2++;

    fprintf( fp, "Mobs        %4d (%8zd bytes), %2d free (%zd bytes)\n",
             count, count * ( sizeof( *fch ) ), count2, count2 * ( sizeof( *fch ) ) );

    /*
     * pcdata 
     */
    count = 0;
    for ( pc = pcdata_free; pc != NULL; pc = pc->next )
        count++;

    fprintf( fp, "Pcdata        %4d (%8zd bytes), %2d free (%zd bytes)\n",
             num_pcs, num_pcs * ( sizeof( *pc ) ), count, count * ( sizeof( *pc ) ) );

    /*
     * descriptors 
     */
    count = 0;
    count2 = 0;
    for ( d = descriptor_list; d != NULL; d = d->next )
        count++;
    for ( d = descriptor_free; d != NULL; d = d->next )
        count2++;

    fprintf( fp, "Descs        %4d (%8zd bytes), %2d free (%zd bytes)\n",
             count, count * ( sizeof( *d ) ), count2, count2 * ( sizeof( *d ) ) );

    /*
     * object prototypes 
     */
    for ( vnum = 0; nMatch < top_obj_index; vnum++ )
        if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL )
        {
            for ( af = pObjIndex->affected; af != NULL; af = af->next )
                aff_count++;
            nMatch++;
        }

    fprintf( fp, "ObjProt        %4d (%8zd bytes)\n",
             top_obj_index, top_obj_index * ( sizeof( *pObjIndex ) ) );

    /*
     * objects 
     */
    count = 0;
    count2 = 0;
    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
        count++;
        for ( af = obj->affected; af != NULL; af = af->next )
            aff_count++;
    }
    for ( obj = obj_free; obj != NULL; obj = obj->next )
        count2++;

    fprintf( fp, "Objs        %4d (%8zd bytes), %2d free (%zd bytes)\n",
             count, count * ( sizeof( *obj ) ), count2, count2 * ( sizeof( *obj ) ) );

    /*
     * affects 
     */
    count = 0;
    for ( af = affect_free; af != NULL; af = af->next )
        count++;

    fprintf( fp, "Affects        %4d (%8zd bytes), %2d free (%zd bytes)\n",
             aff_count, aff_count * ( sizeof( *af ) ), count, count * ( sizeof( *af ) ) );

    /*
     * rooms 
     */
    fprintf( fp, "Rooms        %4d (%8zd bytes)\n", top_room,
             top_room * ( sizeof( *room ) ) );

    /*
     * exits 
     */
    fprintf( fp, "Exits        %4d (%8zd bytes)\n", top_exit,
             top_exit * ( sizeof( *pExit ) ) );

    fclose( fp );

    /*
     * start printing out mobile data 
     */
    fp = fopen( "mob.dmp", "w" );

    fprintf( fp, "\nMobile Analysis\n" );
    fprintf( fp, "---------------\n" );
    nMatch = 0;
    for ( vnum = 0; nMatch < top_mob_index; vnum++ )
        if ( ( pMobIndex = get_mob_index( vnum ) ) != NULL )
        {
            nMatch++;
            fprintf( fp, "#%-4d %3d active %3d killed     %s\n",
                     pMobIndex->vnum, pMobIndex->count,
                     pMobIndex->killed, pMobIndex->short_descr );
        }
    fclose( fp );

    /*
     * start printing out object data 
     */
    fp = fopen( "obj.dmp", "w" );

    fprintf( fp, "\nObject Analysis\n" );
    fprintf( fp, "---------------\n" );
    nMatch = 0;
    for ( vnum = 0; nMatch < top_obj_index; vnum++ )
        if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL )
        {
            nMatch++;
            fprintf( fp, "#%-4d %3d active %3d reset      %s\n",
                     pObjIndex->vnum, pObjIndex->count,
                     pObjIndex->reset_num, pObjIndex->short_descr );
        }

    /*
     * close file 
     */
    fclose( fp );
}

/*
 * Simple linear interpolation.
 */
int interpolate( int level, int value_00, int value_32 )
{
    return value_00 + level * ( value_32 - value_00 ) / 32;
}

/*
 * Append a string to a file.
 */
void append_file( CHAR_DATA *ch, const char *file, const char *str )
{
    FILE                   *fp = NULL;

    if ( !str || !*str )
        return;

    if ( ch && IS_NPC( ch ) )
        return;

    if ( ( fp = fopen( file, "a" ) ) == NULL )
    {
        char                   *e = strerror( errno );

        log_error( "fopen: %s: %s", file, e );
        if ( ch )
            ch_printf( ch, "Could not open the file!\r\n" );
    }
    else
    {
        if ( ch )
            fprintf( fp, "[%5d] %s: %s\n",
                     ch->in_room ? ch->in_room->vnum : 0, NAME( ch ), str );
        else
            fprintf( fp, "[    0] SYSTEM: %s\n", str );
        fclose( fp );
    }

    return;
}

/*
 * This function is here to aid in debugging.
 * If the last expression in a function is another function call,
 *   gcc likes to generate a JMP instead of a CALL.
 * This is called "tail chaining."
 * It hoses the debugger call stack for that call.
 * So I make this the last call in certain critical functions,
 *   where I really need the call stack to be right for debugging!
 *
 * If you don't understand this, then LEAVE IT ALONE.
 * Don't remove any calls to tail_chain anywhere.
 *
 * -- Furey
 */
void tail_chain( void )
{
    return;
}

MPROG_CODE             *get_mprog_index( int vnum )
{
    MPROG_CODE             *prg = NULL;

    for ( prg = mprog_list; prg; prg = prg->next )
    {
        if ( prg->vnum == vnum )
            return ( prg );
    }
    return NULL;
}

/* snarf a socials file */
void load_socials( FILE * fp )
{
    for ( ;; )
    {
        struct social_type      social;
        char                   *temp = NULL;

        /*
         * clear social 
         */
        social.char_no_arg = NULL;
        social.others_no_arg = NULL;
        social.char_found = NULL;
        social.others_found = NULL;
        social.vict_found = NULL;
        social.char_not_found = NULL;
        social.char_auto = NULL;
        social.others_auto = NULL;

        temp = fread_word( fp );
        if ( !strcmp( temp, "#0" ) )
            return;                                    /* done */
#if defined(SOCIAL_DEBUG)
        else
            log_boot( "SOCIAL: %s", temp );
#endif

        strcpy( social.name, temp );
        fread_to_eol( fp );

        temp = fread_string_eol( fp );
        if ( !strcmp( temp, "$" ) )
            social.char_no_arg = NULL;
        else if ( !strcmp( temp, "#" ) )
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.char_no_arg = temp;

        temp = fread_string_eol( fp );
        if ( !strcmp( temp, "$" ) )
            social.others_no_arg = NULL;
        else if ( !strcmp( temp, "#" ) )
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.others_no_arg = temp;

        temp = fread_string_eol( fp );
        if ( !strcmp( temp, "$" ) )
            social.char_found = NULL;
        else if ( !strcmp( temp, "#" ) )
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.char_found = temp;

        temp = fread_string_eol( fp );
        if ( !strcmp( temp, "$" ) )
            social.others_found = NULL;
        else if ( !strcmp( temp, "#" ) )
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.others_found = temp;

        temp = fread_string_eol( fp );
        if ( !strcmp( temp, "$" ) )
            social.vict_found = NULL;
        else if ( !strcmp( temp, "#" ) )
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.vict_found = temp;

        temp = fread_string_eol( fp );
        if ( !strcmp( temp, "$" ) )
            social.char_not_found = NULL;
        else if ( !strcmp( temp, "#" ) )
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.char_not_found = temp;

        temp = fread_string_eol( fp );
        if ( !strcmp( temp, "$" ) )
            social.char_auto = NULL;
        else if ( !strcmp( temp, "#" ) )
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.char_auto = temp;

        temp = fread_string_eol( fp );
        if ( !strcmp( temp, "$" ) )
            social.others_auto = NULL;
        else if ( !strcmp( temp, "#" ) )
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.others_auto = temp;

        social_table[social_count] = social;
        social_count++;
    }
    return;
}

/*
 * Snarf a mob section.  new style
 */
void load_mobiles( FILE * fp )
{
    MOB_INDEX_DATA         *pMobIndex;

    if ( !area_last )                                  /* OLC */
    {
        proper_exit( MUD_HALT, "Load_mobiles: no #AREA seen yet." );
    }

    for ( ;; )
    {
        int                     vnum = 0;
        char                    letter = '\0';
        int                     iHash = 0;

        letter = fread_letter( fp );
        if ( letter != '#' )
        {
            proper_exit( MUD_HALT, "Load_mobiles: # not found." );
        }

        vnum = fread_number( fp );
        if ( vnum == 0 )
            break;

        fBootDb = false;
        if ( get_mob_index( vnum ) != NULL )
        {
            proper_exit( MUD_HALT, "Load_mobiles: vnum %d duplicated.", vnum );
        }
        fBootDb = true;

        pMobIndex = ( MOB_INDEX_DATA * ) alloc_perm( sizeof( *pMobIndex ) );
        pMobIndex->vnum = vnum;
        pMobIndex->area = area_last;                   /* OLC */
        pMobIndex->new_format = true;
        newmobs++;
        pMobIndex->player_name = fread_string( fp );
        pMobIndex->short_descr = fread_string( fp );
        pMobIndex->long_descr = fread_string( fp );
        pMobIndex->description = fread_string( fp );
        pMobIndex->race = race_lookup( fread_string( fp ) );

        pMobIndex->long_descr[0] = UPPER( pMobIndex->long_descr[0] );
        pMobIndex->description[0] = UPPER( pMobIndex->description[0] );

        pMobIndex->act = fread_flag( fp ) | ACT_IS_NPC | race_table[pMobIndex->race].act;
        pMobIndex->affected_by = fread_flag( fp ) | race_table[pMobIndex->race].aff;
        pMobIndex->pShop = NULL;
        pMobIndex->alignment = fread_number( fp );
        pMobIndex->group = fread_number( fp );

        pMobIndex->level = fread_number( fp );
        pMobIndex->hitroll = fread_number( fp );

        /*
         * read hit dice 
         */
        pMobIndex->hit[DICE_NUMBER] = fread_number( fp );
        fread_letter( fp );                            /* 'd' */
        pMobIndex->hit[DICE_TYPE] = fread_number( fp );
        fread_letter( fp );                            /* '+' */
        pMobIndex->hit[DICE_BONUS] = fread_number( fp );

        /*
         * read mana dice 
         */
        pMobIndex->mana[DICE_NUMBER] = fread_number( fp );
        fread_letter( fp );
        pMobIndex->mana[DICE_TYPE] = fread_number( fp );
        fread_letter( fp );
        pMobIndex->mana[DICE_BONUS] = fread_number( fp );

        /*
         * read damage dice 
         */
        pMobIndex->damage[DICE_NUMBER] = fread_number( fp );
        fread_letter( fp );
        pMobIndex->damage[DICE_TYPE] = fread_number( fp );
        fread_letter( fp );
        pMobIndex->damage[DICE_BONUS] = fread_number( fp );
        pMobIndex->dam_type = attack_lookup( fread_word( fp ) );

        /*
         * read armor class 
         */
        pMobIndex->ac[AC_PIERCE] = fread_number( fp ) * 10;
        pMobIndex->ac[AC_BASH] = fread_number( fp ) * 10;
        pMobIndex->ac[AC_SLASH] = fread_number( fp ) * 10;
        pMobIndex->ac[AC_EXOTIC] = fread_number( fp ) * 10;

        /*
         * read flags and add in data from the race table 
         */
        pMobIndex->off_flags = fread_flag( fp ) | race_table[pMobIndex->race].off;
        pMobIndex->imm_flags = fread_flag( fp ) | race_table[pMobIndex->race].imm;
        pMobIndex->res_flags = fread_flag( fp ) | race_table[pMobIndex->race].res;
        pMobIndex->vuln_flags = fread_flag( fp ) | race_table[pMobIndex->race].vuln;

        /*
         * vital statistics 
         */
        pMobIndex->start_pos = position_lookup( fread_word( fp ) );
        pMobIndex->default_pos = position_lookup( fread_word( fp ) );
        pMobIndex->sex = sex_lookup( fread_word( fp ) );

        pMobIndex->wealth = fread_number( fp );

        pMobIndex->form = fread_flag( fp ) | race_table[pMobIndex->race].form;
        pMobIndex->parts = fread_flag( fp ) | race_table[pMobIndex->race].parts;
        /*
         * size 
         */
        CHECK_POS( pMobIndex->size, size_lookup( fread_word( fp ) ), "size" );
        pMobIndex->material = str_dup( fread_word( fp ) );

        for ( ;; )
        {
            letter = fread_letter( fp );

            if ( letter == 'F' )
            {
                char                   *word = NULL;
                int                     vector = 0;

                word = fread_word( fp );
                vector = fread_flag( fp );

                /*
                 * These really shouldn't be str_prefix()
                 */

                if ( !str_cmp( word, "act" ) )
                    REMOVE_BIT( pMobIndex->act, vector );
                else if ( !str_cmp( word, "aff" ) )
                    REMOVE_BIT( pMobIndex->affected_by, vector );
                else if ( !str_cmp( word, "off" ) )
                    REMOVE_BIT( pMobIndex->off_flags, vector );
                else if ( !str_cmp( word, "imm" ) )
                    REMOVE_BIT( pMobIndex->imm_flags, vector );
                else if ( !str_cmp( word, "res" ) )
                    REMOVE_BIT( pMobIndex->res_flags, vector );
                else if ( !str_cmp( word, "vul" ) )
                    REMOVE_BIT( pMobIndex->vuln_flags, vector );
                else if ( !str_cmp( word, "for" ) )
                    REMOVE_BIT( pMobIndex->form, vector );
                else if ( !str_cmp( word, "par" ) )
                    REMOVE_BIT( pMobIndex->parts, vector );
                else
                {
                    proper_exit( MUD_HALT, "Flag remove: flag not found." );
                }
            }
            else if ( letter == 'M' )
            {
                MPROG_LIST             *pMprog = NULL;
                char                   *word = NULL;
                int                     trigger = 0;

                pMprog = ( MPROG_LIST * ) alloc_perm( sizeof( *pMprog ) );
                word = fread_word( fp );
                if ( ( trigger = flag_lookup( word, mprog_flags ) ) == NO_FLAG )
                {
                    proper_exit( MUD_HALT, "MOBprogs: invalid trigger." );
                }
                SET_BIT( pMobIndex->mprog_flags, trigger );
                pMprog->trig_type = trigger;
                pMprog->vnum = fread_number( fp );
                pMprog->trig_phrase = fread_string( fp );
                pMprog->next = pMobIndex->mprogs;
                pMobIndex->mprogs = pMprog;
            }
            else
            {
                ungetc( letter, fp );
                break;
            }
        }

        iHash = vnum % MAX_KEY_HASH;
        pMobIndex->next = mob_index_hash[iHash];
        mob_index_hash[iHash] = pMobIndex;
        top_mob_index++;
        top_vnum_mob = top_vnum_mob < vnum ? vnum : top_vnum_mob;       /* OLC */
        assign_area_vnum( vnum );                      /* OLC */
        kill_table[URANGE( 0, pMobIndex->level, MAX_LEVEL - 1 )].number++;
    }

    return;
}

/*
 * Snarf an obj section. new style
 */
void load_objects( FILE * fp )
{
    OBJ_INDEX_DATA         *pObjIndex = NULL;

    if ( !area_last )                                  /* OLC */
    {
        proper_exit( MUD_HALT, "Load_objects: no #AREA seen yet." );
    }

    for ( ;; )
    {
        int                     vnum = 0;
        char                    letter = '\0';
        int                     iHash = 0;

        letter = fread_letter( fp );
        if ( letter != '#' )
        {
            proper_exit( MUD_HALT, "Load_objects: # not found." );
        }

        vnum = fread_number( fp );
        if ( vnum == 0 )
            break;

        fBootDb = false;
        if ( get_obj_index( vnum ) != NULL )
        {
            proper_exit( MUD_HALT, "Load_objects: vnum %d duplicated.", vnum );
        }
        fBootDb = true;

        pObjIndex = ( OBJ_INDEX_DATA * ) alloc_perm( sizeof( *pObjIndex ) );
        pObjIndex->vnum = vnum;
        pObjIndex->area = area_last;                   /* OLC */
        pObjIndex->new_format = true;
        pObjIndex->reset_num = 0;
        newobjs++;
        pObjIndex->name = fread_string( fp );
        pObjIndex->short_descr = fread_string( fp );
        pObjIndex->description = fread_string( fp );
        pObjIndex->material = fread_string( fp );

        CHECK_POS( pObjIndex->item_type, item_lookup( fread_word( fp ) ), "item_type" );
        pObjIndex->extra_flags = fread_flag( fp );
        pObjIndex->wear_flags = fread_flag( fp );
        switch ( pObjIndex->item_type )
        {
            case ITEM_WEAPON:
                pObjIndex->value[0] = weapon_type_lookup( fread_word( fp ) );
                pObjIndex->value[1] = fread_number( fp );
                pObjIndex->value[2] = fread_number( fp );
                pObjIndex->value[3] = attack_lookup( fread_word( fp ) );
                pObjIndex->value[4] = fread_flag( fp );
                break;
            case ITEM_CONTAINER:
                pObjIndex->value[0] = fread_number( fp );
                pObjIndex->value[1] = fread_flag( fp );
                pObjIndex->value[2] = fread_number( fp );
                pObjIndex->value[3] = fread_number( fp );
                pObjIndex->value[4] = fread_number( fp );
                break;
            case ITEM_DRINK_CON:
            case ITEM_FOUNTAIN:
                pObjIndex->value[0] = fread_number( fp );
                pObjIndex->value[1] = fread_number( fp );
                CHECK_POS( pObjIndex->value[2],
                           liq_lookup( fread_word( fp ) ), "liq_lookup" );
                pObjIndex->value[3] = fread_number( fp );
                pObjIndex->value[4] = fread_number( fp );
                break;
            case ITEM_WAND:
            case ITEM_STAFF:
                pObjIndex->value[0] = fread_number( fp );
                pObjIndex->value[1] = fread_number( fp );
                pObjIndex->value[2] = fread_number( fp );
                pObjIndex->value[3] = skill_lookup( fread_word( fp ) );
                pObjIndex->value[4] = fread_number( fp );
                break;
            case ITEM_POTION:
            case ITEM_PILL:
            case ITEM_SCROLL:
                pObjIndex->value[0] = fread_number( fp );
                pObjIndex->value[1] = skill_lookup( fread_word( fp ) );
                pObjIndex->value[2] = skill_lookup( fread_word( fp ) );
                pObjIndex->value[3] = skill_lookup( fread_word( fp ) );
                pObjIndex->value[4] = skill_lookup( fread_word( fp ) );
                break;
            default:
                pObjIndex->value[0] = fread_flag( fp );
                pObjIndex->value[1] = fread_flag( fp );
                pObjIndex->value[2] = fread_flag( fp );
                pObjIndex->value[3] = fread_flag( fp );
                pObjIndex->value[4] = fread_flag( fp );
                break;
        }
        pObjIndex->level = fread_number( fp );
        pObjIndex->weight = fread_number( fp );
        pObjIndex->cost = fread_number( fp );

        /*
         * condition 
         */
        letter = fread_letter( fp );
        switch ( letter )
        {
            case ( 'P' ):
                pObjIndex->condition = 100;
                break;
            case ( 'G' ):
                pObjIndex->condition = 90;
                break;
            case ( 'A' ):
                pObjIndex->condition = 75;
                break;
            case ( 'W' ):
                pObjIndex->condition = 50;
                break;
            case ( 'D' ):
                pObjIndex->condition = 25;
                break;
            case ( 'B' ):
                pObjIndex->condition = 10;
                break;
            case ( 'R' ):
                pObjIndex->condition = 0;
                break;
            default:
                pObjIndex->condition = 100;
                break;
        }

        for ( ;; )
        {
            char                    another_letter = '\0';

            another_letter = fread_letter( fp );

            if ( another_letter == 'A' )
            {
                AFFECT_DATA            *paf;

                paf = ( AFFECT_DATA * ) alloc_perm( sizeof( *paf ) );
                paf->where = TO_OBJECT;
                paf->type = -1;
                paf->level = pObjIndex->level;
                paf->duration = -1;
                paf->location = fread_number( fp );
                paf->modifier = fread_number( fp );
                paf->bitvector = 0;
                paf->next = pObjIndex->affected;
                pObjIndex->affected = paf;
                top_affect++;
            }

            else if ( another_letter == 'F' )
            {
                AFFECT_DATA            *paf;

                paf = ( AFFECT_DATA * ) alloc_perm( sizeof( *paf ) );
                another_letter = fread_letter( fp );
                switch ( another_letter )
                {
                    case 'A':
                        paf->where = TO_AFFECTS;
                        break;
                    case 'I':
                        paf->where = TO_IMMUNE;
                        break;
                    case 'R':
                        paf->where = TO_RESIST;
                        break;
                    case 'V':
                        paf->where = TO_VULN;
                        break;
                    default:
                        proper_exit( MUD_HALT, "Load_objects: Bad where on flag set." );
                        break;
                }
                paf->type = -1;
                paf->level = pObjIndex->level;
                paf->duration = -1;
                paf->location = fread_number( fp );
                paf->modifier = fread_number( fp );
                paf->bitvector = fread_flag( fp );
                paf->next = pObjIndex->affected;
                pObjIndex->affected = paf;
                top_affect++;
            }

            else if ( another_letter == 'E' )
            {
                EXTRA_DESCR_DATA       *ed;

                ed = ( EXTRA_DESCR_DATA * ) alloc_perm( sizeof( *ed ) );
                ed->keyword = fread_string( fp );
                ed->description = fread_string( fp );
                ed->next = pObjIndex->extra_descr;
                pObjIndex->extra_descr = ed;
                top_ed++;
            }

            else
            {
                ungetc( another_letter, fp );
                break;
            }
        }

        iHash = vnum % MAX_KEY_HASH;
        pObjIndex->next = obj_index_hash[iHash];
        obj_index_hash[iHash] = pObjIndex;
        top_obj_index++;
        top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;       /* OLC */
        assign_area_vnum( vnum );                      /* OLC */
    }

    return;
}

/*****************************************************************************
 Name:          convert_objects
 Purpose:       Converts all old format objects to new format
 Called by:     boot_db (db.c).
 Note:          Loops over all resets to find the level of the mob
                loaded before the object to determine the level of
                the object.
                It might be better to update the levels in load_resets().
                This function is not pretty.. Sorry about that :)
 Author:        Hugin
 ****************************************************************************/
void convert_objects( void )
{
    int                     vnum = 0;
    AREA_DATA              *pArea = NULL;
    RESET_DATA             *pReset = NULL;
    MOB_INDEX_DATA         *pMob = NULL;
    OBJ_INDEX_DATA         *pObj = NULL;
    OBJ_INDEX_DATA         *pObjTo = NULL;
    ROOM_INDEX_DATA        *pRoom = NULL;

    if ( newobjs == top_obj_index )
        return;                                        /* all objects in new format */

    for ( pArea = area_first; pArea; pArea = pArea->next )
    {
        for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ )
        {
            if ( !( pRoom = get_room_index( vnum ) ) )
                continue;

            for ( pReset = pRoom->reset_first; pReset; pReset = pReset->next )
            {
                switch ( pReset->command )
                {
                    case 'M':
                        if ( !( pMob = get_mob_index( pReset->arg1 ) ) )
                            log_error( "Convert_objects: 'M': bad vnum %d.",
                                       pReset->arg1 );
                        break;

                    case 'O':
                        if ( !( pObj = get_obj_index( pReset->arg1 ) ) )
                        {
                            log_error( "Convert_objects: 'O': bad vnum %d.",
                                       pReset->arg1 );
                            break;
                        }

                        if ( pObj->new_format )
                            continue;

                        if ( !pMob )
                        {
                            log_error( "Convert_objects: 'O': No mob reset yet." );
                            break;
                        }

                        pObj->level = pObj->level < 1 ? pMob->level - 2
                            : UMIN( pObj->level, pMob->level - 2 );
                        break;

                    case 'P':
                        {

                            if ( !( pObj = get_obj_index( pReset->arg1 ) ) )
                            {
                                log_error( "Convert_objects: 'P': bad vnum %d.",
                                           pReset->arg1 );
                                break;
                            }

                            if ( pObj->new_format )
                                continue;

                            if ( !( pObjTo = get_obj_index( pReset->arg3 ) ) )
                            {
                                log_error( "Convert_objects: 'P': bad vnum %d.",
                                           pReset->arg3 );
                                break;
                            }

                            pObj->level = pObj->level < 1 ? pObjTo->level
                                : UMIN( pObj->level, pObjTo->level );
                        }
                        break;

                    case 'G':
                    case 'E':
                        if ( !( pObj = get_obj_index( pReset->arg1 ) ) )
                        {
                            log_error( "Convert_objects: 'E' or 'G': bad vnum %d.",
                                       pReset->arg1 );
                            break;
                        }

                        if ( !pMob )
                        {
                            log_error
                                ( "Convert_objects: 'E' or 'G': null mob for vnum %d.",
                                  pReset->arg1 );
                            break;
                        }

                        if ( pObj->new_format )
                            continue;

                        if ( pMob->pShop )
                        {
                            switch ( pObj->item_type )
                            {
                                default:
                                    pObj->level = UMAX( 0, pObj->level );
                                    break;
                                case ITEM_PILL:
                                case ITEM_POTION:
                                    pObj->level = UMAX( 5, pObj->level );
                                    break;
                                case ITEM_SCROLL:
                                case ITEM_ARMOR:
                                case ITEM_WEAPON:
                                    pObj->level = UMAX( 10, pObj->level );
                                    break;
                                case ITEM_WAND:
                                case ITEM_TREASURE:
                                    pObj->level = UMAX( 15, pObj->level );
                                    break;
                                case ITEM_STAFF:
                                    pObj->level = UMAX( 20, pObj->level );
                                    break;
                            }
                        }
                        else
                            pObj->level = pObj->level < 1 ? pMob->level
                                : UMIN( pObj->level, pMob->level );
                        break;
                }                                      /* switch ( pReset->command ) */
            }
        }
    }

    /*
     * do the conversion: 
     */

    for ( pArea = area_first; pArea; pArea = pArea->next )
        for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ )
            if ( ( pObj = get_obj_index( vnum ) ) )
                if ( !pObj->new_format )
                    convert_object( pObj );

    return;
}

/*****************************************************************************
 Name:          convert_object
 Purpose:       Converts an old_format obj to new_format
 Called by:     convert_objects (db2.c).
 Note:          Dug out of create_obj (db.c)
 Author:        Hugin
 ****************************************************************************/
void convert_object( OBJ_INDEX_DATA *pObjIndex )
{
    int                     level = 0;
    int                     number = 0;
    int                     type = 0;                      /* for dice-conversion */

    if ( !pObjIndex || pObjIndex->new_format )
        return;

    level = pObjIndex->level;

    pObjIndex->level = UMAX( 0, pObjIndex->level );    /* just to be sure */
    pObjIndex->cost = 10 * level;

    switch ( pObjIndex->item_type )
    {
        default:
            log_error( "Obj_convert: vnum %d bad type.", pObjIndex->item_type );
            break;

        case ITEM_LIGHT:
        case ITEM_TREASURE:
        case ITEM_FURNITURE:
        case ITEM_TRASH:
        case ITEM_CONTAINER:
        case ITEM_DRINK_CON:
        case ITEM_KEY:
        case ITEM_FOOD:
        case ITEM_BOAT:
        case ITEM_CORPSE_NPC:
        case ITEM_CORPSE_PC:
        case ITEM_FOUNTAIN:
        case ITEM_MAP:
        case ITEM_CLOTHING:
        case ITEM_SCROLL:
            break;

        case ITEM_WAND:
        case ITEM_STAFF:
            pObjIndex->value[2] = pObjIndex->value[1];
            break;

        case ITEM_WEAPON:

            /*
             * The conversion below is based on the values generated
             * in one_hit() (fight.c).  Since I don't want a lvl 50 
             * weapon to do 15d3 damage, the min value will be below
             * the one in one_hit, and to make up for it, I've made 
             * the max value higher.
             * (I don't want 15d2 because this will hardly ever roll
             * 15 or 30, it will only roll damage close to 23.
             * I can't do 4d8+11, because one_hit there is no dice-
             * bounus value to set...)
             *
             * The conversion below gives:
             
             level:   dice      min      max      mean
             1:     1d8      1( 2)    8( 7)     5( 5)
             2:     2d5      2( 3)   10( 8)     6( 6)
             3:     2d5      2( 3)   10( 8)     6( 6)
             5:     2d6      2( 3)   12(10)     7( 7)
             10:     4d5      4( 5)   20(14)    12(10)
             20:     5d5      5( 7)   25(21)    15(14)
             30:     5d7      5(10)   35(29)    20(20)
             50:     5d11     5(15)   55(44)    30(30)
             
             */

            number = UMIN( level / 4 + 1, 5 );
            type = ( level + 7 ) / number;

            pObjIndex->value[1] = number;
            pObjIndex->value[2] = type;
            break;

        case ITEM_ARMOR:
            pObjIndex->value[0] = level / 5 + 3;
            pObjIndex->value[1] = pObjIndex->value[0];
            pObjIndex->value[2] = pObjIndex->value[0];
            break;

        case ITEM_POTION:
        case ITEM_PILL:
            break;

        case ITEM_MONEY:
            pObjIndex->value[0] = pObjIndex->cost;
            break;
    }

    pObjIndex->new_format = true;
    ++newobjs;

    return;
}

/*****************************************************************************
 Name:          convert_mobile
 Purpose:       Converts an old_format mob into new_format
 Called by:     load_old_mob (db.c).
 Note:          Dug out of create_mobile (db.c)
 Author:        Hugin
 ****************************************************************************/
void convert_mobile( MOB_INDEX_DATA *pMobIndex )
{
    int                     i = 0;
    int                     type = 0;
    int                     number = 0;
    int                     bonus = 0;
    int                     level = 0;

    if ( !pMobIndex || pMobIndex->new_format )
        return;

    level = pMobIndex->level;

    pMobIndex->act |= ACT_WARRIOR;

    /*
     * Calculate hit dice.  Gives close to the hitpoints
     * of old format mobs created with create_mobile()  (db.c)
     * A high number of dice makes for less variance in mobiles
     * hitpoints.
     * (might be a good idea to reduce the max number of dice)
     *
     * The conversion below gives:
     
     level:     dice         min         max        diff       mean
     1:       1d2+6       7(  7)     8(   8)     1(   1)     8(   8)
     2:       1d3+15     16( 15)    18(  18)     2(   3)    17(  17)
     3:       1d6+24     25( 24)    30(  30)     5(   6)    27(  27)
     5:      1d17+42     43( 42)    59(  59)    16(  17)    51(  51)
     10:      3d22+96     99( 95)   162( 162)    63(  67)   131(    )
     15:     5d30+161    166(159)   311( 311)   145( 150)   239(    )
     30:    10d61+416    426(419)  1026(1026)   600( 607)   726(    )
     50:    10d169+920   930(923)  2610(2610)  1680(1688)  1770(    )
     
     The values in parenthesis give the values generated in create_mobile.
     Diff = max - min.  Mean is the arithmetic mean.
     (hmm.. must be some roundoff error in my calculations.. smurfette got
     1d6+23 hp at level 3 ? -- anyway.. the values above should be
     approximately right..)
     */
    type = level * level * 27 / 40;
    number = UMIN( type / 40 + 1, 10 );                /* how do they get 11 ??? */
    type = UMAX( 2, type / number );
/*  bonus = UMAX( 0, level * ( 8 + level ) * .9 - number * type ); */
    bonus = UMAX( 0, level * ( ( 8 + level ) * 9 / 10 ) - number * type );

    pMobIndex->hit[DICE_NUMBER] = number;
    pMobIndex->hit[DICE_TYPE] = type;
    pMobIndex->hit[DICE_BONUS] = bonus;

    pMobIndex->mana[DICE_NUMBER] = level;
    pMobIndex->mana[DICE_TYPE] = 10;
    pMobIndex->mana[DICE_BONUS] = 100;

    /*
     * Calculate dam dice.  Gives close to the damage
     * of old format mobs in damage()  (fight.c)
     */
    type = level * 7 / 4;
    number = UMIN( type / 8 + 1, 5 );
    type = UMAX( 2, type / number );
    bonus = UMAX( 0, level * 9 / 4 - number * type );

    pMobIndex->damage[DICE_NUMBER] = number;
    pMobIndex->damage[DICE_TYPE] = type;
    pMobIndex->damage[DICE_BONUS] = bonus;

    switch ( number_range( 1, 3 ) )
    {
        case ( 1 ):
            pMobIndex->dam_type = 3;
            break;                                     /* slash */
        case ( 2 ):
            pMobIndex->dam_type = 7;
            break;                                     /* pound */
        case ( 3 ):
            pMobIndex->dam_type = 11;
            break;                                     /* pierce */
    }

    for ( i = 0; i < 3; i++ )
        pMobIndex->ac[i] = interpolate( level, 100, -100 );
    pMobIndex->ac[3] = interpolate( level, 100, 0 );   /* exotic */

    pMobIndex->wealth /= 100;
    pMobIndex->size = SIZE_MEDIUM;
    pMobIndex->material = str_dup( "none" );

    pMobIndex->new_format = true;
    ++newmobs;

    return;
}

/* stuff for recycling mobprograms */
MPROG_LIST             *mprog_free = NULL;

MPROG_LIST             *new_mprog( void )
{
    static MPROG_LIST       mp_zero;
    MPROG_LIST             *mp = NULL;

    if ( mprog_free == NULL )
        mp = ( MPROG_LIST * ) alloc_perm( sizeof( *mp ) );
    else
    {
        mp = mprog_free;
        mprog_free = mprog_free->next;
    }

    *mp = mp_zero;
    mp->vnum = 0;
    mp->trig_type = 0;
    mp->code = str_dup( "" );
    VALIDATE( mp );
    return mp;
}

void free_mprog( MPROG_LIST *mp )
{
    if ( !IS_VALID( mp ) )
        return;

    INVALIDATE( mp );
    mp->next = mprog_free;
    mprog_free = mp;
}

HELP_AREA              *had_free = NULL;

HELP_AREA              *new_had( void )
{
    HELP_AREA              *had = NULL;
    static HELP_AREA        zHad;

    if ( had_free )
    {
        had = had_free;
        had_free = had_free->next;
    }
    else
        had = ( HELP_AREA * ) alloc_perm( sizeof( *had ) );

    *had = zHad;

    return had;
}

HELP_DATA              *help_free = NULL;

HELP_DATA              *new_help( void )
{
    HELP_DATA              *help = NULL;

    if ( help_free )
    {
        help = help_free;
        help_free = help_free->next;
    }
    else
        help = ( HELP_DATA * ) alloc_perm( sizeof( *help ) );

    return help;
}

void free_help( HELP_DATA *help )
{
    free_string( help->keyword );
    free_string( help->text );
    help->next = help_free;
    help_free = help;
}
