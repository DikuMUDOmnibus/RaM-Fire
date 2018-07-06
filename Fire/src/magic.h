/*
 * RAM $Id: magic.h 70 2009-01-11 18:47:35Z quixadhal $
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

extern int              top_skill;
extern const char      *target_name;

/* affect recycling */
extern AFFECT_DATA     *affect_free;
AFFECT_DATA            *new_affect( void );
void                    free_affect( AFFECT_DATA *af );

int                     qcmp_skill( const void *left, const void *right );
void                    sort_skill_table( void );
int                     bsearch_skill( const char *name, int first, int top );
int                     bsearch_skill_by_prefix( const char *name, int first, int top );

int                     skill_lookup( const char *name );
int                     find_spell( CHAR_DATA *ch, const char *name );
int                     slot_lookup( int slot );
void                    say_spell( CHAR_DATA *ch, int sn );
bool                    saves_spell( int level, CHAR_DATA *victim, int dam_type );
bool                    saves_dispel( int dis_level, int spell_level, int duration );
bool                    check_dispel( int dis_level, CHAR_DATA *victim, int sn );
int                     mana_cost( CHAR_DATA *ch, int min_mana, int level );
void                    obj_cast_spell( int sn, int level, CHAR_DATA *ch,
                                        CHAR_DATA *victim, OBJ_DATA *obj );

/*
 * Spell functions.
 * Defined in magic.c.
 */
void                    spell_null( int sn, int level, CHAR_DATA *ch, void *vo,
                                    int target );
void                    spell_acid_blast( int sn, int level, CHAR_DATA *ch, void *vo,
                                          int target );
void                    spell_armor( int sn, int level, CHAR_DATA *ch, void *vo,
                                     int target );
void                    spell_bless( int sn, int level, CHAR_DATA *ch, void *vo,
                                     int target );
void                    spell_blindness( int sn, int level, CHAR_DATA *ch, void *vo,
                                         int target );
void                    spell_burning_hands( int sn, int level, CHAR_DATA *ch, void *vo,
                                             int target );
void                    spell_call_lightning( int sn, int level, CHAR_DATA *ch, void *vo,
                                              int target );
void                    spell_calm( int sn, int level, CHAR_DATA *ch, void *vo,
                                    int target );
void                    spell_cancellation( int sn, int level, CHAR_DATA *ch, void *vo,
                                            int target );
void                    spell_cause_light( int sn, int level, CHAR_DATA *ch, void *vo,
                                           int target );
void                    spell_cause_critical( int sn, int level, CHAR_DATA *ch, void *vo,
                                              int target );
void                    spell_cause_serious( int sn, int level, CHAR_DATA *ch, void *vo,
                                             int target );
void                    spell_chain_lightning( int sn, int level, CHAR_DATA *ch, void *vo,
                                               int target );
void                    spell_change_sex( int sn, int level, CHAR_DATA *ch, void *vo,
                                          int target );
void                    spell_charm_person( int sn, int level, CHAR_DATA *ch, void *vo,
                                            int target );
void                    spell_chill_touch( int sn, int level, CHAR_DATA *ch, void *vo,
                                           int target );
void                    spell_colour_spray( int sn, int level, CHAR_DATA *ch, void *vo,
                                            int target );
void                    spell_continual_light( int sn, int level, CHAR_DATA *ch, void *vo,
                                               int target );
void                    spell_control_weather( int sn, int level, CHAR_DATA *ch, void *vo,
                                               int target );
void                    spell_create_food( int sn, int level, CHAR_DATA *ch, void *vo,
                                           int target );
void                    spell_create_rose( int sn, int level, CHAR_DATA *ch, void *vo,
                                           int target );
void                    spell_create_spring( int sn, int level, CHAR_DATA *ch, void *vo,
                                             int target );
void                    spell_create_water( int sn, int level, CHAR_DATA *ch, void *vo,
                                            int target );
void                    spell_cure_blindness( int sn, int level, CHAR_DATA *ch, void *vo,
                                              int target );
void                    spell_cure_critical( int sn, int level, CHAR_DATA *ch, void *vo,
                                             int target );
void                    spell_cure_disease( int sn, int level, CHAR_DATA *ch, void *vo,
                                            int target );
void                    spell_cure_light( int sn, int level, CHAR_DATA *ch, void *vo,
                                          int target );
void                    spell_cure_poison( int sn, int level, CHAR_DATA *ch, void *vo,
                                           int target );
void                    spell_cure_serious( int sn, int level, CHAR_DATA *ch, void *vo,
                                            int target );
void                    spell_curse( int sn, int level, CHAR_DATA *ch, void *vo,
                                     int target );
void                    spell_demonfire( int sn, int level, CHAR_DATA *ch, void *vo,
                                         int target );
void                    spell_detect_evil( int sn, int level, CHAR_DATA *ch, void *vo,
                                           int target );
void                    spell_detect_good( int sn, int level, CHAR_DATA *ch, void *vo,
                                           int target );
void                    spell_detect_hidden( int sn, int level, CHAR_DATA *ch, void *vo,
                                             int target );
void                    spell_detect_invisibility( int sn, int level, CHAR_DATA *ch,
                                                   void *vo, int target );
void                    spell_detect_magic( int sn, int level, CHAR_DATA *ch, void *vo,
                                            int target );
void                    spell_detect_poison( int sn, int level, CHAR_DATA *ch, void *vo,
                                             int target );
void                    spell_dispel_evil( int sn, int level, CHAR_DATA *ch, void *vo,
                                           int target );
void                    spell_dispel_good( int sn, int level, CHAR_DATA *ch, void *vo,
                                           int target );
void                    spell_dispel_magic( int sn, int level, CHAR_DATA *ch, void *vo,
                                            int target );
void                    spell_earthquake( int sn, int level, CHAR_DATA *ch, void *vo,
                                          int target );
void                    spell_enchant_armor( int sn, int level, CHAR_DATA *ch, void *vo,
                                             int target );
void                    spell_enchant_weapon( int sn, int level, CHAR_DATA *ch, void *vo,
                                              int target );
void                    spell_energy_drain( int sn, int level, CHAR_DATA *ch, void *vo,
                                            int target );
void                    spell_fireball( int sn, int level, CHAR_DATA *ch, void *vo,
                                        int target );
void                    spell_fireproof( int sn, int level, CHAR_DATA *ch, void *vo,
                                         int target );
void                    spell_flamestrike( int sn, int level, CHAR_DATA *ch, void *vo,
                                           int target );
void                    spell_faerie_fire( int sn, int level, CHAR_DATA *ch, void *vo,
                                           int target );
void                    spell_faerie_fog( int sn, int level, CHAR_DATA *ch, void *vo,
                                          int target );
void                    spell_floating_disc( int sn, int level, CHAR_DATA *ch, void *vo,
                                             int target );
void                    spell_fly( int sn, int level, CHAR_DATA *ch, void *vo,
                                   int target );
void                    spell_frenzy( int sn, int level, CHAR_DATA *ch, void *vo,
                                      int target );
void                    spell_gate( int sn, int level, CHAR_DATA *ch, void *vo,
                                    int target );
void                    spell_giant_strength( int sn, int level, CHAR_DATA *ch, void *vo,
                                              int target );
void                    spell_harm( int sn, int level, CHAR_DATA *ch, void *vo,
                                    int target );
void                    spell_haste( int sn, int level, CHAR_DATA *ch, void *vo,
                                     int target );
void                    spell_heal( int sn, int level, CHAR_DATA *ch, void *vo,
                                    int target );
void                    spell_heat_metal( int sn, int level, CHAR_DATA *ch, void *vo,
                                          int target );
void                    spell_holy_word( int sn, int level, CHAR_DATA *ch, void *vo,
                                         int target );
void                    spell_identify( int sn, int level, CHAR_DATA *ch, void *vo,
                                        int target );
void                    spell_infravision( int sn, int level, CHAR_DATA *ch, void *vo,
                                           int target );
void                    spell_invisibility( int sn, int level, CHAR_DATA *ch, void *vo,
                                            int target );
void                    spell_know_alignment( int sn, int level, CHAR_DATA *ch, void *vo,
                                              int target );
void                    spell_lightning_bolt( int sn, int level, CHAR_DATA *ch, void *vo,
                                              int target );
void                    spell_locate_object( int sn, int level, CHAR_DATA *ch, void *vo,
                                             int target );
void                    spell_magic_missile( int sn, int level, CHAR_DATA *ch, void *vo,
                                             int target );
void                    spell_mass_healing( int sn, int level, CHAR_DATA *ch, void *vo,
                                            int target );
void                    spell_mass_invisibility( int sn, int level, CHAR_DATA *ch,
                                                 void *vo, int target );
void                    spell_pass_door( int sn, int level, CHAR_DATA *ch, void *vo,
                                         int target );
void                    spell_plague( int sn, int level, CHAR_DATA *ch, void *vo,
                                      int target );
void                    spell_poison( int sn, int level, CHAR_DATA *ch, void *vo,
                                      int target );
void                    spell_protection_evil( int sn, int level, CHAR_DATA *ch, void *vo,
                                               int target );
void                    spell_protection_good( int sn, int level, CHAR_DATA *ch, void *vo,
                                               int target );
void                    spell_ray_of_truth( int sn, int level, CHAR_DATA *ch, void *vo,
                                            int target );
void                    spell_recharge( int sn, int level, CHAR_DATA *ch, void *vo,
                                        int target );
void                    spell_refresh( int sn, int level, CHAR_DATA *ch, void *vo,
                                       int target );
void                    spell_remove_curse( int sn, int level, CHAR_DATA *ch, void *vo,
                                            int target );
void                    spell_sanctuary( int sn, int level, CHAR_DATA *ch, void *vo,
                                         int target );
void                    spell_shield( int sn, int level, CHAR_DATA *ch, void *vo,
                                      int target );
void                    spell_shocking_grasp( int sn, int level, CHAR_DATA *ch, void *vo,
                                              int target );
void                    spell_sleep( int sn, int level, CHAR_DATA *ch, void *vo,
                                     int target );
void                    spell_slow( int sn, int level, CHAR_DATA *ch, void *vo,
                                    int target );
void                    spell_stone_skin( int sn, int level, CHAR_DATA *ch, void *vo,
                                          int target );
void                    spell_summon( int sn, int level, CHAR_DATA *ch, void *vo,
                                      int target );
void                    spell_teleport( int sn, int level, CHAR_DATA *ch, void *vo,
                                        int target );
void                    spell_ventriloquate( int sn, int level, CHAR_DATA *ch, void *vo,
                                             int target );
void                    spell_weaken( int sn, int level, CHAR_DATA *ch, void *vo,
                                      int target );
void                    spell_word_of_recall( int sn, int level, CHAR_DATA *ch, void *vo,
                                              int target );
void                    spell_acid_breath( int sn, int level, CHAR_DATA *ch, void *vo,
                                           int target );
void                    spell_fire_breath( int sn, int level, CHAR_DATA *ch, void *vo,
                                           int target );
void                    spell_frost_breath( int sn, int level, CHAR_DATA *ch, void *vo,
                                            int target );
void                    spell_gas_breath( int sn, int level, CHAR_DATA *ch, void *vo,
                                          int target );
void                    spell_lightning_breath( int sn, int level, CHAR_DATA *ch,
                                                void *vo, int target );
void                    spell_general_purpose( int sn, int level, CHAR_DATA *ch, void *vo,
                                               int target );
void                    spell_high_explosive( int sn, int level, CHAR_DATA *ch, void *vo,
                                              int target );
void                    spell_farsight( int sn, int level, CHAR_DATA *ch, void *vo,
                                        int target );
void                    spell_portal( int sn, int level, CHAR_DATA *ch, void *vo,
                                      int target );
void                    spell_nexus( int sn, int level, CHAR_DATA *ch, void *vo,
                                     int target );

void                    acid_effect( void *vo, int level, int dam, int target );
void                    cold_effect( void *vo, int level, int dam, int target );
void                    fire_effect( void *vo, int level, int dam, int target );
void                    poison_effect( void *vo, int level, int dam, int target );
void                    shock_effect( void *vo, int level, int dam, int target );
