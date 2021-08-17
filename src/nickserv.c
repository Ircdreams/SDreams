/* src/nickserv.c - Diverses commandes sur le module nickserv
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
 * $Id: nickserv.c,v 1.209 2008/01/05 18:34:13 romexzf Exp $
 */

#include "main.h"
#include "hash.h"
#include "userinfo.h"
#include "admin_manage.h"
#include "chanserv.h"
#include "outils.h"
#include "mylog.h"
#include "cs_cmds.h"
#include "add_info.h"
#include "del_info.h"
#include "config.h"
#include "aide.h"
#include "data.h"
#include "dnr.h"
#ifdef HAVE_VOTE
#include "vote.h"
#endif
#ifdef USE_MEMOSERV
#include "memoserv.h"
#endif
#ifdef HAVE_TRACK
#include "track.h"
#endif
#include "nickserv.h"
#include "crypt.h"
#include "template.h"
#include <ctype.h>

/*
 * oubli_pass <login> <mail>
 */
int oubli_pass(aNick *nick, aChan *chan, int parc, char **parv)
{
	anUser *user;
	char *passwd;

	if(GetConf(CF_NOMAIL)) return csreply(nick, GetReply(nick, L_CMDDISABLE));

	if(nick->user) return csreply(nick, GetReply(nick, L_LOGUEDIN), nick->user->nick);

	if(!(user = getuserinfo(parv[1])))
		return csreply(nick, GetReply(nick, L_NOSUCHUSER), parv[1]);

	if(user->n) return csreply(nick, GetReply(nick, L_ELSEALREADYLOG));

	if(UOubli(user)) return csreply(nick, GetReply(nick, L_CMDALREADYUSED), parv[0]);

	if(strcasecmp(user->mail, parv[2]))
		return csreply(nick, GetReply(nick, L_NOTMATCHINGMAIL), user->nick, parv[2]);

	SetUOubli(user), SetNRegister(nick);
	passwd = create_password(user->passwd);

	if(!tmpl_mailsend(&tmpl_mail_oubli, user->mail, user->nick, passwd, nick->host))
		return csreply(nick, "Impossible d'envoyer le mail");

	csreply(nick, GetReply(nick, L_PASS_SENT), user->mail);

	return 0;
}

static int login_report_failed(anUser *user, aNick *nick, const char *raison)
{
	if(IsAdmin(user))
	{
		const char *nuh = GetNUHbyNick(nick, 0);
		cswall("Admin Login échoué sur \2%s\2 par \2%s\2", user->nick, nuh);
		log_write(LOG_UCMD, 0, "login %s failed (%s) par %s", user->nick, raison, nuh);
	}
	return 0;
}

int ns_login(aNick *nick, aChan *chan, int parc, char **parv)
{
	anUser *user = NULL;
	anAccess *a;

	if(!(user = getuserinfo(parv[1])))
		return csreply(nick, GetReply(nick, L_NOSUCHUSER), parv[1]);

	if(nick->user) /* already logued in */
	{
		csreply(nick, GetReply(nick, L_LOGUEDIN), nick->user->nick);
		return (nick->user != user) ? login_report_failed(user, nick, "Déjà logué") : 0;
	}

	if(!checkpass(parv[2], user)) /* bad password */
	{
		csreply(nick, GetReply(nick, L_BADPASS), user->nick);
		return login_report_failed(user, nick, "Bad password");
	}

	if(user->n && strcasecmp(parv[0], RealCmd("recover"))) /* username in use */
	{
		csreply(nick, GetReply(nick, L_ELSEALREADYLOG));
		csreply(user->n, GetReply(user->n, L_ELSETRYTOLOG), nick->nick, RealCmd("set"));
		return login_report_failed(user, nick, "Déjà utilisé");
	}

	if(USuspend(user)) /* suspended ! heh unbelievable */
	{
		show_ususpend(nick, user);
		return login_report_failed(user, nick, "Suspendu");
	}

#ifdef HAVE_TRACK
	if(UTracked(user))
		track_notify(user, "%s de %s (%s)", parv[0], user->nick, GetNUHbyNick(nick, 0));
#endif

#ifdef USE_NICKSERV  /* arret du kill */
	if(NHasKill(nick) && !strcasecmp(nick->nick, user->nick)) kill_remove(nick);
#endif

	if(UOubli(user)) DelUOubli(user);
	if(UFirst(user))
	{
		csreply(nick, GetReply(nick, L_SERVICESWELCOME), cs.nick, RealCmd("showcommands"));
		DelUFirst(user);
	}
	SetNRegister(nick);	/* pour empecher les register si deauth/drop */

	cs_account(nick, user);
	nick->user->lastseen = CurrentTS;

	csreply(nick, GetReply(nick, L_LOGINSUCCESS), user->nick);

	if(IsAdmin(user))
	{
		csreply(nick, GetReply(nick, L_LOGINADMIN), user->level);
		adm_active_add(nick);
#ifdef USE_WELCOMESERV
		if(*admin_motd) csreply(nick, GetReply(nick, L_LOGINMOTD), admin_motd);
#endif
	}
#ifdef USE_WELCOMESERV
	else if(*user_motd) csreply(nick, GetReply(nick, L_LOGINMOTD), user_motd);
#endif

	if(GetConf(CF_HOSTHIDING) && GetConf(CF_XMODE) && UWantX(user) && !(nick->flag & N_HIDE))
		putserv("%s " TOKEN_SVSMODE" %s +x", cs.num, nick->numeric);

#ifdef USE_MEMOSERV
	show_notes(nick);
#endif

#ifdef HAVE_VOTE
	if(CanVote(user))
	{
		show_vote(nick);
		csreply(nick, "Pour voter tapez \2/%s %s <n° de la proposition>\2.",
			cs.nick, RealCmd("voter"));
	}
#endif

	/* auto-autoop proposé par BugMaster :) */
	for(a = user->accesshead; a; a = a->next)
	{
		aChan *c = a->c;
		aJoin *j = NULL;

		if(AWait(a) || ASuspend(a) || CSuspend(c)) continue;

		if((j = GetJoinIbyNC(nick, c->netchan)))
		{
			enforce_access_opts(c, nick, a, j); /* autoop/voice */
			a->lastseen = 1; /* mark as on chan */
		}

		if(c->motd) csreply(nick, "\2 [Message du jour] - %s -\2 %s", c->chan, c->motd);
		if(!j && CJoined(c) && CAutoInvite(c) && (HasMode(c->netchan, C_MINV|C_MKEY|C_MUSERONLY)
			|| (HasMode(c->netchan, C_MLIMIT) && c->netchan->users >= c->netchan->modes.limit)))
				putserv("%s " TOKEN_INVITE " %s :%s", cs.num, nick->nick, c->chan);
	}
	return 1;
}

#ifdef USE_NICKSERV
int recover(aNick *nick, aChan *chan, int parc, char **parv)
{
	aNick *who, *whoauth = NULL, *whon = NULL;

	if(!nick->user)
	{
		anUser *u;
		if(parc < 2) return syntax_cmd(nick, FindCoreCommand("recover"));
		/* cache if anyone is already logued into this account */
		if((u = getuserinfo(parv[1])) && u->n != nick) whoauth = u->n;
		/* then perform standard login */
		if(!ns_login(nick, chan, parc, parv)) return 0;
		/* now check if anyone else is using my account as nick */
		if((who = getnickbynick(nick->user->nick)) != nick) whon = who;
	}
	else
	{
		if(parc && strcasecmp(nick->user->nick, parv[1])) /* other nick than his login */
			return csreply(nick, GetReply(nick, L_LOGUEDIN), nick->user->nick);

		if(!strcasecmp(nick->nick, nick->user->nick)) /* already his nick */
			return csreply(nick, GetReply(nick, L_ALREADYYOURNICK), nick->nick);

		if(!(whon = getnickbynick(nick->user->nick))) /* noone use it, use /NICK boring guy! */
			return csreply(nick, GetReply(nick, L_NICKAVAIABLE), nick->user->nick);
	}

	if(whoauth && (whoauth != whon || !UPKill(nick->user)))
	{ /* personne authée sur le même username, deauthons le. (sauf si on va le killer LUI) */
		csreply(whoauth, GetReply(nick, L_DEAUTHBYRECOVER), nick->nick);
		/* attention au cas whoauth!=NULL && whon!=NULL && whon!=whoauth */
		cs_account(whoauth, NULL);
	}

	if(whon) /* personne possédant le nick == username */
	{
		if(!UPKill(nick->user))
		{
			char buf[NICKLEN + 1];/* generates a new nick based on current one and appends */
			do { /* a number of at max 4 digits */
				snprintf(buf, sizeof buf, "%.20s%d", whon->nick, rand() & 4095);
			} while(getnickbynick(buf));
			putserv("%s " TOKEN_SVSNICK " %s :%s", bot.servnum, whon->numeric, buf);
		}
		else
		{
			if(whoauth == whon)
			{
				if(IsAdmin(nick->user)) adm_active_del(whon);
				whon->user = NULL;
			}
			putserv("%s " TOKEN_KILL " %s :%s (Pseudo enregistré)",
				cs.num, whon->numeric, cs.nick);
			del_nickinfo(whon->numeric, "recover");
		}
		csreply(nick, GetReply(nick, L_NICKAVAIABLE), nick->user->nick);
	}
	return 1;
}
#endif

int myaccess(aNick *nick, aChan *chan, int parc, char **parv)
{
	int c = 0, i = 0;
	anAccess *a;

	if(parc)
	{
		/* manage pending access */
		if(!strcasecmp(parv[1], "accept") || !strcasecmp(parv[1], "refuse"))
		{
			int add;

			if(parc < 2) return csreply(nick, "Syntaxe: %s %s <#salon>", parv[0], parv[1]);

			if(!(chan = getchaninfo(parv[2])))
				return csreply(nick, GetReply(nick, L_NOSUCHCHAN), parv[2]);

			for(a = nick->user->accesshead; a && a->c != chan; a = a->next);

			if(!a || !AWait(a))
				return csreply(nick, GetReply(nick, L_YOUNOPROPACCESS), parv[2]);

			add = tolower((unsigned char) *parv[1]) == 'a';

			csreply(nick, GetReply(nick, add ? L_NOWACCESS : L_PROPREFUSED), a->c->chan);
			/* act */
			if(add) a->flag &= ~A_WAITACCESS;
			else del_access(nick->user, a->c);
			return 1;
		}
		if(!strcasecmp(parv[1], "drop")) /* suppress a valid access */
		{
			if(parc < 2) return csreply(nick, "Syntaxe: %s DROP <#salon>", parv[0]);

			if(!(a = GetAccessIbyUserI(nick->user, getchaninfo(parv[2]))))
				return csreply(nick, GetReply(nick, L_YOUNOACCESSON), parv[2]);

			if(AOwner(a)) return csreply(nick, GetReply(nick, L_OWNERMUSTUNREG), parv[2]);

			del_access(nick->user, a->c);

			csreply(nick, GetReply(nick, L_OKDELETED), parv[2]);
			return 1;
		}
	}

	if(!getoption("-access", parv, parc, 1, GOPT_FLAG))
		return show_userinfo(nick, nick->user, 0);

	c = getoption("-chan", parv, parc, 1, GOPT_STR);

	for(a = nick->user->accesshead; a; ++i, a = a->next)
	{
		if(AWait(a) || (c && match(parv[c], a->c->chan))) continue;
		csreply(nick, "- %s -", a->c->chan);
		show_accessn(a, nick->user, nick);
	}

	return csreply(nick, GetReply(nick, L_TOTALFOUND), i, PLUR(i), PLUR(i));
}

int deauth(aNick *nick, aChan *chan, int parc, char **parv)
{
   csreply(nick, GetReply(nick, L_JUSTLOGOUT), nick->user->nick);
   cs_account(nick, NULL);
   return 1;
}

int user_set(aNick *nick, aChan *chan, int parc, char **parv)
{
	const char *arg = parv[1];

	if(!strcasecmp(arg, "mail"))
	{
		if(parc < 2)
			return csreply(nick, "Syntaxe: %s MAIL <nouveau mail>", parv[0]);

		if(strlen(parv[2]) > MAILLEN)
			return csreply(nick, GetReply(nick, L_MAILLENLIMIT), MAILLEN);

		if(!IsValidMail(parv[2]))
			return csreply(nick, GetReply(nick, L_MAIL_INVALID));

		if(!strcasecmp(nick->user->mail, parv[2]))
			return csreply(nick,  GetReply(nick, L_IDENTICMAIL), parv[2]);

		if(GetUserIbyMail(parv[2])) return csreply(nick, GetReply(nick, L_MAIL_INUSE));

		switch_mail(nick->user, parv[2]);
		csreply(nick, GetReply(nick, L_OKCHANGED));
	}
	else if(!strcasecmp(arg, "autohide"))
	{
		if(!GetConf(CF_XMODE)) return csreply(nick, GetReply(nick, L_CMDDISABLE));
		switch_option(nick, parc < 2 ? NULL : parv[2], arg, nick->user->nick,
			&nick->user->flag, U_WANTX);
	}
#ifdef USE_NICKSERV
	else if(!strcasecmp(arg, "protect"))
	{
		int protect;
		if(parc < 2 || !Strtoint(parv[2], &protect, 0, 2))
			return csreply(nick, "Syntaxe: %s PROTECT <0/1/2>", parv[0]);

		nick->user->flag &= ~(U_PKILL | U_PNICK);
		if(protect == 1)
		{
			SetUPNick(nick->user);
			csreply(nick, GetReply(nick, L_SPNICK), cf_kill_interval);
		}
		else if(protect == 2)
		{
			SetUPKill(nick->user);
			csreply(nick, GetReply(nick, L_SPKILL), cf_kill_interval);
		}
		else csreply(nick, GetReply(nick, L_SPNONE));
	}
#endif
#ifdef USE_MEMOSERV
	else if(!strcasecmp(arg, "nomemo"))
		switch_option(nick, parc < 2 ? NULL : parv[2], arg, nick->user->nick,
			&nick->user->flag, U_NOMEMO);
#endif
	else if(!strcasecmp(arg, "pass"))
	{
		if(parc < 3)
			return csreply(nick, GetReply(nick, L_NEED_PASS_TWICE));

		if(strcmp(parv[2], parv[3]))
			return csreply(nick, GetReply(nick, L_PASS_CHANGE_DIFF));

		if(strlen(parv[2]) < 7)
			return csreply(nick, GetReply(nick, L_PASS_LIMIT));

		password_update(nick->user, parv[2], 0);

		csreply(nick, GetReply(nick, L_OKCHANGED));
	}
	else if(!strcasecmp(arg, "username"))
	{
		aDNR *dnr;

		if(parc < 2)
			return csreply(nick, "Syntaxe: %s USERNAME <nouvel username>", parv[0]);

		if(UChanged(nick->user))
			return csreply(nick, GetReply(nick, L_CMDALREADYUSED), "UserName");

		if(strlen(parv[2]) > NICKLEN)
			return csreply(nick, GetReply(nick, L_USERLENLIMIT), NICKLEN);

		if(!IsValidNick(parv[2]))
			return csreply(nick, GetReply(nick, L_USER_INVALID), parv[2]);

		if((dnr = IsBadNick(parv[2])))
			return csreply(nick, GetReply(nick, L_CANREGDNR), parv[2], dnr->raison);

		if(getuserinfo(parv[2]))
			return csreply(nick, GetReply(nick, L_ALREADYREG), parv[2]);

		if(strcasecmp(nick->nick, parv[2]) && getnickbynick(parv[2]))
			return csreply(nick, GetReply(nick, L_UCANTREG_INUSE));

		switch_user(nick->user, parv[2]);
		SetUChanged(nick->user);
		csreply(nick, GetReply(nick, L_OKCHANGED));
	}
	else if(!strcasecmp(arg, "access"))
	{
		int i = 0;
		anAccess *a, *at;

		if(parc < 2 || !Strtoint(parv[2], &i, 0, 2))
			return csreply(nick, "Syntaxe: %s ACCESS <0,1,2>", parv[0]);

		nick->user->flag &= ~(U_POKACCESS | U_PACCEPT | U_PREJECT);
		switch(i)
		{
			case 0:
			nick->user->flag |= U_PREJECT;
			csreply(nick, GetReply(nick, L_PREFUSE));
			for(a = nick->user->accesshead; a; a = at)
			{
				at = a->next;
				if(AWait(a)) del_access(nick->user, a->c);
			}
			break;

			case 1:
			nick->user->flag |= U_POKACCESS;
			csreply(nick, GetReply(nick, L_PASK), cs.nick, RealCmd("myaccess"));
			break;

			case 2:
			nick->user->flag |= U_PACCEPT;
			csreply(nick, GetReply(nick, L_PACCEPT));
			for(a = nick->user->accesshead; a; a = a->next)
				if(AWait(a)) a->flag &= ~A_WAITACCESS;
			break;
		}

	}
	else if(!strcasecmp(arg, "lang"))
	{
		Lang *lang;

		if(parc < 2)
		{
			csreply(nick, GetReply(nick, L_LANGLIST));
			for(lang = DefaultLang; lang; lang = lang->next) csreply(nick, "%s", lang->langue);
			return 0;
		}

		if(!(lang = lang_isloaded(parv[2])))
			return csreply(nick, GetReply(nick, L_NOSUCHLANG), parv[2]);

		nick->user->lang = lang;
		csreply(nick, GetReply(nick, L_NEWLANG));
	}
	else if(!strcasecmp(arg, "novote"))
			switch_option(nick, parc < 2 ? NULL : parv[2], arg,
					nick->user->nick, &nick->user->flag, U_NOVOTE);

	else if(!strcasecmp(arg, "replymsg"))
			switch_option(nick, parc < 2 ? NULL : parv[2], arg,
					nick->user->nick, &nick->user->flag, U_PMREPLY);

	else if(!strcasecmp(arg, "busy") && IsAdmin(nick->user))
			switch_option(nick, parc < 2 ? NULL : parv[2], arg,
					nick->user->nick, &nick->user->flag, U_ADMBUSY);

	else return csreply(nick, GetReply(nick, L_UNKNOWNOPTION), arg);

	return 1;
}
