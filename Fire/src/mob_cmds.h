/*
 * RAM $Id: mob_cmds.h 67 2009-01-05 00:39:32Z quixadhal $
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
*        ROM 2.4 is copyright 1993-1995 Russ Taylor                        *
*        ROM has been brought to you by the ROM consortium                 *
*            Russ Taylor (rtaylor@pacinfo.com)                             *
*            Gabrielle Taylor (gtaylor@pacinfo.com)                        *
*            Brian Moore (rom@rom.efn.org)                                 *
*        By using this code, you have agreed to follow the terms of the    *
*        ROM license, in the file Rom24/doc/rom.license                    *
****************************************************************************/

/***************************************************************************
 *                                                                         *
 *  Based on MERC 2.2 MOBprograms by N'Atas-ha.                            *
 *  Written and adapted to ROM 2.4 by                                      *
 *          Markku Nylander (markku.nylander@uta.fi)                       *
 *                                                                         *
 ***************************************************************************/

struct mob_cmd_type
{
    const char             *const name;
    void                    ( *do_fun ) ( CHAR_DATA *ch, const char *arguments );
};

/* the command table itself */
extern const struct mob_cmd_type mob_cmd_table[];

/*
 * Command functions.
 * Defined in mob_cmds.c
 */
void                    do_mpasound( CHAR_DATA *ch, const char *argument );
void                    do_mpgecho( CHAR_DATA *ch, const char *argument );
void                    do_mpzecho( CHAR_DATA *ch, const char *argument );
void                    do_mpkill( CHAR_DATA *ch, const char *argument );
void                    do_mpassist( CHAR_DATA *ch, const char *argument );
void                    do_mpjunk( CHAR_DATA *ch, const char *argument );
void                    do_mpechoaround( CHAR_DATA *ch, const char *argument );
void                    do_mpecho( CHAR_DATA *ch, const char *argument );
void                    do_mpechoat( CHAR_DATA *ch, const char *argument );
void                    do_mpmload( CHAR_DATA *ch, const char *argument );
void                    do_mpoload( CHAR_DATA *ch, const char *argument );
void                    do_mppurge( CHAR_DATA *ch, const char *argument );
void                    do_mpgoto( CHAR_DATA *ch, const char *argument );
void                    do_mpat( CHAR_DATA *ch, const char *argument );
void                    do_mptransfer( CHAR_DATA *ch, const char *argument );
void                    do_mpgtransfer( CHAR_DATA *ch, const char *argument );
void                    do_mpforce( CHAR_DATA *ch, const char *argument );
void                    do_mpgforce( CHAR_DATA *ch, const char *argument );
void                    do_mpvforce( CHAR_DATA *ch, const char *argument );
void                    do_mpcast( CHAR_DATA *ch, const char *argument );
void                    do_mpdamage( CHAR_DATA *ch, const char *argument );
void                    do_mpremember( CHAR_DATA *ch, const char *argument );
void                    do_mpforget( CHAR_DATA *ch, const char *argument );
void                    do_mpdelay( CHAR_DATA *ch, const char *argument );
void                    do_mpcancel( CHAR_DATA *ch, const char *argument );
void                    do_mpcall( CHAR_DATA *ch, const char *argument );
void                    do_mpflee( CHAR_DATA *ch, const char *argument );
void                    do_mpotransfer( CHAR_DATA *ch, const char *argument );
void                    do_mpremove( CHAR_DATA *ch, const char *argument );
