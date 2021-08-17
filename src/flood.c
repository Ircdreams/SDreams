/* src/flood.c - protection contre le flood
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
 * $Id: flood.c,v 1.22 2007/12/16 20:48:15 romexzf Exp $
 */

#include "main.h"
#include "cs_cmds.h"
#include "config.h"
#include "timers.h"
#include "hash.h"
#include "outils.h"
#include "flood.h"
#include "debug.h"

struct ignore {
	char host[BASE64LEN + 1];
	time_t expire;
	struct ignore *next, *last;
};

static struct ignore *IgnoreList = NULL;

static void add_ignore(const char *mask)
{
	struct ignore *i = malloc(sizeof *i);

	if(!i)
	{
		Debug(W_MAX|W_WARN, "add_ignore, malloc a échoué pour l'ignore de %s", mask);
		return;
	}

	strcpy(i->host, mask);
	i->expire = CurrentTS + cf_ignoretime;
	timer_add(i->expire, TIMER_ABSOLU, callback_delignore, i, NULL);

	i->last = NULL;
	if(IgnoreList) IgnoreList->last = i;
	i->next = IgnoreList;
	IgnoreList = i;
}

/* Renvoie 1 si un flood est detecté */
int checkflood(aNick *nick)
{
	if(GetConf(CF_ADMINEXEMPT) && IsAnAdmin(nick->user)) return 0;

	if(nick->floodtime + FLOODTIME < CurrentTS) /* le privmsg precedent date de trop longtemps */
	{
		nick->floodtime = CurrentTS; /* on met a jour la variable du dernier privmsg */
		nick->floodcount = 1; /* on remet a 1 le nombre de lignes */
	}
	else /* le privmsg est assez récent pour etre controlé */
	{
		if(nick->floodcount >= FLOODLINES) /* flood detecté ! */
		{
			add_ignore(nick->base64);
			cswall("Flood de %s%s (%s@%s)", GetPrefix(nick), nick->nick, nick->ident, nick->host);

			if(GetConf(CF_KILLFORFLOOD))
			{
				putserv("%s "TOKEN_KILL" %s :%s (%s)", cs.num, nick->numeric,
					cs.nick, GetReply(nick, L_FLOODMSG));
				del_nickinfo(nick->numeric, "Flood kill");
			}
			else csntc(nick, GetReply(nick, L_FLOODMSG));
			return 1;
		}
		else ++nick->floodcount;
	}
	return 0;
}

int isignore(aNick *nick)
{
	struct ignore *i = IgnoreList;

	for(; i && strcmp(i->host, nick->base64); i = i->next);
	return i ? 1 : 0;
}

int callback_delignore(Timer *timer)
{
	struct ignore *i = timer->data1;

	if(i->next) i->next->last = i->last;
	if(i->last) i->last->next = i->next;
	else IgnoreList = i->next;
	free(i);
	return 1;
}

void ignore_clean(void)
{
	struct ignore *i = IgnoreList, *ii;

	for(; i; i = ii)
	{
		ii = i->next;
		free(i);
	}
	IgnoreList = NULL;
}

int show_ignores(aNick *nick, aChan *chan, int parc, char **parv)
{
	struct ignore *i = IgnoreList;
	int c = 0;

	if(!IgnoreList) return csntc(nick, GetReply(nick, L_NOINFOAVAILABLE));

	csntc(nick, "Liste des Ignorés:");
	for(; i; i = i->next)
	{
		struct irc_in_addr ip;
		base64toip(i->host, &ip);
		csntc(nick,"\002%d\2. \2%s\2 Expire dans %s",
			++c, GetIP(&ip, NULL), duration(i->expire - CurrentTS));
	}
	return 1;
}
