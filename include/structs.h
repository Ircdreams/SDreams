/* include/structs.h - Déclaration des différentes structures
 *
 * Copyright (C) 2002-2008 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@coderz.info>
 *                         Benjamin Beret <kouak@kouak.org>
 *
 * site web: http://sf.net/projects/scoderz/
 *
 * Services pour serveur IRC. Supporté sur IRCoderz
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
  time_t lasttime; /* pour traffic */
  unsigned long lastbytes;	/*  idem */
  unsigned long dataS; /* bytes envoyées depuis le début */
  unsigned long dataQ; /* bytes reçues */

#ifdef WEB2CS
	int w2c_port;
	int CONtotal;
	char w2c_pass[16 + 1];
	unsigned int WEBtrafficUP;
	unsigned int WEBtrafficDL;
#endif
};

struct bots {
  char nick[NICKLEN + 1];
  char ident[USERLEN + 1];
  char host[HOSTLEN + 1];
  char name[REALEN + 1];
  char mode[10];
  char num[2 * (NUMSERV + 1)];
};

enum TimerType {TIMER_ABSOLU, TIMER_RELATIF, TIMER_PERIODIC};

typedef struct Timer {
	time_t expire;
	time_t delay;
	enum TimerType type;
	int (*callback) (struct Timer *);
	void *data1;
	void *data2;
	struct Timer *last;
	struct Timer *next;
} Timer;

typedef struct aData {
	char from[NICKLEN + 1];
	char raison[RAISONLEN + 1];
	int flag;
#define DATA_RAISON_MANDATORY 	0x01
#define DATA_FREE_ON_DEL 		0x02
#define DATA_T_SUSPEND_CHAN 	0x04
#define DATA_T_SUSPEND_USER 	0x08
#define DATA_T_NOPURGE 			0x10
#define DATA_T_CANTREGCHAN 		0x20

#define DATA_TYPES (DATA_T_SUSPEND_CHAN|DATA_T_SUSPEND_USER| \
	DATA_T_NOPURGE|DATA_T_CANTREGCHAN)

#define DATA_T_NEEDFREE 	(DATA_T_NOPURGE|DATA_T_CANTREGCHAN|DATA_FREE_ON_DEL)
#define DATA_T_NEEDRAISON 	(DATA_T_SUSPEND_USER|DATA_T_SUSPEND_CHAN)

#define DNeedRaison(x) 	((x)->flag & DATA_RAISON_MANDATORY)
#define DNeedFree(x) 	((x)->flag & DATA_T_NEEDFREE)
	time_t debut;
	time_t expire;
	Timer *timer;
	void *data;
} aData;

struct irc_in_addr
{
	unsigned short in6_16[8]; /**< IPv6 encoded parts, little-endian. */
};

/*------------------- Structures informations serveurs -------------------*/

typedef struct Link {
	char serv[HOSTLEN + 1];
	char num[NUMSERV + 1];
	unsigned int maxusers;
	unsigned int smask;
	unsigned int flag;
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
#ifdef HAVE_CRYPTHOST
	char crypt[HOSTLEN + 1];
#endif
	char name[REALEN + 1];
	char base64[BASE64LEN + 1];
	char numeric[2 * (NUMSERV + 1)];
	unsigned int flag;
#define N_REGISTER 	0x00001
#define N_AWAY 		0x00002
#define N_OPER 		0x00004
#define N_SERVICE 	0x00008
#define N_ADM 		0x00010
#define N_INV 		0x00020
#define N_GOD 		0x00040
#define N_WALLOPS 	0x00080
#define N_DEBUG 	0x00100
#define N_FEMME 	0x00200
#define N_HOMME 	0x00400
#define N_HIDE 		0x00800
#define N_REG 		0x01000
#define N_DIE 		0x02000
#define N_DEAF 		0x04000
#define N_SPOOF 	0x08000
#define N_HASKILL 	0x10000
#define N_IP 		0x20000
#define N_UMODES (N_INV |N_HIDE|N_REG|N_OPER|N_SERVICE|N_ADM|N_GOD \
					|N_FEMME|N_HOMME|N_DEBUG|N_WALLOPS|N_DIE|N_DEAF|N_SPOOF)
	time_t ttmco;
	time_t floodtime;
	unsigned int floodcount;
	Timer *timer;
	aServer *serveur;
	struct userinfo *user;
	struct nickinfo *next;
	struct joininfo *joinhead;
	struct irc_in_addr addr_ip;
} aNick;

typedef struct userinfo {
	char nick[NICKLEN + 1];
	char passwd[PWDLEN + 1];
	int level;
	time_t lastseen;
	time_t reg_time;
	int flag;
	unsigned long userid;
#define U_PKILL		 		0x00001 /* nickserv */
#define U_PNICK		 		0x00002 /* nickserv */
#define U_SUSPEND	 		0x00004
#define U_NOPURGE	 		0x00008
#define U_WANTX		 		0x00010
#define U_OUBLI		 		0x00020
#define U_FIRST 	 		0x00040
#define U_NOMEMO	 		0x00080
#define U_PREJECT	 		0x00100
#define U_POKACCESS	 		0x00200
#define U_PACCEPT	 		0x00400
#define U_ALREADYCHANGE 	0x00800
#define U_ADMBUSY 			0x01000
#define U_MD5PASS 			0x02000
#define U_HASVOTE 			0x04000
#define U_NOVOTE 			0x08000
#define U_PMREPLY 			0x10000
#define U_CANTREGCHAN 		0x20000
#define U_TRACKED 			0x40000

/* All user flags */
#define U_ALL (U_PKILL|U_PNICK|U_SUSPEND|U_NOPURGE|U_WANTX|U_OUBLI|U_FIRST| \
				U_NOMEMO|U_PREJECT|U_POKACCESS|U_PACCEPT|U_ALREADYCHANGE|U_TRACKED| \
				U_ADMBUSY|U_MD5PASS|U_HASVOTE|U_NOVOTE|U_PMREPLY|U_CANTREGCHAN)

/* Flags that user cannot set/unset (admin or internal only) */
#define U_BLOCKED (U_TRACKED|U_SUSPEND|U_NOPURGE|U_MD5PASS|U_ALREADYCHANGE|U_ADMBUSY|U_CANTREGCHAN)

/* Flags for runtime only (should not be written) */
#define U_INTERNAL (U_TRACKED)

/* Access policy flags */
#define U_ACCESS_T 		(U_PREJECT|U_POKACCESS|U_PACCEPT)

/* Nick enforcement policy */
#define U_PROTECT_T 	(U_PKILL|U_PNICK)

	char mail[MAILLEN + 1];
	char *lastlogin;
	char *cookie;

	struct Lang *lang;
	aData *suspend, *nopurge, *cantregchan;
	struct access *accesshead;
	aNick *n;
#ifdef USE_MEMOSERV
	struct memos *memohead, *memotail;
#endif
	struct userinfo *next, *mailnext, *idnext;
} anUser;

typedef struct access {
   struct chaninfo *c; /* pour rediriger vers la struct du chan où on a access.*/
   int level;
   int flag;
#define A_OP		 0x01
#define A_VOICE		 0x02
#define A_PROTECT	 0x04
#define A_SUSPEND	 0x08
#define A_WAITACCESS 0x10
   time_t lastseen;
   char *info;
   struct userinfo *user;
   struct access *next;
} anAccess;

#ifdef USE_MEMOSERV
typedef struct memos {
   char de[NICKLEN + 1];
   char message[MEMOLEN + 1];
   time_t date;
   int flag;
#define MEMO_READ 		0x1
#define MEMO_AUTOEXPIRE 0x2
   struct memos *next, *last;
} aMemo;
#endif

/*------------------- Structures chans -------------------*/

struct cmode {
#define C_MMSG 			0x0001 /* +n */
#define C_MTOPIC 		0x0002 /* +t */
#define C_MINV 			0x0004 /* +i */
#define C_MLIMIT 		0x0008 /* +l */
#define C_MKEY 			0x0010 /* +k */
#define C_MSECRET 		0x0020 /* +s */
#define C_MPRIVATE 		0x0040 /* +p */
#define C_MMODERATE 	0x0080 /* +m */
#define C_MNOCTRL 		0x0100 /* +c */
#define C_MNOCTCP 		0x0200 /* +C */
#define C_MOPERONLY 	0x0400 /* +O */
#define C_MUSERONLY 	0x0800 /* +r */
#define C_MDELAYJOIN 	0x1000 /* +D */
#define C_MNONOTICE 	0x2000 /* +N */
#define C_MAPASS 		0x4000 /* +A */
#define C_MUPASS  		0x8000 /* +U */
	unsigned int modes; /* modes sous forme de champ de bits */
	unsigned int limit;
	char key[KEYLEN + 1];
};

typedef struct chaninfo {
	char chan[REGCHANLEN + 1];
	struct cmode defmodes; /* propriétés des salons regs */
	char deftopic[TOPICLEN + 1];
	char welcome[TOPICLEN + 1];
	char description[DESCRIPTIONLEN + 1];
	char url[URLLEN + 1];
	char *motd;
	int flag;
#define C_SETWELCOME	0x0001
#define C_JOINED		0x0002
#define C_STRICTOP		0x0004
#define C_LOCKTOPIC		0x0008
#define C_NOBANS		0x0010
#define C_WARNED		0x0020
#define C_NOOPS			0x0040
#define C_AUTOINVITE 	0x0080
#define C_ALREADYRENAME 0x0100
#define C_FLIMIT 		0x0200
#define C_NOINFO 		0x0400
#define C_NOVOICES 		0x0800
#define C_SUSPEND 		0x1000

	int banlevel;
	int cml;
	int bantype;
	unsigned int limit_inc;
	unsigned int limit_min;
	time_t bantime;
	time_t creation_time;
	time_t lastact;
	anAccess *owner;
	Timer *fltimer; /* floating limit */
	aData *suspend;
	struct baninfo *banhead;
	struct SLink *access; /* utiliser un link pour avoir la liste des access plus rapidement*/
	struct chaninfo *next;
	struct NChan *netchan;
} aChan;

typedef struct NChan {
	char chan[CHANLEN + 1];/* infos sur le salon actuel du réseau */
	char topic[TOPICLEN + 1];
	struct cmode modes;
	time_t timestamp;
	unsigned int users;
#ifdef HAVE_OPLEVELS
	int flags;
#	define NC_ZANNEL 0x1
#	define IsZannel(x) 		((x)->flags & NC_ZANNEL)
#	define SetZannel(x) 	((x)->flags |= NC_ZANNEL)
#	define DelZannel(x) 	((x)->flags &= ~NC_ZANNEL)
#endif
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
#define BAN_ANICKS 	0x1
#define BAN_AUSERS 	0x2
#define BAN_AHOSTS 	0x4
#define BAN_IP 		0x8
	struct irc_in_addr ipmask;
	unsigned char cbits;
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
#define J_OP 		0x01
#define J_VOICE 	0x02
#define J_BURST 	0x04
#define J_CREATE 	0x08
#define J_MANAGER 	0x10
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

/*------------------- Structures kill -------------------*/

#ifdef USE_NICKSERV
enum KillType {TIMER_CHNICK, TIMER_KREGNICK};

typedef struct killinfo {
	aNick *nick;
	enum KillType type;
	struct killinfo *next;
	struct killinfo *last;
} aKill;
#endif

/*------------------- Structures parses et cmds -------------------*/

typedef struct aHashCmd {
	char corename[CMDLEN + 1];
	char name[CMDLEN + 1];
	int (*func) (aNick *, aChan *, int, char **);
	char syntax[SYNTAXLEN + 1];
	int level;
	int args;
	int flag;
#define CMD_CHAN 		0x01
#define CMD_ADMIN 		0x02
#define CMD_DISABLE 	0x04
#define CMD_NEEDNOAUTH 	0x08
#define CMD_SECURE 		0x10
#define CMD_SECURE2 	0x20
#define CMD_MBRSHIP 	0x40
#define CMD_SECURE3 	0x80
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
