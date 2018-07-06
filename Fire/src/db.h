/*
 * RAM $Id: db.h 69 2009-01-11 18:13:26Z quixadhal $
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

/* macro for flag swapping */
#define GET_UNSET(flag1,flag2)        (~(flag1)&((flag1)|(flag2)))

extern HELP_DATA       *help_first;
extern HELP_DATA       *help_last;
extern HELP_AREA       *had_list;

extern SHOP_DATA       *shop_first;
extern SHOP_DATA       *shop_last;

extern AREA_DATA       *area_first;
extern AREA_DATA       *area_last;

extern char             bug_buf[2 * MAX_INPUT_LENGTH];
extern char             log_buf[2 * MAX_INPUT_LENGTH];

extern CHAR_DATA       *char_list;
extern char            *help_greeting;
extern KILL_DATA        kill_table[MAX_LEVEL];
extern OBJ_DATA        *object_list;
extern TIME_INFO_DATA   time_info;
extern WEATHER_DATA     weather_info;
extern bool             MOBtrigger;

extern MOB_INDEX_DATA  *mob_index_hash[MAX_KEY_HASH];
extern OBJ_INDEX_DATA  *obj_index_hash[MAX_KEY_HASH];
extern ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];
extern char            *string_hash[MAX_KEY_HASH];

extern int              top_affect;
extern int              top_area;
extern int              top_ed;
extern int              top_exit;
extern int              top_help;
extern int              top_mob_index;
extern int              top_obj_index;
extern int              top_reset;
extern int              top_room;
extern int              top_shop;
extern int              mobile_count;
extern int              newmobs;
extern int              newobjs;

extern int              top_vnum_mob;
extern int              top_vnum_obj;
extern int              top_vnum_room;

extern bool             fBootDb;
extern FILE            *fpArea;
extern char             strArea[MAX_INPUT_LENGTH];
extern struct social_type social_table[MAX_SOCIALS];
extern int              social_count;

/* These are actually elsewhere, but those files don't have headers */
NOTE_DATA              *new_note( void );
void                    free_note( NOTE_DATA *note );
DESCRIPTOR_DATA        *new_descriptor( void );
void                    free_descriptor( DESCRIPTOR_DATA *d );

/* extra descr recycling */
extern EXTRA_DESCR_DATA *extra_descr_free;
EXTRA_DESCR_DATA       *new_extra_descr( void );
void                    free_extra_descr( EXTRA_DESCR_DATA *ed );

/* object recycling */
extern OBJ_DATA        *obj_free;
OBJ_DATA               *new_obj( void );
void                    free_obj( OBJ_DATA *obj );

/* character recyling */
extern CHAR_DATA       *char_free;
extern PC_DATA         *pcdata_free;
CHAR_DATA              *new_char( void );
void                    free_char( CHAR_DATA *ch );
PC_DATA                *new_pcdata( void );
void                    free_pcdata( PC_DATA *pcdata );

/* mob id and memory procedures */
extern int              last_pc_id;
extern int              last_mob_id;
extern MEM_DATA        *mem_data_free;
int                     get_pc_id( void );
int                     get_mob_id( void );
MEM_DATA               *new_mem_data( void );
void                    free_mem_data( MEM_DATA *memory );

/* OLC */
extern RESET_DATA      *reset_free;
RESET_DATA             *new_reset_data( void );
void                    free_reset_data( RESET_DATA *pReset );
void                    new_reset( ROOM_INDEX_DATA *pR, RESET_DATA *pReset );

extern AREA_DATA       *area_free;
AREA_DATA              *new_area( void );
void                    free_area( AREA_DATA *pArea );

extern EXIT_DATA       *exit_free;
EXIT_DATA              *new_exit( void );
void                    free_exit( EXIT_DATA *pExit );

extern ROOM_INDEX_DATA *room_index_free;
ROOM_INDEX_DATA        *new_room_index( void );
void                    free_room_index( ROOM_INDEX_DATA *pRoom );

extern SHOP_DATA       *shop_free;
SHOP_DATA              *new_shop( void );
void                    free_shop( SHOP_DATA *pShop );

extern OBJ_INDEX_DATA  *obj_index_free;
OBJ_INDEX_DATA         *new_obj_index( void );
void                    free_obj_index( OBJ_INDEX_DATA *pObj );

extern MOB_INDEX_DATA  *mob_index_free;
MOB_INDEX_DATA         *new_mob_index( void );
void                    free_mob_index( MOB_INDEX_DATA *pMob );

extern MPROG_CODE      *mpcode_free;
MPROG_CODE             *new_mpcode( void );
void                    free_mpcode( MPROG_CODE *pMcode );

void                    boot_db( void );
void                    load_area_file( FILE * fp );
void                    load_ram_area_file( FILE * fp, int version );
void                    load_rom_area_file( FILE * fp );

void                    assign_area_vnum( int vnum );  /* OLC */
void                    load_area( FILE * fp );
void                    new_load_area( FILE * fp );    /* OLC */

/* void                    load_helps( FILE * fp ); */
void                    load_helps( FILE * fp, const char *fname );
void                    load_mobprogs( FILE * fp );    /* OLC */
void                    fix_mobprogs( void );          /* OLC */
void                    reset_area( AREA_DATA *pArea ); /* OLC */
void                    reset_room( ROOM_INDEX_DATA *pRoom );   /* OLC */
MPROG_CODE             *get_mprog_index( int vnum );

HELP_AREA              *new_had( void );
HELP_DATA              *new_help( void );
void                    free_help( HELP_DATA * );

void                    convert_mobile( MOB_INDEX_DATA *pMobIndex );    /* OLC ROM */
void                    convert_objects( void );       /* OLC ROM */
void                    convert_object( OBJ_INDEX_DATA *pObjIndex );    /* OLC ROM */

void                    load_old_mob( FILE * fp );
void                    load_old_obj( FILE * fp );
void                    load_resets( FILE * fp );
void                    load_rooms( FILE * fp );
void                    load_shops( FILE * fp );
void                    load_specials( FILE * fp );
void                    fix_exits( void );
void                    area_update( void );

/* void                    reset_area( AREA_DATA *pArea ); */
CHAR_DATA              *create_mobile( MOB_INDEX_DATA *pMobIndex );
void                    clone_mobile( CHAR_DATA *parent, CHAR_DATA *clone );
OBJ_DATA               *create_object( OBJ_INDEX_DATA *pObjIndex, int level );
void                    clone_object( OBJ_DATA *parent, OBJ_DATA *clone );
void                    clear_char( CHAR_DATA *ch );
char                   *get_extra_descr( char *name, EXTRA_DESCR_DATA *ed );
MOB_INDEX_DATA         *get_mob_index( int vnum );
OBJ_INDEX_DATA         *get_obj_index( int vnum );
ROOM_INDEX_DATA        *get_room_index( int vnum );
char                    fread_letter( FILE * fp );
int                     fread_number( FILE * fp );
int                     fread_flag( FILE * fp );
int                     flag_convert( char letter );
char                   *fread_string( FILE * fp );
char                   *fread_string_eol( FILE * fp );
void                    fread_to_eol( FILE * fp );
char                   *fread_word( FILE * fp );
int                     interpolate( int level, int value_00, int value_32 );
void                    append_file( CHAR_DATA *ch, const char *file, const char *str );
void                    tail_chain( void );
void                    load_socials( FILE * fp );
void                    load_mobiles( FILE * fp );
void                    load_objects( FILE * fp );

MPROG_LIST             *new_mprog( void );
void                    free_mprog( MPROG_LIST *mp );
