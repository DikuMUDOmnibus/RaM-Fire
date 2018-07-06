/*
 * RAM $Id: act.h 67 2009-01-05 00:39:32Z quixadhal $
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
 **************************************************************************/

/***************************************************************************
*       ROM 2.4 is copyright 1993-1998 Russ Taylor                         *
*       ROM has been brought to you by the ROM consortium                  *
*           Russ Taylor (rtaylor@hypercube.org)                            *
*           Gabrielle Taylor (gtaylor@hypercube.org)                       *
*           Brian Moore (zump@rom.org)                                     *
*       By using this code, you have agreed to follow the terms of the     *
*       ROM license, in the file Rom24/doc/rom.license                     *
***************************************************************************/

/* act_comm.c */
struct pose_table_type
{
    const char             *message[2 * MAX_CLASS];
};
extern const struct pose_table_type pose_table[];

/* act_info.c */
extern const char      *where_name[];
extern int              max_on;
extern const char      *day_name[];
extern const char      *month_name[];
extern const char      *distance[4];

/* act_move.c */
extern const char      *dir_name[];
extern const char      *dir_abbrev[];
extern const int        rev_dir[];
extern const int        movement_loss[SECT_MAX];

/* act_comm.c */
void                    add_follower( CHAR_DATA *ch, CHAR_DATA *master );
void                    stop_follower( CHAR_DATA *ch );
void                    nuke_pets( CHAR_DATA *ch );
void                    die_follower( CHAR_DATA *ch );
bool                    is_same_group( const CHAR_DATA *ach, const CHAR_DATA *bch );

/* act_info.c */
char                   *format_obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch, bool fShort );
void                    show_list_to_char( OBJ_DATA *list, CHAR_DATA *ch, bool fShort,
                                           bool fShowNothing );
void                    show_char_to_char_0( CHAR_DATA *victim, CHAR_DATA *ch );
void                    show_char_to_char_1( CHAR_DATA *victim, CHAR_DATA *ch );
void                    show_char_to_char( CHAR_DATA *list, CHAR_DATA *ch );
bool                    check_blind( CHAR_DATA *ch );
void                    set_title( CHAR_DATA *ch, const char *title, ... )
    __attribute__ ( ( format( printf, 2, 3 ) ) );
void                    scan_list( ROOM_INDEX_DATA *scan_room, CHAR_DATA *ch, int depth,
                                   int door );
void                    scan_char( CHAR_DATA *victim, CHAR_DATA *ch, int depth,
                                   int door );

/* act_move.c */
void                    move_char( CHAR_DATA *ch, int door, bool follow );
int                     find_door( CHAR_DATA *ch, char *arg );
bool                    has_key( CHAR_DATA *ch, int key );
ROOM_INDEX_DATA        *get_random_room( CHAR_DATA *ch );

/* act_obj.c */
bool                    can_loot( CHAR_DATA *ch, OBJ_DATA *obj );
void                    get_obj( CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *container );
bool                    remove_obj( CHAR_DATA *ch, int iWear, bool fReplace );
void                    wear_obj( CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace );
CHAR_DATA              *find_keeper( CHAR_DATA *ch );
void                    obj_to_keeper( OBJ_DATA *obj, CHAR_DATA *ch );
OBJ_DATA               *get_obj_keeper( CHAR_DATA *ch, CHAR_DATA *keeper,
                                        char *argument );
int                     get_cost( CHAR_DATA *keeper, OBJ_DATA *obj, bool fBuy );

/* act_wiz.c */
ROOM_INDEX_DATA        *find_location( CHAR_DATA *ch, const char *arg );
bool                    obj_check( CHAR_DATA *ch, OBJ_DATA *obj );
void                    recursive_clone( CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *clone );
