/*
 * RAM $Id: save.c 67 2009-01-05 00:39:32Z quixadhal $
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
#include <sys/types.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#include "merc.h"
#include "strings.h"
#include "tables.h"
#include "db.h"
#include "magic.h"

char                   *print_flags( int flag )
{
    int                     count = 0;
    int                     pos = 0;
    static char             buf[52] = "\0\0\0\0\0\0\0";

    for ( count = 0; count < 32; count++ )
    {
        if ( IS_SET( flag, 1 << count ) )
        {
            if ( count < 26 )
                buf[pos] = 'A' + count;
            else
                buf[pos] = 'a' + ( count - 26 );
            pos++;
        }
    }

    if ( pos == 0 )
    {
        buf[pos] = '0';
        pos++;
    }

    buf[pos] = '\0';

    return buf;
}

/*
 * Array of containers read for proper re-nesting of objects.
 */
#define MAX_NEST        100
static OBJ_DATA        *rgObjNest[MAX_NEST];

/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes,
 *   some of the infrastructure is provided.
 */
void save_char_obj( CHAR_DATA *ch )
{
    char                    strsave[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    FILE                   *fp = NULL;

    if ( IS_NPC( ch ) )
        return;

    if ( ch->desc != NULL && ch->desc->original != NULL )
        ch = ch->desc->original;

    sprintf( strsave, "%s/%c/%s",
             ( IS_IMMORTAL( ch ) || ch->level >= LEVEL_IMMORTAL ) ?
             GOD_DIR : PLAYER_DIR, LOWER( ch->name[0] ), capitalize( ch->name ) );
    if ( ( fp = fopen( TEMP_FILE, "w" ) ) == NULL )
    {
        char                   *e = strerror( errno );

        log_error( "fopen: %s: %s", strsave, e );
    }
    else
    {
        fwrite_char( ch, fp );
        if ( ch->carrying != NULL )
            fwrite_obj( ch, ch->carrying, fp, 0 );
        /*
         * save the pets 
         */
        if ( ch->pet != NULL && ch->pet->in_room == ch->in_room )
            fwrite_pet( ch->pet, fp );
        fprintf( fp, "#END\n" );
    }
    fclose( fp );
    rename( TEMP_FILE, strsave );
    return;
}

/*
 * Write the char.
 */
void fwrite_char( CHAR_DATA *ch, FILE * fp )
{
    AFFECT_DATA            *paf = NULL;
    int                     sn = -1;
    int                     gn = -1;
    int                     pos = -1;

    fprintf( fp, "#%s\n", IS_NPC( ch ) ? "MOB" : "PLAYER" );

    fprintf( fp, "Name %s~\n", ch->name );
    fprintf( fp, "Id   %d\n", ch->id );
    fprintf( fp, "LogO %d\n", ( int ) current_time );
    fprintf( fp, "Vers %d\n", 5 );
    if ( ch->short_descr[0] != '\0' )
        fprintf( fp, "ShD  %s~\n", ch->short_descr );
    if ( ch->long_descr[0] != '\0' )
        fprintf( fp, "LnD  %s~\n", ch->long_descr );
    if ( ch->description[0] != '\0' )
        fprintf( fp, "Desc %s~\n", ch->description );
    if ( ch->prompt != NULL || !str_cmp( ch->prompt, "<%hhp %mm %vmv> " ) )
        fprintf( fp, "Prom %s~\n", ch->prompt );
    fprintf( fp, "Race %s~\n", pc_race_table[ch->race].name );
    if ( ch->clan )
        fprintf( fp, "Clan %s~\n", clan_table[ch->clan].name );
    fprintf( fp, "Sex  %d\n", ch->sex );
    fprintf( fp, "Cla  %d\n", ch->iclass );
    fprintf( fp, "Levl %d\n", ch->level );
    if ( ch->trust != 0 )
        fprintf( fp, "Tru  %d\n", ch->trust );
    fprintf( fp, "Sec  %d\n", ch->pcdata->security );  /* OLC */
    fprintf( fp, "Plyd %d\n", ch->played + ( int ) ( current_time - ch->logon ) );
    fprintf( fp, "Not  %d %d %d %d %d\n",
             ( int ) ch->pcdata->last_note, ( int ) ch->pcdata->last_idea,
             ( int ) ch->pcdata->last_penalty, ( int ) ch->pcdata->last_news,
             ( int ) ch->pcdata->last_changes );
    fprintf( fp, "Scro %d\n", ch->lines );
    fprintf( fp, "Room %d\n",
             ( ch->in_room == get_room_index( ROOM_VNUM_LIMBO )
               && ch->was_in_room != NULL )
             ? ch->was_in_room->vnum : ch->in_room ==
             NULL ? ROOM_VNUM_TEMPLE : ch->in_room->vnum );

    fprintf( fp, "HMV  %d %d %d %d %d %d\n",
             ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move );
    if ( ch->gold > 0 )
        fprintf( fp, "Gold %d\n", ch->gold );
    else
        fprintf( fp, "Gold %d\n", 0 );
    if ( ch->silver > 0 )
        fprintf( fp, "Silv %d\n", ch->silver );
    else
        fprintf( fp, "Silv %d\n", 0 );
    fprintf( fp, "Exp  %d\n", ch->exp );
    if ( ch->act != 0 )
        fprintf( fp, "Act  %s\n", print_flags( ch->act ) );
    if ( ch->affected_by != 0 )
        fprintf( fp, "AfBy %s\n", print_flags( ch->affected_by ) );
    fprintf( fp, "Comm %s\n", print_flags( ch->comm ) );
    if ( ch->wiznet )
        fprintf( fp, "Wizn %s\n", print_flags( ch->wiznet ) );
    if ( ch->invis_level )
        fprintf( fp, "Invi %d\n", ch->invis_level );
    if ( ch->incog_level )
        fprintf( fp, "Inco %d\n", ch->incog_level );
    fprintf( fp, "Pos  %d\n",
             ch->position == POS_FIGHTING ? POS_STANDING : ch->position );
    if ( ch->practice != 0 )
        fprintf( fp, "Prac %d\n", ch->practice );
    if ( ch->train != 0 )
        fprintf( fp, "Trai %d\n", ch->train );
    if ( ch->saving_throw != 0 )
        fprintf( fp, "Save  %d\n", ch->saving_throw );
    fprintf( fp, "Alig  %d\n", ch->alignment );
    if ( ch->hitroll != 0 )
        fprintf( fp, "Hit   %d\n", ch->hitroll );
    if ( ch->damroll != 0 )
        fprintf( fp, "Dam   %d\n", ch->damroll );
    fprintf( fp, "ACs %d %d %d %d\n",
             ch->armor[0], ch->armor[1], ch->armor[2], ch->armor[3] );
    if ( ch->wimpy != 0 )
        fprintf( fp, "Wimp  %d\n", ch->wimpy );
    fprintf( fp, "Attr %d %d %d %d %d\n",
             ch->perm_stat[STAT_STR],
             ch->perm_stat[STAT_INT],
             ch->perm_stat[STAT_WIS], ch->perm_stat[STAT_DEX], ch->perm_stat[STAT_CON] );

    fprintf( fp, "AMod %d %d %d %d %d\n",
             ch->mod_stat[STAT_STR],
             ch->mod_stat[STAT_INT],
             ch->mod_stat[STAT_WIS], ch->mod_stat[STAT_DEX], ch->mod_stat[STAT_CON] );

    if ( IS_NPC( ch ) )
    {
        fprintf( fp, "Vnum %d\n", ch->pIndexData->vnum );
    }
    else
    {
        fprintf( fp, "Pass %s~\n", ch->pcdata->pwd );
        if ( ch->pcdata->bamfin[0] != '\0' )
            fprintf( fp, "Bin  %s~\n", ch->pcdata->bamfin );
        if ( ch->pcdata->bamfout[0] != '\0' )
            fprintf( fp, "Bout %s~\n", ch->pcdata->bamfout );
        fprintf( fp, "Titl %s~\n", ch->pcdata->title );
        fprintf( fp, "Pnts %d\n", ch->pcdata->points );
        fprintf( fp, "TSex %d\n", ch->pcdata->true_sex );
        fprintf( fp, "LLev %d\n", ch->pcdata->last_level );
        fprintf( fp, "HMVP %d %d %d\n", ch->pcdata->perm_hit,
                 ch->pcdata->perm_mana, ch->pcdata->perm_move );
        fprintf( fp, "Cnd  %d %d %d %d\n",
                 ch->pcdata->condition[0],
                 ch->pcdata->condition[1],
                 ch->pcdata->condition[2], ch->pcdata->condition[3] );

        /*
         * write alias 
         */
        for ( pos = 0; pos < MAX_ALIAS; pos++ )
        {
            if ( ch->pcdata->alias[pos] == NULL || ch->pcdata->alias_sub[pos] == NULL )
                break;

            fprintf( fp, "Alias %s %s~\n", ch->pcdata->alias[pos],
                     ch->pcdata->alias_sub[pos] );
        }

        for ( sn = 0; sn < MAX_SKILL; sn++ )
        {
            if ( skill_table[sn].name != NULL && ch->pcdata->learned[sn] > 0 )
            {
                fprintf( fp, "Sk %d '%s'\n",
                         ch->pcdata->learned[sn], skill_table[sn].name );
            }
        }

        for ( gn = 0; gn < MAX_GROUP; gn++ )
        {
            if ( group_table[gn].name != NULL && ch->pcdata->group_known[gn] )
            {
                fprintf( fp, "Gr '%s'\n", group_table[gn].name );
            }
        }
    }

    for ( paf = ch->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->type < 0 || paf->type >= MAX_SKILL )
            continue;

        fprintf( fp, "Affc '%s' %3d %3d %3d %3d %3d %10d\n",
                 skill_table[paf->type].name,
                 paf->where,
                 paf->level,
                 paf->duration, paf->modifier, paf->location, paf->bitvector );
    }

    fprintf( fp, "End\n\n" );
    return;
}

/* write a pet */
void fwrite_pet( CHAR_DATA *pet, FILE * fp )
{
    AFFECT_DATA            *paf = NULL;

    fprintf( fp, "#PET\n" );

    fprintf( fp, "Vnum %d\n", pet->pIndexData->vnum );

    fprintf( fp, "Name %s~\n", pet->name );
    fprintf( fp, "LogO %d\n", ( int ) current_time );
    if ( pet->short_descr != pet->pIndexData->short_descr )
        fprintf( fp, "ShD  %s~\n", pet->short_descr );
    if ( pet->long_descr != pet->pIndexData->long_descr )
        fprintf( fp, "LnD  %s~\n", pet->long_descr );
    if ( pet->description != pet->pIndexData->description )
        fprintf( fp, "Desc %s~\n", pet->description );
    if ( pet->race != pet->pIndexData->race )
        fprintf( fp, "Race %s~\n", race_table[pet->race].name );
    if ( pet->clan )
        fprintf( fp, "Clan %s~\n", clan_table[pet->clan].name );
    fprintf( fp, "Sex  %d\n", pet->sex );
    if ( pet->level != pet->pIndexData->level )
        fprintf( fp, "Levl %d\n", pet->level );
    fprintf( fp, "HMV  %d %d %d %d %d %d\n",
             pet->hit, pet->max_hit, pet->mana, pet->max_mana, pet->move, pet->max_move );
    if ( pet->gold > 0 )
        fprintf( fp, "Gold %d\n", pet->gold );
    if ( pet->silver > 0 )
        fprintf( fp, "Silv %d\n", pet->silver );
    if ( pet->exp > 0 )
        fprintf( fp, "Exp  %d\n", pet->exp );
    if ( pet->act != pet->pIndexData->act )
        fprintf( fp, "Act  %s\n", print_flags( pet->act ) );
    if ( pet->affected_by != pet->pIndexData->affected_by )
        fprintf( fp, "AfBy %s\n", print_flags( pet->affected_by ) );
    if ( pet->comm != 0 )
        fprintf( fp, "Comm %s\n", print_flags( pet->comm ) );
    fprintf( fp, "Pos  %d\n", pet->position =
             POS_FIGHTING ? POS_STANDING : pet->position );
    if ( pet->saving_throw != 0 )
        fprintf( fp, "Save %d\n", pet->saving_throw );
    if ( pet->alignment != pet->pIndexData->alignment )
        fprintf( fp, "Alig %d\n", pet->alignment );
    if ( pet->hitroll != pet->pIndexData->hitroll )
        fprintf( fp, "Hit  %d\n", pet->hitroll );
    if ( pet->damroll != pet->pIndexData->damage[DICE_BONUS] )
        fprintf( fp, "Dam  %d\n", pet->damroll );
    fprintf( fp, "ACs  %d %d %d %d\n",
             pet->armor[0], pet->armor[1], pet->armor[2], pet->armor[3] );
    fprintf( fp, "Attr %d %d %d %d %d\n",
             pet->perm_stat[STAT_STR], pet->perm_stat[STAT_INT],
             pet->perm_stat[STAT_WIS], pet->perm_stat[STAT_DEX],
             pet->perm_stat[STAT_CON] );
    fprintf( fp, "AMod %d %d %d %d %d\n",
             pet->mod_stat[STAT_STR], pet->mod_stat[STAT_INT],
             pet->mod_stat[STAT_WIS], pet->mod_stat[STAT_DEX], pet->mod_stat[STAT_CON] );

    for ( paf = pet->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->type < 0 || paf->type >= MAX_SKILL )
            continue;

        fprintf( fp, "Affc '%s' %3d %3d %3d %3d %3d %10d\n",
                 skill_table[paf->type].name,
                 paf->where, paf->level, paf->duration, paf->modifier, paf->location,
                 paf->bitvector );
    }

    fprintf( fp, "End\n" );
    return;
}

/*
 * Write an object and its contents.
 */
void fwrite_obj( CHAR_DATA *ch, OBJ_DATA *obj, FILE * fp, int iNest )
{
    EXTRA_DESCR_DATA       *ed = NULL;
    AFFECT_DATA            *paf = NULL;

    /*
     * Slick recursion to write lists backwards,
     *   so loading them will load in forwards order.
     */
    if ( obj->next_content != NULL )
        fwrite_obj( ch, obj->next_content, fp, iNest );

    /*
     * Castrate storage characters.
     */
    if ( ( ch->level < obj->level - 2 && obj->item_type != ITEM_CONTAINER )
         || obj->item_type == ITEM_KEY
         || ( obj->item_type == ITEM_MAP && !obj->value[0] ) )
        return;

    fprintf( fp, "#O\n" );
    fprintf( fp, "Vnum %d\n", obj->pIndexData->vnum );
    if ( !obj->pIndexData->new_format )
        fprintf( fp, "Oldstyle\n" );
    if ( obj->enchanted )
        fprintf( fp, "Enchanted\n" );
    fprintf( fp, "Nest %d\n", iNest );

    /*
     * these data are only used if they do not match the defaults 
     */

    if ( obj->name != obj->pIndexData->name )
        fprintf( fp, "Name %s~\n", obj->name );
    if ( obj->short_descr != obj->pIndexData->short_descr )
        fprintf( fp, "ShD  %s~\n", obj->short_descr );
    if ( obj->description != obj->pIndexData->description )
        fprintf( fp, "Desc %s~\n", obj->description );
    if ( obj->extra_flags != obj->pIndexData->extra_flags )
        fprintf( fp, "ExtF %d\n", obj->extra_flags );
    if ( obj->wear_flags != obj->pIndexData->wear_flags )
        fprintf( fp, "WeaF %d\n", obj->wear_flags );
    if ( obj->item_type != obj->pIndexData->item_type )
        fprintf( fp, "Ityp %d\n", obj->item_type );
    if ( obj->weight != obj->pIndexData->weight )
        fprintf( fp, "Wt   %d\n", obj->weight );
    if ( obj->condition != obj->pIndexData->condition )
        fprintf( fp, "Cond %d\n", obj->condition );

    /*
     * variable data 
     */

    fprintf( fp, "Wear %d\n", obj->wear_loc );
    if ( obj->level != obj->pIndexData->level )
        fprintf( fp, "Lev  %d\n", obj->level );
    if ( obj->timer != 0 )
        fprintf( fp, "Time %d\n", obj->timer );
    fprintf( fp, "Cost %d\n", obj->cost );
    if ( obj->value[0] != obj->pIndexData->value[0]
         || obj->value[1] != obj->pIndexData->value[1]
         || obj->value[2] != obj->pIndexData->value[2]
         || obj->value[3] != obj->pIndexData->value[3]
         || obj->value[4] != obj->pIndexData->value[4] )
        fprintf( fp, "Val  %d %d %d %d %d\n",
                 obj->value[0], obj->value[1], obj->value[2], obj->value[3],
                 obj->value[4] );

    switch ( obj->item_type )
    {
        case ITEM_POTION:
        case ITEM_SCROLL:
        case ITEM_PILL:
            if ( obj->value[1] > 0 )
            {
                fprintf( fp, "Spell 1 '%s'\n", skill_table[obj->value[1]].name );
            }

            if ( obj->value[2] > 0 )
            {
                fprintf( fp, "Spell 2 '%s'\n", skill_table[obj->value[2]].name );
            }

            if ( obj->value[3] > 0 )
            {
                fprintf( fp, "Spell 3 '%s'\n", skill_table[obj->value[3]].name );
            }

            break;

        case ITEM_STAFF:
        case ITEM_WAND:
            if ( obj->value[3] > 0 )
            {
                fprintf( fp, "Spell 3 '%s'\n", skill_table[obj->value[3]].name );
            }

            break;
    }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->type < 0 || paf->type >= MAX_SKILL )
            continue;
        fprintf( fp, "Affc '%s' %3d %3d %3d %3d %3d %10d\n",
                 skill_table[paf->type].name,
                 paf->where,
                 paf->level,
                 paf->duration, paf->modifier, paf->location, paf->bitvector );
    }

    for ( ed = obj->extra_descr; ed != NULL; ed = ed->next )
    {
        fprintf( fp, "ExDe %s~ %s~\n", ed->keyword, ed->description );
    }

    fprintf( fp, "End\n\n" );

    if ( obj->contains != NULL )
        fwrite_obj( ch, obj->contains, fp, iNest + 1 );

    return;
}

/*
 * Load a char and inventory into a new ch structure.
 */
bool load_char_obj( DESCRIPTOR_DATA *d, const char *name )
{
    char                    strsave[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *ch = NULL;
    FILE                   *fp = NULL;
    bool                    found = false;
    bool                    god = false;

    ch = new_char(  );
    ch->pcdata = new_pcdata(  );

    d->character = ch;
    ch->desc = d;
    ch->name = str_dup( name );

    init_player( ch );

    /*
     * Attempt to load their immortal version, first
     */
    sprintf( strsave, "%s/%c/%s", GOD_DIR, LOWER( name[0] ), capitalize( name ) );
    if ( ( fp = fopen( strsave, "r" ) ) != NULL )
    {
        found = true;
        god = true;
        load_player_sections( ch, fp );
        fclose( fp );
    }
    else
    {
        /*
         * Then, if that failed, try loading their mortal selves.
         */
        sprintf( strsave, "%s/%c/%s", PLAYER_DIR, LOWER( name[0] ), capitalize( name ) );
        if ( ( fp = fopen( strsave, "r" ) ) != NULL )
        {
            found = true;
            god = false;
            load_player_sections( ch, fp );
            fclose( fp );
        }
    }

    if ( found )
        upgrade_player_allver( ch );
    if ( found && ch->version < 2 )                    /* need to add the new skills */
        upgrade_player_v2( ch );
    if ( found && ch->version < 3 )                    /* fix immortal levels */
        upgrade_player_v3( ch );
    if ( found && ch->version < 4 )                    /* fix gold */
        upgrade_player_v4( ch );

    /*
     * If they were loaded from the mortal side, make them mortal!
     */
    if ( found && !god )
    {
        ch->level = UMIN( ch->level, LEVEL_HERO );
        ch->trust = UMIN( ch->trust, LEVEL_HERO );
    }

    return found;
}

void init_player( CHAR_DATA *ch )
{
    int                     stat = 0;
    int                     iNest = 0;

    ch->id = get_pc_id(  );
    ch->race = race_lookup( "human" );
    ch->iclass = class_lookup( "warrior" );;
    ch->level = 1;
    ch->act = PLR_NOSUMMON;
    ch->comm = COMM_COMBINE | COMM_PROMPT;
    ch->prompt = str_dup( "<%hhp %mm %vmv> " );
    ch->pcdata->confirm_delete = false;
    ch->pcdata->pwd = str_dup( "" );
    ch->pcdata->bamfin = str_dup( "" );
    ch->pcdata->bamfout = str_dup( "" );
    ch->pcdata->title = str_dup( "" );
    for ( stat = 0; stat < MAX_STATS; stat++ )
        ch->perm_stat[stat] = 13;
    ch->pcdata->condition[COND_THIRST] = 48;
    ch->pcdata->condition[COND_FULL] = 48;
    ch->pcdata->condition[COND_HUNGER] = 48;
    ch->pcdata->security = 0;                          /* OLC */
    for ( iNest = 0; iNest < MAX_NEST; iNest++ )
        rgObjNest[iNest] = NULL;
}

void load_player_sections( CHAR_DATA *ch, FILE * fp )
{
    char                    letter = '\0';
    char                   *word = NULL;

    for ( ;; )
    {
        letter = fread_letter( fp );
        if ( letter == '*' )
        {
            fread_to_eol( fp );
            continue;
        }

        if ( letter != '#' )
        {
            log_error( "Expected #, found '%c'", letter );
            break;
        }

        word = fread_word( fp );
        if ( !str_cmp( word, "PLAYER" ) )
            fread_char( ch, fp );
        else if ( !str_cmp( word, "OBJECT" ) )
            fread_obj( ch, fp );
        else if ( !str_cmp( word, "O" ) )
            fread_obj( ch, fp );
        else if ( !str_cmp( word, "PET" ) )
            fread_pet( ch, fp );
        else if ( !str_cmp( word, "END" ) )
            break;
        else
        {
            log_error( "Bad section header %s", word );
            break;
        }
    }
}

void upgrade_player_allver( CHAR_DATA *ch )
{
    int                     i = 0;

    /*
     * initialize race 
     */
    if ( ch->race == 0 )
        ch->race = race_lookup( "human" );

    ch->size = pc_race_table[ch->race].size;
    ch->dam_type = 17;                                 /* punch */

    for ( i = 0; i < 5; i++ )
    {
        if ( pc_race_table[ch->race].skills[i] == NULL )
            break;
        group_add( ch, pc_race_table[ch->race].skills[i], false );
    }
    ch->affected_by = ch->affected_by | race_table[ch->race].aff;
    ch->imm_flags = ch->imm_flags | race_table[ch->race].imm;
    ch->res_flags = ch->res_flags | race_table[ch->race].res;
    ch->vuln_flags = ch->vuln_flags | race_table[ch->race].vuln;
    ch->form = race_table[ch->race].form;
    ch->parts = race_table[ch->race].parts;
}

void upgrade_player_v2( CHAR_DATA *ch )
{
    /*
     * RT initialize skills 
     */
    group_add( ch, "rom basics", false );
    group_add( ch, class_table[ch->iclass].base_group, false );
    group_add( ch, class_table[ch->iclass].default_group, true );
    ch->pcdata->learned[skill_lookup( "recall" )] = 50;
}

void upgrade_player_v3( CHAR_DATA *ch )
{
    /*
     * fix levels 
     */
    if ( ch->level > 35 || ch->trust > 35 )
    {
        switch ( ch->level )
        {
            case ( 40 ):
                ch->level = 60;
                break;                                 /* imp -> imp */
            case ( 39 ):
                ch->level = 58;
                break;                                 /* god -> supreme */
            case ( 38 ):
                ch->level = 56;
                break;                                 /* deity -> god */
            case ( 37 ):
                ch->level = 53;
                break;                                 /* angel -> demigod */
        }
        switch ( ch->trust )
        {
            case ( 40 ):
                ch->trust = 60;
                break;                                 /* imp -> imp */
            case ( 39 ):
                ch->trust = 58;
                break;                                 /* god -> supreme */
            case ( 38 ):
                ch->trust = 56;
                break;                                 /* deity -> god */
            case ( 37 ):
                ch->trust = 53;
                break;                                 /* angel -> demigod */
            case ( 36 ):
                ch->trust = 51;
                break;                                 /* hero -> hero */
        }
    }
}

void upgrade_player_v4( CHAR_DATA *ch )
{
    /*
     * ream gold 
     */
    ch->gold /= 100;
}

/*
 * Read in a char.
 */
void fread_char( CHAR_DATA *ch, FILE * fp )
{
    static char             end[MAX_INPUT_LENGTH] = "End";
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                   *word = NULL;
    bool                    fMatch = false;
    int                     count = 0;
    int                     lastlogoff = current_time;
    int                     percent = 0;

    log_auth( ch, "Loading %s.", NAME( ch ) );

    for ( ;; )
    {
        word = ( char * ) ( feof( fp ) ? end : fread_word( fp ) );
        fMatch = false;

        switch ( UPPER( word[0] ) )
        {
            case '*':
                fMatch = true;
                fread_to_eol( fp );
                break;

            case 'A':
                KEY( "Act", ch->act, fread_flag( fp ) );
                KEY( "AffectedBy", ch->affected_by, fread_flag( fp ) );
                KEY( "AfBy", ch->affected_by, fread_flag( fp ) );
                KEY( "Alignment", ch->alignment, fread_number( fp ) );
                KEY( "Alig", ch->alignment, fread_number( fp ) );

                if ( !str_cmp( word, "Alia" ) )
                {
                    if ( count >= MAX_ALIAS )
                    {
                        fread_to_eol( fp );
                        fMatch = true;
                        break;
                    }

                    ch->pcdata->alias[count] = str_dup( fread_word( fp ) );
                    ch->pcdata->alias_sub[count] = str_dup( fread_word( fp ) );
                    count++;
                    fMatch = true;
                    break;
                }

                if ( !str_cmp( word, "Alias" ) )
                {
                    if ( count >= MAX_ALIAS )
                    {
                        fread_to_eol( fp );
                        fMatch = true;
                        break;
                    }

                    ch->pcdata->alias[count] = str_dup( fread_word( fp ) );
                    ch->pcdata->alias_sub[count] = fread_string( fp );
                    count++;
                    fMatch = true;
                    break;
                }

                if ( !str_cmp( word, "AC" ) || !str_cmp( word, "Armor" ) )
                {
                    fread_to_eol( fp );
                    fMatch = true;
                    break;
                }

                if ( !str_cmp( word, "ACs" ) )
                {
                    int                     i;

                    for ( i = 0; i < 4; i++ )
                        ch->armor[i] = fread_number( fp );
                    fMatch = true;
                    break;
                }

                if ( !str_cmp( word, "AffD" ) )
                {
                    AFFECT_DATA            *paf = NULL;
                    int                     sn = 0;

                    paf = new_affect(  );

                    sn = skill_lookup( fread_word( fp ) );
                    if ( sn < 0 )
                        log_error( "Unknown skill %d", sn );
                    else
                        paf->type = sn;

                    paf->level = fread_number( fp );
                    paf->duration = fread_number( fp );
                    paf->modifier = fread_number( fp );
                    paf->location = fread_number( fp );
                    paf->bitvector = fread_number( fp );
                    paf->next = ch->affected;
                    ch->affected = paf;
                    fMatch = true;
                    break;
                }

                if ( !str_cmp( word, "Affc" ) )
                {
                    AFFECT_DATA            *paf = NULL;
                    int                     sn = 0;

                    paf = new_affect(  );

                    sn = skill_lookup( fread_word( fp ) );
                    if ( sn < 0 )
                        log_error( "Unknown skill %d", sn );
                    else
                        paf->type = sn;

                    paf->where = fread_number( fp );
                    paf->level = fread_number( fp );
                    paf->duration = fread_number( fp );
                    paf->modifier = fread_number( fp );
                    paf->location = fread_number( fp );
                    paf->bitvector = fread_number( fp );
                    paf->next = ch->affected;
                    ch->affected = paf;
                    fMatch = true;
                    break;
                }

                if ( !str_cmp( word, "AttrMod" ) || !str_cmp( word, "AMod" ) )
                {
                    int                     stat = 0;

                    for ( stat = 0; stat < MAX_STATS; stat++ )
                        ch->mod_stat[stat] = fread_number( fp );
                    fMatch = true;
                    break;
                }

                if ( !str_cmp( word, "AttrPerm" ) || !str_cmp( word, "Attr" ) )
                {
                    int                     stat = 0;

                    for ( stat = 0; stat < MAX_STATS; stat++ )
                        ch->perm_stat[stat] = fread_number( fp );
                    fMatch = true;
                    break;
                }
                break;

            case 'B':
                KEY( "Bamfin", ch->pcdata->bamfin, fread_string( fp ) );
                KEY( "Bamfout", ch->pcdata->bamfout, fread_string( fp ) );
                KEY( "Bin", ch->pcdata->bamfin, fread_string( fp ) );
                KEY( "Bout", ch->pcdata->bamfout, fread_string( fp ) );
                break;

            case 'C':
                KEY( "Class", ch->iclass, fread_number( fp ) );
                KEY( "Cla", ch->iclass, fread_number( fp ) );
                KEY( "Clan", ch->clan, clan_lookup( fread_string( fp ) ) );

                if ( !str_cmp( word, "Condition" ) || !str_cmp( word, "Cond" ) )
                {
                    ch->pcdata->condition[0] = fread_number( fp );
                    ch->pcdata->condition[1] = fread_number( fp );
                    ch->pcdata->condition[2] = fread_number( fp );
                    fMatch = true;
                    break;
                }
                if ( !str_cmp( word, "Cnd" ) )
                {
                    ch->pcdata->condition[0] = fread_number( fp );
                    ch->pcdata->condition[1] = fread_number( fp );
                    ch->pcdata->condition[2] = fread_number( fp );
                    ch->pcdata->condition[3] = fread_number( fp );
                    fMatch = true;
                    break;
                }
                KEY( "Comm", ch->comm, fread_flag( fp ) );

                break;

            case 'D':
                KEY( "Damroll", ch->damroll, fread_number( fp ) );
                KEY( "Dam", ch->damroll, fread_number( fp ) );
                KEY( "Description", ch->description, fread_string( fp ) );
                KEY( "Desc", ch->description, fread_string( fp ) );
                break;

            case 'E':
                if ( !str_cmp( word, "End" ) )
                {
                    /*
                     * adjust hp mana move up -- here for speed's sake 
                     */
                    percent = ( current_time - lastlogoff ) * 25 / ( 2 * 60 * 60 );

                    percent = UMIN( percent, 100 );

                    if ( percent > 0 && !IS_AFFECTED( ch, AFF_POISON )
                         && !IS_AFFECTED( ch, AFF_PLAGUE ) )
                    {
                        ch->hit += ( ch->max_hit - ch->hit ) * percent / 100;
                        ch->mana += ( ch->max_mana - ch->mana ) * percent / 100;
                        ch->move += ( ch->max_move - ch->move ) * percent / 100;
                    }
                    return;
                }
                KEY( "Exp", ch->exp, fread_number( fp ) );
                break;

            case 'G':
                KEY( "Gold", ch->gold, fread_number( fp ) );
                if ( !str_cmp( word, "Group" ) || !str_cmp( word, "Gr" ) )
                {
                    int                     gn = 0;
                    char                   *temp = NULL;

                    temp = fread_word( fp );
                    gn = group_lookup( temp );
                    /*
                     * gn = group_lookup( fread_word( fp ) ); 
                     */
                    if ( gn < 0 )
                    {
                        log_error( "Invalid group \"%s\"", temp );
                    }
                    else
                        gn_add( ch, gn );
                    fMatch = true;
                }
                break;

            case 'H':
                KEY( "Hitroll", ch->hitroll, fread_number( fp ) );
                KEY( "Hit", ch->hitroll, fread_number( fp ) );

                if ( !str_cmp( word, "HpManaMove" ) || !str_cmp( word, "HMV" ) )
                {
                    ch->hit = fread_number( fp );
                    ch->max_hit = fread_number( fp );
                    ch->mana = fread_number( fp );
                    ch->max_mana = fread_number( fp );
                    ch->move = fread_number( fp );
                    ch->max_move = fread_number( fp );
                    fMatch = true;
                    break;
                }

                if ( !str_cmp( word, "HpManaMovePerm" ) || !str_cmp( word, "HMVP" ) )
                {
                    ch->pcdata->perm_hit = fread_number( fp );
                    ch->pcdata->perm_mana = fread_number( fp );
                    ch->pcdata->perm_move = fread_number( fp );
                    fMatch = true;
                    break;
                }

                break;

            case 'I':
                KEY( "Id", ch->id, fread_number( fp ) );
                KEY( "InvisLevel", ch->invis_level, fread_number( fp ) );
                KEY( "Inco", ch->incog_level, fread_number( fp ) );
                KEY( "Invi", ch->invis_level, fread_number( fp ) );
                break;

            case 'L':
                KEY( "LastLevel", ch->pcdata->last_level, fread_number( fp ) );
                KEY( "LLev", ch->pcdata->last_level, fread_number( fp ) );
                KEY( "Level", ch->level, fread_number( fp ) );
                KEY( "Lev", ch->level, fread_number( fp ) );
                KEY( "Levl", ch->level, fread_number( fp ) );
                KEY( "LogO", lastlogoff, fread_number( fp ) );
                KEY( "LongDescr", ch->long_descr, fread_string( fp ) );
                KEY( "LnD", ch->long_descr, fread_string( fp ) );
                break;

            case 'N':
                KEYS( "Name", ch->name, fread_string( fp ) );
                KEY( "Note", ch->pcdata->last_note, fread_number( fp ) );
                if ( !str_cmp( word, "Not" ) )
                {
                    ch->pcdata->last_note = fread_number( fp );
                    ch->pcdata->last_idea = fread_number( fp );
                    ch->pcdata->last_penalty = fread_number( fp );
                    ch->pcdata->last_news = fread_number( fp );
                    ch->pcdata->last_changes = fread_number( fp );
                    fMatch = true;
                    break;
                }
                break;

            case 'P':
                KEY( "Password", ch->pcdata->pwd, fread_string( fp ) );
                KEY( "Pass", ch->pcdata->pwd, fread_string( fp ) );
                KEY( "Played", ch->played, fread_number( fp ) );
                KEY( "Plyd", ch->played, fread_number( fp ) );
                KEY( "Points", ch->pcdata->points, fread_number( fp ) );
                KEY( "Pnts", ch->pcdata->points, fread_number( fp ) );
                KEY( "Position", ch->position, fread_number( fp ) );
                KEY( "Pos", ch->position, fread_number( fp ) );
                KEY( "Practice", ch->practice, fread_number( fp ) );
                KEY( "Prac", ch->practice, fread_number( fp ) );
                KEYS( "Prompt", ch->prompt, fread_string( fp ) );
                KEYS( "Prom", ch->prompt, fread_string( fp ) );
                break;

            case 'R':
                KEY( "Race", ch->race, race_lookup( fread_string( fp ) ) );

                if ( !str_cmp( word, "Room" ) )
                {
                    ch->in_room = get_room_index( fread_number( fp ) );
                    if ( ch->in_room == NULL )
                        ch->in_room = get_room_index( ROOM_VNUM_LIMBO );
                    fMatch = true;
                    break;
                }

                break;

            case 'S':
                KEY( "SavingThrow", ch->saving_throw, fread_number( fp ) );
                KEY( "Save", ch->saving_throw, fread_number( fp ) );
                KEY( "Scro", ch->lines, fread_number( fp ) );
                KEY( "Sex", ch->sex, fread_number( fp ) );
                KEY( "ShortDescr", ch->short_descr, fread_string( fp ) );
                KEY( "ShD", ch->short_descr, fread_string( fp ) );
                KEY( "Sec", ch->pcdata->security, fread_number( fp ) ); /* OLC */
                KEY( "Silv", ch->silver, fread_number( fp ) );

                if ( !str_cmp( word, "Skill" ) || !str_cmp( word, "Sk" ) )
                {
                    int                     sn = 0;
                    int                     value = 0;
                    char                   *temp = NULL;

                    value = fread_number( fp );
                    temp = fread_word( fp );
                    sn = skill_lookup( temp );
                    /*
                     * sn = skill_lookup( fread_word( fp ) ); 
                     */
                    if ( sn < 0 )
                    {
                        log_error( "Unknown skill \"%s\"", temp );
                    }
                    else
                        ch->pcdata->learned[sn] = value;
                    fMatch = true;
                }

                break;

            case 'T':
                KEY( "TrueSex", ch->pcdata->true_sex, fread_number( fp ) );
                KEY( "TSex", ch->pcdata->true_sex, fread_number( fp ) );
                KEY( "Trai", ch->train, fread_number( fp ) );
                KEY( "Trust", ch->trust, fread_number( fp ) );
                KEY( "Tru", ch->trust, fread_number( fp ) );

                if ( !str_cmp( word, "Title" ) || !str_cmp( word, "Titl" ) )
                {
                    ch->pcdata->title = fread_string( fp );
                    if ( ch->pcdata->title[0] != '.' && ch->pcdata->title[0] != ','
                         && ch->pcdata->title[0] != '!' && ch->pcdata->title[0] != '?' )
                    {
                        sprintf( buf, " %s", ch->pcdata->title );
                        free_string( ch->pcdata->title );
                        ch->pcdata->title = str_dup( buf );
                    }
                    fMatch = true;
                    break;
                }

                break;

            case 'V':
                KEY( "Version", ch->version, fread_number( fp ) );
                KEY( "Vers", ch->version, fread_number( fp ) );
                if ( !str_cmp( word, "Vnum" ) )
                {
                    ch->pIndexData = get_mob_index( fread_number( fp ) );
                    fMatch = true;
                    break;
                }
                break;

            case 'W':
                KEY( "Wimpy", ch->wimpy, fread_number( fp ) );
                KEY( "Wimp", ch->wimpy, fread_number( fp ) );
                KEY( "Wizn", ch->wiznet, fread_flag( fp ) );
                break;
        }

        if ( !fMatch )
        {
            log_error( "%s", "No match" );
            fread_to_eol( fp );
        }
    }
}

/* load a pet from the forgotten reaches */
void fread_pet( CHAR_DATA *ch, FILE * fp )
{
    static char             end[MAX_INPUT_LENGTH] = "END";
    char                   *word = NULL;
    CHAR_DATA              *pet = NULL;
    bool                    fMatch = false;
    int                     lastlogoff = current_time;
    int                     percent = 0;

    /*
     * first entry had BETTER be the vnum or we barf 
     */
    word = ( char * ) ( feof( fp ) ? end : fread_word( fp ) );
    if ( !str_cmp( word, "Vnum" ) )
    {
        int                     vnum = 0;

        vnum = fread_number( fp );
        if ( get_mob_index( vnum ) == NULL )
        {
            log_error( "Bad pet vnum %d", vnum );
            pet = create_mobile( get_mob_index( MOB_VNUM_FIDO ) );
        }
        else
            pet = create_mobile( get_mob_index( vnum ) );
    }
    else
    {
        log_error( "%s", "No pet vnum in file" );
        pet = create_mobile( get_mob_index( MOB_VNUM_FIDO ) );
    }

    for ( ;; )
    {
        word = ( char * ) ( feof( fp ) ? end : fread_word( fp ) );
        fMatch = false;

        switch ( UPPER( word[0] ) )
        {
            case '*':
                fMatch = true;
                fread_to_eol( fp );
                break;

            case 'A':
                KEY( "Act", pet->act, fread_flag( fp ) );
                KEY( "AfBy", pet->affected_by, fread_flag( fp ) );
                KEY( "Alig", pet->alignment, fread_number( fp ) );

                if ( !str_cmp( word, "ACs" ) )
                {
                    int                     i = 0;

                    for ( i = 0; i < 4; i++ )
                        pet->armor[i] = fread_number( fp );
                    fMatch = true;
                    break;
                }

                if ( !str_cmp( word, "AffD" ) )
                {
                    AFFECT_DATA            *paf = NULL;
                    int                     sn = 0;

                    paf = new_affect(  );
                    sn = skill_lookup( fread_word( fp ) );
                    if ( sn < 0 )
                        log_error( "Unknown skill %d", sn );
                    else
                        paf->type = sn;

                    paf->level = fread_number( fp );
                    paf->duration = fread_number( fp );
                    paf->modifier = fread_number( fp );
                    paf->location = fread_number( fp );
                    paf->bitvector = fread_number( fp );
                    paf->next = pet->affected;
                    pet->affected = paf;
                    fMatch = true;
                    break;
                }

                if ( !str_cmp( word, "Affc" ) )
                {
                    AFFECT_DATA            *paf = NULL;
                    int                     sn = 0;

                    paf = new_affect(  );
                    sn = skill_lookup( fread_word( fp ) );
                    if ( sn < 0 )
                        log_error( "Unknown skill %d", sn );
                    else
                        paf->type = sn;

                    paf->where = fread_number( fp );
                    paf->level = fread_number( fp );
                    paf->duration = fread_number( fp );
                    paf->modifier = fread_number( fp );
                    paf->location = fread_number( fp );
                    paf->bitvector = fread_number( fp );
                    paf->next = pet->affected;
                    pet->affected = paf;
                    fMatch = true;
                    break;
                }

                if ( !str_cmp( word, "AMod" ) )
                {
                    int                     stat = 0;

                    for ( stat = 0; stat < MAX_STATS; stat++ )
                        pet->mod_stat[stat] = fread_number( fp );
                    fMatch = true;
                    break;
                }

                if ( !str_cmp( word, "Attr" ) )
                {
                    int                     stat = 0;

                    for ( stat = 0; stat < MAX_STATS; stat++ )
                        pet->perm_stat[stat] = fread_number( fp );
                    fMatch = true;
                    break;
                }
                break;

            case 'C':
                KEY( "Clan", pet->clan, clan_lookup( fread_string( fp ) ) );
                KEY( "Comm", pet->comm, fread_flag( fp ) );
                break;

            case 'D':
                KEY( "Dam", pet->damroll, fread_number( fp ) );
                KEY( "Desc", pet->description, fread_string( fp ) );
                break;

            case 'E':
                if ( !str_cmp( word, "End" ) )
                {
                    pet->leader = ch;
                    pet->master = ch;
                    ch->pet = pet;
                    /*
                     * adjust hp mana move up -- here for speed's sake 
                     */
                    percent = ( current_time - lastlogoff ) * 25 / ( 2 * 60 * 60 );

                    if ( percent > 0 && !IS_AFFECTED( ch, AFF_POISON )
                         && !IS_AFFECTED( ch, AFF_PLAGUE ) )
                    {
                        percent = UMIN( percent, 100 );
                        pet->hit += ( pet->max_hit - pet->hit ) * percent / 100;
                        pet->mana += ( pet->max_mana - pet->mana ) * percent / 100;
                        pet->move += ( pet->max_move - pet->move ) * percent / 100;
                    }
                    return;
                }
                KEY( "Exp", pet->exp, fread_number( fp ) );
                break;

            case 'G':
                KEY( "Gold", pet->gold, fread_number( fp ) );
                break;

            case 'H':
                KEY( "Hit", pet->hitroll, fread_number( fp ) );

                if ( !str_cmp( word, "HMV" ) )
                {
                    pet->hit = fread_number( fp );
                    pet->max_hit = fread_number( fp );
                    pet->mana = fread_number( fp );
                    pet->max_mana = fread_number( fp );
                    pet->move = fread_number( fp );
                    pet->max_move = fread_number( fp );
                    fMatch = true;
                    break;
                }
                break;

            case 'L':
                KEY( "Levl", pet->level, fread_number( fp ) );
                KEY( "LnD", pet->long_descr, fread_string( fp ) );
                KEY( "LogO", lastlogoff, fread_number( fp ) );
                break;

            case 'N':
                KEY( "Name", pet->name, fread_string( fp ) );
                break;

            case 'P':
                KEY( "Pos", pet->position, fread_number( fp ) );
                break;

            case 'R':
                KEY( "Race", pet->race, race_lookup( fread_string( fp ) ) );
                break;

            case 'S':
                KEY( "Save", pet->saving_throw, fread_number( fp ) );
                KEY( "Sex", pet->sex, fread_number( fp ) );
                KEY( "ShD", pet->short_descr, fread_string( fp ) );
                KEY( "Silv", pet->silver, fread_number( fp ) );
                break;

                if ( !fMatch )
                {
                    log_error( "%s", "No match" );
                    fread_to_eol( fp );
                }

        }
    }
}

void fread_obj( CHAR_DATA *ch, FILE * fp )
{
    static char             end[MAX_INPUT_LENGTH] = "End";
    OBJ_DATA               *obj = NULL;
    char                   *word = NULL;
    int                     iNest = 0;
    bool                    fMatch = false;
    bool                    fNest = false;
    bool                    fVnum = false;
    bool                    first = false;             /* used to counter fp offset */
    bool                    new_format = false;        /* to prevent errors */
    bool                    make_new = false;          /* update object */

    word = ( char * ) ( feof( fp ) ? end : fread_word( fp ) );
    if ( !str_cmp( word, "Vnum" ) )
    {
        int                     vnum = 0;

        first = false;                                 /* fp will be in right place */

        vnum = fread_number( fp );
        if ( get_obj_index( vnum ) == NULL )
        {
            log_error( "Bad object vnum %d", vnum );
        }
        else
        {
            obj = create_object( get_obj_index( vnum ), -1 );
            new_format = true;
        }

    }

    if ( obj == NULL )                                 /* either not found or old style */
    {
        obj = new_obj(  );
        obj->name = str_dup( "" );
        obj->short_descr = str_dup( "" );
        obj->description = str_dup( "" );
    }

    fNest = false;
    fVnum = true;
    iNest = 0;

    for ( ;; )
    {
        if ( first )
            first = false;
        else
            word = ( char * ) ( feof( fp ) ? end : fread_word( fp ) );
        fMatch = false;

        switch ( UPPER( word[0] ) )
        {
            case '*':
                fMatch = true;
                fread_to_eol( fp );
                break;

            case 'A':
                if ( !str_cmp( word, "AffD" ) )
                {
                    AFFECT_DATA            *paf = NULL;
                    int                     sn = 0;

                    paf = new_affect(  );
                    sn = skill_lookup( fread_word( fp ) );
                    if ( sn < 0 )
                        log_error( "Unknown skill %d", sn );
                    else
                        paf->type = sn;

                    paf->level = fread_number( fp );
                    paf->duration = fread_number( fp );
                    paf->modifier = fread_number( fp );
                    paf->location = fread_number( fp );
                    paf->bitvector = fread_number( fp );
                    paf->next = obj->affected;
                    obj->affected = paf;
                    fMatch = true;
                    break;
                }
                if ( !str_cmp( word, "Affc" ) )
                {
                    AFFECT_DATA            *paf = NULL;
                    int                     sn = 0;

                    paf = new_affect(  );
                    sn = skill_lookup( fread_word( fp ) );
                    if ( sn < 0 )
                        log_error( "Unknown skill %d", sn );
                    else
                        paf->type = sn;

                    paf->where = fread_number( fp );
                    paf->level = fread_number( fp );
                    paf->duration = fread_number( fp );
                    paf->modifier = fread_number( fp );
                    paf->location = fread_number( fp );
                    paf->bitvector = fread_number( fp );
                    paf->next = obj->affected;
                    obj->affected = paf;
                    fMatch = true;
                    break;
                }
                break;

            case 'C':
                KEY( "Cond", obj->condition, fread_number( fp ) );
                KEY( "Cost", obj->cost, fread_number( fp ) );
                break;

            case 'D':
                KEY( "Description", obj->description, fread_string( fp ) );
                KEY( "Desc", obj->description, fread_string( fp ) );
                break;

            case 'E':

                if ( !str_cmp( word, "Enchanted" ) )
                {
                    obj->enchanted = true;
                    fMatch = true;
                    break;
                }

                KEY( "ExtraFlags", obj->extra_flags, fread_number( fp ) );
                KEY( "ExtF", obj->extra_flags, fread_number( fp ) );

                if ( !str_cmp( word, "ExtraDescr" ) || !str_cmp( word, "ExDe" ) )
                {
                    EXTRA_DESCR_DATA       *ed = NULL;

                    ed = new_extra_descr(  );

                    ed->keyword = fread_string( fp );
                    ed->description = fread_string( fp );
                    ed->next = obj->extra_descr;
                    obj->extra_descr = ed;
                    fMatch = true;
                }

                if ( !str_cmp( word, "End" ) )
                {
                    if ( !fNest || ( fVnum && obj->pIndexData == NULL ) )
                    {
                        log_error( "%s", "Incomplete object" );
                        free_obj( obj );
                        return;
                    }
                    else
                    {
                        if ( !fVnum )
                        {
                            log_info( "Replacing invalid object with dummy object" );
                            free_obj( obj );
                            obj = create_object( get_obj_index( OBJ_VNUM_DUMMY ), 0 );
                        }

                        if ( !new_format )
                        {
                            obj->next = object_list;
                            object_list = obj;
                            obj->pIndexData->count++;
                        }

                        if ( !obj->pIndexData->new_format
                             && obj->item_type == ITEM_ARMOR && obj->value[1] == 0 )
                        {
                            obj->value[1] = obj->value[0];
                            obj->value[2] = obj->value[0];
                        }
                        if ( make_new )
                        {
                            int                     wear = 0;

                            wear = obj->wear_loc;
                            extract_obj( obj );

                            obj = create_object( obj->pIndexData, 0 );
                            obj->wear_loc = wear;
                        }
                        if ( iNest == 0 || rgObjNest[iNest] == NULL )
                            obj_to_char( obj, ch );
                        else
                            obj_to_obj( obj, rgObjNest[iNest - 1] );
                        return;
                    }
                }
                break;

            case 'I':
                KEY( "ItemType", obj->item_type, fread_number( fp ) );
                KEY( "Ityp", obj->item_type, fread_number( fp ) );
                break;

            case 'L':
                KEY( "Level", obj->level, fread_number( fp ) );
                KEY( "Lev", obj->level, fread_number( fp ) );
                break;

            case 'N':
                KEY( "Name", obj->name, fread_string( fp ) );

                if ( !str_cmp( word, "Nest" ) )
                {
                    iNest = fread_number( fp );
                    if ( iNest < 0 || iNest >= MAX_NEST )
                    {
                        log_error( "Bad nesting %d", iNest );
                    }
                    else
                    {
                        rgObjNest[iNest] = obj;
                        fNest = true;
                    }
                    fMatch = true;
                }
                break;

            case 'O':
                if ( !str_cmp( word, "Oldstyle" ) )
                {
                    if ( obj->pIndexData != NULL && obj->pIndexData->new_format )
                        make_new = true;
                    fMatch = true;
                }
                break;

            case 'S':
                KEY( "ShortDescr", obj->short_descr, fread_string( fp ) );
                KEY( "ShD", obj->short_descr, fread_string( fp ) );

                if ( !str_cmp( word, "Spell" ) )
                {
                    int                     iValue = 0;
                    int                     sn = 0;

                    iValue = fread_number( fp );
                    sn = skill_lookup( fread_word( fp ) );
                    if ( iValue < 0 || iValue > 3 )
                    {
                        log_error( "Bad iValue %d", iValue );
                    }
                    else if ( sn < 0 )
                    {
                        log_error( "Unknown skill %d", sn );
                    }
                    else
                    {
                        obj->value[iValue] = sn;
                    }
                    fMatch = true;
                    break;
                }

                break;

            case 'T':
                KEY( "Timer", obj->timer, fread_number( fp ) );
                KEY( "Time", obj->timer, fread_number( fp ) );
                break;

            case 'V':
                if ( !str_cmp( word, "Values" ) || !str_cmp( word, "Vals" ) )
                {
                    obj->value[0] = fread_number( fp );
                    obj->value[1] = fread_number( fp );
                    obj->value[2] = fread_number( fp );
                    obj->value[3] = fread_number( fp );
                    if ( obj->item_type == ITEM_WEAPON && obj->value[0] == 0 )
                        obj->value[0] = obj->pIndexData->value[0];
                    fMatch = true;
                    break;
                }

                if ( !str_cmp( word, "Val" ) )
                {
                    obj->value[0] = fread_number( fp );
                    obj->value[1] = fread_number( fp );
                    obj->value[2] = fread_number( fp );
                    obj->value[3] = fread_number( fp );
                    obj->value[4] = fread_number( fp );
                    fMatch = true;
                    break;
                }

                if ( !str_cmp( word, "Vnum" ) )
                {
                    int                     vnum = 0;

                    vnum = fread_number( fp );
                    if ( ( obj->pIndexData = get_obj_index( vnum ) ) == NULL )
                        log_error( "Bad vnum %d", vnum );
                    else
                        fVnum = true;
                    fMatch = true;
                    break;
                }
                break;

            case 'W':
                KEY( "WearFlags", obj->wear_flags, fread_number( fp ) );
                KEY( "WeaF", obj->wear_flags, fread_number( fp ) );
                KEY( "WearLoc", obj->wear_loc, fread_number( fp ) );
                KEY( "Wear", obj->wear_loc, fread_number( fp ) );
                KEY( "Weight", obj->weight, fread_number( fp ) );
                KEY( "Wt", obj->weight, fread_number( fp ) );
                break;

        }

        if ( !fMatch )
        {
            log_error( "%s", "No match" );
            fread_to_eol( fp );
        }
    }
}
