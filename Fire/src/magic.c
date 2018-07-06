/*
 * RAM $Id: magic.c 70 2009-01-11 18:47:35Z quixadhal $
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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "strings.h"
#include "random.h"
#include "db.h"
#include "act.h"
#include "interp.h"
#include "tables.h"
#include "magic.h"

int                     top_skill = 0;

/* stuff for recycling affects */
AFFECT_DATA            *affect_free = NULL;

AFFECT_DATA            *new_affect( void )
{
    static AFFECT_DATA      af_zero;
    AFFECT_DATA            *af = NULL;

    if ( affect_free == NULL )
        af = ( AFFECT_DATA * ) alloc_perm( sizeof( *af ) );
    else
    {
        af = affect_free;
        affect_free = affect_free->next;
    }

    *af = af_zero;

    VALIDATE( af );
    return af;
}

void free_affect( AFFECT_DATA *af )
{
    if ( !IS_VALID( af ) )
        return;

    INVALIDATE( af );
    af->next = affect_free;
    affect_free = af;
}

int qcmp_skill( const void *left, const void *right )
{
    const struct skill_type *a = ( const struct skill_type * ) left;
    const struct skill_type *b = ( const struct skill_type * ) right;

    if ( !a && !b )
        return 0;
    if ( a && !b )
        return -1;
    if ( !a && b )
        return 1;

    return strcasecmp( a->name, b->name );
}

void sort_skill_table( void )
{
    int                     sn = -1;

    for ( sn = 0; sn < MAX_SKILL; sn++ )
        if ( skill_table[sn].name == NULL )
            break;

    top_skill = sn - 1;

    qsort( ( void * ) ( skill_table + 1 ), top_skill, sizeof( struct skill_type ),
           ( int ( * )( const void *, const void * ) ) qcmp_skill );
}

int bsearch_skill( const char *name, int first, int top )
{
    int                     sn = -1;

    for ( ;; )
    {
        sn = ( first + top ) / 2;

        if ( sn < 0 || sn > top_skill )
            return -1;
        if ( !strcasecmp( name, skill_table[sn].name ) )
            return sn;
        if ( first >= top )
            return -1;
        if ( strcasecmp( name, skill_table[sn].name ) < 0 )
            top = sn - 1;
        else
            first = sn + 1;
    }
}

int bsearch_skill_by_prefix( const char *name, int first, int top )
{
    int                     sn = -1;

    for ( ;; )
    {
        sn = ( first + top ) / 2;

        if ( sn < 0 || sn > top_skill )
            return -1;
        if ( !str_prefix( name, skill_table[sn].name ) )
            return sn;
        if ( first >= top )
            return -1;
        if ( strcasecmp( name, skill_table[sn].name ) < 0 )
            top = sn - 1;
        else
            first = sn + 1;
    }
}

/*
 * Lookup a skill by name.
 */
int skill_lookup( const char *name )
{
    int                     sn = -1;

    if ( !name || !*name )
        return -1;

    if ( ( sn = bsearch_skill( name, 0, top_skill ) ) == -1 )
    {
        log_info( "Cannot find \"%s\" in the skill table.  Checking by prefix...", name );
        if ( ( sn = bsearch_skill_by_prefix( name, 0, top_skill ) ) == -1 )
        {
            log_error( "Cannot find \"%s\" in the skill table, at all!", name );
        }
    }

    return sn;

#if 0
    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if ( skill_table[sn].name == NULL )
            break;
        if ( LOWER( name[0] ) == LOWER( skill_table[sn].name[0] )
             && !str_prefix( name, skill_table[sn].name ) )
            return sn;
    }

    return -1;
#endif
}

int find_spell( CHAR_DATA *ch, const char *name )
{
    /*
     * finds a spell the character can cast if possible 
     */
    int                     sn = -1;
    int                     found = -1;

    if ( IS_NPC( ch ) )
        return skill_lookup( name );

    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if ( skill_table[sn].name == NULL )
            break;
        if ( LOWER( name[0] ) == LOWER( skill_table[sn].name[0] )
             && !str_prefix( name, skill_table[sn].name ) )
        {
            if ( found == -1 )
                found = sn;
            if ( ch->level >= skill_table[sn].skill_level[ch->iclass]
                 && ch->pcdata->learned[sn] > 0 )
                return sn;
        }
    }
    return found;
}

/*
 * Lookup a skill by slot number.
 * Used for object loading.
 */
int slot_lookup( int slot )
{
    int                     sn = -1;

    if ( slot <= 0 )
        return -1;

    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if ( slot == skill_table[sn].slot )
            return sn;
    }

    if ( fBootDb )
    {
        log_error( "Bad skill slot %d", slot );
        abort(  );
    }

    return -1;
}

/*
 * Utter mystical words for an sn.
 */
void say_spell( CHAR_DATA *ch, int sn )
{
    char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    buf2[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *rch = NULL;
    const char             *pName = NULL;
    int                     iSyl = 0;
    int                     length = 0;

    struct syl_type
    {
        const char             *old;
        const char             *inew;
    };

    static const struct syl_type syl_table[] = {
        {" ", " "},
        {"ar", "abra"},
        {"au", "kada"},
        {"bless", "fido"},
        {"blind", "nose"},
        {"bur", "mosa"},
        {"cu", "judi"},
        {"de", "oculo"},
        {"en", "unso"},
        {"light", "dies"},
        {"lo", "hi"},
        {"mor", "zak"},
        {"move", "sido"},
        {"ness", "lacri"},
        {"ning", "illa"},
        {"per", "duda"},
        {"ra", "gru"},
        {"fresh", "ima"},
        {"re", "candus"},
        {"son", "sabru"},
        {"tect", "infra"},
        {"tri", "cula"},
        {"ven", "nofo"},
        {"a", "a"}, {"b", "b"}, {"c", "q"}, {"d", "e"},
        {"e", "z"}, {"f", "y"}, {"g", "o"}, {"h", "p"},
        {"i", "u"}, {"j", "y"}, {"k", "t"}, {"l", "r"},
        {"m", "w"}, {"n", "i"}, {"o", "a"}, {"p", "s"},
        {"q", "d"}, {"r", "f"}, {"s", "g"}, {"t", "h"},
        {"u", "j"}, {"v", "z"}, {"w", "x"}, {"x", "n"},
        {"y", "l"}, {"z", "k"},
        {"", ""}
    };

    for ( pName = skill_table[sn].name; *pName != '\0'; pName += length )
    {
        for ( iSyl = 0; ( length = strlen( syl_table[iSyl].old ) ) != 0; iSyl++ )
        {
            if ( !str_prefix( syl_table[iSyl].old, pName ) )
            {
                strcat( buf, syl_table[iSyl].inew );
                break;
            }
        }

        if ( length == 0 )
            length = 1;
    }

    sprintf( buf2, "$n utters the words, '%s'.", buf );
    sprintf( buf, "$n utters the words, '%s'.", skill_table[sn].name );

    for ( rch = ch->in_room->people; rch; rch = rch->next_in_room )
    {
        if ( rch != ch )
            act( ( !IS_NPC( rch ) && ch->iclass == rch->iclass ) ? buf : buf2,
                 ch, NULL, rch, TO_VICT );
    }

    return;
}

/*
 * Compute a saving throw.
 * Negative apply's make saving throw better.
 */
bool saves_spell( int level, CHAR_DATA *victim, int dam_type )
{
    int                     save = 0;

    save = 50 + ( victim->level - level ) * 5 - victim->saving_throw * 2;
    if ( IS_AFFECTED( victim, AFF_BERSERK ) )
        save += victim->level / 2;

    switch ( check_immune( victim, dam_type ) )
    {
        case IS_IMMUNE:
            return true;
        case IS_RESISTANT:
            save += 2;
            break;
        case IS_VULNERABLE:
            save -= 2;
            break;
    }

    if ( !IS_NPC( victim ) && class_table[victim->iclass].fMana )
        save = 9 * save / 10;
    save = URANGE( 5, save, 95 );
    return number_percent(  ) < save;
}

/* RT save for dispels */
bool saves_dispel( int dis_level, int spell_level, int duration )
{
    int                     save = 0;

    if ( duration == -1 )
        spell_level += 5;
    /*
     * very hard to dispel permanent effects 
     */

    save = 50 + ( spell_level - dis_level ) * 5;
    save = URANGE( 5, save, 95 );
    return number_percent(  ) < save;
}

/* co-routine for dispel magic and cancellation */
bool check_dispel( int dis_level, CHAR_DATA *victim, int sn )
{
    AFFECT_DATA            *af = NULL;

    if ( is_affected( victim, sn ) )
    {
        for ( af = victim->affected; af != NULL; af = af->next )
        {
            if ( af->type == sn )
            {
                if ( !saves_dispel( dis_level, af->level, af->duration ) )
                {
                    affect_strip( victim, sn );
                    if ( skill_table[sn].msg_off )
                    {
                        ch_printf( victim, "%s\r\n", skill_table[sn].msg_off );
                    }
                    return true;
                }
                else
                    af->level--;
            }
        }
    }
    return false;
}

/* for finding mana costs -- temporary version */
int mana_cost( CHAR_DATA *ch, int min_mana, int level )
{
    if ( ch->level + 2 == level )
        return 1000;
    return UMAX( min_mana, ( 100 / ( 2 + ch->level - level ) ) );
}

/*
 * The kludgy global is for spells who want more stuff from command line.
 */
const char             *target_name = NULL;

void do_cast( CHAR_DATA *ch, const char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;
    OBJ_DATA               *obj = NULL;
    void                   *vo = NULL;
    int                     mana = 0;
    int                     sn = -1;
    int                     target = 0;

    /*
     * Switched NPC's can cast spells, but others can't.
     */
    if ( IS_NPC( ch ) && ch->desc == NULL )
        return;

    target_name = one_argument( argument, arg1 );
    one_argument( target_name, arg2 );

    if ( arg1[0] == '\0' )
    {
        ch_printf( ch, "Cast which what where?\r\n" );
        return;
    }

    if ( ( sn = find_spell( ch, arg1 ) ) < 1
         || skill_table[sn].spell_fun == spell_null
         || ( !IS_NPC( ch ) && ( ch->level < skill_table[sn].skill_level[ch->iclass]
                                 || ch->pcdata->learned[sn] == 0 ) ) )
    {
        ch_printf( ch, "You don't know any spells of that name.\r\n" );
        return;
    }

    if ( ch->position < skill_table[sn].minimum_position )
    {
        ch_printf( ch, "You can't concentrate enough.\r\n" );
        return;
    }

    if ( ch->level + 2 == skill_table[sn].skill_level[ch->iclass] )
        mana = 50;
    else
        mana = UMAX( skill_table[sn].min_mana,
                     100 / ( 2 + ch->level - skill_table[sn].skill_level[ch->iclass] ) );

    /*
     * Locate targets.
     */
    victim = NULL;
    obj = NULL;
    vo = NULL;
    target = TARGET_NONE;

    switch ( skill_table[sn].target )
    {
        default:
            log_error( "Bad target for skill number %d", sn );
            return;

        case TAR_IGNORE:
            break;

        case TAR_CHAR_OFFENSIVE:
            if ( arg2[0] == '\0' )
            {
                if ( ( victim = ch->fighting ) == NULL )
                {
                    ch_printf( ch, "Cast the spell on whom?\r\n" );
                    return;
                }
            }
            else
            {
                if ( ( victim = get_char_room( ch, target_name ) ) == NULL )
                {
                    ch_printf( ch, "They aren't here.\r\n" );
                    return;
                }
            }
/*
        if ( ch == victim )
        {
            ch_printf( ch, "You can't do that to yourself.\r\n" );
            return;
        }
*/

            if ( !IS_NPC( ch ) )
            {

                if ( is_safe( ch, victim ) && victim != ch )
                {
                    ch_printf( ch, "Not on that target.\r\n" );
                    return;
                }
                check_killer( ch, victim );
            }

            if ( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim )
            {
                ch_printf( ch, "You can't do that on your own follower.\r\n" );
                return;
            }

            vo = ( void * ) victim;
            target = TARGET_CHAR;
            break;

        case TAR_CHAR_DEFENSIVE:
            if ( arg2[0] == '\0' )
            {
                victim = ch;
            }
            else
            {
                if ( ( victim = get_char_room( ch, target_name ) ) == NULL )
                {
                    ch_printf( ch, "They aren't here.\r\n" );
                    return;
                }
            }

            vo = ( void * ) victim;
            target = TARGET_CHAR;
            break;

        case TAR_CHAR_SELF:
            if ( arg2[0] != '\0' && !is_name( target_name, ch->name ) )
            {
                ch_printf( ch, "You cannot cast this spell on another.\r\n" );
                return;
            }

            vo = ( void * ) ch;
            target = TARGET_CHAR;
            break;

        case TAR_OBJ_INV:
            if ( arg2[0] == '\0' )
            {
                ch_printf( ch, "What should the spell be cast upon?\r\n" );
                return;
            }

            if ( ( obj = get_obj_carry( ch, target_name, ch ) ) == NULL )
            {
                ch_printf( ch, "You are not carrying that.\r\n" );
                return;
            }

            vo = ( void * ) obj;
            target = TARGET_OBJ;
            break;

        case TAR_OBJ_CHAR_OFF:
            if ( arg2[0] == '\0' )
            {
                if ( ( victim = ch->fighting ) == NULL )
                {
                    ch_printf( ch, "Cast the spell on whom or what?\r\n" );
                    return;
                }

                target = TARGET_CHAR;
            }
            else if ( ( victim = get_char_room( ch, target_name ) ) != NULL )
            {
                target = TARGET_CHAR;
            }

            if ( target == TARGET_CHAR )               /* check the sanity of the attack */
            {
                if ( is_safe_spell( ch, victim, false ) && victim != ch )
                {
                    ch_printf( ch, "Not on that target.\r\n" );
                    return;
                }

                if ( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim )
                {
                    ch_printf( ch, "You can't do that on your own follower.\r\n" );
                    return;
                }

                if ( !IS_NPC( ch ) )
                    check_killer( ch, victim );

                vo = ( void * ) victim;
            }
            else if ( ( obj = get_obj_here( ch, target_name ) ) != NULL )
            {
                vo = ( void * ) obj;
                target = TARGET_OBJ;
            }
            else
            {
                ch_printf( ch, "You don't see that here.\r\n" );
                return;
            }
            break;

        case TAR_OBJ_CHAR_DEF:
            if ( arg2[0] == '\0' )
            {
                vo = ( void * ) ch;
                target = TARGET_CHAR;
            }
            else if ( ( victim = get_char_room( ch, target_name ) ) != NULL )
            {
                vo = ( void * ) victim;
                target = TARGET_CHAR;
            }
            else if ( ( obj = get_obj_carry( ch, target_name, ch ) ) != NULL )
            {
                vo = ( void * ) obj;
                target = TARGET_OBJ;
            }
            else
            {
                ch_printf( ch, "You don't see that here.\r\n" );
                return;
            }
            break;
    }

    if ( !IS_NPC( ch ) && ch->mana < mana )
    {
        ch_printf( ch, "You don't have enough mana.\r\n" );
        return;
    }

    if ( str_cmp( skill_table[sn].name, "ventriloquate" ) )
        say_spell( ch, sn );

    WAIT_STATE( ch, skill_table[sn].beats );

    if ( number_percent(  ) > get_skill( ch, sn ) )
    {
        ch_printf( ch, "You lost your concentration.\r\n" );
        check_improve( ch, sn, false, 1 );
        ch->mana -= mana / 2;
    }
    else
    {
        ch->mana -= mana;
        if ( IS_NPC( ch ) || class_table[ch->iclass].fMana )
            /*
             * class has spells 
             */
            ( *skill_table[sn].spell_fun ) ( sn, ch->level, ch, vo, target );
        else
            ( *skill_table[sn].spell_fun ) ( sn, 3 * ch->level / 4, ch, vo, target );
        check_improve( ch, sn, true, 1 );
    }

    if ( ( skill_table[sn].target == TAR_CHAR_OFFENSIVE
           || ( skill_table[sn].target == TAR_OBJ_CHAR_OFF && target == TARGET_CHAR ) )
         && victim != ch && victim->master != ch )
    {
        CHAR_DATA              *vch;
        CHAR_DATA              *vch_next;

        for ( vch = ch->in_room->people; vch; vch = vch_next )
        {
            vch_next = vch->next_in_room;
            if ( victim == vch && victim->fighting == NULL )
            {
                check_killer( victim, ch );
                multi_hit( victim, ch, TYPE_UNDEFINED );
                break;
            }
        }
    }

    return;
}

/*
 * Cast spells at targets using a magical object.
 */
void obj_cast_spell( int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj )
{
    void                   *vo = NULL;
    int                     target = TARGET_NONE;

    if ( sn <= 0 )
        return;

    if ( sn >= MAX_SKILL || skill_table[sn].spell_fun == 0 )
    {
        log_error( "Bad skill number %d", sn );
        return;
    }

    switch ( skill_table[sn].target )
    {
        default:
            log_error( "Bad target for skill number %d", sn );
            return;

        case TAR_IGNORE:
            vo = NULL;
            break;

        case TAR_CHAR_OFFENSIVE:
            if ( victim == NULL )
                victim = ch->fighting;
            if ( victim == NULL )
            {
                ch_printf( ch, "You can't do that.\r\n" );
                return;
            }
            if ( is_safe( ch, victim ) && ch != victim )
            {
                ch_printf( ch, "Something isn't right...\r\n" );
                return;
            }
            vo = ( void * ) victim;
            target = TARGET_CHAR;
            break;

        case TAR_CHAR_DEFENSIVE:
        case TAR_CHAR_SELF:
            if ( victim == NULL )
                victim = ch;
            vo = ( void * ) victim;
            target = TARGET_CHAR;
            break;

        case TAR_OBJ_INV:
            if ( obj == NULL )
            {
                ch_printf( ch, "You can't do that.\r\n" );
                return;
            }
            vo = ( void * ) obj;
            target = TARGET_OBJ;
            break;

        case TAR_OBJ_CHAR_OFF:
            if ( victim == NULL && obj == NULL )
            {
                if ( ch->fighting != NULL )
                    victim = ch->fighting;
                else
                {
                    ch_printf( ch, "You can't do that.\r\n" );
                    return;
                }
            }
            if ( victim != NULL )
            {
                if ( is_safe_spell( ch, victim, false ) && ch != victim )
                {
                    ch_printf( ch, "Somehting isn't right...\r\n" );
                    return;
                }

                vo = ( void * ) victim;
                target = TARGET_CHAR;
            }
            else
            {
                vo = ( void * ) obj;
                target = TARGET_OBJ;
            }
            break;

        case TAR_OBJ_CHAR_DEF:
            if ( victim == NULL && obj == NULL )
            {
                vo = ( void * ) ch;
                target = TARGET_CHAR;
            }
            else if ( victim != NULL )
            {
                vo = ( void * ) victim;
                target = TARGET_CHAR;
            }
            else
            {
                vo = ( void * ) obj;
                target = TARGET_OBJ;
            }

            break;
    }

    target_name = "";
    ( *skill_table[sn].spell_fun ) ( sn, level, ch, vo, target );

    if ( ( skill_table[sn].target == TAR_CHAR_OFFENSIVE
           || ( skill_table[sn].target == TAR_OBJ_CHAR_OFF && target == TARGET_CHAR ) )
         && victim != ch && victim->master != ch )
    {
        CHAR_DATA              *vch;
        CHAR_DATA              *vch_next;

        for ( vch = ch->in_room->people; vch; vch = vch_next )
        {
            vch_next = vch->next_in_room;
            if ( victim == vch && victim->fighting == NULL )
            {
                check_killer( victim, ch );
                multi_hit( victim, ch, TYPE_UNDEFINED );
                break;
            }
        }
    }

    return;
}

/*
 * Spell functions.
 */
void spell_null( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    ch_printf( ch, "That's not a spell!\r\n" );
    return;
}

void spell_acid_blast( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam = 0;

    dam = dice( level, 12 );
    if ( saves_spell( level, victim, DAM_ACID ) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_ACID, true );
    return;
}

void spell_armor( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( is_affected( victim, sn ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You are already armored.\r\n" );
        else
            act( "$N is already armored.", ch, NULL, victim, TO_CHAR );
        return;
    }
    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 24;
    af.modifier = -20;
    af.location = APPLY_AC;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    ch_printf( victim, "You feel someone protecting you.\r\n" );
    if ( ch != victim )
        act( "$N is protected by your magic.", ch, NULL, victim, TO_CHAR );
    return;
}

void spell_bless( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = NULL;
    OBJ_DATA               *obj = NULL;
    AFFECT_DATA             af;

    /*
     * deal with the object case first 
     */
    if ( target == TARGET_OBJ )
    {
        obj = ( OBJ_DATA * ) vo;
        if ( IS_OBJ_STAT( obj, ITEM_BLESS ) )
        {
            act( "$p is already blessed.", ch, obj, NULL, TO_CHAR );
            return;
        }

        if ( IS_OBJ_STAT( obj, ITEM_EVIL ) )
        {
            AFFECT_DATA            *paf = NULL;

            paf = affect_find( obj->affected, skill_lookup( "curse" ) );
            if ( !saves_dispel( level, paf != NULL ? paf->level : obj->level, 0 ) )
            {
                if ( paf != NULL )
                    affect_remove_obj( obj, paf );
                act( "$p glows a pale blue.", ch, obj, NULL, TO_ALL );
                REMOVE_BIT( obj->extra_flags, ITEM_EVIL );
                return;
            }
            else
            {
                act( "The evil of $p is too powerful for you to overcome.",
                     ch, obj, NULL, TO_CHAR );
                return;
            }
        }

        af.where = TO_OBJECT;
        af.type = sn;
        af.level = level;
        af.duration = 6 + level;
        af.location = APPLY_SAVES;
        af.modifier = -1;
        af.bitvector = ITEM_BLESS;
        affect_to_obj( obj, &af );

        act( "$p glows with a holy aura.", ch, obj, NULL, TO_ALL );

        if ( obj->wear_loc != WEAR_NONE )
            ch->saving_throw -= 1;
        return;
    }

    /*
     * character target 
     */
    victim = ( CHAR_DATA * ) vo;

    if ( victim->position == POS_FIGHTING )
    {
        if ( victim == ch )
            ch_printf( ch, "You are too busy fighting.\r\n" );
        else
            act( "$N is too busy fighting to accept a blessing.", ch, NULL, victim,
                 TO_CHAR );
        return;
    }

    if ( is_affected( victim, sn ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You are already blessed.\r\n" );
        else
            act( "$N already has divine favor.", ch, NULL, victim, TO_CHAR );
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 6 + level;
    af.location = APPLY_HITROLL;
    af.modifier = level / 8;
    af.bitvector = 0;
    affect_to_char( victim, &af );

    af.location = APPLY_SAVING_SPELL;
    af.modifier = 0 - level / 8;
    affect_to_char( victim, &af );
    ch_printf( victim, "You feel righteous.\r\n" );
    if ( ch != victim )
        act( "You grant $N the favor of your god.", ch, NULL, victim, TO_CHAR );
    return;
}

void spell_blindness( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( IS_AFFECTED( victim, AFF_BLIND ) || saves_spell( level, victim, DAM_OTHER ) )
        return;

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.location = APPLY_HITROLL;
    af.modifier = -4;
    af.duration = 1 + level;
    af.bitvector = AFF_BLIND;
    affect_to_char( victim, &af );
    ch_printf( victim, "You are blinded!\r\n" );
    act( "$n appears to be blinded.", victim, NULL, NULL, TO_ROOM );
    return;
}

void spell_burning_hands( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    static const int        dam_each[] = {
        0,
        0, 0, 0, 0, 14, 17, 20, 23, 26, 29,
        29, 29, 30, 30, 31, 31, 32, 32, 33, 33,
        34, 34, 35, 35, 36, 36, 37, 37, 38, 38,
        39, 39, 40, 40, 41, 41, 42, 42, 43, 43,
        44, 44, 45, 45, 46, 46, 47, 47, 48, 48
    };
    int                     dam = 0;

    level = UMIN( level, ( int ) ( sizeof( dam_each ) / sizeof( dam_each[0] ) ) - 1 );
    level = UMAX( 0, level );
    dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    if ( saves_spell( level, victim, DAM_FIRE ) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_FIRE, true );
    return;
}

void spell_call_lightning( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *vch = NULL;
    CHAR_DATA              *vch_next = NULL;
    int                     dam = 0;

    if ( !IS_OUTSIDE( ch ) )
    {
        ch_printf( ch, "You must be out of doors.\r\n" );
        return;
    }

    if ( weather_info.sky < SKY_RAINING )
    {
        ch_printf( ch, "You need bad weather.\r\n" );
        return;
    }

    dam = dice( level / 2, 8 );

    ch_printf( ch, "Mota's lightning strikes your foes!\r\n" );
    act( "$n calls Mota's lightning to strike $s foes!", ch, NULL, NULL, TO_ROOM );

    for ( vch = char_list; vch != NULL; vch = vch_next )
    {
        vch_next = vch->next;
        if ( vch->in_room == NULL )
            continue;
        if ( vch->in_room == ch->in_room )
        {
            if ( vch != ch && ( IS_NPC( ch ) ? !IS_NPC( vch ) : IS_NPC( vch ) ) )
                damage( ch, vch, saves_spell( level, vch, DAM_LIGHTNING )
                        ? dam / 2 : dam, sn, DAM_LIGHTNING, true );
            continue;
        }

        if ( vch->in_room->area == ch->in_room->area
             && IS_OUTSIDE( vch ) && IS_AWAKE( vch ) )
            ch_printf( vch, "Lightning flashes in the sky.\r\n" );
    }

    return;
}

/* RT calm spell stops all fighting in the room */
void spell_calm( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *vch = NULL;
    int                     mlevel = 0;
    int                     count = 0;
    int                     high_level = 0;
    int                     chance = 0;
    AFFECT_DATA             af;

    /*
     * get sum of all mobile levels in the room 
     */
    for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room )
    {
        if ( vch->position == POS_FIGHTING )
        {
            count++;
            if ( IS_NPC( vch ) )
                mlevel += vch->level;
            else
                mlevel += vch->level / 2;
            high_level = UMAX( high_level, vch->level );
        }
    }

    /*
     * compute chance of stopping combat 
     */
    chance = 4 * level - high_level + 2 * count;

    if ( IS_IMMORTAL( ch ) )                           /* always works */
        mlevel = 0;

    if ( number_range( 0, chance ) >= mlevel )         /* hard to stop large fights */
    {
        for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room )
        {
            if ( IS_NPC( vch ) && ( IS_SET( vch->imm_flags, IMM_MAGIC ) ||
                                    IS_SET( vch->act, ACT_UNDEAD ) ) )
                return;

            if ( IS_AFFECTED( vch, AFF_CALM ) || IS_AFFECTED( vch, AFF_BERSERK )
                 || is_affected( vch, skill_lookup( "frenzy" ) ) )
                return;

            ch_printf( vch, "A wave of calm passes over you.\r\n" );

            if ( vch->fighting || vch->position == POS_FIGHTING )
                stop_fighting( vch, false );

            af.where = TO_AFFECTS;
            af.type = sn;
            af.level = level;
            af.duration = level / 4;
            af.location = APPLY_HITROLL;
            if ( !IS_NPC( vch ) )
                af.modifier = -5;
            else
                af.modifier = -2;
            af.bitvector = AFF_CALM;
            affect_to_char( vch, &af );

            af.location = APPLY_DAMROLL;
            affect_to_char( vch, &af );
        }
    }
}

void spell_cancellation( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    bool                    found = false;

    level += 2;

    if ( ( !IS_NPC( ch ) && IS_NPC( victim ) &&
           !( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim ) ) ||
         ( IS_NPC( ch ) && !IS_NPC( victim ) ) )
    {
        ch_printf( ch, "You failed, try dispel magic.\r\n" );
        return;
    }

    /*
     * unlike dispel magic, the victim gets NO save 
     */

    /*
     * begin running through the spells 
     */

    if ( check_dispel( level, victim, skill_lookup( "armor" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "bless" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "blindness" ) ) )
    {
        found = true;
        act( "$n is no longer blinded.", victim, NULL, NULL, TO_ROOM );
    }

    if ( check_dispel( level, victim, skill_lookup( "calm" ) ) )
    {
        found = true;
        act( "$n no longer looks so peaceful...", victim, NULL, NULL, TO_ROOM );
    }

    if ( check_dispel( level, victim, skill_lookup( "change sex" ) ) )
    {
        found = true;
        act( "$n looks more like $mself again.", victim, NULL, NULL, TO_ROOM );
    }

    if ( check_dispel( level, victim, skill_lookup( "charm person" ) ) )
    {
        found = true;
        act( "$n regains $s free will.", victim, NULL, NULL, TO_ROOM );
    }

    if ( check_dispel( level, victim, skill_lookup( "chill touch" ) ) )
    {
        found = true;
        act( "$n looks warmer.", victim, NULL, NULL, TO_ROOM );
    }

    if ( check_dispel( level, victim, skill_lookup( "curse" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "detect evil" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "detect good" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "detect hidden" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "detect invisibility" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "detect magic" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "faerie fire" ) ) )
    {
        act( "$n's outline fades.", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "fly" ) ) )
    {
        act( "$n falls to the ground!", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "frenzy" ) ) )
    {
        act( "$n no longer looks so wild.", victim, NULL, NULL, TO_ROOM );;
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "giant strength" ) ) )
    {
        act( "$n no longer looks so mighty.", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "haste" ) ) )
    {
        act( "$n is no longer moving so quickly.", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "infravision" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "invisibility" ) ) )
    {
        act( "$n fades into existance.", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "mass invisibility" ) ) )
    {
        act( "$n fades into existance.", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "pass door" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "protection evil" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "protection good" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "sanctuary" ) ) )
    {
        act( "The white aura around $n's body vanishes.", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "shield" ) ) )
    {
        act( "The shield protecting $n vanishes.", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "sleep" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "slow" ) ) )
    {
        act( "$n is no longer moving so slowly.", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "stone skin" ) ) )
    {
        act( "$n's skin regains its normal texture.", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "weaken" ) ) )
    {
        act( "$n looks stronger.", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( found )
        ch_printf( ch, "Ok.\r\n" );
    else
        ch_printf( ch, "Spell failed.\r\n" );
}

void spell_cause_light( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    damage( ch, ( CHAR_DATA * ) vo, dice( 1, 8 ) + level / 3, sn, DAM_HARM, true );
    return;
}

void spell_cause_critical( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    damage( ch, ( CHAR_DATA * ) vo, dice( 3, 8 ) + level - 6, sn, DAM_HARM, true );
    return;
}

void spell_cause_serious( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    damage( ch, ( CHAR_DATA * ) vo, dice( 2, 8 ) + level / 2, sn, DAM_HARM, true );
    return;
}

void spell_chain_lightning( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    CHAR_DATA              *tmp_vict = NULL;
    CHAR_DATA              *last_vict = NULL;
    CHAR_DATA              *next_vict = NULL;
    bool                    found = false;
    int                     dam = 0;

    /*
     * first strike 
     */

    act( "A lightning bolt leaps from $n's hand and arcs to $N.",
         ch, NULL, victim, TO_ROOM );
    act( "A lightning bolt leaps from your hand and arcs to $N.",
         ch, NULL, victim, TO_CHAR );
    act( "A lightning bolt leaps from $n's hand and hits you!",
         ch, NULL, victim, TO_VICT );

    dam = dice( level, 6 );
    if ( saves_spell( level, victim, DAM_LIGHTNING ) )
        dam /= 3;
    damage( ch, victim, dam, sn, DAM_LIGHTNING, true );
    last_vict = victim;
    level -= 4;                                        /* decrement damage */

    /*
     * new targets 
     */
    while ( level > 0 )
    {
        found = false;
        for ( tmp_vict = ch->in_room->people; tmp_vict != NULL; tmp_vict = next_vict )
        {
            next_vict = tmp_vict->next_in_room;
            if ( !is_safe_spell( ch, tmp_vict, true ) && tmp_vict != last_vict )
            {
                found = true;
                last_vict = tmp_vict;
                act( "The bolt arcs to $n!", tmp_vict, NULL, NULL, TO_ROOM );
                act( "The bolt hits you!", tmp_vict, NULL, NULL, TO_CHAR );
                dam = dice( level, 6 );
                if ( saves_spell( level, tmp_vict, DAM_LIGHTNING ) )
                    dam /= 3;
                damage( ch, tmp_vict, dam, sn, DAM_LIGHTNING, true );
                level -= 4;                            /* decrement damage */
            }
        }                                              /* end target searching loop */

        if ( !found )                                  /* no target found, hit the caster 
                                                        */
        {
            if ( ch == NULL )
                return;

            if ( last_vict == ch )                     /* no double hits */
            {
                act( "The bolt seems to have fizzled out.", ch, NULL, NULL, TO_ROOM );
                act( "The bolt grounds out through your body.", ch, NULL, NULL, TO_CHAR );
                return;
            }

            last_vict = ch;
            act( "The bolt arcs to $n...whoops!", ch, NULL, NULL, TO_ROOM );
            ch_printf( ch, "You are struck by your own lightning!\r\n" );
            dam = dice( level, 6 );
            if ( saves_spell( level, ch, DAM_LIGHTNING ) )
                dam /= 3;
            damage( ch, ch, dam, sn, DAM_LIGHTNING, true );
            level -= 4;                                /* decrement damage */
            if ( ch == NULL )
                return;
        }
        /*
         * now go back and find more targets 
         */
    }
}

void spell_change_sex( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( is_affected( victim, sn ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You've already been changed.\r\n" );
        else
            act( "$N has already had $s(?) sex changed.", ch, NULL, victim, TO_CHAR );
        return;
    }
    if ( saves_spell( level, victim, DAM_OTHER ) )
        return;
    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 2 * level;
    af.location = APPLY_SEX;
    do
    {
        af.modifier = number_range( 0, 2 ) - victim->sex;
    }
    while ( af.modifier == 0 );
    af.bitvector = 0;
    affect_to_char( victim, &af );
    ch_printf( victim, "You feel different.\r\n" );
    act( "$n doesn't look like $mself anymore...", victim, NULL, NULL, TO_ROOM );
    return;
}

void spell_charm_person( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( is_safe( ch, victim ) )
        return;

    if ( victim == ch )
    {
        ch_printf( ch, "You like yourself even better!\r\n" );
        return;
    }

    if ( IS_AFFECTED( victim, AFF_CHARM )
         || IS_AFFECTED( ch, AFF_CHARM )
         || level < victim->level
         || IS_SET( victim->imm_flags, IMM_CHARM )
         || saves_spell( level, victim, DAM_CHARM ) )
        return;

    if ( IS_SET( victim->in_room->room_flags, ROOM_LAW ) )
    {
        ch_printf( ch, "The mayor does not allow charming in the city limits.\r\n" );
        return;
    }

    if ( victim->master )
        stop_follower( victim );
    add_follower( victim, ch );
    victim->leader = ch;
    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = number_fuzzy( level / 4 );
    af.location = 0;
    af.modifier = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char( victim, &af );
    act( "Isn't $n just so nice?", ch, NULL, victim, TO_VICT );
    if ( ch != victim )
        act( "$N looks at you with adoring eyes.", ch, NULL, victim, TO_CHAR );
    return;
}

void spell_chill_touch( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    static const int        dam_each[] = {
        0,
        0, 0, 6, 7, 8, 9, 12, 13, 13, 13,
        14, 14, 14, 15, 15, 15, 16, 16, 16, 17,
        17, 17, 18, 18, 18, 19, 19, 19, 20, 20,
        20, 21, 21, 21, 22, 22, 22, 23, 23, 23,
        24, 24, 24, 25, 25, 25, 26, 26, 26, 27
    };
    AFFECT_DATA             af;
    int                     dam = 0;

    level = UMIN( level, ( int ) ( sizeof( dam_each ) / sizeof( dam_each[0] ) ) - 1 );
    level = UMAX( 0, level );
    dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    if ( !saves_spell( level, victim, DAM_COLD ) )
    {
        act( "$n turns blue and shivers.", victim, NULL, NULL, TO_ROOM );
        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = level;
        af.duration = 6;
        af.location = APPLY_STR;
        af.modifier = -1;
        af.bitvector = 0;
        affect_join( victim, &af );
    }
    else
    {
        dam /= 2;
    }

    damage( ch, victim, dam, sn, DAM_COLD, true );
    return;
}

void spell_colour_spray( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    static const int        dam_each[] = {
        0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        30, 35, 40, 45, 50, 55, 55, 55, 56, 57,
        58, 58, 59, 60, 61, 61, 62, 63, 64, 64,
        65, 66, 67, 67, 68, 69, 70, 70, 71, 72,
        73, 73, 74, 75, 76, 76, 77, 78, 79, 79
    };
    int                     dam = 0;

    level = UMIN( level, ( int ) ( sizeof( dam_each ) / sizeof( dam_each[0] ) ) - 1 );
    level = UMAX( 0, level );
    dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    if ( saves_spell( level, victim, DAM_LIGHT ) )
        dam /= 2;
    else
        spell_blindness( skill_lookup( "blindness" ),
                         level / 2, ch, ( void * ) victim, TARGET_CHAR );

    damage( ch, victim, dam, sn, DAM_LIGHT, true );
    return;
}

void spell_continual_light( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA               *light = NULL;

    if ( target_name[0] != '\0' )                      /* do a glow on some object */
    {
        light = get_obj_carry( ch, target_name, ch );

        if ( light == NULL )
        {
            ch_printf( ch, "You don't see that here.\r\n" );
            return;
        }

        if ( IS_OBJ_STAT( light, ITEM_GLOW ) )
        {
            act( "$p is already glowing.", ch, light, NULL, TO_CHAR );
            return;
        }

        SET_BIT( light->extra_flags, ITEM_GLOW );
        act( "$p glows with a white light.", ch, light, NULL, TO_ALL );
        return;
    }

    light = create_object( get_obj_index( OBJ_VNUM_LIGHT_BALL ), 0 );
    obj_to_room( light, ch->in_room );
    act( "$n twiddles $s thumbs and $p appears.", ch, light, NULL, TO_ROOM );
    act( "You twiddle your thumbs and $p appears.", ch, light, NULL, TO_CHAR );
    return;
}

void spell_control_weather( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    if ( !str_cmp( target_name, "better" ) )
        weather_info.change += dice( level / 3, 4 );
    else if ( !str_cmp( target_name, "worse" ) )
        weather_info.change -= dice( level / 3, 4 );
    else
        ch_printf( ch, "Do you want it to get better or worse?\r\n" );

    ch_printf( ch, "Ok.\r\n" );
    return;
}

void spell_create_food( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA               *mushroom = NULL;

    mushroom = create_object( get_obj_index( OBJ_VNUM_MUSHROOM ), 0 );
    mushroom->value[0] = level / 2;
    mushroom->value[1] = level;
    obj_to_room( mushroom, ch->in_room );
    act( "$p suddenly appears.", ch, mushroom, NULL, TO_ROOM );
    act( "$p suddenly appears.", ch, mushroom, NULL, TO_CHAR );
    return;
}

void spell_create_rose( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA               *rose = NULL;

    rose = create_object( get_obj_index( OBJ_VNUM_ROSE ), 0 );
    act( "$n has created a beautiful red rose.", ch, rose, NULL, TO_ROOM );
    ch_printf( ch, "You create a beautiful red rose.\r\n" );
    obj_to_char( rose, ch );
    return;
}

void spell_create_spring( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA               *spring = NULL;

    spring = create_object( get_obj_index( OBJ_VNUM_SPRING ), 0 );
    spring->timer = level;
    obj_to_room( spring, ch->in_room );
    act( "$p flows from the ground.", ch, spring, NULL, TO_ROOM );
    act( "$p flows from the ground.", ch, spring, NULL, TO_CHAR );
    return;
}

void spell_create_water( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) vo;
    int                     water = 0;

    if ( obj->item_type != ITEM_DRINK_CON )
    {
        ch_printf( ch, "It is unable to hold water.\r\n" );
        return;
    }

    if ( obj->value[2] != LIQ_WATER && obj->value[1] != 0 )
    {
        ch_printf( ch, "It contains some other liquid.\r\n" );
        return;
    }

    water = UMIN( level * ( weather_info.sky >= SKY_RAINING ? 4 : 2 ),
                  obj->value[0] - obj->value[1] );

    if ( water > 0 )
    {
        obj->value[2] = LIQ_WATER;
        obj->value[1] += water;
        if ( !is_name( "water", obj->name ) )
        {
            char                    buf[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

            sprintf( buf, "%s water", obj->name );
            free_string( obj->name );
            obj->name = str_dup( buf );
        }
        act( "$p is filled.", ch, obj, NULL, TO_CHAR );
    }

    return;
}

void spell_cure_blindness( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     lsn = -1;

    if ( ( lsn = skill_lookup( "blindness" ) ) == -1 )
    {
        log_error( "Can't find the \"%s\" skill in %s?", "blindness", __FUNCTION__ );
        return;
    }

    if ( !is_affected( victim, lsn ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You aren't blind.\r\n" );
        else
            act( "$N doesn't appear to be blinded.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( check_dispel( level, victim, lsn ) )
    {
        ch_printf( victim, "Your vision returns!\r\n" );
        act( "$n is no longer blinded.", victim, NULL, NULL, TO_ROOM );
    }
    else
        ch_printf( ch, "Spell failed.\r\n" );
}

void spell_cure_critical( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     heal = 0;

    heal = dice( 3, 8 ) + level - 6;
    victim->hit = UMIN( victim->hit + heal, victim->max_hit );
    update_pos( victim );
    ch_printf( victim, "You feel better!\r\n" );
    if ( ch != victim )
        ch_printf( ch, "Ok.\r\n" );
    return;
}

/* RT added to cure plague */
void spell_cure_disease( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     lsn = -1;

    if ( ( lsn = skill_lookup( "plague" ) ) == -1 )
    {
        log_error( "Can't find the \"%s\" skill in %s?", "plague", __FUNCTION__ );
        return;
    }

    if ( !is_affected( victim, lsn ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You aren't ill.\r\n" );
        else
            act( "$N doesn't appear to be diseased.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( check_dispel( level, victim, lsn ) )
    {
        ch_printf( victim, "Your sores vanish.\r\n" );
        act( "$n looks relieved as $s sores vanish.", victim, NULL, NULL, TO_ROOM );
    }
    else
        ch_printf( ch, "Spell failed.\r\n" );
}

void spell_cure_light( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     heal = 0;

    heal = dice( 1, 8 ) + level / 3;
    victim->hit = UMIN( victim->hit + heal, victim->max_hit );
    update_pos( victim );
    ch_printf( victim, "You feel better!\r\n" );
    if ( ch != victim )
        ch_printf( ch, "Ok.\r\n" );
    return;
}

void spell_cure_poison( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     lsn = -1;

    if ( ( lsn = skill_lookup( "poison" ) ) == -1 )
    {
        log_error( "Can't find the \"%s\" skill in %s?", "poison", __FUNCTION__ );
        return;
    }

    if ( !is_affected( victim, lsn ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You aren't poisoned.\r\n" );
        else
            act( "$N doesn't appear to be poisoned.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( check_dispel( level, victim, lsn ) )
    {
        ch_printf( victim, "A warm feeling runs through your body.\r\n" );
        act( "$n looks much better.", victim, NULL, NULL, TO_ROOM );
    }
    else
        ch_printf( ch, "Spell failed.\r\n" );
}

void spell_cure_serious( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     heal = 0;

    heal = dice( 2, 8 ) + level / 2;
    victim->hit = UMIN( victim->hit + heal, victim->max_hit );
    update_pos( victim );
    ch_printf( victim, "You feel better!\r\n" );
    if ( ch != victim )
        ch_printf( ch, "Ok.\r\n" );
    return;
}

void spell_curse( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = NULL;
    OBJ_DATA               *obj = NULL;
    AFFECT_DATA             af;

    /*
     * deal with the object case first 
     */
    if ( target == TARGET_OBJ )
    {
        obj = ( OBJ_DATA * ) vo;
        if ( IS_OBJ_STAT( obj, ITEM_EVIL ) )
        {
            act( "$p is already filled with evil.", ch, obj, NULL, TO_CHAR );
            return;
        }

        if ( IS_OBJ_STAT( obj, ITEM_BLESS ) )
        {
            AFFECT_DATA            *paf = NULL;

            paf = affect_find( obj->affected, skill_lookup( "bless" ) );
            if ( !saves_dispel( level, paf != NULL ? paf->level : obj->level, 0 ) )
            {
                if ( paf != NULL )
                    affect_remove_obj( obj, paf );
                act( "$p glows with a red aura.", ch, obj, NULL, TO_ALL );
                REMOVE_BIT( obj->extra_flags, ITEM_BLESS );
                return;
            }
            else
            {
                act( "The holy aura of $p is too powerful for you to overcome.",
                     ch, obj, NULL, TO_CHAR );
                return;
            }
        }

        af.where = TO_OBJECT;
        af.type = sn;
        af.level = level;
        af.duration = 2 * level;
        af.location = APPLY_SAVES;
        af.modifier = +1;
        af.bitvector = ITEM_EVIL;
        affect_to_obj( obj, &af );

        act( "$p glows with a malevolent aura.", ch, obj, NULL, TO_ALL );

        if ( obj->wear_loc != WEAR_NONE )
            ch->saving_throw += 1;
        return;
    }

    /*
     * character curses 
     */
    victim = ( CHAR_DATA * ) vo;

    if ( IS_AFFECTED( victim, AFF_CURSE ) || saves_spell( level, victim, DAM_NEGATIVE ) )
        return;
    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 2 * level;
    af.location = APPLY_HITROLL;
    af.modifier = -1 * ( level / 8 );
    af.bitvector = AFF_CURSE;
    affect_to_char( victim, &af );

    af.location = APPLY_SAVING_SPELL;
    af.modifier = level / 8;
    affect_to_char( victim, &af );

    ch_printf( victim, "You feel unclean.\r\n" );
    if ( ch != victim )
        act( "$N looks very uncomfortable.", ch, NULL, victim, TO_CHAR );
    return;
}

/* RT replacement demonfire spell */

void spell_demonfire( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam = 0;

    if ( !IS_NPC( ch ) && !IS_EVIL( ch ) )
    {
        victim = ch;
        ch_printf( ch, "The demons turn upon you!\r\n" );
    }

    ch->alignment = UMAX( -1000, ch->alignment - 50 );

    if ( victim != ch )
    {
        act( "$n calls forth the demons of Hell upon $N!", ch, NULL, victim, TO_ROOM );
        act( "$n has assailed you with the demons of Hell!", ch, NULL, victim, TO_VICT );
        ch_printf( ch, "You conjure forth the demons of hell!\r\n" );
    }
    dam = dice( level, 10 );
    if ( saves_spell( level, victim, DAM_NEGATIVE ) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_NEGATIVE, true );
    spell_curse( skill_lookup( "curse" ), 3 * level / 4, ch, ( void * ) victim,
                 TARGET_CHAR );
}

void spell_detect_evil( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( IS_AFFECTED( victim, AFF_DETECT_EVIL ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You can already sense evil.\r\n" );
        else
            act( "$N can already detect evil.", ch, NULL, victim, TO_CHAR );
        return;
    }
    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_DETECT_EVIL;
    affect_to_char( victim, &af );
    ch_printf( victim, "Your eyes tingle.\r\n" );
    if ( ch != victim )
        ch_printf( ch, "Ok.\r\n" );
    return;
}

void spell_detect_good( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( IS_AFFECTED( victim, AFF_DETECT_GOOD ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You can already sense good.\r\n" );
        else
            act( "$N can already detect good.", ch, NULL, victim, TO_CHAR );
        return;
    }
    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_DETECT_GOOD;
    affect_to_char( victim, &af );
    ch_printf( victim, "Your eyes tingle.\r\n" );
    if ( ch != victim )
        ch_printf( ch, "Ok.\r\n" );
    return;
}

void spell_detect_hidden( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( IS_AFFECTED( victim, AFF_DETECT_HIDDEN ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You are already as alert as you can be. \r\n" );
        else
            act( "$N can already sense hidden lifeforms.", ch, NULL, victim, TO_CHAR );
        return;
    }
    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_DETECT_HIDDEN;
    affect_to_char( victim, &af );
    ch_printf( victim, "Your awareness improves.\r\n" );
    if ( ch != victim )
        ch_printf( ch, "Ok.\r\n" );
    return;
}

void spell_detect_invisibility( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( IS_AFFECTED( victim, AFF_DETECT_INVISIBILITY ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You can already see invisible.\r\n" );
        else
            act( "$N can already see invisible things.", ch, NULL, victim, TO_CHAR );
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_DETECT_INVISIBILITY;
    affect_to_char( victim, &af );
    ch_printf( victim, "Your eyes tingle.\r\n" );
    if ( ch != victim )
        ch_printf( ch, "Ok.\r\n" );
    return;
}

void spell_detect_magic( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( IS_AFFECTED( victim, AFF_DETECT_MAGIC ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You can already sense magical auras.\r\n" );
        else
            act( "$N can already detect magic.", ch, NULL, victim, TO_CHAR );
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_DETECT_MAGIC;
    affect_to_char( victim, &af );
    ch_printf( victim, "Your eyes tingle.\r\n" );
    if ( ch != victim )
        ch_printf( ch, "Ok.\r\n" );
    return;
}

void spell_detect_poison( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) vo;

    if ( obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_FOOD )
    {
        if ( obj->value[3] != 0 )
            ch_printf( ch, "You smell poisonous fumes.\r\n" );
        else
            ch_printf( ch, "It looks delicious.\r\n" );
    }
    else
    {
        ch_printf( ch, "It doesn't look poisoned.\r\n" );
    }

    return;
}

void spell_dispel_evil( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam = 0;

    if ( !IS_NPC( ch ) && IS_EVIL( ch ) )
        victim = ch;

    if ( IS_GOOD( victim ) )
    {
        act( "Mota protects $N.", ch, NULL, victim, TO_ROOM );
        return;
    }

    if ( IS_NEUTRAL( victim ) )
    {
        act( "$N does not seem to be affected.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( victim->hit > ( ch->level * 4 ) )
        dam = dice( level, 4 );
    else
        dam = UMAX( victim->hit, dice( level, 4 ) );
    if ( saves_spell( level, victim, DAM_HOLY ) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_HOLY, true );
    return;
}

void spell_dispel_good( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam = 0;

    if ( !IS_NPC( ch ) && IS_GOOD( ch ) )
        victim = ch;

    if ( IS_EVIL( victim ) )
    {
        act( "$N is protected by $S evil.", ch, NULL, victim, TO_ROOM );
        return;
    }

    if ( IS_NEUTRAL( victim ) )
    {
        act( "$N does not seem to be affected.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( victim->hit > ( ch->level * 4 ) )
        dam = dice( level, 4 );
    else
        dam = UMAX( victim->hit, dice( level, 4 ) );
    if ( saves_spell( level, victim, DAM_NEGATIVE ) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_NEGATIVE, true );
    return;
}

/* modified for enhanced use */

void spell_dispel_magic( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    bool                    found = false;

    if ( saves_spell( level, victim, DAM_OTHER ) )
    {
        ch_printf( victim, "You feel a brief tingling sensation.\r\n" );
        ch_printf( ch, "You failed.\r\n" );
        return;
    }

    /*
     * begin running through the spells 
     */

    if ( check_dispel( level, victim, skill_lookup( "armor" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "bless" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "blindness" ) ) )
    {
        found = true;
        act( "$n is no longer blinded.", victim, NULL, NULL, TO_ROOM );
    }

    if ( check_dispel( level, victim, skill_lookup( "calm" ) ) )
    {
        found = true;
        act( "$n no longer looks so peaceful...", victim, NULL, NULL, TO_ROOM );
    }

    if ( check_dispel( level, victim, skill_lookup( "change sex" ) ) )
    {
        found = true;
        act( "$n looks more like $mself again.", victim, NULL, NULL, TO_ROOM );
    }

    if ( check_dispel( level, victim, skill_lookup( "charm person" ) ) )
    {
        found = true;
        act( "$n regains $s free will.", victim, NULL, NULL, TO_ROOM );
    }

    if ( check_dispel( level, victim, skill_lookup( "chill touch" ) ) )
    {
        found = true;
        act( "$n looks warmer.", victim, NULL, NULL, TO_ROOM );
    }

    if ( check_dispel( level, victim, skill_lookup( "curse" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "detect evil" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "detect good" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "detect hidden" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "detect invisibility" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "detect magic" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "faerie fire" ) ) )
    {
        act( "$n's outline fades.", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "fly" ) ) )
    {
        act( "$n falls to the ground!", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "frenzy" ) ) )
    {
        act( "$n no longer looks so wild.", victim, NULL, NULL, TO_ROOM );;
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "giant strength" ) ) )
    {
        act( "$n no longer looks so mighty.", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "haste" ) ) )
    {
        act( "$n is no longer moving so quickly.", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "infravision" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "invisibility" ) ) )
    {
        act( "$n fades into existance.", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "mass invisibility" ) ) )
    {
        act( "$n fades into existance.", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "pass door" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "protection evil" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "protection good" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "sanctuary" ) ) )
    {
        act( "The white aura around $n's body vanishes.", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( IS_AFFECTED( victim, AFF_SANCTUARY )
         && !saves_dispel( level, victim->level, -1 )
         && !is_affected( victim, skill_lookup( "sanctuary" ) ) )
    {
        REMOVE_BIT( victim->affected_by, AFF_SANCTUARY );
        act( "The white aura around $n's body vanishes.", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "shield" ) ) )
    {
        act( "The shield protecting $n vanishes.", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "sleep" ) ) )
        found = true;

    if ( check_dispel( level, victim, skill_lookup( "slow" ) ) )
    {
        act( "$n is no longer moving so slowly.", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "stone skin" ) ) )
    {
        act( "$n's skin regains its normal texture.", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( check_dispel( level, victim, skill_lookup( "weaken" ) ) )
    {
        act( "$n looks stronger.", victim, NULL, NULL, TO_ROOM );
        found = true;
    }

    if ( found )
        ch_printf( ch, "Ok.\r\n" );
    else
        ch_printf( ch, "Spell failed.\r\n" );
    return;
}

void spell_earthquake( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *vch = NULL;
    CHAR_DATA              *vch_next = NULL;

    ch_printf( ch, "The earth trembles beneath your feet!\r\n" );
    act( "$n makes the earth tremble and shiver.", ch, NULL, NULL, TO_ROOM );

    for ( vch = char_list; vch != NULL; vch = vch_next )
    {
        vch_next = vch->next;
        if ( vch->in_room == NULL )
            continue;
        if ( vch->in_room == ch->in_room )
        {
            if ( vch != ch && !is_safe_spell( ch, vch, true ) )
            {
                if ( IS_AFFECTED( vch, AFF_FLYING ) )
                    damage( ch, vch, 0, sn, DAM_BASH, true );
                else
                    damage( ch, vch, level + dice( 2, 8 ), sn, DAM_BASH, true );
            }
            continue;
        }

        if ( vch->in_room->area == ch->in_room->area )
            ch_printf( vch, "The earth trembles and shivers.\r\n" );
    }

    return;
}

void spell_enchant_armor( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) vo;
    AFFECT_DATA            *paf = NULL;
    int                     result = 0;
    int                     fail = 0;
    int                     ac_bonus = 0;
    int                     added = 0;
    bool                    ac_found = false;

    if ( obj->item_type != ITEM_ARMOR )
    {
        ch_printf( ch, "That isn't an armor.\r\n" );
        return;
    }

    if ( obj->wear_loc != -1 )
    {
        ch_printf( ch, "The item must be carried to be enchanted.\r\n" );
        return;
    }

    /*
     * this means they have no bonus 
     */
    ac_bonus = 0;
    fail = 25;                                         /* base 25% chance of failure */

    /*
     * find the bonuses 
     */

    if ( !obj->enchanted )
        for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
        {
            if ( paf->location == APPLY_AC )
            {
                ac_bonus = paf->modifier;
                ac_found = true;
                fail += 5 * ( ac_bonus * ac_bonus );
            }

            else                                       /* things get a little harder */
                fail += 20;
        }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->location == APPLY_AC )
        {
            ac_bonus = paf->modifier;
            ac_found = true;
            fail += 5 * ( ac_bonus * ac_bonus );
        }

        else                                           /* things get a little harder */
            fail += 20;
    }

    /*
     * apply other modifiers 
     */
    fail -= level;

    if ( IS_OBJ_STAT( obj, ITEM_BLESS ) )
        fail -= 15;
    if ( IS_OBJ_STAT( obj, ITEM_GLOW ) )
        fail -= 5;

    fail = URANGE( 5, fail, 85 );

    result = number_percent(  );

    /*
     * the moment of truth 
     */
    if ( result < ( fail / 5 ) )                       /* item destroyed */
    {
        act( "$p flares blindingly... and evaporates!", ch, obj, NULL, TO_CHAR );
        act( "$p flares blindingly... and evaporates!", ch, obj, NULL, TO_ROOM );
        extract_obj( obj );
        return;
    }

    if ( result < ( fail / 3 ) )                       /* item disenchanted */
    {
        AFFECT_DATA            *paf_next;

        act( "$p glows brightly, then fades...oops.", ch, obj, NULL, TO_CHAR );
        act( "$p glows brightly, then fades.", ch, obj, NULL, TO_ROOM );
        obj->enchanted = true;

        /*
         * remove all affects 
         */
        for ( paf = obj->affected; paf != NULL; paf = paf_next )
        {
            paf_next = paf->next;
            free_affect( paf );
        }
        obj->affected = NULL;

        /*
         * clear all flags 
         */
        obj->extra_flags = 0;
        return;
    }

    if ( result <= fail )                              /* failed, no bad result */
    {
        ch_printf( ch, "Nothing seemed to happen.\r\n" );
        return;
    }

    /*
     * okay, move all the old flags into new vectors if we have to 
     */
    if ( !obj->enchanted )
    {
        AFFECT_DATA            *af_new;

        obj->enchanted = true;

        for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
        {
            af_new = new_affect(  );

            af_new->next = obj->affected;
            obj->affected = af_new;

            af_new->where = paf->where;
            af_new->type = UMAX( 0, paf->type );
            af_new->level = paf->level;
            af_new->duration = paf->duration;
            af_new->location = paf->location;
            af_new->modifier = paf->modifier;
            af_new->bitvector = paf->bitvector;
        }
    }

    if ( result <= ( 90 - level / 5 ) )                /* success! */
    {
        act( "$p shimmers with a gold aura.", ch, obj, NULL, TO_CHAR );
        act( "$p shimmers with a gold aura.", ch, obj, NULL, TO_ROOM );
        SET_BIT( obj->extra_flags, ITEM_MAGIC );
        added = -1;
    }

    else                                               /* exceptional enchant */
    {
        act( "$p glows a brillant gold!", ch, obj, NULL, TO_CHAR );
        act( "$p glows a brillant gold!", ch, obj, NULL, TO_ROOM );
        SET_BIT( obj->extra_flags, ITEM_MAGIC );
        SET_BIT( obj->extra_flags, ITEM_GLOW );
        added = -2;
    }

    /*
     * now add the enchantments 
     */

    if ( obj->level < LEVEL_HERO )
        obj->level = UMIN( LEVEL_HERO - 1, obj->level + 1 );

    if ( ac_found )
    {
        for ( paf = obj->affected; paf != NULL; paf = paf->next )
        {
            if ( paf->location == APPLY_AC )
            {
                paf->type = sn;
                paf->modifier += added;
                paf->level = UMAX( paf->level, level );
            }
        }
    }
    else                                               /* add a new affect */
    {
        paf = new_affect(  );

        paf->where = TO_OBJECT;
        paf->type = sn;
        paf->level = level;
        paf->duration = -1;
        paf->location = APPLY_AC;
        paf->modifier = added;
        paf->bitvector = 0;
        paf->next = obj->affected;
        obj->affected = paf;
    }
}

void spell_enchant_weapon( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) vo;
    AFFECT_DATA            *paf = NULL;
    int                     result = 0;
    int                     fail = 0;
    int                     hit_bonus = 0;
    int                     dam_bonus = 0;
    int                     added = 0;
    bool                    hit_found = false;
    bool                    dam_found = false;

    if ( obj->item_type != ITEM_WEAPON )
    {
        ch_printf( ch, "That isn't a weapon.\r\n" );
        return;
    }

    if ( obj->wear_loc != -1 )
    {
        ch_printf( ch, "The item must be carried to be enchanted.\r\n" );
        return;
    }

    /*
     * this means they have no bonus 
     */
    hit_bonus = 0;
    dam_bonus = 0;
    fail = 25;                                         /* base 25% chance of failure */

    /*
     * find the bonuses 
     */

    if ( !obj->enchanted )
        for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
        {
            if ( paf->location == APPLY_HITROLL )
            {
                hit_bonus = paf->modifier;
                hit_found = true;
                fail += 2 * ( hit_bonus * hit_bonus );
            }

            else if ( paf->location == APPLY_DAMROLL )
            {
                dam_bonus = paf->modifier;
                dam_found = true;
                fail += 2 * ( dam_bonus * dam_bonus );
            }

            else                                       /* things get a little harder */
                fail += 25;
        }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->location == APPLY_HITROLL )
        {
            hit_bonus = paf->modifier;
            hit_found = true;
            fail += 2 * ( hit_bonus * hit_bonus );
        }

        else if ( paf->location == APPLY_DAMROLL )
        {
            dam_bonus = paf->modifier;
            dam_found = true;
            fail += 2 * ( dam_bonus * dam_bonus );
        }

        else                                           /* things get a little harder */
            fail += 25;
    }

    /*
     * apply other modifiers 
     */
    fail -= 3 * level / 2;

    if ( IS_OBJ_STAT( obj, ITEM_BLESS ) )
        fail -= 15;
    if ( IS_OBJ_STAT( obj, ITEM_GLOW ) )
        fail -= 5;

    fail = URANGE( 5, fail, 95 );

    result = number_percent(  );

    /*
     * the moment of truth 
     */
    if ( result < ( fail / 5 ) )                       /* item destroyed */
    {
        act( "$p shivers violently and explodes!", ch, obj, NULL, TO_CHAR );
        act( "$p shivers violently and explodeds!", ch, obj, NULL, TO_ROOM );
        extract_obj( obj );
        return;
    }

    if ( result < ( fail / 2 ) )                       /* item disenchanted */
    {
        AFFECT_DATA            *paf_next;

        act( "$p glows brightly, then fades...oops.", ch, obj, NULL, TO_CHAR );
        act( "$p glows brightly, then fades.", ch, obj, NULL, TO_ROOM );
        obj->enchanted = true;

        /*
         * remove all affects 
         */
        for ( paf = obj->affected; paf != NULL; paf = paf_next )
        {
            paf_next = paf->next;
            free_affect( paf );
        }
        obj->affected = NULL;

        /*
         * clear all flags 
         */
        obj->extra_flags = 0;
        return;
    }

    if ( result <= fail )                              /* failed, no bad result */
    {
        ch_printf( ch, "Nothing seemed to happen.\r\n" );
        return;
    }

    /*
     * okay, move all the old flags into new vectors if we have to 
     */
    if ( !obj->enchanted )
    {
        AFFECT_DATA            *af_new;

        obj->enchanted = true;

        for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
        {
            af_new = new_affect(  );

            af_new->next = obj->affected;
            obj->affected = af_new;

            af_new->where = paf->where;
            af_new->type = UMAX( 0, paf->type );
            af_new->level = paf->level;
            af_new->duration = paf->duration;
            af_new->location = paf->location;
            af_new->modifier = paf->modifier;
            af_new->bitvector = paf->bitvector;
        }
    }

    if ( result <= ( 100 - level / 5 ) )               /* success! */
    {
        act( "$p glows blue.", ch, obj, NULL, TO_CHAR );
        act( "$p glows blue.", ch, obj, NULL, TO_ROOM );
        SET_BIT( obj->extra_flags, ITEM_MAGIC );
        added = 1;
    }

    else                                               /* exceptional enchant */
    {
        act( "$p glows a brillant blue!", ch, obj, NULL, TO_CHAR );
        act( "$p glows a brillant blue!", ch, obj, NULL, TO_ROOM );
        SET_BIT( obj->extra_flags, ITEM_MAGIC );
        SET_BIT( obj->extra_flags, ITEM_GLOW );
        added = 2;
    }

    /*
     * now add the enchantments 
     */

    if ( obj->level < LEVEL_HERO - 1 )
        obj->level = UMIN( LEVEL_HERO - 1, obj->level + 1 );

    if ( dam_found )
    {
        for ( paf = obj->affected; paf != NULL; paf = paf->next )
        {
            if ( paf->location == APPLY_DAMROLL )
            {
                paf->type = sn;
                paf->modifier += added;
                paf->level = UMAX( paf->level, level );
                if ( paf->modifier > 4 )
                    SET_BIT( obj->extra_flags, ITEM_HUM );
            }
        }
    }
    else                                               /* add a new affect */
    {
        paf = new_affect(  );

        paf->where = TO_OBJECT;
        paf->type = sn;
        paf->level = level;
        paf->duration = -1;
        paf->location = APPLY_DAMROLL;
        paf->modifier = added;
        paf->bitvector = 0;
        paf->next = obj->affected;
        obj->affected = paf;
    }

    if ( hit_found )
    {
        for ( paf = obj->affected; paf != NULL; paf = paf->next )
        {
            if ( paf->location == APPLY_HITROLL )
            {
                paf->type = sn;
                paf->modifier += added;
                paf->level = UMAX( paf->level, level );
                if ( paf->modifier > 4 )
                    SET_BIT( obj->extra_flags, ITEM_HUM );
            }
        }
    }
    else                                               /* add a new affect */
    {
        paf = new_affect(  );

        paf->type = sn;
        paf->level = level;
        paf->duration = -1;
        paf->location = APPLY_HITROLL;
        paf->modifier = added;
        paf->bitvector = 0;
        paf->next = obj->affected;
        obj->affected = paf;
    }
}

/*
 * Drain XP, MANA, HP.
 * Caster gains HP.
 */
void spell_energy_drain( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam = 0;

    if ( victim != ch )
        ch->alignment = UMAX( -1000, ch->alignment - 50 );

    if ( saves_spell( level, victim, DAM_NEGATIVE ) )
    {
        ch_printf( victim, "You feel a momentary chill.\r\n" );
        return;
    }

    if ( victim->level <= 2 )
    {
        dam = ch->hit + 1;
    }
    else
    {
        gain_exp( victim, 0 - number_range( level / 2, 3 * level / 2 ) );
        victim->mana /= 2;
        victim->move /= 2;
        dam = dice( 1, level );
        ch->hit += dam;
    }

    ch_printf( victim, "You feel your life slipping away!\r\n" );
    ch_printf( ch, "Wow....what a rush!\r\n" );
    damage( ch, victim, dam, sn, DAM_NEGATIVE, true );

    return;
}

void spell_fireball( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    static const int        dam_each[] = {
        0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 30, 35, 40, 45, 50, 55,
        60, 65, 70, 75, 80, 82, 84, 86, 88, 90,
        92, 94, 96, 98, 100, 102, 104, 106, 108, 110,
        112, 114, 116, 118, 120, 122, 124, 126, 128, 130
    };
    int                     dam = 0;

    level = UMIN( level, ( int ) ( sizeof( dam_each ) / sizeof( dam_each[0] ) ) - 1 );
    level = UMAX( 0, level );
    dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    if ( saves_spell( level, victim, DAM_FIRE ) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_FIRE, true );
    return;
}

void spell_fireproof( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) vo;
    AFFECT_DATA             af;

    if ( IS_OBJ_STAT( obj, ITEM_BURN_PROOF ) )
    {
        act( "$p is already protected from burning.", ch, obj, NULL, TO_CHAR );
        return;
    }

    af.where = TO_OBJECT;
    af.type = sn;
    af.level = level;
    af.duration = number_fuzzy( level / 4 );
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = ITEM_BURN_PROOF;

    affect_to_obj( obj, &af );

    act( "You protect $p from fire.", ch, obj, NULL, TO_CHAR );
    act( "$p is surrounded by a protective aura.", ch, obj, NULL, TO_ROOM );
}

void spell_flamestrike( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam = 0;

    dam = dice( 6 + level / 2, 8 );
    if ( saves_spell( level, victim, DAM_FIRE ) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_FIRE, true );
    return;
}

void spell_faerie_fire( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( IS_AFFECTED( victim, AFF_FAERIE_FIRE ) )
        return;
    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.location = APPLY_AC;
    af.modifier = 2 * level;
    af.bitvector = AFF_FAERIE_FIRE;
    affect_to_char( victim, &af );
    ch_printf( victim, "You are surrounded by a pink outline.\r\n" );
    act( "$n is surrounded by a pink outline.", victim, NULL, NULL, TO_ROOM );
    return;
}

void spell_faerie_fog( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *ich = NULL;

    act( "$n conjures a cloud of purple smoke.", ch, NULL, NULL, TO_ROOM );
    ch_printf( ch, "You conjure a cloud of purple smoke.\r\n" );

    for ( ich = ch->in_room->people; ich != NULL; ich = ich->next_in_room )
    {
        if ( ich->invis_level > 0 )
            continue;

        if ( ich == ch || saves_spell( level, ich, DAM_OTHER ) )
            continue;

        affect_strip( ich, skill_lookup( "invisibility" ) );
        affect_strip( ich, skill_lookup( "mass invisibility" ) );
        affect_strip( ich, skill_lookup( "sneak " ) );
        REMOVE_BIT( ich->affected_by, AFF_HIDE );
        REMOVE_BIT( ich->affected_by, AFF_INVISIBLE );
        REMOVE_BIT( ich->affected_by, AFF_SNEAK );
        act( "$n is revealed!", ich, NULL, NULL, TO_ROOM );
        ch_printf( ich, "You are revealed!\r\n" );
    }

    return;
}

void spell_floating_disc( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA               *disc = NULL;
    OBJ_DATA               *floating = NULL;

    floating = get_eq_char( ch, WEAR_FLOAT );
    if ( floating != NULL && IS_OBJ_STAT( floating, ITEM_NOREMOVE ) )
    {
        act( "You can't remove $p.", ch, floating, NULL, TO_CHAR );
        return;
    }

    disc = create_object( get_obj_index( OBJ_VNUM_DISC ), 0 );
    disc->value[0] = ch->level * 10;                   /* 10 pounds per level capacity */
    disc->value[3] = ch->level * 5;                    /* 5 pounds per level max per item 
                                                        */
    disc->timer = ch->level * 2 - number_range( 0, level / 2 );

    act( "$n has created a floating black disc.", ch, NULL, NULL, TO_ROOM );
    ch_printf( ch, "You create a floating disc.\r\n" );
    obj_to_char( disc, ch );
    wear_obj( ch, disc, true );
    return;
}

void spell_fly( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( IS_AFFECTED( victim, AFF_FLYING ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You are already airborne.\r\n" );
        else
            act( "$N doesn't need your help to fly.", ch, NULL, victim, TO_CHAR );
        return;
    }
    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level + 3;
    af.location = 0;
    af.modifier = 0;
    af.bitvector = AFF_FLYING;
    affect_to_char( victim, &af );
    ch_printf( victim, "Your feet rise off the ground.\r\n" );
    act( "$n's feet rise off the ground.", victim, NULL, NULL, TO_ROOM );
    return;
}

/* RT clerical berserking spell */

void spell_frenzy( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( is_affected( victim, sn ) || IS_AFFECTED( victim, AFF_BERSERK ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You are already in a frenzy.\r\n" );
        else
            act( "$N is already in a frenzy.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( is_affected( victim, skill_lookup( "calm" ) ) )
    {
        if ( victim == ch )
            ch_printf( ch, "Why don't you just relax for a while?\r\n" );
        else
            act( "$N doesn't look like $e wants to fight anymore.",
                 ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( ( IS_GOOD( ch ) && !IS_GOOD( victim ) ) ||
         ( IS_NEUTRAL( ch ) && !IS_NEUTRAL( victim ) ) ||
         ( IS_EVIL( ch ) && !IS_EVIL( victim ) ) )
    {
        act( "Your god doesn't seem to like $N", ch, NULL, victim, TO_CHAR );
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level / 3;
    af.modifier = level / 6;
    af.bitvector = 0;

    af.location = APPLY_HITROLL;
    affect_to_char( victim, &af );

    af.location = APPLY_DAMROLL;
    affect_to_char( victim, &af );

    af.modifier = 10 * ( level / 12 );
    af.location = APPLY_AC;
    affect_to_char( victim, &af );

    ch_printf( victim, "You are filled with holy wrath!\r\n" );
    act( "$n gets a wild look in $s eyes!", victim, NULL, NULL, TO_ROOM );
}

/* RT ROM-style gate */

void spell_gate( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = NULL;
    bool                    gate_pet = false;

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL || victim == ch
         || victim->in_room == NULL || !can_see_room( ch, victim->in_room )
         || IS_SET( victim->in_room->room_flags, ROOM_SAFE )
         || IS_SET( victim->in_room->room_flags, ROOM_PRIVATE )
         || IS_SET( victim->in_room->room_flags, ROOM_SOLITARY )
         || IS_SET( victim->in_room->room_flags, ROOM_NO_RECALL )
         || IS_SET( ch->in_room->room_flags, ROOM_NO_RECALL )
         || victim->level >= level + 3 || ( is_clan( victim )
                                            && !is_same_clan( ch, victim ) )
         || ( !IS_NPC( victim ) && victim->level >= LEVEL_HERO ) || ( IS_NPC( victim )
                                                                      && IS_SET( victim->
                                                                                 imm_flags,
                                                                                 IMM_SUMMON ) )
         || ( IS_NPC( victim ) && saves_spell( level, victim, DAM_OTHER ) ) )
    {
        ch_printf( ch, "You failed.\r\n" );
        return;
    }
    if ( ch->pet != NULL && ch->in_room == ch->pet->in_room )
        gate_pet = true;
    else
        gate_pet = false;

    act( "$n steps through a gate and vanishes.", ch, NULL, NULL, TO_ROOM );
    ch_printf( ch, "You step through a gate and vanish.\r\n" );
    char_from_room( ch );
    char_to_room( ch, victim->in_room );

    act( "$n has arrived through a gate.", ch, NULL, NULL, TO_ROOM );
    do_function( ch, &do_look, "auto" );

    if ( gate_pet )
    {
        act( "$n steps through a gate and vanishes.", ch->pet, NULL, NULL, TO_ROOM );
        ch_printf( ch->pet, "You step through a gate and vanish.\r\n" );
        char_from_room( ch->pet );
        char_to_room( ch->pet, victim->in_room );
        act( "$n has arrived through a gate.", ch->pet, NULL, NULL, TO_ROOM );
        do_function( ch->pet, &do_look, "auto" );
    }
}

void spell_giant_strength( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( is_affected( victim, sn ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You are already as strong as you can get!\r\n" );
        else
            act( "$N can't get any stronger.", ch, NULL, victim, TO_CHAR );
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.location = APPLY_STR;
    af.modifier = 1 + ( level >= 18 ) + ( level >= 25 ) + ( level >= 32 );
    af.bitvector = 0;
    affect_to_char( victim, &af );
    ch_printf( victim, "Your muscles surge with heightened power!\r\n" );
    act( "$n's muscles surge with heightened power.", victim, NULL, NULL, TO_ROOM );
    return;
}

void spell_harm( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam = 0;

    dam = UMAX( 20, victim->hit - dice( 1, 4 ) );
    if ( saves_spell( level, victim, DAM_HARM ) )
        dam = UMIN( 50, dam / 2 );
    dam = UMIN( 100, dam );
    damage( ch, victim, dam, sn, DAM_HARM, true );
    return;
}

/* RT haste spell */

void spell_haste( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( is_affected( victim, sn ) || IS_AFFECTED( victim, AFF_HASTE )
         || IS_SET( victim->off_flags, OFF_FAST ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You can't move any faster!\r\n" );
        else
            act( "$N is already moving as fast as $E can.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( IS_AFFECTED( victim, AFF_SLOW ) )
    {
        if ( !check_dispel( level, victim, skill_lookup( "slow" ) ) )
        {
            if ( victim != ch )
                ch_printf( ch, "Spell failed.\r\n" );
            ch_printf( victim, "You feel momentarily faster.\r\n" );
            return;
        }
        act( "$n is moving less slowly.", victim, NULL, NULL, TO_ROOM );
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    if ( victim == ch )
        af.duration = level / 2;
    else
        af.duration = level / 4;
    af.location = APPLY_DEX;
    af.modifier = 1 + ( level >= 18 ) + ( level >= 25 ) + ( level >= 32 );
    af.bitvector = AFF_HASTE;
    affect_to_char( victim, &af );
    ch_printf( victim, "You feel yourself moving more quickly.\r\n" );
    act( "$n is moving more quickly.", victim, NULL, NULL, TO_ROOM );
    if ( ch != victim )
        ch_printf( ch, "Ok.\r\n" );
    return;
}

void spell_heal( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    victim->hit = UMIN( victim->hit + 100, victim->max_hit );
    update_pos( victim );
    ch_printf( victim, "A warm feeling fills your body.\r\n" );
    if ( ch != victim )
        ch_printf( ch, "Ok.\r\n" );
    return;
}

void spell_heat_metal( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    OBJ_DATA               *obj_lose = NULL;
    OBJ_DATA               *obj_next = NULL;
    int                     dam = 0;
    bool                    fail = true;

    if ( !saves_spell( level + 2, victim, DAM_FIRE )
         && !IS_SET( victim->imm_flags, IMM_FIRE ) )
    {
        for ( obj_lose = victim->carrying; obj_lose != NULL; obj_lose = obj_next )
        {
            obj_next = obj_lose->next_content;
            if ( number_range( 1, 2 * level ) > obj_lose->level
                 && !saves_spell( level, victim, DAM_FIRE )
                 && !IS_OBJ_STAT( obj_lose, ITEM_NONMETAL )
                 && !IS_OBJ_STAT( obj_lose, ITEM_BURN_PROOF ) )
            {
                switch ( obj_lose->item_type )
                {
                    case ITEM_ARMOR:
                        if ( obj_lose->wear_loc != -1 ) /* remove the item */
                        {
                            if ( can_drop_obj( victim, obj_lose )
                                 && ( obj_lose->weight / 10 ) <
                                 number_range( 1, 2 * get_curr_stat( victim, STAT_DEX ) )
                                 && remove_obj( victim, obj_lose->wear_loc, true ) )
                            {
                                act( "$n yelps and throws $p to the ground!",
                                     victim, obj_lose, NULL, TO_ROOM );
                                act( "You remove and drop $p before it burns you.",
                                     victim, obj_lose, NULL, TO_CHAR );
                                dam += ( number_range( 1, obj_lose->level ) / 3 );
                                obj_from_char( obj_lose );
                                obj_to_room( obj_lose, victim->in_room );
                                fail = false;
                            }
                            else                       /* stuck on the body! ouch! */
                            {
                                act( "Your skin is seared by $p!",
                                     victim, obj_lose, NULL, TO_CHAR );
                                dam += ( number_range( 1, obj_lose->level ) );
                                fail = false;
                            }

                        }
                        else                           /* drop it if we can */
                        {
                            if ( can_drop_obj( victim, obj_lose ) )
                            {
                                act( "$n yelps and throws $p to the ground!",
                                     victim, obj_lose, NULL, TO_ROOM );
                                act( "You and drop $p before it burns you.",
                                     victim, obj_lose, NULL, TO_CHAR );
                                dam += ( number_range( 1, obj_lose->level ) / 6 );
                                obj_from_char( obj_lose );
                                obj_to_room( obj_lose, victim->in_room );
                                fail = false;
                            }
                            else                       /* cannot drop */
                            {
                                act( "Your skin is seared by $p!",
                                     victim, obj_lose, NULL, TO_CHAR );
                                dam += ( number_range( 1, obj_lose->level ) / 2 );
                                fail = false;
                            }
                        }
                        break;
                    case ITEM_WEAPON:
                        if ( obj_lose->wear_loc != -1 ) /* try to drop it */
                        {
                            if ( IS_WEAPON_STAT( obj_lose, WEAPON_FLAMING ) )
                                continue;

                            if ( can_drop_obj( victim, obj_lose )
                                 && remove_obj( victim, obj_lose->wear_loc, true ) )
                            {
                                act( "$n is burned by $p, and throws it to the ground.",
                                     victim, obj_lose, NULL, TO_ROOM );
                                ch_printf( victim,
                                           "You throw your red-hot weapon to the ground!\r\n" );
                                dam += 1;
                                obj_from_char( obj_lose );
                                obj_to_room( obj_lose, victim->in_room );
                                fail = false;
                            }
                            else                       /* YOWCH! */
                            {
                                ch_printf( victim, "Your weapon sears your flesh!\r\n" );
                                dam += number_range( 1, obj_lose->level );
                                fail = false;
                            }
                        }
                        else                           /* drop it if we can */
                        {
                            if ( can_drop_obj( victim, obj_lose ) )
                            {
                                act( "$n throws a burning hot $p to the ground!",
                                     victim, obj_lose, NULL, TO_ROOM );
                                act( "You and drop $p before it burns you.",
                                     victim, obj_lose, NULL, TO_CHAR );
                                dam += ( number_range( 1, obj_lose->level ) / 6 );
                                obj_from_char( obj_lose );
                                obj_to_room( obj_lose, victim->in_room );
                                fail = false;
                            }
                            else                       /* cannot drop */
                            {
                                act( "Your skin is seared by $p!",
                                     victim, obj_lose, NULL, TO_CHAR );
                                dam += ( number_range( 1, obj_lose->level ) / 2 );
                                fail = false;
                            }
                        }
                        break;
                }
            }
        }
    }
    if ( fail )
    {
        ch_printf( ch, "Your spell had no effect.\r\n" );
        ch_printf( victim, "You feel momentarily warmer.\r\n" );
    }
    else                                               /* damage! */
    {
        if ( saves_spell( level, victim, DAM_FIRE ) )
            dam = 2 * dam / 3;
        damage( ch, victim, dam, sn, DAM_FIRE, true );
    }
}

/* RT really nasty high-level attack spell */
void spell_holy_word( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *vch = NULL;
    CHAR_DATA              *vch_next = NULL;
    int                     dam = 0;
    int                     bless_num = 0;
    int                     curse_num = 0;
    int                     frenzy_num = 0;

    bless_num = skill_lookup( "bless" );
    curse_num = skill_lookup( "curse" );
    frenzy_num = skill_lookup( "frenzy" );

    act( "$n utters a word of divine power!", ch, NULL, NULL, TO_ROOM );
    ch_printf( ch, "You utter a word of divine power.\r\n" );

    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next )
    {
        vch_next = vch->next_in_room;

        if ( ( IS_GOOD( ch ) && IS_GOOD( vch ) ) ||
             ( IS_EVIL( ch ) && IS_EVIL( vch ) ) ||
             ( IS_NEUTRAL( ch ) && IS_NEUTRAL( vch ) ) )
        {
            ch_printf( vch, "You feel full more powerful.\r\n" );
            spell_frenzy( frenzy_num, level, ch, ( void * ) vch, TARGET_CHAR );
            spell_bless( bless_num, level, ch, ( void * ) vch, TARGET_CHAR );
        }

        else if ( ( IS_GOOD( ch ) && IS_EVIL( vch ) ) ||
                  ( IS_EVIL( ch ) && IS_GOOD( vch ) ) )
        {
            if ( !is_safe_spell( ch, vch, true ) )
            {
                spell_curse( curse_num, level, ch, ( void * ) vch, TARGET_CHAR );
                ch_printf( vch, "You are struck down!\r\n" );
                dam = dice( level, 6 );
                damage( ch, vch, dam, sn, DAM_ENERGY, true );
            }
        }

        else if ( IS_NEUTRAL( ch ) )
        {
            if ( !is_safe_spell( ch, vch, true ) )
            {
                spell_curse( curse_num, level / 2, ch, ( void * ) vch, TARGET_CHAR );
                ch_printf( vch, "You are struck down!\r\n" );
                dam = dice( level, 4 );
                damage( ch, vch, dam, sn, DAM_ENERGY, true );
            }
        }
    }

    ch_printf( ch, "You feel drained.\r\n" );
    ch->move = 0;
    ch->hit /= 2;
}

void spell_identify( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) vo;
    AFFECT_DATA            *paf = NULL;

    ch_printf( ch,
               "Object '%s' is type %s, extra flags %s.\r\nWeight is %d, value is %d, level is %d.\r\n",
               obj->name,
               item_name( obj->item_type ),
               extra_bit_name( obj->extra_flags ),
               obj->weight / 10, obj->cost, obj->level );

    switch ( obj->item_type )
    {
        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_PILL:
            ch_printf( ch, "Level %d spells of:", obj->value[0] );

            if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL )
            {
                ch_printf( ch, " '%s'", skill_table[obj->value[1]].name );
            }

            if ( obj->value[2] >= 0 && obj->value[2] < MAX_SKILL )
            {
                ch_printf( ch, " '%s'", skill_table[obj->value[2]].name );
            }

            if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
            {
                ch_printf( ch, " '%s'", skill_table[obj->value[3]].name );
            }

            if ( obj->value[4] >= 0 && obj->value[4] < MAX_SKILL )
            {
                ch_printf( ch, " '%s'", skill_table[obj->value[4]].name );
            }

            ch_printf( ch, ".\r\n" );
            break;

        case ITEM_WAND:
        case ITEM_STAFF:
            ch_printf( ch, "Has %d charges of level %d", obj->value[2], obj->value[0] );

            if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
            {
                ch_printf( ch, " '%s'", skill_table[obj->value[3]].name );
            }

            ch_printf( ch, ".\r\n" );
            break;

        case ITEM_DRINK_CON:
            ch_printf( ch, "It holds %s-colored %s.\r\n",
                       liq_table[obj->value[2]].liq_color,
                       liq_table[obj->value[2]].liq_name );
            break;

        case ITEM_CONTAINER:
            ch_printf( ch, "Capacity: %d#  Maximum weight: %d#  flags: %s\r\n",
                       obj->value[0], obj->value[3], cont_bit_name( obj->value[1] ) );
            if ( obj->value[4] != 100 )
            {
                ch_printf( ch, "Weight multiplier: %d%%\r\n", obj->value[4] );
            }
            break;

        case ITEM_WEAPON:
            ch_printf( ch, "Weapon type is " );
            switch ( obj->value[0] )
            {
                case ( WEAPON_EXOTIC ):
                    ch_printf( ch, "exotic.\r\n" );
                    break;
                case ( WEAPON_SWORD ):
                    ch_printf( ch, "sword.\r\n" );
                    break;
                case ( WEAPON_DAGGER ):
                    ch_printf( ch, "dagger.\r\n" );
                    break;
                case ( WEAPON_SPEAR ):
                    ch_printf( ch, "spear/staff.\r\n" );
                    break;
                case ( WEAPON_MACE ):
                    ch_printf( ch, "mace/club.\r\n" );
                    break;
                case ( WEAPON_AXE ):
                    ch_printf( ch, "axe.\r\n" );
                    break;
                case ( WEAPON_FLAIL ):
                    ch_printf( ch, "flail.\r\n" );
                    break;
                case ( WEAPON_WHIP ):
                    ch_printf( ch, "whip.\r\n" );
                    break;
                case ( WEAPON_POLEARM ):
                    ch_printf( ch, "polearm.\r\n" );
                    break;
                default:
                    ch_printf( ch, "unknown.\r\n" );
                    break;
            }
            if ( obj->pIndexData->new_format )
                ch_printf( ch, "Damage is %dd%d (average %d).\r\n",
                           obj->value[1], obj->value[2],
                           ( 1 + obj->value[2] ) * obj->value[1] / 2 );
            else
                ch_printf( ch, "Damage is %d to %d (average %d).\r\n",
                           obj->value[1], obj->value[2],
                           ( obj->value[1] + obj->value[2] ) / 2 );
            if ( obj->value[4] )                       /* weapon flags */
            {
                ch_printf( ch, "Weapons flags: %s\r\n",
                           weapon_bit_name( obj->value[4] ) );
            }
            break;

        case ITEM_ARMOR:
            ch_printf( ch,
                       "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic.\r\n",
                       obj->value[0], obj->value[1], obj->value[2], obj->value[3] );
            break;
    }

    if ( !obj->enchanted )
        for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
        {
            if ( paf->location != APPLY_NONE && paf->modifier != 0 )
            {
                ch_printf( ch, "Affects %s by %d.\r\n",
                           affect_loc_name( paf->location ), paf->modifier );
                if ( paf->bitvector )
                {
                    switch ( paf->where )
                    {
                        case TO_AFFECTS:
                            ch_printf( ch, "Adds %s affect.\n",
                                       affect_bit_name( paf->bitvector ) );
                            break;
                        case TO_OBJECT:
                            ch_printf( ch, "Adds %s object flag.\n",
                                       extra_bit_name( paf->bitvector ) );
                            break;
                        case TO_IMMUNE:
                            ch_printf( ch, "Adds immunity to %s.\n",
                                       imm_bit_name( paf->bitvector ) );
                            break;
                        case TO_RESIST:
                            ch_printf( ch, "Adds resistance to %s.\r\n",
                                       imm_bit_name( paf->bitvector ) );
                            break;
                        case TO_VULN:
                            ch_printf( ch, "Adds vulnerability to %s.\r\n",
                                       imm_bit_name( paf->bitvector ) );
                            break;
                        default:
                            ch_printf( ch, "Unknown bit %d: %d\r\n",
                                       paf->where, paf->bitvector );
                            break;
                    }
                }
            }
        }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->location != APPLY_NONE && paf->modifier != 0 )
        {
            ch_printf( ch, "Affects %s by %d",
                       affect_loc_name( paf->location ), paf->modifier );
            if ( paf->duration > -1 )
                ch_printf( ch, ", %d hours.\r\n", paf->duration );
            else
                ch_printf( ch, ".\r\n" );
            if ( paf->bitvector )
            {
                switch ( paf->where )
                {
                    case TO_AFFECTS:
                        ch_printf( ch, "Adds %s affect.\n",
                                   affect_bit_name( paf->bitvector ) );
                        break;
                    case TO_OBJECT:
                        ch_printf( ch, "Adds %s object flag.\n",
                                   extra_bit_name( paf->bitvector ) );
                        break;
                    case TO_WEAPON:
                        ch_printf( ch, "Adds %s weapon flags.\n",
                                   weapon_bit_name( paf->bitvector ) );
                        break;
                    case TO_IMMUNE:
                        ch_printf( ch, "Adds immunity to %s.\n",
                                   imm_bit_name( paf->bitvector ) );
                        break;
                    case TO_RESIST:
                        ch_printf( ch, "Adds resistance to %s.\r\n",
                                   imm_bit_name( paf->bitvector ) );
                        break;
                    case TO_VULN:
                        ch_printf( ch, "Adds vulnerability to %s.\r\n",
                                   imm_bit_name( paf->bitvector ) );
                        break;
                    default:
                        ch_printf( ch, "Unknown bit %d: %d\r\n",
                                   paf->where, paf->bitvector );
                        break;
                }
            }
        }
    }

    return;
}

void spell_infravision( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( IS_AFFECTED( victim, AFF_INFRARED ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You can already see in the dark.\r\n" );
        else
            act( "$N already has infravision.\r\n", ch, NULL, victim, TO_CHAR );
        return;
    }
    act( "$n's eyes glow red.\r\n", ch, NULL, NULL, TO_ROOM );

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 2 * level;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_INFRARED;
    affect_to_char( victim, &af );
    ch_printf( victim, "Your eyes glow red.\r\n" );
    return;
}

void spell_invisibility( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = NULL;
    OBJ_DATA               *obj = NULL;
    AFFECT_DATA             af;

    /*
     * object invisibility 
     */
    if ( target == TARGET_OBJ )
    {
        obj = ( OBJ_DATA * ) vo;

        if ( IS_OBJ_STAT( obj, ITEM_INVIS ) )
        {
            act( "$p is already invisible.", ch, obj, NULL, TO_CHAR );
            return;
        }

        af.where = TO_OBJECT;
        af.type = sn;
        af.level = level;
        af.duration = level + 12;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = ITEM_INVIS;
        affect_to_obj( obj, &af );

        act( "$p fades out of sight.", ch, obj, NULL, TO_ALL );
        return;
    }

    /*
     * character invisibility 
     */
    victim = ( CHAR_DATA * ) vo;

    if ( IS_AFFECTED( victim, AFF_INVISIBLE ) )
        return;

    act( "$n fades out of existence.", victim, NULL, NULL, TO_ROOM );

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level + 12;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_INVISIBLE;
    affect_to_char( victim, &af );
    ch_printf( victim, "You fade out of existence.\r\n" );
    return;
}

void spell_know_alignment( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    const char             *msg = NULL;
    int                     ap = 0;

    ap = victim->alignment;

    if ( ap > 700 )
        msg = "$N has a pure and good aura.";
    else if ( ap > 350 )
        msg = "$N is of excellent moral character.";
    else if ( ap > 100 )
        msg = "$N is often kind and thoughtful.";
    else if ( ap > -100 )
        msg = "$N doesn't have a firm moral commitment.";
    else if ( ap > -350 )
        msg = "$N lies to $S friends.";
    else if ( ap > -700 )
        msg = "$N is a black-hearted murderer.";
    else
        msg = "$N is the embodiment of pure evil!.";

    act( msg, ch, NULL, victim, TO_CHAR );
    return;
}

void spell_lightning_bolt( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    static const int        dam_each[] = {
        0,
        0, 0, 0, 0, 0, 0, 0, 0, 25, 28,
        31, 34, 37, 40, 40, 41, 42, 42, 43, 44,
        44, 45, 46, 46, 47, 48, 48, 49, 50, 50,
        51, 52, 52, 53, 54, 54, 55, 56, 56, 57,
        58, 58, 59, 60, 60, 61, 62, 62, 63, 64
    };
    int                     dam = 0;

    level = UMIN( level, ( int ) ( sizeof( dam_each ) / sizeof( dam_each[0] ) - 1 ) );
    level = UMAX( 0, level );
    dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    if ( saves_spell( level, victim, DAM_LIGHTNING ) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_LIGHTNING, true );
    return;
}

void spell_locate_object( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    char                    buf[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    BUFFER                 *buffer = NULL;
    OBJ_DATA               *obj = NULL;
    OBJ_DATA               *in_obj = NULL;
    bool                    found = false;
    int                     number = 0;
    int                     max_found = 0;

    max_found = IS_IMMORTAL( ch ) ? 200 : 2 * level;

    buffer = new_buf(  );

    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
        if ( !can_see_obj( ch, obj ) || !is_name( target_name, obj->name )
             || IS_OBJ_STAT( obj, ITEM_NOLOCATE ) || number_percent(  ) > 2 * level
             || ch->level < obj->level )
            continue;

        found = true;
        number++;

        for ( in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj )
            ;

        if ( in_obj->carried_by != NULL && can_see( ch, in_obj->carried_by ) )
        {
            sprintf( buf, "one is carried by %s\r\n", PERS( in_obj->carried_by, ch ) );
        }
        else
        {
            if ( IS_IMMORTAL( ch ) && in_obj->in_room != NULL )
                sprintf( buf, "one is in %s [Room %d]\r\n",
                         in_obj->in_room->name, in_obj->in_room->vnum );
            else
                sprintf( buf, "one is in %s\r\n",
                         in_obj->in_room == NULL ? "somewhere" : in_obj->in_room->name );
        }

        buf[0] = UPPER( buf[0] );
        add_buf( buffer, buf );

        if ( number >= max_found )
            break;
    }

    if ( !found )
        ch_printf( ch, "Nothing like that in heaven or earth.\r\n" );
    else
        page_to_char( buf_string( buffer ), ch );

    free_buf( buffer );

    return;
}

void spell_magic_missile( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    static const int        dam_each[] = {
        0,
        3, 3, 4, 4, 5, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 8, 8, 8, 8, 8,
        9, 9, 9, 9, 9, 10, 10, 10, 10, 10,
        11, 11, 11, 11, 11, 12, 12, 12, 12, 12,
        13, 13, 13, 13, 13, 14, 14, 14, 14, 14
    };
    int                     dam = 0;

    level = UMIN( level, ( int ) ( sizeof( dam_each ) / sizeof( dam_each[0] ) ) - 1 );
    level = UMAX( 0, level );
    dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    if ( saves_spell( level, victim, DAM_ENERGY ) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_ENERGY, true );
    return;
}

void spell_mass_healing( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *gch = NULL;
    int                     heal_num = 0;
    int                     refresh_num = 0;

    heal_num = skill_lookup( "heal" );
    refresh_num = skill_lookup( "refresh" );

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( ( IS_NPC( ch ) && IS_NPC( gch ) ) || ( !IS_NPC( ch ) && !IS_NPC( gch ) ) )
        {
            spell_heal( heal_num, level, ch, ( void * ) gch, TARGET_CHAR );
            spell_refresh( refresh_num, level, ch, ( void * ) gch, TARGET_CHAR );
        }
    }
}

void spell_mass_invisibility( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA             af;
    CHAR_DATA              *gch = NULL;

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( !is_same_group( gch, ch ) || IS_AFFECTED( gch, AFF_INVISIBLE ) )
            continue;
        act( "$n slowly fades out of existence.", gch, NULL, NULL, TO_ROOM );
        ch_printf( gch, "You slowly fade out of existence.\r\n" );

        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = level / 2;
        af.duration = 24;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = AFF_INVISIBLE;
        affect_to_char( gch, &af );
    }
    ch_printf( ch, "Ok.\r\n" );

    return;
}

void spell_pass_door( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( IS_AFFECTED( victim, AFF_PASS_DOOR ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You are already out of phase.\r\n" );
        else
            act( "$N is already shifted out of phase.", ch, NULL, victim, TO_CHAR );
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = number_fuzzy( level / 4 );
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_PASS_DOOR;
    affect_to_char( victim, &af );
    act( "$n turns translucent.", victim, NULL, NULL, TO_ROOM );
    ch_printf( victim, "You turn translucent.\r\n" );
    return;
}

/* RT plague spell, very nasty */

void spell_plague( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( saves_spell( level, victim, DAM_DISEASE ) ||
         ( IS_NPC( victim ) && IS_SET( victim->act, ACT_UNDEAD ) ) )
    {
        if ( ch == victim )
            ch_printf( ch, "You feel momentarily ill, but it passes.\r\n" );
        else
            act( "$N seems to be unaffected.", ch, NULL, victim, TO_CHAR );
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level * 3 / 4;
    af.duration = level;
    af.location = APPLY_STR;
    af.modifier = -5;
    af.bitvector = AFF_PLAGUE;
    affect_join( victim, &af );

    ch_printf( victim, "You scream in agony as plague sores erupt from your skin.\r\n" );
    act( "$n screams in agony as plague sores erupt from $s skin.",
         victim, NULL, NULL, TO_ROOM );
}

void spell_poison( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = NULL;
    OBJ_DATA               *obj = NULL;
    AFFECT_DATA             af;

    if ( target == TARGET_OBJ )
    {
        obj = ( OBJ_DATA * ) vo;

        if ( obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON )
        {
            if ( IS_OBJ_STAT( obj, ITEM_BLESS ) || IS_OBJ_STAT( obj, ITEM_BURN_PROOF ) )
            {
                act( "Your spell fails to corrupt $p.", ch, obj, NULL, TO_CHAR );
                return;
            }
            obj->value[3] = 1;
            act( "$p is infused with poisonous vapors.", ch, obj, NULL, TO_ALL );
            return;
        }

        if ( obj->item_type == ITEM_WEAPON )
        {
            if ( IS_WEAPON_STAT( obj, WEAPON_FLAMING )
                 || IS_WEAPON_STAT( obj, WEAPON_FROST )
                 || IS_WEAPON_STAT( obj, WEAPON_VAMPIRIC )
                 || IS_WEAPON_STAT( obj, WEAPON_SHARP )
                 || IS_WEAPON_STAT( obj, WEAPON_VORPAL )
                 || IS_WEAPON_STAT( obj, WEAPON_SHOCKING )
                 || IS_OBJ_STAT( obj, ITEM_BLESS )
                 || IS_OBJ_STAT( obj, ITEM_BURN_PROOF ) )
            {
                act( "You can't seem to envenom $p.", ch, obj, NULL, TO_CHAR );
                return;
            }

            if ( IS_WEAPON_STAT( obj, WEAPON_POISON ) )
            {
                act( "$p is already envenomed.", ch, obj, NULL, TO_CHAR );
                return;
            }

            af.where = TO_WEAPON;
            af.type = sn;
            af.level = level / 2;
            af.duration = level / 8;
            af.location = 0;
            af.modifier = 0;
            af.bitvector = WEAPON_POISON;
            affect_to_obj( obj, &af );

            act( "$p is coated with deadly venom.", ch, obj, NULL, TO_ALL );
            return;
        }

        act( "You can't poison $p.", ch, obj, NULL, TO_CHAR );
        return;
    }

    victim = ( CHAR_DATA * ) vo;

    if ( saves_spell( level, victim, DAM_POISON ) )
    {
        act( "$n turns slightly green, but it passes.", victim, NULL, NULL, TO_ROOM );
        ch_printf( victim, "You feel momentarily ill, but it passes.\r\n" );
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.location = APPLY_STR;
    af.modifier = -2;
    af.bitvector = AFF_POISON;
    affect_join( victim, &af );
    ch_printf( victim, "You feel very sick.\r\n" );
    act( "$n looks very ill.", victim, NULL, NULL, TO_ROOM );
    return;
}

void spell_protection_evil( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( IS_AFFECTED( victim, AFF_PROTECT_EVIL )
         || IS_AFFECTED( victim, AFF_PROTECT_GOOD ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You are already protected.\r\n" );
        else
            act( "$N is already protected.", ch, NULL, victim, TO_CHAR );
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 24;
    af.location = APPLY_SAVING_SPELL;
    af.modifier = -1;
    af.bitvector = AFF_PROTECT_EVIL;
    affect_to_char( victim, &af );
    ch_printf( victim, "You feel holy and pure.\r\n" );
    if ( ch != victim )
        act( "$N is protected from evil.", ch, NULL, victim, TO_CHAR );
    return;
}

void spell_protection_good( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( IS_AFFECTED( victim, AFF_PROTECT_GOOD )
         || IS_AFFECTED( victim, AFF_PROTECT_EVIL ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You are already protected.\r\n" );
        else
            act( "$N is already protected.", ch, NULL, victim, TO_CHAR );
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 24;
    af.location = APPLY_SAVING_SPELL;
    af.modifier = -1;
    af.bitvector = AFF_PROTECT_GOOD;
    affect_to_char( victim, &af );
    ch_printf( victim, "You feel aligned with darkness.\r\n" );
    if ( ch != victim )
        act( "$N is protected from good.", ch, NULL, victim, TO_CHAR );
    return;
}

void spell_ray_of_truth( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam = 0;
    int                     align = 0;

    if ( IS_EVIL( ch ) )
    {
        victim = ch;
        ch_printf( ch, "The energy explodes inside you!\r\n" );
    }

    if ( victim != ch )
    {
        act( "$n raises $s hand, and a blinding ray of light shoots forth!",
             ch, NULL, NULL, TO_ROOM );
        ch_printf( ch,
                   "You raise your hand and a blinding ray of light shoots forth!\r\n" );
    }

    if ( IS_GOOD( victim ) )
    {
        act( "$n seems unharmed by the light.", victim, NULL, victim, TO_ROOM );
        ch_printf( victim, "The light seems powerless to affect you.\r\n" );
        return;
    }

    dam = dice( level, 10 );
    if ( saves_spell( level, victim, DAM_HOLY ) )
        dam /= 2;

    align = victim->alignment;
    align -= 350;

    if ( align < -1000 )
        align = -1000 + ( align + 1000 ) / 3;

    dam = ( dam * align * align ) / 1000000;

    damage( ch, victim, dam, sn, DAM_HOLY, true );
    spell_blindness( skill_lookup( "blindness" ), 3 * level / 4, ch, ( void * ) victim,
                     TARGET_CHAR );
}

void spell_recharge( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) vo;
    int                     chance = 0;
    int                     percent = 0;

    if ( obj->item_type != ITEM_WAND && obj->item_type != ITEM_STAFF )
    {
        ch_printf( ch, "That item does not carry charges.\r\n" );
        return;
    }

    if ( obj->value[3] >= 3 * level / 2 )
    {
        ch_printf( ch, "Your skills are not great enough for that.\r\n" );
        return;
    }

    if ( obj->value[1] == 0 )
    {
        ch_printf( ch, "That item has already been recharged once.\r\n" );
        return;
    }

    chance = 40 + 2 * level;

    chance -= obj->value[3];                           /* harder to do high-level spells */
    chance -= ( obj->value[1] - obj->value[2] ) * ( obj->value[1] - obj->value[2] );

    chance = UMAX( level / 2, chance );

    percent = number_percent(  );

    if ( percent < chance / 2 )
    {
        act( "$p glows softly.", ch, obj, NULL, TO_CHAR );
        act( "$p glows softly.", ch, obj, NULL, TO_ROOM );
        obj->value[2] = UMAX( obj->value[1], obj->value[2] );
        obj->value[1] = 0;
        return;
    }

    else if ( percent <= chance )
    {
        int                     chargeback,
                                chargemax;

        act( "$p glows softly.", ch, obj, NULL, TO_CHAR );
        act( "$p glows softly.", ch, obj, NULL, TO_CHAR );

        chargemax = obj->value[1] - obj->value[2];

        if ( chargemax > 0 )
            chargeback = UMAX( 1, chargemax * percent / 100 );
        else
            chargeback = 0;

        obj->value[2] += chargeback;
        obj->value[1] = 0;
        return;
    }

    else if ( percent <= UMIN( 95, 3 * chance / 2 ) )
    {
        ch_printf( ch, "Nothing seems to happen.\r\n" );
        if ( obj->value[1] > 1 )
            obj->value[1]--;
        return;
    }

    else                                               /* whoops! */
    {
        act( "$p glows brightly and explodes!", ch, obj, NULL, TO_CHAR );
        act( "$p glows brightly and explodes!", ch, obj, NULL, TO_ROOM );
        extract_obj( obj );
    }
}

void spell_refresh( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    victim->move = UMIN( victim->move + level, victim->max_move );
    if ( victim->max_move == victim->move )
        ch_printf( victim, "You feel fully refreshed!\r\n" );
    else
        ch_printf( victim, "You feel less tired.\r\n" );
    if ( ch != victim )
        ch_printf( ch, "Ok.\r\n" );
    return;
}

void spell_remove_curse( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = NULL;
    OBJ_DATA               *obj = NULL;
    bool                    found = false;

    /*
     * do object cases first 
     */
    if ( target == TARGET_OBJ )
    {
        obj = ( OBJ_DATA * ) vo;

        if ( IS_OBJ_STAT( obj, ITEM_NODROP ) || IS_OBJ_STAT( obj, ITEM_NOREMOVE ) )
        {
            if ( !IS_OBJ_STAT( obj, ITEM_NOUNCURSE )
                 && !saves_dispel( level + 2, obj->level, 0 ) )
            {
                REMOVE_BIT( obj->extra_flags, ITEM_NODROP );
                REMOVE_BIT( obj->extra_flags, ITEM_NOREMOVE );
                act( "$p glows blue.", ch, obj, NULL, TO_ALL );
                return;
            }

            act( "The curse on $p is beyond your power.", ch, obj, NULL, TO_CHAR );
            return;
        }
        act( "There doesn't seem to be a curse on $p.", ch, obj, NULL, TO_CHAR );
        return;
    }

    /*
     * characters 
     */
    victim = ( CHAR_DATA * ) vo;

    if ( check_dispel( level, victim, skill_lookup( "curse" ) ) )
    {
        ch_printf( victim, "You feel better.\r\n" );
        act( "$n looks more relaxed.", victim, NULL, NULL, TO_ROOM );
    }

    for ( obj = victim->carrying; ( obj != NULL && !found ); obj = obj->next_content )
    {
        if ( ( IS_OBJ_STAT( obj, ITEM_NODROP ) || IS_OBJ_STAT( obj, ITEM_NOREMOVE ) )
             && !IS_OBJ_STAT( obj, ITEM_NOUNCURSE ) )
        {                                              /* attempt to remove curse */
            if ( !saves_dispel( level, obj->level, 0 ) )
            {
                found = true;
                REMOVE_BIT( obj->extra_flags, ITEM_NODROP );
                REMOVE_BIT( obj->extra_flags, ITEM_NOREMOVE );
                act( "Your $p glows blue.", victim, obj, NULL, TO_CHAR );
                act( "$n's $p glows blue.", victim, obj, NULL, TO_ROOM );
            }
        }
    }
}

void spell_sanctuary( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( IS_AFFECTED( victim, AFF_SANCTUARY ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You are already in sanctuary.\r\n" );
        else
            act( "$N is already in sanctuary.", ch, NULL, victim, TO_CHAR );
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level / 6;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_SANCTUARY;
    affect_to_char( victim, &af );
    act( "$n is surrounded by a white aura.", victim, NULL, NULL, TO_ROOM );
    ch_printf( victim, "You are surrounded by a white aura.\r\n" );
    return;
}

void spell_shield( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( is_affected( victim, sn ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You are already shielded from harm.\r\n" );
        else
            act( "$N is already protected by a shield.", ch, NULL, victim, TO_CHAR );
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 8 + level;
    af.location = APPLY_AC;
    af.modifier = -20;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    act( "$n is surrounded by a force shield.", victim, NULL, NULL, TO_ROOM );
    ch_printf( victim, "You are surrounded by a force shield.\r\n" );
    return;
}

void spell_shocking_grasp( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    static const int        dam_each[] = {
        0,
        0, 0, 0, 0, 0, 0, 20, 25, 29, 33,
        36, 39, 39, 39, 40, 40, 41, 41, 42, 42,
        43, 43, 44, 44, 45, 45, 46, 46, 47, 47,
        48, 48, 49, 49, 50, 50, 51, 51, 52, 52,
        53, 53, 54, 54, 55, 55, 56, 56, 57, 57
    };
    int                     dam = 0;

    level = UMIN( level, ( int ) ( sizeof( dam_each ) / sizeof( dam_each[0] ) ) - 1 );
    level = UMAX( 0, level );
    dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    if ( saves_spell( level, victim, DAM_LIGHTNING ) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_LIGHTNING, true );
    return;
}

void spell_sleep( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( IS_AFFECTED( victim, AFF_SLEEP )
         || ( IS_NPC( victim ) && IS_SET( victim->act, ACT_UNDEAD ) )
         || ( level + 2 ) < victim->level || saves_spell( level - 4, victim, DAM_CHARM ) )
        return;

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 4 + level;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_SLEEP;
    affect_join( victim, &af );

    if ( IS_AWAKE( victim ) )
    {
        ch_printf( victim, "You feel very sleepy ..... zzzzzz.\r\n" );
        act( "$n goes to sleep.", victim, NULL, NULL, TO_ROOM );
        victim->position = POS_SLEEPING;
    }
    return;
}

void spell_slow( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( is_affected( victim, sn ) || IS_AFFECTED( victim, AFF_SLOW ) )
    {
        if ( victim == ch )
            ch_printf( ch, "You can't move any slower!\r\n" );
        else
            act( "$N can't get any slower than that.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( saves_spell( level, victim, DAM_OTHER )
         || IS_SET( victim->imm_flags, IMM_MAGIC ) )
    {
        if ( victim != ch )
            ch_printf( ch, "Nothing seemed to happen.\r\n" );
        ch_printf( victim, "You feel momentarily lethargic.\r\n" );
        return;
    }

    if ( IS_AFFECTED( victim, AFF_HASTE ) )
    {
        if ( !check_dispel( level, victim, skill_lookup( "haste" ) ) )
        {
            if ( victim != ch )
                ch_printf( ch, "Spell failed.\r\n" );
            ch_printf( victim, "You feel momentarily slower.\r\n" );
            return;
        }

        act( "$n is moving less quickly.", victim, NULL, NULL, TO_ROOM );
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level / 2;
    af.location = APPLY_DEX;
    af.modifier = -1 - ( level >= 18 ) - ( level >= 25 ) - ( level >= 32 );
    af.bitvector = AFF_SLOW;
    affect_to_char( victim, &af );
    ch_printf( victim, "You feel yourself slowing d o w n...\r\n" );
    act( "$n starts to move in slow motion.", victim, NULL, NULL, TO_ROOM );
    return;
}

void spell_stone_skin( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( is_affected( ch, sn ) )
    {
        if ( victim == ch )
            ch_printf( ch, "Your skin is already as hard as a rock.\r\n" );
        else
            act( "$N is already as hard as can be.", ch, NULL, victim, TO_CHAR );
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.location = APPLY_AC;
    af.modifier = -40;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    act( "$n's skin turns to stone.", victim, NULL, NULL, TO_ROOM );
    ch_printf( victim, "Your skin turns to stone.\r\n" );
    return;
}

void spell_summon( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = NULL;

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL
         || victim == ch
         || victim->in_room == NULL
         || IS_SET( ch->in_room->room_flags, ROOM_SAFE )
         || IS_SET( victim->in_room->room_flags, ROOM_SAFE )
         || IS_SET( victim->in_room->room_flags, ROOM_PRIVATE )
         || IS_SET( victim->in_room->room_flags, ROOM_SOLITARY )
         || IS_SET( victim->in_room->room_flags, ROOM_NO_RECALL )
         || ( IS_NPC( victim ) && IS_SET( victim->act, ACT_AGGRESSIVE ) )
         || victim->level >= level + 3
         || ( !IS_NPC( victim ) && victim->level >= LEVEL_IMMORTAL )
         || victim->fighting != NULL
         || ( IS_NPC( victim ) && IS_SET( victim->imm_flags, IMM_SUMMON ) )
         || ( IS_NPC( victim ) && victim->pIndexData->pShop != NULL )
         || ( !IS_NPC( victim ) && IS_SET( victim->act, PLR_NOSUMMON ) )
         || ( IS_NPC( victim ) && saves_spell( level, victim, DAM_OTHER ) ) )

    {
        ch_printf( ch, "You failed.\r\n" );
        return;
    }

    act( "$n disappears suddenly.", victim, NULL, NULL, TO_ROOM );
    char_from_room( victim );
    char_to_room( victim, ch->in_room );
    act( "$n arrives suddenly.", victim, NULL, NULL, TO_ROOM );
    act( "$n has summoned you!", ch, NULL, victim, TO_VICT );
    do_function( victim, &do_look, "auto" );
    return;
}

void spell_teleport( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    ROOM_INDEX_DATA        *pRoomIndex = NULL;

    if ( victim->in_room == NULL
         || IS_SET( victim->in_room->room_flags, ROOM_NO_RECALL )
         || ( victim != ch && IS_SET( victim->imm_flags, IMM_SUMMON ) )
         || ( !IS_NPC( ch ) && victim->fighting != NULL )
         || ( victim != ch && ( saves_spell( level - 5, victim, DAM_OTHER ) ) ) )
    {
        ch_printf( ch, "You failed.\r\n" );
        return;
    }

    pRoomIndex = get_random_room( victim );

    if ( victim != ch )
        ch_printf( victim, "You have been teleported!\r\n" );

    act( "$n vanishes!", victim, NULL, NULL, TO_ROOM );
    char_from_room( victim );
    char_to_room( victim, pRoomIndex );
    act( "$n slowly fades into existence.", victim, NULL, NULL, TO_ROOM );
    do_function( victim, &do_look, "auto" );
    return;
}

void spell_ventriloquate( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    char                    buf1[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    buf2[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";
    char                    speaker[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *vch = NULL;

    target_name = one_argument( target_name, speaker );

    sprintf( buf1, "%s says '%s'.\r\n", speaker, target_name );
    sprintf( buf2, "Someone makes %s say '%s'.\r\n", speaker, target_name );
    buf1[0] = UPPER( buf1[0] );

    for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room )
    {
        if ( !is_exact_name( speaker, vch->name ) && IS_AWAKE( vch ) )
            send_to_char( saves_spell( level, vch, DAM_OTHER ) ? buf2 : buf1, vch );
    }

    return;
}

void spell_weaken( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( is_affected( victim, sn ) || saves_spell( level, victim, DAM_OTHER ) )
        return;

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level / 2;
    af.location = APPLY_STR;
    af.modifier = -1 * ( level / 5 );
    af.bitvector = AFF_WEAKEN;
    affect_to_char( victim, &af );
    ch_printf( victim, "You feel your strength slip away.\r\n" );
    act( "$n looks tired and weak.", victim, NULL, NULL, TO_ROOM );
    return;
}

/* RT recall spell is back */

void spell_word_of_recall( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    ROOM_INDEX_DATA        *location = NULL;

    if ( IS_NPC( victim ) )
        return;

    if ( ( location = get_room_index( ROOM_VNUM_TEMPLE ) ) == NULL )
    {
        ch_printf( victim, "You are completely lost.\r\n" );
        return;
    }

    if ( IS_SET( victim->in_room->room_flags, ROOM_NO_RECALL ) ||
         IS_AFFECTED( victim, AFF_CURSE ) )
    {
        ch_printf( victim, "Spell failed.\r\n" );
        return;
    }

    if ( victim->fighting != NULL )
        stop_fighting( victim, true );

    ch->move /= 2;
    act( "$n disappears.", victim, NULL, NULL, TO_ROOM );
    char_from_room( victim );
    char_to_room( victim, location );
    act( "$n appears in the room.", victim, NULL, NULL, TO_ROOM );
    do_function( victim, &do_look, "auto" );
}

/*
 * NPC spells.
 */
void spell_acid_breath( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam = 0;
    int                     hp_dam = 0;
    int                     dice_dam = 0;
    int                     hpch = 0;

    act( "$n spits acid at $N.", ch, NULL, victim, TO_NOTVICT );
    act( "$n spits a stream of corrosive acid at you.", ch, NULL, victim, TO_VICT );
    act( "You spit acid at $N.", ch, NULL, victim, TO_CHAR );

    hpch = UMAX( 12, ch->hit );
    hp_dam = number_range( hpch / 11 + 1, hpch / 6 );
    dice_dam = dice( level, 16 );

    dam = UMAX( hp_dam + dice_dam / 10, dice_dam + hp_dam / 10 );

    if ( saves_spell( level, victim, DAM_ACID ) )
    {
        acid_effect( victim, level / 2, dam / 4, TARGET_CHAR );
        damage( ch, victim, dam / 2, sn, DAM_ACID, true );
    }
    else
    {
        acid_effect( victim, level, dam, TARGET_CHAR );
        damage( ch, victim, dam, sn, DAM_ACID, true );
    }
}

void spell_fire_breath( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    CHAR_DATA              *vch = NULL;
    CHAR_DATA              *vch_next = NULL;
    int                     dam = 0;
    int                     hp_dam = 0;
    int                     dice_dam = 0;
    int                     hpch = 0;

    act( "$n breathes forth a cone of fire.", ch, NULL, victim, TO_NOTVICT );
    act( "$n breathes a cone of hot fire over you!", ch, NULL, victim, TO_VICT );
    act( "You breath forth a cone of fire.", ch, NULL, NULL, TO_CHAR );

    hpch = UMAX( 10, ch->hit );
    hp_dam = number_range( hpch / 9 + 1, hpch / 5 );
    dice_dam = dice( level, 20 );

    dam = UMAX( hp_dam + dice_dam / 10, dice_dam + hp_dam / 10 );
    fire_effect( victim->in_room, level, dam / 2, TARGET_ROOM );

    for ( vch = victim->in_room->people; vch != NULL; vch = vch_next )
    {
        vch_next = vch->next_in_room;

        if ( is_safe_spell( ch, vch, true )
             || ( IS_NPC( vch ) && IS_NPC( ch )
                  && ( ch->fighting != vch || vch->fighting != ch ) ) )
            continue;

        if ( vch == victim )                           /* full damage */
        {
            if ( saves_spell( level, vch, DAM_FIRE ) )
            {
                fire_effect( vch, level / 2, dam / 4, TARGET_CHAR );
                damage( ch, vch, dam / 2, sn, DAM_FIRE, true );
            }
            else
            {
                fire_effect( vch, level, dam, TARGET_CHAR );
                damage( ch, vch, dam, sn, DAM_FIRE, true );
            }
        }
        else                                           /* partial damage */
        {
            if ( saves_spell( level - 2, vch, DAM_FIRE ) )
            {
                fire_effect( vch, level / 4, dam / 8, TARGET_CHAR );
                damage( ch, vch, dam / 4, sn, DAM_FIRE, true );
            }
            else
            {
                fire_effect( vch, level / 2, dam / 4, TARGET_CHAR );
                damage( ch, vch, dam / 2, sn, DAM_FIRE, true );
            }
        }
    }
}

void spell_frost_breath( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    CHAR_DATA              *vch = NULL;
    CHAR_DATA              *vch_next = NULL;
    int                     dam = 0;
    int                     hp_dam = 0;
    int                     dice_dam = 0;
    int                     hpch = 0;

    act( "$n breathes out a freezing cone of frost!", ch, NULL, victim, TO_NOTVICT );
    act( "$n breathes a freezing cone of frost over you!", ch, NULL, victim, TO_VICT );
    act( "You breath out a cone of frost.", ch, NULL, NULL, TO_CHAR );

    hpch = UMAX( 12, ch->hit );
    hp_dam = number_range( hpch / 11 + 1, hpch / 6 );
    dice_dam = dice( level, 16 );

    dam = UMAX( hp_dam + dice_dam / 10, dice_dam + hp_dam / 10 );
    cold_effect( victim->in_room, level, dam / 2, TARGET_ROOM );

    for ( vch = victim->in_room->people; vch != NULL; vch = vch_next )
    {
        vch_next = vch->next_in_room;

        if ( is_safe_spell( ch, vch, true )
             || ( IS_NPC( vch ) && IS_NPC( ch )
                  && ( ch->fighting != vch || vch->fighting != ch ) ) )
            continue;

        if ( vch == victim )                           /* full damage */
        {
            if ( saves_spell( level, vch, DAM_COLD ) )
            {
                cold_effect( vch, level / 2, dam / 4, TARGET_CHAR );
                damage( ch, vch, dam / 2, sn, DAM_COLD, true );
            }
            else
            {
                cold_effect( vch, level, dam, TARGET_CHAR );
                damage( ch, vch, dam, sn, DAM_COLD, true );
            }
        }
        else
        {
            if ( saves_spell( level - 2, vch, DAM_COLD ) )
            {
                cold_effect( vch, level / 4, dam / 8, TARGET_CHAR );
                damage( ch, vch, dam / 4, sn, DAM_COLD, true );
            }
            else
            {
                cold_effect( vch, level / 2, dam / 4, TARGET_CHAR );
                damage( ch, vch, dam / 2, sn, DAM_COLD, true );
            }
        }
    }
}

void spell_gas_breath( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *vch = NULL;
    CHAR_DATA              *vch_next = NULL;
    int                     dam = 0;
    int                     hp_dam = 0;
    int                     dice_dam = 0;
    int                     hpch = 0;

    act( "$n breathes out a cloud of poisonous gas!", ch, NULL, NULL, TO_ROOM );
    act( "You breath out a cloud of poisonous gas.", ch, NULL, NULL, TO_CHAR );

    hpch = UMAX( 16, ch->hit );
    hp_dam = number_range( hpch / 15 + 1, 8 );
    dice_dam = dice( level, 12 );

    dam = UMAX( hp_dam + dice_dam / 10, dice_dam + hp_dam / 10 );
    poison_effect( ch->in_room, level, dam, TARGET_ROOM );

    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next )
    {
        vch_next = vch->next_in_room;

        if ( is_safe_spell( ch, vch, true )
             || ( IS_NPC( ch ) && IS_NPC( vch )
                  && ( ch->fighting == vch || vch->fighting == ch ) ) )
            continue;

        if ( saves_spell( level, vch, DAM_POISON ) )
        {
            poison_effect( vch, level / 2, dam / 4, TARGET_CHAR );
            damage( ch, vch, dam / 2, sn, DAM_POISON, true );
        }
        else
        {
            poison_effect( vch, level, dam, TARGET_CHAR );
            damage( ch, vch, dam, sn, DAM_POISON, true );
        }
    }
}

void spell_lightning_breath( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam = 0;
    int                     hp_dam = 0;
    int                     dice_dam = 0;
    int                     hpch = 0;

    act( "$n breathes a bolt of lightning at $N.", ch, NULL, victim, TO_NOTVICT );
    act( "$n breathes a bolt of lightning at you!", ch, NULL, victim, TO_VICT );
    act( "You breathe a bolt of lightning at $N.", ch, NULL, victim, TO_CHAR );

    hpch = UMAX( 10, ch->hit );
    hp_dam = number_range( hpch / 9 + 1, hpch / 5 );
    dice_dam = dice( level, 20 );

    dam = UMAX( hp_dam + dice_dam / 10, dice_dam + hp_dam / 10 );

    if ( saves_spell( level, victim, DAM_LIGHTNING ) )
    {
        shock_effect( victim, level / 2, dam / 4, TARGET_CHAR );
        damage( ch, victim, dam / 2, sn, DAM_LIGHTNING, true );
    }
    else
    {
        shock_effect( victim, level, dam, TARGET_CHAR );
        damage( ch, victim, dam, sn, DAM_LIGHTNING, true );
    }
}

/*
 * Spells for mega1.are from Glop/Erkenbrand.
 */
void spell_general_purpose( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam = 0;

    dam = number_range( 25, 100 );
    if ( saves_spell( level, victim, DAM_PIERCE ) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_PIERCE, true );
    return;
}

void spell_high_explosive( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    int                     dam = 0;

    dam = number_range( 30, 120 );
    if ( saves_spell( level, victim, DAM_PIERCE ) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_PIERCE, true );
    return;
}

void spell_farsight( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    if ( IS_AFFECTED( ch, AFF_BLIND ) )
    {
        ch_printf( ch, "Maybe it would help if you could see?\r\n" );
        return;
    }

    do_function( ch, &do_scan, target_name );
}

void spell_portal( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = NULL;
    OBJ_DATA               *portal = NULL;
    OBJ_DATA               *stone = NULL;

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL || victim == ch
         || victim->in_room == NULL || !can_see_room( ch, victim->in_room )
         || IS_SET( victim->in_room->room_flags, ROOM_SAFE )
         || IS_SET( victim->in_room->room_flags, ROOM_PRIVATE )
         || IS_SET( victim->in_room->room_flags, ROOM_SOLITARY )
         || IS_SET( victim->in_room->room_flags, ROOM_NO_RECALL )
         || IS_SET( ch->in_room->room_flags, ROOM_NO_RECALL )
         || victim->level >= level + 3 || ( !IS_NPC( victim )
                                            && victim->level >= LEVEL_HERO )
         || ( IS_NPC( victim ) && IS_SET( victim->imm_flags, IMM_SUMMON ) )
         || ( IS_NPC( victim ) && saves_spell( level, victim, DAM_NONE ) )
         || ( is_clan( victim ) && !is_same_clan( ch, victim ) ) )
    {
        ch_printf( ch, "You failed.\r\n" );
        return;
    }

    stone = get_eq_char( ch, WEAR_HOLD );
    if ( !IS_IMMORTAL( ch ) && ( stone == NULL || stone->item_type != ITEM_WARP_STONE ) )
    {
        ch_printf( ch, "You lack the proper component for this spell.\r\n" );
        return;
    }

    if ( stone != NULL && stone->item_type == ITEM_WARP_STONE )
    {
        act( "You draw upon the power of $p.", ch, stone, NULL, TO_CHAR );
        act( "It flares brightly and vanishes!", ch, stone, NULL, TO_CHAR );
        extract_obj( stone );
    }

    portal = create_object( get_obj_index( OBJ_VNUM_PORTAL ), 0 );
    portal->timer = 2 + level / 25;
    portal->value[3] = victim->in_room->vnum;

    obj_to_room( portal, ch->in_room );

    act( "$p rises up from the ground.", ch, portal, NULL, TO_ROOM );
    act( "$p rises up before you.", ch, portal, NULL, TO_CHAR );
}

void spell_nexus( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA              *victim = NULL;
    OBJ_DATA               *portal = NULL;
    OBJ_DATA               *stone = NULL;
    ROOM_INDEX_DATA        *to_room = NULL;
    ROOM_INDEX_DATA        *from_room = NULL;

    from_room = ch->in_room;

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL || victim == ch
         || ( to_room = victim->in_room ) == NULL || !can_see_room( ch, to_room )
         || !can_see_room( ch, from_room ) || IS_SET( to_room->room_flags, ROOM_SAFE )
         || IS_SET( from_room->room_flags, ROOM_SAFE )
         || IS_SET( to_room->room_flags, ROOM_PRIVATE )
         || IS_SET( to_room->room_flags, ROOM_SOLITARY )
         || IS_SET( to_room->room_flags, ROOM_NO_RECALL )
         || IS_SET( from_room->room_flags, ROOM_NO_RECALL ) || victim->level >= level + 3
         || ( !IS_NPC( victim ) && victim->level >= LEVEL_HERO ) || ( IS_NPC( victim )
                                                                      && IS_SET( victim->
                                                                                 imm_flags,
                                                                                 IMM_SUMMON ) )
         || ( IS_NPC( victim ) && saves_spell( level, victim, DAM_NONE ) )
         || ( is_clan( victim ) && !is_same_clan( ch, victim ) ) )
    {
        ch_printf( ch, "You failed.\r\n" );
        return;
    }

    stone = get_eq_char( ch, WEAR_HOLD );
    if ( !IS_IMMORTAL( ch ) && ( stone == NULL || stone->item_type != ITEM_WARP_STONE ) )
    {
        ch_printf( ch, "You lack the proper component for this spell.\r\n" );
        return;
    }

    if ( stone != NULL && stone->item_type == ITEM_WARP_STONE )
    {
        act( "You draw upon the power of $p.", ch, stone, NULL, TO_CHAR );
        act( "It flares brightly and vanishes!", ch, stone, NULL, TO_CHAR );
        extract_obj( stone );
    }

    /*
     * portal one 
     */
    portal = create_object( get_obj_index( OBJ_VNUM_PORTAL ), 0 );
    portal->timer = 1 + level / 10;
    portal->value[3] = to_room->vnum;

    obj_to_room( portal, from_room );

    act( "$p rises up from the ground.", ch, portal, NULL, TO_ROOM );
    act( "$p rises up before you.", ch, portal, NULL, TO_CHAR );

    /*
     * no second portal if rooms are the same 
     */
    if ( to_room == from_room )
        return;

    /*
     * portal two 
     */
    portal = create_object( get_obj_index( OBJ_VNUM_PORTAL ), 0 );
    portal->timer = 1 + level / 10;
    portal->value[3] = from_room->vnum;

    obj_to_room( portal, to_room );

    if ( to_room->people != NULL )
    {
        act( "$p rises up from the ground.", to_room->people, portal, NULL, TO_ROOM );
        act( "$p rises up from the ground.", to_room->people, portal, NULL, TO_CHAR );
    }
}

void acid_effect( void *vo, int level, int dam, int target )
{
    if ( target == TARGET_ROOM )                       /* nail objects on the floor */
    {
        ROOM_INDEX_DATA        *room = ( ROOM_INDEX_DATA * ) vo;
        OBJ_DATA               *obj = NULL;
        OBJ_DATA               *obj_next = NULL;

        for ( obj = room->contents; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;
            acid_effect( obj, level, dam, TARGET_OBJ );
        }
        return;
    }

    if ( target == TARGET_CHAR )                       /* do the effect on a victim */
    {
        CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
        OBJ_DATA               *obj = NULL;
        OBJ_DATA               *obj_next = NULL;

        /*
         * let's toast some gear 
         */
        for ( obj = victim->carrying; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;
            acid_effect( obj, level, dam, TARGET_OBJ );
        }
        return;
    }

    if ( target == TARGET_OBJ )                        /* toast an object */
    {
        OBJ_DATA               *obj = ( OBJ_DATA * ) vo;
        OBJ_DATA               *t_obj = NULL;
        OBJ_DATA               *n_obj = NULL;
        int                     chance = 0;
        const char             *msg = NULL;

        if ( IS_OBJ_STAT( obj, ITEM_BURN_PROOF )
             || IS_OBJ_STAT( obj, ITEM_NOPURGE ) || number_range( 0, 4 ) == 0 )
            return;

        chance = level / 4 + dam / 10;

        if ( chance > 25 )
            chance = ( chance - 25 ) / 2 + 25;
        if ( chance > 50 )
            chance = ( chance - 50 ) / 2 + 50;

        if ( IS_OBJ_STAT( obj, ITEM_BLESS ) )
            chance -= 5;

        chance -= obj->level * 2;

        switch ( obj->item_type )
        {
            default:
                return;
            case ITEM_CONTAINER:
            case ITEM_CORPSE_PC:
            case ITEM_CORPSE_NPC:
                msg = "$p fumes and dissolves.";
                break;
            case ITEM_ARMOR:
                msg = "$p is pitted and etched.";
                break;
            case ITEM_CLOTHING:
                msg = "$p is corroded into scrap.";
                break;
            case ITEM_STAFF:
            case ITEM_WAND:
                chance -= 10;
                msg = "$p corrodes and breaks.";
                break;
            case ITEM_SCROLL:
                chance += 10;
                msg = "$p is burned into waste.";
                break;
        }

        chance = URANGE( 5, chance, 95 );

        if ( number_percent(  ) > chance )
            return;

        if ( obj->carried_by != NULL )
            act( msg, obj->carried_by, obj, NULL, TO_ALL );
        else if ( obj->in_room != NULL && obj->in_room->people != NULL )
            act( msg, obj->in_room->people, obj, NULL, TO_ALL );

        if ( obj->item_type == ITEM_ARMOR )            /* etch it */
        {
            AFFECT_DATA            *paf;
            bool                    af_found = false;
            int                     i;

            affect_enchant( obj );

            for ( paf = obj->affected; paf != NULL; paf = paf->next )
            {
                if ( paf->location == APPLY_AC )
                {
                    af_found = true;
                    paf->type = -1;
                    paf->modifier += 1;
                    paf->level = UMAX( paf->level, level );
                    break;
                }
            }

            if ( !af_found )
                /*
                 * needs a new affect 
                 */
            {
                paf = new_affect(  );

                paf->type = -1;
                paf->level = level;
                paf->duration = -1;
                paf->location = APPLY_AC;
                paf->modifier = 1;
                paf->bitvector = 0;
                paf->next = obj->affected;
                obj->affected = paf;
            }

            if ( obj->carried_by != NULL && obj->wear_loc != WEAR_NONE )
                for ( i = 0; i < 4; i++ )
                    obj->carried_by->armor[i] += 1;
            return;
        }

        /*
         * get rid of the object 
         */
        if ( obj->contains )                           /* dump contents */
        {
            for ( t_obj = obj->contains; t_obj != NULL; t_obj = n_obj )
            {
                n_obj = t_obj->next_content;
                obj_from_obj( t_obj );
                if ( obj->in_room != NULL )
                    obj_to_room( t_obj, obj->in_room );
                else if ( obj->carried_by != NULL )
                    obj_to_room( t_obj, obj->carried_by->in_room );
                else
                {
                    extract_obj( t_obj );
                    continue;
                }

                acid_effect( t_obj, level / 2, dam / 2, TARGET_OBJ );
            }
        }

        extract_obj( obj );
        return;
    }
}

void cold_effect( void *vo, int level, int dam, int target )
{
    if ( target == TARGET_ROOM )                       /* nail objects on the floor */
    {
        ROOM_INDEX_DATA        *room = ( ROOM_INDEX_DATA * ) vo;
        OBJ_DATA               *obj = NULL;
        OBJ_DATA               *obj_next = NULL;

        for ( obj = room->contents; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;
            cold_effect( obj, level, dam, TARGET_OBJ );
        }
        return;
    }

    if ( target == TARGET_CHAR )                       /* whack a character */
    {
        CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
        OBJ_DATA               *obj,
                               *obj_next;

        /*
         * chill touch effect 
         */
        if ( !saves_spell( level / 4 + dam / 20, victim, DAM_COLD ) )
        {
            AFFECT_DATA             af;

            act( "$n turns blue and shivers.", victim, NULL, NULL, TO_ROOM );
            act( "A chill sinks deep into your bones.", victim, NULL, NULL, TO_CHAR );
            af.where = TO_AFFECTS;
            af.type = skill_lookup( "chill touch" );
            af.level = level;
            af.duration = 6;
            af.location = APPLY_STR;
            af.modifier = -1;
            af.bitvector = 0;
            affect_join( victim, &af );
        }

        /*
         * hunger! (warmth sucked out 
         */
        if ( !IS_NPC( victim ) )
            gain_condition( victim, COND_HUNGER, dam / 20 );

        /*
         * let's toast some gear 
         */
        for ( obj = victim->carrying; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;
            cold_effect( obj, level, dam, TARGET_OBJ );
        }
        return;
    }

    if ( target == TARGET_OBJ )                        /* toast an object */
    {
        OBJ_DATA               *obj = ( OBJ_DATA * ) vo;
        int                     chance;
        const char             *msg;

        if ( IS_OBJ_STAT( obj, ITEM_BURN_PROOF )
             || IS_OBJ_STAT( obj, ITEM_NOPURGE ) || number_range( 0, 4 ) == 0 )
            return;

        chance = level / 4 + dam / 10;

        if ( chance > 25 )
            chance = ( chance - 25 ) / 2 + 25;
        if ( chance > 50 )
            chance = ( chance - 50 ) / 2 + 50;

        if ( IS_OBJ_STAT( obj, ITEM_BLESS ) )
            chance -= 5;

        chance -= obj->level * 2;

        switch ( obj->item_type )
        {
            default:
                return;
            case ITEM_POTION:
                msg = "$p freezes and shatters!";
                chance += 25;
                break;
            case ITEM_DRINK_CON:
                msg = "$p freezes and shatters!";
                chance += 5;
                break;
        }

        chance = URANGE( 5, chance, 95 );

        if ( number_percent(  ) > chance )
            return;

        if ( obj->carried_by != NULL )
            act( msg, obj->carried_by, obj, NULL, TO_ALL );
        else if ( obj->in_room != NULL && obj->in_room->people != NULL )
            act( msg, obj->in_room->people, obj, NULL, TO_ALL );

        extract_obj( obj );
        return;
    }
}

void fire_effect( void *vo, int level, int dam, int target )
{
    if ( target == TARGET_ROOM )                       /* nail objects on the floor */
    {
        ROOM_INDEX_DATA        *room = ( ROOM_INDEX_DATA * ) vo;
        OBJ_DATA               *obj = NULL;
        OBJ_DATA               *obj_next = NULL;

        for ( obj = room->contents; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;
            fire_effect( obj, level, dam, TARGET_OBJ );
        }
        return;
    }

    if ( target == TARGET_CHAR )                       /* do the effect on a victim */
    {
        CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
        OBJ_DATA               *obj = NULL;
        OBJ_DATA               *obj_next = NULL;

        /*
         * chance of blindness 
         */
        if ( !IS_AFFECTED( victim, AFF_BLIND )
             && !saves_spell( level / 4 + dam / 20, victim, DAM_FIRE ) )
        {
            AFFECT_DATA             af;

            act( "$n is blinded by smoke!", victim, NULL, NULL, TO_ROOM );
            act( "Your eyes tear up from smoke...you can't see a thing!",
                 victim, NULL, NULL, TO_CHAR );

            af.where = TO_AFFECTS;
            af.type = skill_lookup( "fire breath" );
            af.level = level;
            af.duration = number_range( 0, level / 10 );
            af.location = APPLY_HITROLL;
            af.modifier = -4;
            af.bitvector = AFF_BLIND;

            affect_to_char( victim, &af );
        }

        /*
         * getting thirsty 
         */
        if ( !IS_NPC( victim ) )
            gain_condition( victim, COND_THIRST, dam / 20 );

        /*
         * let's toast some gear! 
         */
        for ( obj = victim->carrying; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;

            fire_effect( obj, level, dam, TARGET_OBJ );
        }
        return;
    }

    if ( target == TARGET_OBJ )                        /* toast an object */
    {
        OBJ_DATA               *obj = ( OBJ_DATA * ) vo;
        OBJ_DATA               *t_obj = NULL;
        OBJ_DATA               *n_obj = NULL;
        int                     chance = 0;
        const char             *msg = NULL;

        if ( IS_OBJ_STAT( obj, ITEM_BURN_PROOF )
             || IS_OBJ_STAT( obj, ITEM_NOPURGE ) || number_range( 0, 4 ) == 0 )
            return;

        chance = level / 4 + dam / 10;

        if ( chance > 25 )
            chance = ( chance - 25 ) / 2 + 25;
        if ( chance > 50 )
            chance = ( chance - 50 ) / 2 + 50;

        if ( IS_OBJ_STAT( obj, ITEM_BLESS ) )
            chance -= 5;
        chance -= obj->level * 2;

        switch ( obj->item_type )
        {
            default:
                return;
            case ITEM_CONTAINER:
                msg = "$p ignites and burns!";
                break;
            case ITEM_POTION:
                chance += 25;
                msg = "$p bubbles and boils!";
                break;
            case ITEM_SCROLL:
                chance += 50;
                msg = "$p crackles and burns!";
                break;
            case ITEM_STAFF:
                chance += 10;
                msg = "$p smokes and chars!";
                break;
            case ITEM_WAND:
                msg = "$p sparks and sputters!";
                break;
            case ITEM_FOOD:
                msg = "$p blackens and crisps!";
                break;
            case ITEM_PILL:
                msg = "$p melts and drips!";
                break;
        }

        chance = URANGE( 5, chance, 95 );

        if ( number_percent(  ) > chance )
            return;

        if ( obj->carried_by != NULL )
            act( msg, obj->carried_by, obj, NULL, TO_ALL );
        else if ( obj->in_room != NULL && obj->in_room->people != NULL )
            act( msg, obj->in_room->people, obj, NULL, TO_ALL );

        if ( obj->contains )
        {
            /*
             * dump the contents 
             */

            for ( t_obj = obj->contains; t_obj != NULL; t_obj = n_obj )
            {
                n_obj = t_obj->next_content;
                obj_from_obj( t_obj );
                if ( obj->in_room != NULL )
                    obj_to_room( t_obj, obj->in_room );
                else if ( obj->carried_by != NULL )
                    obj_to_room( t_obj, obj->carried_by->in_room );
                else
                {
                    extract_obj( t_obj );
                    continue;
                }
                fire_effect( t_obj, level / 2, dam / 2, TARGET_OBJ );
            }
        }

        extract_obj( obj );
        return;
    }
}

void poison_effect( void *vo, int level, int dam, int target )
{
    if ( target == TARGET_ROOM )                       /* nail objects on the floor */
    {
        ROOM_INDEX_DATA        *room = ( ROOM_INDEX_DATA * ) vo;
        OBJ_DATA               *obj = NULL;
        OBJ_DATA               *obj_next = NULL;

        for ( obj = room->contents; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;
            poison_effect( obj, level, dam, TARGET_OBJ );
        }
        return;
    }

    if ( target == TARGET_CHAR )                       /* do the effect on a victim */
    {
        CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
        OBJ_DATA               *obj = NULL;
        OBJ_DATA               *obj_next = NULL;

        /*
         * chance of poisoning 
         */
        if ( !saves_spell( level / 4 + dam / 20, victim, DAM_POISON ) )
        {
            AFFECT_DATA             af;

            ch_printf( victim, "You feel poison coursing through your veins.\r\n" );
            act( "$n looks very ill.", victim, NULL, NULL, TO_ROOM );

            af.where = TO_AFFECTS;
            af.type = skill_lookup( "poison" );
            af.level = level;
            af.duration = level / 2;
            af.location = APPLY_STR;
            af.modifier = -1;
            af.bitvector = AFF_POISON;
            affect_join( victim, &af );
        }

        /*
         * equipment 
         */
        for ( obj = victim->carrying; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;
            poison_effect( obj, level, dam, TARGET_OBJ );
        }
        return;
    }

    if ( target == TARGET_OBJ )                        /* do some poisoning */
    {
        OBJ_DATA               *obj = ( OBJ_DATA * ) vo;
        int                     chance = 0;

        if ( IS_OBJ_STAT( obj, ITEM_BURN_PROOF )
             || IS_OBJ_STAT( obj, ITEM_BLESS ) || number_range( 0, 4 ) == 0 )
            return;

        chance = level / 4 + dam / 10;
        if ( chance > 25 )
            chance = ( chance - 25 ) / 2 + 25;
        if ( chance > 50 )
            chance = ( chance - 50 ) / 2 + 50;

        chance -= obj->level * 2;

        switch ( obj->item_type )
        {
            default:
                return;
            case ITEM_FOOD:
                break;
            case ITEM_DRINK_CON:
                if ( obj->value[0] == obj->value[1] )
                    return;
                break;
        }

        chance = URANGE( 5, chance, 95 );

        if ( number_percent(  ) > chance )
            return;

        obj->value[3] = 1;
        return;
    }
}

void shock_effect( void *vo, int level, int dam, int target )
{
    if ( target == TARGET_ROOM )
    {
        ROOM_INDEX_DATA        *room = ( ROOM_INDEX_DATA * ) vo;
        OBJ_DATA               *obj = NULL;
        OBJ_DATA               *obj_next = NULL;

        for ( obj = room->contents; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;
            shock_effect( obj, level, dam, TARGET_OBJ );
        }
        return;
    }

    if ( target == TARGET_CHAR )
    {
        CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
        OBJ_DATA               *obj,
                               *obj_next;

        /*
         * daze and confused? 
         */
        if ( !saves_spell( level / 4 + dam / 20, victim, DAM_LIGHTNING ) )
        {
            ch_printf( victim, "Your muscles stop responding.\r\n" );
            DAZE_STATE( victim, UMAX( 12, level / 4 + dam / 20 ) );
        }

        /*
         * toast some gear 
         */
        for ( obj = victim->carrying; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;
            shock_effect( obj, level, dam, TARGET_OBJ );
        }
        return;
    }

    if ( target == TARGET_OBJ )
    {
        OBJ_DATA               *obj = ( OBJ_DATA * ) vo;
        int                     chance = 0;
        const char             *msg = NULL;

        if ( IS_OBJ_STAT( obj, ITEM_BURN_PROOF )
             || IS_OBJ_STAT( obj, ITEM_NOPURGE ) || number_range( 0, 4 ) == 0 )
            return;

        chance = level / 4 + dam / 10;

        if ( chance > 25 )
            chance = ( chance - 25 ) / 2 + 25;
        if ( chance > 50 )
            chance = ( chance - 50 ) / 2 + 50;

        if ( IS_OBJ_STAT( obj, ITEM_BLESS ) )
            chance -= 5;

        chance -= obj->level * 2;

        switch ( obj->item_type )
        {
            default:
                return;
            case ITEM_WAND:
            case ITEM_STAFF:
                chance += 10;
                msg = "$p overloads and explodes!";
                break;
            case ITEM_JEWELRY:
                chance -= 10;
                msg = "$p is fused into a worthless lump.";
        }

        chance = URANGE( 5, chance, 95 );

        if ( number_percent(  ) > chance )
            return;

        if ( obj->carried_by != NULL )
            act( msg, obj->carried_by, obj, NULL, TO_ALL );
        else if ( obj->in_room != NULL && obj->in_room->people != NULL )
            act( msg, obj->in_room->people, obj, NULL, TO_ALL );

        extract_obj( obj );
        return;
    }
}
