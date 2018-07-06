/*
 * RAM $Id: merc.h 70 2009-01-11 18:47:35Z quixadhal $
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

/*
 * LEGACY:
 * These things are here to make snippet integration easier.
 * Please try to make them go away in new code!
 */
#define args( list )                    list
#define DECLARE_DO_FUN( fun )           DO_FUN    fun
#define DECLARE_SPEC_FUN( fun )         SPEC_FUN  fun
#define DECLARE_SPELL_FUN( fun )        SPELL_FUN fun

typedef short int sh_int;

/*
 * Accomodate both gcc and g++
 */
#if !defined __cplusplus

typedef unsigned char bool;

#define TRUE         1
#define true         TRUE
#define FALSE        0
#define false        FALSE

#else

#define TRUE         true
#define FALSE        false

#endif

/*
 * Structure types.
 */
typedef struct affect_data AFFECT_DATA;
typedef struct area_data AREA_DATA;
typedef struct buf_type BUFFER;
typedef struct char_data CHAR_DATA;
typedef struct descriptor_data DESCRIPTOR_DATA;
typedef struct exit_data EXIT_DATA;
typedef struct extra_descr_data EXTRA_DESCR_DATA;
typedef struct help_data HELP_DATA;
typedef struct help_area_data HELP_AREA;
typedef struct kill_data KILL_DATA;
typedef struct mem_data MEM_DATA;
typedef struct mob_index_data MOB_INDEX_DATA;
typedef struct note_data NOTE_DATA;
typedef struct obj_data OBJ_DATA;
typedef struct obj_index_data OBJ_INDEX_DATA;
typedef struct pc_data PC_DATA;
typedef struct gen_data GEN_DATA;
typedef struct reset_data RESET_DATA;
typedef struct room_index_data ROOM_INDEX_DATA;
typedef struct shop_data SHOP_DATA;
typedef struct time_info_data TIME_INFO_DATA;
typedef struct weather_data WEATHER_DATA;
typedef struct mprog_list MPROG_LIST;
typedef struct mprog_code MPROG_CODE;

/*
 * Function types.
 */
typedef void DO_FUN     ( CHAR_DATA *ch, const char *argument );
typedef bool SPEC_FUN   ( CHAR_DATA *ch );
typedef void SPELL_FUN  ( int sn, int level, CHAR_DATA *ch, void *vo, int target );

/*
 * String and memory management parameters.
 */
#define MAX_KEY_HASH                                1024
#define MAX_STRING_LENGTH                           4608
#define MAX_INPUT_LENGTH                             256
#define PAGELEN                                       22

/*
 * Game parameters.
 * Increase the max'es if you add more of something.
 * Adjust the pulse numbers to suit yourself.
 */
#define MAX_SOCIALS                                  256
#define MAX_SKILL                                    150
#define MAX_GROUP                                     30
#define MAX_IN_GROUP                                  15
#define MAX_ALIAS                                      5
#define MAX_CLASS                                      4
#define MAX_PC_RACE                                    5
#define MAX_CLAN                                       3
#define MAX_DAMAGE_MESSAGE                            41
#define MAX_LEVEL                                     60
#define LEVEL_HERO                       (MAX_LEVEL - 9)
#define LEVEL_IMMORTAL                   (MAX_LEVEL - 8)

#define PULSE_PER_SECOND           4
#define PULSE_VIOLENCE           ( 3 * PULSE_PER_SECOND)
#define PULSE_MOBILE             ( 4 * PULSE_PER_SECOND)
#define PULSE_TICK               (60 * PULSE_PER_SECOND)
#define PULSE_AREA              (120 * PULSE_PER_SECOND)

#define IMPLEMENTOR                            MAX_LEVEL
#define CREATOR                          (MAX_LEVEL - 1)
#define SUPREME                          (MAX_LEVEL - 2)
#define DEITY                            (MAX_LEVEL - 3)
#define GOD                              (MAX_LEVEL - 4)
#define IMMORTAL                         (MAX_LEVEL - 5)
#define DEMI                             (MAX_LEVEL - 6)
#define ANGEL                            (MAX_LEVEL - 7)
#define AVATAR                           (MAX_LEVEL - 8)
#define HERO                                  LEVEL_HERO

struct buf_type
{
    BUFFER                 *next;
    bool                    valid;
    int                     state;                     /* error state of the buffer */
    int                     size;                      /* size in k */
    char                   *string;                    /* buffer's string */
};

/*
 * Time and weather stuff.
 */
#define SUN_DARK                 0
#define SUN_RISE                 1
#define SUN_LIGHT                2
#define SUN_SET                  3

#define SKY_CLOUDLESS            0
#define SKY_CLOUDY               1
#define SKY_RAINING              2
#define SKY_LIGHTNING            3

struct time_info_data
{
    int                     hour;
    int                     day;
    int                     month;
    int                     year;
};

struct weather_data
{
    int                     mmhg;
    int                     change;
    int                     sky;
    int                     sunlight;
};

/*
 * Connected state for a channel.
 */
#define CON_PLAYING               0
#define CON_GET_NAME              1
#define CON_GET_OLD_PASSWORD      2
#define CON_CONFIRM_NEW_NAME      3
#define CON_GET_NEW_PASSWORD      4
#define CON_CONFIRM_NEW_PASSWORD  5
#define CON_GET_NEW_RACE          6
#define CON_GET_NEW_SEX           7
#define CON_GET_NEW_CLASS         8
#define CON_GET_ALIGNMENT         9
#define CON_DEFAULT_CHOICE       10
#define CON_GEN_GROUPS           11
#define CON_PICK_WEAPON          12
#define CON_READ_IMOTD           13
#define CON_READ_MOTD            14
#define CON_BREAK_CONNECT        15

/*
 * Descriptor (channel) structure.
 */
struct descriptor_data
{
    DESCRIPTOR_DATA        *next;
    DESCRIPTOR_DATA        *snoop_by;
    CHAR_DATA              *character;
    CHAR_DATA              *original;
    bool                    valid;
    char                   *host;
    int                     socket;
    int                     connected;
    bool                    fcommand;
    char                    inbuf[4 * MAX_INPUT_LENGTH];
    char                    incomm[MAX_INPUT_LENGTH];
    char                    inlast[MAX_INPUT_LENGTH];
    int                     repeat;
    char                   *outbuf;
    int                     outsize;
    int                     outtop;
    char                   *showstr_head;
    char                   *showstr_point;
    void                   *pEdit;                     /* OLC */
    char                  **pString;                   /* OLC */
    int                     editor;                    /* OLC */
};

/*
 * Attribute bonus structures.
 */
struct str_app_type
{
    int                     tohit;
    int                     todam;
    int                     carry;
    int                     wield;
};

struct int_app_type
{
    int                     learn;
};

struct wis_app_type
{
    int                     practice;
};

struct dex_app_type
{
    int                     defensive;
};

struct con_app_type
{
    int                     hitp;
    int                     shock;
};

/*
 * TO types for act.
 */
#define TO_ROOM             0
#define TO_NOTVICT          1
#define TO_VICT             2
#define TO_CHAR             3
#define TO_ALL              4

/*
 * Help table types.
 */
struct help_data
{
    HELP_DATA              *next;
    HELP_DATA              *next_area;
    int                     level;
    char                   *keyword;
    char                   *text;
};

struct help_area_data
{
    HELP_AREA              *next;
    HELP_DATA              *first;
    HELP_DATA              *last;
    AREA_DATA              *area;
    char                   *filename;
    bool                    changed;
};

/*
 * Shop types.
 */
#define MAX_TRADE           5

struct shop_data
{
    SHOP_DATA              *next;                      /* Next shop in list */
    int                     keeper;                    /* Vnum of shop keeper mob */
    int                     buy_type[MAX_TRADE];       /* Item types shop will buy */
    int                     profit_buy;                /* Cost multiplier for buying */
    int                     profit_sell;               /* Cost multiplier for selling */
    int                     open_hour;                 /* First opening hour */
    int                     close_hour;                /* First closing hour */
};

/*
 * Per-class stuff.
 */

#define MAX_GUILD           2
#define MAX_STATS           5
#define STAT_STR            0
#define STAT_INT            1
#define STAT_WIS            2
#define STAT_DEX            3
#define STAT_CON            4

struct class_type
{
    const char             *name;                      /* the full name of the class */
    const char              who_name[4];               /* Three-letter name for 'who' */
    int                     attr_prime;                /* Prime attribute */
    int                     weapon;                    /* First weapon */
    int                     guild[MAX_GUILD];          /* Vnum of guild rooms */
    int                     skill_adept;               /* Maximum skill level */
    int                     thac0_00;                  /* Thac0 for level 0 */
    int                     thac0_32;                  /* Thac0 for level 32 */
    int                     hp_min;                    /* Min hp gained on leveling */
    int                     hp_max;                    /* Max hp gained on leveling */
    bool                    fMana;                     /* Class gains mana on level */
    const char             *base_group;                /* base skills gained */
    const char             *default_group;             /* default skills gained */
};

struct item_type
{
    int                     type;
    const char             *name;
};

struct weapon_type
{
    const char             *name;
    int                     vnum;
    int                     type;
    const char             *skill;
};

struct wiznet_type
{
    const char             *name;
    int                     flag;
    int                     level;
};

struct attack_type
{
    const char             *name;                      /* name */
    const char             *noun;                      /* message */
    int                     damage;                    /* damage class */
};

struct race_type
{
    const char             *name;                      /* call name of the race */
    bool                    pc_race;                   /* can be chosen by pcs */
    int                     act;                       /* act bits for the race */
    int                     aff;                       /* aff bits for the race */
    int                     off;                       /* off bits for the race */
    int                     imm;                       /* imm bits for the race */
    int                     res;                       /* res bits for the race */
    int                     vuln;                      /* vuln bits for the race */
    int                     form;                      /* default form flag for the race */
    int                     parts;                     /* default parts for the race */
};

struct pc_race_type                                    /* additional data for pc races */
{
    const char             *name;                      /* MUST be in race_type */
    const char              who_name[6];
    int                     points;                    /* cost in points of the race */
    int                     class_mult[MAX_CLASS];     /* exp multiplier for class, * 100 
                                                        */
    const char             *skills[5];                 /* bonus skills for the race */
    int                     stats[MAX_STATS];          /* starting stats */
    int                     max_stats[MAX_STATS];      /* maximum stats */
    int                     size;                      /* aff bits for the race */
};

/*
 * Data structure for notes.
 */

#define NOTE_NOTE           0
#define NOTE_IDEA           1
#define NOTE_PENALTY        2
#define NOTE_NEWS           3
#define NOTE_CHANGES        4
struct note_data
{
    NOTE_DATA              *next;
    bool                    valid;
    int                     type;
    char                   *sender;
    char                   *date;
    char                   *to_list;
    char                   *subject;
    char                   *text;
    time_t                  date_stamp;
};

/*
 * An affect.
 */
struct affect_data
{
    AFFECT_DATA            *next;
    bool                    valid;
    int                     where;
    int                     type;
    int                     level;
    int                     duration;
    int                     location;
    int                     modifier;
    int                     bitvector;
};

/* where definitions */
#define TO_AFFECTS          0
#define TO_OBJECT           1
#define TO_IMMUNE           2
#define TO_RESIST           3
#define TO_VULN             4
#define TO_WEAPON           5

/*
 * A kill structure (indexed by level).
 */
struct kill_data
{
    int                     number;
    int                     killed;
};

/*
 * This is for the list of ALL players
 */
struct player_list
{
    char                   *name;
    time_t                  date_created;
    time_t                  last_login;
    time_t                  time_played;
    int                     login_count;
    int                     level;
    int                     iRace;
    int                     iClass;
    CHAR_DATA              *ch;
};

#define CREATE( result, type, number )                                  \
do                                                                      \
{                                                                       \
  if ( !( ( result ) = ( type * ) calloc( ( number ),                   \
                                          sizeof( type ) ) ) )          \
  {                                                                     \
    proper_exit( MUD_HALT, "calloc failure @ %s:%d",                    \
                 __FILE__, __LINE__ );                                  \
  }                                                                     \
} while( 0 )

#define RECREATE( result, type, number )                                \
do                                                                      \
{                                                                       \
  if ( !( ( result ) = ( type * ) realloc( ( result ),                  \
                                           sizeof( type )               \
                                           * ( number ) ) ) )           \
  {                                                                     \
    proper_exit( MUD_HALT, "realloc failure @ %s:%d",                   \
                 __FILE__, __LINE__ );                                  \
  }                                                                     \
} while( 0 )

#define DESTROY( point )                                                \
do                                                                      \
{                                                                       \
  if ( ( point ) )                                                      \
  {                                                                     \
    free( ( point ) );                                                  \
    ( point ) = NULL;                                                   \
  }                                                                     \
} while( 0 )

/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (Start of section ... start here)                     *
 *                                                                         *
 ***************************************************************************/

/*
 * Well known mob virtual numbers.
 * Defined in #MOBILES.
 */
#define MOB_VNUM_FIDO       3090
#define MOB_VNUM_CITYGUARD  3060
#define MOB_VNUM_VAMPIRE    3404

#define MOB_VNUM_PATROLMAN  2106
#define GROUP_VNUM_TROLLS   2100
#define GROUP_VNUM_OGRES    2101

/* RT ASCII conversions -- used so we can have letters in this file */

#define A                           1
#define B                           2
#define C                           4
#define D                           8
#define E                          16
#define F                          32
#define G                          64
#define H                         128

#define I                         256
#define J                         512
#define K                        1024
#define L                        2048
#define M                        4096
#define N                        8192
#define O                       16384
#define P                       32768

#define Q                       65536
#define R                      131072
#define S                      262144
#define T                      524288
#define U                     1048576
#define V                     2097152
#define W                     4194304
#define X                     8388608

#define Y                    16777216
#define Z                    33554432
#define aa                   67108864                  /* doubled due to conflicts */
#define bb                  134217728
#define cc                  268435456
#define dd                  536870912
#define ee                 1073741824

/*
 * ACT bits for mobs.
 * Used in #MOBILES.
 */
#define ACT_IS_NPC                (A)                  /* Auto set for mobs */
#define ACT_SENTINEL              (B)                  /* Stays in one room */
#define ACT_SCAVENGER             (C)                  /* Picks up objects */
#define ACT_AGGRESSIVE            (F)                  /* Attacks PC's */
#define ACT_STAY_AREA             (G)                  /* Won't leave area */
#define ACT_WIMPY                 (H)
#define ACT_PET                   (I)                  /* Auto set for pets */
#define ACT_TRAIN                 (J)                  /* Can train PC's */
#define ACT_PRACTICE              (K)                  /* Can practice PC's */
#define ACT_UNDEAD                (O)
#define ACT_CLERIC                (Q)
#define ACT_MAGE                  (R)
#define ACT_THIEF                 (S)
#define ACT_WARRIOR               (T)
#define ACT_NOALIGN               (U)
#define ACT_NOPURGE               (V)
#define ACT_OUTDOORS              (W)
#define ACT_INDOORS               (Y)
#define ACT_IS_HEALER            (aa)
#define ACT_GAIN                 (bb)
#define ACT_UPDATE_ALWAYS        (cc)
#define ACT_IS_CHANGER           (dd)

/* damage classes */
#define DAM_NONE                    0
#define DAM_BASH                    1
#define DAM_PIERCE                  2
#define DAM_SLASH                   3
#define DAM_FIRE                    4
#define DAM_COLD                    5
#define DAM_LIGHTNING               6
#define DAM_ACID                    7
#define DAM_POISON                  8
#define DAM_NEGATIVE                9
#define DAM_HOLY                   10
#define DAM_ENERGY                 11
#define DAM_MENTAL                 12
#define DAM_DISEASE                13
#define DAM_DROWNING               14
#define DAM_LIGHT                  15
#define DAM_OTHER                  16
#define DAM_HARM                   17
#define DAM_CHARM                  18
#define DAM_SOUND                  19

/* OFF bits for mobiles */
#define OFF_AREA_ATTACK           (A)
#define OFF_BACKSTAB              (B)
#define OFF_BASH                  (C)
#define OFF_BERSERK               (D)
#define OFF_DISARM                (E)
#define OFF_DODGE                 (F)
#define OFF_FADE                  (G)
#define OFF_FAST                  (H)
#define OFF_KICK                  (I)
#define OFF_KICK_DIRT             (J)
#define OFF_PARRY                 (K)
#define OFF_RESCUE                (L)
#define OFF_TAIL                  (M)
#define OFF_TRIP                  (N)
#define OFF_CRUSH                 (O)
#define ASSIST_ALL                (P)
#define ASSIST_ALIGN              (Q)
#define ASSIST_RACE               (R)
#define ASSIST_PLAYERS            (S)
#define ASSIST_GUARD              (T)
#define ASSIST_VNUM               (U)

/* return values for check_imm */
#define IS_NORMAL                   0
#define IS_IMMUNE                   1
#define IS_RESISTANT                2
#define IS_VULNERABLE               3

/* IMM bits for mobs */
#define IMM_SUMMON                (A)
#define IMM_CHARM                 (B)
#define IMM_MAGIC                 (C)
#define IMM_WEAPON                (D)
#define IMM_BASH                  (E)
#define IMM_PIERCE                (F)
#define IMM_SLASH                 (G)
#define IMM_FIRE                  (H)
#define IMM_COLD                  (I)
#define IMM_LIGHTNING             (J)
#define IMM_ACID                  (K)
#define IMM_POISON                (L)
#define IMM_NEGATIVE              (M)
#define IMM_HOLY                  (N)
#define IMM_ENERGY                (O)
#define IMM_MENTAL                (P)
#define IMM_DISEASE               (Q)
#define IMM_DROWNING              (R)
#define IMM_LIGHT                 (S)
#define IMM_SOUND                 (T)
#define IMM_WOOD                  (X)
#define IMM_SILVER                (Y)
#define IMM_IRON                  (Z)

/* RES bits for mobs */
#define RES_SUMMON                (A)
#define RES_CHARM                 (B)
#define RES_MAGIC                 (C)
#define RES_WEAPON                (D)
#define RES_BASH                  (E)
#define RES_PIERCE                (F)
#define RES_SLASH                 (G)
#define RES_FIRE                  (H)
#define RES_COLD                  (I)
#define RES_LIGHTNING             (J)
#define RES_ACID                  (K)
#define RES_POISON                (L)
#define RES_NEGATIVE              (M)
#define RES_HOLY                  (N)
#define RES_ENERGY                (O)
#define RES_MENTAL                (P)
#define RES_DISEASE               (Q)
#define RES_DROWNING              (R)
#define RES_LIGHT                 (S)
#define RES_SOUND                 (T)
#define RES_WOOD                  (X)
#define RES_SILVER                (Y)
#define RES_IRON                  (Z)

/* VULN bits for mobs */
#define VULN_SUMMON               (A)
#define VULN_CHARM                (B)
#define VULN_MAGIC                (C)
#define VULN_WEAPON               (D)
#define VULN_BASH                 (E)
#define VULN_PIERCE               (F)
#define VULN_SLASH                (G)
#define VULN_FIRE                 (H)
#define VULN_COLD                 (I)
#define VULN_LIGHTNING            (J)
#define VULN_ACID                 (K)
#define VULN_POISON               (L)
#define VULN_NEGATIVE             (M)
#define VULN_HOLY                 (N)
#define VULN_ENERGY               (O)
#define VULN_MENTAL               (P)
#define VULN_DISEASE              (Q)
#define VULN_DROWNING             (R)
#define VULN_LIGHT                (S)
#define VULN_SOUND                (T)
#define VULN_WOOD                 (X)
#define VULN_SILVER               (Y)
#define VULN_IRON                 (Z)

/* body form */
#define FORM_EDIBLE               (A)
#define FORM_POISON               (B)
#define FORM_MAGICAL              (C)
#define FORM_INSTANT_DECAY        (D)
#define FORM_OTHER                (E)                  /* defined by material bit */

/* actual form */
#define FORM_ANIMAL               (G)
#define FORM_SENTIENT             (H)
#define FORM_UNDEAD               (I)
#define FORM_CONSTRUCT            (J)
#define FORM_MIST                 (K)
#define FORM_INTANGIBLE           (L)

#define FORM_BIPED                (M)
#define FORM_CENTAUR              (N)
#define FORM_INSECT               (O)
#define FORM_SPIDER               (P)
#define FORM_CRUSTACEAN           (Q)
#define FORM_WORM                 (R)
#define FORM_BLOB                 (S)

#define FORM_MAMMAL               (V)
#define FORM_BIRD                 (W)
#define FORM_REPTILE              (X)
#define FORM_SNAKE                (Y)
#define FORM_DRAGON               (Z)
#define FORM_AMPHIBIAN           (aa)
#define FORM_FISH                (bb)
#define FORM_COLD_BLOOD          (cc)

/* body parts */
#define PART_HEAD                 (A)
#define PART_ARMS                 (B)
#define PART_LEGS                 (C)
#define PART_HEART                (D)
#define PART_BRAINS               (E)
#define PART_GUTS                 (F)
#define PART_HANDS                (G)
#define PART_FEET                 (H)
#define PART_FINGERS              (I)
#define PART_EAR                  (J)
#define PART_EYE                  (K)
#define PART_LONG_TONGUE          (L)
#define PART_EYESTALKS            (M)
#define PART_TENTACLES            (N)
#define PART_FINS                 (O)
#define PART_WINGS                (P)
#define PART_TAIL                 (Q)
/* for combat */
#define PART_CLAWS                (U)
#define PART_FANGS                (V)
#define PART_HORNS                (W)
#define PART_SCALES               (X)
#define PART_TUSKS                (Y)

/*
 * Bits for 'affected_by'.
 * Used in #MOBILES.
 */
#define AFF_BLIND                 (A)
#define AFF_INVISIBLE             (B)
#define AFF_DETECT_EVIL           (C)
#define AFF_DETECT_INVISIBILITY   (D)
#define AFF_DETECT_MAGIC          (E)
#define AFF_DETECT_HIDDEN         (F)
#define AFF_DETECT_GOOD           (G)
#define AFF_SANCTUARY             (H)
#define AFF_FAERIE_FIRE           (I)
#define AFF_INFRARED              (J)
#define AFF_CURSE                 (K)
#define AFF_UNUSED_FLAG           (L)                  /* unused */
#define AFF_POISON                (M)
#define AFF_PROTECT_EVIL          (N)
#define AFF_PROTECT_GOOD          (O)
#define AFF_SNEAK                 (P)
#define AFF_HIDE                  (Q)
#define AFF_SLEEP                 (R)
#define AFF_CHARM                 (S)
#define AFF_FLYING                (T)
#define AFF_PASS_DOOR             (U)
#define AFF_HASTE                 (V)
#define AFF_CALM                  (W)
#define AFF_PLAGUE                (X)
#define AFF_WEAKEN                (Y)
#define AFF_DARK_VISION           (Z)
#define AFF_BERSERK              (aa)
#define AFF_SWIM                 (bb)
#define AFF_REGENERATION         (cc)
#define AFF_SLOW                 (dd)

/*
 * Sex.
 * Used in #MOBILES.
 */
#define SEX_NEUTRAL                 0
#define SEX_MALE                    1
#define SEX_FEMALE                  2

/* AC types */
#define AC_PIERCE                   0
#define AC_BASH                     1
#define AC_SLASH                    2
#define AC_EXOTIC                   3

/* dice */
#define DICE_NUMBER                 0
#define DICE_TYPE                   1
#define DICE_BONUS                  2

/* size */
#define SIZE_TINY                   0
#define SIZE_SMALL                  1
#define SIZE_MEDIUM                 2
#define SIZE_LARGE                  3
#define SIZE_HUGE                   4
#define SIZE_GIANT                  5

/*
 * Well known object virtual numbers.
 * Defined in #OBJECTS.
 */
#define OBJ_VNUM_SILVER_ONE         1
#define OBJ_VNUM_GOLD_ONE           2
#define OBJ_VNUM_GOLD_SOME          3
#define OBJ_VNUM_SILVER_SOME        4
#define OBJ_VNUM_COINS              5

#define OBJ_VNUM_CORPSE_NPC        10
#define OBJ_VNUM_CORPSE_PC         11
#define OBJ_VNUM_SEVERED_HEAD      12
#define OBJ_VNUM_TORN_HEART        13
#define OBJ_VNUM_SLICED_ARM        14
#define OBJ_VNUM_SLICED_LEG        15
#define OBJ_VNUM_GUTS              16
#define OBJ_VNUM_BRAINS            17

#define OBJ_VNUM_MUSHROOM          20
#define OBJ_VNUM_LIGHT_BALL        21
#define OBJ_VNUM_SPRING            22
#define OBJ_VNUM_DISC              23
#define OBJ_VNUM_PORTAL            25

#define OBJ_VNUM_ROSE            1001

#define OBJ_VNUM_PIT             3010

#define OBJ_VNUM_SCHOOL_MACE     3700
#define OBJ_VNUM_SCHOOL_DAGGER   3701
#define OBJ_VNUM_SCHOOL_SWORD    3702
#define OBJ_VNUM_SCHOOL_SPEAR    3717
#define OBJ_VNUM_SCHOOL_STAFF    3718
#define OBJ_VNUM_SCHOOL_AXE      3719
#define OBJ_VNUM_SCHOOL_FLAIL    3720
#define OBJ_VNUM_SCHOOL_WHIP     3721
#define OBJ_VNUM_SCHOOL_POLEARM  3722

#define OBJ_VNUM_SCHOOL_VEST     3703
#define OBJ_VNUM_SCHOOL_SHIELD   3704
#define OBJ_VNUM_SCHOOL_BANNER   3716
#define OBJ_VNUM_MAP             3162

#define OBJ_VNUM_WHISTLE         2116

/*
 * Item types.
 * Used in #OBJECTS.
 */
#define ITEM_LIGHT                  1
#define ITEM_SCROLL                 2
#define ITEM_WAND                   3
#define ITEM_STAFF                  4
#define ITEM_WEAPON                 5
#define ITEM_TREASURE               8
#define ITEM_ARMOR                  9
#define ITEM_POTION                10
#define ITEM_CLOTHING              11
#define ITEM_FURNITURE             12
#define ITEM_TRASH                 13
#define ITEM_CONTAINER             15
#define ITEM_DRINK_CON             17
#define ITEM_KEY                   18
#define ITEM_FOOD                  19
#define ITEM_MONEY                 20
#define ITEM_BOAT                  22
#define ITEM_CORPSE_NPC            23
#define ITEM_CORPSE_PC             24
#define ITEM_FOUNTAIN              25
#define ITEM_PILL                  26
#define ITEM_PROTECT               27
#define ITEM_MAP                   28
#define ITEM_PORTAL                29
#define ITEM_WARP_STONE            30
#define ITEM_ROOM_KEY              31
#define ITEM_GEM                   32
#define ITEM_JEWELRY               33

/*
 * Extra flags.
 * Used in #OBJECTS.
 */
#define ITEM_GLOW                 (A)
#define ITEM_HUM                  (B)
#define ITEM_DARK                 (C)
#define ITEM_LOCK                 (D)
#define ITEM_EVIL                 (E)
#define ITEM_INVIS                (F)
#define ITEM_MAGIC                (G)
#define ITEM_NODROP               (H)
#define ITEM_BLESS                (I)
#define ITEM_ANTI_GOOD            (J)
#define ITEM_ANTI_EVIL            (K)
#define ITEM_ANTI_NEUTRAL         (L)
#define ITEM_NOREMOVE             (M)
#define ITEM_INVENTORY            (N)
#define ITEM_NOPURGE              (O)
#define ITEM_ROT_DEATH            (P)
#define ITEM_VIS_DEATH            (Q)
#define ITEM_NONMETAL             (S)
#define ITEM_NOLOCATE             (T)
#define ITEM_MELT_DROP            (U)
#define ITEM_HAD_TIMER            (V)
#define ITEM_SELL_EXTRACT         (W)
#define ITEM_BURN_PROOF           (Y)
#define ITEM_NOUNCURSE            (Z)

/*
 * Wear flags.
 * Used in #OBJECTS.
 */
#define ITEM_TAKE                 (A)
#define ITEM_WEAR_FINGER          (B)
#define ITEM_WEAR_NECK            (C)
#define ITEM_WEAR_BODY            (D)
#define ITEM_WEAR_HEAD            (E)
#define ITEM_WEAR_LEGS            (F)
#define ITEM_WEAR_FEET            (G)
#define ITEM_WEAR_HANDS           (H)
#define ITEM_WEAR_ARMS            (I)
#define ITEM_WEAR_SHIELD          (J)
#define ITEM_WEAR_ABOUT           (K)
#define ITEM_WEAR_WAIST           (L)
#define ITEM_WEAR_WRIST           (M)
#define ITEM_WIELD                (N)
#define ITEM_HOLD                 (O)
#define ITEM_NO_SAC               (P)
#define ITEM_WEAR_FLOAT           (Q)

/* weapon class */
#define WEAPON_EXOTIC               0
#define WEAPON_SWORD                1
#define WEAPON_DAGGER               2
#define WEAPON_SPEAR                3
#define WEAPON_MACE                 4
#define WEAPON_AXE                  5
#define WEAPON_FLAIL                6
#define WEAPON_WHIP                 7
#define WEAPON_POLEARM              8

/* weapon types */
#define WEAPON_FLAMING            (A)
#define WEAPON_FROST              (B)
#define WEAPON_VAMPIRIC           (C)
#define WEAPON_SHARP              (D)
#define WEAPON_VORPAL             (E)
#define WEAPON_TWO_HANDS          (F)
#define WEAPON_SHOCKING           (G)
#define WEAPON_POISON             (H)

/* gate flags */
#define GATE_NORMAL_EXIT          (A)
#define GATE_NOCURSE              (B)
#define GATE_GOWITH               (C)
#define GATE_BUGGY                (D)
#define GATE_RANDOM               (E)

/* furniture flags */
#define STAND_AT                  (A)
#define STAND_ON                  (B)
#define STAND_IN                  (C)
#define SIT_AT                    (D)
#define SIT_ON                    (E)
#define SIT_IN                    (F)
#define REST_AT                   (G)
#define REST_ON                   (H)
#define REST_IN                   (I)
#define SLEEP_AT                  (J)
#define SLEEP_ON                  (K)
#define SLEEP_IN                  (L)
#define PUT_AT                    (M)
#define PUT_ON                    (N)
#define PUT_IN                    (O)
#define PUT_INSIDE                (P)

/*
 * Apply types (for affects).
 * Used in #OBJECTS.
 */
#define APPLY_NONE                  0
#define APPLY_STR                   1
#define APPLY_DEX                   2
#define APPLY_INT                   3
#define APPLY_WIS                   4
#define APPLY_CON                   5
#define APPLY_SEX                   6
#define APPLY_CLASS                 7
#define APPLY_LEVEL                 8
#define APPLY_AGE                   9
#define APPLY_HEIGHT               10
#define APPLY_WEIGHT               11
#define APPLY_MANA                 12
#define APPLY_HIT                  13
#define APPLY_MOVE                 14
#define APPLY_GOLD                 15
#define APPLY_EXP                  16
#define APPLY_AC                   17
#define APPLY_HITROLL              18
#define APPLY_DAMROLL              19
#define APPLY_SAVES                20
#define APPLY_SAVING_PARA          20
#define APPLY_SAVING_ROD           21
#define APPLY_SAVING_PETRI         22
#define APPLY_SAVING_BREATH        23
#define APPLY_SAVING_SPELL         24
#define APPLY_SPELL_AFFECT         25

/*
 * Values for containers (value[1]).
 * Used in #OBJECTS.
 */
#define CONT_CLOSEABLE              1
#define CONT_PICKPROOF              2
#define CONT_CLOSED                 4
#define CONT_LOCKED                 8
#define CONT_PUT_ON                16

/*
 * Well known room virtual numbers.
 * Defined in #ROOMS.
 */
#define ROOM_VNUM_LIMBO             2
#define ROOM_VNUM_CHAT           1200
#define ROOM_VNUM_TEMPLE         3001
#define ROOM_VNUM_ALTAR          3054
#define ROOM_VNUM_SCHOOL         3700
#define ROOM_VNUM_BALANCE        4500
#define ROOM_VNUM_CIRCLE         4400
#define ROOM_VNUM_DEMISE         4201
#define ROOM_VNUM_HONOR          4300

/*
 * Room flags.
 * Used in #ROOMS.
 */
#define ROOM_DARK                 (A)
#define ROOM_NO_MOB               (C)
#define ROOM_INDOORS              (D)

#define ROOM_PRIVATE              (J)
#define ROOM_SAFE                 (K)
#define ROOM_SOLITARY             (L)
#define ROOM_PET_SHOP             (M)
#define ROOM_NO_RECALL            (N)
#define ROOM_IMP_ONLY             (O)
#define ROOM_GODS_ONLY            (P)
#define ROOM_HEROES_ONLY          (Q)
#define ROOM_NEWBIES_ONLY         (R)
#define ROOM_LAW                  (S)
#define ROOM_NOWHERE              (T)

/*
 * Directions.
 * Used in #ROOMS.
 */
#define DIR_NORTH                   0
#define DIR_EAST                    1
#define DIR_SOUTH                   2
#define DIR_WEST                    3
#define DIR_UP                      4
#define DIR_DOWN                    5

/*
 * Exit flags.
 * Used in #ROOMS.
 */
#define EX_ISDOOR                 (A)
#define EX_CLOSED                 (B)
#define EX_LOCKED                 (C)
#define EX_PICKPROOF              (F)
#define EX_NOPASS                 (G)
#define EX_EASY                   (H)
#define EX_HARD                   (I)
#define EX_INFURIATING            (J)
#define EX_NOCLOSE                (K)
#define EX_NOLOCK                 (L)

/*
 * Sector types.
 * Used in #ROOMS.
 */
#define SECT_INSIDE                 0
#define SECT_CITY                   1
#define SECT_FIELD                  2
#define SECT_FOREST                 3
#define SECT_HILLS                  4
#define SECT_MOUNTAIN               5
#define SECT_WATER_SWIM             6
#define SECT_WATER_NOSWIM           7
#define SECT_UNUSED                 8
#define SECT_AIR                    9
#define SECT_DESERT                10
#define SECT_MAX                   11

/*
 * Equpiment wear locations.
 * Used in #RESETS.
 */
#define WEAR_NONE                  -1
#define WEAR_LIGHT                  0
#define WEAR_FINGER_L               1
#define WEAR_FINGER_R               2
#define WEAR_NECK_1                 3
#define WEAR_NECK_2                 4
#define WEAR_BODY                   5
#define WEAR_HEAD                   6
#define WEAR_LEGS                   7
#define WEAR_FEET                   8
#define WEAR_HANDS                  9
#define WEAR_ARMS                  10
#define WEAR_SHIELD                11
#define WEAR_ABOUT                 12
#define WEAR_WAIST                 13
#define WEAR_WRIST_L               14
#define WEAR_WRIST_R               15
#define WEAR_WIELD                 16
#define WEAR_HOLD                  17
#define WEAR_FLOAT                 18
#define MAX_WEAR                   19

/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (End of this section ... stop here)                   *
 *                                                                         *
 ***************************************************************************/

/*
 * Conditions.
 */
#define COND_DRUNK                  0
#define COND_FULL                   1
#define COND_THIRST                 2
#define COND_HUNGER                 3

/*
 * Positions.
 */
#define POS_DEAD                    0
#define POS_MORTAL                  1
#define POS_INCAP                   2
#define POS_STUNNED                 3
#define POS_SLEEPING                4
#define POS_RESTING                 5
#define POS_SITTING                 6
#define POS_FIGHTING                7
#define POS_STANDING                8

/*
 * ACT bits for players.
 */
#define PLR_IS_NPC                (A)                  /* Don't EVER set. */

/* RT auto flags */
#define PLR_AUTOASSIST            (C)
#define PLR_AUTOEXIT              (D)
#define PLR_AUTOLOOT              (E)
#define PLR_AUTOSAC               (F)
#define PLR_AUTOGOLD              (G)
#define PLR_AUTOSPLIT             (H)

/* RT personal flags */
#define PLR_HOLYLIGHT             (N)
#define PLR_CANLOOT               (P)
#define PLR_NOSUMMON              (Q)
#define PLR_NOFOLLOW              (R)
/* 2 bits reserved, S-T */

/* penalty flags */
#define PLR_PERMIT                (U)
#define PLR_LOG                   (W)
#define PLR_DENY                  (X)
#define PLR_FREEZE                (Y)
#define PLR_THIEF                 (Z)
#define PLR_KILLER               (aa)

/* RT comm flags -- may be used on both mobs and chars */
#define COMM_QUIET                (A)
#define COMM_DEAF                 (B)
#define COMM_NOWIZ                (C)
#define COMM_NOAUCTION            (D)
#define COMM_NOGOSSIP             (E)
#define COMM_NOQUESTION           (F)
/* #define COMM_NOMUSIC           (G) Legacy */
#define COMM_NOCLAN               (H)
#define COMM_NOQUOTE              (I)
#define COMM_SHOUTSOFF            (J)

/* display flags */
#define COMM_COMPACT              (L)
#define COMM_BRIEF                (M)
#define COMM_PROMPT               (N)
#define COMM_COMBINE              (O)
#define COMM_TELNET_GA            (P)
#define COMM_SHOW_AFFECTS         (Q)
#define COMM_NOGRATS              (R)

/* penalties */
#define COMM_NOEMOTE              (T)
#define COMM_NOSHOUT              (U)
#define COMM_NOTELL               (V)
#define COMM_NOCHANNELS           (W)
#define COMM_SNOOP_PROOF          (Y)
#define COMM_AFK                  (Z)

/* WIZnet flags */
#define WIZ_ON                    (A)
#define WIZ_TICKS                 (B)
#define WIZ_LOGINS                (C)
#define WIZ_SITES                 (D)
#define WIZ_LINKS                 (E)
#define WIZ_DEATHS                (F)
#define WIZ_RESETS                (G)
#define WIZ_MOBDEATHS             (H)
#define WIZ_FLAGS                 (I)
#define WIZ_PENALTIES             (J)
#define WIZ_SACCING               (K)
#define WIZ_LEVELS                (L)
#define WIZ_SECURE                (M)
#define WIZ_SWITCHES              (N)
#define WIZ_SNOOPS                (O)
#define WIZ_RESTORE               (P)
#define WIZ_LOAD                  (Q)
#define WIZ_NEWBIE                (R)
#define WIZ_PREFIX                (S)
#define WIZ_SPAM                  (T)

/*
 * Prototype for a mob.
 * This is the in-memory version of #MOBILES.
 */
struct mob_index_data
{
    MOB_INDEX_DATA         *next;
    SPEC_FUN               *spec_fun;
    SHOP_DATA              *pShop;
    MPROG_LIST             *mprogs;
    AREA_DATA              *area;                      /* OLC */
    int                     vnum;
    int                     group;
    bool                    new_format;
    int                     count;
    int                     killed;
    char                   *player_name;
    char                   *short_descr;
    char                   *long_descr;
    char                   *description;
    int                     act;
    int                     affected_by;
    int                     alignment;
    int                     level;
    int                     hitroll;
    int                     hit[3];
    int                     mana[3];
    int                     damage[3];
    int                     ac[4];
    int                     dam_type;
    int                     off_flags;
    int                     imm_flags;
    int                     res_flags;
    int                     vuln_flags;
    int                     start_pos;
    int                     default_pos;
    int                     sex;
    int                     race;
    int                     wealth;
    int                     form;
    int                     parts;
    int                     size;
    char                   *material;
    int                     mprog_flags;
};

/* memory settings */
#define MEM_CUSTOMER        A
#define MEM_SELLER          B
#define MEM_HOSTILE         C
#define MEM_AFRAID          D

/* memory for mobs */
struct mem_data
{
    MEM_DATA               *next;
    bool                    valid;
    int                     id;
    int                     reaction;
    time_t                  when;
};

/*
 * One character (PC or NPC).
 */
struct char_data
{
    CHAR_DATA              *next;
    CHAR_DATA              *next_in_room;
    CHAR_DATA              *master;
    CHAR_DATA              *leader;
    CHAR_DATA              *fighting;
    CHAR_DATA              *reply;
    CHAR_DATA              *pet;
    CHAR_DATA              *mprog_target;
    MEM_DATA               *memory;
    SPEC_FUN               *spec_fun;
    MOB_INDEX_DATA         *pIndexData;
    DESCRIPTOR_DATA        *desc;
    AFFECT_DATA            *affected;
    NOTE_DATA              *pnote;
    OBJ_DATA               *carrying;
    OBJ_DATA               *on;
    ROOM_INDEX_DATA        *in_room;
    ROOM_INDEX_DATA        *was_in_room;
    AREA_DATA              *zone;
    PC_DATA                *pcdata;
    GEN_DATA               *gen_data;
    bool                    valid;
    char                   *name;
    int                     id;
    int                     version;
    char                   *short_descr;
    char                   *long_descr;
    char                   *description;
    char                   *prompt;
    char                   *prefix;
    int                     group;
    int                     clan;
    int                     sex;
    int                     iclass;
    int                     race;
    int                     level;
    int                     trust;
    int                     played;
    int                     lines;                     /* for the pager */
    time_t                  logon;
    int                     timer;
    int                     wait;
    int                     daze;
    int                     hit;
    int                     max_hit;
    int                     mana;
    int                     max_mana;
    int                     move;
    int                     max_move;
    int                     gold;
    int                     silver;
    int                     exp;
    int                     act;
    int                     comm;                      /* RT added to pad the vector */
    int                     wiznet;                    /* wiz stuff */
    int                     imm_flags;
    int                     res_flags;
    int                     vuln_flags;
    int                     invis_level;
    int                     incog_level;
    int                     affected_by;
    int                     position;
    int                     practice;
    int                     train;
    int                     carry_weight;
    int                     carry_number;
    int                     saving_throw;
    int                     alignment;
    int                     hitroll;
    int                     damroll;
    int                     armor[4];
    int                     wimpy;
    /*
     * stats 
     */
    int                     perm_stat[MAX_STATS];
    int                     mod_stat[MAX_STATS];
    /*
     * parts stuff 
     */
    int                     form;
    int                     parts;
    int                     size;
    char                   *material;
    /*
     * mobile stuff 
     */
    int                     off_flags;
    int                     damage[3];
    int                     dam_type;
    int                     start_pos;
    int                     default_pos;
    int                     mprog_delay;
};

/*
 * Data which only PC's have.
 */
struct pc_data
{
    PC_DATA                *next;
    BUFFER                 *buffer;
    bool                    valid;
    char                   *pwd;
    char                   *bamfin;
    char                   *bamfout;
    char                   *title;
    time_t                  last_note;
    time_t                  last_idea;
    time_t                  last_penalty;
    time_t                  last_news;
    time_t                  last_changes;
    int                     perm_hit;
    int                     perm_mana;
    int                     perm_move;
    int                     true_sex;
    int                     last_level;
    int                     condition[4];
    int                     learned[MAX_SKILL];
    bool                    group_known[MAX_GROUP];
    int                     points;
    bool                    confirm_delete;
    char                   *alias[MAX_ALIAS];
    char                   *alias_sub[MAX_ALIAS];
    int                     security;                  /* OLC - Builder security */
};

/* Data for generating characters -- only used during generation */
struct gen_data
{
    GEN_DATA               *next;
    bool                    valid;
    bool                    skill_chosen[MAX_SKILL];
    bool                    group_chosen[MAX_GROUP];
    int                     points_chosen;
};

/*
 * Liquids.
 */
#define LIQ_WATER           0

struct liq_type
{
    const char             *liq_name;
    const char             *liq_color;
    int                     liq_affect[5];
};

/*
 * Extra description data for a room or object.
 */
struct extra_descr_data
{
    EXTRA_DESCR_DATA       *next;                      /* Next in list */
    bool                    valid;
    char                   *keyword;                   /* Keyword in look/examine */
    char                   *description;               /* What to see */
};

/*
 * Prototype for an object.
 */
struct obj_index_data
{
    OBJ_INDEX_DATA         *next;
    EXTRA_DESCR_DATA       *extra_descr;
    AFFECT_DATA            *affected;
    AREA_DATA              *area;                      /* OLC */
    bool                    new_format;
    char                   *name;
    char                   *short_descr;
    char                   *description;
    int                     vnum;
    int                     reset_num;
    char                   *material;
    int                     item_type;
    int                     extra_flags;
    int                     wear_flags;
    int                     level;
    int                     condition;
    int                     count;
    int                     weight;
    int                     cost;
    int                     value[5];
};

/*
 * One object.
 */
struct obj_data
{
    OBJ_DATA               *next;
    OBJ_DATA               *next_content;
    OBJ_DATA               *contains;
    OBJ_DATA               *in_obj;
    OBJ_DATA               *on;
    CHAR_DATA              *carried_by;
    EXTRA_DESCR_DATA       *extra_descr;
    AFFECT_DATA            *affected;
    OBJ_INDEX_DATA         *pIndexData;
    ROOM_INDEX_DATA        *in_room;
    bool                    valid;
    bool                    enchanted;
    char                   *owner;
    char                   *name;
    char                   *short_descr;
    char                   *description;
    int                     item_type;
    int                     extra_flags;
    int                     wear_flags;
    int                     wear_loc;
    int                     weight;
    int                     cost;
    int                     level;
    int                     condition;
    char                   *material;
    int                     timer;
    int                     value[5];
};

#define MAX_EXIT            6

/*
 * Exit data.
 */
struct exit_data
{
    union
    {
        ROOM_INDEX_DATA        *to_room;
        int                     vnum;
    } u1;
    int                     exit_info;
    int                     key;
    char                   *keyword;
    char                   *description;
    EXIT_DATA              *next;                      /* OLC */
    int                     rs_flags;                  /* OLC */
    int                     orig_door;                 /* OLC */
};

/*
 * Reset commands:
 *   '*': comment
 *   'M': read a mobile 
 *   'O': read an object
 *   'P': put object in object
 *   'G': give object to mobile
 *   'E': equip object to mobile
 *   'D': set state of door
 *   'R': randomize room exits
 *   'S': stop (end of list)
 */

/*
 * Area-reset definition.
 */
struct reset_data
{
    RESET_DATA             *next;
    char                    command;
    int                     arg1;
    int                     arg2;
    int                     arg3;
    int                     arg4;
};

/*
 * Area definition.
 */
struct area_data
{
    AREA_DATA              *next;
    RESET_DATA             *reset_first;
    RESET_DATA             *reset_last;
    HELP_AREA              *helps;
    char                   *file_name;
    char                   *name;
    char                   *credits;
    int                     age;
    int                     nplayer;
    int                     low_range;
    int                     high_range;
    int                     min_vnum;
    int                     max_vnum;
    bool                    empty;
    char                   *builders;                  /* OLC - Listing of */
    int                     vnum;                      /* OLC - Area vnum */
    int                     area_flags;                /* OLC */
    int                     security;                  /* OLC - Value 1-9 */
};

/*
 * Room type.
 */
struct room_index_data
{
    ROOM_INDEX_DATA        *next;
    CHAR_DATA              *people;
    OBJ_DATA               *contents;
    EXTRA_DESCR_DATA       *extra_descr;
    AREA_DATA              *area;
    EXIT_DATA              *exit[MAX_EXIT];
    RESET_DATA             *reset_first;               /* OLC */
    RESET_DATA             *reset_last;                /* OLC */
    char                   *name;
    char                   *description;
    char                   *owner;
    int                     vnum;
    int                     room_flags;
    int                     light;
    int                     sector_type;
    int                     heal_rate;
    int                     mana_rate;
    int                     clan;
};

/*
 * Types of attacks.
 * Must be non-overlapping with spell/skill types,
 * but may be arbitrary beyond that.
 */
#define TYPE_UNDEFINED             -1
#define TYPE_HIT                 1000

/*
 *  Target types.
 */
#define TAR_IGNORE                  0
#define TAR_CHAR_OFFENSIVE          1
#define TAR_CHAR_DEFENSIVE          2
#define TAR_CHAR_SELF               3
#define TAR_OBJ_INV                 4
#define TAR_OBJ_CHAR_DEF            5
#define TAR_OBJ_CHAR_OFF            6

#define TARGET_CHAR                 0
#define TARGET_OBJ                  1
#define TARGET_ROOM                 2
#define TARGET_NONE                 3

/*
 * Skills include spells as a particular case.
 */
struct skill_type
{
    const char             *name;                      /* Name of skill */
    int                     skill_level[MAX_CLASS];    /* Level needed by class */
    int                     rating[MAX_CLASS];         /* How hard it is to learn */
    SPELL_FUN              *spell_fun;                 /* Spell pointer (for spells) */
    int                     target;                    /* Legal targets */
    int                     minimum_position;          /* Position for caster / user */
    int                    *pgsn;                      /* Pointer to associated gsn */
    int                     slot;                      /* Slot for #OBJECT loading */
    int                     min_mana;                  /* Minimum mana used */
    int                     beats;                     /* Waiting time after use */
    const char             *noun_damage;               /* Damage message */
    const char             *msg_off;                   /* Wear off message */
    const char             *msg_obj;                   /* Wear off message for obects */
};

struct group_type
{
    const char             *name;
    int                     rating[MAX_CLASS];
    const char             *spells[MAX_IN_GROUP];
};

/*
 * MOBprog definitions
 */
#define TRIG_ACT        (A)
#define TRIG_BRIBE      (B)
#define TRIG_DEATH      (C)
#define TRIG_ENTRY      (D)
#define TRIG_FIGHT      (E)
#define TRIG_GIVE       (F)
#define TRIG_GREET      (G)
#define TRIG_GRALL      (H)
#define TRIG_KILL       (I)
#define TRIG_HPCNT      (J)
#define TRIG_RANDOM     (K)
#define TRIG_SPEECH     (L)
#define TRIG_EXIT       (M)
#define TRIG_EXALL      (N)
#define TRIG_DELAY      (O)
#define TRIG_SURR       (P)

struct mprog_list
{
    int                     trig_type;
    char                   *trig_phrase;
    int                     vnum;
    char                   *code;
    MPROG_LIST             *next;
    bool                    valid;
};

struct mprog_code
{
    int                     vnum;
    char                   *code;
    MPROG_CODE             *next;
};

/*****************************************************************************
 *                                    OLC                                    *
 *****************************************************************************/

/*
 * Object defined in limbo.are
 * Used in save.c to load objects that don't exist.
 */
#define OBJ_VNUM_DUMMY   30

/*
 * Area flags.
 */
#define         AREA_NONE       0
#define         AREA_CHANGED    1                      /* Area has been modified. */
#define         AREA_ADDED      2                      /* Area has been added to. */
#define         AREA_LOADING    4                      /* Used for counting in db.c */

#define MAX_DIR   6
#define NO_FLAG -99                                    /* Must not be used in flags or
                                                        * stats. */
/*
 * Utility macros.
 */
#define IS_VALID(data)              ((data) != NULL && (data)->valid)
#define VALIDATE(data)              ((data)->valid = true)
#define INVALIDATE(data)            ((data)->valid = false)
#define UMIN(a, b)                  ((a) < (b) ? (a) : (b))
#define UMAX(a, b)                  ((a) > (b) ? (a) : (b))
#define URANGE(a, b, c)             ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))
#define LOWER(c)                    ((c) >= 'A' && (c) <= 'Z' ? (c)+'a'-'A' : (c))
#define UPPER(c)                    ((c) >= 'a' && (c) <= 'z' ? (c)+'A'-'a' : (c))
#define IS_SET(flag, bit)           ((flag) & (bit))
#define SET_BIT(var, bit)           ((var) |= (bit))
#define REMOVE_BIT(var, bit)        ((var) &= ~(bit))
#define IS_NULLSTR(str)             ((str) == NULL || (str)[0] == '\0')
#define ENTRE(min,num,max)          ( ((min) < (num)) && ((num) < (max)) )
#define CHECK_POS(a, b, c)          {                                                         \
                                        (a) = (b);                                            \
                                        if ( (a) < 0 )                                        \
                                            log_error("CHECK_POS : \"%s\" == %d < 0", c, a ); \
                                    }

/*
 * Character macros.
 */
#define IS_NPC(ch)                  (IS_SET((ch)->act, ACT_IS_NPC))
#define IS_IMMORTAL(ch)             (get_trust(ch) >= LEVEL_IMMORTAL)
#define IS_HERO(ch)                 (get_trust(ch) >= LEVEL_HERO)
#define IS_TRUSTED(ch,level)        (get_trust((ch)) >= (level))
#define IS_AFFECTED(ch, sn)         (IS_SET((ch)->affected_by, (sn)))

#define GET_AGE(ch)                 ((int) (17 + ((ch)->played + current_time \
                                               - (ch)->logon ) / 72000))

#define IS_GOOD(ch)                 (ch->alignment >= 350)
#define IS_EVIL(ch)                 (ch->alignment <= -350)
#define IS_NEUTRAL(ch)              (!IS_GOOD(ch) && !IS_EVIL(ch))

#define IS_AWAKE(ch)                (ch->position > POS_SLEEPING)
#define GET_AC(ch,type)             ((ch)->armor[type] + ( IS_AWAKE(ch) \
                                    ? dex_app[get_curr_stat(ch,STAT_DEX)].defensive : 0))
#define GET_HITROLL(ch)             ((ch)->hitroll \
                                    + str_app[get_curr_stat(ch,STAT_STR)].tohit)
#define GET_DAMROLL(ch)             ((ch)->damroll \
                                    + str_app[get_curr_stat(ch,STAT_STR)].todam)

#define IS_OUTSIDE(ch)              (!IS_SET((ch)->in_room->room_flags,ROOM_INDOORS))

#define WAIT_STATE(ch, npulse)      ((ch)->wait = UMAX((ch)->wait, (npulse)))
#define DAZE_STATE(ch, npulse)      ((ch)->daze = UMAX((ch)->daze, (npulse)))
#define get_carry_weight(ch)        ((ch)->carry_weight + (ch)->silver/10 +  \
                                                      (ch)->gold * 2 / 5)

#define act(format,ch,arg1,arg2,type) \
        act_new((format),(ch),(arg1),(arg2),(type),POS_RESTING)

#define HAS_TRIGGER(ch,trig)        (IS_SET((ch)->pIndexData->mprog_flags,(trig)))
#define IS_SWITCHED( ch )           ( ch->desc && ch->desc->original )
#define IS_BUILDER(ch, Area)        ( !IS_NPC(ch) && !IS_SWITCHED( ch ) &&   \
                                    ( ch->pcdata->security >= Area->security \
                                      || strstr( Area->builders, ch->name )  \
                                      || strstr( Area->builders, "All" ) ) )
/*
 * Object macros.
 */
#define CAN_WEAR(obj, part)         (IS_SET((obj)->wear_flags,  (part)))
#define IS_OBJ_STAT(obj, si)        (IS_SET((obj)->extra_flags, (si)))
#define IS_WEAPON_STAT(obj,si)      (IS_SET((obj)->value[4], (si)))
#define WEIGHT_MULT(obj)            ((obj)->item_type == ITEM_CONTAINER \
                                    ? (obj)->value[4] : 100)

/*
 * Description macros.
 */
#define ONAME(obj)                  ( ( (obj) && (obj)->name ) \
                                    ? (obj)->name \
                                    : "something" )
#define NAME(ch)                    ( (ch) \
                                    ? ( IS_NPC(ch) ? (ch)->short_descr : (ch)->name ) \
                                    : "someone" )
#define OPERS(obj, looker)           ( can_see_obj((looker), (obj)) \
                                    ? (obj)->short_descr \
                                    : "something" )
#define PERS(ch, looker)            ( can_see((looker), (ch)) \
                                    ? ( IS_NPC(ch) ? (ch)->short_descr : (ch)->name ) \
                                    : "someone" )
#define HESHE(ch)                   ( ( (ch)->sex == SEX_MALE ) \
                                    ? "he" \
                                    : ( ( (ch)->sex == SEX_FEMALE ) \
                                        ? "she" \
                                        : "it" ) )
#define HIMHER(ch)                  ( ( (ch)->sex == SEX_MALE ) \
                                    ? "him" \
                                    : ( ( (ch)->sex == SEX_FEMALE ) \
                                        ? "her" \
                                        : "it" ) )
#define HISHER(ch)                  ( ( (ch)->sex == SEX_MALE ) \
                                    ? "his" \
                                    : ( ( (ch)->sex == SEX_FEMALE ) \
                                        ? "her" \
                                        : "its" ) )
#define ANA(str)                    ( index( "aeiouyAEIOUY", *(str) ) ? "an" : "a")
#define UANA(str)                   ( index( "aeiouyAEIOUY", *(str) ) ? "An" : "A")

/*
 * These macros are used in save.c, and also by OLC
 */

#define KEY( literal, field, value )                             \
                                if ( !str_cmp( word, literal ) ) \
                                {                                \
                                    field  = value;              \
                                    fMatch = true;               \
                                    break;                       \
                                }

#define KEYS( literal, field, value )                            \
                                if ( !str_cmp( word, literal ) ) \
                                {                                \
                                    free_string(field);          \
                                    field  = value;              \
                                    fMatch = true;               \
                                    break;                       \
                                }

/*
 * Structure for a social in the socials table.
 */
struct social_type
{
    char                    name[20];
    char                   *char_no_arg;
    char                   *others_no_arg;
    char                   *char_found;
    char                   *others_found;
    char                   *vict_found;
    char                   *char_not_found;
    char                   *char_auto;
    char                   *others_auto;
};

/*
 * We've switched from crypt() to sha256_crypt() to avoid
 * issues with some systems having crypt, and others not having
 * it.  Rather than fight silly US export laws, we'll just use
 * a free and superior cryptographic function.
 *
 * However, we recognize that some of you like to use plain
 * text passwords for some reason, so defining NOCRYPT will
 * still accomplish that.
 */
#if defined(NOCRYPT)
#define CRYPT(s)            (s)
#else
#define CRYPT(s)            sha256_crypt(s)
#endif

#define DEFAULT_PORT        4000

/*
 * Data files used by the server.
 *
 * AREA_LIST contains a list of areas to boot.
 * All files are read in completely at bootup.
 * Most output files (bug, idea, typo, shutdown) are append-only.
 */
#define PLAYER_DIR          "../player"                /* Player files */
#define GOD_DIR             "../gods"                  /* list of gods */
#define AREA_DIR            "../area"                  /* The WORLD, Baby! */
#define SYS_DIR             "../system"                /* Miscellaneous files */
#define TEMP_FILE           SYS_DIR "/romtmp"

#define AREA_LIST           AREA_DIR "/area.lst"       /* List of areas */
#define BUG_FILE            SYS_DIR "/bugs.txt"        /* For 'bug' and bug() */
#define TYPO_FILE           SYS_DIR "/typos.txt"       /* For 'typo' */
#define NOTE_FILE           SYS_DIR "/notes.not"       /* For 'notes' */
#define IDEA_FILE           SYS_DIR "/ideas.not"
#define PENALTY_FILE        SYS_DIR "/penal.not"
#define NEWS_FILE           SYS_DIR "/news.not"
#define CHANGES_FILE        SYS_DIR "/chang.not"
#define SHUTDOWN_FILE       SYS_DIR "/shutdown.txt"    /* For 'shutdown' */
#define PLAYER_FILE         PLAYER_DIR "/players.txt"

#define MUD_REBOOT          0                          /* Normal exit */
#define MUD_HALT            1                          /* Exit due to error or signal */

#define LOG_INFO            0                          /* These must match up with */
#define LOG_ERROR           1                          /* LogNames, in bug.c */
#define LOG_FATAL           2
#define LOG_BOOT            3
#define LOG_AUTH            4
#define LOG_KILL            5
#define LOG_BALANCE         6
#define LOG_RESET           7

#define log_info(Str, ...) \
        event_logger(LOG_INFO, NULL, \
                     __FILE__, __FUNCTION__, __LINE__, \
                     NULL, NULL, \
                     GOD, (Str), ## __VA_ARGS__ )
#define log_error(Str, ...) \
        event_logger(LOG_ERROR, NULL, \
                     __FILE__, __FUNCTION__, __LINE__, \
                     NULL, NULL, \
                     GOD, (Str), ## __VA_ARGS__ )
#define log_fatal(Str, ...) \
        event_logger(LOG_FATAL, NULL, \
                     __FILE__, __FUNCTION__, __LINE__, \
                     NULL, NULL, \
                     GOD, (Str), ## __VA_ARGS__ )
#define log_boot(Str, ...) \
        event_logger(LOG_BOOT, NULL, \
                     __FILE__, __FUNCTION__, __LINE__, \
                     NULL, NULL, \
                     GOD, (Str), ## __VA_ARGS__ )
#define log_auth(Ch, Str, ...) \
        event_logger(LOG_AUTH, NULL, \
                     NULL, NULL, 0, \
                     (Ch), NULL, \
                     GOD, (Str), ## __VA_ARGS__ )
#define log_kill(Ch, Victim, Str, ...) \
        event_logger(LOG_KILL, NULL, \
                     NULL, NULL, 0, \
                     (Ch), (Victim), \
                     GOD, (Str), ## __VA_ARGS__ )
#define log_balance(Ch, Str, ...) \
        event_logger(LOG_BALANCE, NULL, \
                     NULL, NULL, 0, \
                     (Ch), NULL, \
                     GOD, (Str), ## __VA_ARGS__ )
#define log_reset(Str, ...) \
        event_logger(LOG_RESET, NULL, \
                     NULL, NULL, 0, \
                     NULL, NULL, \
                     GOD, (Str), ## __VA_ARGS__ )

/*
 * Our function prototypes.
 * One big lump ... this is every function in Merc.
 */

/* alias.c */
void                    substitute_alias( DESCRIPTOR_DATA *d, char *argument );

/* bug.c */
void                    bug( const char *str, int param );
void                    log_string( const char *str );
void                    event_logger( unsigned int Type, const char *BugFile,
                                      const char *File, const char *Func, int Line,
                                      struct char_data *ch, struct char_data *victim,
                                      int Level, const char *Str, ... )
    __attribute__ ( ( format( printf, 9, 10 ) ) );

/* comm.c */
extern const char       echo_off_str[];
extern const char       echo_on_str[];
extern const char       go_ahead_str[];

extern DESCRIPTOR_DATA *descriptor_list;
extern DESCRIPTOR_DATA *descriptor_free;
extern MPROG_CODE      *mprog_list;

extern bool             need_god;
extern bool             merc_down;
extern bool             wizlock;
extern bool             newlock;
extern char             str_boot_time[MAX_INPUT_LENGTH];
extern time_t           current_time;

int                     main( int argc, char **argv );
int                     init_listening_socket( int port );
void                    game_loop( int control );
void                    init_descriptor( int control );
void                    close_descriptor( DESCRIPTOR_DATA *dclose );
bool                    read_from_descriptor( DESCRIPTOR_DATA *d );
void                    read_from_buffer( DESCRIPTOR_DATA *d );
bool                    process_output( DESCRIPTOR_DATA *d, bool fPrompt );
void                    nanny( DESCRIPTOR_DATA *d, const char *argument );
bool                    check_parse_name( const char *name );
bool                    check_reconnect( DESCRIPTOR_DATA *d, const char *name,
                                         bool fConn );
bool                    check_playing( DESCRIPTOR_DATA *d, const char *name );
void                    stop_idling( CHAR_DATA *ch );
void                    proper_exit( int code, const char *Str, ... )
    __attribute__ ( ( format( printf, 2, 3 ) ) );;
void                    shutdown_request( int a );
void                    show_string( struct descriptor_data *d, const char *input );
void                    bust_a_prompt( CHAR_DATA *ch );
bool                    write_to_socket( int desc, const char *txt, int length );
void                    write_to_buffer( DESCRIPTOR_DATA *d, const char *txt,
                                         int length );
void                    desc_printf( DESCRIPTOR_DATA *d, const char *format, ... )
    __attribute__ ( ( format( printf, 2, 3 ) ) );
void                    send_to_char( const char *format, CHAR_DATA *ch );
void                    ch_printf( CHAR_DATA *ch, const char *format, ... )
    __attribute__ ( ( format( printf, 2, 3 ) ) );
void                    page_to_char( const char *format, CHAR_DATA *ch );
void                    page_printf( CHAR_DATA *ch, const char *format, ... )
    __attribute__ ( ( format( printf, 2, 3 ) ) );
void                    act_new( const char *format, CHAR_DATA *ch, const void *arg1,
                                 const void *arg2, int type, int min_pos );
void                    act_printf( const char *format, CHAR_DATA *ch, const void *arg1,
                                    const void *arg2, int type, int min_pos,
                                    bool hide_invis, ... )
    __attribute__ ( ( format( printf, 1, 8 ) ) );
void                    wiznet( const char *format, CHAR_DATA *ch, OBJ_DATA *obj,
                                int flag, int flag_skip, int min_level );
void                    wiz_printf( CHAR_DATA *ch, OBJ_DATA *obj, int flag, int flag_skip,
                                    int min_level, const char *format, ... )
    __attribute__ ( ( format( printf, 6, 7 ) ) );

/* fight.c */
void                    violence_update( void );
void                    check_assist( CHAR_DATA *ch, CHAR_DATA *victim );
void                    multi_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt );
void                    mob_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt );
void                    one_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt );
bool                    damage( CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt,
                                int dam_type, bool show );
bool                    is_safe( CHAR_DATA *ch, CHAR_DATA *victim );
bool                    is_safe_spell( CHAR_DATA *ch, CHAR_DATA *victim, bool area );
void                    check_killer( CHAR_DATA *ch, CHAR_DATA *victim );
bool                    check_parry( CHAR_DATA *ch, CHAR_DATA *victim );
bool                    check_shield_block( CHAR_DATA *ch, CHAR_DATA *victim );
bool                    check_dodge( CHAR_DATA *ch, CHAR_DATA *victim );
void                    update_pos( CHAR_DATA *victim );
void                    set_fighting( CHAR_DATA *ch, CHAR_DATA *victim );
void                    stop_fighting( CHAR_DATA *ch, bool fBoth );
void                    make_corpse( CHAR_DATA *ch );
void                    death_cry( CHAR_DATA *ch );
void                    raw_kill( CHAR_DATA *victim );
void                    group_gain( CHAR_DATA *ch, CHAR_DATA *victim );
int                     xp_compute( CHAR_DATA *gch, CHAR_DATA *victim, int total_levels );
void                    dam_message( CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt,
                                     bool immune );
void                    disarm( CHAR_DATA *ch, CHAR_DATA *victim );

/* handler.c */
bool                    is_friend( CHAR_DATA *ch, CHAR_DATA *victim );
int                     count_users( OBJ_DATA *obj );
int                     material_lookup( const char *name );
int                     weapon_lookup( const char *name );
int                     weapon_type_lookup( const char *name );
const char             *item_name( int target_type );
const char             *weapon_name( int target_type );
int                     attack_lookup( const char *name );
int                     wiznet_lookup( const char *name );
int                     class_lookup( const char *name );
int                     check_immune( CHAR_DATA *ch, int dam_type );
bool                    is_clan( CHAR_DATA *ch );
bool                    is_same_clan( CHAR_DATA *ch, CHAR_DATA *victim );
bool                    is_old_mob( CHAR_DATA *ch );
int                     get_skill( const CHAR_DATA *ch, int sn );
int                     get_skill_byname( const CHAR_DATA *ch, const char *name );
int                     get_weapon_sn( CHAR_DATA *ch );
int                     get_weapon_skill( CHAR_DATA *ch, int sn );
void                    reset_char( CHAR_DATA *ch );
int                     get_trust( const CHAR_DATA *ch );
int                     get_age( CHAR_DATA *ch );
int                     get_curr_stat( const CHAR_DATA *ch, int stat_index );
int                     get_max_train( CHAR_DATA *ch, int stat_index );
int                     can_carry_n( CHAR_DATA *ch );
int                     can_carry_w( CHAR_DATA *ch );
bool                    is_name( const char *str, const char *namelist );
bool                    is_exact_name( const char *str, const char *namelist );
void                    affect_enchant( OBJ_DATA *obj );
void                    affect_modify( CHAR_DATA *ch, AFFECT_DATA *paf, bool fAdd );
AFFECT_DATA            *affect_find( AFFECT_DATA *paf, int sn );
void                    affect_check( CHAR_DATA *ch, int where, int vector );
void                    affect_to_char( CHAR_DATA *ch, AFFECT_DATA *paf );
void                    affect_to_obj( OBJ_DATA *obj, AFFECT_DATA *paf );
void                    affect_remove( CHAR_DATA *ch, AFFECT_DATA *paf );
void                    affect_remove_obj( OBJ_DATA *obj, AFFECT_DATA *paf );
void                    affect_strip( CHAR_DATA *ch, int sn );
bool                    is_affected( CHAR_DATA *ch, int sn );
void                    affect_join( CHAR_DATA *ch, AFFECT_DATA *paf );
void                    char_from_room( CHAR_DATA *ch );
void                    char_to_room( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex );
void                    obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch );
void                    obj_from_char( OBJ_DATA *obj );
int                     apply_ac( OBJ_DATA *obj, int iWear, int type );
OBJ_DATA               *get_eq_char( CHAR_DATA *ch, int iWear );
void                    equip_char( CHAR_DATA *ch, OBJ_DATA *obj, int iWear );
void                    unequip_char( CHAR_DATA *ch, OBJ_DATA *obj );
int                     count_obj_list( OBJ_INDEX_DATA *pObjIndex, OBJ_DATA *list );
void                    obj_from_room( OBJ_DATA *obj );
void                    obj_to_room( OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex );
void                    obj_to_obj( OBJ_DATA *obj, OBJ_DATA *obj_to );
void                    obj_from_obj( OBJ_DATA *obj );
void                    extract_obj( OBJ_DATA *obj );
void                    extract_char( CHAR_DATA *ch, bool fPull );
CHAR_DATA              *get_char_room( CHAR_DATA *ch, const char *argument );
CHAR_DATA              *get_char_world( CHAR_DATA *ch, const char *argument );
OBJ_DATA               *get_obj_type( OBJ_INDEX_DATA *pObjIndex );
OBJ_DATA               *get_obj_list( CHAR_DATA *ch, const char *argument,
                                      OBJ_DATA *list );
OBJ_DATA               *get_obj_carry( const CHAR_DATA *ch, const char *argument,
                                       const CHAR_DATA *viewer );
OBJ_DATA               *get_obj_wear( const CHAR_DATA *ch, const char *argument );
OBJ_DATA               *get_obj_here( CHAR_DATA *ch, const char *argument );
OBJ_DATA               *get_obj_world( CHAR_DATA *ch, const char *argument );
void                    deduct_cost( CHAR_DATA *ch, int cost );
OBJ_DATA               *create_money( int gold, int silver );
int                     get_obj_number( OBJ_DATA *obj );
int                     get_obj_weight( OBJ_DATA *obj );
int                     get_true_weight( OBJ_DATA *obj );
bool                    room_is_dark( ROOM_INDEX_DATA *pRoomIndex );
bool                    is_room_owner( CHAR_DATA *ch, ROOM_INDEX_DATA *room );
bool                    room_is_private( ROOM_INDEX_DATA *pRoomIndex );
bool                    can_see_room( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex );
bool                    can_see( const CHAR_DATA *ch, const CHAR_DATA *victim );
bool                    can_see_obj( const CHAR_DATA *ch, const OBJ_DATA *obj );
bool                    can_drop_obj( CHAR_DATA *ch, OBJ_DATA *obj );
const char             *affect_loc_name( int location );
const char             *affect_bit_name( int vector );
const char             *extra_bit_name( int vector );
const char             *act_bit_name( int vector );
const char             *comm_bit_name( int vector );
const char             *imm_bit_name( int vector );
const char             *wear_bit_name( int vector );
const char             *form_bit_name( int vector );
const char             *part_bit_name( int vector );
const char             *weapon_bit_name( int vector );
const char             *cont_bit_name( int vector );
const char             *off_bit_name( int vector );

/* mob_prog.c */
void                    program_flow( int vnum, char *source,
                                      CHAR_DATA *mob, CHAR_DATA *ch,
                                      const void *arg1, const void *arg2 );
void                    mp_act_trigger( const char *argument, CHAR_DATA *mob,
                                        CHAR_DATA *ch, const void *arg1,
                                        const void *arg2, int type );
bool                    mp_percent_trigger( CHAR_DATA *mob, CHAR_DATA *ch,
                                            const void *arg1,
                                            const void *arg2, int type );
void                    mp_bribe_trigger( CHAR_DATA *mob, CHAR_DATA *ch, int amount );
bool                    mp_exit_trigger( CHAR_DATA *ch, int dir );
void                    mp_give_trigger( CHAR_DATA *mob, CHAR_DATA *ch, OBJ_DATA *obj );
void                    mp_greet_trigger( CHAR_DATA *ch );
void                    mp_hprct_trigger( CHAR_DATA *mob, CHAR_DATA *ch );

/* mob_cmds.c */
void                    mob_interpret( CHAR_DATA *ch, const char *argument );

/* note.c */
extern NOTE_DATA       *note_list;
extern NOTE_DATA       *idea_list;
extern NOTE_DATA       *penalty_list;
extern NOTE_DATA       *news_list;
extern NOTE_DATA       *changes_list;
int                     count_spool( CHAR_DATA *ch, NOTE_DATA *spool );
void                    save_notes( int type );
void                    load_notes( void );
void                    load_thread( const char *name, NOTE_DATA **list, int type,
                                     time_t free_time );
void                    append_note( NOTE_DATA *pnote );
bool                    is_note_to( CHAR_DATA *ch, NOTE_DATA *pnote );
void                    note_attach( CHAR_DATA *ch, int type );
void                    note_remove( CHAR_DATA *ch, NOTE_DATA *pnote, bool idelete );
bool                    hide_note( CHAR_DATA *ch, NOTE_DATA *pnote );
void                    update_read( CHAR_DATA *ch, NOTE_DATA *pnote );
void                    parse_note( CHAR_DATA *ch, const char *argument, int type );

/* playerlist.c */
extern struct player_list *all_players;
extern int              top_player;
void                    dump_player_list( void );
void                    load_player_list( void );
void                    update_player_list( CHAR_DATA *ch, bool online );
struct player_list     *find_player_in_list( const char *name );

/* save.c */
char                   *print_flags( int flag );
void                    save_char_obj( CHAR_DATA *ch );
void                    fwrite_char( CHAR_DATA *ch, FILE * fp );
void                    fwrite_pet( CHAR_DATA *pet, FILE * fp );
void                    fwrite_obj( CHAR_DATA *ch, OBJ_DATA *obj, FILE * fp, int iNest );
bool                    load_char_obj( DESCRIPTOR_DATA *d, const char *name );
void                    init_player( CHAR_DATA *ch );
void                    load_player_sections( CHAR_DATA *ch, FILE * fp );
void                    upgrade_player_allver( CHAR_DATA *ch );
void                    upgrade_player_v2( CHAR_DATA *ch );
void                    upgrade_player_v3( CHAR_DATA *ch );
void                    upgrade_player_v4( CHAR_DATA *ch );
void                    fread_char( CHAR_DATA *ch, FILE * fp );
void                    fread_pet( CHAR_DATA *ch, FILE * fp );
void                    fread_obj( CHAR_DATA *ch, FILE * fp );

/* skills.c */
GEN_DATA               *new_gen_data( void );
void                    free_gen_data( GEN_DATA *gen );
void                    list_group_costs( CHAR_DATA *ch );
void                    list_group_chosen( CHAR_DATA *ch );
int                     exp_per_level( CHAR_DATA *ch, int points );
bool                    parse_gen_groups( CHAR_DATA *ch, const char *argument );
void                    check_improve( CHAR_DATA *ch, int sn, bool success,
                                       int multiplier );
int                     group_lookup( const char *name );
void                    gn_add( CHAR_DATA *ch, int gn );
void                    gn_remove( CHAR_DATA *ch, int gn );
void                    group_add( CHAR_DATA *ch, const char *name, bool deduct );
void                    group_remove( CHAR_DATA *ch, const char *name );

/* update.c */
extern int              save_number;

void                    advance_level( CHAR_DATA *ch, bool hide );
void                    gain_exp( CHAR_DATA *ch, int gain );
int                     hit_gain( CHAR_DATA *ch );
int                     mana_gain( CHAR_DATA *ch );
int                     move_gain( CHAR_DATA *ch );
void                    gain_condition( CHAR_DATA *ch, int iCond, int value );
void                    mobile_update( void );
void                    weather_update( void );
void                    char_update( void );
void                    obj_update( void );
void                    aggr_update( void );
void                    update_handler( void );
