/*
 * RAM $Id: tables.c 70 2009-01-11 18:47:35Z quixadhal $
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

/***************************************************************************
 *  This code was written by Jason Dinkel and inspired by Russ Taylor,     *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 *  The code below uses a table lookup system that is based on suggestions *
 *  from Russ Taylor.  There are many routines in handler.c that would     *
 *  benefit with the use of tables.  You may consider simplifying your     *
 *  code base by implementing a system like below with such functions.     *
 *                                                                         *
 *                                                   -Jason Dinkel         *
 ***************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "merc.h"
#include "strings.h"
#include "db.h"
#include "magic.h"
#include "interp.h"
#include "tables.h"

/* for clans */
const struct clan_type  clan_table[MAX_CLAN] = {
/* name, who entry, death-transfer room, independent */
/* independent should be false if is a real clan */
    {"", "", ROOM_VNUM_ALTAR, true},
    {"loner", "[ Loner ] ", ROOM_VNUM_ALTAR, true},
    {"rom", "[  ROM  ] ", ROOM_VNUM_ALTAR, false}
};

/* for position */
const struct position_type position_table[] = {
    {"dead", "dead"},
    {"mortally wounded", "mort"},
    {"incapacitated", "incap"},
    {"stunned", "stun"},
    {"sleeping", "sleep"},
    {"resting", "rest"},
    {"sitting", "sit"},
    {"fighting", "fight"},
    {"standing", "stand"},
    {NULL, NULL}
};

/* for sex */
const struct sex_type   sex_table[] = {
    {"none"},
    {"male"},
    {"female"},
    {"either"},
    {NULL}
};

/* for sizes */
const struct size_type  size_table[] = {
    {"tiny"},
    {"small"},
    {"medium"},
    {"large"},
    {"huge",},
    {"giant"},
    {NULL}
};

/* various flag tables */
const struct flag_type  act_flags[] = {
    {"npc", A, false},
    {"sentinel", B, true},
    {"scavenger", C, true},
    {"aggressive", F, true},
    {"stay_area", G, true},
    {"wimpy", H, true},
    {"pet", I, true},
    {"train", J, true},
    {"practice", K, true},
    {"undead", O, true},
    {"cleric", Q, true},
    {"mage", R, true},
    {"thief", S, true},
    {"warrior", T, true},
    {"noalign", U, true},
    {"nopurge", V, true},
    {"outdoors", W, true},
    {"indoors", Y, true},
    {"healer", aa, true},
    {"gain", bb, true},
    {"update_always", cc, true},
    {"changer", dd, true},
    {NULL, 0, false}
};

const struct flag_type  plr_flags[] = {
    {"npc", A, false},
    {"autoassist", C, false},
    {"autoexit", D, false},
    {"autoloot", E, false},
    {"autosac", F, false},
    {"autogold", G, false},
    {"autosplit", H, false},
    {"holylight", N, false},
    {"can_loot", P, false},
    {"nosummon", Q, false},
    {"nofollow", R, false},
    {"permit", U, true},
    {"log", W, false},
    {"deny", X, false},
    {"freeze", Y, false},
    {"thief", Z, false},
    {"killer", aa, false},
    {NULL, 0, 0}
};

const struct flag_type  affect_flags[] = {
    {"blind", A, true},
    {"invisible", B, true},
    {"detect_evil", C, true},
    {"detect_invisibility", D, true},
    {"detect_magic", E, true},
    {"detect_hidden", F, true},
    {"detect_good", G, true},
    {"sanctuary", H, true},
    {"faerie_fire", I, true},
    {"infrared", J, true},
    {"curse", K, true},
    {"poison", M, true},
    {"protect_evil", N, true},
    {"protect_good", O, true},
    {"sneak", P, true},
    {"hide", Q, true},
    {"sleep", R, true},
    {"charm", S, true},
    {"flying", T, true},
    {"pass_door", U, true},
    {"haste", V, true},
    {"calm", W, true},
    {"plague", X, true},
    {"weaken", Y, true},
    {"dark_vision", Z, true},
    {"berserk", aa, true},
    {"swim", bb, true},
    {"regeneration", cc, true},
    {"slow", dd, true},
    {NULL, 0, 0}
};

const struct flag_type  off_flags[] = {
    {"area_attack", A, true},
    {"backstab", B, true},
    {"bash", C, true},
    {"berserk", D, true},
    {"disarm", E, true},
    {"dodge", F, true},
    {"fade", G, true},
    {"fast", H, true},
    {"kick", I, true},
    {"dirt_kick", J, true},
    {"parry", K, true},
    {"rescue", L, true},
    {"tail", M, true},
    {"trip", N, true},
    {"crush", O, true},
    {"assist_all", P, true},
    {"assist_align", Q, true},
    {"assist_race", R, true},
    {"assist_players", S, true},
    {"assist_guard", T, true},
    {"assist_vnum", U, true},
    {NULL, 0, 0}
};

const struct flag_type  imm_flags[] = {
    {"summon", A, true},
    {"charm", B, true},
    {"magic", C, true},
    {"weapon", D, true},
    {"bash", E, true},
    {"pierce", F, true},
    {"slash", G, true},
    {"fire", H, true},
    {"cold", I, true},
    {"lightning", J, true},
    {"acid", K, true},
    {"poison", L, true},
    {"negative", M, true},
    {"holy", N, true},
    {"energy", O, true},
    {"mental", P, true},
    {"disease", Q, true},
    {"drowning", R, true},
    {"light", S, true},
    {"sound", T, true},
    {"wood", X, true},
    {"silver", Y, true},
    {"iron", Z, true},
    {NULL, 0, 0}
};

const struct flag_type  form_flags[] = {
    {"edible", FORM_EDIBLE, true},
    {"poison", FORM_POISON, true},
    {"magical", FORM_MAGICAL, true},
    {"instant_decay", FORM_INSTANT_DECAY, true},
    {"other", FORM_OTHER, true},
    {"animal", FORM_ANIMAL, true},
    {"sentient", FORM_SENTIENT, true},
    {"undead", FORM_UNDEAD, true},
    {"construct", FORM_CONSTRUCT, true},
    {"mist", FORM_MIST, true},
    {"intangible", FORM_INTANGIBLE, true},
    {"biped", FORM_BIPED, true},
    {"centaur", FORM_CENTAUR, true},
    {"insect", FORM_INSECT, true},
    {"spider", FORM_SPIDER, true},
    {"crustacean", FORM_CRUSTACEAN, true},
    {"worm", FORM_WORM, true},
    {"blob", FORM_BLOB, true},
    {"mammal", FORM_MAMMAL, true},
    {"bird", FORM_BIRD, true},
    {"reptile", FORM_REPTILE, true},
    {"snake", FORM_SNAKE, true},
    {"dragon", FORM_DRAGON, true},
    {"amphibian", FORM_AMPHIBIAN, true},
    {"fish", FORM_FISH, true},
    {"cold_blood", FORM_COLD_BLOOD, true},
    {NULL, 0, 0}
};

const struct flag_type  part_flags[] = {
    {"head", PART_HEAD, true},
    {"arms", PART_ARMS, true},
    {"legs", PART_LEGS, true},
    {"heart", PART_HEART, true},
    {"brains", PART_BRAINS, true},
    {"guts", PART_GUTS, true},
    {"hands", PART_HANDS, true},
    {"feet", PART_FEET, true},
    {"fingers", PART_FINGERS, true},
    {"ear", PART_EAR, true},
    {"eye", PART_EYE, true},
    {"long_tongue", PART_LONG_TONGUE, true},
    {"eyestalks", PART_EYESTALKS, true},
    {"tentacles", PART_TENTACLES, true},
    {"fins", PART_FINS, true},
    {"wings", PART_WINGS, true},
    {"tail", PART_TAIL, true},
    {"claws", PART_CLAWS, true},
    {"fangs", PART_FANGS, true},
    {"horns", PART_HORNS, true},
    {"scales", PART_SCALES, true},
    {"tusks", PART_TUSKS, true},
    {NULL, 0, 0}
};

const struct flag_type  comm_flags[] = {
    {"quiet", COMM_QUIET, true},
    {"deaf", COMM_DEAF, true},
    {"nowiz", COMM_NOWIZ, true},
    {"noclangossip", COMM_NOAUCTION, true},
    {"nogossip", COMM_NOGOSSIP, true},
    {"noquestion", COMM_NOQUESTION, true},
/*  {"nomusic", COMM_NOMUSIC, true}, Legacy */
    {"noclan", COMM_NOCLAN, true},
    {"noquote", COMM_NOQUOTE, true},
    {"shoutsoff", COMM_SHOUTSOFF, true},
    {"compact", COMM_COMPACT, true},
    {"brief", COMM_BRIEF, true},
    {"prompt", COMM_PROMPT, true},
    {"combine", COMM_COMBINE, true},
    {"telnet_ga", COMM_TELNET_GA, true},
    {"show_affects", COMM_SHOW_AFFECTS, true},
    {"nograts", COMM_NOGRATS, true},
    {"noemote", COMM_NOEMOTE, false},
    {"noshout", COMM_NOSHOUT, false},
    {"notell", COMM_NOTELL, false},
    {"nochannels", COMM_NOCHANNELS, false},
    {"snoop_proof", COMM_SNOOP_PROOF, false},
    {"afk", COMM_AFK, true},
    {NULL, 0, 0}
};

/* item type list */
const struct item_type  item_table[] = {
    {ITEM_LIGHT, "light"},
    {ITEM_SCROLL, "scroll"},
    {ITEM_WAND, "wand"},
    {ITEM_STAFF, "staff"},
    {ITEM_WEAPON, "weapon"},
    {ITEM_TREASURE, "treasure"},
    {ITEM_ARMOR, "armor"},
    {ITEM_POTION, "potion"},
    {ITEM_CLOTHING, "clothing"},
    {ITEM_FURNITURE, "furniture"},
    {ITEM_TRASH, "trash"},
    {ITEM_CONTAINER, "container"},
    {ITEM_DRINK_CON, "drink"},
    {ITEM_KEY, "key"},
    {ITEM_FOOD, "food"},
    {ITEM_MONEY, "money"},
    {ITEM_BOAT, "boat"},
    {ITEM_CORPSE_NPC, "npc_corpse"},
    {ITEM_CORPSE_PC, "pc_corpse"},
    {ITEM_FOUNTAIN, "fountain"},
    {ITEM_PILL, "pill"},
    {ITEM_PROTECT, "protect"},
    {ITEM_MAP, "map"},
    {ITEM_PORTAL, "portal"},
    {ITEM_WARP_STONE, "warp_stone"},
    {ITEM_ROOM_KEY, "room_key"},
    {ITEM_GEM, "gem"},
    {ITEM_JEWELRY, "jewelry"},
/*  {ITEM_JUKEBOX, "jukebox"}, Legacy */
    {0, NULL}
};

/* weapon selection table */
const struct weapon_type weapon_table[] = {
    {"sword", OBJ_VNUM_SCHOOL_SWORD, WEAPON_SWORD, "sword"},
    {"mace", OBJ_VNUM_SCHOOL_MACE, WEAPON_MACE, "mace"},
    {"dagger", OBJ_VNUM_SCHOOL_DAGGER, WEAPON_DAGGER, "dagger"},
    {"axe", OBJ_VNUM_SCHOOL_AXE, WEAPON_AXE, "axe"},
    {"staff", OBJ_VNUM_SCHOOL_STAFF, WEAPON_SPEAR, "spear"},
    {"flail", OBJ_VNUM_SCHOOL_FLAIL, WEAPON_FLAIL, "flail"},
    {"whip", OBJ_VNUM_SCHOOL_WHIP, WEAPON_WHIP, "whip"},
    {"polearm", OBJ_VNUM_SCHOOL_POLEARM, WEAPON_POLEARM, "polearm"},
    {NULL, 0, 0, NULL}
};

/* wiznet table and prototype for future flag setting */
const struct wiznet_type wiznet_table[] = {
    {"on", WIZ_ON, IM},
    {"prefix", WIZ_PREFIX, IM},
    {"ticks", WIZ_TICKS, IM},
    {"logins", WIZ_LOGINS, IM},
    {"sites", WIZ_SITES, L4},
    {"links", WIZ_LINKS, L7},
    {"newbies", WIZ_NEWBIE, IM},
    {"spam", WIZ_SPAM, L5},
    {"deaths", WIZ_DEATHS, IM},
    {"resets", WIZ_RESETS, L4},
    {"mobdeaths", WIZ_MOBDEATHS, L4},
    {"flags", WIZ_FLAGS, L5},
    {"penalties", WIZ_PENALTIES, L5},
    {"saccing", WIZ_SACCING, L5},
    {"levels", WIZ_LEVELS, IM},
    {"load", WIZ_LOAD, L2},
    {"restore", WIZ_RESTORE, L2},
    {"snoops", WIZ_SNOOPS, L2},
    {"switches", WIZ_SWITCHES, L2},
    {"secure", WIZ_SECURE, L1},
    {NULL, 0, 0}
};

/* attack table  -- not very organized :( */
const struct attack_type attack_table[MAX_DAMAGE_MESSAGE] = {
    {"none", "hit", -1},                               /* 0 */
    {"slice", "slice", DAM_SLASH},
    {"stab", "stab", DAM_PIERCE},
    {"slash", "slash", DAM_SLASH},
    {"whip", "whip", DAM_SLASH},
    {"claw", "claw", DAM_SLASH},                       /* 5 */
    {"blast", "blast", DAM_BASH},
    {"pound", "pound", DAM_BASH},
    {"crush", "crush", DAM_BASH},
    {"grep", "grep", DAM_SLASH},
    {"bite", "bite", DAM_PIERCE},                      /* 10 */
    {"pierce", "pierce", DAM_PIERCE},
    {"suction", "suction", DAM_BASH},
    {"beating", "beating", DAM_BASH},
    {"digestion", "digestion", DAM_ACID},
    {"charge", "charge", DAM_BASH},                    /* 15 */
    {"slap", "slap", DAM_BASH},
    {"punch", "punch", DAM_BASH},
    {"wrath", "wrath", DAM_ENERGY},
    {"magic", "magic", DAM_ENERGY},
    {"divine", "divine power", DAM_HOLY},              /* 20 */
    {"cleave", "cleave", DAM_SLASH},
    {"scratch", "scratch", DAM_PIERCE},
    {"peck", "peck", DAM_PIERCE},
    {"peckb", "peck", DAM_BASH},
    {"chop", "chop", DAM_SLASH},                       /* 25 */
    {"sting", "sting", DAM_PIERCE},
    {"smash", "smash", DAM_BASH},
    {"shbite", "shocking bite", DAM_LIGHTNING},
    {"flbite", "flaming bite", DAM_FIRE},
    {"frbite", "freezing bite", DAM_COLD},             /* 30 */
    {"acbite", "acidic bite", DAM_ACID},
    {"chomp", "chomp", DAM_PIERCE},
    {"drain", "life drain", DAM_NEGATIVE},
    {"thrust", "thrust", DAM_PIERCE},
    {"slime", "slime", DAM_ACID},
    {"shock", "shock", DAM_LIGHTNING},
    {"thwack", "thwack", DAM_BASH},
    {"flame", "flame", DAM_FIRE},
    {"chill", "chill", DAM_COLD},
    {NULL, NULL, 0}
};

/* race table */
const struct race_type  race_table[] = {
/*
    {
        name,                pc_race?,
        act bits,        aff_by bits,        off bits,
        imm,                res,                vuln,
        form,                parts 
    },
*/
    {"unique", false, 0, 0, 0, 0, 0, 0, 0, 0},

    {
     "human", true,
     0, 0, 0,
     0, 0, 0,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K},

    {
     "elf", true,
     0, AFF_INFRARED, 0,
     0, RES_CHARM, VULN_IRON,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K},

    {
     "dwarf", true,
     0, AFF_INFRARED, 0,
     0, RES_POISON | RES_DISEASE, VULN_DROWNING,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K},

    {
     "giant", true,
     0, 0, 0,
     0, RES_FIRE | RES_COLD, VULN_MENTAL | VULN_LIGHTNING,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K},

    {
     "bat", false,
     0, AFF_FLYING | AFF_DARK_VISION, OFF_DODGE | OFF_FAST,
     0, 0, VULN_LIGHT,
     A | G | V, A | C | D | E | F | H | J | K | P},

    {
     "bear", false,
     0, 0, OFF_CRUSH | OFF_DISARM | OFF_BERSERK,
     0, RES_BASH | RES_COLD, 0,
     A | G | V, A | B | C | D | E | F | H | J | K | U | V},

    {
     "cat", false,
     0, AFF_DARK_VISION, OFF_FAST | OFF_DODGE,
     0, 0, 0,
     A | G | V, A | C | D | E | F | H | J | K | Q | U | V},

    {
     "centipede", false,
     0, AFF_DARK_VISION, 0,
     0, RES_PIERCE | RES_COLD, VULN_BASH,
     A | B | G | O, A | C | K},

    {
     "dog", false,
     0, 0, OFF_FAST,
     0, 0, 0,
     A | G | V, A | C | D | E | F | H | J | K | U | V},

    {
     "doll", false,
     0, 0, 0,
     IMM_COLD | IMM_POISON | IMM_HOLY | IMM_NEGATIVE | IMM_MENTAL | IMM_DISEASE
     | IMM_DROWNING, RES_BASH | RES_LIGHT,
     VULN_SLASH | VULN_FIRE | VULN_ACID | VULN_LIGHTNING | VULN_ENERGY,
     E | J | M | cc, A | B | C | G | H | K},

    {"dragon", false,
     0, AFF_INFRARED | AFF_FLYING, 0,
     0, RES_FIRE | RES_BASH | RES_CHARM,
     VULN_PIERCE | VULN_COLD,
     A | H | Z, A | C | D | E | F | G | H | I | J | K | P | Q | U | V | X},

    {
     "fido", false,
     0, 0, OFF_DODGE | ASSIST_RACE,
     0, 0, VULN_MAGIC,
     A | B | G | V, A | C | D | E | F | H | J | K | Q | V},

    {
     "fox", false,
     0, AFF_DARK_VISION, OFF_FAST | OFF_DODGE,
     0, 0, 0,
     A | G | V, A | C | D | E | F | H | J | K | Q | V},

    {
     "goblin", false,
     0, AFF_INFRARED, 0,
     0, RES_DISEASE, VULN_MAGIC,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K},

    {
     "hobgoblin", false,
     0, AFF_INFRARED, 0,
     0, RES_DISEASE | RES_POISON, 0,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K | Y},

    {
     "kobold", false,
     0, AFF_INFRARED, 0,
     0, RES_POISON, VULN_MAGIC,
     A | B | H | M | V, A | B | C | D | E | F | G | H | I | J | K | Q},

    {
     "lizard", false,
     0, 0, 0,
     0, RES_POISON, VULN_COLD,
     A | G | X | cc, A | C | D | E | F | H | K | Q | V},

    {
     "modron", false,
     0, AFF_INFRARED, ASSIST_RACE | ASSIST_ALIGN,
     IMM_CHARM | IMM_DISEASE | IMM_MENTAL | IMM_HOLY | IMM_NEGATIVE,
     RES_FIRE | RES_COLD | RES_ACID, 0,
     H, A | B | C | G | H | J | K},

    {
     "orc", false,
     0, AFF_INFRARED, 0,
     0, RES_DISEASE, VULN_LIGHT,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K},

    {
     "pig", false,
     0, 0, 0,
     0, 0, 0,
     A | G | V, A | C | D | E | F | H | J | K},

    {
     "rabbit", false,
     0, 0, OFF_DODGE | OFF_FAST,
     0, 0, 0,
     A | G | V, A | C | D | E | F | H | J | K},

    {
     "school monster", false,
     ACT_NOALIGN, 0, 0,
     IMM_CHARM | IMM_SUMMON, 0, VULN_MAGIC,
     A | M | V, A | B | C | D | E | F | H | J | K | Q | U},

    {
     "snake", false,
     0, 0, 0,
     0, RES_POISON, VULN_COLD,
     A | G | X | Y | cc, A | D | E | F | K | L | Q | V | X},

    {
     "song bird", false,
     0, AFF_FLYING, OFF_FAST | OFF_DODGE,
     0, 0, 0,
     A | G | W, A | C | D | E | F | H | K | P},

    {
     "troll", false,
     0, AFF_REGENERATION | AFF_INFRARED | AFF_DETECT_HIDDEN,
     OFF_BERSERK,
     0, RES_CHARM | RES_BASH, VULN_FIRE | VULN_ACID,
     A | B | H | M | V, A | B | C | D | E | F | G | H | I | J | K | U | V},

    {
     "water fowl", false,
     0, AFF_SWIM | AFF_FLYING, 0,
     0, RES_DROWNING, 0,
     A | G | W, A | C | D | E | F | H | K | P},

    {
     "wolf", false,
     0, AFF_DARK_VISION, OFF_FAST | OFF_DODGE,
     0, 0, 0,
     A | G | V, A | C | D | E | F | J | K | Q | V},

    {
     "wyvern", false,
     0, AFF_FLYING | AFF_DETECT_INVISIBILITY | AFF_DETECT_HIDDEN,
     OFF_BASH | OFF_FAST | OFF_DODGE,
     IMM_POISON, 0, VULN_LIGHT,
     A | B | G | Z, A | C | D | E | F | H | J | K | Q | V | X},

    {
     "unique", false,
     0, 0, 0,
     0, 0, 0,
     0, 0},

    {
     NULL, 0, 0, 0, 0, 0, 0}
};

const struct pc_race_type pc_race_table[] = {
    {"null race", "", 0, {100, 100, 100, 100},
     {""}, {13, 13, 13, 13, 13}, {18, 18, 18, 18, 18}, 0},

/*
    {
        "race name",         short name,         points,        { class multipliers },
        { bonus skills },
        { base stats },                { max stats },                size 
    },
*/
    {
     "human", "Human", 0, {100, 100, 100, 100},
     {""},
     {13, 13, 13, 13, 13}, {18, 18, 18, 18, 18}, SIZE_MEDIUM},

    {
     "elf", " Elf ", 5, {100, 125, 100, 120},
     {"sneak", "hide"},
     {12, 14, 13, 15, 11}, {16, 20, 18, 21, 15}, SIZE_SMALL},

    {
     "dwarf", "Dwarf", 8, {150, 100, 125, 100},
     {"berserk"},
     {14, 12, 14, 10, 15}, {20, 16, 19, 14, 21}, SIZE_MEDIUM},

    {
     "giant", "Giant", 6, {200, 150, 150, 105},
     {"bash", "fast healing"},
     {16, 11, 13, 11, 14}, {22, 15, 18, 15, 20}, SIZE_LARGE}
};

/*
 * Class table.
 */
const struct class_type class_table[MAX_CLASS] = {
    {
     "mage", "Mag", STAT_INT, OBJ_VNUM_SCHOOL_DAGGER,
     {3018, 9618}, 75, 20, 6, 6, 8, true,
     "mage basics", "mage default"},

    {
     "cleric", "Cle", STAT_WIS, OBJ_VNUM_SCHOOL_MACE,
     {3003, 9619}, 75, 20, 2, 7, 10, true,
     "cleric basics", "cleric default"},

    {
     "thief", "Thi", STAT_DEX, OBJ_VNUM_SCHOOL_DAGGER,
     {3028, 9639}, 75, 20, -4, 8, 13, false,
     "thief basics", "thief default"},

    {
     "warrior", "War", STAT_STR, OBJ_VNUM_SCHOOL_SWORD,
     {3022, 9633}, 75, 20, -10, 11, 15, false,
     "warrior basics", "warrior default"}
};

/*
 * Titles.
 */
const char             *title_table[MAX_CLASS][MAX_LEVEL + 1][2] = {
    {
     {"Man", "Woman"},

     {"Apprentice of Magic", "Apprentice of Magic"},
     {"Spell Student", "Spell Student"},
     {"Scholar of Magic", "Scholar of Magic"},
     {"Delver in Spells", "Delveress in Spells"},
     {"Medium of Magic", "Medium of Magic"},

     {"Scribe of Magic", "Scribess of Magic"},
     {"Seer", "Seeress"},
     {"Sage", "Sage"},
     {"Illusionist", "Illusionist"},
     {"Abjurer", "Abjuress"},

     {"Invoker", "Invoker"},
     {"Enchanter", "Enchantress"},
     {"Conjurer", "Conjuress"},
     {"Magician", "Witch"},
     {"Creator", "Creator"},

     {"Savant", "Savant"},
     {"Magus", "Craftess"},
     {"Wizard", "Wizard"},
     {"Warlock", "War Witch"},
     {"Sorcerer", "Sorceress"},

     {"Elder Sorcerer", "Elder Sorceress"},
     {"Grand Sorcerer", "Grand Sorceress"},
     {"Great Sorcerer", "Great Sorceress"},
     {"Golem Maker", "Golem Maker"},
     {"Greater Golem Maker", "Greater Golem Maker"},

     {"Maker of Stones", "Maker of Stones",},
     {"Maker of Potions", "Maker of Potions",},
     {"Maker of Scrolls", "Maker of Scrolls",},
     {"Maker of Wands", "Maker of Wands",},
     {"Maker of Staves", "Maker of Staves",},

     {"Demon Summoner", "Demon Summoner"},
     {"Greater Demon Summoner", "Greater Demon Summoner"},
     {"Dragon Charmer", "Dragon Charmer"},
     {"Greater Dragon Charmer", "Greater Dragon Charmer"},
     {"Master of all Magic", "Master of all Magic"},

     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},

     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},

     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},

     {"Mage Hero", "Mage Heroine"},
     {"Avatar of Magic", "Avatar of Magic"},
     {"Angel of Magic", "Angel of Magic"},
     {"Demigod of Magic", "Demigoddess of Magic"},
     {"Immortal of Magic", "Immortal of Magic"},
     {"God of Magic", "Goddess of Magic"},
     {"Deity of Magic", "Deity of Magic"},
     {"Supremity of Magic", "Supremity of Magic"},
     {"Creator", "Creator"},
     {"Implementor", "Implementress"}
     },

    {
     {"Man", "Woman"},

     {"Believer", "Believer"},
     {"Attendant", "Attendant"},
     {"Acolyte", "Acolyte"},
     {"Novice", "Novice"},
     {"Missionary", "Missionary"},

     {"Adept", "Adept"},
     {"Deacon", "Deaconess"},
     {"Vicar", "Vicaress"},
     {"Priest", "Priestess"},
     {"Minister", "Lady Minister"},

     {"Canon", "Canon"},
     {"Levite", "Levitess"},
     {"Curate", "Curess"},
     {"Monk", "Nun"},
     {"Healer", "Healess"},

     {"Chaplain", "Chaplain"},
     {"Expositor", "Expositress"},
     {"Bishop", "Bishop"},
     {"Arch Bishop", "Arch Lady of the Church"},
     {"Patriarch", "Matriarch"},

     {"Elder Patriarch", "Elder Matriarch"},
     {"Grand Patriarch", "Grand Matriarch"},
     {"Great Patriarch", "Great Matriarch"},
     {"Demon Killer", "Demon Killer"},
     {"Greater Demon Killer", "Greater Demon Killer"},

     {"Cardinal of the Sea", "Cardinal of the Sea"},
     {"Cardinal of the Earth", "Cardinal of the Earth"},
     {"Cardinal of the Air", "Cardinal of the Air"},
     {"Cardinal of the Ether", "Cardinal of the Ether"},
     {"Cardinal of the Heavens", "Cardinal of the Heavens"},

     {"Avatar of an Immortal", "Avatar of an Immortal"},
     {"Avatar of a Deity", "Avatar of a Deity"},
     {"Avatar of a Supremity", "Avatar of a Supremity"},
     {"Avatar of an Implementor", "Avatar of an Implementor"},
     {"Master of all Divinity", "Mistress of all Divinity"},

     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},

     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},

     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},

     {"Holy Hero", "Holy Heroine"},
     {"Holy Avatar", "Holy Avatar"},
     {"Angel", "Angel"},
     {"Demigod", "Demigoddess",},
     {"Immortal", "Immortal"},
     {"God", "Goddess"},
     {"Deity", "Deity"},
     {"Supreme Master", "Supreme Mistress"},
     {"Creator", "Creator"},
     {"Implementor", "Implementress"}
     },

    {
     {"Man", "Woman"},

     {"Pilferer", "Pilferess"},
     {"Footpad", "Footpad"},
     {"Filcher", "Filcheress"},
     {"Pick-Pocket", "Pick-Pocket"},
     {"Sneak", "Sneak"},

     {"Pincher", "Pincheress"},
     {"Cut-Purse", "Cut-Purse"},
     {"Snatcher", "Snatcheress"},
     {"Sharper", "Sharpress"},
     {"Rogue", "Rogue"},

     {"Robber", "Robber"},
     {"Magsman", "Magswoman"},
     {"Highwayman", "Highwaywoman"},
     {"Burglar", "Burglaress"},
     {"Thief", "Thief"},

     {"Knifer", "Knifer"},
     {"Quick-Blade", "Quick-Blade"},
     {"Killer", "Murderess"},
     {"Brigand", "Brigand"},
     {"Cut-Throat", "Cut-Throat"},

     {"Spy", "Spy"},
     {"Grand Spy", "Grand Spy"},
     {"Master Spy", "Master Spy"},
     {"Assassin", "Assassin"},
     {"Greater Assassin", "Greater Assassin"},

     {"Master of Vision", "Mistress of Vision"},
     {"Master of Hearing", "Mistress of Hearing"},
     {"Master of Smell", "Mistress of Smell"},
     {"Master of Taste", "Mistress of Taste"},
     {"Master of Touch", "Mistress of Touch"},

     {"Crime Lord", "Crime Mistress"},
     {"Infamous Crime Lord", "Infamous Crime Mistress"},
     {"Greater Crime Lord", "Greater Crime Mistress"},
     {"Master Crime Lord", "Master Crime Mistress"},
     {"Godfather", "Godmother"},

     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},

     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},

     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},

     {"Assassin Hero", "Assassin Heroine"},
     {"Avatar of Death", "Avatar of Death",},
     {"Angel of Death", "Angel of Death"},
     {"Demigod of Assassins", "Demigoddess of Assassins"},
     {"Immortal Assasin", "Immortal Assassin"},
     {"God of Assassins", "God of Assassins",},
     {"Deity of Assassins", "Deity of Assassins"},
     {"Supreme Master", "Supreme Mistress"},
     {"Creator", "Creator"},
     {"Implementor", "Implementress"}
     },

    {
     {"Man", "Woman"},

     {"Swordpupil", "Swordpupil"},
     {"Recruit", "Recruit"},
     {"Sentry", "Sentress"},
     {"Fighter", "Fighter"},
     {"Soldier", "Soldier"},

     {"Warrior", "Warrior"},
     {"Veteran", "Veteran"},
     {"Swordsman", "Swordswoman"},
     {"Fencer", "Fenceress"},
     {"Combatant", "Combatess"},

     {"Hero", "Heroine"},
     {"Myrmidon", "Myrmidon"},
     {"Swashbuckler", "Swashbuckleress"},
     {"Mercenary", "Mercenaress"},
     {"Swordmaster", "Swordmistress"},

     {"Lieutenant", "Lieutenant"},
     {"Champion", "Lady Champion"},
     {"Dragoon", "Lady Dragoon"},
     {"Cavalier", "Lady Cavalier"},
     {"Knight", "Lady Knight"},

     {"Grand Knight", "Grand Knight"},
     {"Master Knight", "Master Knight"},
     {"Paladin", "Paladin"},
     {"Grand Paladin", "Grand Paladin"},
     {"Demon Slayer", "Demon Slayer"},

     {"Greater Demon Slayer", "Greater Demon Slayer"},
     {"Dragon Slayer", "Dragon Slayer"},
     {"Greater Dragon Slayer", "Greater Dragon Slayer"},
     {"Underlord", "Underlord"},
     {"Overlord", "Overlord"},

     {"Baron of Thunder", "Baroness of Thunder"},
     {"Baron of Storms", "Baroness of Storms"},
     {"Baron of Tornadoes", "Baroness of Tornadoes"},
     {"Baron of Hurricanes", "Baroness of Hurricanes"},
     {"Baron of Meteors", "Baroness of Meteors"},

     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},

     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},

     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},

     {"Knight Hero", "Knight Heroine"},
     {"Avatar of War", "Avatar of War"},
     {"Angel of War", "Angel of War"},
     {"Demigod of War", "Demigoddess of War"},
     {"Immortal Warlord", "Immortal Warlord"},
     {"God of War", "God of War"},
     {"Deity of War", "Deity of War"},
     {"Supreme Master of War", "Supreme Mistress of War"},
     {"Creator", "Creator"},
     {"Implementor", "Implementress"}
     }
};

/*
 * Attribute bonus tables.
 */
const struct str_app_type str_app[26] = {
    {-5, -4, 0, 0},                                    /* 0 */
    {-5, -4, 3, 1},                                    /* 1 */
    {-3, -2, 3, 2},
    {-3, -1, 10, 3},                                   /* 3 */
    {-2, -1, 25, 4},
    {-2, -1, 55, 5},                                   /* 5 */
    {-1, 0, 80, 6},
    {-1, 0, 90, 7},
    {0, 0, 100, 8},
    {0, 0, 100, 9},
    {0, 0, 115, 10},                                   /* 10 */
    {0, 0, 115, 11},
    {0, 0, 130, 12},
    {0, 0, 130, 13},                                   /* 13 */
    {0, 1, 140, 14},
    {1, 1, 150, 15},                                   /* 15 */
    {1, 2, 165, 16},
    {2, 3, 180, 22},
    {2, 3, 200, 25},                                   /* 18 */
    {3, 4, 225, 30},
    {3, 5, 250, 35},                                   /* 20 */
    {4, 6, 300, 40},
    {4, 6, 350, 45},
    {5, 7, 400, 50},
    {5, 8, 450, 55},
    {6, 9, 500, 60}                                    /* 25 */
};

const struct int_app_type int_app[26] = {
    {3},                                               /* 0 */
    {5},                                               /* 1 */
    {7},
    {8},                                               /* 3 */
    {9},
    {10},                                              /* 5 */
    {11},
    {12},
    {13},
    {15},
    {17},                                              /* 10 */
    {19},
    {22},
    {25},
    {28},
    {31},                                              /* 15 */
    {34},
    {37},
    {40},                                              /* 18 */
    {44},
    {49},                                              /* 20 */
    {55},
    {60},
    {70},
    {80},
    {85}                                               /* 25 */
};

const struct wis_app_type wis_app[26] = {
    {0},                                               /* 0 */
    {0},                                               /* 1 */
    {0},
    {0},                                               /* 3 */
    {0},
    {1},                                               /* 5 */
    {1},
    {1},
    {1},
    {1},
    {1},                                               /* 10 */
    {1},
    {1},
    {1},
    {1},
    {2},                                               /* 15 */
    {2},
    {2},
    {3},                                               /* 18 */
    {3},
    {3},                                               /* 20 */
    {3},
    {4},
    {4},
    {4},
    {5}                                                /* 25 */
};

const struct dex_app_type dex_app[26] = {
    {60},                                              /* 0 */
    {50},                                              /* 1 */
    {50},
    {40},
    {30},
    {20},                                              /* 5 */
    {10},
    {0},
    {0},
    {0},
    {0},                                               /* 10 */
    {0},
    {0},
    {0},
    {0},
    {-10},                                             /* 15 */
    {-15},
    {-20},
    {-30},
    {-40},
    {-50},                                             /* 20 */
    {-60},
    {-75},
    {-90},
    {-105},
    {-120}                                             /* 25 */
};

const struct con_app_type con_app[26] = {
    {-4, 20},                                          /* 0 */
    {-3, 25},                                          /* 1 */
    {-2, 30},
    {-2, 35},                                          /* 3 */
    {-1, 40},
    {-1, 45},                                          /* 5 */
    {-1, 50},
    {0, 55},
    {0, 60},
    {0, 65},
    {0, 70},                                           /* 10 */
    {0, 75},
    {0, 80},
    {0, 85},
    {0, 88},
    {1, 90},                                           /* 15 */
    {2, 95},
    {2, 97},
    {3, 99},                                           /* 18 */
    {3, 99},
    {4, 99},                                           /* 20 */
    {4, 99},
    {5, 99},
    {6, 99},
    {7, 99},
    {8, 99}                                            /* 25 */
};

/*
 * Liquid properties.
 * Used in world.obj.
 */
const struct liq_type   liq_table[] = {
/*    name                        color        proof, full, thirst, food, ssize */
    {"water", "clear", {0, 1, 10, 0, 16}},
    {"beer", "amber", {12, 1, 8, 1, 12}},
    {"red wine", "burgundy", {30, 1, 8, 1, 5}},
    {"ale", "brown", {15, 1, 8, 1, 12}},
    {"dark ale", "dark", {16, 1, 8, 1, 12}},

    {"whisky", "golden", {120, 1, 5, 0, 2}},
    {"lemonade", "pink", {0, 1, 9, 2, 12}},
    {"firebreather", "boiling", {190, 0, 4, 0, 2}},
    {"local specialty", "clear", {151, 1, 3, 0, 2}},
    {"slime mold juice", "green", {0, 2, -8, 1, 2}},

    {"milk", "white", {0, 2, 9, 3, 12}},
    {"tea", "tan", {0, 1, 8, 0, 6}},
    {"coffee", "black", {0, 1, 8, 0, 6}},
    {"blood", "red", {0, 2, -1, 2, 6}},
    {"salt water", "clear", {0, 1, -2, 0, 1}},

    {"coke", "brown", {0, 2, 9, 2, 12}},
    {"root beer", "brown", {0, 2, 9, 2, 12}},
    {"elvish wine", "green", {35, 2, 8, 1, 5}},
    {"white wine", "golden", {28, 1, 8, 1, 5}},
    {"champagne", "golden", {32, 1, 8, 1, 5}},

    {"mead", "honey-colored", {34, 2, 8, 2, 12}},
    {"rose wine", "pink", {26, 1, 8, 1, 5}},
    {"benedictine wine", "burgundy", {40, 1, 8, 1, 5}},
    {"vodka", "clear", {130, 1, 5, 0, 2}},
    {"cranberry juice", "red", {0, 1, 9, 2, 12}},

    {"orange juice", "orange", {0, 2, 9, 3, 12}},
    {"absinthe", "green", {200, 1, 4, 0, 2}},
    {"brandy", "golden", {80, 1, 5, 0, 4}},
    {"aquavit", "clear", {140, 1, 5, 0, 2}},
    {"schnapps", "clear", {90, 1, 5, 0, 2}},

    {"icewine", "purple", {50, 2, 6, 1, 5}},
    {"amontillado", "burgundy", {35, 2, 8, 1, 5}},
    {"sherry", "red", {38, 2, 7, 1, 5}},
    {"framboise", "red", {50, 1, 7, 1, 5}},
    {"rum", "amber", {151, 1, 4, 0, 2}},

    {"cordial", "clear", {100, 1, 5, 0, 2}},
    {NULL, NULL, {0, 0, 0, 0, 0}}
};

/*
 * The skill and spell table.
 * Slot numbers must never be changed as they appear in #OBJECTS sections.
 */
#define SLOT(n)        n

struct skill_type       skill_table[MAX_SKILL] = {

/*
 * Magic spells.
 */

    {
     "reserved", {99, 99, 99, 99}, {99, 99, 99, 99},
     0, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 0 ), 0, 0,
     "", "", ""},

    {
     "acid blast", {28, 53, 35, 32}, {1, 1, 2, 2},
     spell_acid_blast, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 70 ), 20, 12,
     "acid blast", "!Acid Blast!"},

    {
     "armor", {7, 2, 10, 5}, {1, 1, 2, 2},
     spell_armor, TAR_CHAR_DEFENSIVE, POS_STANDING,
     NULL, SLOT( 1 ), 5, 12,
     "", "You feel less armored.", ""},

    {
     "bless", {53, 7, 53, 8}, {1, 1, 2, 2},
     spell_bless, TAR_OBJ_CHAR_DEF, POS_STANDING,
     NULL, SLOT( 3 ), 5, 12,
     "", "You feel less righteous.",
     "$p's holy aura fades."},

    {
     "blindness", {12, 8, 17, 15}, {1, 1, 2, 2},
     spell_blindness, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 4 ), 5, 12,
     "", "You can see again.", ""},

    {
     "burning hands", {7, 53, 10, 9}, {1, 1, 2, 2},
     spell_burning_hands, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 5 ), 15, 12,
     "burning hands", "!Burning Hands!", ""},

    {
     "call lightning", {26, 18, 31, 22}, {1, 1, 2, 2},
     spell_call_lightning, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 6 ), 15, 12,
     "lightning bolt", "!Call Lightning!", ""},

    {"calm", {48, 16, 50, 20}, {1, 1, 2, 2},
     spell_calm, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 509 ), 30, 12,
     "", "You have lost your peace of mind.", ""},

    {
     "cancellation", {18, 26, 34, 34}, {1, 1, 2, 2},
     spell_cancellation, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
     NULL, SLOT( 507 ), 20, 12,
     "" "!cancellation!", ""},

    {
     "cause critical", {53, 13, 53, 19}, {1, 1, 2, 2},
     spell_cause_critical, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 63 ), 20, 12,
     "spell", "!Cause Critical!", ""},

    {
     "cause light", {53, 1, 53, 3}, {1, 1, 2, 2},
     spell_cause_light, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 62 ), 15, 12,
     "spell", "!Cause Light!", ""},

    {
     "cause serious", {53, 7, 53, 10}, {1, 1, 2, 2},
     spell_cause_serious, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 64 ), 17, 12,
     "spell", "!Cause Serious!", ""},

    {
     "chain lightning", {33, 53, 39, 36}, {1, 1, 2, 2},
     spell_chain_lightning, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 500 ), 25, 12,
     "lightning", "!Chain Lightning!", ""},

    {
     "change sex", {53, 53, 53, 53}, {1, 1, 2, 2},
     spell_change_sex, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
     NULL, SLOT( 82 ), 15, 12,
     "", "Your body feels familiar again.", ""},

    {
     "charm person", {20, 53, 25, 53}, {1, 1, 2, 2},
     spell_charm_person, TAR_CHAR_OFFENSIVE, POS_STANDING,
     NULL, SLOT( 7 ), 5, 12,
     "", "You feel more self-confident.", ""},

    {
     "chill touch", {4, 53, 6, 6}, {1, 1, 2, 2},
     spell_chill_touch, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 8 ), 15, 12,
     "chilling touch", "You feel less cold.", ""},

    {
     "colour spray", {16, 53, 22, 20}, {1, 1, 2, 2},
     spell_colour_spray, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 10 ), 15, 12,
     "colour spray", "!Colour Spray!", ""},

    {
     "continual light", {6, 4, 6, 9}, {1, 1, 2, 2},
     spell_continual_light, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 57 ), 7, 12,
     "", "!Continual Light!", ""},

    {
     "control weather", {15, 19, 28, 22}, {1, 1, 2, 2},
     spell_control_weather, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 11 ), 25, 12,
     "", "!Control Weather!", ""},

    {
     "create food", {10, 5, 11, 12}, {1, 1, 2, 2},
     spell_create_food, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 12 ), 5, 12,
     "", "!Create Food!", ""},

    {
     "create rose", {16, 11, 10, 24}, {1, 1, 2, 2},
     spell_create_rose, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 511 ), 30, 12,
     "", "!Create Rose!", ""},

    {
     "create spring", {14, 17, 23, 20}, {1, 1, 2, 2},
     spell_create_spring, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 80 ), 20, 12,
     "", "!Create Spring!", ""},

    {
     "create water", {8, 3, 12, 11}, {1, 1, 2, 2},
     spell_create_water, TAR_OBJ_INV, POS_STANDING,
     NULL, SLOT( 13 ), 5, 12,
     "", "!Create Water!", ""},

    {
     "cure blindness", {53, 6, 53, 8}, {1, 1, 2, 2},
     spell_cure_blindness, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
     NULL, SLOT( 14 ), 5, 12,
     "", "!Cure Blindness!", ""},

    {
     "cure critical", {53, 13, 53, 19}, {1, 1, 2, 2},
     spell_cure_critical, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
     NULL, SLOT( 15 ), 20, 12,
     "", "!Cure Critical!", ""},

    {
     "cure disease", {53, 13, 53, 14}, {1, 1, 2, 2},
     spell_cure_disease, TAR_CHAR_DEFENSIVE, POS_STANDING,
     NULL, SLOT( 501 ), 20, 12,
     "", "!Cure Disease!", ""},

    {
     "cure light", {53, 1, 53, 3}, {1, 1, 2, 2},
     spell_cure_light, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
     NULL, SLOT( 16 ), 10, 12,
     "", "!Cure Light!", ""},

    {
     "cure poison", {53, 14, 53, 16}, {1, 1, 2, 2},
     spell_cure_poison, TAR_CHAR_DEFENSIVE, POS_STANDING,
     NULL, SLOT( 43 ), 5, 12,
     "", "!Cure Poison!", ""},

    {
     "cure serious", {53, 7, 53, 10}, {1, 1, 2, 2},
     spell_cure_serious, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
     NULL, SLOT( 61 ), 15, 12,
     "", "!Cure Serious!", ""},

    {
     "curse", {18, 18, 26, 22}, {1, 1, 2, 2},
     spell_curse, TAR_OBJ_CHAR_OFF, POS_FIGHTING,
     NULL, SLOT( 17 ), 20, 12,
     "curse", "The curse wears off.",
     "$p is no longer impure."},

    {
     "demonfire", {53, 34, 53, 45}, {1, 1, 2, 2},
     spell_demonfire, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 505 ), 20, 12,
     "torments", "!Demonfire!", ""},

    {
     "detect evil", {11, 4, 12, 53}, {1, 1, 2, 2},
     spell_detect_evil, TAR_CHAR_SELF, POS_STANDING,
     NULL, SLOT( 18 ), 5, 12,
     "", "The red in your vision disappears.", ""},

    {
     "detect good", {11, 4, 12, 53}, {1, 1, 2, 2},
     spell_detect_good, TAR_CHAR_SELF, POS_STANDING,
     NULL, SLOT( 513 ), 5, 12,
     "", "The gold in your vision disappears.", ""},

    {
     "detect hidden", {15, 11, 12, 53}, {1, 1, 2, 2},
     spell_detect_hidden, TAR_CHAR_SELF, POS_STANDING,
     NULL, SLOT( 44 ), 5, 12,
     "", "You feel less aware of your surroundings.",
     ""},

    {
     "detect invisibility", {3, 8, 6, 53}, {1, 1, 2, 2},
     spell_detect_invisibility, TAR_CHAR_SELF, POS_STANDING,
     NULL, SLOT( 19 ), 5, 12,
     "", "You no longer see invisible objects.",
     ""},

    {
     "detect magic", {2, 6, 5, 53}, {1, 1, 2, 2},
     spell_detect_magic, TAR_CHAR_SELF, POS_STANDING,
     NULL, SLOT( 20 ), 5, 12,
     "", "The detect magic wears off.", ""},

    {
     "detect poison", {15, 7, 9, 53}, {1, 1, 2, 2},
     spell_detect_poison, TAR_OBJ_INV, POS_STANDING,
     NULL, SLOT( 21 ), 5, 12,
     "", "!Detect Poison!", ""},

    {
     "dispel evil", {53, 15, 53, 21}, {1, 1, 2, 2},
     spell_dispel_evil, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 22 ), 15, 12,
     "dispel evil", "!Dispel Evil!", ""},

    {
     "dispel good", {53, 15, 53, 21}, {1, 1, 2, 2},
     spell_dispel_good, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 512 ), 15, 12,
     "dispel good", "!Dispel Good!", ""},

    {
     "dispel magic", {16, 24, 30, 30}, {1, 1, 2, 2},
     spell_dispel_magic, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 59 ), 15, 12,
     "", "!Dispel Magic!", ""},

    {
     "earthquake", {53, 10, 53, 14}, {1, 1, 2, 2},
     spell_earthquake, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 23 ), 15, 12,
     "earthquake", "!Earthquake!", ""},

    {
     "enchant armor", {16, 53, 53, 53}, {2, 2, 4, 4},
     spell_enchant_armor, TAR_OBJ_INV, POS_STANDING,
     NULL, SLOT( 510 ), 100, 24,
     "", "!Enchant Armor!", ""},

    {
     "enchant weapon", {17, 53, 53, 53}, {2, 2, 4, 4},
     spell_enchant_weapon, TAR_OBJ_INV, POS_STANDING,
     NULL, SLOT( 24 ), 100, 24,
     "", "!Enchant Weapon!", ""},

    {
     "energy drain", {19, 22, 26, 23}, {1, 1, 2, 2},
     spell_energy_drain, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 25 ), 35, 12,
     "energy drain", "!Energy Drain!", ""},

    {
     "faerie fire", {6, 3, 5, 8}, {1, 1, 2, 2},
     spell_faerie_fire, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 72 ), 5, 12,
     "faerie fire", "The pink aura around you fades away.",
     ""},

    {
     "faerie fog", {14, 21, 16, 24}, {1, 1, 2, 2},
     spell_faerie_fog, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 73 ), 12, 12,
     "faerie fog", "!Faerie Fog!", ""},

    {
     "farsight", {14, 16, 16, 53}, {1, 1, 2, 2},
     spell_farsight, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 521 ), 36, 20,
     "farsight", "!Farsight!", ""},

    {
     "fireball", {22, 53, 30, 26}, {1, 1, 2, 2},
     spell_fireball, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 26 ), 15, 12,
     "fireball", "!Fireball!", ""},

    {
     "fireproof", {13, 12, 19, 18}, {1, 1, 2, 2},
     spell_fireproof, TAR_OBJ_INV, POS_STANDING,
     NULL, SLOT( 523 ), 10, 12,
     "", "", "$p's protective aura fades."},

    {
     "flamestrike", {53, 20, 53, 27}, {1, 1, 2, 2},
     spell_flamestrike, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 65 ), 20, 12,
     "flamestrike", "!Flamestrike!", ""},

    {
     "fly", {10, 18, 20, 22}, {1, 1, 2, 2},
     spell_fly, TAR_CHAR_DEFENSIVE, POS_STANDING,
     NULL, SLOT( 56 ), 10, 18,
     "", "You slowly float to the ground.", ""},

    {
     "floating disc", {4, 10, 7, 16}, {1, 1, 2, 2},
     spell_floating_disc, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 522 ), 40, 24,
     "", "!Floating disc!", ""},

    {
     "frenzy", {53, 24, 53, 26}, {1, 1, 2, 2},
     spell_frenzy, TAR_CHAR_DEFENSIVE, POS_STANDING,
     NULL, SLOT( 504 ), 30, 24,
     "", "Your rage ebbs.", ""},

    {
     "gate", {27, 17, 32, 28}, {1, 1, 2, 2},
     spell_gate, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 83 ), 80, 12,
     "", "!Gate!", ""},

    {
     "giant strength", {11, 53, 22, 20}, {1, 1, 2, 2},
     spell_giant_strength, TAR_CHAR_DEFENSIVE, POS_STANDING,
     NULL, SLOT( 39 ), 20, 12,
     "", "You feel weaker.", ""},

    {
     "harm", {53, 23, 53, 28}, {1, 1, 2, 2},
     spell_harm, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 27 ), 35, 12,
     "harm spell", "!Harm!", ""},

    {
     "haste", {21, 53, 26, 29}, {1, 1, 2, 2},
     spell_haste, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
     NULL, SLOT( 502 ), 30, 12,
     "", "You feel yourself slow down.", ""},

    {
     "heal", {53, 21, 33, 30}, {1, 1, 2, 2},
     spell_heal, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
     NULL, SLOT( 28 ), 50, 12,
     "", "!Heal!", ""},

    {
     "heat metal", {53, 16, 53, 23}, {1, 1, 2, 2},
     spell_heat_metal, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 516 ), 25, 18,
     "spell", "!Heat Metal!", ""},

    {
     "holy word", {53, 36, 53, 42}, {2, 2, 4, 4},
     spell_holy_word, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 506 ), 200, 24,
     "divine wrath", "!Holy Word!", ""},

    {
     "identify", {15, 16, 18, 53}, {1, 1, 2, 2},
     spell_identify, TAR_OBJ_INV, POS_STANDING,
     NULL, SLOT( 53 ), 12, 24,
     "", "!Identify!", ""},

    {
     "infravision", {9, 13, 10, 16}, {1, 1, 2, 2},
     spell_infravision, TAR_CHAR_DEFENSIVE, POS_STANDING,
     NULL, SLOT( 77 ), 5, 18,
     "", "You no longer see in the dark.", ""},

    {
     "invisibility", {5, 53, 9, 53}, {1, 1, 2, 2},
     spell_invisibility, TAR_OBJ_CHAR_DEF, POS_STANDING,
     NULL, SLOT( 29 ), 5, 12,
     "", "You are no longer invisible.",
     "$p fades into view."},

    {
     "know alignment", {12, 9, 20, 53}, {1, 1, 2, 2},
     spell_know_alignment, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
     NULL, SLOT( 58 ), 9, 12,
     "", "!Know Alignment!", ""},

    {
     "lightning bolt", {13, 23, 18, 16}, {1, 1, 2, 2},
     spell_lightning_bolt, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 30 ), 15, 12,
     "lightning bolt", "!Lightning Bolt!", ""},

    {
     "locate object", {9, 15, 11, 53}, {1, 1, 2, 2},
     spell_locate_object, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 31 ), 20, 18,
     "", "!Locate Object!", ""},

    {
     "magic missile", {1, 53, 2, 2}, {1, 1, 2, 2},
     spell_magic_missile, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 32 ), 15, 12,
     "magic missile", "!Magic Missile!", ""},

    {
     "mass healing", {53, 38, 53, 46}, {2, 2, 4, 4},
     spell_mass_healing, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 508 ), 100, 36,
     "", "!Mass Healing!", ""},

    {
     "mass invisibility", {22, 25, 31, 53}, {1, 1, 2, 2},
     spell_mass_invisibility, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 69 ), 20, 24,
     "", "You are no longer invisible.", ""},

    {
     "nexus", {40, 35, 50, 45}, {2, 2, 4, 4},
     spell_nexus, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 520 ), 150, 36,
     "", "!Nexus!", ""},

    {
     "pass door", {24, 32, 25, 37}, {1, 1, 2, 2},
     spell_pass_door, TAR_CHAR_SELF, POS_STANDING,
     NULL, SLOT( 74 ), 20, 12,
     "", "You feel solid again.", ""},

    {
     "plague", {23, 17, 36, 26}, {1, 1, 2, 2},
     spell_plague, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 503 ), 20, 12,
     "sickness", "Your sores vanish.", ""},

    {
     "poison", {17, 12, 15, 21}, {1, 1, 2, 2},
     spell_poison, TAR_OBJ_CHAR_OFF, POS_FIGHTING,
     NULL, SLOT( 33 ), 10, 12,
     "poison", "You feel less sick.",
     "The poison on $p dries up."},

    {
     "portal", {35, 30, 45, 40}, {2, 2, 4, 4},
     spell_portal, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 519 ), 100, 24,
     "", "!Portal!", ""},

    {
     "protection evil", {12, 9, 17, 11}, {1, 1, 2, 2},
     spell_protection_evil, TAR_CHAR_SELF, POS_STANDING,
     NULL, SLOT( 34 ), 5, 12,
     "", "You feel less protected.", ""},

    {
     "protection good", {12, 9, 17, 11}, {1, 1, 2, 2},
     spell_protection_good, TAR_CHAR_SELF, POS_STANDING,
     NULL, SLOT( 514 ), 5, 12,
     "", "You feel less protected.", ""},

    {
     "ray of truth", {53, 35, 53, 47}, {1, 1, 2, 2},
     spell_ray_of_truth, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 518 ), 20, 12,
     "ray of truth", "!Ray of Truth!", ""},

    {
     "recharge", {9, 53, 53, 53}, {1, 1, 2, 2},
     spell_recharge, TAR_OBJ_INV, POS_STANDING,
     NULL, SLOT( 517 ), 60, 24,
     "", "!Recharge!", ""},

    {
     "refresh", {8, 5, 12, 9}, {1, 1, 2, 2},
     spell_refresh, TAR_CHAR_DEFENSIVE, POS_STANDING,
     NULL, SLOT( 81 ), 12, 18,
     "refresh", "!Refresh!", ""},

    {
     "remove curse", {53, 18, 53, 22}, {1, 1, 2, 2},
     spell_remove_curse, TAR_OBJ_CHAR_DEF, POS_STANDING,
     NULL, SLOT( 35 ), 5, 12,
     "", "!Remove Curse!", ""},

    {
     "sanctuary", {36, 20, 42, 30}, {1, 1, 2, 2},
     spell_sanctuary, TAR_CHAR_DEFENSIVE, POS_STANDING,
     NULL, SLOT( 36 ), 75, 12,
     "", "The white aura around your body fades.",
     ""},

    {
     "shield", {20, 35, 35, 40}, {1, 1, 2, 2},
     spell_shield, TAR_CHAR_DEFENSIVE, POS_STANDING,
     NULL, SLOT( 67 ), 12, 18,
     "", "Your force shield shimmers then fades away.",
     ""},

    {
     "shocking grasp", {10, 53, 14, 13}, {1, 1, 2, 2},
     spell_shocking_grasp, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 53 ), 15, 12,
     "shocking grasp", "!Shocking Grasp!", ""},

    {
     "sleep", {10, 53, 11, 53}, {1, 1, 2, 2},
     spell_sleep, TAR_CHAR_OFFENSIVE, POS_STANDING,
     NULL, SLOT( 38 ), 15, 12,
     "", "You feel less tired.", ""},

    {
     "slow", {23, 30, 29, 32}, {1, 1, 2, 2},
     spell_slow, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 515 ), 30, 12,
     "", "You feel yourself speed up.", ""},

    {
     "stone skin", {25, 40, 40, 45}, {1, 1, 2, 2},
     spell_stone_skin, TAR_CHAR_SELF, POS_STANDING,
     NULL, SLOT( 66 ), 12, 18,
     "", "Your skin feels soft again.", ""},

    {
     "summon", {24, 12, 29, 22}, {1, 1, 2, 2},
     spell_summon, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 40 ), 50, 12,
     "", "!Summon!", ""},

    {
     "teleport", {13, 22, 25, 36}, {1, 1, 2, 2},
     spell_teleport, TAR_CHAR_SELF, POS_FIGHTING,
     NULL, SLOT( 2 ), 35, 12,
     "", "!Teleport!", ""},

    {
     "ventriloquate", {1, 53, 2, 53}, {1, 1, 2, 2},
     spell_ventriloquate, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 41 ), 5, 12,
     "", "!Ventriloquate!", ""},

    {
     "weaken", {11, 14, 16, 17}, {1, 1, 2, 2},
     spell_weaken, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 68 ), 20, 12,
     "spell", "You feel stronger.", ""},

    {
     "word of recall", {32, 28, 40, 30}, {1, 1, 2, 2},
     spell_word_of_recall, TAR_CHAR_SELF, POS_RESTING,
     NULL, SLOT( 42 ), 5, 12,
     "", "!Word of Recall!", ""},

/*
 * Dragon breath
 */
    {
     "acid breath", {31, 32, 33, 34}, {1, 1, 2, 2},
     spell_acid_breath, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 200 ), 100, 24,
     "blast of acid", "!Acid Breath!", ""},

    {
     "fire breath", {40, 45, 50, 51}, {1, 1, 2, 2},
     spell_fire_breath, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 201 ), 200, 24,
     "blast of flame", "The smoke leaves your eyes.", ""},

    {
     "frost breath", {34, 36, 38, 40}, {1, 1, 2, 2},
     spell_frost_breath, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 202 ), 125, 24,
     "blast of frost", "!Frost Breath!", ""},

    {
     "gas breath", {39, 43, 47, 50}, {1, 1, 2, 2},
     spell_gas_breath, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 203 ), 175, 24,
     "blast of gas", "!Gas Breath!", ""},

    {
     "lightning breath", {37, 40, 43, 46}, {1, 1, 2, 2},
     spell_lightning_breath, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 204 ), 150, 24,
     "blast of lightning", "!Lightning Breath!", ""},

/*
 * Spells for mega1.are from Glop/Erkenbrand.
 */
    {
     "general purpose", {53, 53, 53, 53}, {0, 0, 0, 0},
     spell_general_purpose, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 401 ), 0, 12,
     "general purpose ammo", "!General Purpose Ammo!", ""},

    {
     "high explosive", {53, 53, 53, 53}, {0, 0, 0, 0},
     spell_high_explosive, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 402 ), 0, 12,
     "high explosive ammo", "!High Explosive Ammo!", ""},

/* combat and weapons skills */

    {
     "axe", {1, 1, 1, 1}, {6, 6, 5, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 98 ), 0, 0,
     "", "!Axe!", ""},

    {
     "dagger", {1, 1, 1, 1}, {2, 3, 2, 2},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 99 ), 0, 0,
     "", "!Dagger!", ""},

    {
     "flail", {1, 1, 1, 1}, {6, 3, 6, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 100 ), 0, 0,
     "", "!Flail!", ""},

    {
     "mace", {1, 1, 1, 1}, {5, 2, 3, 3},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 101 ), 0, 0,
     "", "!Mace!", ""},

    {
     "polearm", {1, 1, 1, 1}, {6, 6, 6, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 102 ), 0, 0,
     "", "!Polearm!", ""},

    {
     "shield block", {1, 1, 1, 1}, {6, 4, 6, 2},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 103 ), 0, 0,
     "", "!Shield!", ""},

    {
     "spear", {1, 1, 1, 1}, {4, 4, 4, 3},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 104 ), 0, 0,
     "", "!Spear!", ""},

    {
     "sword", {1, 1, 1, 1}, {5, 6, 3, 2},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 105 ), 0, 0,
     "", "!sword!", ""},

    {
     "whip", {1, 1, 1, 1}, {6, 5, 5, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 106 ), 0, 0,
     "", "!Whip!", ""},

    {
     "backstab", {53, 53, 1, 53}, {0, 0, 5, 0},
     spell_null, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 107 ), 0, 24,
     "backstab", "!Backstab!", ""},

    {
     "bash", {53, 53, 53, 1}, {0, 0, 0, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 108 ), 0, 24,
     "bash", "!Bash!", ""},

    {
     "berserk", {53, 53, 53, 18}, {0, 0, 0, 5},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 109 ), 0, 24,
     "", "You feel your pulse slow down.", ""},

    {
     "dirt kicking", {53, 53, 3, 3}, {0, 0, 4, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 110 ), 0, 24,
     "kicked dirt", "You rub the dirt out of your eyes.", ""},

    {
     "disarm", {53, 53, 12, 11}, {0, 0, 6, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 111 ), 0, 24,
     "", "!Disarm!", ""},

    {
     "dodge", {20, 22, 1, 13}, {8, 8, 4, 6},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 112 ), 0, 0,
     "", "!Dodge!", ""},

    {
     "enhanced damage", {45, 30, 25, 1}, {10, 9, 5, 3},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 113 ), 0, 0,
     "", "!Enhanced Damage!", ""},

    {
     "envenom", {53, 53, 10, 53}, {0, 0, 4, 0},
     spell_null, TAR_IGNORE, POS_RESTING,
     NULL, SLOT( 114 ), 0, 36,
     "", "!Envenom!", ""},

    {
     "hand to hand", {25, 10, 15, 6}, {8, 5, 6, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 115 ), 0, 0,
     "", "!Hand to Hand!", ""},

    {
     "kick", {53, 12, 14, 8}, {0, 4, 6, 3},
     spell_null, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT( 116 ), 0, 12,
     "kick", "!Kick!", ""},

    {
     "parry", {22, 20, 13, 1}, {8, 8, 6, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 117 ), 0, 0,
     "", "!Parry!", ""},

    {
     "rescue", {53, 53, 53, 1}, {0, 0, 0, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 118 ), 0, 12,
     "", "!Rescue!", ""},

    {
     "trip", {53, 53, 1, 15}, {0, 0, 4, 8},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 119 ), 0, 24,
     "trip", "!Trip!", ""},

    {
     "second attack", {30, 24, 12, 5}, {10, 8, 5, 3},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 120 ), 0, 0,
     "", "!Second Attack!", ""},

    {
     "third attack", {53, 53, 24, 12}, {0, 0, 10, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT( 121 ), 0, 0,
     "", "!Third Attack!", ""},

/* non-combat skills */

    {
     "fast healing", {15, 9, 16, 6}, {8, 5, 6, 4},
     spell_null, TAR_IGNORE, POS_SLEEPING,
     NULL, SLOT( 122 ), 0, 0,
     "", "!Fast Healing!", ""},

    {
     "haggle", {7, 18, 1, 14}, {5, 8, 3, 6},
     spell_null, TAR_IGNORE, POS_RESTING,
     NULL, SLOT( 123 ), 0, 0,
     "", "!Haggle!", ""},

    {
     "hide", {53, 53, 1, 12}, {0, 0, 4, 6},
     spell_null, TAR_IGNORE, POS_RESTING,
     NULL, SLOT( 124 ), 0, 12,
     "", "!Hide!", ""},

    {
     "lore", {10, 10, 6, 20}, {6, 6, 4, 8},
     spell_null, TAR_IGNORE, POS_RESTING,
     NULL, SLOT( 125 ), 0, 36,
     "", "!Lore!", ""},

    {
     "meditation", {6, 6, 15, 15}, {5, 5, 8, 8},
     spell_null, TAR_IGNORE, POS_SLEEPING,
     NULL, SLOT( 126 ), 0, 0,
     "", "Meditation", ""},

    {
     "peek", {8, 21, 1, 14}, {5, 7, 3, 6},
     spell_null, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 127 ), 0, 0,
     "", "!Peek!", ""},

    {
     "pick lock", {25, 25, 7, 25}, {8, 8, 4, 8},
     spell_null, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 128 ), 0, 12,
     "", "!Pick!", ""},

    {
     "sneak", {53, 53, 4, 10}, {0, 0, 4, 6},
     spell_null, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 129 ), 0, 12,
     "", "You no longer feel stealthy.", ""},

    {
     "steal", {53, 53, 5, 53}, {0, 0, 4, 0},
     spell_null, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 130 ), 0, 24,
     "", "!Steal!", ""},

    {
     "scrolls", {1, 1, 1, 1}, {2, 3, 5, 8},
     spell_null, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 131 ), 0, 24,
     "", "!Scrolls!", ""},

    {
     "staves", {1, 1, 1, 1}, {2, 3, 5, 8},
     spell_null, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 132 ), 0, 12,
     "", "!Staves!", ""},

    {
     "wands", {1, 1, 1, 1}, {2, 3, 5, 8},
     spell_null, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 133 ), 0, 12,
     "", "!Wands!", ""},

    {
     "recall", {1, 1, 1, 1}, {2, 2, 2, 2},
     spell_null, TAR_IGNORE, POS_STANDING,
     NULL, SLOT( 134 ), 0, 12,
     "", "!Recall!", ""}
};

const struct group_type group_table[MAX_GROUP] = {

    {
     "rom basics", {0, 0, 0, 0},
     {"scrolls", "staves", "wands", "recall"}
     },

    {
     "mage basics", {0, -1, -1, -1},
     {"dagger"}
     },

    {
     "cleric basics", {-1, 0, -1, -1},
     {"mace"}
     },

    {
     "thief basics", {-1, -1, 0, -1},
     {"dagger", "steal"}
     },

    {
     "warrior basics", {-1, -1, -1, 0},
     {"sword", "second attack"}
     },

    {
     "mage default", {40, -1, -1, -1},
     {"lore", "beguiling", "combat", "detection", "enhancement", "illusion",
      "maladictions", "protective", "transportation", "weather"}
     },

    {
     "cleric default", {-1, 40, -1, -1},
     {"flail", "attack", "creation", "curative", "benedictions",
      "detection", "healing", "maladictions", "protective", "shield block",
      "transportation", "weather"}
     },

    {
     "thief default", {-1, -1, 40, -1},
     {"mace", "sword", "backstab", "disarm", "dodge", "second attack",
      "trip", "hide", "peek", "pick lock", "sneak"}
     },

    {
     "warrior default", {-1, -1, -1, 40},
     {"weaponsmaster", "shield block", "bash", "disarm", "enhanced damage",
      "parry", "rescue", "third attack"}
     },

    {
     "weaponsmaster", {40, 40, 40, 20},
     {"axe", "dagger", "flail", "mace", "polearm", "spear", "sword", "whip"}
     },

    {
     "attack", {-1, 5, -1, 8},
     {"demonfire", "dispel evil", "dispel good", "earthquake",
      "flamestrike", "heat metal", "ray of truth"}
     },

    {
     "beguiling", {4, -1, 6, -1},
     {"calm", "charm person", "sleep"}
     },

    {
     "benedictions", {-1, 4, -1, 8},
     {"bless", "calm", "frenzy", "holy word", "remove curse"}
     },

    {
     "combat", {6, -1, 10, 9},
     {"acid blast", "burning hands", "chain lightning", "chill touch",
      "colour spray", "fireball", "lightning bolt", "magic missile",
      "shocking grasp"}
     },

    {
     "creation", {4, 4, 8, 8},
     {"continual light", "create food", "create spring", "create water",
      "create rose", "floating disc"}
     },

    {
     "curative", {-1, 4, -1, 8},
     {"cure blindness", "cure disease", "cure poison"}
     },

    {
     "detection", {4, 3, 6, -1},
     {"detect evil", "detect good", "detect hidden", "detect invisibility",
      "detect magic", "detect poison", "farsight", "identify",
      "know alignment", "locate object"}
     },

    {
     "draconian", {8, -1, -1, -1},
     {"acid breath", "fire breath", "frost breath", "gas breath",
      "lightning breath"}
     },

    {
     "enchantment", {6, -1, -1, -1},
     {"enchant armor", "enchant weapon", "fireproof", "recharge"}
     },

    {
     "enhancement", {5, -1, 9, 9},
     {"giant strength", "haste", "infravision", "refresh"}
     },

    {
     "harmful", {-1, 3, -1, 6},
     {"cause critical", "cause light", "cause serious", "harm"}
     },

    {
     "healing", {-1, 3, -1, 6},
     {"cure critical", "cure light", "cure serious", "heal",
      "mass healing", "refresh"}
     },

    {
     "illusion", {4, -1, 7, -1},
     {"invisibility", "mass invisibility", "ventriloquate"}
     },

    {
     "maladictions", {5, 5, 9, 9},
     {"blindness", "change sex", "curse", "energy drain", "plague",
      "poison", "slow", "weaken"}
     },

    {
     "protective", {4, 4, 7, 8},
     {"armor", "cancellation", "dispel magic", "fireproof",
      "protection evil", "protection good", "sanctuary", "shield",
      "stone skin"}
     },

    {
     "transportation", {4, 4, 8, 9},
     {"fly", "gate", "nexus", "pass door", "portal", "summon", "teleport",
      "word of recall"}
     },

    {
     "weather", {4, 4, 8, 8},
     {"call lightning", "control weather", "faerie fire", "faerie fog",
      "lightning bolt"}
     }

};

const struct flag_type  mprog_flags[] = {
    {"act", TRIG_ACT, true},
    {"bribe", TRIG_BRIBE, true},
    {"death", TRIG_DEATH, true},
    {"entry", TRIG_ENTRY, true},
    {"fight", TRIG_FIGHT, true},
    {"give", TRIG_GIVE, true},
    {"greet", TRIG_GREET, true},
    {"grall", TRIG_GRALL, true},
    {"kill", TRIG_KILL, true},
    {"hpcnt", TRIG_HPCNT, true},
    {"random", TRIG_RANDOM, true},
    {"speech", TRIG_SPEECH, true},
    {"exit", TRIG_EXIT, true},
    {"exall", TRIG_EXALL, true},
    {"delay", TRIG_DELAY, true},
    {"surr", TRIG_SURR, true},
    {NULL, 0, true}
};

const struct flag_type  area_flags[] = {
    {"none", AREA_NONE, false},
    {"changed", AREA_CHANGED, true},
    {"added", AREA_ADDED, true},
    {"loading", AREA_LOADING, false},
    {NULL, 0, 0}
};

const struct flag_type  sex_flags[] = {
    {"male", SEX_MALE, true},
    {"female", SEX_FEMALE, true},
    {"neutral", SEX_NEUTRAL, true},
    {"random", 3, true},                               /* ROM */
    {"none", SEX_NEUTRAL, true},
    {NULL, 0, 0}
};

const struct flag_type  exit_flags[] = {
    {"door", EX_ISDOOR, true},
    {"closed", EX_CLOSED, true},
    {"locked", EX_LOCKED, true},
    {"pickproof", EX_PICKPROOF, true},
    {"nopass", EX_NOPASS, true},
    {"easy", EX_EASY, true},
    {"hard", EX_HARD, true},
    {"infuriating", EX_INFURIATING, true},
    {"noclose", EX_NOCLOSE, true},
    {"nolock", EX_NOLOCK, true},
    {NULL, 0, 0}
};

const struct flag_type  door_resets[] = {
    {"open and unlocked", 0, true},
    {"closed and unlocked", 1, true},
    {"closed and locked", 2, true},
    {NULL, 0, 0}
};

const struct flag_type  room_flags[] = {
    {"dark", ROOM_DARK, true},
    {"no_mob", ROOM_NO_MOB, true},
    {"indoors", ROOM_INDOORS, true},
    {"private", ROOM_PRIVATE, true},
    {"safe", ROOM_SAFE, true},
    {"solitary", ROOM_SOLITARY, true},
    {"pet_shop", ROOM_PET_SHOP, true},
    {"no_recall", ROOM_NO_RECALL, true},
    {"imp_only", ROOM_IMP_ONLY, true},
    {"gods_only", ROOM_GODS_ONLY, true},
    {"heroes_only", ROOM_HEROES_ONLY, true},
    {"newbies_only", ROOM_NEWBIES_ONLY, true},
    {"law", ROOM_LAW, true},
    {"nowhere", ROOM_NOWHERE, true},
    {NULL, 0, 0}
};

const struct flag_type  sector_flags[] = {
    {"inside", SECT_INSIDE, true},
    {"city", SECT_CITY, true},
    {"field", SECT_FIELD, true},
    {"forest", SECT_FOREST, true},
    {"hills", SECT_HILLS, true},
    {"mountain", SECT_MOUNTAIN, true},
    {"swim", SECT_WATER_SWIM, true},
    {"noswim", SECT_WATER_NOSWIM, true},
    {"unused", SECT_UNUSED, true},
    {"air", SECT_AIR, true},
    {"desert", SECT_DESERT, true},
    {NULL, 0, 0}
};

const struct flag_type  type_flags[] = {
    {"light", ITEM_LIGHT, true},
    {"scroll", ITEM_SCROLL, true},
    {"wand", ITEM_WAND, true},
    {"staff", ITEM_STAFF, true},
    {"weapon", ITEM_WEAPON, true},
    {"treasure", ITEM_TREASURE, true},
    {"armor", ITEM_ARMOR, true},
    {"potion", ITEM_POTION, true},
    {"furniture", ITEM_FURNITURE, true},
    {"trash", ITEM_TRASH, true},
    {"container", ITEM_CONTAINER, true},
    {"drinkcontainer", ITEM_DRINK_CON, true},
    {"key", ITEM_KEY, true},
    {"food", ITEM_FOOD, true},
    {"money", ITEM_MONEY, true},
    {"boat", ITEM_BOAT, true},
    {"npccorpse", ITEM_CORPSE_NPC, true},
    {"pc corpse", ITEM_CORPSE_PC, false},
    {"fountain", ITEM_FOUNTAIN, true},
    {"pill", ITEM_PILL, true},
    {"protect", ITEM_PROTECT, true},
    {"map", ITEM_MAP, true},
    {"portal", ITEM_PORTAL, true},
    {"warpstone", ITEM_WARP_STONE, true},
    {"roomkey", ITEM_ROOM_KEY, true},
    {"gem", ITEM_GEM, true},
    {"jewelry", ITEM_JEWELRY, true},
/*  {"jukebox", ITEM_JUKEBOX, true}, Legacy */
    {NULL, 0, 0}
};

const struct flag_type  extra_flags[] = {
    {"glow", ITEM_GLOW, true},
    {"hum", ITEM_HUM, true},
    {"dark", ITEM_DARK, true},
    {"lock", ITEM_LOCK, true},
    {"evil", ITEM_EVIL, true},
    {"invis", ITEM_INVIS, true},
    {"magic", ITEM_MAGIC, true},
    {"nodrop", ITEM_NODROP, true},
    {"bless", ITEM_BLESS, true},
    {"antigood", ITEM_ANTI_GOOD, true},
    {"antievil", ITEM_ANTI_EVIL, true},
    {"antineutral", ITEM_ANTI_NEUTRAL, true},
    {"noremove", ITEM_NOREMOVE, true},
    {"inventory", ITEM_INVENTORY, true},
    {"nopurge", ITEM_NOPURGE, true},
    {"rotdeath", ITEM_ROT_DEATH, true},
    {"visdeath", ITEM_VIS_DEATH, true},
    {"nonmetal", ITEM_NONMETAL, true},
    {"meltdrop", ITEM_MELT_DROP, true},
    {"hadtimer", ITEM_HAD_TIMER, true},
    {"sellextract", ITEM_SELL_EXTRACT, true},
    {"burnproof", ITEM_BURN_PROOF, true},
    {"nouncurse", ITEM_NOUNCURSE, true},
    {NULL, 0, 0}
};

const struct flag_type  wear_flags[] = {
    {"take", ITEM_TAKE, true},
    {"finger", ITEM_WEAR_FINGER, true},
    {"neck", ITEM_WEAR_NECK, true},
    {"body", ITEM_WEAR_BODY, true},
    {"head", ITEM_WEAR_HEAD, true},
    {"legs", ITEM_WEAR_LEGS, true},
    {"feet", ITEM_WEAR_FEET, true},
    {"hands", ITEM_WEAR_HANDS, true},
    {"arms", ITEM_WEAR_ARMS, true},
    {"shield", ITEM_WEAR_SHIELD, true},
    {"about", ITEM_WEAR_ABOUT, true},
    {"waist", ITEM_WEAR_WAIST, true},
    {"wrist", ITEM_WEAR_WRIST, true},
    {"wield", ITEM_WIELD, true},
    {"hold", ITEM_HOLD, true},
    {"nosac", ITEM_NO_SAC, true},
    {"wearfloat", ITEM_WEAR_FLOAT, true},
/*    {   "twohands",            ITEM_TWO_HANDS,         true    }, */
    {NULL, 0, 0}
};

/*
 * Used when adding an affect to tell where it goes.
 * See addaffect and delaffect in act_olc.c
 */
const struct flag_type  apply_flags[] = {
    {"none", APPLY_NONE, true},
    {"strength", APPLY_STR, true},
    {"dexterity", APPLY_DEX, true},
    {"intelligence", APPLY_INT, true},
    {"wisdom", APPLY_WIS, true},
    {"constitution", APPLY_CON, true},
    {"sex", APPLY_SEX, true},
    {"class", APPLY_CLASS, true},
    {"level", APPLY_LEVEL, true},
    {"age", APPLY_AGE, true},
    {"height", APPLY_HEIGHT, true},
    {"weight", APPLY_WEIGHT, true},
    {"mana", APPLY_MANA, true},
    {"hp", APPLY_HIT, true},
    {"move", APPLY_MOVE, true},
    {"gold", APPLY_GOLD, true},
    {"experience", APPLY_EXP, true},
    {"ac", APPLY_AC, true},
    {"hitroll", APPLY_HITROLL, true},
    {"damroll", APPLY_DAMROLL, true},
    {"saves", APPLY_SAVES, true},
    {"savingpara", APPLY_SAVING_PARA, true},
    {"savingrod", APPLY_SAVING_ROD, true},
    {"savingpetri", APPLY_SAVING_PETRI, true},
    {"savingbreath", APPLY_SAVING_BREATH, true},
    {"savingspell", APPLY_SAVING_SPELL, true},
    {"spellaffect", APPLY_SPELL_AFFECT, false},
    {NULL, 0, 0}
};

/*
 * What is seen.
 */
const struct flag_type  wear_loc_strings[] = {
    {"in the inventory", WEAR_NONE, true},
    {"as a light", WEAR_LIGHT, true},
    {"on the left finger", WEAR_FINGER_L, true},
    {"on the right finger", WEAR_FINGER_R, true},
    {"around the neck (1)", WEAR_NECK_1, true},
    {"around the neck (2)", WEAR_NECK_2, true},
    {"on the body", WEAR_BODY, true},
    {"over the head", WEAR_HEAD, true},
    {"on the legs", WEAR_LEGS, true},
    {"on the feet", WEAR_FEET, true},
    {"on the hands", WEAR_HANDS, true},
    {"on the arms", WEAR_ARMS, true},
    {"as a shield", WEAR_SHIELD, true},
    {"about the shoulders", WEAR_ABOUT, true},
    {"around the waist", WEAR_WAIST, true},
    {"on the left wrist", WEAR_WRIST_L, true},
    {"on the right wrist", WEAR_WRIST_R, true},
    {"wielded", WEAR_WIELD, true},
    {"held in the hands", WEAR_HOLD, true},
    {"floating nearby", WEAR_FLOAT, true},
    {NULL, 0, 0}
};

const struct flag_type  wear_loc_flags[] = {
    {"none", WEAR_NONE, true},
    {"light", WEAR_LIGHT, true},
    {"lfinger", WEAR_FINGER_L, true},
    {"rfinger", WEAR_FINGER_R, true},
    {"neck1", WEAR_NECK_1, true},
    {"neck2", WEAR_NECK_2, true},
    {"body", WEAR_BODY, true},
    {"head", WEAR_HEAD, true},
    {"legs", WEAR_LEGS, true},
    {"feet", WEAR_FEET, true},
    {"hands", WEAR_HANDS, true},
    {"arms", WEAR_ARMS, true},
    {"shield", WEAR_SHIELD, true},
    {"about", WEAR_ABOUT, true},
    {"waist", WEAR_WAIST, true},
    {"lwrist", WEAR_WRIST_L, true},
    {"rwrist", WEAR_WRIST_R, true},
    {"wielded", WEAR_WIELD, true},
    {"hold", WEAR_HOLD, true},
    {"floating", WEAR_FLOAT, true},
    {NULL, 0, 0}
};

const struct flag_type  container_flags[] = {
    {"closeable", 1, true},
    {"pickproof", 2, true},
    {"closed", 4, true},
    {"locked", 8, true},
    {"puton", 16, true},
    {NULL, 0, 0}
};

/*****************************************************************************
                      ROM - specific tables:
 ****************************************************************************/

const struct flag_type  ac_type[] = {
    {"pierce", AC_PIERCE, true},
    {"bash", AC_BASH, true},
    {"slash", AC_SLASH, true},
    {"exotic", AC_EXOTIC, true},
    {NULL, 0, 0}
};

const struct flag_type  size_flags[] = {
    {"tiny", SIZE_TINY, true},
    {"small", SIZE_SMALL, true},
    {"medium", SIZE_MEDIUM, true},
    {"large", SIZE_LARGE, true},
    {"huge", SIZE_HUGE, true},
    {"giant", SIZE_GIANT, true},
    {NULL, 0, 0},
};

const struct flag_type  weapon_class[] = {
    {"exotic", WEAPON_EXOTIC, true},
    {"sword", WEAPON_SWORD, true},
    {"dagger", WEAPON_DAGGER, true},
    {"spear", WEAPON_SPEAR, true},
    {"mace", WEAPON_MACE, true},
    {"axe", WEAPON_AXE, true},
    {"flail", WEAPON_FLAIL, true},
    {"whip", WEAPON_WHIP, true},
    {"polearm", WEAPON_POLEARM, true},
    {NULL, 0, 0}
};

const struct flag_type  weapon_type2[] = {
    {"flaming", WEAPON_FLAMING, true},
    {"frost", WEAPON_FROST, true},
    {"vampiric", WEAPON_VAMPIRIC, true},
    {"sharp", WEAPON_SHARP, true},
    {"vorpal", WEAPON_VORPAL, true},
    {"twohands", WEAPON_TWO_HANDS, true},
    {"shocking", WEAPON_SHOCKING, true},
    {"poison", WEAPON_POISON, true},
    {NULL, 0, 0}
};

const struct flag_type  res_flags[] = {
    {"summon", RES_SUMMON, true},
    {"charm", RES_CHARM, true},
    {"magic", RES_MAGIC, true},
    {"weapon", RES_WEAPON, true},
    {"bash", RES_BASH, true},
    {"pierce", RES_PIERCE, true},
    {"slash", RES_SLASH, true},
    {"fire", RES_FIRE, true},
    {"cold", RES_COLD, true},
    {"lightning", RES_LIGHTNING, true},
    {"acid", RES_ACID, true},
    {"poison", RES_POISON, true},
    {"negative", RES_NEGATIVE, true},
    {"holy", RES_HOLY, true},
    {"energy", RES_ENERGY, true},
    {"mental", RES_MENTAL, true},
    {"disease", RES_DISEASE, true},
    {"drowning", RES_DROWNING, true},
    {"light", RES_LIGHT, true},
    {"sound", RES_SOUND, true},
    {"wood", RES_WOOD, true},
    {"silver", RES_SILVER, true},
    {"iron", RES_IRON, true},
    {NULL, 0, 0}
};

const struct flag_type  vuln_flags[] = {
    {"summon", VULN_SUMMON, true},
    {"charm", VULN_CHARM, true},
    {"magic", VULN_MAGIC, true},
    {"weapon", VULN_WEAPON, true},
    {"bash", VULN_BASH, true},
    {"pierce", VULN_PIERCE, true},
    {"slash", VULN_SLASH, true},
    {"fire", VULN_FIRE, true},
    {"cold", VULN_COLD, true},
    {"lightning", VULN_LIGHTNING, true},
    {"acid", VULN_ACID, true},
    {"poison", VULN_POISON, true},
    {"negative", VULN_NEGATIVE, true},
    {"holy", VULN_HOLY, true},
    {"energy", VULN_ENERGY, true},
    {"mental", VULN_MENTAL, true},
    {"disease", VULN_DISEASE, true},
    {"drowning", VULN_DROWNING, true},
    {"light", VULN_LIGHT, true},
    {"sound", VULN_SOUND, true},
    {"wood", VULN_WOOD, true},
    {"silver", VULN_SILVER, true},
    {"iron", VULN_IRON, true},
    {NULL, 0, 0}
};

const struct flag_type  position_flags[] = {
    {"dead", POS_DEAD, false},
    {"mortal", POS_MORTAL, false},
    {"incap", POS_INCAP, false},
    {"stunned", POS_STUNNED, false},
    {"sleeping", POS_SLEEPING, true},
    {"resting", POS_RESTING, true},
    {"sitting", POS_SITTING, true},
    {"fighting", POS_FIGHTING, false},
    {"standing", POS_STANDING, true},
    {NULL, 0, 0}
};

const struct flag_type  portal_flags[] = {
    {"normal_exit", GATE_NORMAL_EXIT, true},
    {"no_curse", GATE_NOCURSE, true},
    {"go_with", GATE_GOWITH, true},
    {"buggy", GATE_BUGGY, true},
    {"random", GATE_RANDOM, true},
    {NULL, 0, 0}
};

const struct flag_type  furniture_flags[] = {
    {"stand_at", STAND_AT, true},
    {"stand_on", STAND_ON, true},
    {"stand_in", STAND_IN, true},
    {"sit_at", SIT_AT, true},
    {"sit_on", SIT_ON, true},
    {"sit_in", SIT_IN, true},
    {"rest_at", REST_AT, true},
    {"rest_on", REST_ON, true},
    {"rest_in", REST_IN, true},
    {"sleep_at", SLEEP_AT, true},
    {"sleep_on", SLEEP_ON, true},
    {"sleep_in", SLEEP_IN, true},
    {"put_at", PUT_AT, true},
    {"put_on", PUT_ON, true},
    {"put_in", PUT_IN, true},
    {"put_inside", PUT_INSIDE, true},
    {NULL, 0, 0}
};

const struct flag_type  apply_types[] = {
    {"affects", TO_AFFECTS, true},
    {"object", TO_OBJECT, true},
    {"immune", TO_IMMUNE, true},
    {"resist", TO_RESIST, true},
    {"vuln", TO_VULN, true},
    {"weapon", TO_WEAPON, true},
    {NULL, 0, true}
};

/*****************************************************************************
 * Name:        flag_stat_table                                              *
 * Purpose:     This table catagorizes the tables following the lookup       *
 *              functions below into stats and flags.  Flags can be toggled  *
 *              but stats can only be assigned.  Update this table when a    *
 *              new set of flags is installed.                               *
 ****************************************************************************/
const struct flag_stat_type flag_stat_table[] = {
/*  {structure, stat}, */
    {area_flags, false},
    {sex_flags, true},
    {exit_flags, false},
    {door_resets, true},
    {room_flags, false},
    {sector_flags, true},
    {type_flags, true},
    {extra_flags, false},
    {wear_flags, false},
    {act_flags, false},
    {affect_flags, false},
    {apply_flags, true},
    {wear_loc_flags, true},
    {wear_loc_strings, true},
    {container_flags, false},

/* ROM specific flags: */

    {form_flags, false},
    {part_flags, false},
    {ac_type, true},
    {size_flags, true},
    {position_flags, true},
    {off_flags, false},
    {imm_flags, false},
    {res_flags, false},
    {vuln_flags, false},
    {weapon_class, true},
    {weapon_type2, false},
    {apply_types, true},
    {0, 0}
};

const struct bit_type   bitvector_type[] = {
    {affect_flags, "affect"},
    {apply_flags, "apply"},
    {imm_flags, "imm"},
    {res_flags, "res"},
    {vuln_flags, "vuln"},
    {weapon_type2, "weapon"}
};

int flag_lookup( const char *name, const struct flag_type *flag_table )
{
    int                     flag = 0;

    for ( flag = 0; flag_table[flag].name != NULL; flag++ )
    {
        if ( LOWER( name[0] ) == LOWER( flag_table[flag].name[0] )
             && !str_prefix( name, flag_table[flag].name ) )
            return flag_table[flag].bit;
    }
    return NO_FLAG;
}

void do_flag( CHAR_DATA *ch, const char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg2[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    arg3[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    char                    word[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    CHAR_DATA              *victim = NULL;
    int                    *flag = NULL;
    int                     old = 0;
    int                     inew = 0;
    int                     marked = 0;
    int                     pos = 0;
    char                    type = '\0';
    const struct flag_type *flag_table = NULL;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    type = argument[0];

    if ( type == '=' || type == '-' || type == '+' )
        argument = one_argument( argument, word );

    if ( arg1[0] == '\0' )
    {
        ch_printf( ch, "Syntax:\r\n" );
        ch_printf( ch, "  flag mob  <name> <field> <flags>\r\n" );
        ch_printf( ch, "  flag char <name> <field> <flags>\r\n" );
        ch_printf( ch, "  mob  flags: act,aff,off,imm,res,vuln,form,part\r\n" );
        ch_printf( ch, "  char flags: plr,comm,aff,imm,res,vuln,\r\n" );
        ch_printf( ch, "  +: add flag, -: remove flag, = set equal to\r\n" );
        ch_printf( ch, "  otherwise flag toggles the flags listed.\r\n" );
        return;
    }

    if ( arg2[0] == '\0' )
    {
        ch_printf( ch, "What do you wish to set flags on?\r\n" );
        return;
    }

    if ( arg3[0] == '\0' )
    {
        ch_printf( ch, "You need to specify a flag to set.\r\n" );
        return;
    }

    if ( argument[0] == '\0' )
    {
        ch_printf( ch, "Which flags do you wish to change?\r\n" );
        return;
    }

    if ( !str_prefix( arg1, "mob" ) || !str_prefix( arg1, "char" ) )
    {
        victim = get_char_world( ch, arg2 );
        if ( victim == NULL )
        {
            ch_printf( ch, "You can't find them.\r\n" );
            return;
        }

        /*
         * select a flag to set 
         */
        if ( !str_prefix( arg3, "act" ) )
        {
            if ( !IS_NPC( victim ) )
            {
                ch_printf( ch, "Use plr for PCs.\r\n" );
                return;
            }

            flag = &victim->act;
            flag_table = act_flags;
        }

        else if ( !str_prefix( arg3, "plr" ) )
        {
            if ( IS_NPC( victim ) )
            {
                ch_printf( ch, "Use act for NPCs.\r\n" );
                return;
            }

            flag = &victim->act;
            flag_table = plr_flags;
        }

        else if ( !str_prefix( arg3, "aff" ) )
        {
            flag = &victim->affected_by;
            flag_table = affect_flags;
        }

        else if ( !str_prefix( arg3, "immunity" ) )
        {
            flag = &victim->imm_flags;
            flag_table = imm_flags;
        }

        else if ( !str_prefix( arg3, "resist" ) )
        {
            flag = &victim->res_flags;
            flag_table = imm_flags;
        }

        else if ( !str_prefix( arg3, "vuln" ) )
        {
            flag = &victim->vuln_flags;
            flag_table = imm_flags;
        }

        else if ( !str_prefix( arg3, "form" ) )
        {
            if ( !IS_NPC( victim ) )
            {
                ch_printf( ch, "Form can't be set on PCs.\r\n" );
                return;
            }

            flag = &victim->form;
            flag_table = form_flags;
        }

        else if ( !str_prefix( arg3, "parts" ) )
        {
            if ( !IS_NPC( victim ) )
            {
                ch_printf( ch, "Parts can't be set on PCs.\r\n" );
                return;
            }

            flag = &victim->parts;
            flag_table = part_flags;
        }

        else if ( !str_prefix( arg3, "comm" ) )
        {
            if ( IS_NPC( victim ) )
            {
                ch_printf( ch, "Comm can't be set on NPCs.\r\n" );
                return;
            }

            flag = &victim->comm;
            flag_table = comm_flags;
        }

        else
        {
            ch_printf( ch, "That's not an acceptable flag.\r\n" );
            return;
        }

        old = *flag;
        victim->zone = NULL;

        if ( type != '=' )
            inew = old;

        /*
         * mark the words 
         */
        for ( ;; )
        {
            argument = one_argument( argument, word );

            if ( word[0] == '\0' )
                break;

            pos = flag_lookup( word, flag_table );
            if ( pos == NO_FLAG )
            {
                ch_printf( ch, "That flag doesn't exist!\r\n" );
                return;
            }
            else
                SET_BIT( marked, pos );
        }

        for ( pos = 0; flag_table[pos].name != NULL; pos++ )
        {
            if ( !flag_table[pos].settable && IS_SET( old, flag_table[pos].bit ) )
            {
                SET_BIT( inew, flag_table[pos].bit );
                continue;
            }

            if ( IS_SET( marked, flag_table[pos].bit ) )
            {
                switch ( type )
                {
                    case '=':
                    case '+':
                        SET_BIT( inew, flag_table[pos].bit );
                        break;
                    case '-':
                        REMOVE_BIT( inew, flag_table[pos].bit );
                        break;
                    default:
                        if ( IS_SET( inew, flag_table[pos].bit ) )
                            REMOVE_BIT( inew, flag_table[pos].bit );
                        else
                            SET_BIT( inew, flag_table[pos].bit );
                }
            }
        }
        *flag = inew;
        return;
    }
}

int clan_lookup( const char *name )
{
    int                     clan = 0;

    for ( clan = 0; clan < MAX_CLAN; clan++ )
    {
        if ( LOWER( name[0] ) == LOWER( clan_table[clan].name[0] )
             && !str_prefix( name, clan_table[clan].name ) )
            return clan;
    }

    return 0;
}

int position_lookup( const char *name )
{
    int                     pos = 0;

    for ( pos = 0; position_table[pos].name != NULL; pos++ )
    {
        if ( LOWER( name[0] ) == LOWER( position_table[pos].name[0] )
             && !str_prefix( name, position_table[pos].name ) )
            return pos;
    }

    return -1;
}

int sex_lookup( const char *name )
{
    int                     sex = 0;

    for ( sex = 0; sex_table[sex].name != NULL; sex++ )
    {
        if ( LOWER( name[0] ) == LOWER( sex_table[sex].name[0] )
             && !str_prefix( name, sex_table[sex].name ) )
            return sex;
    }

    return -1;
}

int size_lookup( const char *name )
{
    int                     size = 0;

    for ( size = 0; size_table[size].name != NULL; size++ )
    {
        if ( LOWER( name[0] ) == LOWER( size_table[size].name[0] )
             && !str_prefix( name, size_table[size].name ) )
            return size;
    }

    return -1;
}

/* returns race number */
int race_lookup( const char *name )
{
    int                     race;

    for ( race = 0; race_table[race].name != NULL; race++ )
    {
        if ( LOWER( name[0] ) == LOWER( race_table[race].name[0] )
             && !str_prefix( name, race_table[race].name ) )
            return race;
    }

    return 0;
}

int item_lookup( const char *name )
{
    int                     type;

    for ( type = 0; item_table[type].name != NULL; type++ )
    {
        if ( LOWER( name[0] ) == LOWER( item_table[type].name[0] )
             && !str_prefix( name, item_table[type].name ) )
            return item_table[type].type;
    }

    return -1;
}

int liq_lookup( const char *name )
{
    int                     liq;

    for ( liq = 0; liq_table[liq].liq_name != NULL; liq++ )
    {
        if ( LOWER( name[0] ) == LOWER( liq_table[liq].liq_name[0] )
             && !str_prefix( name, liq_table[liq].liq_name ) )
            return liq;
    }

    return -1;
}

HELP_DATA              *help_lookup( const char *keyword )
{
    HELP_DATA              *pHelp;
    char                    temp[MAX_INPUT_LENGTH],
                            argall[MAX_INPUT_LENGTH];

    argall[0] = '\0';

    while ( keyword[0] != '\0' )
    {
        keyword = one_argument( keyword, temp );
        if ( argall[0] != '\0' )
            strcat( argall, " " );
        strcat( argall, temp );
    }

    for ( pHelp = help_first; pHelp != NULL; pHelp = pHelp->next )
        if ( is_name( argall, pHelp->keyword ) )
            return pHelp;

    return NULL;
}

HELP_AREA              *had_lookup( const char *arg )
{
    HELP_AREA              *temp;

    for ( temp = had_list; temp; temp = temp->next )
        if ( !str_cmp( arg, temp->filename ) )
            return temp;

    return NULL;
}

/*****************************************************************************
 * Name:        is_stat( table )                                             *
 * Purpose:     Returns true if the table is a stat table and false if flag. *
 * Called by:   flag_value and flag_string.                                  *
 * Note:        This function is local and used only in bit.c.               *
 ****************************************************************************/
bool is_stat( const struct flag_type *flag_table )
{
    int                     flag = 0;

    for ( flag = 0; flag_stat_table[flag].structure; flag++ )
    {
        if ( flag_stat_table[flag].structure == flag_table && flag_stat_table[flag].stat )
            return true;
    }
    return false;
}

/*****************************************************************************
 * Name:        flag_value( table, flag )                                    *
 * Purpose:     Returns the value of the flags entered. Multi-flags accepted.*
 * Called by:   olc.c and olc_act.c.                                         *
 ****************************************************************************/
int flag_value( const struct flag_type *flag_table, const char *argument )
{
    char                    word[MAX_INPUT_LENGTH] = "\0\0\0\0\0\0\0";
    int                     bit = 0;
    int                     marked = 0;
    bool                    found = false;

    if ( is_stat( flag_table ) )
        return flag_lookup( argument, flag_table );

    /*
     * Accept multiple flags.
     */
    for ( ;; )
    {
        argument = one_argument( argument, word );

        if ( word[0] == '\0' )
            break;

        if ( ( bit = flag_lookup( word, flag_table ) ) != NO_FLAG )
        {
            SET_BIT( marked, bit );
            found = true;
        }
    }

    if ( found )
        return marked;
    else
        return NO_FLAG;
}

/*****************************************************************************
 * Name:        flag_string( table, flags/stat )                             *
 * Purpose:     Returns string with name(s) of the flags or stat entered.    *
 * Called by:   act_olc.c, olc.c, and olc_save.c.                            *
 ****************************************************************************/
const char             *flag_string( const struct flag_type *flag_table, int bits )
{
    static char             buf[2][512];
    static int              cnt = 0;
    int                     flag = 0;

    if ( ++cnt > 1 )
        cnt = 0;

    buf[cnt][0] = '\0';

    for ( flag = 0; flag_table[flag].name != NULL; flag++ )
    {
        if ( !is_stat( flag_table ) && IS_SET( bits, flag_table[flag].bit ) )
        {
            strcat( buf[cnt], " " );
            strcat( buf[cnt], flag_table[flag].name );
        }
        else if ( flag_table[flag].bit == bits )
        {
            strcat( buf[cnt], " " );
            strcat( buf[cnt], flag_table[flag].name );
            break;
        }
    }
    return ( buf[cnt][0] != '\0' ) ? buf[cnt] + 1 : "none";
}
