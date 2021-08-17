/* src/userinfo.c - Affiche les infos détaillées d'un username
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
 * $Id: userinfo.c,v 1.5 2008/01/04 13:21:34 romexzf Exp $
 */

#include "main.h"
#include "outils.h"
#include "cs_cmds.h"
#include "data.h"
#include "userinfo.h"
#ifdef HAVE_VOTE
#include "vote.h"
#endif

static char *GetUserOptions(anUser *user)
{	 /* last '15' is langsize, 1 is \0 */
	static char option[8+9+7+12+6+9+12+12+7+9+17+19+15+15+1];
	int i = 0;
	option[0] = '\0';

	if(UNopurge(user)) strcpy(option, "NoPurge "), i += 8;
	if(USuspend(user)) strcpy(option + i, "Suspendu "), i += 9;
	if(UNoMemo(user)) strcpy(option + i, "NoMemo "), i += 7;
	if(UFirst(user)) strcpy(option + i, "Registering "), i += 12;
	if(UOubli(user)) strcpy(option + i, "Oubli "), i += 6;
	if(UWantX(user)) strcpy(option + i, "AutoHide "), i += 9;
	if(UChanged(user)) strcpy(option + i, "UserChanged "), i += 12;
	if(UCantRegChan(user)) strcpy(option + i, "CantRegChan "), i += 12;
	if(UNoVote(user)) strcpy(option + i, "NoVote "), i += 7;
	if(UPMReply(user)) strcpy(option + i, "ReplyMsg "), i += 9;
#ifdef USE_NICKSERV
	if(UPKill(user)) strcpy(option + i, "Protection(Kill) "), i += 17;
	else if(UPNick(user)) strcpy(option + i, "Protection(ChNick) "), i += 19;
#endif
	if(UPReject(user)) strcpy(option + i, "Accès(Rejet) "), i += 13;
	else if(UPAsk(user)) strcpy(option + i, "Accès(Ask) "), i += 11;
	else if(UPAccept(user)) strcpy(option + i, "Accès(Accepte) "), i += 15;
	strcpy(option + i, user->lang->langue);

	if(option[0] == '\0') strcpy(option, "Aucune");
	return option;
}

int show_userinfo(aNick *nick, anUser *user, int flag)
{
	anAccess *a;
	char buf_a[451] = {0}, buf_w[451] = {0};
	unsigned int used_a = 0, used_w = 0, first_a = 1, first_w = 1;

#define ITEM_SIZE (REGCHANLEN + 10) /* channel + " (" + level + ") " */

	csreply(nick, GetReply(nick, L_INFO_ABOUT), user->nick);
	csreply(nick, GetReply(nick, L_MAILIS), user->mail, UNDEF(user->reg_time));

	if(UFirst(user))
		csreply(nick, GetReply(nick, L_UREGTIME), duration(CurrentTS - user->lastseen));
	else csreply(nick, GetReply(nick, L_LASTLOGINTIME), duration(CurrentTS - user->lastseen),
			user->lastlogin ? user->lastlogin : "<unknown>");

	if(IsAdmin(user)) csreply(nick, GetReply(nick, L_USERISADMIN), user->level);

	for(a = user->accesshead; a; a = a->next)
	{
		if(AWait(a))
		{
			if(used_w + ITEM_SIZE >= sizeof buf_w)
			{
				csreply(nick, first_w ? GetReply(nick, L_PROPACCESS) : "%s ...", buf_w);
				used_w = first_w = 0;
			}
			used_w += mysnprintf(buf_w + used_w, sizeof buf_w - used_w, "%s (%d) ",
							a->c->chan, a->level);
		}
		else
		{
			if(used_a + ITEM_SIZE >= sizeof buf_a)
			{
				csreply(nick, first_a ? GetReply(nick, L_ACCESSON) : "%s ...", buf_a);
				used_a = first_a = 0;
			}
			used_a += mysnprintf(buf_a + used_a, sizeof buf_a - used_a, "%s (%d) ",
							a->c->chan, a->level);
		}
	}

	if(used_a) csreply(nick, first_a ? GetReply(nick, L_ACCESSON) : "%s", buf_a);
	else if(first_a) csreply(nick, GetReply(nick, L_NOACCESS));
	if(used_w) csreply(nick, first_w ? GetReply(nick, L_PROPACCESS) : "%s", buf_w);
	else if(first_w) csreply(nick, GetReply(nick, L_NOPROPACCESS));

	if(flag && user->n) csreply(nick, " %s%s est actuellement logué sous cet Username (%s@%s)",
		GetPrefix(user->n), user->n->nick, user->n->ident, user->n->host);
	else if(flag) csreply(nick, " Personne n'est actuellement logué sous cet username");

	csreply(nick, GetReply(nick, L_OPTIONS), GetUserOptions(user));

	if(user->suspend) show_ususpend(nick, user);
	if(user->cantregchan) show_cantregchan(nick, user);
	if(user->nopurge) show_nopurge(nick, user);

#ifdef HAVE_VOTE
	if(!flag && CanVote(user))
		csreply(nick, " Vous n'avez pas encore voté pour: %s", vote[0].prop);
#endif
	return 0;
}
