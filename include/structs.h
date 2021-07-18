/* include/structs.h - Déclaration des différentes structures
 * Copyright (C) 2004 ircdreams.org
 *
 * contact: bugs@ircdreams.org
 * site web: http://ircdreams.org
 *
 * Services pour serveur IRC. Supporté sur IrcDreams V.2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*------------------- Structures robot -------------------*/
struct robot {
  char server[HOSTLEN + 1];
  char servnum[NUMSERV + 1];
  char pass[21];
  char ip[16];
  char bindip[16];
  int port;
  char name[REALEN + 1];
  char chan[REGCHANLEN + 1];
  char pchan[REGCHANLEN + 1];
  char cara;
  int sock;
  time_t uptime;
  time_t lasttime;	/* pour traffic */
  unsigned long lastbytes;      /*  idem */
  unsigned long dataS; /* bytes envoyées depuis le début */
  unsigned long dataQ; /* bytes reçues */

  int w2c_port; 
  int CONtotal; 
  char w2c_pass[16 + 1]; 
  unsigned int WEBtrafficUP; 
  unsigned int WEBtrafficDL; 
};

struct bots {
  char nick[NICKLEN + 1];
  char ident[USERLEN + 1];
  char host[HOSTLEN + 1];
  char name[REALEN + 1];
  char mode[10];
  char num[2 * (NUMSERV + 1 )];
};

enum TimerType {TIMER_ABSOLU, TIMER_RELATIF, TIMER_PERIODIC}; 
    
typedef struct Timer { 
        time_t expire; 
        time_t delay; 
        enum TimerType type; 
#ifdef TDEBUG
	char data[150];
	int id;
#endif
        int (*callback) (struct Timer *); 
        void *data1; 
        void *data2; 
        struct Timer *last; 
        struct Timer *next; 
} Timer; 


struct suspendinfo {
	char from[NICKLEN + 1];
	char raison[251];
	time_t expire;
	time_t debut;
	Timer *timer;
	void *data;
};

/*------------------- Structures informations serveurs -------------------*/

typedef struct Link {
	char serv[HOSTLEN + 1];
	char num[NUMSERV + 1];
	int maxusers;
	int smask;
	int flag;
#define ST_BURST 	0x01
#define ST_ONLINE 	0x02
#define ISHUB 		0x04
	struct Link *hub;
} aServer;

/*------------------- Structures users et accès -------------------*/
typedef struct nickinfo {
	char nick[NICKLEN + 1];
	char ident[USERLEN + 1];
	char host[HOSTLEN + 1];
        char crypt[HOSTLEN + 1];
	struct Link *serveur;
	char name[REALEN + 1];
	char base64[BASE64LEN + 1];
	char numeric[2 * (NUMSERV + 1 )];
	unsigned int flag;
#define N_REGISTER 	0x000001
#define N_AWAY 		0x000002
#define N_OPER 		0x000004
#define N_SERVICE 	0x000008
#define N_ADM 		0x000010
#define N_INV 		0x000020
#define N_GOD 		0x000040
#define N_WALLOPS 	0x000080
#define N_DEBUG 	0x000100
#define N_FEMME 	0x000200
#define N_HOMME 	0x000400
#define N_CRYPT 	0x000800
#define N_REG 		0x001000
#define N_DIE 		0x002000
#define N_DEAF 		0x004000
#define N_SPOOF 	0x008000
#define N_HELPER	0x010000
#define N_WHOIS		0x020000
#define N_IDLE		0x040000
#define N_CHANNEL	0x080000
#define N_PRIVATE	0x100000
#define N_HIDE		0x200000
#define N_HASKILL       0x400000
/* 0x1000000 utilisé avant pour les modos */
#define N_REGPRIVATE	0x2000000
#define N_STRIPOPER	0x4000000
#define N_UMODES (N_INV |N_CRYPT|N_REG|N_OPER|N_SERVICE|N_ADM|N_GOD \
	|N_FEMME|N_HOMME|N_DEBUG|N_WALLOPS|N_DIE|N_DEAF|N_SPOOF|N_HELPER \
	|N_WHOIS|N_IDLE|N_CHANNEL|N_PRIVATE|N_HIDE|N_REGPRIVATE|N_STRIPOPER)

	time_t ttmco;
	time_t floodtime;
	unsigned int floodcount;
	Timer *timer;
	struct userinfo *user;
	struct nickinfo *next;
	struct joininfo *joinhead;
} aNick;

typedef struct userinfo {
   int uid;
   char nick[NICKLEN + 1];
   char passwd[PWDLEN + 1];
   int level;
   time_t lastseen;
   time_t reg_time;
   int flag;
#define U_PKILL		 0x0001
#define U_PNICK		 0x0002
#define U_WANTDROP	 0x0004
#define U_NOPURGE	 0x0008
#define U_WANTX		 0x0010
#define U_OUBLI		 0x0020
#define U_FIRST 	 0x0040
#define U_NOMEMO	 0x0080
#define U_PREJECT	 0x0100
#define U_POKACCESS	 0x0200
#define U_PACCEPT	 0x0400
#define U_ALREADYCHANGE	 0x0800
#define U_ADMBUSY 	 0x1000
#define U_VHOST		 0x2000
#define U_MD5PASS	 0x4000
#define U_HASVOTE	 0x8000
#define U_NOVOTE	 0x10000
#define U_NOMAIL	 0x20000
#define U_SWHOIS	 0x40000
#define U_PMREPLY        0x80000
#define U_MALE		 0x100000
#define U_FEMELLE	 0x200000
#define U_REALHOST	 0x400000

#define U_ALL (U_PKILL|U_PNICK|U_WANTDROP|U_NOPURGE|U_WANTX|U_OUBLI|U_FIRST| \
		U_NOMEMO|U_PREJECT|U_POKACCESS|U_PACCEPT|U_ALREADYCHANGE|\
		U_ADMBUSY|U_VHOST|U_MD5PASS|U_HASVOTE|U_NOVOTE|U_NOMAIL|\
		U_SWHOIS|U_PMREPLY|U_MALE|U_FEMELLE|U_REALHOST)
   char mail[MAILLEN + 1];
   char *lastlogin;
   time_t cantregchan;

   char vhost[HOSTLEN + 1];
   char *swhois;
   struct Lang *lang;
   struct suspendinfo *suspend;
   struct access *accesshead;
   aNick *n;
   struct memos *memohead;
   struct userinfo *next, *mailnext, *vhostnext, *user_nextalias, *hash_next;
   struct alias *aliashead;
} anUser;

typedef struct access {
   struct chaninfo *c; /* pour rediriger vers la struct du chan où on a access.*/
   int level;
   int flag;
#define A_OP		 0x01
#define A_HALFOP	 0x02
#define A_VOICE		 0x04
#define A_PROTECT	 0x08
#define A_SUSPEND	 0x10
#define A_WAITACCESS 	 0x20
   time_t lastseen;
   char *info;
   struct userinfo *user;
   struct access *next;
} anAccess;

typedef struct memos {
   char de[NICKLEN + 1];
   time_t date;
   char message[MEMOLEN + 1];
   int flag;
#define MEMO_READ       0x1 
#define MEMO_AUTOEXPIRE 0x2 
   struct memos *next;
} aMemo;

typedef struct alias {
   char name[NICKLEN + 1];
   anUser *user;
   struct alias *user_nextalias;
   struct alias *hash_next;
} anAlias;

typedef struct aDNR_ {
	char *mask;
	char *raison;
	struct aDNR_ *next;
	struct aDNR_ *last;
	unsigned int flag;
#define DNR_TYPEUSER 0x01 
#define DNR_TYPECHAN 0x02 
#define DNR_MASK        0x04 
        time_t date; 
        char from[NICKLEN + 1]; 
} aDNR; 

/*------------------- Structures chans -------------------*/

struct cmode {
#define C_MMSG 		0x0001
#define C_MTOPIC 	0x0002
#define C_MINV 		0x0004
#define C_MLIMIT 	0x0008
#define C_MKEY 		0x0010
#define C_MSECRET 	0x0020
#define C_MPRIVATE 	0x0040
#define C_MMODERATE 	0x0080
#define C_MNOCTRL 	0x0100
#define C_MNOCTCP 	0x0200
#define C_MOPERONLY 	0x0400
#define C_MUSERONLY 	0x0800
#define C_MACCONLY	0x1000
#define C_MNONOTICE	0x2000
#define C_MNOQUITPARTS	0x4000
#define C_MAUDITORIUM	0X8000
#define C_MNOAMSG	0X10000
#define C_MNOCAPS	0X20000
#define C_MNOWEBPUB	0X40000
#define C_MNOCHANPUB	0X80000
	unsigned int modes; /* modes sous forme de flag*/
	int limit;
	char key[KEYLEN + 1];
};

typedef struct chaninfo {
	char chan[REGCHANLEN + 1];

	struct cmode defmodes;/* propriétés des salons regs*/
	char deftopic[TOPICLEN + 1];
	char welcome[TOPICLEN + 1];
	char description[DESCRIPTIONLEN + 1];
	char *motd;
	char url[URLLEN + 1];
	int flag;
#define C_SETWELCOME		0x0001
#define C_JOINED		0x0002
#define C_STRICTOP		0x0004
#define C_LOCKTOPIC		0x0008
#define C_NOBANS		0x0010
#define C_WARNED		0x0020
#define C_NOOPS			0x0040
#define C_AUTOINVITE 		0x0080
#define C_ALREADYRENAME 	0x0100
#define C_FLIMIT 		0x0200
#define C_AUTOVOICE		0x0400
#define C_NOINFO		0x0800
#define C_NOPUBCMD		0x1000
#define C_NOVOICES		0x2000
#define C_NOHALFOPS		0x4000
	int banlevel;
	int cml;
	int bantype;
	int limit_inc;
	int limit_min;
	time_t bantime;
	time_t creation_time;
	time_t lastact;
	anAccess *owner;
	Timer *timer;
	struct baninfo *banhead;
	struct suspendinfo *suspend;
	struct SLink *access; /* utiliser un link pour avoir la liste des access plus rapidement*/
	struct chaninfo *next;
	struct NChan *netchan;
} aChan;

typedef struct NChan { 
        char chan[CHANLEN + 1];/* infos sur le salon actuel du réseau */
        char topic[TOPICLEN + 1];
        time_t timestamp;
        struct cmode modes;
        int users;
	int flags;
        aChan *regchan;
        struct SLink *members; /* listes ds users dans le chan */
        struct NChan *next;
} aNChan;

typedef struct baninfo {
	char nick[NICKLEN + 1];
	char user[USERLEN + 1];
	char host[HOSTLEN + 1];
	char de[NICKLEN + 1];
	char *raison; 
        char *mask; 
	time_t debut;
	time_t fin;
	int level;
	int flag;
#define BAN_ANICKS 0x1 
#define BAN_AUSERS 0x2 
#define BAN_AHOSTS 0x4
	Timer *timer; 
	struct baninfo *next;
	struct baninfo *last;
} aBan;

typedef struct joininfo {
   aNChan *chan;
   aNick *nick;
   struct joininfo *next; 
   struct SLink *link; 
   unsigned short status;
#define J_OP		0x01
#define J_HALFOP	0X02
#define J_VOICE		0x04
#define J_BURST		0x08
#define J_CREATE	0x10
} aJoin;

typedef struct Lang {
        char langue[LANGLEN + 1];
        char *msg[LANGMSGNB];
        int id;
        struct Lang *next;
} Lang;

typedef struct HelpBuf { 
        char **buf; 
        int count; 
} HelpBuf; 
    
/*------------------- Structures flood et kill -------------------*/

enum KillType {TIMER_CHNICK, TIMER_KREGNICK};

typedef struct killinfo {
	aNick *nick;
	enum KillType type;
	struct killinfo *next;
	struct killinfo *last;
} aKill;

struct ignore {
	char host[BASE64LEN + 1];
	time_t expire;
	struct ignore *next;
	struct ignore *last;
};

/*------------------- Structures parses et cmds -------------------*/

typedef struct aHashCmd {
	char corename[CMDLEN + 1];
	char name[CMDLEN + 1];
	int (*func) (aNick *, aChan *, int, char **);
	char syntax[SYNTAXLEN + 1];
	int level;
	int args;
	int flag;
#define CMD_CHAN                0x01
#define CMD_ADMIN               0x02
#define CMD_DISABLE     	0x04
#define CMD_NEEDNOAUTH  	0x08
#define CMD_SECURE              0x10
#define CMD_SECURE2     	0x20
#define CMD_HELPER		0x40
#define CMD_MBRSHIP     	0x80
#define CMD_SECURE3     	0x100
#define CMD_INTERNAL (CMD_NEEDNOAUTH|CMD_SECURE|CMD_SECURE2|CMD_CHAN|CMD_MBRSHIP|CMD_SECURE3)
	int used;
	struct aHashCmd *next;
	struct aHashCmd *corenext;
	HelpBuf **help;
} aHashCmd;

/*------------------- Structures pour les Links -------------------*/

typedef struct SLink {
  struct SLink *next;
  struct SLink *last;
  union {
    anAccess *a;
    aJoin *j;
  } value;
} aLink;

struct cntryinfo
{
   char iso[5];
   char cntry[100];
   struct cntryinfo *next;
};
