/* src/chanserv.c - Diverses commandes sur le module chanserv
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
 * $Id: chanserv.c,v 1.50 2006/03/15 19:04:42 bugs Exp $
 */

#include "main.h"
#include "hash.h"
#include "cs_cmds.h"
#include "outils.h"
#include "chanserv.h"
#include "ban.h"
#include "add_info.h"
#include "del_info.h"
#include "fichiers.h"
#include "config.h"
#include "admin_user.h"
#include "divers.h"
#include "template.h"

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
	char *nick2 = parv[2], comp = '\0';
	int i = 0, autoop, autohop, protect, suspend, autovoice, all, wait, l = 0, wild = 0;

	if(chan->netchan && HasMode(chan->netchan, C_MSECRET | C_MPRIVATE) /* salon +s ou +p */
		&& (!nick->user || !GetAccessIbyUserI(nick->user, chan))	/* non logué ou n'a pas d'accès */
		&& !IsAnAdmin(nick->user) && !GetJoinIbyNC(nick, chan->netchan)) /* ET ni admin, ni présent */
			return csreply(nick, GetReply(nick, L_YOUNOACCESSON), chan->chan);

	if(*nick2 == '>' || *nick2 == '<' || *nick2 == '=')
	{
		comp = *nick2++;
		if(!*nick2 || !is_num(nick2))
			return csreply(nick, "La valeur du niveau doit être numérique derrière <,> ou =");
		l = atoi(nick2);
	}
	else wild = HasWildCard(nick2);
	
	autoop = getoption("-autoop", parv, parc, 3, -1);
	autohop = getoption("-autohop", parv, parc, 3, -1);
	autovoice = getoption("-autovoice", parv, parc, 3, -1);
	protect = getoption("-protect", parv, parc, 3, -1);
	suspend = getoption("-suspend", parv, parc, 3, -1);
	wait = getoption("-wait", parv, parc, 3, -1);
	all = nick->user && (IsAdmin(nick->user) || (getoption("-all", parv, parc, 3, -1) &&
		(chan->owner->user == nick->user || ChanLevelbyUserI(nick->user, chan) >= 450))) ? 1 : 0;

	csreply(nick, GetReply(nick, L_ACCESSLIST), chan->chan);

	for(;lp;lp = lp->next)
	{
		register anAccess *a = lp->value.a;

		if(((wait && AWait(a)) || (!wait && !AWait(a)))
			&& ((comp != '\0' && ((comp == '>' && a->level > l) /* recherche */
			          || (comp == '<' && a->level < l)	/* via level */
			          || (comp == '=' && l == a->level)))
			  || (*nick2 == '*' && !nick2[1]) /* recherche sur '*' */
			  || (wild /* ou sur un vrai mask */
			  		&& !match(nick2, a->user->nick))
			  || !strcasecmp(nick2, a->user->nick)) /* ou access exact */
			&& (!autoop || AOp(a)) /* ET match des options Aop/Avoice/Suspend etc */
			&& (!autohop || AHalfop(a))
			&& (!autovoice || AVoice(a))
			&& (!protect || AProtect(a))
			&& (!suspend || ASuspend(a)))
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
int add_user(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	anUser *u;
	anAccess *a;
	aLink *lp;
	const char *salon = parv[1];
	int lvl, flag = 0, ma;

	for(ma = 0, lp = chaninfo->access;lp;lp = lp->next, ma++) 
		if(ma >= WARNACCESS - 1) return csreply(nick, "La liste des accès est pleine! (Maximum = %d)", WARNACCESS);

	if(!(u = ParseNickOrUser(nick, parv[2]))) return 0;

	if((lvl = strtol(parv[3], NULL, 10)) < 1 || lvl >= OWNERLEVEL)
		return csreply(nick, GetReply(nick, L_VALIDLEVEL));

	if(lvl >= ChanLevelbyUserI(nick->user, chaninfo) && !IsAdmin(nick->user))
		return csreply(nick, GetReply(nick, L_MAXLEVELISYOURS));

	if(lvl >= 400) flag = A_OP|A_PROTECT;

	if(UPReject(u)) return csreply(nick, GetReply(nick, L_XREFUSEACCESS), u->nick);

	for(a = u->accesshead;a && a->c != chaninfo;a = a->next);
	if(a)
	{
		if(AWait(a)) csreply(nick, GetReply(nick, L_ALREADYAWAIT), u->nick);
                else csreply(nick, GetReply(nick, L_ALREADYACCESS), u->nick, salon);
                return 0;
	}
	add_access(u, salon, lvl, flag|(UPAsk(u) ? A_WAITACCESS : 0), CurrentTS);

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
	if(GetConf(CF_MEMOSERV))
	{
		char memo[MEMOLEN + 1], memomail[MEMOLEN +1];
		if(UPAsk(u)) 
			mysnprintf(memomail, MEMOLEN, "%s vous a proposé un access de %d sur %s.\nPour accepter tapez: /%s %s ACCEPT %s\nPour refuser tapez: /%s %s REFUSE %s",
				nick->nick, lvl, salon, cs.nick, RealCmd("myaccess"), salon, cs.nick, RealCmd("myaccess"), salon);
		else
			mysnprintf(memomail, MEMOLEN, "\2%s\2 vous a donné un accès de niveau %d sur \2%s\2.", nick->nick, lvl, salon);

		mysnprintf(memo, MEMOLEN, GetReply(nick, UPAsk(u) ? L_YOUHAVEAWAIT1 : L_YOUHAVEACCESS), nick->nick, lvl, salon);
		if(!UNoMail(u)) tmpl_mailsend(&tmpl_mail_memo, u->mail, u->nick, NULL, NULL, nick->user->nick, memomail);
		if(!u->n) add_memo(u, nick->user->nick, CurrentTS, memo, MEMO_AUTOEXPIRE);
	}
	return 0;
}

/*
 * del_user <salon> <user>
 */
int del_user(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	anUser *u;
	anAccess *a;
	const char *salon = parv[1];

	if(!(u = ParseNickOrUser(nick, parv[2]))) return 0;

	for(a = u->accesshead;a && a->c != chaninfo;a = a->next);
	if(!a) return csreply(nick, GetReply(nick, L_XNOACCESSON), u->nick, salon);

	if(ChanLevelbyUserI(nick->user, chaninfo) <= a->level && !IsAdmin(nick->user))
		return csreply(nick, GetReply(nick, L_GREATERLEVEL), u->nick);

	if(a->level == OWNERLEVEL) return csreply(nick, GetReply(nick, L_XISOWNER), u->nick);

	if(GetConf(CF_MEMOSERV))
        {
        	char memomail[MEMOLEN + 1];
                snprintf(memomail, MEMOLEN, "%s a vous supprimé votre accès sur %s", nick->nick, salon);

		if(!UNoMail(u)) tmpl_mailsend(&tmpl_mail_memo, u->mail, u->nick, NULL, NULL, nick->user->nick, memomail);
                if(!u->n) add_memo(u, nick->user->nick, CurrentTS, memomail, MEMO_AUTOEXPIRE);
        }

	del_access(u, chaninfo);

	if(u->n) csreply(u->n, "%s a vous supprimé votre accès sur %s", nick->nick, salon); 

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
			(parc < 3) ? defraison : parv2msg(parc, parv, 3, 150));
	}
	else
	{	/* kick sur un mask*/
		const char *r = (parc < 3) ? defraison : parv2msg(parc, parv, 3, 150);
		aLink *lp = chan->netchan->members;

		for(;lp;lp = lp->next)
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

	if(c || (c = strchr(modes, 'b')) || (c = strchr(modes, 'v')) || (c = strchr(modes, 'h')) || 
		(!IsOper(nick) && (c = strchr(modes, 'O'))))
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

int info(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	anAccess *acces = GetAccessIbyUserI(nick->user, chaninfo);

	if(!acces) return 0;

	if(parc < 2) return csreply(nick, GetReply(nick, L_INFOLINEORNONE));

	if(!strcasecmp(parv[2], "none"))
	{
		if(acces->info) free(acces->info), acces->info = NULL;
		csreply(nick, GetReply(nick, L_OKDELETED), chaninfo->chan);
	}
	else
	{
		str_dup(&acces->info, parv2msg(parc, parv, 2, 199));
		csreply(nick, GetReply(nick, L_INFOCHANGED), chaninfo->chan, acces->info);
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
	char buf[400] = {0}, tmp[2*NICKLEN+9]; 
        int i = 0, c = 0; 

	for(;lp;lp = lp->next)
		{
		anAccess *a = lp->value.a; 
                if(a->user->n && !AWait(a) && !ASuspend(a)) 
			{
				int tmps = i;
				int j = mysnprintf(tmp, sizeof tmp, "%s@%s[\002%d\2] ", a->user->n->nick, a->user->nick, a->level);
				i += j;
                        	if(i >= sizeof buf)
				{
					csreply(nick, c++ ? "%s" : "Liste des identifiés: %s", buf); 
                                	i = j; 
                                	strcpy(buf, tmp); 
                        	} 
                        	else strcpy(&buf[tmps], tmp); 
			}
		}
	if(*buf) csreply(nick, !c ? "Liste des identifiés: %s" : "%s", buf); 
        else if(!c) csreply(nick, GetReply(nick, L_NOINFOAVAILABLE)); 
	return 1;
}

int admin_say(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	putserv("%s " TOKEN_PRIVMSG " %s :%s", cs.num, parv[1], parv2msg(parc, parv, 2, 400));
	return 1;
}

int admin_do(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	putserv("%s " TOKEN_PRIVMSG " %s :\1ACTION %s\1", cs.num, parv[1], parv2msg(parc, parv, 2, 400));
	return 1;
}
