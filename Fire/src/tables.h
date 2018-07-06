/*
 * RAM $Id: tables.h 67 2009-01-05 00:39:32Z quixadhal $
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
*           Russ Taylor (rtaylor@efn.org)                                  *
*           Russ Taylor (rtaylor@hypercube.org)                            *
*           Gabrielle Taylor (gtaylor@hypercube.org)                       *
*       By using this code, you have agreed to follow the terms of the     *
*       ROM license, in the file Rom24/doc/rom.license                     *
***************************************************************************/

struct flag_type
{
    const char             *name;
    int                     bit;
    bool                    settable;
};

struct clan_type
{
    const char             *name;
    const char             *who_name;
    int                     hall;
    bool                    independent;               /* true for loners */
};

struct position_type
{
    const char             *name;
    const char             *short_name;
};

struct sex_type
{
    const char             *name;
};

struct size_type
{
    const char             *name;
};

struct flag_stat_type
{
    const struct flag_type *structure;
    bool                    stat;
};

struct bit_type
{
    const struct flag_type *table;
    const char             *help;
};

/* game tables */
extern const struct clan_type clan_table[MAX_CLAN];
extern const struct position_type position_table[];
extern const struct sex_type sex_table[];
extern const struct size_type size_table[];

/* flag tables */
extern const struct flag_type act_flags[];
extern const struct flag_type plr_flags[];
extern const struct flag_type affect_flags[];
extern const struct flag_type off_flags[];
extern const struct flag_type imm_flags[];
extern const struct flag_type form_flags[];
extern const struct flag_type part_flags[];
extern const struct flag_type comm_flags[];

/* This appears to not be implemented, yet. */
/* extern const struct flag_type weapon_flags[]; */

extern const struct flag_type extra_flags[];
extern const struct flag_type wear_flags[];
extern const struct flag_type room_flags[];
extern const struct flag_type container_flags[];
extern const struct flag_type portal_flags[];
extern const struct flag_type exit_flags[];

/* These used to be in const.c */

extern const struct str_app_type str_app[26];
extern const struct int_app_type int_app[26];
extern const struct wis_app_type wis_app[26];
extern const struct dex_app_type dex_app[26];
extern const struct con_app_type con_app[26];
extern const struct class_type class_table[MAX_CLASS];
extern const struct weapon_type weapon_table[];
extern const struct item_type item_table[];
extern const struct wiznet_type wiznet_table[];
extern const struct attack_type attack_table[];
extern const struct race_type race_table[];
extern const struct pc_race_type pc_race_table[];
extern const struct liq_type liq_table[];
extern struct skill_type skill_table[MAX_SKILL];
extern const struct group_type group_table[MAX_GROUP];
extern const char      *title_table[MAX_CLASS][MAX_LEVEL + 1][2];

/* This is for OLC */
extern const struct flag_stat_type flag_stat_table[];
extern const struct flag_type mprog_flags[];
extern const struct flag_type area_flags[];
extern const struct flag_type sector_flags[];
extern const struct flag_type door_resets[];
extern const struct flag_type wear_loc_strings[];
extern const struct flag_type wear_loc_flags[];
extern const struct flag_type res_flags[];
extern const struct flag_type vuln_flags[];
extern const struct flag_type type_flags[];
extern const struct flag_type apply_flags[];
extern const struct flag_type sex_flags[];
extern const struct flag_type furniture_flags[];
extern const struct flag_type weapon_class[];
extern const struct flag_type apply_types[];
extern const struct flag_type weapon_type2[];
extern const struct flag_type size_flags[];
extern const struct flag_type position_flags[];
extern const struct flag_type ac_type[];
extern const struct bit_type bitvector_type[];

int                     flag_lookup( const char *name,
                                     const struct flag_type *flag_table );
int                     clan_lookup( const char *name );
int                     position_lookup( const char *name );
int                     sex_lookup( const char *name );
int                     size_lookup( const char *name );

/* These are also for OLC */
bool                    is_stat( const struct flag_type *flag_table );
int                     flag_value( const struct flag_type *flag_table,
                                    const char *argument );
const char             *flag_string( const struct flag_type *flag_table, int bits );
HELP_DATA              *help_lookup( const char *keyword );
HELP_AREA              *had_lookup( const char *arg );
int                     race_lookup( const char *name );
int                     item_lookup( const char *name );
int                     liq_lookup( const char *name );
