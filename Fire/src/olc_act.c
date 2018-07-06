/*
 * RAM $Id: olc_act.c 85 2009-02-08 17:09:18Z ram $
 */

/***************************************************************************
 *  File: olc_act.c                                                        *
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
#include "interp.h"
#include "act.h"
#include "magic.h"
#include "tables.h"
#include "special.h"
#include "strings.h"
#include "random.h"
#include "olc.h"
#include "db.h"

/*char * mprog_type_to_name ( int type );*/

#define ALT_FLAGVALUE_SET( _blargh, _table, _arg )         \
        {                                                  \
                int blah = flag_value( _table, _arg );     \
                _blargh = (blah == NO_FLAG) ? 0 : blah;    \
        }

#define ALT_FLAGVALUE_TOGGLE( _blargh, _table, _arg )      \
        {                                                  \
                int blah = flag_value( _table, _arg );     \
                _blargh ^= (blah == NO_FLAG) ? 0 : blah;   \
        }

/* Return TRUE if area changed, FALSE if not. */
#define REDIT( fun )                bool fun( CHAR_DATA *ch, char *argument )
#define OEDIT( fun )                bool fun( CHAR_DATA *ch, char *argument )
#define MEDIT( fun )                bool fun( CHAR_DATA *ch, char *argument )
#define AEDIT( fun )                bool fun( CHAR_DATA *ch, char *argument )

struct olc_help_type
{
    const char             *command;
    const void             *structure;
    const char             *desc;
};

bool show_version( CHAR_DATA *ch, const char *argument )
{
    ch_printf( ch, "%s", VERSION );
    ch_printf( ch, "\r\n" );
    ch_printf( ch, "%s", AUTHOR );
    ch_printf( ch, "\r\n" );
    ch_printf( ch, "%s", DATE );
    ch_printf( ch, "\r\n" );
    ch_printf( ch, "%s", CREDITS );
    ch_printf( ch, "\r\n" );

    return FALSE;
}

/*
 * This table contains help commands and a brief description of each.
 * ------------------------------------------------------------------
 */
const struct olc_help_type help_table[] = {
    {"area", area_flags, "Area attributes."},
    {"room", room_flags, "Room attributes."},
    {"sector", sector_flags, "Sector types, terrain."},
    {"exit", exit_flags, "Exit types."},
    {"type", type_flags, "Types of objects."},
    {"extra", extra_flags, "Object attributes."},
    {"wear", wear_flags, "Where to wear object."},
    {"spec", spec_table, "Available special programs."},
    {"sex", sex_flags, "Sexes."},
    {"act", act_flags, "Mobile attributes."},
    {"affect", affect_flags, "Mobile affects."},
    {"wear-loc", wear_loc_flags, "Where mobile wears object."},
    {"spells", skill_table, "Names of current spells."},
    {"container", container_flags, "Container status."},

/* ROM specific bits: */

    {"armor", ac_type, "Ac for different attacks."},
    {"apply", apply_flags, "Apply flags"},
    {"form", form_flags, "Mobile body form."},
    {"part", part_flags, "Mobile body parts."},
    {"imm", imm_flags, "Mobile immunity."},
    {"res", res_flags, "Mobile resistance."},
    {"vuln", vuln_flags, "Mobile vulnerability."},
    {"off", off_flags, "Mobile offensive behaviour."},
    {"size", size_flags, "Mobile size."},
    {"position", position_flags, "Mobile positions."},
    {"wclass", weapon_class, "Weapon class."},
    {"wtype", weapon_type2, "Special weapon type."},
    {"portal", portal_flags, "Portal types."},
    {"furniture", furniture_flags, "Furniture types."},
    {"liquid", liq_table, "Liquid types."},
    {"apptype", apply_types, "Apply types."},
    {"weapon", attack_table, "Weapon types."},
    {"mprog", mprog_flags, "MobProgram flags."},
    {NULL, NULL, NULL}
};

/*****************************************************************************
 Name:           show_flag_cmds
 Purpose:        Displays settable flags and stats.
 Called by:      show_help(olc_act.c).
 ****************************************************************************/
void show_flag_cmds( CHAR_DATA *ch, const struct flag_type *flag_table )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    buf1[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    int                     flag = 0;
    int                     col = 0;

    for ( flag = 0; flag_table[flag].name != NULL; flag++ )
    {
        if ( flag_table[flag].settable )
        {
            sprintf( buf, "%-19.18s", flag_table[flag].name );
            strcat( buf1, buf );
            if ( ++col % 4 == 0 )
                strcat( buf1, "\r\n" );
        }
    }

    if ( col % 4 != 0 )
        strcat( buf1, "\r\n" );

    ch_printf( ch, "%s", buf1 );
    return;
}

/*****************************************************************************
 Name:           show_skill_cmds
 Purpose:        Displays all skill functions.
                 Does remove those damn immortal commands from the list.
                 Could be improved by:
                 (1) Adding a check for a particular class.
                 (2) Adding a check for a level range.
 Called by:      show_help(olc_act.c).
 ****************************************************************************/
void show_skill_cmds( CHAR_DATA *ch, int tar )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    buf1[MAX_STRING_LENGTH * 2] = "\0\0\0\0\0\0\0";
    int                     sn = 0;
    int                     col = 0;

    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if ( !skill_table[sn].name )
            break;

        if ( !str_cmp( skill_table[sn].name, "reserved" )
             || skill_table[sn].spell_fun == spell_null )
            continue;

        if ( tar == -1 || skill_table[sn].target == tar )
        {
            sprintf( buf, "%-19.18s", skill_table[sn].name );
            strcat( buf1, buf );
            if ( ++col % 4 == 0 )
                strcat( buf1, "\r\n" );
        }
    }

    if ( col % 4 != 0 )
        strcat( buf1, "\r\n" );

    ch_printf( ch, "%s", buf1 );
    return;
}

/*****************************************************************************
 Name:           show_spec_cmds
 Purpose:        Displays settable special functions.
 Called by:      show_help(olc_act.c).
 ****************************************************************************/
void show_spec_cmds( CHAR_DATA *ch )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    buf1[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    int                     spec = 0;
    int                     col = 0;

    ch_printf( ch, "Preceed special functions with 'spec_'\r\n\r\n" );
    for ( spec = 0; spec_table[spec].function != NULL; spec++ )
    {
        sprintf( buf, "%-19.18s", &spec_table[spec].name[5] );
        strcat( buf1, buf );
        if ( ++col % 4 == 0 )
            strcat( buf1, "\r\n" );
    }

    if ( col % 4 != 0 )
        strcat( buf1, "\r\n" );

    ch_printf( ch, "%s", buf1 );
    return;
}

/*****************************************************************************
 Name:           show_help
 Purpose:        Displays help for many tables used in OLC.
 Called by:      olc interpreters.
 ****************************************************************************/
bool show_help( CHAR_DATA *ch, const char *argument )
{
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    spell[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    int                     cnt = 0;

    argument = one_argument( argument, arg );
    one_argument( argument, spell );

    /*
     * Display syntax.
     */
    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  ? [command]\r\n\r\n" );
        ch_printf( ch, "[command]  [description]\r\n" );
        for ( cnt = 0; help_table[cnt].command != NULL; cnt++ )
        {
            ch_printf( ch, "%-10.10s -%s\r\n",
                       capitalize( help_table[cnt].command ), help_table[cnt].desc );
        }
        return false;
    }

    /*
     * Find the command, show changeable data.
     * ---------------------------------------
     */
    for ( cnt = 0; help_table[cnt].command != NULL; cnt++ )
    {
        if ( arg[0] == help_table[cnt].command[0]
             && !str_prefix( arg, help_table[cnt].command ) )
        {
            if ( help_table[cnt].structure == spec_table )
            {
                show_spec_cmds( ch );
                return false;
            }
            else if ( help_table[cnt].structure == liq_table )
            {
                show_liqlist( ch );
                return false;
            }
            else if ( help_table[cnt].structure == attack_table )
            {
                show_damlist( ch );
                return false;
            }
            else if ( help_table[cnt].structure == skill_table )
            {

                if ( spell[0] == '\0' )
                {
                    ch_printf( ch, "Syntax:  ? spells "
                               "[ignore/attack/defend/self/object/all]\r\n" );
                    return false;
                }

                if ( !str_prefix( spell, "all" ) )
                    show_skill_cmds( ch, -1 );
                else if ( !str_prefix( spell, "ignore" ) )
                    show_skill_cmds( ch, TAR_IGNORE );
                else if ( !str_prefix( spell, "attack" ) )
                    show_skill_cmds( ch, TAR_CHAR_OFFENSIVE );
                else if ( !str_prefix( spell, "defend" ) )
                    show_skill_cmds( ch, TAR_CHAR_DEFENSIVE );
                else if ( !str_prefix( spell, "self" ) )
                    show_skill_cmds( ch, TAR_CHAR_SELF );
                else if ( !str_prefix( spell, "object" ) )
                    show_skill_cmds( ch, TAR_OBJ_INV );
                else
                    ch_printf( ch, "Syntax:  ? spell "
                               "[ignore/attack/defend/self/object/all]\r\n" );

                return false;
            }
            else
            {
                show_flag_cmds( ch,
                                ( const struct flag_type * ) help_table[cnt].structure );
                return false;
            }
        }
    }

    show_help( ch, "" );
    return FALSE;
}

bool redit_rlist( CHAR_DATA *ch, const char *argument )
{
    ROOM_INDEX_DATA        *pRoomIndex = NULL;
    AREA_DATA              *pArea = NULL;
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    BUFFER                 *buf1 = NULL;
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    bool                    found = false;
    int                     vnum = 0;
    int                     col = 0;

    one_argument( argument, arg );

    pArea = ch->in_room->area;
    buf1 = new_buf(  );
    found = false;

    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ )
    {
        if ( ( pRoomIndex = get_room_index( vnum ) ) )
        {
            found = true;
            sprintf( buf, "[%5d] %-17.16s", vnum, capitalize( pRoomIndex->name ) );
            add_buf( buf1, buf );
            if ( ++col % 3 == 0 )
                add_buf( buf1, "\r\n" );
        }
    }

    if ( !found )
    {
        ch_printf( ch, "Room(s) not found in this area.\r\n" );
        return false;
    }

    if ( col % 3 != 0 )
        add_buf( buf1, "\r\n" );

    page_to_char( buf_string( buf1 ), ch );
    free_buf( buf1 );
    return false;
}

bool redit_mlist( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMobIndex = NULL;
    AREA_DATA              *pArea = NULL;
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    BUFFER                 *buf1 = NULL;
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    bool                    fAll = false;
    bool                    found = false;
    int                     vnum = 0;
    int                     col = 0;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  mlist <all/name>\r\n" );
        return false;
    }

    buf1 = new_buf(  );
    pArea = ch->in_room->area;
    fAll = !str_cmp( arg, "all" );
    found = false;

    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ )
    {
        if ( ( pMobIndex = get_mob_index( vnum ) ) != NULL )
        {
            if ( fAll || is_name( arg, pMobIndex->player_name ) )
            {
                found = true;
                sprintf( buf, "[%5d] %-17.16s",
                         pMobIndex->vnum, capitalize( pMobIndex->short_descr ) );
                add_buf( buf1, buf );
                if ( ++col % 3 == 0 )
                    add_buf( buf1, "\r\n" );
            }
        }
    }

    if ( !found )
    {
        ch_printf( ch, "Mobile(s) not found in this area.\r\n" );
        return false;
    }

    if ( col % 3 != 0 )
        add_buf( buf1, "\r\n" );

    page_to_char( buf_string( buf1 ), ch );
    free_buf( buf1 );
    return false;
}

bool redit_olist( CHAR_DATA *ch, const char *argument )
{
    OBJ_INDEX_DATA         *pObjIndex = NULL;
    AREA_DATA              *pArea = NULL;
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    BUFFER                 *buf1 = NULL;
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    bool                    fAll = false;
    bool                    found = false;
    int                     vnum = 0;
    int                     col = 0;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  olist <all/name/item_type>\r\n" );
        return false;
    }

    pArea = ch->in_room->area;
    buf1 = new_buf(  );
    fAll = !str_cmp( arg, "all" );
    found = false;

    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ )
    {
        if ( ( pObjIndex = get_obj_index( vnum ) ) )
        {
            if ( fAll || is_name( arg, pObjIndex->name )
                 || flag_value( type_flags, arg ) == pObjIndex->item_type )
            {
                found = true;
                sprintf( buf, "[%5d] %-17.16s",
                         pObjIndex->vnum, capitalize( pObjIndex->short_descr ) );
                add_buf( buf1, buf );
                if ( ++col % 3 == 0 )
                    add_buf( buf1, "\r\n" );
            }
        }
    }

    if ( !found )
    {
        ch_printf( ch, "Object(s) not found in this area.\r\n" );
        return false;
    }

    if ( col % 3 != 0 )
        add_buf( buf1, "\r\n" );

    page_to_char( buf_string( buf1 ), ch );
    free_buf( buf1 );
    return false;
}

bool redit_mshow( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;
    int                     value = 0;

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  mshow <vnum>\r\n" );
        return false;
    }

    if ( !is_number( argument ) )
    {
        ch_printf( ch, "REdit: Enter a number.\r\n" );
        return false;
    }

    if ( is_number( argument ) )
    {
        value = atoi( argument );
        if ( !( pMob = get_mob_index( value ) ) )
        {
            ch_printf( ch, "REdit:  That mobile does not exist.\r\n" );
            return false;
        }

        ch->desc->pEdit = ( void * ) pMob;
    }

    medit_show( ch, argument );
    ch->desc->pEdit = ( void * ) ch->in_room;
    return false;
}

bool redit_oshow( CHAR_DATA *ch, const char *argument )
{
    OBJ_INDEX_DATA         *pObj = NULL;
    int                     value = 0;

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  oshow <vnum>\r\n" );
        return false;
    }

    if ( !is_number( argument ) )
    {
        ch_printf( ch, "REdit: Enter a number.\r\n" );
        return false;
    }

    if ( is_number( argument ) )
    {
        value = atoi( argument );
        if ( !( pObj = get_obj_index( value ) ) )
        {
            ch_printf( ch, "REdit:  That object does not exist.\r\n" );
            return false;
        }

        ch->desc->pEdit = ( void * ) pObj;
    }

    oedit_show( ch, argument );
    ch->desc->pEdit = ( void * ) ch->in_room;
    return false;
}

/*****************************************************************************
 Name:           check_range( lower vnum, upper vnum )
 Purpose:        Ensures the range spans only one area.
 Called by:      aedit_vnum(olc_act.c).
 ****************************************************************************/
bool check_range( int lower, int upper )
{
    AREA_DATA              *pArea = NULL;
    int                     cnt = 0;

    for ( pArea = area_first; pArea; pArea = pArea->next )
    {
        /*
         * lower < area < upper
         */
        if ( ( lower <= pArea->min_vnum && pArea->min_vnum <= upper )
             || ( lower <= pArea->max_vnum && pArea->max_vnum <= upper ) )
            ++cnt;

        if ( cnt > 1 )
            return false;
    }
    return true;
}

AREA_DATA              *get_vnum_area( int vnum )
{
    AREA_DATA              *pArea;

    for ( pArea = area_first; pArea; pArea = pArea->next )
    {
        if ( vnum >= pArea->min_vnum && vnum <= pArea->max_vnum )
            return pArea;
    }

    return 0;
}

/*
 * Area Editor Functions.
 */
bool aedit_show( CHAR_DATA *ch, const char *argument )
{
    AREA_DATA              *pArea = NULL;

    EDIT_AREA( ch, pArea );

    ch_printf( ch, "Name:     [%5d] %s\r\n", pArea->vnum, pArea->name );

#if 0                                                  /* ROM OLC */
    sprintf( buf, "Recall:   [%5d] %s\r\n", pArea->recall,
             get_room_index( pArea->recall )
             ? get_room_index( pArea->recall )->name : "none" );
    send_to_char( buf, ch );
#endif /* ROM */

    ch_printf( ch, "File:     %s\r\n", pArea->file_name );

    ch_printf( ch, "Vnums:    [%d-%d]\r\n", pArea->min_vnum, pArea->max_vnum );

    ch_printf( ch, "Age:      [%d]\r\n", pArea->age );

    ch_printf( ch, "Players:  [%d]\r\n", pArea->nplayer );

    ch_printf( ch, "Security: [%d]\r\n", pArea->security );

    ch_printf( ch, "Builders: [%s]\r\n", pArea->builders );

    ch_printf( ch, "Credits : [%s]\r\n", pArea->credits );

    ch_printf( ch, "Flags:    [%s]\r\n", flag_string( area_flags, pArea->area_flags ) );

    return false;
}

bool aedit_reset( CHAR_DATA *ch, const char *argument )
{
    AREA_DATA              *pArea = NULL;

    EDIT_AREA( ch, pArea );

    reset_area( pArea );
    ch_printf( ch, "Area reset.\r\n" );

    return false;
}

bool aedit_create( CHAR_DATA *ch, const char *argument )
{
    AREA_DATA              *pArea = NULL;

    pArea = new_area(  );
    area_last->next = pArea;
    area_last = pArea;                                 /* Thanks, Walker. */
    ch->desc->pEdit = ( void * ) pArea;

    SET_BIT( pArea->area_flags, AREA_ADDED );
    ch_printf( ch, "Area Created.\r\n" );
    return false;
}

bool aedit_name( CHAR_DATA *ch, const char *argument )
{
    AREA_DATA              *pArea = NULL;

    EDIT_AREA( ch, pArea );

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Syntax:   name [$name]\r\n" );
        return false;
    }

    free_string( pArea->name );
    pArea->name = str_dup( argument );

    ch_printf( ch, "Name set.\r\n" );
    return true;
}

bool aedit_credits( CHAR_DATA *ch, const char *argument )
{
    AREA_DATA              *pArea = NULL;

    EDIT_AREA( ch, pArea );

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Syntax:   credits [$credits]\r\n" );
        return false;
    }

    free_string( pArea->credits );
    pArea->credits = str_dup( argument );

    ch_printf( ch, "Credits set.\r\n" );
    return true;
}

bool aedit_file( CHAR_DATA *ch, const char *argument )
{
    AREA_DATA              *pArea = NULL;
    char                    file[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    int                     i = 0;
    int                     length = 0;

    EDIT_AREA( ch, pArea );

    one_argument( argument, file );                    /* Forces Lowercase */

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  filename [$file]\r\n" );
        return false;
    }

    /*
     * Simple Syntax Check.
     */
    length = strlen( argument );
    if ( length > 8 )
    {
        ch_printf( ch, "No more than eight characters allowed.\r\n" );
        return false;
    }

    /*
     * Allow only letters and numbers.
     */
    for ( i = 0; i < length; i++ )
    {
        if ( !isalnum( file[i] ) )
        {
            ch_printf( ch, "Only letters and numbers are valid.\r\n" );
            return false;
        }
    }

    free_string( pArea->file_name );
    strcat( file, ".are" );
    pArea->file_name = str_dup( file );

    ch_printf( ch, "Filename set.\r\n" );
    return true;
}

bool aedit_age( CHAR_DATA *ch, const char *argument )
{
    AREA_DATA              *pArea = NULL;
    char                    age[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    EDIT_AREA( ch, pArea );

    one_argument( argument, age );

    if ( !is_number( age ) || age[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  age [#xage]\r\n" );
        return false;
    }

    pArea->age = atoi( age );

    ch_printf( ch, "Age set.\r\n" );
    return true;
}

#if 0                                                  /* ROM OLC */
AEDIT( aedit_recall )
{
    AREA_DATA              *pArea = NULL;
    char                    room[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    int                     value = 0;

    EDIT_AREA( ch, pArea );

    one_argument( argument, room );

    if ( !is_number( argument ) || argument[0] == '\0' )
    {
        send_to_char( "Syntax:  recall [#xrvnum]\r\n", ch );
        return FALSE;
    }

    value = atoi( room );

    if ( !get_room_index( value ) )
    {
        send_to_char( "AEdit:  Room vnum does not exist.\r\n", ch );
        return FALSE;
    }

    pArea->recall = value;

    send_to_char( "Recall set.\r\n", ch );
    return TRUE;
}
#endif /* ROM OLC */

bool aedit_security( CHAR_DATA *ch, const char *argument )
{
    AREA_DATA              *pArea = NULL;
    char                    sec[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    int                     value = 0;

    EDIT_AREA( ch, pArea );

    one_argument( argument, sec );

    if ( !is_number( sec ) || sec[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  security [#xlevel]\r\n" );
        return false;
    }

    value = atoi( sec );

    if ( value > ch->pcdata->security || value < 0 )
    {
        if ( ch->pcdata->security != 0 )
        {
            ch_printf( ch, "Security is 0-%d.\r\n", ch->pcdata->security );
        }
        else
            ch_printf( ch, "Security is 0 only.\r\n" );
        return false;
    }

    pArea->security = value;

    ch_printf( ch, "Security set.\r\n" );
    return true;
}

bool aedit_builder( CHAR_DATA *ch, const char *argument )
{
    AREA_DATA              *pArea = NULL;
    char                    name[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    EDIT_AREA( ch, pArea );

    one_argument( argument, name );

    if ( name[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  builder [$name]  -toggles builder\r\n" );
        ch_printf( ch, "Syntax:  builder All      -allows everyone\r\n" );
        return false;
    }

    name[0] = UPPER( name[0] );

    if ( strstr( pArea->builders, name ) != '\0' )
    {
        pArea->builders = string_replace( pArea->builders, name, "\0" );
        pArea->builders = string_unpad( pArea->builders );

        if ( pArea->builders[0] == '\0' )
        {
            free_string( pArea->builders );
            pArea->builders = str_dup( "None" );
        }
        ch_printf( ch, "Builder removed.\r\n" );
        return true;
    }
    else
    {
        buf[0] = '\0';
        if ( strstr( pArea->builders, "None" ) != '\0' )
        {
            pArea->builders = string_replace( pArea->builders, "None", "\0" );
            pArea->builders = string_unpad( pArea->builders );
        }

        if ( pArea->builders[0] != '\0' )
        {
            strcat( buf, pArea->builders );
            strcat( buf, " " );
        }
        strcat( buf, name );
        free_string( pArea->builders );
        pArea->builders = string_proper( str_dup( buf ) );

        ch_printf( ch, "Builder added.\r\n" );
        ch_printf( ch, "%s", pArea->builders );
        return true;
    }

    return FALSE;
}

bool aedit_vnum( CHAR_DATA *ch, const char *argument )
{
    AREA_DATA              *pArea = NULL;
    char                    lower[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    upper[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    int                     ilower = 0;
    int                     iupper = 0;

    EDIT_AREA( ch, pArea );

    argument = one_argument( argument, lower );
    one_argument( argument, upper );

    if ( !is_number( lower ) || lower[0] == '\0'
         || !is_number( upper ) || upper[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  vnum [#xlower] [#xupper]\r\n" );
        return false;
    }

    if ( ( ilower = atoi( lower ) ) > ( iupper = atoi( upper ) ) )
    {
        ch_printf( ch, "AEdit:  Upper must be larger then lower.\r\n" );
        return false;
    }

    if ( !check_range( atoi( lower ), atoi( upper ) ) )
    {
        ch_printf( ch, "AEdit:  Range must include only this area.\r\n" );
        return false;
    }

    if ( get_vnum_area( ilower ) && get_vnum_area( ilower ) != pArea )
    {
        ch_printf( ch, "AEdit:  Lower vnum already assigned.\r\n" );
        return false;
    }

    pArea->min_vnum = ilower;
    ch_printf( ch, "Lower vnum set.\r\n" );

    if ( get_vnum_area( iupper ) && get_vnum_area( iupper ) != pArea )
    {
        ch_printf( ch, "AEdit:  Upper vnum already assigned.\r\n" );
        return true;                                   /* The lower value has been set. */
    }

    pArea->max_vnum = iupper;
    ch_printf( ch, "Upper vnum set.\r\n" );

    return true;
}

bool aedit_lvnum( CHAR_DATA *ch, const char *argument )
{
    AREA_DATA              *pArea = NULL;
    char                    lower[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    int                     ilower = 0;
    int                     iupper = 0;

    EDIT_AREA( ch, pArea );

    one_argument( argument, lower );

    if ( !is_number( lower ) || lower[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  min_vnum [#xlower]\r\n" );
        return false;
    }

    if ( ( ilower = atoi( lower ) ) > ( iupper = pArea->max_vnum ) )
    {
        ch_printf( ch, "AEdit:  Value must be less than the max_vnum.\r\n" );
        return false;
    }

    if ( !check_range( ilower, iupper ) )
    {
        ch_printf( ch, "AEdit:  Range must include only this area.\r\n" );
        return false;
    }

    if ( get_vnum_area( ilower ) && get_vnum_area( ilower ) != pArea )
    {
        ch_printf( ch, "AEdit:  Lower vnum already assigned.\r\n" );
        return false;
    }

    pArea->min_vnum = ilower;
    ch_printf( ch, "Lower vnum set.\r\n" );
    return true;
}

bool aedit_uvnum( CHAR_DATA *ch, const char *argument )
{
    AREA_DATA              *pArea = NULL;
    char                    upper[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    int                     ilower = 0;
    int                     iupper = 0;

    EDIT_AREA( ch, pArea );

    one_argument( argument, upper );

    if ( !is_number( upper ) || upper[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  max_vnum [#xupper]\r\n" );
        return false;
    }

    if ( ( ilower = pArea->min_vnum ) > ( iupper = atoi( upper ) ) )
    {
        ch_printf( ch, "AEdit:  Upper must be larger then lower.\r\n" );
        return false;
    }

    if ( !check_range( ilower, iupper ) )
    {
        ch_printf( ch, "AEdit:  Range must include only this area.\r\n" );
        return false;
    }

    if ( get_vnum_area( iupper ) && get_vnum_area( iupper ) != pArea )
    {
        ch_printf( ch, "AEdit:  Upper vnum already assigned.\r\n" );
        return false;
    }

    pArea->max_vnum = iupper;
    ch_printf( ch, "Upper vnum set.\r\n" );

    return true;
}

/*
 * Room Editor Functions.
 */
bool redit_show( CHAR_DATA *ch, const char *argument )
{
    ROOM_INDEX_DATA        *pRoom = NULL;
    OBJ_DATA               *obj = NULL;
    CHAR_DATA              *rch = NULL;
    int                     door = 0;

    EDIT_ROOM( ch, pRoom );

    ch_printf( ch, "Description:\r\n%s", pRoom->description );
    ch_printf( ch, "Name:       [%s]\r\nArea:       [%5d] %s\r\n",
             pRoom->name, pRoom->area->vnum, pRoom->area->name );
    ch_printf( ch, "Vnum:       [%5d]\r\nSector:     [%s]\r\n",
             pRoom->vnum, flag_string( sector_flags, pRoom->sector_type ) );
    ch_printf( ch, "Room flags: [%s]\r\n", flag_string( room_flags, pRoom->room_flags ) );

    if ( pRoom->heal_rate != 100 || pRoom->mana_rate != 100 )
    {
        ch_printf( ch, "Health rec: [%d]\r\nMana rec  : [%d]\r\n",
                 pRoom->heal_rate, pRoom->mana_rate );
    }

    if ( pRoom->clan > 0 )
    {
        ch_printf( ch, "Clan      : [%d] %s\r\n",
                 pRoom->clan, clan_table[pRoom->clan].name );
    }

    if ( !IS_NULLSTR( pRoom->owner ) )
    {
        ch_printf( ch, "Owner     : [%s]\r\n", pRoom->owner );
    }

    if ( pRoom->extra_descr )
    {
        EXTRA_DESCR_DATA       *ed = NULL;

        ch_printf( ch, "Desc Kwds:  [" );
        for ( ed = pRoom->extra_descr; ed; ed = ed->next )
        {
            ch_printf( ch, "%s", ed->keyword );
            if ( ed->next )
                ch_printf( ch, " " );
        }
        ch_printf( ch, "]\r\n" );
    }

    if ( pRoom->people )
    {
        ch_printf( ch, "Characters: [" );
        for ( rch = pRoom->people; rch; rch = rch->next_in_room )
        {
            char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

            one_argument( rch->name, buf );
            ch_printf( ch, "%s", buf );
            if ( rch->next_in_room )
                ch_printf( ch, " " );
        }
        ch_printf( ch, "]\r\n" );
    }

    if ( pRoom->contents )
    {
        ch_printf( ch, "Objects:    [" );
        for ( obj = pRoom->contents; obj; obj = obj->next_content )
        {
            char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

            one_argument( obj->name, buf );
            ch_printf( ch, "%s", buf );
            if ( obj->next_content )
                ch_printf( ch, " " );
        }
        ch_printf( ch, "]\r\n" );
    }

    for ( door = 0; door < MAX_DIR; door++ )
    {
        EXIT_DATA              *pexit = NULL;

        if ( ( pexit = pRoom->exit[door] ) )
        {
            char                    word[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
            char                    reset_state[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
            const char             *state = NULL;
            int                     i = 0;
            int                     length = 0;

            ch_printf( ch, "-%-5s to [%5d] Key: [%5d] ",
                     capitalize( dir_name[door] ),
                     pexit->u1.to_room ? pexit->u1.to_room->vnum : 0, pexit->key );

            /*
             * Format up the exit info.
             * Capitalize all flags that are not part of the reset info.
             */
            strcpy( reset_state, flag_string( exit_flags, pexit->rs_flags ) );
            state = flag_string( exit_flags, pexit->exit_info );
            ch_printf( ch, " Exit flags: [" );

            state = one_argument( state, word );

            if ( word[0] == '\0' )
            {
                ch_printf( ch, "]\r\n" );
            }
            else
            {
                while ( word[0] != '\0' )
                {
                    if ( str_infix( word, reset_state ) )
                    {
                        length = strlen( word );
                        for ( i = 0; i < length; i++ )
                            word[i] = UPPER( word[i] );
                    }
                    ch_printf( ch, "%s", word );
                    state = one_argument( state, word );

                    if ( word[0] != '\0' )
                    {
                        ch_printf( ch, " " );
                    }
                }
                ch_printf( ch, "]\r\n" );
            }

            if ( pexit->keyword && pexit->keyword[0] != '\0' )
            {
                ch_printf( ch, "Kwds: [%s]\r\n", pexit->keyword );
            }
            if ( pexit->description && pexit->description[0] != '\0' )
            {
                ch_printf( ch, "%s", pexit->description );
            }
        }
    }

    return false;
}

/* Local function. */
bool change_exit( CHAR_DATA *ch, const char *argument, int door )
{
    ROOM_INDEX_DATA        *pRoom = NULL;
    char                    command[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    int                     value = 0;

    EDIT_ROOM( ch, pRoom );

    /*
     * Set the exit flags, needs full argument.
     * ----------------------------------------
     */
    if ( ( value = flag_value( exit_flags, argument ) ) != NO_FLAG )
    {
        ROOM_INDEX_DATA        *pToRoom = NULL;
        int                     rev = 0;               /* ROM OLC */

        if ( !pRoom->exit[door] )
        {
            ch_printf( ch, "Exit does not exist.\r\n" );
            return false;
        }

        /*
         * This room.
         */
        TOGGLE_BIT( pRoom->exit[door]->rs_flags, value );
        /*
         * Don't toggle exit_info because it can be changed by players. 
         */
        pRoom->exit[door]->exit_info = pRoom->exit[door]->rs_flags;

        /*
         * Connected room.
         */
        pToRoom = pRoom->exit[door]->u1.to_room;       /* ROM OLC */
        rev = rev_dir[door];

        if ( pToRoom->exit[rev] != NULL )
        {
            pToRoom->exit[rev]->rs_flags = pRoom->exit[door]->rs_flags;
            pToRoom->exit[rev]->exit_info = pRoom->exit[door]->exit_info;
        }

        ch_printf( ch, "Exit flag toggled.\r\n" );
        return true;
    }

    /*
     * Now parse the arguments.
     */
    argument = one_argument( argument, command );
    one_argument( argument, arg );

    if ( command[0] == '\0' && argument[0] == '\0' )   /* Move command. */
    {
        move_char( ch, door, TRUE );                   /* ROM OLC */
        return false;
    }

    if ( command[0] == '?' )
    {
        do_help( ch, "EXIT" );
        return false;
    }

    if ( !str_cmp( command, "delete" ) )
    {
        ROOM_INDEX_DATA        *pToRoom = NULL;
        int                     rev = 0;               /* ROM OLC */

        if ( !pRoom->exit[door] )
        {
            ch_printf( ch, "REdit:  Cannot delete a null exit.\r\n" );
            return false;
        }

        /*
         * Remove ToRoom Exit.
         */
        rev = rev_dir[door];
        pToRoom = pRoom->exit[door]->u1.to_room;       /* ROM OLC */

        if ( pToRoom->exit[rev] )
        {
            free_exit( pToRoom->exit[rev] );
            pToRoom->exit[rev] = NULL;
        }

        /*
         * Remove this exit.
         */
        free_exit( pRoom->exit[door] );
        pRoom->exit[door] = NULL;

        ch_printf( ch, "Exit unlinked.\r\n" );
        return true;
    }

    if ( !str_cmp( command, "link" ) )
    {
        EXIT_DATA              *pExit = NULL;
        ROOM_INDEX_DATA        *toRoom = NULL;

        if ( arg[0] == '\0' || !is_number( arg ) )
        {
            ch_printf( ch, "Syntax:  [direction] link [vnum]\r\n" );
            return false;
        }

        value = atoi( arg );

        if ( !( toRoom = get_room_index( value ) ) )
        {
            ch_printf( ch, "REdit:  Cannot link to non-existant room.\r\n" );
            return false;
        }

        if ( !IS_BUILDER( ch, toRoom->area ) )
        {
            ch_printf( ch, "REdit:  Cannot link to that area.\r\n" );
            return false;
        }

        if ( toRoom->exit[rev_dir[door]] )
        {
            ch_printf( ch, "REdit:  Remote side's exit already exists.\r\n" );
            return false;
        }

        if ( !pRoom->exit[door] )
            pRoom->exit[door] = new_exit(  );

        pRoom->exit[door]->u1.to_room = toRoom;
        pRoom->exit[door]->orig_door = door;

        door = rev_dir[door];
        pExit = new_exit(  );
        pExit->u1.to_room = pRoom;
        pExit->orig_door = door;
        toRoom->exit[door] = pExit;

        ch_printf( ch, "Two-way link established.\r\n" );
        return true;
    }

    if ( !str_cmp( command, "dig" ) )
    {
        char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

        if ( arg[0] == '\0' || !is_number( arg ) )
        {
            ch_printf( ch, "Syntax: [direction] dig <vnum>\r\n" );
            return false;
        }

        redit_create( ch, arg );
        sprintf( buf, "link %s", arg );
        change_exit( ch, buf, door );
        return true;
    }

    if ( !str_cmp( command, "room" ) )
    {
        ROOM_INDEX_DATA        *toRoom = NULL;

        if ( arg[0] == '\0' || !is_number( arg ) )
        {
            ch_printf( ch, "Syntax:  [direction] room [vnum]\r\n" );
            return false;
        }

        value = atoi( arg );

        if ( !( toRoom = get_room_index( value ) ) )
        {
            ch_printf( ch, "REdit:  Cannot link to non-existant room.\r\n" );
            return false;
        }

        if ( !pRoom->exit[door] )
            pRoom->exit[door] = new_exit(  );

        pRoom->exit[door]->u1.to_room = toRoom;        /* ROM OLC */
        pRoom->exit[door]->orig_door = door;

        ch_printf( ch, "One-way link established.\r\n" );
        return true;
    }

    if ( !str_cmp( command, "key" ) )
    {
        OBJ_INDEX_DATA         *key = NULL;

        if ( arg[0] == '\0' || !is_number( arg ) )
        {
            ch_printf( ch, "Syntax:  [direction] key [vnum]\r\n" );
            return false;
        }

        if ( !pRoom->exit[door] )
        {
            ch_printf( ch, "Exit does not exist.\r\n" );
            return false;
        }

        value = atoi( arg );

        if ( !( key = get_obj_index( value ) ) )
        {
            ch_printf( ch, "REdit:  Key doesn't exist.\r\n" );
            return false;
        }

        if ( key->item_type != ITEM_KEY )
        {
            ch_printf( ch, "REdit:  Object is not key.\r\n" );
            return false;
        }

        pRoom->exit[door]->key = value;

        ch_printf( ch, "Exit key set.\r\n" );
        return true;
    }

    if ( !str_cmp( command, "name" ) )
    {
        if ( arg[0] == '\0' )
        {
            ch_printf( ch, "Syntax:  [direction] name [string]\r\n" );
            ch_printf( ch, "         [direction] name none\r\n" );
            return false;
        }

        if ( !pRoom->exit[door] )
        {
            ch_printf( ch, "Exit does not exist.\r\n" );
            return false;
        }

        free_string( pRoom->exit[door]->keyword );

        if ( str_cmp( arg, "none" ) )
            pRoom->exit[door]->keyword = str_dup( arg );
        else
            pRoom->exit[door]->keyword = str_dup( "" );

        ch_printf( ch, "Exit name set.\r\n" );
        return true;
    }

    if ( !str_prefix( command, "description" ) )
    {
        if ( arg[0] == '\0' )
        {
            if ( !pRoom->exit[door] )
            {
                ch_printf( ch, "Exit does not exist.\r\n" );
                return false;
            }

            string_append( ch, &pRoom->exit[door]->description );
            return true;
        }

        ch_printf( ch, "Syntax:  [direction] desc\r\n" );
        return false;
    }

    return false;
}

bool redit_north( CHAR_DATA *ch, const char *argument )
{
    if ( change_exit( ch, argument, DIR_NORTH ) )
        return true;

    return false;
}

bool redit_south( CHAR_DATA *ch, const char *argument )
{
    if ( change_exit( ch, argument, DIR_SOUTH ) )
        return true;

    return false;
}

bool redit_east( CHAR_DATA *ch, const char *argument )
{
    if ( change_exit( ch, argument, DIR_EAST ) )
        return true;

    return false;
}

bool redit_west( CHAR_DATA *ch, const char *argument )
{
    if ( change_exit( ch, argument, DIR_WEST ) )
        return true;

    return false;
}

bool redit_up( CHAR_DATA *ch, const char *argument )
{
    if ( change_exit( ch, argument, DIR_UP ) )
        return true;

    return false;
}

bool redit_down( CHAR_DATA *ch, const char *argument )
{
    if ( change_exit( ch, argument, DIR_DOWN ) )
        return true;

    return false;
}

bool redit_ed( CHAR_DATA *ch, const char *argument )
{
    ROOM_INDEX_DATA        *pRoom = NULL;
    EXTRA_DESCR_DATA       *ed = NULL;
    char                    command[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    keyword[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";

    EDIT_ROOM( ch, pRoom );

    argument = one_argument( argument, command );
    one_argument( argument, keyword );

    if ( command[0] == '\0' || keyword[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  ed add [keyword]\r\n" );
        ch_printf( ch, "         ed edit [keyword]\r\n" );
        ch_printf( ch, "         ed delete [keyword]\r\n" );
        ch_printf( ch, "         ed format [keyword]\r\n" );
        return false;
    }

    if ( !str_cmp( command, "add" ) )
    {
        if ( keyword[0] == '\0' )
        {
            ch_printf( ch, "Syntax:  ed add [keyword]\r\n" );
            return false;
        }

        ed = new_extra_descr(  );
        ed->keyword = str_dup( keyword );
        ed->description = str_dup( "" );
        ed->next = pRoom->extra_descr;
        pRoom->extra_descr = ed;

        string_append( ch, &ed->description );

        return true;
    }

    if ( !str_cmp( command, "edit" ) )
    {
        if ( keyword[0] == '\0' )
        {
            ch_printf( ch, "Syntax:  ed edit [keyword]\r\n" );
            return false;
        }

        for ( ed = pRoom->extra_descr; ed; ed = ed->next )
        {
            if ( is_name( keyword, ed->keyword ) )
                break;
        }

        if ( !ed )
        {
            ch_printf( ch, "REdit:  Extra description keyword not found.\r\n" );
            return false;
        }

        string_append( ch, &ed->description );

        return true;
    }

    if ( !str_cmp( command, "delete" ) )
    {
        EXTRA_DESCR_DATA       *ped = NULL;

        if ( keyword[0] == '\0' )
        {
            ch_printf( ch, "Syntax:  ed delete [keyword]\r\n" );
            return false;
        }

        for ( ed = pRoom->extra_descr; ed; ed = ed->next )
        {
            if ( is_name( keyword, ed->keyword ) )
                break;
            ped = ed;
        }

        if ( !ed )
        {
            ch_printf( ch, "REdit:  Extra description keyword not found.\r\n" );
            return false;
        }

        if ( !ped )
            pRoom->extra_descr = ed->next;
        else
            ped->next = ed->next;

        free_extra_descr( ed );

        ch_printf( ch, "Extra description deleted.\r\n" );
        return true;
    }

    if ( !str_cmp( command, "format" ) )
    {
        if ( keyword[0] == '\0' )
        {
            ch_printf( ch, "Syntax:  ed format [keyword]\r\n" );
            return false;
        }

        for ( ed = pRoom->extra_descr; ed; ed = ed->next )
        {
            if ( is_name( keyword, ed->keyword ) )
                break;
        }

        if ( !ed )
        {
            ch_printf( ch, "REdit:  Extra description keyword not found.\r\n" );
            return false;
        }

        ed->description = format_string( ed->description );

        ch_printf( ch, "Extra description formatted.\r\n" );
        return true;
    }

    redit_ed( ch, "" );
    return false;
}

bool redit_create( CHAR_DATA *ch, const char *argument )
{
    AREA_DATA              *pArea = NULL;
    ROOM_INDEX_DATA        *pRoom = NULL;
    int                     value = 0;
    int                     iHash = 0;

    EDIT_ROOM( ch, pRoom );

    value = atoi( argument );

    if ( argument[0] == '\0' || value <= 0 )
    {
        ch_printf( ch, "Syntax:  create [vnum > 0]\r\n" );
        return false;
    }

    pArea = get_vnum_area( value );
    if ( !pArea )
    {
        ch_printf( ch, "REdit:  That vnum is not assigned an area.\r\n" );
        return false;
    }

    if ( !IS_BUILDER( ch, pArea ) )
    {
        ch_printf( ch, "REdit:  Vnum in an area you cannot build in.\r\n" );
        return false;
    }

    if ( get_room_index( value ) )
    {
        ch_printf( ch, "REdit:  Room vnum already exists.\r\n" );
        return false;
    }

    pRoom = new_room_index(  );
    pRoom->area = pArea;
    pRoom->vnum = value;

    if ( value > top_vnum_room )
        top_vnum_room = value;

    iHash = value % MAX_KEY_HASH;
    pRoom->next = room_index_hash[iHash];
    room_index_hash[iHash] = pRoom;
    ch->desc->pEdit = ( void * ) pRoom;

    ch_printf( ch, "Room created.\r\n" );
    return true;
}

bool redit_name( CHAR_DATA *ch, const char *argument )
{
    ROOM_INDEX_DATA        *pRoom = NULL;

    EDIT_ROOM( ch, pRoom );

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  name [name]\r\n" );
        return false;
    }

    free_string( pRoom->name );
    pRoom->name = str_dup( argument );

    ch_printf( ch, "Name set.\r\n" );
    return true;
}

bool redit_desc( CHAR_DATA *ch, const char *argument )
{
    ROOM_INDEX_DATA        *pRoom = NULL;

    EDIT_ROOM( ch, pRoom );

    if ( argument[0] == '\0' )
    {
        string_append( ch, &pRoom->description );
        return true;
    }

    ch_printf( ch, "Syntax:  desc\r\n" );
    return false;
}

bool redit_heal( CHAR_DATA *ch, const char *argument )
{
    ROOM_INDEX_DATA        *pRoom = NULL;

    EDIT_ROOM( ch, pRoom );

    if ( is_number( argument ) )
    {
        pRoom->heal_rate = atoi( argument );
        ch_printf( ch, "Heal rate set.\r\n" );
        return true;
    }

    ch_printf( ch, "Syntax : heal <#xnumber>\r\n" );
    return false;
}

bool redit_mana( CHAR_DATA *ch, const char *argument )
{
    ROOM_INDEX_DATA        *pRoom = NULL;

    EDIT_ROOM( ch, pRoom );

    if ( is_number( argument ) )
    {
        pRoom->mana_rate = atoi( argument );
        ch_printf( ch, "Mana rate set.\r\n" );
        return true;
    }

    ch_printf( ch, "Syntax : mana <#xnumber>\r\n" );
    return false;
}

bool redit_clan( CHAR_DATA *ch, const char *argument )
{
    ROOM_INDEX_DATA        *pRoom = NULL;

    EDIT_ROOM( ch, pRoom );

    pRoom->clan = clan_lookup( argument );

    ch_printf( ch, "Clan set.\r\n" );
    return true;
}

bool redit_format( CHAR_DATA *ch, const char *argument )
{
    ROOM_INDEX_DATA        *pRoom = NULL;

    EDIT_ROOM( ch, pRoom );

    pRoom->description = format_string( pRoom->description );

    ch_printf( ch, "String formatted.\r\n" );
    return true;
}

bool redit_mreset( CHAR_DATA *ch, const char *argument )
{
    ROOM_INDEX_DATA        *pRoom = NULL;
    MOB_INDEX_DATA         *pMobIndex = NULL;
    CHAR_DATA              *newmob = NULL;
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    RESET_DATA             *pReset = NULL;

    EDIT_ROOM( ch, pRoom );

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( arg[0] == '\0' || !is_number( arg ) )
    {
        ch_printf( ch, "Syntax:  mreset <vnum> <max #x> <mix #x>\r\n" );
        return false;
    }

    if ( !( pMobIndex = get_mob_index( atoi( arg ) ) ) )
    {
        ch_printf( ch, "REdit:  No mobile has that vnum.\r\n" );
        return false;
    }

    if ( pMobIndex->area != pRoom->area )
    {
        ch_printf( ch, "REdit:  No such mobile in this area.\r\n" );
        return false;
    }

    /*
     * Create the mobile reset.
     */
    pReset = new_reset_data(  );
    pReset->command = 'M';
    pReset->arg1 = pMobIndex->vnum;
    pReset->arg2 = is_number( arg2 ) ? atoi( arg2 ) : MAX_MOB;
    pReset->arg3 = pRoom->vnum;
    pReset->arg4 = is_number( argument ) ? atoi( argument ) : 1;
    add_reset( pRoom, pReset, 0 /* Last slot */  );

    /*
     * Create the mobile.
     */
    newmob = create_mobile( pMobIndex );
    char_to_room( newmob, pRoom );

    ch_printf( ch, "%s (%d) has been loaded and added to resets.\r\n"
               "There will be a maximum of %d loaded to this room.\r\n",
               capitalize( pMobIndex->short_descr ), pMobIndex->vnum, pReset->arg2 );
    act( "$n has created $N!", ch, NULL, newmob, TO_ROOM );
    return true;
}

struct wear_type
{
    int                     wear_loc;
    int                     wear_bit;
};

const struct wear_type  wear_table[] = {
    {WEAR_NONE, ITEM_TAKE},
    {WEAR_LIGHT, ITEM_LIGHT},
    {WEAR_FINGER_L, ITEM_WEAR_FINGER},
    {WEAR_FINGER_R, ITEM_WEAR_FINGER},
    {WEAR_NECK_1, ITEM_WEAR_NECK},
    {WEAR_NECK_2, ITEM_WEAR_NECK},
    {WEAR_BODY, ITEM_WEAR_BODY},
    {WEAR_HEAD, ITEM_WEAR_HEAD},
    {WEAR_LEGS, ITEM_WEAR_LEGS},
    {WEAR_FEET, ITEM_WEAR_FEET},
    {WEAR_HANDS, ITEM_WEAR_HANDS},
    {WEAR_ARMS, ITEM_WEAR_ARMS},
    {WEAR_SHIELD, ITEM_WEAR_SHIELD},
    {WEAR_ABOUT, ITEM_WEAR_ABOUT},
    {WEAR_WAIST, ITEM_WEAR_WAIST},
    {WEAR_WRIST_L, ITEM_WEAR_WRIST},
    {WEAR_WRIST_R, ITEM_WEAR_WRIST},
    {WEAR_WIELD, ITEM_WIELD},
    {WEAR_HOLD, ITEM_HOLD},
    {NO_FLAG, NO_FLAG}
};

/*****************************************************************************
 Name:           wear_loc
 Purpose:        Returns the location of the bit that matches the count.
                 1 = first match, 2 = second match etc.
 Called by:      oedit_reset(olc_act.c).
 ****************************************************************************/
int wear_loc( int bits, int count )
{
    int                     flag = 0;

    for ( flag = 0; wear_table[flag].wear_bit != NO_FLAG; flag++ )
    {
        if ( IS_SET( bits, wear_table[flag].wear_bit ) && --count < 1 )
            return wear_table[flag].wear_loc;
    }

    return NO_FLAG;
}

/*****************************************************************************
 Name:           wear_bit
 Purpose:        Converts a wear_loc into a bit.
 Called by:      redit_oreset(olc_act.c).
 ****************************************************************************/
int wear_bit( int loc )
{
    int                     flag = 0;

    for ( flag = 0; wear_table[flag].wear_loc != NO_FLAG; flag++ )
    {
        if ( loc == wear_table[flag].wear_loc )
            return wear_table[flag].wear_bit;
    }

    return 0;
}

bool redit_oreset( CHAR_DATA *ch, const char *argument )
{
    ROOM_INDEX_DATA        *pRoom = NULL;
    OBJ_INDEX_DATA         *pObjIndex = NULL;
    OBJ_DATA               *newobj = NULL;
    OBJ_DATA               *to_obj = NULL;
    CHAR_DATA              *to_mob = NULL;
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    int                     olevel = 0;
    RESET_DATA             *pReset = NULL;

    EDIT_ROOM( ch, pRoom );

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || !is_number( arg1 ) )
    {
        ch_printf( ch, "Syntax:  oreset <vnum> <args>\r\n" );
        ch_printf( ch, "        -no_args               = into room\r\n" );
        ch_printf( ch, "        -<obj_name>            = into obj\r\n" );
        ch_printf( ch, "        -<mob_name> <wear_loc> = into mob\r\n" );
        return false;
    }

    if ( !( pObjIndex = get_obj_index( atoi( arg1 ) ) ) )
    {
        ch_printf( ch, "REdit:  No object has that vnum.\r\n" );
        return false;
    }

    if ( pObjIndex->area != pRoom->area )
    {
        ch_printf( ch, "REdit:  No such object in this area.\r\n" );
        return false;
    }

    /*
     * Load into room.
     */
    if ( arg2[0] == '\0' )
    {
        pReset = new_reset_data(  );
        pReset->command = 'O';
        pReset->arg1 = pObjIndex->vnum;
        pReset->arg2 = 0;
        pReset->arg3 = pRoom->vnum;
        pReset->arg4 = 0;
        add_reset( pRoom, pReset, 0 /* Last slot */  );

        newobj = create_object( pObjIndex, number_fuzzy( olevel ) );
        obj_to_room( newobj, pRoom );

        ch_printf( ch, "%s (%d) has been loaded and added to resets.\r\n",
                   capitalize( pObjIndex->short_descr ), pObjIndex->vnum );
    }
    else
        /*
         * Load into object's inventory.
         */
    if ( argument[0] == '\0'
             && ( ( to_obj = get_obj_list( ch, arg2, pRoom->contents ) ) != NULL ) )
    {
        pReset = new_reset_data(  );
        pReset->command = 'P';
        pReset->arg1 = pObjIndex->vnum;
        pReset->arg2 = 0;
        pReset->arg3 = to_obj->pIndexData->vnum;
        pReset->arg4 = 1;
        add_reset( pRoom, pReset, 0 /* Last slot */  );

        newobj = create_object( pObjIndex, number_fuzzy( olevel ) );
        newobj->cost = 0;
        obj_to_obj( newobj, to_obj );

        ch_printf( ch, "%s (%d) has been loaded into "
                   "%s (%d) and added to resets.\r\n",
                   capitalize( newobj->short_descr ),
                   newobj->pIndexData->vnum,
                   to_obj->short_descr, to_obj->pIndexData->vnum );
    }
    else
        /*
         * Load into mobile's inventory.
         */
    if ( ( to_mob = get_char_room( ch, arg2 ) ) != NULL )
    {
        int                     the_wear_loc;

        /*
         * Make sure the location on mobile is valid.
         */
        if ( ( the_wear_loc = flag_value( wear_loc_flags, argument ) ) == NO_FLAG )
        {
            ch_printf( ch, "REdit: Invalid wear_loc.  '? wear-loc'\r\n" );
            return false;
        }

        /*
         * Disallow loading a sword(WEAR_WIELD) into WEAR_HEAD.
         */
        if ( !IS_SET( pObjIndex->wear_flags, wear_bit( the_wear_loc ) ) )
        {
            ch_printf( ch,
                       "%s (%d) has wear flags: [%s]\r\n",
                       capitalize( pObjIndex->short_descr ),
                       pObjIndex->vnum,
                       flag_string( wear_flags, pObjIndex->wear_flags ) );
            return false;
        }

        /*
         * Can't load into same position.
         */
        if ( get_eq_char( to_mob, the_wear_loc ) )
        {
            ch_printf( ch, "REdit:  Object already equipped.\r\n" );
            return false;
        }

        pReset = new_reset_data(  );
        pReset->arg1 = pObjIndex->vnum;
        pReset->arg2 = the_wear_loc;
        if ( pReset->arg2 == WEAR_NONE )
            pReset->command = 'G';
        else
            pReset->command = 'E';
        pReset->arg3 = the_wear_loc;

        add_reset( pRoom, pReset, 0 /* Last slot */  );

        olevel = URANGE( 0, to_mob->level - 2, LEVEL_HERO );
        newobj = create_object( pObjIndex, number_fuzzy( olevel ) );

        if ( to_mob->pIndexData->pShop )               /* Shop-keeper? */
        {
            switch ( pObjIndex->item_type )
            {
                default:
                    olevel = 0;
                    break;
                case ITEM_PILL:
                    olevel = number_range( 0, 10 );
                    break;
                case ITEM_POTION:
                    olevel = number_range( 0, 10 );
                    break;
                case ITEM_SCROLL:
                    olevel = number_range( 5, 15 );
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
                    if ( pReset->command == 'G' )
                        olevel = number_range( 5, 15 );
                    else
                        olevel = number_fuzzy( olevel );
                    break;
            }

            newobj = create_object( pObjIndex, olevel );
            if ( pReset->arg2 == WEAR_NONE )
                SET_BIT( newobj->extra_flags, ITEM_INVENTORY );
        }
        else
            newobj = create_object( pObjIndex, number_fuzzy( olevel ) );

        obj_to_char( newobj, to_mob );
        if ( pReset->command == 'E' )
            equip_char( to_mob, newobj, pReset->arg3 );

        ch_printf( ch, "%s (%d) has been loaded "
                   "%s of %s (%d) and added to resets.\r\n",
                   capitalize( pObjIndex->short_descr ),
                   pObjIndex->vnum,
                   flag_string( wear_loc_strings, pReset->arg3 ),
                   to_mob->short_descr, to_mob->pIndexData->vnum );
    }
    else                                               /* Display Syntax */
    {
        ch_printf( ch, "REdit:  That mobile isn't here.\r\n" );
        return false;
    }

    act( "$n has created $p!", ch, newobj, NULL, TO_ROOM );
    return true;
}

/*
 * Object Editor Functions.
 */
void show_obj_values( CHAR_DATA *ch, OBJ_INDEX_DATA *obj )
{
    switch ( obj->item_type )
    {
        default:                                      /* No values. */
            break;

        case ITEM_LIGHT:
            if ( obj->value[2] == -1 || obj->value[2] == 999 )  /* ROM OLC */
                ch_printf( ch, "[v2] Light:  Infinite[-1]\r\n" );
            else
                ch_printf( ch, "[v2] Light:  [%d]\r\n", obj->value[2] );
            break;

        case ITEM_WAND:
        case ITEM_STAFF:
            ch_printf( ch,
                       "[v0] Level:          [%d]\r\n"
                       "[v1] Charges Total:  [%d]\r\n"
                       "[v2] Charges Left:   [%d]\r\n"
                       "[v3] Spell:          %s\r\n",
                       obj->value[0],
                       obj->value[1],
                       obj->value[2],
                       obj->value[3] != -1 ? skill_table[obj->value[3]].name : "none" );
            break;

        case ITEM_PORTAL:
            ch_printf( ch,
                       "[v0] Charges:        [%d]\r\n"
                       "[v1] Exit Flags:     %s\r\n"
                       "[v2] Portal Flags:   %s\r\n"
                       "[v3] Goes to (vnum): [%d]\r\n",
                       obj->value[0],
                       flag_string( exit_flags, obj->value[1] ),
                       flag_string( portal_flags, obj->value[2] ), obj->value[3] );
            break;

        case ITEM_FURNITURE:
            ch_printf( ch,
                       "[v0] Max people:      [%d]\r\n"
                       "[v1] Max weight:      [%d]\r\n"
                       "[v2] Furniture Flags: %s\r\n"
                       "[v3] Heal bonus:      [%d]\r\n"
                       "[v4] Mana bonus:      [%d]\r\n",
                       obj->value[0],
                       obj->value[1],
                       flag_string( furniture_flags, obj->value[2] ),
                       obj->value[3], obj->value[4] );
            break;

        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_PILL:
            ch_printf( ch,
                       "[v0] Level:  [%d]\r\n"
                       "[v1] Spell:  %s\r\n"
                       "[v2] Spell:  %s\r\n"
                       "[v3] Spell:  %s\r\n"
                       "[v4] Spell:  %s\r\n",
                       obj->value[0],
                       obj->value[1] != -1 ? skill_table[obj->value[1]].name
                       : "none",
                       obj->value[2] != -1 ? skill_table[obj->value[2]].name
                       : "none",
                       obj->value[3] != -1 ? skill_table[obj->value[3]].name
                       : "none",
                       obj->value[4] != -1 ? skill_table[obj->value[4]].name : "none" );
            break;

/* ARMOR for ROM */

        case ITEM_ARMOR:
            ch_printf( ch,
                       "[v0] Ac pierce       [%d]\r\n"
                       "[v1] Ac bash         [%d]\r\n"
                       "[v2] Ac slash        [%d]\r\n"
                       "[v3] Ac exotic       [%d]\r\n",
                       obj->value[0], obj->value[1], obj->value[2], obj->value[3] );
            break;

/* WEAPON changed in ROM: */
/* I had to split the output here, I have no idea why, but it helped -- Hugin */
/* It somehow fixed a bug in showing scroll/pill/potions too ?! */
        case ITEM_WEAPON:
            ch_printf( ch, "[v0] Weapon class:   %s\r\n",
                       flag_string( weapon_class, obj->value[0] ) );
            ch_printf( ch, "[v1] Number of dice: [%d]\r\n", obj->value[1] );
            ch_printf( ch, "[v2] Type of dice:   [%d]\r\n", obj->value[2] );
            ch_printf( ch, "[v3] Type:           %s\r\n",
                       attack_table[obj->value[3]].name );
            ch_printf( ch, "[v4] Special type:   %s\r\n",
                       flag_string( weapon_type2, obj->value[4] ) );
            break;

        case ITEM_CONTAINER:
            ch_printf( ch,
                       "[v0] Weight:     [%d kg]\r\n"
                       "[v1] Flags:      [%s]\r\n"
                       "[v2] Key:     %s [%d]\r\n"
                       "[v3] Capacity    [%d]\r\n"
                       "[v4] Weight Mult [%d]\r\n",
                       obj->value[0],
                       flag_string( container_flags, obj->value[1] ),
                       get_obj_index( obj->value[2] )
                       ? get_obj_index( obj->value[2] )->short_descr
                       : "none", obj->value[2], obj->value[3], obj->value[4] );
            break;

        case ITEM_DRINK_CON:
            ch_printf( ch,
                       "[v0] Liquid Total: [%d]\r\n"
                       "[v1] Liquid Left:  [%d]\r\n"
                       "[v2] Liquid:       %s\r\n"
                       "[v3] Poisoned:     %s\r\n",
                       obj->value[0],
                       obj->value[1],
                       liq_table[obj->value[2]].liq_name,
                       obj->value[3] != 0 ? "Yes" : "No" );
            break;

        case ITEM_FOUNTAIN:
            ch_printf( ch,
                       "[v0] Liquid Total: [%d]\r\n"
                       "[v1] Liquid Left:  [%d]\r\n"
                       "[v2] Liquid:            %s\r\n",
                       obj->value[0], obj->value[1], liq_table[obj->value[2]].liq_name );
            break;

        case ITEM_FOOD:
            ch_printf( ch,
                       "[v0] Food hours: [%d]\r\n"
                       "[v1] Full hours: [%d]\r\n"
                       "[v3] Poisoned:   %s\r\n",
                       obj->value[0], obj->value[1], obj->value[3] != 0 ? "Yes" : "No" );
            break;

        case ITEM_MONEY:
            ch_printf( ch, "[v0] Gold:   [%d]\r\n", obj->value[0] );
            break;
    }

    return;
}

bool set_obj_values( CHAR_DATA *ch, OBJ_INDEX_DATA *pObj, int value_num,
                     const char *argument )
{
    switch ( pObj->item_type )
    {
        default:
            break;

        case ITEM_LIGHT:
            switch ( value_num )
            {
                default:
                    do_help( ch, "ITEM_LIGHT" );
                    return false;
                case 2:
                    ch_printf( ch, "HOURS OF LIGHT SET.\r\n\r\n" );
                    pObj->value[2] = atoi( argument );
                    break;
            }
            break;

        case ITEM_WAND:
        case ITEM_STAFF:
            switch ( value_num )
            {
                default:
                    do_help( ch, "ITEM_STAFF_WAND" );
                    return false;
                case 0:
                    ch_printf( ch, "SPELL LEVEL SET.\r\n\r\n" );
                    pObj->value[0] = atoi( argument );
                    break;
                case 1:
                    ch_printf( ch, "TOTAL NUMBER OF CHARGES SET.\r\n\r\n" );
                    pObj->value[1] = atoi( argument );
                    break;
                case 2:
                    ch_printf( ch, "CURRENT NUMBER OF CHARGES SET.\r\n\r\n" );
                    pObj->value[2] = atoi( argument );
                    break;
                case 3:
                    ch_printf( ch, "SPELL TYPE SET.\r\n" );
                    pObj->value[3] = skill_lookup( argument );
                    break;
            }
            break;

        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_PILL:
            switch ( value_num )
            {
                default:
                    do_help( ch, "ITEM_SCROLL_POTION_PILL" );
                    return false;
                case 0:
                    ch_printf( ch, "SPELL LEVEL SET.\r\n\r\n" );
                    pObj->value[0] = atoi( argument );
                    break;
                case 1:
                    ch_printf( ch, "SPELL TYPE 1 SET.\r\n\r\n" );
                    pObj->value[1] = skill_lookup( argument );
                    break;
                case 2:
                    ch_printf( ch, "SPELL TYPE 2 SET.\r\n\r\n" );
                    pObj->value[2] = skill_lookup( argument );
                    break;
                case 3:
                    ch_printf( ch, "SPELL TYPE 3 SET.\r\n\r\n" );
                    pObj->value[3] = skill_lookup( argument );
                    break;
                case 4:
                    ch_printf( ch, "SPELL TYPE 4 SET.\r\n\r\n" );
                    pObj->value[4] = skill_lookup( argument );
                    break;
            }
            break;

/* ARMOR for ROM: */

        case ITEM_ARMOR:
            switch ( value_num )
            {
                default:
                    do_help( ch, "ITEM_ARMOR" );
                    return false;
                case 0:
                    ch_printf( ch, "AC PIERCE SET.\r\n\r\n" );
                    pObj->value[0] = atoi( argument );
                    break;
                case 1:
                    ch_printf( ch, "AC BASH SET.\r\n\r\n" );
                    pObj->value[1] = atoi( argument );
                    break;
                case 2:
                    ch_printf( ch, "AC SLASH SET.\r\n\r\n" );
                    pObj->value[2] = atoi( argument );
                    break;
                case 3:
                    ch_printf( ch, "AC EXOTIC SET.\r\n\r\n" );
                    pObj->value[3] = atoi( argument );
                    break;
            }
            break;

/* WEAPONS changed in ROM */

        case ITEM_WEAPON:
            switch ( value_num )
            {
                default:
                    do_help( ch, "ITEM_WEAPON" );
                    return false;
                case 0:
                    ch_printf( ch, "WEAPON CLASS SET.\r\n\r\n" );
                    ALT_FLAGVALUE_SET( pObj->value[0], weapon_class, argument );
                    break;
                case 1:
                    ch_printf( ch, "NUMBER OF DICE SET.\r\n\r\n" );
                    pObj->value[1] = atoi( argument );
                    break;
                case 2:
                    ch_printf( ch, "TYPE OF DICE SET.\r\n\r\n" );
                    pObj->value[2] = atoi( argument );
                    break;
                case 3:
                    ch_printf( ch, "WEAPON TYPE SET.\r\n\r\n" );
                    pObj->value[3] = attack_lookup( argument );
                    break;
                case 4:
                    ch_printf( ch, "SPECIAL WEAPON TYPE TOGGLED.\r\n\r\n" );
                    ALT_FLAGVALUE_TOGGLE( pObj->value[4], weapon_type2, argument );
                    break;
            }
            break;

        case ITEM_PORTAL:
            switch ( value_num )
            {
                default:
                    do_help( ch, "ITEM_PORTAL" );
                    return false;

                case 0:
                    ch_printf( ch, "CHARGES SET.\r\n\r\n" );
                    pObj->value[0] = atoi( argument );
                    break;
                case 1:
                    ch_printf( ch, "EXIT FLAGS SET.\r\n\r\n" );
                    ALT_FLAGVALUE_SET( pObj->value[1], exit_flags, argument );
                    break;
                case 2:
                    ch_printf( ch, "PORTAL FLAGS SET.\r\n\r\n" );
                    ALT_FLAGVALUE_SET( pObj->value[2], portal_flags, argument );
                    break;
                case 3:
                    ch_printf( ch, "EXIT VNUM SET.\r\n\r\n" );
                    pObj->value[3] = atoi( argument );
                    break;
            }
            break;

        case ITEM_FURNITURE:
            switch ( value_num )
            {
                default:
                    do_help( ch, "ITEM_FURNITURE" );
                    return false;

                case 0:
                    send_to_char( "NUMBER OF PEOPLE SET.\r\n\r\n", ch );
                    pObj->value[0] = atoi( argument );
                    break;
                case 1:
                    send_to_char( "MAX WEIGHT SET.\r\n\r\n", ch );
                    pObj->value[1] = atoi( argument );
                    break;
                case 2:
                    send_to_char( "FURNITURE FLAGS TOGGLED.\r\n\r\n", ch );
                    ALT_FLAGVALUE_TOGGLE( pObj->value[2], furniture_flags, argument );
                    break;
                case 3:
                    send_to_char( "HEAL BONUS SET.\r\n\r\n", ch );
                    pObj->value[3] = atoi( argument );
                    break;
                case 4:
                    send_to_char( "MANA BONUS SET.\r\n\r\n", ch );
                    pObj->value[4] = atoi( argument );
                    break;
            }
            break;

        case ITEM_CONTAINER:
            switch ( value_num )
            {
                    int                     value;

                default:
                    do_help( ch, "ITEM_CONTAINER" );
                    return false;
                case 0:
                    ch_printf( ch, "WEIGHT CAPACITY SET.\r\n\r\n" );
                    pObj->value[0] = atoi( argument );
                    break;
                case 1:
                    if ( ( value = flag_value( container_flags, argument ) ) != NO_FLAG )
                        TOGGLE_BIT( pObj->value[1], value );
                    else
                    {
                        do_help( ch, "ITEM_CONTAINER" );
                        return false;
                    }
                    ch_printf( ch, "CONTAINER TYPE SET.\r\n\r\n" );
                    break;
                case 2:
                    if ( atoi( argument ) != 0 )
                    {
                        if ( !get_obj_index( atoi( argument ) ) )
                        {
                            ch_printf( ch, "THERE IS NO SUCH ITEM.\r\n\r\n" );
                            return false;
                        }

                        if ( get_obj_index( atoi( argument ) )->item_type != ITEM_KEY )
                        {
                            ch_printf( ch, "THAT ITEM IS NOT A KEY.\r\n\r\n" );
                            return false;
                        }
                    }
                    ch_printf( ch, "CONTAINER KEY SET.\r\n\r\n" );
                    pObj->value[2] = atoi( argument );
                    break;
                case 3:
                    ch_printf( ch, "CONTAINER MAX WEIGHT SET.\r\n" );
                    pObj->value[3] = atoi( argument );
                    break;
                case 4:
                    ch_printf( ch, "WEIGHT MULTIPLIER SET.\r\n\r\n" );
                    pObj->value[4] = atoi( argument );
                    break;
            }
            break;

        case ITEM_DRINK_CON:
            switch ( value_num )
            {
                default:
                    do_help( ch, "ITEM_DRINK" );
/* OLC                    do_help( ch, "liquids" );    */
                    return FALSE;
                case 0:
                    ch_printf( ch, "MAXIMUM AMOUT OF LIQUID HOURS SET.\r\n\r\n" );
                    pObj->value[0] = atoi( argument );
                    break;
                case 1:
                    ch_printf( ch, "CURRENT AMOUNT OF LIQUID HOURS SET.\r\n\r\n" );
                    pObj->value[1] = atoi( argument );
                    break;
                case 2:
                    ch_printf( ch, "LIQUID TYPE SET.\r\n\r\n" );
                    pObj->value[2] = ( liq_lookup( argument ) != -1 ?
                                       liq_lookup( argument ) : 0 );
                    break;
                case 3:
                    ch_printf( ch, "POISON VALUE TOGGLED.\r\n\r\n" );
                    pObj->value[3] = ( pObj->value[3] == 0 ) ? 1 : 0;
                    break;
            }
            break;

        case ITEM_FOUNTAIN:
            switch ( value_num )
            {
                default:
                    do_help( ch, "ITEM_FOUNTAIN" );
/* OLC                    do_help( ch, "liquids" );    */
                    return false;
                case 0:
                    ch_printf( ch, "MAXIMUM AMOUT OF LIQUID HOURS SET.\r\n\r\n" );
                    pObj->value[0] = atoi( argument );
                    break;
                case 1:
                    ch_printf( ch, "CURRENT AMOUNT OF LIQUID HOURS SET.\r\n\r\n" );
                    pObj->value[1] = atoi( argument );
                    break;
                case 2:
                    ch_printf( ch, "LIQUID TYPE SET.\r\n\r\n" );
                    pObj->value[2] = ( liq_lookup( argument ) != -1 ?
                                       liq_lookup( argument ) : 0 );
                    break;
            }
            break;

        case ITEM_FOOD:
            switch ( value_num )
            {
                default:
                    do_help( ch, "ITEM_FOOD" );
                    return false;
                case 0:
                    ch_printf( ch, "HOURS OF FOOD SET.\r\n\r\n" );
                    pObj->value[0] = atoi( argument );
                    break;
                case 1:
                    ch_printf( ch, "HOURS OF FULL SET.\r\n\r\n" );
                    pObj->value[1] = atoi( argument );
                    break;
                case 3:
                    ch_printf( ch, "POISON VALUE TOGGLED.\r\n\r\n" );
                    pObj->value[3] = ( pObj->value[3] == 0 ) ? 1 : 0;
                    break;
            }
            break;

        case ITEM_MONEY:
            switch ( value_num )
            {
                default:
                    do_help( ch, "ITEM_MONEY" );
                    return false;
                case 0:
                    ch_printf( ch, "GOLD AMOUNT SET.\r\n\r\n" );
                    pObj->value[0] = atoi( argument );
                    break;
                case 1:
                    ch_printf( ch, "SILVER AMOUNT SET.\r\n\r\n" );
                    pObj->value[1] = atoi( argument );
                    break;
            }
            break;
    }

    show_obj_values( ch, pObj );

    return true;
}

bool oedit_show( CHAR_DATA *ch, const char *argument )
{
    OBJ_INDEX_DATA         *pObj = NULL;
    AFFECT_DATA            *paf = NULL;
    int                     cnt = 0;

    EDIT_OBJ( ch, pObj );

    ch_printf( ch, "Name:        [%s]\r\nArea:        [%5d] %s\r\n",
               pObj->name,
               !pObj->area ? -1 : pObj->area->vnum,
               !pObj->area ? "No Area" : pObj->area->name );

    ch_printf( ch, "Vnum:        [%5d]\r\nType:        [%s]\r\n",
               pObj->vnum, flag_string( type_flags, pObj->item_type ) );

    ch_printf( ch, "Level:       [%5d]\r\n", pObj->level );

    ch_printf( ch, "Wear flags:  [%s]\r\n", flag_string( wear_flags, pObj->wear_flags ) );

    ch_printf( ch, "Extra flags: [%s]\r\n",
               flag_string( extra_flags, pObj->extra_flags ) );

    ch_printf( ch, "Material:    [%s]\r\n",            /* ROM */
               pObj->material );

    ch_printf( ch, "Condition:   [%5d]\r\n",           /* ROM */
               pObj->condition );

    ch_printf( ch, "Weight:      [%5d]\r\nCost:        [%5d]\r\n",
               pObj->weight, pObj->cost );

    if ( pObj->extra_descr )
    {
        EXTRA_DESCR_DATA       *ed = NULL;

        ch_printf( ch, "Ex desc kwd: " );

        for ( ed = pObj->extra_descr; ed; ed = ed->next )
        {
            ch_printf( ch, "[" );
            ch_printf( ch, "%s", ed->keyword );
            ch_printf( ch, "]" );
        }

        ch_printf( ch, "\r\n" );
    }

    ch_printf( ch, "Short desc:  %s\r\nLong desc:\r\n     %s\r\n",
               pObj->short_descr, pObj->description );

    for ( cnt = 0, paf = pObj->affected; paf; paf = paf->next )
    {
        if ( cnt == 0 )
        {
            ch_printf( ch, "Number Modifier Affects\r\n" );
            ch_printf( ch, "------ -------- -------\r\n" );
        }
        ch_printf( ch, "[%4d] %-8d %s\r\n", cnt,
                   paf->modifier, flag_string( apply_flags, paf->location ) );
        cnt++;
    }

    show_obj_values( ch, pObj );

    return false;
}

/*
 * Need to issue warning if flag isn't valid. -- does so now -- Hugin.
 */
bool oedit_addaffect( CHAR_DATA *ch, const char *argument )
{
    int                     value = 0;
    OBJ_INDEX_DATA         *pObj = NULL;
    AFFECT_DATA            *pAf = NULL;
    char                    loc[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    mod[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    EDIT_OBJ( ch, pObj );

    argument = one_argument( argument, loc );
    one_argument( argument, mod );

    if ( loc[0] == '\0' || mod[0] == '\0' || !is_number( mod ) )
    {
        ch_printf( ch, "Syntax:  addaffect [location] [#xmod]\r\n" );
        return false;
    }

    if ( ( value = flag_value( apply_flags, loc ) ) == NO_FLAG )        /* Hugin */
    {
        ch_printf( ch, "Valid affects are:\r\n" );
        show_help( ch, "apply" );
        return false;
    }

    pAf = new_affect(  );
    pAf->location = value;
    pAf->modifier = atoi( mod );
    pAf->where = TO_OBJECT;
    pAf->type = -1;
    pAf->duration = -1;
    pAf->bitvector = 0;
    pAf->level = pObj->level;
    pAf->next = pObj->affected;
    pObj->affected = pAf;

    ch_printf( ch, "Affect added.\r\n" );
    return true;
}

bool oedit_addapply( CHAR_DATA *ch, const char *argument )
{
    int                     value = 0;
    int                     bv = 0;
    int                     typ = 0;
    OBJ_INDEX_DATA         *pObj = NULL;
    AFFECT_DATA            *pAf = NULL;
    char                    loc[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    mod[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    type[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    bvector[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    EDIT_OBJ( ch, pObj );

    argument = one_argument( argument, type );
    argument = one_argument( argument, loc );
    argument = one_argument( argument, mod );
    one_argument( argument, bvector );

    if ( type[0] == '\0' || ( typ = flag_value( apply_types, type ) ) == NO_FLAG )
    {
        ch_printf( ch, "Invalid apply type. Valid apply types are:\r\n" );
        show_help( ch, "apptype" );
        return false;
    }

    if ( loc[0] == '\0' || ( value = flag_value( apply_flags, loc ) ) == NO_FLAG )
    {
        ch_printf( ch, "Valid applys are:\r\n" );
        show_help( ch, "apply" );
        return false;
    }

    if ( bvector[0] == '\0'
         || ( bv = flag_value( bitvector_type[typ].table, bvector ) ) == NO_FLAG )
    {
        ch_printf( ch, "Invalid bitvector type.\r\n" );
        ch_printf( ch, "Valid bitvector types are:\r\n" );
        show_help( ch, bitvector_type[typ].help );
        return false;
    }

    if ( mod[0] == '\0' || !is_number( mod ) )
    {
        ch_printf( ch, "Syntax:  addapply [type] [location] [#xmod] [bitvector]\r\n" );
        return false;
    }

    pAf = new_affect(  );
    pAf->location = value;
    pAf->modifier = atoi( mod );
    pAf->where = apply_types[typ].bit;
    pAf->type = -1;
    pAf->duration = -1;
    pAf->bitvector = bv;
    pAf->level = pObj->level;
    pAf->next = pObj->affected;
    pObj->affected = pAf;

    ch_printf( ch, "Apply added.\r\n" );
    return true;
}

/*
 * My thanks to Hans Hvidsten Birkeland and Noam Krendel(Walker)
 * for really teaching me how to manipulate pointers.
 */
bool oedit_delaffect( CHAR_DATA *ch, const char *argument )
{
    OBJ_INDEX_DATA         *pObj = NULL;
    AFFECT_DATA            *pAf = NULL;
    AFFECT_DATA            *pAf_next = NULL;
    char                    affect[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    int                     value = 0;
    int                     cnt = 0;

    EDIT_OBJ( ch, pObj );

    one_argument( argument, affect );

    if ( !is_number( affect ) || affect[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  delaffect [#xaffect]\r\n" );
        return false;
    }

    value = atoi( affect );

    if ( value < 0 )
    {
        ch_printf( ch, "Only non-negative affect-numbers allowed.\r\n" );
        return false;
    }

    if ( !( pAf = pObj->affected ) )
    {
        ch_printf( ch, "OEdit:  Non-existant affect.\r\n" );
        return false;
    }

    if ( value == 0 )                                  /* First case: Remove first affect 
                                                        */
    {
        pAf = pObj->affected;
        pObj->affected = pAf->next;
        free_affect( pAf );
    }
    else                                               /* Affect to remove is not the * * 
                                                        * first */
    {
        while ( ( pAf_next = pAf->next ) && ( ++cnt < value ) )
            pAf = pAf_next;

        if ( pAf_next )                                /* See if it's the next affect */
        {
            pAf->next = pAf_next->next;
            free_affect( pAf_next );
        }
        else                                           /* Doesn't exist */
        {
            ch_printf( ch, "No such affect.\r\n" );
            return false;
        }
    }

    ch_printf( ch, "Affect removed.\r\n" );
    return true;
}

bool oedit_name( CHAR_DATA *ch, const char *argument )
{
    OBJ_INDEX_DATA         *pObj = NULL;

    EDIT_OBJ( ch, pObj );

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  name [string]\r\n" );
        return false;
    }

    free_string( pObj->name );
    pObj->name = str_dup( argument );

    ch_printf( ch, "Name set.\r\n" );
    return true;
}

bool oedit_short( CHAR_DATA *ch, const char *argument )
{
    OBJ_INDEX_DATA         *pObj = NULL;

    EDIT_OBJ( ch, pObj );

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  short [string]\r\n" );
        return false;
    }

    free_string( pObj->short_descr );
    pObj->short_descr = str_dup( argument );
    pObj->short_descr[0] = LOWER( pObj->short_descr[0] );

    ch_printf( ch, "Short description set.\r\n" );
    return true;
}

bool oedit_long( CHAR_DATA *ch, const char *argument )
{
    OBJ_INDEX_DATA         *pObj = NULL;

    EDIT_OBJ( ch, pObj );

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  long [string]\r\n" );
        return false;
    }

    free_string( pObj->description );
    pObj->description = str_dup( argument );
    pObj->description[0] = UPPER( pObj->description[0] );

    ch_printf( ch, "Long description set.\r\n" );
    return true;
}

bool set_value( CHAR_DATA *ch, OBJ_INDEX_DATA *pObj, const char *argument, int value )
{
    if ( argument[0] == '\0' )
    {
        set_obj_values( ch, pObj, -1, "" );            /* '\0' changed to "" -- Hugin */
        return false;
    }

    if ( set_obj_values( ch, pObj, value, argument ) )
        return true;

    return false;
}

/*****************************************************************************
 Name:           oedit_values
 Purpose:        Finds the object and sets its value.
 Called by:      The four valueX functions below. (now five -- Hugin )
 ****************************************************************************/
bool oedit_values( CHAR_DATA *ch, const char *argument, int value )
{
    OBJ_INDEX_DATA         *pObj = NULL;

    EDIT_OBJ( ch, pObj );

    if ( set_value( ch, pObj, argument, value ) )
        return true;

    return false;
}

bool oedit_value0( CHAR_DATA *ch, const char *argument )
{
    if ( oedit_values( ch, argument, 0 ) )
        return true;

    return false;
}

bool oedit_value1( CHAR_DATA *ch, const char *argument )
{
    if ( oedit_values( ch, argument, 1 ) )
        return true;

    return false;
}

bool oedit_value2( CHAR_DATA *ch, const char *argument )
{
    if ( oedit_values( ch, argument, 2 ) )
        return true;

    return false;
}

bool oedit_value3( CHAR_DATA *ch, const char *argument )
{
    if ( oedit_values( ch, argument, 3 ) )
        return true;

    return false;
}

bool oedit_value4( CHAR_DATA *ch, const char *argument )
{
    if ( oedit_values( ch, argument, 4 ) )
        return true;

    return false;
}

bool oedit_weight( CHAR_DATA *ch, const char *argument )
{
    OBJ_INDEX_DATA         *pObj;

    EDIT_OBJ( ch, pObj );

    if ( argument[0] == '\0' || !is_number( argument ) )
    {
        ch_printf( ch, "Syntax:  weight [number]\r\n" );
        return false;
    }

    pObj->weight = atoi( argument );

    ch_printf( ch, "Weight set.\r\n" );
    return true;
}

bool oedit_cost( CHAR_DATA *ch, const char *argument )
{
    OBJ_INDEX_DATA         *pObj = NULL;

    EDIT_OBJ( ch, pObj );

    if ( argument[0] == '\0' || !is_number( argument ) )
    {
        ch_printf( ch, "Syntax:  cost [number]\r\n" );
        return false;
    }

    pObj->cost = atoi( argument );

    ch_printf( ch, "Cost set.\r\n" );
    return true;
}

bool oedit_create( CHAR_DATA *ch, const char *argument )
{
    OBJ_INDEX_DATA         *pObj = NULL;
    AREA_DATA              *pArea = NULL;
    int                     value = 0;
    int                     iHash = 0;

    value = atoi( argument );
    if ( argument[0] == '\0' || value == 0 )
    {
        ch_printf( ch, "Syntax:  oedit create [vnum]\r\n" );
        return false;
    }

    pArea = get_vnum_area( value );
    if ( !pArea )
    {
        ch_printf( ch, "OEdit:  That vnum is not assigned an area.\r\n" );
        return false;
    }

    if ( !IS_BUILDER( ch, pArea ) )
    {
        ch_printf( ch, "OEdit:  Vnum in an area you cannot build in.\r\n" );
        return false;
    }

    if ( get_obj_index( value ) )
    {
        ch_printf( ch, "OEdit:  Object vnum already exists.\r\n" );
        return false;
    }

    pObj = new_obj_index(  );
    pObj->vnum = value;
    pObj->area = pArea;

    if ( value > top_vnum_obj )
        top_vnum_obj = value;

    iHash = value % MAX_KEY_HASH;
    pObj->next = obj_index_hash[iHash];
    obj_index_hash[iHash] = pObj;
    ch->desc->pEdit = ( void * ) pObj;

    ch_printf( ch, "Object Created.\r\n" );
    return true;
}

bool oedit_ed( CHAR_DATA *ch, const char *argument )
{
    OBJ_INDEX_DATA         *pObj = NULL;
    EXTRA_DESCR_DATA       *ed = NULL;
    char                    command[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    keyword[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";

    EDIT_OBJ( ch, pObj );

    argument = one_argument( argument, command );
    one_argument( argument, keyword );

    if ( command[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  ed add [keyword]\r\n" );
        ch_printf( ch, "         ed delete [keyword]\r\n" );
        ch_printf( ch, "         ed edit [keyword]\r\n" );
        ch_printf( ch, "         ed format [keyword]\r\n" );
        return false;
    }

    if ( !str_cmp( command, "add" ) )
    {
        if ( keyword[0] == '\0' )
        {
            ch_printf( ch, "Syntax:  ed add [keyword]\r\n" );
            return false;
        }

        ed = new_extra_descr(  );
        ed->keyword = str_dup( keyword );
        ed->next = pObj->extra_descr;
        pObj->extra_descr = ed;

        string_append( ch, &ed->description );

        return true;
    }

    if ( !str_cmp( command, "edit" ) )
    {
        if ( keyword[0] == '\0' )
        {
            ch_printf( ch, "Syntax:  ed edit [keyword]\r\n" );
            return false;
        }

        for ( ed = pObj->extra_descr; ed; ed = ed->next )
        {
            if ( is_name( keyword, ed->keyword ) )
                break;
        }

        if ( !ed )
        {
            ch_printf( ch, "OEdit:  Extra description keyword not found.\r\n" );
            return false;
        }

        string_append( ch, &ed->description );

        return true;
    }

    if ( !str_cmp( command, "delete" ) )
    {
        EXTRA_DESCR_DATA       *ped = NULL;

        if ( keyword[0] == '\0' )
        {
            ch_printf( ch, "Syntax:  ed delete [keyword]\r\n" );
            return false;
        }

        for ( ed = pObj->extra_descr; ed; ed = ed->next )
        {
            if ( is_name( keyword, ed->keyword ) )
                break;
            ped = ed;
        }

        if ( !ed )
        {
            ch_printf( ch, "OEdit:  Extra description keyword not found.\r\n" );
            return false;
        }

        if ( !ped )
            pObj->extra_descr = ed->next;
        else
            ped->next = ed->next;

        free_extra_descr( ed );

        ch_printf( ch, "Extra description deleted.\r\n" );
        return true;
    }

    if ( !str_cmp( command, "format" ) )
    {
        EXTRA_DESCR_DATA       *ped = NULL;

        if ( keyword[0] == '\0' )
        {
            ch_printf( ch, "Syntax:  ed format [keyword]\r\n" );
            return false;
        }

        for ( ed = pObj->extra_descr; ed; ed = ed->next )
        {
            if ( is_name( keyword, ed->keyword ) )
                break;
            ped = ed;
        }

        if ( !ed )
        {
            ch_printf( ch, "OEdit:  Extra description keyword not found.\r\n" );
            return false;
        }

        ed->description = format_string( ed->description );

        ch_printf( ch, "Extra description formatted.\r\n" );
        return true;
    }

    oedit_ed( ch, "" );
    return false;
}

/* ROM object functions : */

bool oedit_extra( CHAR_DATA *ch, const char *argument )
{
    OBJ_INDEX_DATA         *pObj = NULL;
    int                     value = 0;

    if ( argument[0] != '\0' )
    {
        EDIT_OBJ( ch, pObj );

        if ( ( value = flag_value( extra_flags, argument ) ) != NO_FLAG )
        {
            TOGGLE_BIT( pObj->extra_flags, value );

            ch_printf( ch, "Extra flag toggled.\r\n" );
            return true;
        }
    }

    ch_printf( ch, "Syntax:  extra [flag]\r\n"
               "Type '? extra' for a list of flags.\r\n" );
    return false;
}

bool oedit_wear( CHAR_DATA *ch, const char *argument )
{
    OBJ_INDEX_DATA         *pObj = NULL;
    int                     value = 0;

    if ( argument[0] != '\0' )
    {
        EDIT_OBJ( ch, pObj );

        if ( ( value = flag_value( wear_flags, argument ) ) != NO_FLAG )
        {
            TOGGLE_BIT( pObj->wear_flags, value );

            ch_printf( ch, "Wear flag toggled.\r\n" );
            return true;
        }
    }

    ch_printf( ch, "Syntax:  wear [flag]\r\n" "Type '? wear' for a list of flags.\r\n" );
    return false;
}

bool oedit_type( CHAR_DATA *ch, const char *argument )
{
    OBJ_INDEX_DATA         *pObj = NULL;
    int                     value = 0;

    if ( argument[0] != '\0' )
    {
        EDIT_OBJ( ch, pObj );

        if ( ( value = flag_value( type_flags, argument ) ) != NO_FLAG )
        {
            pObj->item_type = value;

            ch_printf( ch, "Type set.\r\n" );

            /*
             * Clear the values.
             */
            pObj->value[0] = 0;
            pObj->value[1] = 0;
            pObj->value[2] = 0;
            pObj->value[3] = 0;
            pObj->value[4] = 0;                        /* ROM */

            return true;
        }
    }

    ch_printf( ch, "Syntax:  type [flag]\r\n" "Type '? type' for a list of flags.\r\n" );
    return false;
}

bool oedit_material( CHAR_DATA *ch, const char *argument )
{
    OBJ_INDEX_DATA         *pObj = NULL;

    EDIT_OBJ( ch, pObj );

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  material [string]\r\n" );
        return false;
    }

    free_string( pObj->material );
    pObj->material = str_dup( argument );

    ch_printf( ch, "Material set.\r\n" );
    return true;
}

bool oedit_level( CHAR_DATA *ch, const char *argument )
{
    OBJ_INDEX_DATA         *pObj = NULL;

    EDIT_OBJ( ch, pObj );

    if ( argument[0] == '\0' || !is_number( argument ) )
    {
        ch_printf( ch, "Syntax:  level [number]\r\n" );
        return false;
    }

    pObj->level = atoi( argument );

    ch_printf( ch, "Level set.\r\n" );
    return true;
}

bool oedit_condition( CHAR_DATA *ch, const char *argument )
{
    OBJ_INDEX_DATA         *pObj = NULL;
    int                     value = 0;

    if ( argument[0] != '\0' && ( value = atoi( argument ) ) >= 0 && ( value <= 100 ) )
    {
        EDIT_OBJ( ch, pObj );

        pObj->condition = value;
        ch_printf( ch, "Condition set.\r\n" );

        return true;
    }

    ch_printf( ch, "Syntax:  condition [number]\r\n"
               "Where number can range from 0 (ruined) to 100 (perfect).\r\n" );
    return false;
}

/*
 * Mobile Editor Functions.
 */
bool medit_show( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;
    MPROG_LIST             *list = NULL;

    EDIT_MOB( ch, pMob );

    ch_printf( ch, "Name:        [%s]\r\nArea:        [%5d] %s\r\n",
               pMob->player_name,
               !pMob->area ? -1 : pMob->area->vnum,
               !pMob->area ? "No Area" : pMob->area->name );

    ch_printf( ch, "Act:         [%s]\r\n", flag_string( act_flags, pMob->act ) );

    ch_printf( ch, "Vnum:        [%5d] Sex:   [%s]   Race: [%s]\r\n",
               pMob->vnum,
               pMob->sex == SEX_MALE ? "male   " :
               pMob->sex == SEX_FEMALE ? "female " :
               pMob->sex == 3 ? "random " : "neutral", race_table[pMob->race].name );

    ch_printf( ch,
               "Level:       [%2d]    Align: [%4d]      Hitroll: [%2d] Dam Type:    [%s]\r\n",
               pMob->level, pMob->alignment,
               pMob->hitroll, attack_table[pMob->dam_type].name );

    if ( pMob->group )
    {
        ch_printf( ch, "Group:       [%5d]\r\n", pMob->group );
    }

    ch_printf( ch, "Hit dice:    [%2dd%-3d+%4d] ",
               pMob->hit[DICE_NUMBER], pMob->hit[DICE_TYPE], pMob->hit[DICE_BONUS] );

    ch_printf( ch, "Damage dice: [%2dd%-3d+%4d] ",
               pMob->damage[DICE_NUMBER],
               pMob->damage[DICE_TYPE], pMob->damage[DICE_BONUS] );

    ch_printf( ch, "Mana dice:   [%2dd%-3d+%4d]\r\n",
               pMob->mana[DICE_NUMBER], pMob->mana[DICE_TYPE], pMob->mana[DICE_BONUS] );

/* ROM values end */

    ch_printf( ch, "Affected by: [%s]\r\n",
               flag_string( affect_flags, pMob->affected_by ) );

/* ROM values: */

    ch_printf( ch, "Armor:       [pierce: %d  bash: %d  slash: %d  magic: %d]\r\n",
               pMob->ac[AC_PIERCE], pMob->ac[AC_BASH],
               pMob->ac[AC_SLASH], pMob->ac[AC_EXOTIC] );

    ch_printf( ch, "Form:        [%s]\r\n", flag_string( form_flags, pMob->form ) );

    ch_printf( ch, "Parts:       [%s]\r\n", flag_string( part_flags, pMob->parts ) );

    ch_printf( ch, "Imm:         [%s]\r\n", flag_string( imm_flags, pMob->imm_flags ) );

    ch_printf( ch, "Res:         [%s]\r\n", flag_string( res_flags, pMob->res_flags ) );

    ch_printf( ch, "Vuln:        [%s]\r\n", flag_string( vuln_flags, pMob->vuln_flags ) );

    ch_printf( ch, "Off:         [%s]\r\n", flag_string( off_flags, pMob->off_flags ) );

    ch_printf( ch, "Size:        [%s]\r\n", flag_string( size_flags, pMob->size ) );

    ch_printf( ch, "Material:    [%s]\r\n", pMob->material );

    ch_printf( ch, "Start pos.   [%s]\r\n",
               flag_string( position_flags, pMob->start_pos ) );

    ch_printf( ch, "Default pos  [%s]\r\n",
               flag_string( position_flags, pMob->default_pos ) );

    ch_printf( ch, "Wealth:      [%5d]\r\n", pMob->wealth );

/* ROM values end */

    if ( pMob->spec_fun )
    {
        ch_printf( ch, "Spec fun:    [%s]\r\n", spec_name( pMob->spec_fun ) );
    }

    ch_printf( ch, "Short descr: %s\r\nLong descr:\r\n%s",
               pMob->short_descr, pMob->long_descr );

    ch_printf( ch, "Description:\r\n%s", pMob->description );

    if ( pMob->pShop )
    {
        SHOP_DATA              *pShop = NULL;
        int                     iTrade = 0;

        pShop = pMob->pShop;

        ch_printf( ch,
                   "Shop data for [%5d]:\r\n"
                   "  Markup for purchaser: %d%%\r\n"
                   "  Markdown for seller:  %d%%\r\n",
                   pShop->keeper, pShop->profit_buy, pShop->profit_sell );
        ch_printf( ch, "  Hours: %d to %d.\r\n", pShop->open_hour, pShop->close_hour );

        for ( iTrade = 0; iTrade < MAX_TRADE; iTrade++ )
        {
            if ( pShop->buy_type[iTrade] != 0 )
            {
                if ( iTrade == 0 )
                {
                    ch_printf( ch, "  Number Trades Type\r\n" );
                    ch_printf( ch, "  ------ -----------\r\n" );
                }
                ch_printf( ch, "  [%4d] %s\r\n", iTrade,
                           flag_string( type_flags, pShop->buy_type[iTrade] ) );
            }
        }
    }

    if ( pMob->mprogs )
    {
        int                     cnt;

        ch_printf( ch, "\r\nMOBPrograms for [%5d]:\r\n", pMob->vnum );

        for ( cnt = 0, list = pMob->mprogs; list; list = list->next )
        {
            if ( cnt == 0 )
            {
                ch_printf( ch, " Number Vnum Trigger Phrase\r\n" );
                ch_printf( ch, " ------ ---- ------- ------\r\n" );
            }

            ch_printf( ch, "[%5d] %4d %7s %s\r\n", cnt,
                       list->vnum, mprog_type_to_name( list->trig_type ),
                       list->trig_phrase );
            cnt++;
        }
    }

    return false;
}

bool medit_create( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;
    AREA_DATA              *pArea = NULL;
    int                     value = 0;
    int                     iHash = 0;

    value = atoi( argument );
    if ( argument[0] == '\0' || value == 0 )
    {
        ch_printf( ch, "Syntax:  medit create [vnum]\r\n" );
        return false;
    }

    pArea = get_vnum_area( value );

    if ( !pArea )
    {
        ch_printf( ch, "MEdit:  That vnum is not assigned an area.\r\n" );
        return false;
    }

    if ( !IS_BUILDER( ch, pArea ) )
    {
        ch_printf( ch, "MEdit:  Vnum in an area you cannot build in.\r\n" );
        return false;
    }

    if ( get_mob_index( value ) )
    {
        ch_printf( ch, "MEdit:  Mobile vnum already exists.\r\n" );
        return false;
    }

    pMob = new_mob_index(  );
    pMob->vnum = value;
    pMob->area = pArea;

    if ( value > top_vnum_mob )
        top_vnum_mob = value;

    pMob->act = ACT_IS_NPC;
    iHash = value % MAX_KEY_HASH;
    pMob->next = mob_index_hash[iHash];
    mob_index_hash[iHash] = pMob;
    ch->desc->pEdit = ( void * ) pMob;

    ch_printf( ch, "Mobile Created.\r\n" );
    return true;
}

bool medit_spec( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;

    EDIT_MOB( ch, pMob );

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  spec [special function]\r\n" );
        return false;
    }

    if ( !str_cmp( argument, "none" ) )
    {
        pMob->spec_fun = NULL;

        ch_printf( ch, "Spec removed.\r\n" );
        return true;
    }

    if ( spec_lookup( argument ) )
    {
        pMob->spec_fun = spec_lookup( argument );
        ch_printf( ch, "Spec set.\r\n" );
        return true;
    }

    ch_printf( ch, "MEdit: No such special function.\r\n" );
    return false;
}

bool medit_damtype( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;

    EDIT_MOB( ch, pMob );

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  damtype [damage message]\r\n" );
        ch_printf( ch, "To see a list of types of messages, enter '? weapon'.\r\n" );
        return false;
    }

    pMob->dam_type = attack_lookup( argument );
    ch_printf( ch, "Damage type set.\r\n" );
    return true;
}

bool medit_align( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;

    EDIT_MOB( ch, pMob );

    if ( argument[0] == '\0' || !is_number( argument ) )
    {
        ch_printf( ch, "Syntax:  alignment [number]\r\n" );
        return false;
    }

    pMob->alignment = atoi( argument );

    ch_printf( ch, "Alignment set.\r\n" );
    return true;
}

bool medit_level( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;

    EDIT_MOB( ch, pMob );

    if ( argument[0] == '\0' || !is_number( argument ) )
    {
        ch_printf( ch, "Syntax:  level [number]\r\n" );
        return false;
    }

    pMob->level = atoi( argument );

    ch_printf( ch, "Level set.\r\n" );
    return true;
}

bool medit_desc( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;

    EDIT_MOB( ch, pMob );

    if ( argument[0] == '\0' )
    {
        string_append( ch, &pMob->description );
        return true;
    }

    ch_printf( ch, "Syntax:  desc    - line edit\r\n" );
    return false;
}

bool medit_long( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;
    char                    local_argument[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    EDIT_MOB( ch, pMob );

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  long [string]\r\n" );
        return false;
    }

    strcpy( local_argument, argument );

    free_string( pMob->long_descr );
    strcat( local_argument, "\r\n" );
    pMob->long_descr = str_dup( local_argument );
    pMob->long_descr[0] = UPPER( pMob->long_descr[0] );

    ch_printf( ch, "Long description set.\r\n" );
    return true;
}

bool medit_short( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;

    EDIT_MOB( ch, pMob );

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  short [string]\r\n" );
        return false;
    }

    free_string( pMob->short_descr );
    pMob->short_descr = str_dup( argument );

    ch_printf( ch, "Short description set.\r\n" );
    return true;
}

bool medit_name( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;

    EDIT_MOB( ch, pMob );

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  name [string]\r\n" );
        return false;
    }

    free_string( pMob->player_name );
    pMob->player_name = str_dup( argument );

    ch_printf( ch, "Name set.\r\n" );
    return true;
}

bool medit_shop( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;
    char                    command[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";

    argument = one_argument( argument, command );
    argument = one_argument( argument, arg1 );

    EDIT_MOB( ch, pMob );

    if ( command[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  shop hours [opening] [closing]\r\n" );
        ch_printf( ch, "         shop profit [buying%%] [selling%%]\r\n" );
        ch_printf( ch, "         shop type [0-4] [item type]\r\n" );
        ch_printf( ch, "         shop assign\r\n" );
        ch_printf( ch, "         shop remove\r\n" );
        return false;
    }

    if ( !str_cmp( command, "hours" ) )
    {
        if ( arg1[0] == '\0' || !is_number( arg1 )
             || argument[0] == '\0' || !is_number( argument ) )
        {
            ch_printf( ch, "Syntax:  shop hours [opening] [closing]\r\n" );
            return false;
        }

        if ( !pMob->pShop )
        {
            ch_printf( ch, "MEdit:  You must first create a shop (shop assign).\r\n" );
            return false;
        }

        pMob->pShop->open_hour = atoi( arg1 );
        pMob->pShop->close_hour = atoi( argument );

        ch_printf( ch, "Shop hours set.\r\n" );
        return true;
    }

    if ( !str_cmp( command, "profit" ) )
    {
        if ( arg1[0] == '\0' || !is_number( arg1 )
             || argument[0] == '\0' || !is_number( argument ) )
        {
            ch_printf( ch, "Syntax:  shop profit [buying%%] [selling%%]\r\n" );
            return false;
        }

        if ( !pMob->pShop )
        {
            ch_printf( ch, "MEdit:  You must first create a shop (shop assign).\r\n" );
            return false;
        }

        pMob->pShop->profit_buy = atoi( arg1 );
        pMob->pShop->profit_sell = atoi( argument );

        ch_printf( ch, "Shop profit set.\r\n" );
        return true;
    }

    if ( !str_cmp( command, "type" ) )
    {
        int                     value = 0;

        if ( arg1[0] == '\0' || !is_number( arg1 ) || argument[0] == '\0' )
        {
            ch_printf( ch, "Syntax:  shop type [0-4] [item type]\r\n" );
            return false;
        }

        if ( atoi( arg1 ) >= MAX_TRADE )
        {
            ch_printf( ch, "MEdit:  May sell %d items max.\r\n", MAX_TRADE );
            return false;
        }

        if ( !pMob->pShop )
        {
            ch_printf( ch, "MEdit:  You must first create a shop (shop assign).\r\n" );
            return false;
        }

        if ( ( value = flag_value( type_flags, argument ) ) == NO_FLAG )
        {
            ch_printf( ch, "MEdit:  That type of item is not known.\r\n" );
            return false;
        }

        pMob->pShop->buy_type[atoi( arg1 )] = value;

        ch_printf( ch, "Shop type set.\r\n" );
        return true;
    }

    /*
     * shop assign && shop delete by Phoenix 
     */

    if ( !str_prefix( command, "assign" ) )
    {
        if ( pMob->pShop )
        {
            ch_printf( ch, "Mob already has a shop assigned to it.\r\n" );
            return false;
        }

        pMob->pShop = new_shop(  );
        if ( !shop_first )
            shop_first = pMob->pShop;
        if ( shop_last )
            shop_last->next = pMob->pShop;
        shop_last = pMob->pShop;

        pMob->pShop->keeper = pMob->vnum;

        ch_printf( ch, "New shop assigned to mobile.\r\n" );
        return true;
    }

    if ( !str_prefix( command, "remove" ) )
    {
        SHOP_DATA              *pShop = NULL;

        pShop = pMob->pShop;
        pMob->pShop = NULL;

        if ( pShop == shop_first )
        {
            if ( !pShop->next )
            {
                shop_first = NULL;
                shop_last = NULL;
            }
            else
                shop_first = pShop->next;
        }
        else
        {
            SHOP_DATA              *ipShop = NULL;

            for ( ipShop = shop_first; ipShop; ipShop = ipShop->next )
            {
                if ( ipShop->next == pShop )
                {
                    if ( !pShop->next )
                    {
                        shop_last = ipShop;
                        shop_last->next = NULL;
                    }
                    else
                        ipShop->next = pShop->next;
                }
            }
        }

        free_shop( pShop );

        ch_printf( ch, "Mobile is no longer a shopkeeper.\r\n" );
        return true;
    }

    medit_shop( ch, "" );
    return false;
}

/* ROM medit functions: */

bool medit_sex( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;
    int                     value = 0;

    if ( argument[0] != '\0' )
    {
        EDIT_MOB( ch, pMob );

        if ( ( value = flag_value( sex_flags, argument ) ) != NO_FLAG )
        {
            pMob->sex = value;

            ch_printf( ch, "Sex set.\r\n" );
            return true;
        }
    }

    ch_printf( ch, "Syntax: sex [sex]\r\n" "Type '? sex' for a list of flags.\r\n" );
    return false;
}

bool medit_act( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;
    int                     value = 0;

    if ( argument[0] != '\0' )
    {
        EDIT_MOB( ch, pMob );

        if ( ( value = flag_value( act_flags, argument ) ) != NO_FLAG )
        {
            pMob->act ^= value;
            SET_BIT( pMob->act, ACT_IS_NPC );

            ch_printf( ch, "Act flag toggled.\r\n" );
            return true;
        }
    }

    ch_printf( ch, "Syntax: act [flag]\r\n" "Type '? act' for a list of flags.\r\n" );
    return false;
}

bool medit_affect( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;
    int                     value = 0;

    if ( argument[0] != '\0' )
    {
        EDIT_MOB( ch, pMob );

        if ( ( value = flag_value( affect_flags, argument ) ) != NO_FLAG )
        {
            pMob->affected_by ^= value;

            ch_printf( ch, "Affect flag toggled.\r\n" );
            return true;
        }
    }

    ch_printf( ch, "Syntax: affect [flag]\r\n"
               "Type '? affect' for a list of flags.\r\n" );
    return false;
}

bool medit_armor( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    int                     pierce = 0;
    int                     bash = 0;
    int                     slash = 0;
    int                     exotic = 0;

    do                                                 /* So that I can use break and
                                                        * send the syntax in one place */
    {
        if ( argument[0] == '\0' )
            break;

        EDIT_MOB( ch, pMob );
        argument = one_argument( argument, arg );

        if ( !is_number( arg ) )
            break;
        pierce = atoi( arg );
        argument = one_argument( argument, arg );

        if ( arg[0] != '\0' )
        {
            if ( !is_number( arg ) )
                break;
            bash = atoi( arg );
            argument = one_argument( argument, arg );
        }
        else
            bash = pMob->ac[AC_BASH];

        if ( arg[0] != '\0' )
        {
            if ( !is_number( arg ) )
                break;
            slash = atoi( arg );
            argument = one_argument( argument, arg );
        }
        else
            slash = pMob->ac[AC_SLASH];

        if ( arg[0] != '\0' )
        {
            if ( !is_number( arg ) )
                break;
            exotic = atoi( arg );
        }
        else
            exotic = pMob->ac[AC_EXOTIC];

        pMob->ac[AC_PIERCE] = pierce;
        pMob->ac[AC_BASH] = bash;
        pMob->ac[AC_SLASH] = slash;
        pMob->ac[AC_EXOTIC] = exotic;

        ch_printf( ch, "AC set.\r\n" );
        return true;
    }
    while ( false );                                   /* Just do it once.. */

    ch_printf( ch, "Syntax:  armor [ac-pierce [ac-bash [ac-slash [ac-exotic]]]]\r\n"
               "help MOB_AC  gives a list of reasonable ac-values.\r\n" );
    return false;
}

bool medit_form( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;
    int                     value = 0;

    if ( argument[0] != '\0' )
    {
        EDIT_MOB( ch, pMob );

        if ( ( value = flag_value( form_flags, argument ) ) != NO_FLAG )
        {
            pMob->form ^= value;
            ch_printf( ch, "Form toggled.\r\n" );
            return true;
        }
    }

    ch_printf( ch, "Syntax: form [flags]\r\n" "Type '? form' for a list of flags.\r\n" );
    return false;
}

bool medit_part( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;
    int                     value = 0;

    if ( argument[0] != '\0' )
    {
        EDIT_MOB( ch, pMob );

        if ( ( value = flag_value( part_flags, argument ) ) != NO_FLAG )
        {
            pMob->parts ^= value;
            ch_printf( ch, "Parts toggled.\r\n" );
            return true;
        }
    }

    ch_printf( ch, "Syntax: part [flags]\r\n" "Type '? part' for a list of flags.\r\n" );
    return false;
}

bool medit_imm( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;
    int                     value = 0;

    if ( argument[0] != '\0' )
    {
        EDIT_MOB( ch, pMob );

        if ( ( value = flag_value( imm_flags, argument ) ) != NO_FLAG )
        {
            pMob->imm_flags ^= value;
            ch_printf( ch, "Immunity toggled.\r\n" );
            return true;
        }
    }

    ch_printf( ch, "Syntax: imm [flags]\r\n" "Type '? imm' for a list of flags.\r\n" );
    return false;
}

bool medit_res( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;
    int                     value = 0;

    if ( argument[0] != '\0' )
    {
        EDIT_MOB( ch, pMob );

        if ( ( value = flag_value( res_flags, argument ) ) != NO_FLAG )
        {
            pMob->res_flags ^= value;
            ch_printf( ch, "Resistance toggled.\r\n" );
            return true;
        }
    }

    ch_printf( ch, "Syntax: res [flags]\r\n" "Type '? res' for a list of flags.\r\n" );
    return false;
}

bool medit_vuln( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;
    int                     value = 0;

    if ( argument[0] != '\0' )
    {
        EDIT_MOB( ch, pMob );

        if ( ( value = flag_value( vuln_flags, argument ) ) != NO_FLAG )
        {
            pMob->vuln_flags ^= value;
            ch_printf( ch, "Vulnerability toggled.\r\n" );
            return true;
        }
    }

    ch_printf( ch, "Syntax: vuln [flags]\r\n" "Type '? vuln' for a list of flags.\r\n" );
    return false;
}

bool medit_material( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;

    EDIT_MOB( ch, pMob );

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  material [string]\r\n" );
        return false;
    }

    free_string( pMob->material );
    pMob->material = str_dup( argument );

    ch_printf( ch, "Material set.\r\n" );
    return true;
}

bool medit_off( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;
    int                     value = 0;

    if ( argument[0] != '\0' )
    {
        EDIT_MOB( ch, pMob );

        if ( ( value = flag_value( off_flags, argument ) ) != NO_FLAG )
        {
            pMob->off_flags ^= value;
            ch_printf( ch, "Offensive behaviour toggled.\r\n" );
            return true;
        }
    }

    ch_printf( ch, "Syntax: off [flags]\r\n" "Type '? off' for a list of flags.\r\n" );
    return false;
}

bool medit_size( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;
    int                     value = 0;

    if ( argument[0] != '\0' )
    {
        EDIT_MOB( ch, pMob );

        if ( ( value = flag_value( size_flags, argument ) ) != NO_FLAG )
        {
            pMob->size = value;
            ch_printf( ch, "Size set.\r\n" );
            return true;
        }
    }

    ch_printf( ch, "Syntax: size [size]\r\n" "Type '? size' for a list of sizes.\r\n" );
    return false;
}

bool medit_hitdice( CHAR_DATA *ch, const char *argument )
{
    static char             syntax[] = "Syntax:  hitdice <number> d <type> + <bonus>\r\n";
    const char             *num = NULL;
    char                   *type = NULL;
    char                   *bonus = NULL;
    char                    cp_loc[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                   *cp = cp_loc;
    MOB_INDEX_DATA         *pMob = NULL;

    EDIT_MOB( ch, pMob );

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "%s", syntax );
        return false;
    }

    strcpy( cp_loc, argument );
    num = argument;

    while ( isdigit( *cp ) )
        ++cp;
    while ( *cp != '\0' && !isdigit( *cp ) )
        *( cp++ ) = '\0';

    type = cp;

    while ( isdigit( *cp ) )
        ++cp;
    while ( *cp != '\0' && !isdigit( *cp ) )
        *( cp++ ) = '\0';

    bonus = cp;

    while ( isdigit( *cp ) )
        ++cp;
    if ( *cp != '\0' )
        *cp = '\0';

    if ( ( !is_number( num ) || atoi( num ) < 1 )
         || ( !is_number( type ) || atoi( type ) < 1 )
         || ( !is_number( bonus ) || atoi( bonus ) < 0 ) )
    {
        ch_printf( ch, "%s", syntax );
        return false;
    }

    pMob->hit[DICE_NUMBER] = atoi( num );
    pMob->hit[DICE_TYPE] = atoi( type );
    pMob->hit[DICE_BONUS] = atoi( bonus );

    ch_printf( ch, "Hitdice set.\r\n" );
    return true;
}

bool medit_manadice( CHAR_DATA *ch, const char *argument )
{
    static char             syntax[] =
        "Syntax:  manadice <number> d <type> + <bonus>\r\n";
    const char             *num = NULL;
    char                   *type = NULL;
    char                   *bonus = NULL;
    char                    cp_loc[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                   *cp = cp_loc;
    MOB_INDEX_DATA         *pMob = NULL;

    EDIT_MOB( ch, pMob );

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "%s", syntax );
        return false;
    }

    strcpy( cp_loc, argument );
    num = argument;

    while ( isdigit( *cp ) )
        ++cp;
    while ( *cp != '\0' && !isdigit( *cp ) )
        *( cp++ ) = '\0';

    type = cp;

    while ( isdigit( *cp ) )
        ++cp;
    while ( *cp != '\0' && !isdigit( *cp ) )
        *( cp++ ) = '\0';

    bonus = cp;

    while ( isdigit( *cp ) )
        ++cp;
    if ( *cp != '\0' )
        *cp = '\0';

    if ( !( is_number( num ) && is_number( type ) && is_number( bonus ) ) )
    {
        ch_printf( ch, "%s", syntax );
        return false;
    }

    if ( ( !is_number( num ) || atoi( num ) < 1 )
         || ( !is_number( type ) || atoi( type ) < 1 )
         || ( !is_number( bonus ) || atoi( bonus ) < 0 ) )
    {
        ch_printf( ch, "%s", syntax );
        return false;
    }

    pMob->mana[DICE_NUMBER] = atoi( num );
    pMob->mana[DICE_TYPE] = atoi( type );
    pMob->mana[DICE_BONUS] = atoi( bonus );

    ch_printf( ch, "Manadice set.\r\n" );
    return true;
}

bool medit_damdice( CHAR_DATA *ch, const char *argument )
{
    static char             syntax[] = "Syntax:  damdice <number> d <type> + <bonus>\r\n";
    const char             *num = NULL;
    char                   *type = NULL;
    char                   *bonus = NULL;
    char                    cp_loc[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                   *cp = cp_loc;
    MOB_INDEX_DATA         *pMob = NULL;

    EDIT_MOB( ch, pMob );

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "%s", syntax );
        return false;
    }

    strcpy( cp_loc, argument );
    num = argument;

    while ( isdigit( *cp ) )
        ++cp;
    while ( *cp != '\0' && !isdigit( *cp ) )
        *( cp++ ) = '\0';

    type = cp;

    while ( isdigit( *cp ) )
        ++cp;
    while ( *cp != '\0' && !isdigit( *cp ) )
        *( cp++ ) = '\0';

    bonus = cp;

    while ( isdigit( *cp ) )
        ++cp;
    if ( *cp != '\0' )
        *cp = '\0';

    if ( !( is_number( num ) && is_number( type ) && is_number( bonus ) ) )
    {
        ch_printf( ch, "%s", syntax );
        return false;
    }

    if ( ( !is_number( num ) || atoi( num ) < 1 )
         || ( !is_number( type ) || atoi( type ) < 1 )
         || ( !is_number( bonus ) || atoi( bonus ) < 0 ) )
    {
        ch_printf( ch, "%s", syntax );
        return false;
    }

    pMob->damage[DICE_NUMBER] = atoi( num );
    pMob->damage[DICE_TYPE] = atoi( type );
    pMob->damage[DICE_BONUS] = atoi( bonus );

    ch_printf( ch, "Damdice set.\r\n" );
    return true;
}

bool medit_race( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;
    int                     race = 0;

    if ( argument[0] != '\0' && ( race = race_lookup( argument ) ) != 0 )
    {
        EDIT_MOB( ch, pMob );

        pMob->race = race;
        pMob->act |= race_table[race].act;
        pMob->affected_by |= race_table[race].aff;
        pMob->off_flags |= race_table[race].off;
        pMob->imm_flags |= race_table[race].imm;
        pMob->res_flags |= race_table[race].res;
        pMob->vuln_flags |= race_table[race].vuln;
        pMob->form |= race_table[race].form;
        pMob->parts |= race_table[race].parts;

        ch_printf( ch, "Race set.\r\n" );
        return true;
    }

    if ( argument[0] == '?' )
    {

        ch_printf( ch, "Available races are:" );

        for ( race = 0; race_table[race].name != NULL; race++ )
        {
            if ( ( race % 3 ) == 0 )
                send_to_char( "\r\n", ch );
            ch_printf( ch, " %-15s", race_table[race].name );
        }

        ch_printf( ch, "\r\n" );
        return false;
    }

    ch_printf( ch, "Syntax:  race [race]\r\n" "Type 'race ?' for a list of races.\r\n" );
    return false;
}

bool medit_position( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;
    char                    arg[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    int                     value = 0;

    argument = one_argument( argument, arg );

    switch ( arg[0] )
    {
        default:
            break;

        case 'S':
        case 's':
            if ( str_prefix( arg, "start" ) )
                break;

            if ( ( value = flag_value( position_flags, argument ) ) == NO_FLAG )
                break;

            EDIT_MOB( ch, pMob );

            pMob->start_pos = value;
            ch_printf( ch, "Start position set.\r\n" );
            return true;

        case 'D':
        case 'd':
            if ( str_prefix( arg, "default" ) )
                break;

            if ( ( value = flag_value( position_flags, argument ) ) == NO_FLAG )
                break;

            EDIT_MOB( ch, pMob );

            pMob->default_pos = value;
            ch_printf( ch, "Default position set.\r\n" );
            return true;
    }

    ch_printf( ch, "Syntax:  position [start/default] [position]\r\n"
               "Type '? position' for a list of positions.\r\n" );
    return false;
}

bool medit_gold( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;

    EDIT_MOB( ch, pMob );

    if ( argument[0] == '\0' || !is_number( argument ) )
    {
        ch_printf( ch, "Syntax:  wealth [number]\r\n" );
        return false;
    }

    pMob->wealth = atoi( argument );

    ch_printf( ch, "Wealth set.\r\n" );
    return true;
}

bool medit_hitroll( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;

    EDIT_MOB( ch, pMob );

    if ( argument[0] == '\0' || !is_number( argument ) )
    {
        ch_printf( ch, "Syntax:  hitroll [number]\r\n" );
        return false;
    }

    pMob->hitroll = atoi( argument );

    ch_printf( ch, "Hitroll set.\r\n" );
    return true;
}

void show_liqlist( CHAR_DATA *ch )
{
    int                     liq = 0;
    BUFFER                 *buffer = NULL;
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    buffer = new_buf(  );

    for ( liq = 0; liq_table[liq].liq_name != NULL; liq++ )
    {
        if ( ( liq % 21 ) == 0 )
            add_buf( buffer,
                     "Name                 Color          Proof Full Thirst Food Ssize\r\n" );

        sprintf( buf, "%-20s %-14s %5d %4d %6d %4d %5d\r\n",
                 liq_table[liq].liq_name, liq_table[liq].liq_color,
                 liq_table[liq].liq_affect[0], liq_table[liq].liq_affect[1],
                 liq_table[liq].liq_affect[2], liq_table[liq].liq_affect[3],
                 liq_table[liq].liq_affect[4] );
        add_buf( buffer, buf );
    }

    page_to_char( buf_string( buffer ), ch );
    free_buf( buffer );

    return;
}

void show_damlist( CHAR_DATA *ch )
{
    int                     att = 0;
    BUFFER                 *buffer = NULL;
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    buffer = new_buf(  );

    for ( att = 0; attack_table[att].name != NULL; att++ )
    {
        if ( ( att % 21 ) == 0 )
            add_buf( buffer, "Name                 Noun\r\n" );

        sprintf( buf, "%-20s %-20s\r\n", attack_table[att].name, attack_table[att].noun );
        add_buf( buffer, buf );
    }

    page_to_char( buf_string( buffer ), ch );
    free_buf( buffer );

    return;
}

bool medit_group( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;
    MOB_INDEX_DATA         *pMTemp = NULL;
    char                    arg[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    int                     temp = 0;
    BUFFER                 *buffer = NULL;
    bool                    found = false;

    EDIT_MOB( ch, pMob );

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Syntax: group [number]\r\n" );
        ch_printf( ch, "        group show [number]\r\n" );
        return false;
    }

    if ( is_number( argument ) )
    {
        pMob->group = atoi( argument );
        ch_printf( ch, "Group set.\r\n" );
        return true;
    }

    argument = one_argument( argument, arg );

    if ( !strcmp( arg, "show" ) && is_number( argument ) )
    {
        if ( atoi( argument ) == 0 )
        {
            ch_printf( ch, "Are you crazy?\r\n" );
            return false;
        }

        buffer = new_buf(  );

        for ( temp = 0; temp < 65536; temp++ )
        {
            pMTemp = get_mob_index( temp );
            if ( pMTemp && ( pMTemp->group == atoi( argument ) ) )
            {
                found = true;
                sprintf( buf, "[%5d] %s\r\n", pMTemp->vnum, pMTemp->player_name );
                add_buf( buffer, buf );
            }
        }

        if ( found )
            page_to_char( buf_string( buffer ), ch );
        else
            ch_printf( ch, "No mobs in that group.\r\n" );

        free_buf( buffer );
        return false;
    }

    return false;
}

bool redit_owner( CHAR_DATA *ch, const char *argument )
{
    ROOM_INDEX_DATA        *pRoom = NULL;

    EDIT_ROOM( ch, pRoom );

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  owner [owner]\r\n" );
        ch_printf( ch, "         owner none\r\n" );
        return false;
    }

    free_string( pRoom->owner );
    if ( !str_cmp( argument, "none" ) )
        pRoom->owner = str_dup( "" );
    else
        pRoom->owner = str_dup( argument );

    ch_printf( ch, "Owner set.\r\n" );
    return true;
}

bool medit_addmprog( CHAR_DATA *ch, const char *argument )
{
    int                     value = 0;
    MOB_INDEX_DATA         *pMob = NULL;
    MPROG_LIST             *list = NULL;
    MPROG_CODE             *code = NULL;
    char                    trigger[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    phrase[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    num[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    EDIT_MOB( ch, pMob );
    argument = one_argument( argument, num );
    argument = one_argument( argument, trigger );
    argument = one_argument( argument, phrase );

    if ( !is_number( num ) || trigger[0] == '\0' || phrase[0] == '\0' )
    {
        ch_printf( ch, "Syntax:   addmprog [vnum] [trigger] [phrase]\r\n" );
        return false;
    }

    if ( ( value = flag_value( mprog_flags, trigger ) ) == NO_FLAG )
    {
        ch_printf( ch, "Valid flags are:\r\n" );
        show_help( ch, "mprog" );
        return false;
    }

    if ( ( code = get_mprog_index( atoi( num ) ) ) == NULL )
    {
        ch_printf( ch, "No such MOBProgram.\r\n" );
        return false;
    }

    list = new_mprog(  );
    list->vnum = atoi( num );
    list->trig_type = value;
    list->trig_phrase = str_dup( phrase );
    list->code = code->code;
    SET_BIT( pMob->mprog_flags, value );
    list->next = pMob->mprogs;
    pMob->mprogs = list;

    ch_printf( ch, "Mprog Added.\r\n" );
    return true;
}

bool medit_delmprog( CHAR_DATA *ch, const char *argument )
{
    MOB_INDEX_DATA         *pMob = NULL;
    MPROG_LIST             *list = NULL;
    MPROG_LIST             *list_next = NULL;
    char                    mprog[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    int                     value = 0;
    int                     cnt = 0;

    EDIT_MOB( ch, pMob );

    one_argument( argument, mprog );
    if ( !is_number( mprog ) || mprog[0] == '\0' )
    {
        ch_printf( ch, "Syntax:  delmprog [#mprog]\r\n" );
        return false;
    }

    value = atoi( mprog );

    if ( value < 0 )
    {
        ch_printf( ch, "Only non-negative mprog-numbers allowed.\r\n" );
        return false;
    }

    if ( !( list = pMob->mprogs ) )
    {
        ch_printf( ch, "MEdit:  Non existant mprog.\r\n" );
        return false;
    }

    if ( value == 0 )
    {
        REMOVE_BIT( pMob->mprog_flags, pMob->mprogs->trig_type );
        list = pMob->mprogs;
        pMob->mprogs = list->next;
        free_mprog( list );
    }
    else
    {
        while ( ( list_next = list->next ) && ( ++cnt < value ) )
            list = list_next;

        if ( list_next )
        {
            REMOVE_BIT( pMob->mprog_flags, list_next->trig_type );
            list->next = list_next->next;
            free_mprog( list_next );
        }
        else
        {
            ch_printf( ch, "No such mprog.\r\n" );
            return false;
        }
    }

    ch_printf( ch, "Mprog removed.\r\n" );
    return true;
}

bool redit_room( CHAR_DATA *ch, const char *argument )
{
    ROOM_INDEX_DATA        *room = NULL;
    int                     value = 0;

    EDIT_ROOM( ch, room );

    if ( ( value = flag_value( room_flags, argument ) ) == NO_FLAG )
    {
        ch_printf( ch, "Syntax: room [flags]\r\n" );
        return false;
    }

    TOGGLE_BIT( room->room_flags, value );
    ch_printf( ch, "Room flags toggled.\r\n" );
    return true;
}

bool redit_sector( CHAR_DATA *ch, const char *argument )
{
    ROOM_INDEX_DATA        *room = NULL;
    int                     value = 0;

    EDIT_ROOM( ch, room );

    if ( ( value = flag_value( sector_flags, argument ) ) == NO_FLAG )
    {
        ch_printf( ch, "Syntax sector [type]\r\n" );
        return false;
    }

    room->sector_type = value;
    ch_printf( ch, "Sector type set.\r\n" );

    return true;
}
