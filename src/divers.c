/* src/divers.c - Diverses commandes
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
 * $Id: divers.c,v 1.68 2008/01/05 01:24:13 romexzf Exp $
 */

#include "main.h"
#include "version.h"
#include "divers.h"
#include "config.h"
#include "outils.h"
#include "hash.h"
#include "mylog.h"
#include "cs_cmds.h"
#include "admin_manage.h"
#include "flood.h"
#include "showcommands.h"
#include "aide.h"
#include "data.h"
#include "dnr.h"
#include "timers.h"
#include "template.h"
#include "socket.h"

int uptime(aNick *nick, aChan *chan, int parc, char **parv)
{
	return csntc(nick, GetReply(nick, L_UPTIME), duration(CurrentTS - bot.uptime));
}

int ctcp_ping(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	if(parc) csntc(nick, "\1PING %s", parv[1]);
	return 1;
}

int ctcp_version(aNick *nick, aChan *chan, int parc, char **parv)
{
    csntc(nick, "\1VERSION Services SDreams " SPVERSION " (Rev:" REVDATE ") "
    	"http://www.bugsounet.fr/ (Build " __DATE__ " "__TIME__ ")\1");
    csntc(nick, "\1VERSION Copyright (C) 2021 @bugsounet\1");
	return 1;
}

int lastseen(aNick *nick, aChan *chan, int parc, char **parv)
{
	anUser *u = getuserinfo(parv[1]);

	if(!u) csntc(nick, GetReply(nick, L_NOSUCHUSER), parv[1]);
	else if(u->n) csntc(nick, GetReply(nick, L_ELSEALREADYLOG));
	else csntc(nick, GetReply(nick, L_FULLLASTSEEN), u->nick,
				duration(CurrentTS - u->lastseen), get_time(nick, u->lastseen));
	return 1;
}

int show_admins(aNick *nick, aChan *chan, int parc, char **parv)
{
	int i = 0;
#ifdef OLDADMINLIST
	anUser *u;
	csntc(nick, "\2Présent  Niveau  Username       Pseudo\2");
	for(; i < USERHASHSIZE; ++i) for(u = user_tab[i]; u; u = u->next)
		if(IsAdmin(u)) csntc(nick, "\2\003%s\2\3      %d       %-13s  \0032%s\3",
				u->n ? IsAway(u->n) ? "14ABS" : "3OUI" : "4NON",
				u->level, u->nick, u->n ? u->n->nick : "");
#else
	aNick *n = NULL;
	char nicks[205] = {0};
	size_t size = 0;

	for(; i < adminmax; ++i)
	{
		size_t tmps = size;
		if(!(n = adminlist[i]) || UIsBusy(n->user) || IsAway(n)) continue;

		size += strlen(n->nick);
		if(size >= sizeof nicks - 5) break; /* He gets enough admins to be helped.. */
		strcpy(&nicks[tmps], n->nick);
		nicks[size++] = ' ';
		nicks[size] = 0; /* note that all bytes are set to 0 but.. */
	}
	if(!*nicks) return csntc(nick, GetReply(nick, L_NOADMINAVAILABLE));
	csntc(nick, GetReply(nick, L_ADMINAVAILABLE));
	csntc(nick, "  %s", nicks);
#endif
	return 1;
}

int verify(aNick *nick, aChan *chan, int parc, char **parv)
{
	aNick *n;

	if(!strcasecmp(parv[1], cs.nick)) return csntc(nick, "Yeah, c'est bien moi :)");
	if(!(n = getnickbynick(parv[1]))) return csntc(nick, GetReply(nick, L_NOSUCHNICK), parv[1]);
	if(!n->user) return csntc(nick, GetReply(nick, L_NOTLOGUED), n->nick);

	csntc(nick, "%s est logué sous l'username %s %s %s",
		(IsAnAdmin(nick->user) || n == nick) ? GetNUHbyNick(n, 0) : n->nick, n->user->nick,
		IsAdmin(n->user) ? "- Administrateur des Services" : "", IsOper(n) ? "- IRCop" : "");
	return 1;
}

void CleanUp(void)
{
	aChan *c, *ct;
	anUser *u, *ut;
	aHashCmd *cmd, *cmd2;
	int i = 0;

	free(cf_quit_msg);
	free(cf_mailprog);
	free(cf_pasdeperm);

	socket_close();
	purge_network(); /* nicks(+joins) + netchans + servers */

	for(i = 0; i < CHANHASHSIZE; ++i) for(c = chan_tab[i]; c; c = ct)
	{
		aLink *lp = c->access, *lp_t = NULL;
		aBan *b = c->banhead, *bt = NULL;

		ct = c->next;
		/* Access links */
		for(; lp; free(lp), lp = lp_t) lp_t = lp->next;
		/* Bans */
		for(; b; free(b->raison), free(b->mask), free(b), b = bt) bt = b->next;

		if(c->suspend) data_free(c->suspend);
		free(c->motd);
		free(c);
	}
	for(i = 0; i < USERHASHSIZE; ++i) for(u = user_tab[i]; u; u = ut)
	{
		anAccess *a = u->accesshead, *at = NULL;
#ifdef USE_MEMOSERV
		aMemo *m = u->memohead, *mt = NULL;
		for(; m; free(m), m = mt) mt = m->next;
#endif
		ut = u->next;
		/* Access */
		for(; a; free(a->info), free(a), a = at) at = a->next;

		if(u->suspend) data_free(u->suspend);
		if(u->cantregchan) data_free(u->cantregchan);
		if(u->nopurge) data_free(u->nopurge);
		free(u->cookie);
		free(u->lastlogin);
		free(u);
	}

	for(i = 0; i < CMDHASHSIZE; ++i) for(cmd = cmd_hash[i]; cmd; cmd = cmd2)
	{
		int j = 0, h = 0;
		cmd2 = cmd->next;
		for(; j < LangCount ; ++j, h = 0)
		{
			if(!cmd->help[j]) continue; /* no help (ping etc.) */
			for(; h < cmd->help[j]->count; ++h) free(cmd->help[j]->buf[h]);
			free(cmd->help[j]->buf);
			free(cmd->help[j]);
		}
		free(cmd->help);
		free(cmd);
	}

	tmpl_clean();
	lang_clean();
	ignore_clean();
	timer_clean();
	dnr_clean();
	log_clean(0);
}

