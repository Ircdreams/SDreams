/* src/web2cs-commands.c - Commandes du Web2CS
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
 * $Id: web2cs-commands.c,v 1.58 2008/02/04 23:56:47 romexzf Exp $
 */

/* NEW CS LIVE */

#include "main.h"
#ifdef WEB2CS
#include <ctype.h>
#include "web2cs-engine.h"
#include "hash.h"
#include "outils.h"
#include "cs_cmds.h"
#include "del_info.h"
#include "add_info.h"
#include "crypt.h"
#include "dnr.h"
#include "memoserv.h"
#include "data.h"
#include "web2cs-commands.h"

static void w2c_parse_qstring(char *buf, size_t size, char **parv,
								int parc, int i, int *iptr)
{
	int inquote = 0;

	for(--size; size && i < parc; ++i)
	{
		const char *p = parv[i];

		for(; size && *p; ++p)
		{
			if(*p == '"')
			{
				if((inquote = inquote ? 0 : 1)) continue;
				else break;
			}
			else if(*p == '\\') ++p;
			*buf++ = *p;
			--size;
		}
		if(!inquote) break;
		if(size--) *buf++ = ' ';
	}
	*buf = 0;
	*iptr = i;
}

/* **** CMDS **** */

int w2c_passwd(WClient *cl, int parc, char **parv)
{
	if(IsAuth(cl)) return 0; /* On devrait pas kill le sock? */

	if(strcmp(parv[1], bot.w2c_pass)) return w2c_exit_client(cl, "ERROR SYS_BADPWD");

	cl->flag |= W2C_AUTH;
	return 0;
}

static int w2c_do_auth(WClient *cl, anUser **u_p, const char *user,
						const char *pass, const char *ip)
{
	anUser *u;

	if(cl->user) return w2c_exit_client(cl, "ERROR SYS_ALREADY_LOGUEDIN");

	if(!(u = getuserinfo(user)))
		return w2c_exit_client(cl, "ERROR USER_UNKNOWN");

	if(USuspend(u)) return w2c_exit_client(cl, "ERROR USER_SUSPENDED");

	if(strcmp(u->passwd, pass)) /* bad password */
	{	/* check if it's not the alternative one (sendpass) */
		if(!UOubli(u) || strcmp(MD5pass(create_password(u->passwd), NULL), pass))
		{
			if(IsAdmin(u))
			{
				cswall("(Web) Login Admin échoué sur \2%s\2 par \2%s\2", u->nick, ip);
				log_write(LOG_UCMD, 0, "login %s failed (BadPass) par Web@%s", u->nick, ip);
			}
			return w2c_exit_client(cl, "ERROR USER_BADPWD");
		}
		else if(UOubli(u)) /* it succeeds against the generated one */
				password_update(u, pass, PWD_HASHED); /* save it as new password */
	}

	u->lastseen = CurrentTS;
	if(UOubli(u)) DelUOubli(u);
	if(UFirst(u)) DelUFirst(u);

	if(!u->lastlogin || strcmp(ip, u->lastlogin + 4))
	{
		char host[4 + DOTTEDIPLEN + 1] = "web@"; /* IPv4 + 'web@' + \0 */
		Strncpy(host + 4, ip, sizeof host - 5); /* wanna overflow me? */

		str_dup(&u->lastlogin, host);
	}

	*u_p = u;
	return 1;
}

/* Web2CS login */

int w2c_login(WClient *cl, int parc, char **parv)
{
	w2c_do_auth(cl, &cl->user, parv[1], parv[2], parv[3]);
	return 0;
}

/* Web Auth : Performs an auth for a given username & password
 * It return the userID and an auth Cookie if asked */

int w2c_wauth(WClient *cl, int parc, char **parv)
{
	anUser *u = NULL;
	if(!w2c_do_auth(cl, &u, parv[1], parv[2], parv[3])) return 0;

	if(parc > 4 || u->cookie)
		w2c_sendrpl(cl, "=user=%s mail=%s userid=%U cookie=%s options=%d", u->nick,
			u->mail, u->userid, parc > 4 ? create_cookie(u) : u->cookie, u->flag);
	else w2c_sendrpl(cl, "=user=%s mail=%s userid=%U options=%d",
					u->nick, u->mail, u->userid, u->flag);

	return w2c_exit_client(cl, "OK");
}

int w2c_wcookie(WClient *cl, int parc, char **parv)
{
	anUser *user = GetUserIbyID(strtoul(parv[1], NULL, 10));

	if(!user) return w2c_exit_client(cl, "ERROR USER_UNKNOWN");

	if(USuspend(user)) return w2c_exit_client(cl, "ERROR USER_SUSPENDED");

	if(!user->cookie) return w2c_exit_client(cl, "ERROR USER_HASNOCOOKIE");

	if(strcmp(user->cookie, parv[2])) return w2c_exit_client(cl, "ERROR USER_BADCOOKIE");

	w2c_sendrpl(cl, "=user=%s mail=%s userid=%U cookie=%s", user->nick, user->mail,
		user->userid, parc > 4 ? create_cookie(user) : user->cookie);

	return w2c_exit_client(cl, "OK");
}

int w2c_oubli(WClient *cl, int parc, char **parv)
{
	anUser *user;

	if(cl->user) return w2c_exit_client(cl, "ERROR SYS_ALREADY_LOGUEDIN");

	if(!(user = getuserinfo(parv[1]))) return w2c_exit_client(cl, "ERROR USER_UNKNOWN");

	if(USuspend(user)) return w2c_exit_client(cl, "ERROR USER_SUSPENDED");

	if(UOubli(user)) return w2c_exit_client(cl, "ERROR USER_PWD_ALREDYSENT");

	if(strcasecmp(user->mail, parv[2]))
		return w2c_exit_client(cl, "ERROR USER_BADMAIL");

	SetUOubli(user);
	w2c_sendrpl(cl, "newpass %s", create_password(user->passwd));
	return w2c_exit_client(cl, "OK");
}

#define SHOW_TOPIC 		0x01
#define SHOW_RIGHT 		0x02
#define SHOW_USERS 		0x04
#define SHOW_INFOS 		0x08
#define SHOW_OWNER 		0x10
#define SHOW_BANS 		0x20
#define SHOW_ACCESS 	0x40
#define SHOW_ALL 		(SHOW_TOPIC|SHOW_USERS|SHOW_INFOS|SHOW_OWNER|SHOW_BANS|SHOW_ACCESS)

static inline void w2c_access_display(WClient *cl, anAccess *a)
{
	w2c_sendrpl(cl, "access %s %s %d %T %d :%s", a->c->chan, a->user->nick,
		a->level, a->lastseen, a->flag, NONE(a->info));
}

static int w2c_showaccess(WClient *cl, aChan *chan, int flag)
{
	aLink *lp = chan->access;

	for(; lp; lp = lp->next) w2c_access_display(cl, lp->value.a);

	return 0;
}

int w2c_channel(WClient *cl, int parc, char **parv)
{
	aChan *chan = getchaninfo(parv[1]);
	int i;

	if(!strcasecmp(parv[2], "info"))
	{
		int infos = 0;
		aNChan *netchan;

		if(!chan) return w2c_exit_client(cl, "ERROR CHAN_UNKNOWN");
		else if(parc < 4) return w2c_exit_client(cl, "OK");

		if(IsAdmin(cl->user) || GetAccessIbyUserI(cl->user, chan)) infos |= SHOW_RIGHT;

		if((netchan = chan->netchan)
			&& HasMode(netchan, C_MSECRET|C_MPRIVATE) && !(infos & SHOW_RIGHT))
				return w2c_exit_client(cl, "ERROR CHAN_NOACCESS");

		while(*parv[3])
			switch(*parv[3]++)
			{
				case '*': infos |= SHOW_ALL; break;
				case 'i': infos |= SHOW_INFOS; break;
				case 't': infos |= SHOW_TOPIC; break;
				case 'o': infos |= SHOW_OWNER; break;
				case 'u': if(infos & SHOW_RIGHT) infos |= SHOW_USERS; break;
				case 'a': infos |= SHOW_ACCESS; break;
				case 'b': infos |= SHOW_BANS; break;
				default: break;
			}

		/* aucune infos demandées */
		if(!(infos & ~SHOW_RIGHT)) return w2c_exit_client(cl, "OK");

		/* OK, construisons/envoyons les infos */
		if(infos & SHOW_INFOS)
		{
			w2c_sendrpl(cl, "=owner=%s ct=%T banlevel=%d cml=%d bantype=%d "
				"bantime=%T options=%d liminc=%u limgrace=%u defmodes=%u "
				"deflimit=%u defkey=%s",
				chan->owner ? chan->owner->user->nick : "***", chan->creation_time,
				chan->banlevel,	chan->cml, chan->bantype, chan->bantime, chan->flag,
				chan->limit_inc, chan->limit_min, chan->defmodes.modes, chan->defmodes.limit,
				!HasDMode(chan, C_MKEY) || infos & SHOW_RIGHT ? chan->defmodes.key : "***");

			if(netchan) w2c_sendrpl(cl, "=modes=%u limit=%u key=%s",
				netchan->modes.modes, netchan->modes.limit,
				!HasMode(netchan, C_MKEY) || infos & SHOW_RIGHT ? netchan->modes.key : "***");
			else w2c_sendrpl(cl, "=modes=0 limit=0 key=");

			w2c_sendrpl(cl, "description %s", chan->description);
			if(*chan->deftopic) w2c_sendrpl(cl, "deftopic %s", chan->deftopic);
			if(*chan->welcome) w2c_sendrpl(cl, "welcome %s", chan->welcome);
			if(infos & SHOW_RIGHT && chan->motd) w2c_sendrpl(cl, "motd %s", chan->motd);
		}
		else if(infos & SHOW_OWNER) /* send owner only if we did no already sent it */
				w2c_sendrpl(cl, "owner %s", chan->owner ? chan->owner->user->nick : "<Aucun>");

		if(infos & SHOW_TOPIC && netchan && *netchan->topic)
			w2c_sendrpl(cl, "topic %s", netchan->topic);

		if(infos & SHOW_ACCESS) w2c_showaccess(cl, chan, 0);
		if(infos & SHOW_BANS)
		{
			aBan *ban = chan->banhead;

			for(; ban; ban = ban->next)
				w2c_sendrpl(cl, "ban %s %s %T %T %d :%s", ban->mask,
					ban->de, ban->debut, ban->fin, ban->level, ban->raison);
		}
	}
	else if(!strcasecmp(parv[2], "set"))
	{
		int level = OWNERLEVEL, itype = 0, changes = 0;

		if(!chan) return w2c_exit_client(cl, "ERROR CHAN_UNKNOWN");
		if(!IsAdmin(cl->user) && (level = ChanLevelbyUserI(cl->user, chan)) < A_MANAGERLEVEL)
			return w2c_exit_client(cl, "ERROR CHAN_NOACCESS");

		for(i = 4; i < parc; i += 2)
		{
			const char *opt = parv[i - 1], *param = parv[i];

			if(!strcasecmp(opt, "deftopic"))
				w2c_parse_qstring(chan->deftopic, sizeof chan->deftopic, parv,
					parc, i, &i);
			else if(!strcasecmp(opt, "theme"))
				w2c_parse_qstring(chan->description, sizeof chan->description,
					parv, parc, i, &i);
			else if(!strcasecmp(opt, "welcome"))
				w2c_parse_qstring(chan->welcome, sizeof chan->welcome, parv,
					parc, i, &i);
			else if(!strcasecmp(opt, "chanurl"))
				w2c_parse_qstring(chan->url, sizeof chan->url, parv, parc, i, &i);
			else if(!strcasecmp(opt, "motd"))
			{
				char buf[TOPICLEN + 1];
				w2c_parse_qstring(buf, sizeof buf, parv, parc, i, &i);
				if(*buf) str_dup(&chan->motd, buf);
				else free(chan->motd), chan->motd = NULL;
			}
			else if(!strcasecmp(opt, "defkey"))
				Strncpy(chan->defmodes.key, param, KEYLEN);
			else if(!strcasecmp(opt, "deflimit") && (itype = strtol(param, NULL, 10)) >= 0)
				chan->defmodes.limit = itype;
			else if(!strcasecmp(opt, "defmodes"))
				chan->defmodes.modes = strtoul(param, NULL, 10);
			else if(!strcasecmp(opt, "banlevel") && level >= chan->banlevel
					&& !Strtoint(param, &itype, 0, level))
						chan->banlevel = itype;
			else if(!strcasecmp(opt, "cml") && level >= chan->cml
					&& !Strtoint(param, &itype, 0, level))
						chan->cml = itype;
			else if(!strcasecmp(opt, "bantime") && (itype = strtol(param, NULL, 10)) >= 0)
				chan->bantime = itype;
			else if(!strcasecmp(opt, "flags") && (itype = strtol(param, NULL, 10)) >= 0)
				chan->flag = itype;
			else if(!strcasecmp(opt, "bantype") && !Strtoint(param, &itype, 1, 5))
				chan->bantype = itype;
			else continue;
			++changes;
		}
		if(!changes) return w2c_exit_client(cl, "ERROR SYS_SYNTAX");
	}
	else return w2c_exit_client(cl, "ERROR SYS_SYNTAX");
	return w2c_exit_client(cl, "OK");
}

static int w2c_memolist(WClient *cl, anUser *user, int id)
{
	aMemo *m = user->memohead;
	int i = 0;

	for(; m; m = m->next)
	{
		if(++i == id) return w2c_sendrpl(cl, "memo %s %T %d :%s", m->de, m->date,
								m->flag, m->message);
		else if(!id) w2c_sendrpl(cl, "memo %s %T %d :%s", m->de, m->date, m->flag,
						m->message), m->flag |= MEMO_READ;
	}
	return 0;
}

int w2c_user(WClient *cl, int parc, char **parv)
{
	anUser *user;
	int adm = IsAdmin(cl->user) ? cl->user->level : 0;

	/* handle most of case whithout looping: Not Admin or query on himself */
	if(!adm || !strcasecmp(parv[1], cl->user->nick)) user = cl->user;
	else if(!(user = getuserinfo(parv[1]))) /* check admin query */
			return w2c_exit_client(cl, "ERROR USER_UNKNOWN");

	if(!strcasecmp(parv[2], "info"))
	{
		anAccess *a;

		w2c_sendrpl(cl, "=user=%s userid=%U mail=%s lastseen=%T level=%d regtime=%T "
			"options=%d lang=%s lastlogin=%s", user->nick, user->userid, user->mail,
			user->lastseen,	user->level, user->reg_time, user->flag,
			user->lang->langue, user->lastlogin ? user->lastlogin : "<Indisponible>");

		if(user == cl->user) w2c_memolist(cl, user, 0);

		for(a = user->accesshead; a; a = a->next) w2c_access_display(cl, a);
	}
	else if(!strcasecmp(parv[2], "set"))
	{
		/*
		 * Set: can handle multi-options change on single line
		 */
		int i = 4, changes = 0;

		if(adm && user->level >= adm && user != cl->user)
			return w2c_exit_client(cl, "ERROR USER_NORIGHT");

		for(; i < parc; i += 2)
		{
			const char *opt = parv[i-1], *param = parv[i];

			if(!strcasecmp(opt, "mail"))
			{
				if(strlen(param) > MAILLEN || !IsValidMail(param)
					|| !strcasecmp(user->mail, param))
					return w2c_exit_client(cl, "ERROR USER_INVALIDMAIL");

				if(GetUserIbyMail(param))
					return w2c_exit_client(cl, "ERROR USER_MAILINUSE");

				switch_mail(user, param);
			} /* mail */

			else if(!strcasecmp(opt, "pass"))
			{
				if(!strcmp(param, user->passwd)) continue;

				password_update(user, param, PWD_HASHED);
			} /* password */

			else if(!strcasecmp(opt, "username"))
			{
				if(UChanged(user) && !adm)
					return w2c_exit_client(cl, "ERROR USER_ALREADYCHANGED");

				if(strlen(param) > NICKLEN || !IsValidNick(param))
					return w2c_exit_client(cl, "ERROR USER_INVALID");

				if(IsBadNick(param))
					return w2c_exit_client(cl, "ERROR USER_FORBIDDEN");

				if(getuserinfo(param))
					return w2c_exit_client(cl, "ERROR USER_INUSE");

				if(getnickbynick(param))
					return w2c_exit_client(cl, "ERROR USER_NICKINUSE");

				switch_user(user, param);
				if(!adm) SetUChanged(user);
			} /* username */

			else if(!strcasecmp(opt, "lang"))
			{
				Lang *lang = lang_isloaded(param);

				if(!lang) return w2c_exit_client(cl, "ERROR USER_UNKNOWNLANG");

				user->lang = lang;
			} /* lang */

			else if(!strcasecmp(opt, "flag"))
			{
				int flag = (strtol(param, NULL, 10) & U_ALL); /* clear stranger flags */

				if(!flag || flag == user->flag) continue; /* not a number, or no change */

				/* check if user tries to changed *its* blocked flag (suspend..) */
				if(!adm && (flag & U_BLOCKED) != (user->flag & U_BLOCKED))
					return w2c_exit_client(cl, "ERROR USER_NORIGHT");

				if((U_PROTECT_T & flag) == U_PROTECT_T || !(U_ACCESS_T & flag))
					return w2c_exit_client(cl, "ERROR USER_INVALIDFLAG");

				if((U_ACCESS_T & user->flag) != (U_ACCESS_T & flag))
				{
					anAccess *a, *at;
					if(flag & U_PREJECT) /* refuse all pending propositions */
						for(a = user->accesshead; a; a = at)
						{
							at = a->next;
							if(AWait(a)) del_access(user, a->c);
						}
					else if(flag & U_PACCEPT) /* accept all propositions */
						for(a = user->accesshead; a; a = a->next)
							if(AWait(a)) a->flag &= ~A_WAITACCESS;
				}
				user->flag = flag;
			} /* options */
			++changes;
		} /* for */

		/* no change performed */
		if(!changes) return w2c_exit_client(cl, "ERROR SYS_SYNTAX");

		/* at least one (doesn't check for bad syntax in extra changes) */
		w2c_sendrpl(cl, "=user=%s mail=%s level=%d options=%d lang=%s",
			user->nick, user->mail, user->level, user->flag, user->lang->langue);
	} /* set */

	else if(!strcasecmp(parv[2], "aaccept") || !strcasecmp(parv[2], "adrop"))
	{
		anAccess *a;
		aChan *c;
		int changes = 0, j = 2, del = 0;

		for(; j < parc; ++j)
		{
			if(!strcasecmp(parv[j], "adrop")) del = 1;
			else if(!strcasecmp(parv[j], "aaccept")) del = 0;
			else
			{
				for(a = user->accesshead, c = getchaninfo(parv[j]); c && a; a = a->next)
					if(a->c == c)
					{
						if(del) del_access(user, a->c), ++changes;
						else if(AWait(a)) a->flag &= ~A_WAITACCESS, ++changes;
						break;
					}
			}
		}

		if(!changes) return w2c_exit_client(cl, "ERROR SYS_SYNTAX");
		w2c_sendrpl(cl, "performed %d", changes);
	} /* aaccess */

	else if(!strcasecmp(parv[2], "suspend"))
	{
		char *ptr = NULL;
		time_t timeout = 0;
		int i = 3;

		if(parc < 4) return w2c_exit_client(cl, "ERROR SYS_SYNTAX");

		if(adm && user->level >= adm && user != cl->user)
			return w2c_exit_client(cl, "ERROR USER_NORIGHT");

		/* if he isn't suspend, but used to be, we need to free it to recreate it */
		if(user->suspend && !USuspend(user))
			data_free(user->suspend), user->suspend = NULL;

		if(*parv[3] == '%') timeout = convert_duration(++parv[3]), ++i;
		ptr = parv2msg(parc-1, parv, i, RAISONLEN);

		switch(data_handle(user->suspend, cl->user->nick, ptr, timeout,
				DATA_T_SUSPEND_USER, user))
		{
			case -1: /* error */ return w2c_exit_client(cl, "ERROR SYS_SYNTAX");
			case 1: /* created */
				if(user->n)
				{
					csreply(user->n, "Votre Username vient d'être suspendu par l'Admin %s.",
						cl->user->nick);
					cs_account(user->n, NULL);
				}
			case 0: /* deleted */
			case 2: /* updated */
				w2c_sendrpl(cl, "options %d", user->flag); /* report.. */
		}
	}

	else if(!strcasecmp(parv[2], "nopurge"))
	{
		char *ptr = NULL;
		time_t timeout = 0;
		int i = 3;

		if(parc < 4) return w2c_exit_client(cl, "ERROR SYS_SYNTAX");

		if(!adm || (user->level >= adm && user != cl->user))
			return w2c_exit_client(cl, "ERROR USER_NORIGHT");

		if(*parv[3] == '%') timeout = convert_duration(++parv[3]), ++i;
		ptr = parv2msg(parc-1, parv, i, RAISONLEN);

		data_handle(user->nopurge, cl->user->nick, ptr, timeout, DATA_T_NOPURGE, user);
		w2c_sendrpl(cl, "options %d", user->flag); /* report.. */
	}

	return w2c_exit_client(cl, "OK");
}

int w2c_register(WClient *cl, int parc, char **parv)
{
	anUser *user;
	aNick *nick;

	if(cl->user) return w2c_exit_client(cl, "ERROR SYS_ALREADY_LOGUEDIN");

	if(strlen(parv[1]) > NICKLEN || !IsValidNick(parv[1]))
		return w2c_exit_client(cl, "ERROR USER_INVALID");

	if(strlen(parv[2]) > MAILLEN || !IsValidMail(parv[2]) || strlen(parv[3]) < 6)
		return w2c_exit_client(cl, "ERROR USER_INVALIDMAIL");

	if(IsBadNick(parv[1])) return w2c_exit_client(cl, "ERROR USER_FORBIDDEN");
	if(getuserinfo(parv[1])) return w2c_exit_client(cl, "ERROR USER_INUSE");

	if((nick = getnickbynick(parv[1])) && strcmp(GetUserIP(nick, NULL), parv[4]))
		return w2c_exit_client(cl, "ERROR USER_NICKINUSE");

	if(GetUserIbyMail(parv[2])) return w2c_exit_client(cl, "ERROR USER_MAILINUSE");

	user = add_regnick(parv[1], MD5pass(parv[3], NULL), CurrentTS, CurrentTS, 1,
				U_DEFAULT|U_FIRST, parv[2], 0);

	user->lang = DefaultLang;
	log_write(LOG_UCMD, 0, "register %s par web@%s", user->nick, parv[4]);

	w2c_sendrpl(cl, "userid %U", user->userid);
	return w2c_exit_client(cl, "OK");
}

int w2c_regchan(WClient *cl, int parc, char **parv)
{
	aChan *chan;
	const char *c = parv[1];
	int i = 1;

	for(; c[i] && i < REGCHANLEN && c[i] != ',' && isascii(c[i]); ++i);

	if(c[i] || *c != '#') return w2c_exit_client(cl, "ERROR SYS_SYNTAX");

	if(getchaninfo(c)) return w2c_exit_client(cl, "ERROR CHAN_INUSE");
	if(UCantRegChan(cl->user)) return w2c_exit_client(cl, "ERROR USER_CANNOTREGCHAN");

	if(!IsAdmin(cl->user))
	{
		if(IsBadChan(c)) return w2c_exit_client(cl, "ERROR CHAN_FORBIDDEN");
		if(IsAnOwner(cl->user))	return w2c_exit_client(cl, "ERROR USER_ALREADYOWNER");
		if(GetNChan(c)) return w2c_exit_client(cl, "ERROR CHAN_NOTEMPTY");
	}
	/* BEWARE! parv array is indexed till parc-1 in web2CS  */
	chan = add_chan(c, parv2msg(parc-1, parv, 2, DESCRIPTIONLEN));
	add_access(cl->user, c, OWNERLEVEL, A_MANAGERFLAGS, OnChanTS(cl->user, chan));
	csjoin(chan, 0); /* join if channel exists */

	log_write(LOG_CCMD, 0, "regchan %s par %s@Web", c, cl->user->nick);

	return 1;
}

int w2c_memo(WClient *cl, int parc, char **parv)
{
	const char *cmd = parv[1];

	if(!strcasecmp(cmd, "READ")) w2c_memolist(cl, cl->user, 0);
	else if(!strcasecmp(cmd, "DEL"))
	{
		int i = 0, count = 0;

		if(parc < 3 || (strcasecmp(parv[2], "all")
			&& !(i = item_parselist(parv[2], NULL, 0, &count))))
				return w2c_exit_client(cl, "ERROR SYS_SYNTAX");

		memo_del(cl->user, NULL, i, count, NULL /* not verbose */);
		w2c_memolist(cl, cl->user, 0);
	}

	return w2c_exit_client(cl, "OK");
}

int w2c_isreg(WClient *cl, int parc, char **parv)
{
	return w2c_exit_client(cl, getuserinfo(parv[1]) ? "OK" : "ERROR USER_UNKNOWN");
}

#endif
