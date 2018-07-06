/*
 * RAM $Id: interp.h 67 2009-01-05 00:39:32Z quixadhal $
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

/* this is a listing of all the commands and command related data */

/* for command types */
#define ML        MAX_LEVEL                            /* implementor */
#define L1        MAX_LEVEL - 1                        /* creator */
#define L2        MAX_LEVEL - 2                        /* supreme being */
#define L3        MAX_LEVEL - 3                        /* deity */
#define L4        MAX_LEVEL - 4                        /* god */
#define L5        MAX_LEVEL - 5                        /* immortal */
#define L6        MAX_LEVEL - 6                        /* demigod */
#define L7        MAX_LEVEL - 7                        /* angel */
#define L8        MAX_LEVEL - 8                        /* avatar */
#define IM        LEVEL_IMMORTAL                       /* avatar */
#define HE        LEVEL_HERO                           /* hero */

#define COM_INGORE        1

/*
 * Structure for a command in the command lookup table.
 */
struct cmd_type
{
    const char             *name;
    DO_FUN                 *do_fun;
    int                     position;
    int                     level;
    int                     log;
    int                     show;
};

/* Why is this in interp again??? */
extern bool             fLogAll;

/* the command table itself */
extern const struct cmd_type cmd_table[];

void                    interpret( CHAR_DATA *ch, const char *argument );
void                    do_function( CHAR_DATA *ch, DO_FUN *do_fun,
                                     const char *argument );
bool                    check_social( CHAR_DATA *ch, const char *command,
                                      const char *argument );
bool                    is_number( const char *arg );
int                     number_argument( const char *argument, char *arg );
int                     mult_argument( const char *argument, char *arg );
const char             *one_argument( const char *argument, char *arg_first );
void                    do_commands( CHAR_DATA *ch, const char *argument );
void                    do_wizhelp( CHAR_DATA *ch, const char *argument );

/*
 * Command functions.
 * Defined in act_*.c (mostly).
 */

/* act_comm.c */
void                    do_delet( CHAR_DATA *ch, const char *argument );
void                    do_delete( CHAR_DATA *ch, const char *argument );
void                    do_channels( CHAR_DATA *ch, const char *argument );
void                    do_deaf( CHAR_DATA *ch, const char *argument );
void                    do_quiet( CHAR_DATA *ch, const char *argument );
void                    do_afk( CHAR_DATA *ch, const char *argument );
void                    do_replay( CHAR_DATA *ch, const char *argument );
void                    do_auction( CHAR_DATA *ch, const char *argument );
void                    do_gossip( CHAR_DATA *ch, const char *argument );
void                    do_grats( CHAR_DATA *ch, const char *argument );
void                    do_quote( CHAR_DATA *ch, const char *argument );
void                    do_question( CHAR_DATA *ch, const char *argument );
void                    do_answer( CHAR_DATA *ch, const char *argument );
void                    do_clantalk( CHAR_DATA *ch, const char *argument );
void                    do_immtalk( CHAR_DATA *ch, const char *argument );
void                    do_say( CHAR_DATA *ch, const char *argument );
void                    do_shout( CHAR_DATA *ch, const char *argument );
void                    do_tell( CHAR_DATA *ch, const char *argument );
void                    do_reply( CHAR_DATA *ch, const char *argument );
void                    do_yell( CHAR_DATA *ch, const char *argument );
void                    do_emote( CHAR_DATA *ch, const char *argument );
void                    do_pmote( CHAR_DATA *ch, const char *argument );
void                    do_pose( CHAR_DATA *ch, const char *argument );
void                    do_bug( CHAR_DATA *ch, const char *argument );
void                    do_typo( CHAR_DATA *ch, const char *argument );
void                    do_rent( CHAR_DATA *ch, const char *argument );
void                    do_qui( CHAR_DATA *ch, const char *argument );
void                    do_quit( CHAR_DATA *ch, const char *argument );
void                    do_save( CHAR_DATA *ch, const char *argument );
void                    do_follow( CHAR_DATA *ch, const char *argument );
void                    do_order( CHAR_DATA *ch, const char *argument );
void                    do_group( CHAR_DATA *ch, const char *argument );
void                    do_split( CHAR_DATA *ch, const char *argument );
void                    do_gtell( CHAR_DATA *ch, const char *argument );

/* act_info.c */
void                    do_scroll( CHAR_DATA *ch, const char *argument );
void                    do_socials( CHAR_DATA *ch, const char *argument );
void                    do_motd( CHAR_DATA *ch, const char *argument );
void                    do_imotd( CHAR_DATA *ch, const char *argument );
void                    do_rules( CHAR_DATA *ch, const char *argument );
void                    do_story( CHAR_DATA *ch, const char *argument );
void                    do_wizlist( CHAR_DATA *ch, const char *argument );
void                    do_autolist( CHAR_DATA *ch, const char *argument );
void                    do_autoassist( CHAR_DATA *ch, const char *argument );
void                    do_autoexit( CHAR_DATA *ch, const char *argument );
void                    do_autogold( CHAR_DATA *ch, const char *argument );
void                    do_autoloot( CHAR_DATA *ch, const char *argument );
void                    do_autosac( CHAR_DATA *ch, const char *argument );
void                    do_autosplit( CHAR_DATA *ch, const char *argument );
void                    do_brief( CHAR_DATA *ch, const char *argument );
void                    do_compact( CHAR_DATA *ch, const char *argument );
void                    do_show( CHAR_DATA *ch, const char *argument );
void                    do_prompt( CHAR_DATA *ch, const char *argument );
void                    do_combine( CHAR_DATA *ch, const char *argument );
void                    do_noloot( CHAR_DATA *ch, const char *argument );
void                    do_nofollow( CHAR_DATA *ch, const char *argument );
void                    do_nosummon( CHAR_DATA *ch, const char *argument );
void                    do_look( CHAR_DATA *ch, const char *argument );
void                    do_read( CHAR_DATA *ch, const char *argument );
void                    do_examine( CHAR_DATA *ch, const char *argument );
void                    do_exits( CHAR_DATA *ch, const char *argument );
void                    do_worth( CHAR_DATA *ch, const char *argument );
void                    do_score( CHAR_DATA *ch, const char *argument );
void                    do_affects( CHAR_DATA *ch, const char *argument );
void                    do_time( CHAR_DATA *ch, const char *argument );
void                    do_weather( CHAR_DATA *ch, const char *argument );
void                    do_help( CHAR_DATA *ch, const char *argument );
void                    do_whois( CHAR_DATA *ch, const char *argument );
void                    do_who( CHAR_DATA *ch, const char *argument );
void                    do_count( CHAR_DATA *ch, const char *argument );
void                    do_inventory( CHAR_DATA *ch, const char *argument );
void                    do_equipment( CHAR_DATA *ch, const char *argument );
void                    do_compare( CHAR_DATA *ch, const char *argument );
void                    do_credits( CHAR_DATA *ch, const char *argument );
void                    do_where( CHAR_DATA *ch, const char *argument );
void                    do_consider( CHAR_DATA *ch, const char *argument );
void                    do_title( CHAR_DATA *ch, const char *argument );
void                    do_description( CHAR_DATA *ch, const char *argument );
void                    do_report( CHAR_DATA *ch, const char *argument );
void                    do_practice( CHAR_DATA *ch, const char *argument );
void                    do_wimpy( CHAR_DATA *ch, const char *argument );
void                    do_password( CHAR_DATA *ch, const char *argument );

/* act_move.c */
void                    do_north( CHAR_DATA *ch, const char *argument );
void                    do_east( CHAR_DATA *ch, const char *argument );
void                    do_south( CHAR_DATA *ch, const char *argument );
void                    do_west( CHAR_DATA *ch, const char *argument );
void                    do_up( CHAR_DATA *ch, const char *argument );
void                    do_down( CHAR_DATA *ch, const char *argument );
void                    do_open( CHAR_DATA *ch, const char *argument );
void                    do_close( CHAR_DATA *ch, const char *argument );
void                    do_lock( CHAR_DATA *ch, const char *argument );
void                    do_unlock( CHAR_DATA *ch, const char *argument );
void                    do_pick( CHAR_DATA *ch, const char *argument );
void                    do_stand( CHAR_DATA *ch, const char *argument );
void                    do_rest( CHAR_DATA *ch, const char *argument );
void                    do_sit( CHAR_DATA *ch, const char *argument );
void                    do_sleep( CHAR_DATA *ch, const char *argument );
void                    do_wake( CHAR_DATA *ch, const char *argument );
void                    do_sneak( CHAR_DATA *ch, const char *argument );
void                    do_hide( CHAR_DATA *ch, const char *argument );
void                    do_visible( CHAR_DATA *ch, const char *argument );
void                    do_recall( CHAR_DATA *ch, const char *argument );
void                    do_train( CHAR_DATA *ch, const char *argument );
void                    do_enter( CHAR_DATA *ch, const char *argument );

/* act_obj.c */
void                    do_get( CHAR_DATA *ch, const char *argument );
void                    do_put( CHAR_DATA *ch, const char *argument );
void                    do_drop( CHAR_DATA *ch, const char *argument );
void                    do_give( CHAR_DATA *ch, const char *argument );
void                    do_envenom( CHAR_DATA *ch, const char *argument );
void                    do_fill( CHAR_DATA *ch, const char *argument );
void                    do_pour( CHAR_DATA *ch, const char *argument );
void                    do_drink( CHAR_DATA *ch, const char *argument );
void                    do_eat( CHAR_DATA *ch, const char *argument );
void                    do_wear( CHAR_DATA *ch, const char *argument );
void                    do_remove( CHAR_DATA *ch, const char *argument );
void                    do_sacrifice( CHAR_DATA *ch, const char *argument );
void                    do_quaff( CHAR_DATA *ch, const char *argument );
void                    do_recite( CHAR_DATA *ch, const char *argument );
void                    do_brandish( CHAR_DATA *ch, const char *argument );
void                    do_zap( CHAR_DATA *ch, const char *argument );
void                    do_steal( CHAR_DATA *ch, const char *argument );
void                    do_buy( CHAR_DATA *ch, const char *argument );
void                    do_list( CHAR_DATA *ch, const char *argument );
void                    do_sell( CHAR_DATA *ch, const char *argument );
void                    do_value( CHAR_DATA *ch, const char *argument );

/* act_wiz.c */
void                    do_wiznet( CHAR_DATA *ch, const char *argument );
void                    do_guild( CHAR_DATA *ch, const char *argument );
void                    do_outfit( CHAR_DATA *ch, const char *argument );
void                    do_nochannels( CHAR_DATA *ch, const char *argument );
void                    do_smote( CHAR_DATA *ch, const char *argument );
void                    do_bamfin( CHAR_DATA *ch, const char *argument );
void                    do_bamfout( CHAR_DATA *ch, const char *argument );
void                    do_deny( CHAR_DATA *ch, const char *argument );
void                    do_disconnect( CHAR_DATA *ch, const char *argument );
void                    do_pardon( CHAR_DATA *ch, const char *argument );
void                    do_echo( CHAR_DATA *ch, const char *argument );
void                    do_recho( CHAR_DATA *ch, const char *argument );
void                    do_zecho( CHAR_DATA *ch, const char *argument );
void                    do_pecho( CHAR_DATA *ch, const char *argument );
void                    do_transfer( CHAR_DATA *ch, const char *argument );
void                    do_at( CHAR_DATA *ch, const char *argument );
void                    do_goto( CHAR_DATA *ch, const char *argument );
void                    do_violate( CHAR_DATA *ch, const char *argument );
void                    do_stat( CHAR_DATA *ch, const char *argument );
void                    do_rstat( CHAR_DATA *ch, const char *argument );
void                    do_ostat( CHAR_DATA *ch, const char *argument );
void                    do_mstat( CHAR_DATA *ch, const char *argument );
void                    do_vnum( CHAR_DATA *ch, const char *argument );
void                    do_mfind( CHAR_DATA *ch, const char *argument );
void                    do_ofind( CHAR_DATA *ch, const char *argument );
void                    do_owhere( CHAR_DATA *ch, const char *argument );
void                    do_mwhere( CHAR_DATA *ch, const char *argument );
void                    do_reboo( CHAR_DATA *ch, const char *argument );
void                    do_reboot( CHAR_DATA *ch, const char *argument );
void                    do_shutdow( CHAR_DATA *ch, const char *argument );
void                    do_shutdown( CHAR_DATA *ch, const char *argument );
void                    do_protect( CHAR_DATA *ch, const char *argument );
void                    do_snoop( CHAR_DATA *ch, const char *argument );
void                    do_switch( CHAR_DATA *ch, const char *argument );
void                    do_return( CHAR_DATA *ch, const char *argument );
void                    do_clone( CHAR_DATA *ch, const char *argument );
void                    do_load( CHAR_DATA *ch, const char *argument );
void                    do_mload( CHAR_DATA *ch, const char *argument );
void                    do_oload( CHAR_DATA *ch, const char *argument );
void                    do_purge( CHAR_DATA *ch, const char *argument );
void                    do_advance( CHAR_DATA *ch, const char *argument );
void                    do_trust( CHAR_DATA *ch, const char *argument );
void                    do_restore( CHAR_DATA *ch, const char *argument );
void                    do_freeze( CHAR_DATA *ch, const char *argument );
void                    do_log( CHAR_DATA *ch, const char *argument );
void                    do_noemote( CHAR_DATA *ch, const char *argument );
void                    do_noshout( CHAR_DATA *ch, const char *argument );
void                    do_notell( CHAR_DATA *ch, const char *argument );
void                    do_peace( CHAR_DATA *ch, const char *argument );
void                    do_wizlock( CHAR_DATA *ch, const char *argument );
void                    do_newlock( CHAR_DATA *ch, const char *argument );
void                    do_slookup( CHAR_DATA *ch, const char *argument );
void                    do_set( CHAR_DATA *ch, const char *argument );
void                    do_sset( CHAR_DATA *ch, const char *argument );
void                    do_mset( CHAR_DATA *ch, const char *argument );
void                    do_string( CHAR_DATA *ch, const char *argument );
void                    do_oset( CHAR_DATA *ch, const char *argument );
void                    do_rset( CHAR_DATA *ch, const char *argument );
void                    do_sockets( CHAR_DATA *ch, const char *argument );
void                    do_force( CHAR_DATA *ch, const char *argument );
void                    do_invis( CHAR_DATA *ch, const char *argument );
void                    do_incognito( CHAR_DATA *ch, const char *argument );
void                    do_holylight( CHAR_DATA *ch, const char *argument );
void                    do_prefi( CHAR_DATA *ch, const char *argument );
void                    do_prefix( CHAR_DATA *ch, const char *argument );
void                    do_mob( CHAR_DATA *ch, const char *argument );  /* OLC */
void                    do_mpstat( CHAR_DATA *ch, const char *argument );       /* OLC */
void                    do_mpdump( CHAR_DATA *ch, const char *argument );       /* OLC */

/* alias.c */
void                    do_alia( CHAR_DATA *ch, const char *argument );
void                    do_alias( CHAR_DATA *ch, const char *argument );
void                    do_unalias( CHAR_DATA *ch, const char *argument );

/* ban.c */
void                    do_ban( CHAR_DATA *ch, const char *argument );
void                    do_permban( CHAR_DATA *ch, const char *argument );
void                    do_allow( CHAR_DATA *ch, const char *argument );

/* db.c */
void                    do_areas( CHAR_DATA *ch, const char *argument );
void                    do_memory( CHAR_DATA *ch, const char *argument );
void                    do_dump( CHAR_DATA *ch, const char *argument );

/* fight.c */
void                    do_berserk( CHAR_DATA *ch, const char *argument );
void                    do_bash( CHAR_DATA *ch, const char *argument );
void                    do_dirt( CHAR_DATA *ch, const char *argument );
void                    do_trip( CHAR_DATA *ch, const char *argument );
void                    do_kill( CHAR_DATA *ch, const char *argument );
void                    do_murde( CHAR_DATA *ch, const char *argument );
void                    do_murder( CHAR_DATA *ch, const char *argument );
void                    do_backstab( CHAR_DATA *ch, const char *argument );
void                    do_flee( CHAR_DATA *ch, const char *argument );
void                    do_rescue( CHAR_DATA *ch, const char *argument );
void                    do_kick( CHAR_DATA *ch, const char *argument );
void                    do_disarm( CHAR_DATA *ch, const char *argument );
void                    do_sla( CHAR_DATA *ch, const char *argument );
void                    do_slay( CHAR_DATA *ch, const char *argument );
void                    do_surrender( CHAR_DATA *ch, const char *argument );    /* OLC */

/* healer.c */
void                    do_heal( CHAR_DATA *ch, const char *argument );

/* magic.c */
void                    do_cast( CHAR_DATA *ch, const char *argument );

/* note.c */
void                    do_unread( CHAR_DATA *ch, const char *argument );
void                    do_note( CHAR_DATA *ch, const char *argument );
void                    do_idea( CHAR_DATA *ch, const char *argument );
void                    do_penalty( CHAR_DATA *ch, const char *argument );
void                    do_news( CHAR_DATA *ch, const char *argument );
void                    do_changes( CHAR_DATA *ch, const char *argument );

/* scan.c */
void                    do_scan( CHAR_DATA *ch, const char *argument );

/* skills.c */
void                    do_gain( CHAR_DATA *ch, const char *argument );
void                    do_spells( CHAR_DATA *ch, const char *argument );
void                    do_skills( CHAR_DATA *ch, const char *argument );
void                    do_groups( CHAR_DATA *ch, const char *argument );

/* tables.c */
void                    do_flag( CHAR_DATA *ch, const char *argument );

/* OLC */
void                    do_olc( CHAR_DATA *ch, const char *argument );
void                    do_asave( CHAR_DATA *ch, const char *argument );
void                    do_alist( CHAR_DATA *ch, const char *argument );
void                    do_resets( CHAR_DATA *ch, const char *argument );
void                    do_redit( CHAR_DATA *ch, const char *argument );
void                    do_aedit( CHAR_DATA *ch, const char *argument );
void                    do_medit( CHAR_DATA *ch, const char *argument );
void                    do_oedit( CHAR_DATA *ch, const char *argument );
void                    do_mpedit( CHAR_DATA *ch, const char *argument );
void                    do_hedit( CHAR_DATA *ch, const char *argument );
