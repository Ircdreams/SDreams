/* include/main.h - Déclarations principales
 * Copyright (C) 2004-2006 ircdreams.org
 *
 * contact: bugs@ircdreams.org
 * site web: http://www.ircdreams.org
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
 * $Id: main.h,v 1.225 2006/03/15 06:43:23 bugs Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include "mystring.h"
#include "../config.h"

#ifdef USEBSD
#       include <sys/types.h>
#	include <sys/select.h>
#endif

#include "lang.h"

/* Variables */

#define CTCP_DELIM_CHAR '\001'
#define NICKLEN 45
#define TOPICLEN 250
#define HOSTLEN 63
#define REALEN 50
#define USERLEN 10
#define KEYLEN 23
#define CHANLEN 200
#define REGCHANLEN 100
#define DESCRIPTIONLEN 80
#define CMDLEN 14
#define SYNTAXLEN 100
#define NUHLEN (NICKLEN + USERLEN + HOSTLEN + 3)
#define MAILLEN 70
#define PWDLEN 16
#define TIMELEN 40 /* wednesday, 2000-12-12 12:12:12 */
#define RAISONLEN 300
#define SWHOISLEN 50
#define URLLEN                  80
#define LANGLEN                 15

#define DOTTEDIPLEN      15
#define BASE64LEN        6

#define MAXADMLVL 	7 /* level max des admins */
#define ADMINLEVEL 	3 /* level min des admins, level entre USER&ADMIN LEVEL ~= Helpers? */
#define HELPLEVEL	2

#define OWNERLEVEL 500 /* vous pouvez définir le niveau max dans un salon, mais, après ne le changer pas
						si vous avez une db active !! vous feriez ignorer tous les accès owner. */

/*si vous ne voulez pas tout logger, mettez rien entre les "" des différents fichiers*/
#define DNR_FILE DBDIR"/dnr.db"
#define CMDS_FILE DBDIR"/cmds.db"
#define SDREAMS_PID "sdreams.pid" /* fichier où sera stocké le n° de pid de sdreams */

#define DBUSERS "users.db" 
#define DBCHANS "channels.db"
#define MAXUSER "maxuser.db"

#define WAIT_CONNECT 10 /* Temps qu'attend le serveur avant de retenter une connexion */

#define MODE_OBVH       0x1 /* mode string is only some of +obvh */
#define MODE_ALLFMT 0x2 /* needs full *snprintf formatting */

#define JOIN_FORCE      0x1 /* join channel even if empty */
#define JOIN_REG        0x2 /* this join is due to regchan */
#define JOIN_TOPIC	0x4 /* try to reset the correct topic */
#define JOIN_CREATE     0x8 /* create channel */

#define IsAdmin(x)              ((x)->level >= ADMINLEVEL)
#define IsAnAdmin(x)    	((x) && IsAdmin(x)) /* x est un admin ? */
#define IsOper(x) 		((x)->flag & N_OPER)
#define IsService(x)		((x)->flag & N_SERVICE)
#define IsOperOrService(x)      ((x)->flag & (N_OPER|N_SERVICE))
#define IsAway(x) 		((x)->flag & N_AWAY)
#define IsHelper(x)		((x)->flag & N_HELPER)
#define IsAnHelper(x)		((x) && ((x)->level == HELPLEVEL))
#define IsHiding(x)		((x)->flag & N_HIDE)
#define IsHidden(x)             ((x)->flag & (N_HIDE|N_SPOOF)) /* +x || +H */

#define MAXNUM 4096
#define NUMSERV 2

#include <structs.h>

#define AOp(x)		 ((x)->flag & A_OP)
#define AHalfop(x)	 ((x)->flag & A_HALFOP)
#define AVoice(x)	 ((x)->flag & A_VOICE)
#define ASuspend(x)	 ((x)->flag & A_SUSPEND)
#define AProtect(x)	 ((x)->flag & A_PROTECT)
#define AWait(x)	 ((x)->flag & A_WAITACCESS)
#define AOnChan(x)       ((x)->lastseen == 1)
#define SetAOp(x)	 ((x)->flag |= A_OP)
#define SetHalfop(x)	 ((x)->flag |= A_HALFOP)
#define SetAVoice(x)	 ((x)->flag |= A_VOICE)
#define SetAProtect(x)	 ((x)->flag |= A_PROTECT)
#define SetASuspend(x)	 ((x)->flag |= A_SUSPEND)
#define DelAOp(x)	 ((x)->flag &= ~A_OP)
#define DelHalfop(x)	 ((x)->flag &= ~A_HALFOP)
#define DelAVoice(x)	 ((x)->flag &= ~A_VOICE)
#define DelAProtect(x)	 ((x)->flag &= ~A_PROTECT)
#define DelASuspend(x)	 ((x)->flag &= ~A_SUSPEND)
#define AOwner(x)        ((x)->level == OWNERLEVEL)

#define NRegister(x)	 ((x)->flag & N_REGISTER)
#define SetNRegister(x)	 ((x)->flag |= N_REGISTER)
#define DelNRegister(x)	 ((x)->flag &= ~N_REGISTER)
#define NHasKill(x)      ((x)->timer) 

#define UPKill(x)	 ((x)->flag & U_PKILL)
#define UPNick(x)	 ((x)->flag & U_PNICK)
#define SetUPKill(x)	 ((x)->flag |= U_PKILL)
#define SetUPNick(x)	 ((x)->flag |= U_PNICK)
#define DelUPKill(x)	 ((x)->flag &= ~U_PKILL)
#define DelUPNick(x)	 ((x)->flag &= ~U_PNICK)
#define IsProtected(x)	 ((x)->flag & (U_PNICK | U_PKILL))

#define UWantDrop(x)	 ((x)->flag & U_WANTDROP)
#define UWantX(x)	 ((x)->flag & U_WANTX)
#define UNopurge(x)	 ((x)->flag & U_NOPURGE)
#define UOubli(x)	 ((x)->flag & U_OUBLI)
#define UFirst(x)	 ((x)->flag & U_FIRST)
#define UNoMemo(x)	 ((x)->flag & U_NOMEMO)
#define UIsBusy(x) 	 ((x)->flag & U_ADMBUSY)
#define UVhost(x)	 ((x)->flag & U_VHOST)
#define USWhois(x)	 ((x)->flag & U_SWHOIS)
#define UMD5(x)		 ((x)->flag & U_MD5PASS)
#define UPReject(x)	 ((x)->flag & U_PREJECT)
#define UPAccept(x) 	 ((x)->flag & U_PACCEPT)
#define UPAsk(x) 	 ((x)->flag & U_POKACCESS)
#define UVote(x)         ((x)->flag & U_HASVOTE)
#define UNoVote(x)       ((x)->flag & U_NOVOTE)
#define UPMReply(x)      ((x)->flag & U_PMREPLY)
#define UNoMail(x)	 ((x)->flag & U_NOMAIL)
#define UMale(x)	 ((x)->flag & U_MALE)
#define UFemelle(x)	 ((x)->flag & U_FEMELLE)
#define URealHost(x)     ((x)->flag & U_REALHOST)
#define SetUWantX(x)	 ((x)->flag |= U_WANTX)
#define SetUNopurge(x)	 ((x)->flag |= U_NOPURGE)
#define SetUOubli(x)	 ((x)->flag |= U_OUBLI)
#define SetUFirst(x)	 ((x)->flag |= U_FIRST)
#define SetUNoMemo(x)	 ((x)->flag |= U_NOMEMO)
#define SetUVhost(x)	 ((x)->flag |= U_VHOST)
#define SetUMD5(x)	 ((x)->flag |= U_MD5PASS)
#define SetUVote(x)      ((x)->flag |= U_HASVOTE)
#define SetUNomail(x)	 ((x)->flag |= U_NOMAIL)
#define SetUSWhois(x)	 ((x)->flag |= U_SWHOIS)
#define SetUMale(x)      ((x)->flag |= U_MALE)
#define SetUFemelle(x)   ((x)->flag |= U_FEMELLE)
#define SetURealHost(x)  ((x)->flag |= U_REALHOST)
#define DelUWantX(x)	 ((x)->flag &= ~U_WANTX)
#define DelUNopurge(x)	 ((x)->flag &= ~U_NOPURGE)
#define DelUOubli(x)	 ((x)->flag &= ~U_OUBLI)
#define DelUFirst(x)	 ((x)->flag &= ~U_FIRST)
#define DelUNoMemo(x)	 ((x)->flag &= ~U_NOMEMO)
#define DelUVhost(x)	 ((x)->flag &= ~U_VHOST)
#define DelUVote(x)      ((x)->flag &= ~U_HASVOTE)
#define DelUMail(x)	 ((x)->flag &= ~U_NOMAIL)
#define DelUSWhois(x)	 ((x)->flag &= ~U_SWHOIS)
#define DelUMale(x)      ((x)->flag &= ~U_MALE)
#define DelUFemelle(x)   ((x)->flag &= ~U_FEMELLE)
#define DelURealHost(x)  ((x)->flag &= ~U_REALHOST)

#define CAutoVoice(x)           ((x)->flag & C_AUTOVOICE)
#define CSetWelcome(x)		((x)->flag & C_SETWELCOME)
#define CJoined(x)		((x)->flag & C_JOINED)
#define CStrictOp(x)		((x)->flag & C_STRICTOP)
#define CLockTopic(x)		((x)->flag & C_LOCKTOPIC)
#define CNoBans(x)		((x)->flag & C_NOBANS)
#define CWarned(x)              ((x)->flag & C_WARNED)
#define CNoOps(x)		((x)->flag & C_NOOPS)
#define CAutoInvite(x)		((x)->flag & C_AUTOINVITE)
#define CFLimit(x)		((x)->flag & C_FLIMIT)
#define CNoInfo(x)              ((x)->flag & C_NOINFO)
#define CNoPubCmd(x)		((x)->flag & C_NOPUBCMD)
#define CNoVoices(x)            ((x)->flag & C_NOVOICES)
#define CNoHalfops(x)		((x)->flag & C_NOHALFOPS)
#define SetCAutoVoice(x)        ((x)->flag |= C_AUTOVOICE)
#define SetCSetWelcome(x)	((x)->flag |= C_SETWELCOME)
#define SetCJoined(x)		((x)->flag |= C_JOINED)
#define SetCStrictOp(x)		((x)->flag |= C_STRICTOP)
#define SetCLockTopic(x)	((x)->flag |= C_LOCKTOPIC)
#define SetCNoBans(x)		((x)->flag |= C_NOBANS)
#define SetCWarned(x)           ((x)->flag |= C_WARNED)
#define SetCNoOps(x)		((x)->flag |= C_NOOPS)
#define SetCNoInfo(x)           ((x)->flag |= C_NOINFO)
#define SetCNoPubCmd(x)         ((x)->flag |= C_NOPUBCMD)
#define SetCNoVoices(x)         ((x)->flag |= C_NOVOICES)
#define SetCNoHalfops(x)	((x)->flag |= C_NOHALFOPS)
#define DelCAutoVoice(x)        ((x)->flag &= ~C_AUTOVOICE)
#define DelCSetWelcome(x)	((x)->flag &= ~C_SETWELCOME)
#define DelCJoined(x)		((x)->flag &= ~C_JOINED)
#define DelCStrictOp(x)		((x)->flag &= ~C_STRICTOP)
#define DelCLockTopic(x)	((x)->flag &= ~C_LOCKTOPIC)
#define DelCNoBans(x)		((x)->flag &= ~C_NOBANS)
#define DelCWarned(x)           ((x)->flag &= ~C_WARNED)
#define DelCNoOps(x)		((x)->flag &= ~C_NOOPS)
#define DelCNoInfo(x)           ((x)->flag &= ~C_NOINFO)
#define DelCNoPubCmd(x)         ((x)->flag &= ~C_NOPUBCMD)
#define DelCNoVoices(x)         ((x)->flag &= ~C_NOVOICES)
#define DelCNoHalfops(x)	((x)->flag &= ~C_NOHALFOPS)

#define IsOp(x) 		((x)->status & J_OP)
#define IsHalfop(x)		((x)->status & J_HALFOP)
#define IsVoice(x) 		((x)->status & J_VOICE)
#define DoOp(x) 		((x)->status |= J_OP)
#define DoHalfop(x)		((x)->status |= J_HALFOP)
#define DoVoice(x) 		((x)->status |= J_VOICE)
#define DeOp(x) 		((x)->status &= ~J_OP)
#define DeHalfop(x)		((x)->status &= ~J_HALFOP)
#define DeVoice(x) 		((x)->status &= ~J_VOICE)

#define HasMode(chan, flag)     ((chan)->modes.modes & (flag))
#define HasDMode(chan, flag)    ((chan)->defmodes.modes & (flag))
#define ChanCmd(x) 		((x)->flag & CMD_CHAN)
#define AdmCmd(x) 		((x)->flag & CMD_ADMIN)
#define NeedNoAuthCmd(x)        ((x)->flag & CMD_NEEDNOAUTH)
#define SecureCmd(x)            ((x)->flag & CMD_SECURE) 
#define Secure2Cmd(x)           ((x)->flag & CMD_SECURE2) 
#define Secure3Cmd(x)           ((x)->flag & CMD_SECURE3)
#define HelpCmd(x)		((x)->flag & CMD_HELPER)
#define NeedMemberShipCmd(x)    ((x)->flag & CMD_MBRSHIP)

#define GetConf(x) 				(ConfFlag & (x))
#define ASIZE(x) 				(sizeof (x) / sizeof *(x))

#define IsSuspend(x) ((x)->suspend && (!(x)->suspend->expire ||\
				(x)->suspend->expire > CurrentTS))
#define CantRegChan(x) (!(x)->cantregchan || (x)->cantregchan > CurrentTS)

#define PLUR(x)  	((x) > 1 ? "s" : "")
#define PLURX(x) 	((x) > 1 ? "x" : "")
#define NONE(x) 	((x) ? (x) : "")
#define UNDEF(x)        ((x) ? get_time(nick, (x)) : "- Non défini -")

#define IsBadNick(nick)                 find_dnr(nick, DNR_TYPEUSER|DNR_MASK)
#define IsBadChan(chan)                 find_dnr(chan, DNR_TYPECHAN|DNR_MASK)
#define HasWildCard(string) 	(strchr((string), '*') || strchr((string), '?'))

#define SPECIAL_CHAR "{}[]|-_\\^`.()<>"

/* global vars */
extern char user_motd[];
extern char admin_motd[];
extern struct robot bot;
extern int ConfFlag;
extern struct bots cs;
extern aHashCmd *cmd_hash[];
extern anUser *user_tab[];
extern aChan *chan_tab[];
extern aNChan *nchan_tab[];
extern aNick **num_tab[];
extern aServer *serv_tab[];
extern aNick *nick_tab[];

extern int running;
extern int deconnexion;
extern int complete;
extern time_t CurrentTS;
extern int nbmaxuser;
extern int nbuser;
extern int burst;

extern aKill *killhead;
extern aDNR *dnrhead;
extern struct ignore *ignorehead;
extern Timer *Timers;
extern struct nickinfo *nickhead;
extern struct cntryinfo *cntryhead;

extern Lang *DefaultLang;
extern int LangCount;
extern int CmdsCount; 

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

#define U_DEFAULT (U_POKACCESS | U_WANTX | U_PNICK | U_MD5PASS)
