/* src/memoserv.c - Diverses commandes sur le module memoserv
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
 * $Id: memoserv.c,v 1.48 2008/01/04 13:21:34 romexzf Exp $
 */

#include "main.h"
#ifdef USE_MEMOSERV
#include "hash.h"
#include "outils.h"
#include "cs_cmds.h"
#include "add_info.h"
#include "memoserv.h"

static item_opt memo_opts[10] = { {0} };

static void memo_free(anUser *user, aMemo *memo)
{
	if(memo->next) memo->next->last = memo->last;
	else user->memotail = memo->last;
	if(memo->last) memo->last->next = memo->next;
	else user->memohead = memo->next;
	free(memo);
}

void show_notes(aNick *nick)
{
	aMemo *memo = nick->user->memohead;
	int i = 0;

	for(; memo; memo = memo->next) if(!MemoRead(memo)) ++i;

	if(i) csreply(nick, GetReply(nick, L_HAVENEWMEMO), i, PLUR(i),
				cs.nick, RealCmd("memos"), PLUR(i));
}

int memo_del(anUser *user, item_opt *opts, int size, int count, aNick *nick)
{
	aMemo *memo = user->memohead, *mnext = NULL;
	int current = 0, found = 0;

	for(; memo; memo = mnext)
	{
		mnext = memo->next;
		++current;
		if(!size) free(memo), ++found; /* del them all */
		else if(item_isinlist(opts, size, current) != -1) /* need to be deleted */
		{
			memo_free(user, memo);
            if(nick) csreply(nick, GetReply(nick, L_MEMODEL), current);
            if(++found == count) break;
		}
	}

	if(!size) user->memohead = user->memotail = NULL; /* all */

	if(nick)
	{   /* most frequent case : del only one memo give a specific answer */
	    if(count == 1 && !found) csreply(nick, GetReply(nick, L_MEMONOTFOUND), opts[0].min);
        /* else report summary count (del * or list != 1) */
	    else if(!size || found != 1) csreply(nick, GetReply(nick, L_MEMOSDEL), found);
	}
	return found;
}

int memos(aNick *nick, aChan *chan, int parc, char **parv)
{
	aMemo *memo = nick->user->memohead;
	int i = 0;
	const char *cmd = parv[1];

	if(!strcasecmp(cmd, "LIRE") || !strcasecmp(cmd, "READ"))
	{	/* default (no arg) is NEW. */
		aMemo *mnext = NULL;
        int current = 0, found = 0;

		if(parc < 2 || !strcasecmp(parv[2], "NEW")) i = -2;
		else if(!strcasecmp(parv[2], "ALL")) i = -1;
		else if((i = strtol(parv[2], NULL, 10)) <= 0)
				return csreply(nick, "Syntaxe : %s READ <ALL|NEW|n°[,list]>", parv[0]);

		for(; memo; memo = mnext)
		{
			mnext = memo->next;
			if(++current == i || i == -1 || (i == -2 && !MemoRead(memo)))
			{
				csreply(nick, "%c#%d De\2 %s\2 [%s] : %s", MemoRead(memo)	? ' ' : '*',
					current, memo->de, get_time(nick, memo->date), memo->message);
				++found;
				if(memo->flag & MEMO_AUTOEXPIRE) memo_free(nick->user, memo);
				else if(!MemoRead(memo)) memo->flag |= MEMO_READ;
				if(i > 0) break;
			}
		}

		if(found)
		{
			if(i == -1) csreply(nick, GetReply(nick, L_MEMOTOTAL), found, cs.nick, parv[0]);
			else if(i == -2) csreply(nick, GetReply(nick, L_MEMONEWTOTAL), found);
			else csreply(nick, GetReply(nick, L_MEMOEND), i, cs.nick, parv[0], i);
		}
		else if(i == -1) csreply(nick, GetReply(nick, L_NOMEMOFOUND));
		else if(i == -2) csreply(nick, GetReply(nick, L_MEMONEWNOTFOUND));
		else csreply(nick, GetReply(nick, L_MEMONOTFOUND), i);
	}

	else if(!strcasecmp(cmd, "SUPPR") || !strcasecmp(cmd, "ERASE")
		|| !strcasecmp(cmd, "SUPPRIMER") || !strcasecmp(cmd, "DEL"))
	{
		int count = 0;

        if(!nick->user->memohead) return csreply(nick, GetReply(nick, L_NOMEMOFOUND));

		if(parc < 2 || (strcasecmp(parv[2], "all")
		&& !(i = item_parselist(parv[2], memo_opts, ASIZE(memo_opts), &count))))
			return csreply(nick, "Syntaxe : %s DEL <ALL|n°[,list]>", parv[0]);

		memo_del(nick->user, memo_opts, i, count, nick /* verbose */);
	}

	else if(!strcasecmp(cmd, "SEND") || !strcasecmp(cmd, "ENVOYER"))
	{
		anUser *user;
		aNick *who;
		char *tmp;

		if(parc < 3)
			return csreply(nick, "Syntaxe : %s SEND <username> <message>", parv[0]);

		if(!(user = ParseNickOrUser(nick, parv[2]))) return 0;

		if(user == nick->user)
			return csreply(nick, GetReply(nick, L_CANTSENDYOURSELF));

		if(UNoMemo(user) && !IsAdmin(nick->user))
			return csreply(nick, GetReply(nick, L_XNOMEMO), user->nick);

		tmp = parv2msg(parc, parv, 3, MEMOLEN + 3);

		if(strlen(tmp) > MEMOLEN)
			return csreply(nick, GetReply(nick, L_MEMOLEN), MEMOLEN);

		who = !user->n || IsAway(user->n) ? NULL : user->n;

		if(who) csreply(who, GetReply(who, L_MEMOFROM), nick->nick, nick->user->nick, tmp);

		for(memo = user->memohead; memo; memo = memo->next)
			if(!strcasecmp(memo->de, nick->user->nick)) ++i;

		if(!IsAdmin(nick->user) && i >= MAXMEMOS)
		{
			if(who) csreply(who, GetReply(who, L_MAXMEMOWARN), MAXMEMOS, nick->user->nick);
			else if(i == MAXMEMOS)
			{
				char buf[MEMOLEN + 1];
				mysnprintf(buf, sizeof buf, GetUReply(user, L_MAXMEMOWARN),
					MAXMEMOS, nick->user->nick);
				add_memo(user, "Services", CurrentTS, buf, MEMO_AUTOEXPIRE);
			}
			return csreply(nick, GetReply(nick, L_MAXMEMO), user->nick, MAXMEMOS);
		}

		add_memo(user, nick->user->nick, CurrentTS, tmp, who ? MEMO_READ : 0);
		csreply(nick, GetReply(nick, L_MEMOSENT), user->nick);
	}

	else return csreply(nick, GetReply(nick, L_UNKNOWNOPTION), cmd);
	return 1;
}
#endif
