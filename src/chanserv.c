/* src/chanserv.c - Diverses commandes sur le module chanserv
 *
 * Copyright (C) 2002-2007 David Cortier  <Cesar@ircube.org>
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
 * $Id: chanserv.c,v 1.98 2007/11/17 16:57:16 romexzf Exp $
 */

#include "main.h"
#include "hash.h"
#include "cs_cmds.h"
#include "outils.h"
#include "chanserv.h"
#include "config.h"
#include "ban.h"
#include "add_info.h"
#include "del_info.h"

inline void show_accessn(anAccess *acces, anUser *user, aNick *num)
{
	csreply(num, GetReply(num, L_ACUSER), user->nick, acces->level);
	csreply(num, "\2Options:\2%s", GetAccessOptions(acces));
	if(AOnChan(acces)) csreply(num, GetReply(num, L_ACONCHAN));
	else csreply(num, GetReply(num, L_ACLASTSEEN), get_time(num, acces->lastseen));
	if(acces->info) csreply(num, "\2Infoline:\2 %s", acces->info);
}

/*
 * show_access <chan> <recherche>
 */
int show_access(aNick *nick, aChan *chan, int parc, char **parv)
{
	register aLink *lp = chan->access;
	char *arg = parv[2], comp = '\0';
	int i = 0, flags = 0, all, level = 0, wild = 0;

	if(chan->netchan && HasMode(chan->netchan, C_MSECRET|C_MPRIVATE) /* salon +s|p */
		&& (!nick->user || !GetAccessIbyUserI(nick->user, chan)) /* no access */
		&& !IsAnAdmin(nick->user) && !GetJoinIbyNC(nick, chan->netchan)) /* not admin, not member */
			return csreply(nick, GetReply(nick, L_YOUNOACCESSON), chan->chan);

	if(*arg == '>' || *arg == '<' || *arg == '=')
	{
		comp = *arg++;
		if(!*arg || !Strtoint(arg, &level, 1, OWNERLEVEL))
			return csreply(nick, "Veuillez préciser un niveau valide après '%c'", comp);
	}
	else wild = HasWildCard(arg);

	if(getoption("-autoop", parv, parc, 3, GOPT_FLAG)) flags |= A_OP;
	if(getoption("-autovoice", parv, parc, 3, GOPT_FLAG)) flags |= A_VOICE;
	if(getoption("-protect", parv, parc, 3, GOPT_FLAG)) flags |= A_PROTECT;
	if(getoption("-suspend", parv, parc, 3, GOPT_FLAG)) flags |= A_SUSPEND;
	if(getoption("-wait", parv, parc, 3, GOPT_FLAG)) flags |= A_WAITACCESS;

	/* allow full listing as default if Admin */
	all = nick->user && (IsAdmin(nick->user) /* or if '-all' is specified and level is 450+ */
			|| (getoption("-all", parv, parc, 3, GOPT_FLAG) &&
				(chan->owner->user == nick->user
					|| ChanLevelbyUserI(nick->user, chan) >= A_MANAGERLEVEL))) ? 1 : 0;

	csreply(nick, GetReply(nick, L_ACCESSLIST), chan->chan);

	for(; lp; lp = lp->next)
	{
		register anAccess *a = lp->value.a;

		if(((comp != '\0' && ((comp == '>' && a->level > level) /* match level's criteria */
			          || (comp == '<' && a->level < level)
			          || (comp == '=' && level == a->level)))
			  || (*arg == '*' && !arg[1]) /* seeking for '*' */
			  || (wild && !match(arg, a->user->nick)) /* or more complex mask */
			  || !strcasecmp(arg, a->user->nick)) /* or exact username */
			&& (!flags || (a->flag & flags) == flags)
			&& (!AWait(a) || flags & A_WAITACCESS)) /* do not show if it's a pending access */
		{
			if(++i > MAXACCESSMATCHES && !all) continue;
			show_accessn(a, a->user, nick);
			csreply(nick, "-");
		}
	}

	if(i > MAXACCESSMATCHES && !all)
		csreply(nick, GetReply(nick, L_EXCESSMATCHES), i, MAXACCESSMATCHES);
	else csreply(nick, GetReply(nick, L_TOTALFOUND), i, PLUR(i), PLUR(i));
	return 0;
}

/*
 * add_user <salon> <user> <level>
 */
int add_user(aNick *nick, aChan *chan, int parc, char **parv)
{
	anUser *u;
	anAccess *a;
	const char *salon = parv[1];
	int lvl = 0, flag = 0;

	if(!(u = ParseNickOrUser(nick, parv[2]))) return 0;

	if(!Strtoint(parv[3], &lvl, 1, OWNERLEVEL-1))
		return csreply(nick, GetReply(nick, L_VALIDLEVEL));

	if(!IsAdmin(nick->user) && lvl >= ChanLevelbyUserI(nick->user, chan))
		return csreply(nick, GetReply(nick, L_MAXLEVELISYOURS));

	if(lvl >= A_MANAGERLEVEL) flag = A_MANAGERFLAGS;

	if(UPReject(u)) return csreply(nick, GetReply(nick, L_XREFUSEACCESS), u->nick);

	for(a = u->accesshead; a && a->c != chan; a = a->next);
	if(a)
	{
		if(AWait(a)) csreply(nick, GetReply(nick, L_ALREADYAWAIT), u->nick);
		else csreply(nick, GetReply(nick, L_ALREADYACCESS), u->nick, salon);
		return 0;
	}
	add_access(u, salon, lvl, flag|(UPAsk(u) ? A_WAITACCESS : 0), OnChanTS(u, chan));

	if(UPAccept(u))
	{
		csreply(nick, GetReply(nick, L_XHASACCESS), u->nick, lvl, salon);
		if(u->n) csreply(u->n, GetReply(u->n, L_YOUHAVEACCESS), nick->nick, lvl, salon);
	}
	else
	{
		csreply(nick, GetReply(nick, L_XHASAWAIT), u->nick, salon);
		if(u->n)
		{
			csreply(u->n, GetReply(u->n, L_YOUHAVEAWAIT1), nick->nick, lvl, salon);
			csreply(u->n, GetReply(u->n, L_YOUHAVEAWAIT2), cs.nick, RealCmd("myaccess"),
				salon, cs.nick, RealCmd("myaccess"), salon);
		}
	}
#ifdef USE_MEMOSERV
	if(!u->n)
	{
		char memo[MEMOLEN + 1];
		mysnprintf(memo, MEMOLEN, GetReply(nick, UPAsk(u) ? L_YOUHAVEAWAIT1 : L_YOUHAVEACCESS),
			nick->nick, lvl, salon);
		add_memo(u, nick->user->nick, CurrentTS, memo, MEMO_AUTOEXPIRE);
	}
#endif
	return 0;
}

/*
 * del_user <salon> <user>
 */
int del_user(aNick *nick, aChan *chan, int parc, char **parv)
{
	anUser *u;
	anAccess *a;
	const char *salon = parv[1];

	if(!(u = ParseNickOrUser(nick, parv[2]))) return 0;

	for(a = u->accesshead; a && a->c != chan; a = a->next);
	if(!a) return csreply(nick, GetReply(nick, L_XNOACCESSON), u->nick, salon);

	if(!IsAdmin(nick->user) && ChanLevelbyUserI(nick->user, chan) <= a->level)
		return csreply(nick, GetReply(nick, L_GREATERLEVEL), u->nick);

	if(a->level == OWNERLEVEL) return csreply(nick, GetReply(nick, L_XISOWNER), u->nick);

	del_access(u, chan);

	csreply(nick, GetReply(nick, L_OKDELETED), u->nick);
	return 1;
}

int kick(aNick *nick, aChan *chan, int parc, char **parv)
{
	const char *c = parv[1], *cible = parv[2];
	aNick *targ;

	if(!HasWildCard(cible))
	{	/*la cible est un nick unique .. */

		if(!strcasecmp(cs.nick, cible)) return csreply(nick, "Non!");

		if(!(targ = GetMemberIbyNick(chan, cible)))
			return csreply(nick, GetReply(nick, L_NOTONCHAN), cible, chan->chan);

		if(!check_protect(nick, targ, chan)) return 0;

		cskick(c, targ->numeric, "(\2$\2) $", nick->user->nick,
			(parc < 3) ? cf_defraison : parv2msg(parc, parv, 3, RAISONLEN));
	}
	else
	{	/* kick sur un mask*/
		const char *r = (parc < 3) ? cf_defraison : parv2msg(parc, parv, 3, RAISONLEN);
		aLink *lp = chan->netchan->members;

		for(; lp; lp = lp->next)
		{
			targ = lp->value.j->nick;

			if(nick != targ && !match(cible, GetNUHbyNick(targ, 0))
				&& check_protect(nick, targ, chan))
					cskick(c, targ->numeric, "(\2$\2) $", nick->user->nick, r);
		}
	}
	return 1;
}

int mode(aNick *nick, aChan *chan, int parc, char **parv)
{
	int k = 0, l = 0;
	char *modes = parv[2], *c = strchr(modes, 'o');

	if(c || (c = strchr(modes, 'b')) || (c = strchr(modes, 'v'))
#ifdef HAVE_OPLEVELS
	 || (c = strchr(modes, 'A')) || (c = strchr(modes, 'U'))
#endif
	 || (!IsOper(nick) && (c = strchr(modes, 'O'))))
		return csreply(nick, GetReply(nick, L_CANTCHANGEMODE), *c);

	if(chan->cml && !IsAdmin(nick->user) && ChanLevelbyUserI(nick->user, chan) < chan->cml)
		return csreply(nick, GetReply(nick, L_CMLIS), chan->cml);

	if((k = count_char(modes, 'k')) > 1 || (l = count_char(modes, 'l')) > 1)
		return csreply(nick, "Erreur de syntaxe");
	k += l;

	csmode(chan, 0, "$ $ $", modes, (k && parc > 2) ? parv[3] : "",
		(k > 1 && parc > 3) ? parv[4] : "");
	return 1;
}

int topic(aNick *nick, aChan *chan, int parc, char **parv)
{
	if(CLockTopic(chan)) return csreply(nick, GetReply(nick, L_LOCKTOPIC), parv[1]);

	cstopic(chan, parv2msg(parc, parv, 2, TOPICLEN));
	return 1;
}

int rdeftopic(aNick *nick, aChan *chan, int parc, char **parv)
{
	if(CLockTopic(chan)) return csreply(nick, GetReply(nick, L_LOCKTOPIC), parv[1]);

	if(*chan->deftopic) cstopic(chan, chan->deftopic);
	return 1;
}

int rdefmodes(aNick *nick, aChan *chan, int parc, char **parv)
{
	modes_reset_default(chan);
	return 1;
}

int info(aNick *nick, aChan *chan, int parc, char **parv)
{
	anAccess *acces = GetAccessIbyUserI(nick->user, chan);

	if(!acces) return 0;

	if(parc < 2) return csreply(nick, GetReply(nick, L_INFOLINEORNONE));

	if(!strcasecmp(parv[2], "none"))
	{
		if(acces->info) free(acces->info), acces->info = NULL;
		csreply(nick, GetReply(nick, L_OKDELETED), chan->chan);
	}
	else
	{
		str_dup(&acces->info, parv2msg(parc, parv, 2, 199));
		csreply(nick, GetReply(nick, L_INFOCHANGED), chan->chan, acces->info);
	}
	return 0;
}

int invite(aNick *nick, aChan *chan, int parc, char **parv)
{
	aNick *inv = nick;

	if(parc > 1 && strcasecmp(parv[2], nick->nick))
	{
		if(!IsAdmin(nick->user)) return csreply(nick, GetReply(nick, L_CANTINVELSE));
		else if(!(inv = getnickbynick(parv[2])))
				return csreply(nick, GetReply(nick, L_NOSUCHNICK), parv[2]);
	}

	if(GetJoinIbyNC(inv, chan->netchan))
		return csreply(nick, GetReply(nick, L_ALREADYONCHAN), inv->nick, chan->chan);

	if(is_ban(inv, chan, NULL))
		return csreply(nick, GetReply(nick, L_XISBANCANTINV), inv->nick, chan->chan);

	putserv("%s " TOKEN_INVITE " %s :%s", cs.num, inv->nick, chan->chan);
	putserv("%s " TOKEN_WALLCHOPS " %s :Invitation de %s",	cs.num,	chan->chan, inv->nick);
	return 1;
}

int clearmodes(aNick *nick, aChan *chan, int parc, char **parv)
{
	csmode(chan, 0, "-$", GetCModes(chan->netchan->modes));
	return 1;
}

/*
 * see_alist #
 */
int see_alist(aNick *nick, aChan *chan, int parc, char **parv)
{
	aLink *lp = chan->access;
	char buf[450] = {0};
	unsigned int used = 0, first = 1;

#define ITEM_SIZE (2*NICKLEN + 10) /* @ [ \2 500 \2 ] ' ' + \0 */

	for(; lp; lp = lp->next)
	{
		anAccess *a = lp->value.a;
		if(a->user->n && !AWait(a) && !ASuspend(a))
		{
			if(used + ITEM_SIZE >= sizeof buf)
			{
				csreply(nick, first ? "Liste des identifiés: %s" : "%s", buf);
				used = first = 0;
			}
			used += mysnprintf(buf + used, sizeof buf - used, "%s@%s[\002%d\2] ",
						a->user->n->nick, a->user->nick, a->level);
		}
	}
	if(*buf) csreply(nick, first ? "Liste des identifiés: %s" : "%s", buf);
	else if(first) csreply(nick, GetReply(nick, L_NOINFOAVAILABLE));
	return 1;
}

int admin_say(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	putserv("%s " TOKEN_PRIVMSG " %s :%s",
		cs.num, parv[1], parv2msg(parc, parv, 2, 400));
	return 1;
}

int admin_do(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	putserv("%s " TOKEN_PRIVMSG " %s :\1ACTION %s\1",
		cs.num, parv[1], parv2msg(parc, parv, 2, 400));
	return 1;
}

