/*
 * RAM $Id: ban.h 69 2009-01-11 18:13:26Z quixadhal $
 */

#define BAN_FILE            SYS_DIR "/ban.txt"

typedef struct ban_data BAN_DATA;

/*
 * Site ban structure.
 */
#define BAN_SUFFIX               A
#define BAN_PREFIX               B
#define BAN_NEWBIES              C
#define BAN_ALL                  D
#define BAN_PERMIT               E
#define BAN_PERMANENT            F

struct ban_data
{
    BAN_DATA               *next;
    bool                    valid;
    int                     ban_flags;
    int                     level;
    std::string             name;
};

/* ban.c */
extern BAN_DATA        *ban_list;
void                    save_bans( void );
void                    load_bans( void );
bool                    check_ban( char *site, int type );
void                    ban_site( CHAR_DATA *ch, const char *argument, bool fPerm );

