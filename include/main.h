/* include/main.h - Déclarations principales
 *
 * Copyright (C) 2002-2008 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@coderz.info>
 *                         Benjamin Beret <kouak@kouak.org>
 *
 * SDreams v2 (C) 2021 -- Ext by @bugsounet <bugsounet@bugsounet.fr>
 * site web: http://www.ircdreams.org
 *
 * Services pour serveur IRC. Supporté sur Ircdreams v3
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>
#include "mystring.h"

#include "../config.h"

#ifdef USEBSD
#	include <sys/types.h>
#endif

#include "lang.h"

/*#define OLDADMINLIST*/
/*#define HAVE_IPV6*/

#ifdef WEB2CS
#	define SPVERSION "2.0.0-DEV(Web)"
#	ifdef USEBSD
#		include <sys/select.h>
#	endif
#else
#	define SPVERSION "2.0.0-DEV"
#endif

#define NNICK "AA"
#define CTCP_DELIM_CHAR '\001'

#define NICKLEN 		30
#define TOPICLEN 		250
#define HOSTLEN 		63
#define REALEN 			50
#define USERLEN 		10
#define KEYLEN 			23
#define CHANLEN 		200
#define REGCHANLEN 		30 /* Max length in use is 22... */
#define DESCRIPTIONLEN 	80
#define CMDLEN 			14
#define SYNTAXLEN 		100
#define NUHLEN 			(NICKLEN + USERLEN + HOSTLEN + 3) /* 3 stands for '!' + '@' + '\0' */
#define MAILLEN 		70 /* should be enough.. */
#define PWDLEN 			16 /* Our MD5 hash length */
#define TIMELEN 		40 /* wednesday, 2000-12-12 12:12:12 */
#define RAISONLEN 		300
#define URLLEN 			80
#define LANGLEN 		15

#ifdef HAVE_IPV6
#	define DOTTEDIPLEN 	45
#	define BASE64LEN 	25
#else
#	define DOTTEDIPLEN 	15
#	define BASE64LEN 	6
#endif

#define MAXADMLVL 	7 /* level max des admins */
#define ADMINLEVEL 	3 /* level min des admins, level entre USER&ADMIN LEVEL ~= Helpers? */
#define HELPLEVEL	2

#define OWNERLEVEL 500 /* Vous pouvez modifier ici le niveau du propriétaire des salons
						* mais ne le changez plus une fois que vous avez une DB active,
						* vous perderiez tous les salons */

#define DNR_FILE DBDIR"/dnr.db"
#define CMDS_FILE DBDIR"/cmds.db"
#define SDREAMS_PID "sdreams.pid" /* fichier où sera stocké le n° de pid de scoderz */
#define PURGEDELAY 86400 /* Temps entre deux purges des users/chans */

#define	DBUSERS "users.db"
#define	DBCHANS	"channels.db"

#define WAIT_CONNECT 10 /* Temps qu'attend le serveur avant de retenter une connexion */

#define MODE_OBV 	0x1 /* mode string is only some of +obv */
#define MODE_ALLFMT 0x2 /* needs full *snprintf formatting */

#define JOIN_FORCE 	0x1 /* join channel even if empty */
#define JOIN_REG 	0x2 /* this join is due to regchan */
#define JOIN_TOPIC 	0x4 /* try to reset the correct topic */
#define JOIN_CREATE 0x8 /* create channel */

#define IsAdmin(x) 			((x)->level >= ADMINLEVEL)
#define IsAnAdmin(x) 		((x) && IsAdmin(x)) /* x est un admin ? */
#define IsOper(x) 			((x)->flag & N_OPER)
#define IsOperOrService(x) 	((x)->flag & (N_OPER|N_SERVICE))
#define IsAway(x) 			((x)->flag & N_AWAY)
#define IsIPv6(x) 			((x)->base64[6]) /* base64 is 6 char long */
#define IsHidden(x) 		((x)->flag & (N_HIDE|N_SPOOF)) /* +x || +H */
#define IsIP(x) 			((x)->flag & N_IP)

#define MAXNUM 4096
#define NUMSERV 2

#ifdef PUREIRCU
#	define OPPREFIX bot.servnum
#else
#	define OPPREFIX cs.num
#endif

#include <structs.h>

#define AOp(x)		 	((x)->flag & A_OP)
#define AVoice(x)	 	((x)->flag & A_VOICE)
#define ASuspend(x)	 	((x)->flag & A_SUSPEND)
#define AProtect(x)	 	((x)->flag & A_PROTECT)
#define AWait(x) 		((x)->flag & A_WAITACCESS)
#define AOnChan(x) 		((x)->lastseen == 1)
#define SetAOp(x)	 	((x)->flag |= A_OP)
#define SetAVoice(x) 	((x)->flag |= A_VOICE)
#define SetAProtect(x) 	((x)->flag |= A_PROTECT)
#define SetASuspend(x) 	((x)->flag |= A_SUSPEND)
#define DelAOp(x)	 	((x)->flag &= ~A_OP)
#define DelAVoice(x) 	((x)->flag &= ~A_VOICE)
#define DelAProtect(x) 	((x)->flag &= ~A_PROTECT)
#define DelASuspend(x) 	((x)->flag &= ~A_SUSPEND)
#define AOwner(x) 		((x)->level == OWNERLEVEL)

#define NRegister(x)	 ((x)->flag & N_REGISTER)
#define SetNRegister(x)	 ((x)->flag |= N_REGISTER)
#define DelNRegister(x)	 ((x)->flag &= ~N_REGISTER)
#define NHasKill(x)  	 ((x)->timer)

#ifdef USE_NICKSERV
#define UPKill(x)	 ((x)->flag & U_PKILL)
#define UPNick(x)	 ((x)->flag & U_PNICK)
#define SetUPKill(x)	 ((x)->flag |= U_PKILL)
#define SetUPNick(x)	 ((x)->flag |= U_PNICK)
#define DelUPKill(x)	 ((x)->flag &= ~U_PKILL)
#define DelUPNick(x)	 ((x)->flag &= ~U_PNICK)
#define IsProtected(x)	 ((x)->flag & (U_PNICK | U_PKILL))
#endif /* USE_NICKSERV */

#define USuspend(x)	 	((x)->flag & U_SUSPEND)
#define UWantX(x)	 	((x)->flag & U_WANTX)
#define UNopurge(x)	 	((x)->flag & U_NOPURGE)
#define UOubli(x)	 	((x)->flag & U_OUBLI)
#define UFirst(x)	 	((x)->flag & U_FIRST)
#define UNoMemo(x)	 	((x)->flag & U_NOMEMO)
#define UIsBusy(x) 	 	((x)->flag & U_ADMBUSY)
#define UMD5(x)		 	((x)->flag & U_MD5PASS)
#define UPReject(x) 	((x)->flag & U_PREJECT)
#define UPAccept(x) 	((x)->flag & U_PACCEPT)
#define UPAsk(x) 		((x)->flag & U_POKACCESS)
#define UVote(x) 		((x)->flag & U_HASVOTE)
#define UNoVote(x) 		((x)->flag & U_NOVOTE)
#define UPMReply(x) 	((x)->flag & U_PMREPLY)
#define UChanged(x) 	((x)->flag & U_ALREADYCHANGE)
#define UCantRegChan(x) ((x)->flag & U_CANTREGCHAN)
#define UCantRegChan(x) ((x)->flag & U_CANTREGCHAN)
#define UTracked(x) 	((x)->flag & U_TRACKED)

#define SetUSuspend(x) 		((x)->flag |= U_SUSPEND)
#define SetUChanged(x) 	 	((x)->flag |= U_ALREADYCHANGE)
#define SetUWantX(x)	 	((x)->flag |= U_WANTX)
#define SetUNopurge(x)		((x)->flag |= U_NOPURGE)
#define SetUOubli(x)	 	((x)->flag |= U_OUBLI)
#define SetUFirst(x)	 	((x)->flag |= U_FIRST)
#define SetUNoMemo(x)	 	((x)->flag |= U_NOMEMO)
#define SetUMD5(x)	 	 	((x)->flag |= U_MD5PASS)
#define SetUVote(x)	 	 	((x)->flag |= U_HASVOTE)
#define SetUCantRegChan(x)  ((x)->flag |= U_CANTREGCHAN)
#define SetUTracked(x) 		((x)->flag |= U_TRACKED)

#define DelUSuspend(x)	 	((x)->flag &= ~U_SUSPEND)
#define DelUWantX(x)	 	((x)->flag &= ~U_WANTX)
#define DelUNopurge(x)	 	((x)->flag &= ~U_NOPURGE)
#define DelUOubli(x)	 	((x)->flag &= ~U_OUBLI)
#define DelUFirst(x)	 	((x)->flag &= ~U_FIRST)
#define DelUNoMemo(x)	 	((x)->flag &= ~U_NOMEMO)
#define DelUVote(x) 	 	((x)->flag &= ~U_HASVOTE)
#define DelUChanged(x) 	 	((x)->flag &= ~U_ALREADYCHANGE)
#define DelUCantRegChan(x) 	((x)->flag &= ~U_CANTREGCHAN)
#define DelUTracked(x) 		((x)->flag &= ~U_TRACKED)

#define CSetWelcome(x)		((x)->flag & C_SETWELCOME)
#define CJoined(x)			((x)->flag & C_JOINED)
#define CStrictOp(x)		((x)->flag & C_STRICTOP)
#define CLockTopic(x)		((x)->flag & C_LOCKTOPIC)
#define CNoBans(x)			((x)->flag & C_NOBANS)
#define CWarned(x)			((x)->flag & C_WARNED)
#define CNoOps(x)			((x)->flag & C_NOOPS)
#define CAutoInvite(x)		((x)->flag & C_AUTOINVITE)
#define CFLimit(x)			((x)->flag & C_FLIMIT)
#define CNoInfo(x) 			((x)->flag & C_NOINFO)
#define CNoVoices(x)		((x)->flag & C_NOVOICES)
#define CSuspend(x) 		((x)->flag & C_SUSPEND)
#define SetCSetWelcome(x)	((x)->flag |= C_SETWELCOME)
#define SetCJoined(x)		((x)->flag |= C_JOINED)
#define SetCStrictOp(x)		((x)->flag |= C_STRICTOP)
#define SetCLockTopic(x)	((x)->flag |= C_LOCKTOPIC)
#define SetCNoBans(x)		((x)->flag |= C_NOBANS)
#define SetCWarned(x)		((x)->flag |= C_WARNED)
#define SetCNoOps(x)		((x)->flag |= C_NOOPS)
#define SetCNoInfo(x) 		((x)->flag |= C_NOINFO)
#define SetCNoVoices(x)		((x)->flag |= C_NOVOICES)
#define SetCSuspend(x)		((x)->flag |= C_SUSPEND)

#define DelCSetWelcome(x)	((x)->flag &= ~C_SETWELCOME)
#define DelCJoined(x)		((x)->flag &= ~C_JOINED)
#define DelCStrictOp(x)		((x)->flag &= ~C_STRICTOP)
#define DelCLockTopic(x)	((x)->flag &= ~C_LOCKTOPIC)
#define DelCNoBans(x)		((x)->flag &= ~C_NOBANS)
#define DelCWarned(x)		((x)->flag &= ~C_WARNED)
#define DelCNoOps(x)		((x)->flag &= ~C_NOOPS)
#define DelCNoInfo(x) 		((x)->flag &= ~C_NOINFO)
#define DelCNoVoices(x)		((x)->flag &= ~C_NOVOICES)
#define DelCSuspend(x)		((x)->flag &= ~C_SUSPEND)

#define IsOp(x) 		((x)->status & J_OP)
#define IsVoice(x) 		((x)->status & J_VOICE)
#define DoOp(x) 		((x)->status |= J_OP)
#define DoVoice(x) 		((x)->status |= J_VOICE)
#define DeOp(x) 		((x)->status &= ~J_OP)
#define DeVoice(x) 		((x)->status &= ~J_VOICE)

#define HasMode(chan, flag) 	((chan)->modes.modes & (flag))
#define HasDMode(chan, flag) 	((chan)->defmodes.modes & (flag))

#define ChanCmd(x) 				((x)->flag & CMD_CHAN)
#define AdmCmd(x) 				((x)->flag & CMD_ADMIN)
#define NeedNoAuthCmd(x)		((x)->flag & CMD_NEEDNOAUTH)
#define SecureCmd(x) 			((x)->flag & CMD_SECURE)
#define Secure2Cmd(x) 			((x)->flag & CMD_SECURE2)
#define Secure3Cmd(x) 			((x)->flag & CMD_SECURE3)
#define NeedMemberShipCmd(x) 	((x)->flag & CMD_MBRSHIP)
#define DisableCmd(x) 			((x)->flag & CMD_DISABLE)
#define IsCTCP(x) 				(*(x)->name == CTCP_DELIM_CHAR)

#define ASIZE(x) 				(sizeof (x) / sizeof *(x))

#define PLUR(x)  	((x) > 1 ? "s" : "")
#define PLURX(x) 	((x) > 1 ? "x" : "")
#define NONE(x) 	((x) ? (x) : "")
#define UNDEF(x) 	((x) ? get_time(nick, (x)) : "- Non défini -")

#define UserOnChan(user, chan) 	((user)->n && GetJoinIbyNC((user)->n, (chan)->netchan))
#define OnChanTS(user, chan) 	(UserOnChan((user), (chan)) ? 1 : CurrentTS)

#define SPECIAL_CHAR "{}[]|-_\\^`"

/* global vars */

#ifdef USE_WELCOMESERV
extern struct bots ws;
extern char user_motd[];
extern char admin_motd[];
#endif
extern struct robot bot;
extern struct bots cs;

extern aHashCmd *cmd_hash[];
extern anUser *user_tab[];
extern aChan *chan_tab[];
extern aNChan *nchan_tab[];
extern aNick **num_tab[];
extern aServer *serv_tab[MAXNUM];
extern aNick *nick_tab[];

extern int running;
extern time_t CurrentTS;

#ifdef USE_NICKSERV
extern aKill *killhead;
#endif

extern Lang *DefaultLang;

static inline char *GetReply(aNick *nick, int msgid)
{
	return nick->user ? nick->user->lang->msg[msgid] : DefaultLang->msg[msgid];
}

static inline char *GetUReply(anUser *user, int msgid)
{
	return user->lang->msg[msgid];
}


/**** CONFIGURATIONS PAR DEFAUT ****/
/* A METTRE SOUS LA FORME: #define U|C_DEFAULT (FLAG1 | FLAG2 | ... FLAGn) */

/* Salons
	C_SETWELCOME 	envoi du welcome
	C_STRICTOP 		ops avec accès seulement
	C_LOCKTOPIC		topic bloqué
	C_NOBANS		pas de bans possibles
	C_NOOPS			pas de nouveaux ops
	C_AUTOINVITE 	invite auto au login si chan +kirl
	C_FLIMIT 		limite flottante (paramètres définissables plus bas)
*/

#define C_DEFAULT 0
#define C_DEFMODES (C_MMSG | C_MTOPIC)

#define DEFAUT_LIMITINC 4
#define DEFAUT_LIMITMIN 2
#define DEFAUT_BANLEVEL 200
#define DEFAUT_CMODELEVEL 300
#define C_DEFAULT_BANTYPE 4
#define C_DEFAULT_BANTIME 604800

/* Usernames
	U_PKILL 	protection par kill
	U_PNICK		protection par chnick
	U_NOPURGE 	non concerné par l'expiration auto
	U_WANTX		auto +x au login
	U_NOMEMO	rejette les mémos

	1 parmi les 3 suivants pour la politique vis à vis des accès
	U_PREJECT	rejette
	U_POKACCESS	demande confirmation
	U_PACCEPT 	accepte

	Ex:
	#define U_DEFAULT (U_PACCEPT | U_PNICK | U_WANTX)
	mettra par défaut : l'auto acceptation des accès (pas de confirmation nécessaire)
						la protection par changement de pseudo
						la mise du +x automatique au login
*/

#define U_DEFAULT (U_POKACCESS | U_MD5PASS)

#define A_MANAGERLEVEL 450
#define A_MANAGERFLAGS (A_OP|A_PROTECT)
