/*
 * RAM $Id: olc.h 67 2009-01-05 00:39:32Z quixadhal $
 */

/***************************************************************************
 *  File: olc.h                                                            *
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
/*
 * This is a header file for all the OLC files.  Feel free to copy it into
 * merc.h if you wish.  Many of these routines may be handy elsewhere in
 * the code.  -Jason Dinkel
 */

/*
 * The version info.  Please use this info when reporting bugs.
 * It is displayed in the game by typing 'version' while editing.
 * Do not remove these from the code - by request of Jason Dinkel
 */
#define VERSION "ILAB Online Creation [Beta 1.0, ROM 2.3 modified]\r\n" \
                "     Port a ROM 2.4 v1.8\r\n"
#define AUTHOR  "     By Jason(jdinkel@mines.colorado.edu)\r\n"   \
                "     Modified for use with ROM 2.3\r\n"          \
                "     By Hans Birkeland (hansbi@ifi.uio.no)\r\n"  \
                "     Modificado para uso en ROM 2.4b6\r\n"       \
                "     Por Ivan Toledo (itoledo@ctcreuna.cl)\r\n"  \
                "     Modified for use with RaM Fire\r\n"         \
                "     By Nick Phillips (eudoxus@fastmail.fm)\r\n"
#define DATE    "     (Apr. 7, 1995 - ROM mod, Apr 16, 1995)\r\n" \
                "     (Port a ROM 2.4 - Nov 2, 1996)\r\n"         \
                "     Version actual : 1.8 - Sep 8, 1998\r\n"     \
                "     (Ported to RaM Fire - Dec 9, 2008\r\n"
#define CREDITS "     Original by Surreality(cxw197@psu.edu) and Locke(locke@lm.com)"

/*
 * New typedefs.
 */
typedef bool OLC_FUN    args( ( CHAR_DATA *ch, char *argument ) );

#define DECLARE_OLC_FUN( fun )  OLC_FUN    fun

/* Command procedures needed ROM OLC */
/*
void                    do_help( CHAR_DATA *ch, const char *argument );
void spell_null( int sn, int level, CHAR_DATA *ch, void *vo, int target );
*/

/*
 * Connected states for editor.
 */
#define ED_NONE         0
#define ED_AREA         1
#define ED_ROOM         2
#define ED_OBJECT       3
#define ED_MOBILE       4
#define ED_MPCODE       5
#define ED_HELP         6

/*
 * Interpreter Prototypes
 */
void                    aedit( CHAR_DATA *ch, const char *argument );
void                    redit( CHAR_DATA *ch, const char *argument );
void                    medit( CHAR_DATA *ch, const char *argument );
void                    oedit( CHAR_DATA *ch, const char *argument );
void                    mpedit( CHAR_DATA *ch, const char *argument );
void                    hedit( CHAR_DATA *ch, const char *argument );

/*
 * OLC Constants
 */
#define MAX_MOB 1                                      /* Default maximum number for
                                                        * resetting mobs */

/*
 * Structure for an OLC editor command.
 */
struct olc_cmd_type
{
    const char             *const name;
    bool                    ( *olc_fun ) ( CHAR_DATA *ch, const char *argument );
};

/*
 * Structure for an OLC editor startup command.
 */
struct editor_cmd_type
{
    const char             *const name;
    void                    ( *do_fun ) ( CHAR_DATA *ch, const char *argument );
};

/*
 * Utils.
 */
AREA_DATA              *get_vnum_area( int vnum );
AREA_DATA              *get_area_data( int vnum );
void                    add_reset( ROOM_INDEX_DATA *room, RESET_DATA *pReset,
                                   int reset_index );

/*
 * Interpreter Table Prototypes
 */
extern const struct olc_cmd_type aedit_table[];
extern const struct olc_cmd_type redit_table[];
extern const struct olc_cmd_type oedit_table[];
extern const struct olc_cmd_type medit_table[];
extern const struct olc_cmd_type mpedit_table[];
extern const struct olc_cmd_type hedit_table[];

/*
 * Editor Commands.
 */
/*
void                    do_aedit( CHAR_DATA *ch, const char *argument );
void                    do_redit( CHAR_DATA *ch, const char *argument );
void                    do_oedit( CHAR_DATA *ch, const char *argument );
void                    do_medit( CHAR_DATA *ch, const char *argument );
void                    do_mpedit( CHAR_DATA *ch, const char *argument );
void                    do_hedit( CHAR_DATA *ch, const char *argument );
*/

/*
 * General Functions
 */
bool run_olc_editor     args( ( DESCRIPTOR_DATA *d ) );
char                   *olc_ed_name args( ( CHAR_DATA *ch ) );
char                   *olc_ed_vnum args( ( CHAR_DATA *ch ) );

void                    show_olc_cmds( CHAR_DATA *ch,
                                       const struct olc_cmd_type *olc_table );
void                    display_resets( CHAR_DATA *ch );
bool                    show_commands( CHAR_DATA *ch, const char *argument );
bool                    show_help( CHAR_DATA *ch, const char *argument );
bool                    edit_done( CHAR_DATA *ch );
bool                    show_version( CHAR_DATA *ch, const char *argument );
void                    show_flag_cmds( CHAR_DATA *ch,
                                        const struct flag_type *flag_table );
void                    show_skill_cmds( CHAR_DATA *ch, int tar );
void                    show_spec_cmds( CHAR_DATA *ch );
bool                    check_range( int lower, int upper );
bool                    change_exit( CHAR_DATA *ch, const char *argument, int door );
int                     wear_loc( int bits, int count );
int                     wear_bit( int loc );
void                    show_obj_values( CHAR_DATA *ch, OBJ_INDEX_DATA *obj );
bool                    set_obj_values( CHAR_DATA *ch, OBJ_INDEX_DATA *pObj,
                                        int value_num, const char *argument );
bool                    set_value( CHAR_DATA *ch, OBJ_INDEX_DATA *pObj,
                                   const char *argument, int value );
bool                    oedit_values( CHAR_DATA *ch, const char *argument, int value );

/*
 * OLC Save
 */
char                   *fix_string( const char *str );
void                    save_area_list( void );
char                   *fwrite_flag( int flags, char buf[] );
void                    save_mobprogs( FILE * fp, AREA_DATA *pArea );
void                    save_mobile( FILE * fp, MOB_INDEX_DATA *pMobIndex );
void                    save_mobiles( FILE * fp, AREA_DATA *pArea );
void                    save_object( FILE * fp, OBJ_INDEX_DATA *pObjIndex );
void                    save_objects( FILE * fp, AREA_DATA *pArea );
void                    save_rooms( FILE * fp, AREA_DATA *pArea );
void                    save_specials( FILE * fp, AREA_DATA *pArea );
void                    save_door_resets( FILE * fp, AREA_DATA *pArea );
void                    save_resets( FILE * fp, AREA_DATA *pArea );
void                    save_shops( FILE * fp, AREA_DATA *pArea );
void                    save_helps( FILE * fp, HELP_AREA *ha );
void                    save_other_helps( CHAR_DATA *ch );
void                    save_area( AREA_DATA *pArea );

/*
 * String Editing Functions
 */
void                    string_edit( CHAR_DATA *ch, char **pString );
void                    string_append( CHAR_DATA *ch, char **pString );
char                   *string_replace( char *orig, const char *pOld, const char *pNew );
void                    string_add( CHAR_DATA *ch, const char *argument );
char                   *format_string( char *oldstring /* , bool fSpace */  );
const char             *first_arg( const char *argument, char *arg_first, bool fCase );
char                   *string_unpad( char *argument );
char                   *string_proper( char *argument );
char                   *string_linedel( char *string, int line );
char                   *string_lineadd( char *string, char *newstr, int line );
char                   *getline( char *str, char *buf );
char                   *numlineas( char *string );

/*
 * Area Editor Prototypes
 */
bool                    aedit_show( CHAR_DATA *ch, const char *argument );
bool                    aedit_create( CHAR_DATA *ch, const char *argument );
bool                    aedit_name( CHAR_DATA *ch, const char *argument );
bool                    aedit_file( CHAR_DATA *ch, const char *argument );
bool                    aedit_age( CHAR_DATA *ch, const char *argument );

/* DECLARE_OLC_FUN( aedit_recall );       ROM OLC */
bool                    aedit_reset( CHAR_DATA *ch, const char *argument );
bool                    aedit_security( CHAR_DATA *ch, const char *argument );
bool                    aedit_builder( CHAR_DATA *ch, const char *argument );
bool                    aedit_vnum( CHAR_DATA *ch, const char *argument );
bool                    aedit_lvnum( CHAR_DATA *ch, const char *argument );
bool                    aedit_uvnum( CHAR_DATA *ch, const char *argument );
bool                    aedit_credits( CHAR_DATA *ch, const char *argument );

/*
 * Room Editor Prototypes
 */
bool                    redit_show( CHAR_DATA *ch, const char *argument );
bool                    redit_create( CHAR_DATA *ch, const char *argument );
bool                    redit_name( CHAR_DATA *ch, const char *argument );
bool                    redit_desc( CHAR_DATA *ch, const char *argument );
bool                    redit_ed( CHAR_DATA *ch, const char *argument );
bool                    redit_format( CHAR_DATA *ch, const char *argument );
bool                    redit_north( CHAR_DATA *ch, const char *argument );
bool                    redit_south( CHAR_DATA *ch, const char *argument );
bool                    redit_east( CHAR_DATA *ch, const char *argument );
bool                    redit_west( CHAR_DATA *ch, const char *argument );
bool                    redit_up( CHAR_DATA *ch, const char *argument );
bool                    redit_down( CHAR_DATA *ch, const char *argument );
bool                    redit_mreset( CHAR_DATA *ch, const char *argument );
bool                    redit_oreset( CHAR_DATA *ch, const char *argument );
bool                    redit_mlist( CHAR_DATA *ch, const char *argument );
bool                    redit_rlist( CHAR_DATA *ch, const char *argument );
bool                    redit_olist( CHAR_DATA *ch, const char *argument );
bool                    redit_mshow( CHAR_DATA *ch, const char *argument );
bool                    redit_oshow( CHAR_DATA *ch, const char *argument );
bool                    redit_heal( CHAR_DATA *ch, const char *argument );
bool                    redit_mana( CHAR_DATA *ch, const char *argument );
bool                    redit_clan( CHAR_DATA *ch, const char *argument );
bool                    redit_owner( CHAR_DATA *ch, const char *argument );
bool                    redit_room( CHAR_DATA *ch, const char *argument );
bool                    redit_sector( CHAR_DATA *ch, const char *argument );

/*
 * Object Editor Prototypes
 */
bool                    oedit_show( CHAR_DATA *ch, const char *argument );
bool                    oedit_create( CHAR_DATA *ch, const char *argument );
bool                    oedit_name( CHAR_DATA *ch, const char *argument );
bool                    oedit_short( CHAR_DATA *ch, const char *argument );
bool                    oedit_long( CHAR_DATA *ch, const char *argument );
bool                    oedit_addaffect( CHAR_DATA *ch, const char *argument );
bool                    oedit_addapply( CHAR_DATA *ch, const char *argument );
bool                    oedit_delaffect( CHAR_DATA *ch, const char *argument );
bool                    oedit_value0( CHAR_DATA *ch, const char *argument );
bool                    oedit_value1( CHAR_DATA *ch, const char *argument );
bool                    oedit_value2( CHAR_DATA *ch, const char *argument );
bool                    oedit_value3( CHAR_DATA *ch, const char *argument );
bool                    oedit_value4( CHAR_DATA *ch, const char *argument );
bool                    oedit_weight( CHAR_DATA *ch, const char *argument );
bool                    oedit_cost( CHAR_DATA *ch, const char *argument );
bool                    oedit_ed( CHAR_DATA *ch, const char *argument );

/* ROM */
bool                    oedit_extra( CHAR_DATA *ch, const char *argument );
bool                    oedit_wear( CHAR_DATA *ch, const char *argument );
bool                    oedit_type( CHAR_DATA *ch, const char *argument );
bool                    oedit_affect( CHAR_DATA *ch, const char *argument );
bool                    oedit_material( CHAR_DATA *ch, const char *argument );
bool                    oedit_level( CHAR_DATA *ch, const char *argument );
bool                    oedit_condition( CHAR_DATA *ch, const char *argument );

/*
 * Mobile Editor Prototypes
 */
bool                    medit_show( CHAR_DATA *ch, const char *argument );
bool                    medit_create( CHAR_DATA *ch, const char *argument );
bool                    medit_name( CHAR_DATA *ch, const char *argument );
bool                    medit_short( CHAR_DATA *ch, const char *argument );
bool                    medit_long( CHAR_DATA *ch, const char *argument );
bool                    medit_shop( CHAR_DATA *ch, const char *argument );
bool                    medit_desc( CHAR_DATA *ch, const char *argument );
bool                    medit_level( CHAR_DATA *ch, const char *argument );
bool                    medit_align( CHAR_DATA *ch, const char *argument );
bool                    medit_spec( CHAR_DATA *ch, const char *argument );

/* ROM */
bool                    medit_sex( CHAR_DATA *ch, const char *argument );
bool                    medit_act( CHAR_DATA *ch, const char *argument );
bool                    medit_affect( CHAR_DATA *ch, const char *argument );
bool                    medit_armor( CHAR_DATA *ch, const char *argument );
bool                    medit_form( CHAR_DATA *ch, const char *argument );
bool                    medit_part( CHAR_DATA *ch, const char *argument );
bool                    medit_imm( CHAR_DATA *ch, const char *argument );
bool                    medit_res( CHAR_DATA *ch, const char *argument );
bool                    medit_vuln( CHAR_DATA *ch, const char *argument );
bool                    medit_material( CHAR_DATA *ch, const char *argument );
bool                    medit_off( CHAR_DATA *ch, const char *argument );
bool                    medit_size( CHAR_DATA *ch, const char *argument );
bool                    medit_hitdice( CHAR_DATA *ch, const char *argument );
bool                    medit_manadice( CHAR_DATA *ch, const char *argument );
bool                    medit_damdice( CHAR_DATA *ch, const char *argument );
bool                    medit_race( CHAR_DATA *ch, const char *argument );
bool                    medit_position( CHAR_DATA *ch, const char *argument );
bool                    medit_gold( CHAR_DATA *ch, const char *argument );
bool                    medit_hitroll( CHAR_DATA *ch, const char *argument );
bool                    medit_damtype( CHAR_DATA *ch, const char *argument );
bool                    medit_group( CHAR_DATA *ch, const char *argument );
bool                    medit_addmprog( CHAR_DATA *ch, const char *argument );
bool                    medit_delmprog( CHAR_DATA *ch, const char *argument );

/* Mobprog editor */
bool                    mpedit_create( CHAR_DATA *ch, const char *argument );
bool                    mpedit_code( CHAR_DATA *ch, const char *argument );
bool                    mpedit_show( CHAR_DATA *ch, const char *argument );
bool                    mpedit_list( CHAR_DATA *ch, const char *argument );

/* Help editors */
HELP_AREA              *get_help_area( HELP_DATA *help );
bool                    hedit_keyword( CHAR_DATA *ch, const char *argument );
bool                    hedit_text( CHAR_DATA *ch, const char *argument );
bool                    hedit_new( CHAR_DATA *ch, const char *argument );
bool                    hedit_level( CHAR_DATA *ch, const char *argument );
bool                    hedit_delete( CHAR_DATA *ch, const char *argument );
bool                    hedit_show( CHAR_DATA *ch, const char *argument );
bool                    hedit_list( CHAR_DATA *ch, const char *argument );

/*
 * Macros
 */
#define TOGGLE_BIT(var, bit)    ((var) ^= (bit))

/* Return pointers to what is being edited. */
#define EDIT_MOB(Ch, Mob)       ( Mob = (MOB_INDEX_DATA *)Ch->desc->pEdit )
#define EDIT_OBJ(Ch, Obj)       ( Obj = (OBJ_INDEX_DATA *)Ch->desc->pEdit )
#define EDIT_ROOM(Ch, Room)     ( Room = Ch->in_room )
#define EDIT_AREA(Ch, Area)     ( Area = (AREA_DATA *)Ch->desc->pEdit )
#define EDIT_MPCODE(Ch, Code)   ( Code = (MPROG_CODE*)Ch->desc->pEdit )

/*
 * Prototypes
 */

void                    show_liqlist( CHAR_DATA *ch );
void                    show_damlist( CHAR_DATA *ch );

const char             *mprog_type_to_name( int type );
