/* src/admin_chan.c - commandes admin pour gerer les salons
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
 * $Id: admin_chan.c,v 1.100 2008/01/05 01:24:13 romexzf Exp $
 */

#include "main.h"
#include "outils.h"
#include "mylog.h"
#include "cs_cmds.h"
#include "fichiers.h"
#include "admin_chan.h"
#include "add_info.h"
#include "del_info.h"
#include "hash.h"
#include "cs_register.h"
#include "data.h"

#define MAXMATCHES 40

int admin_chan(aNick *nick, aChan *chan, int parc, char **parv)
{
	char *c = NULL;
	const char *arg = parv[1];

	if(parc > 1) chan = getchaninfo((c = parv[2]));

	if(!strcasecmp(arg, "suspend"))
	{
		time_t timeout = 0;
		char *ptr = NULL;

		if(parc < 2) return csreply(nick, "Syntaxe: %s SUSPEND <salon> [arg]", parv[0]);

		if(!chan) return csreply(nick, GetReply(nick, L_NOSUCHCHAN), c);

		if(parc >= 3) /* do more parsing */
		{
			if(*parv[3] == '%' && (timeout = convert_duration(++parv[3])) <= 0)
				return csreply(nick, GetReply(nick, L_INCORRECTDURATION));
			ptr = parv2msg(parc, parv, timeout ? 4 : 3, 250);
		}
		if(chan->suspend && !CSuspend(chan)) data_free(chan->suspend), chan->suspend = NULL;

		switch(data_handle(chan->suspend, nick->user->nick, ptr, timeout,
				DATA_T_SUSPEND_CHAN, chan))
		{
			case -1: /* error */
				return csreply(nick, "Veuillez préciser une raison pour suspendre ce salon.");
			case 0: /* deleted */
				csreply(nick, "Le salon \2%s\2 n'est plus suspendu.", c);
				log_write(LOG_CCMD, 0, "suspend %s off par %s@%s",
					c, nick->nick, nick->user->nick);
				break;
			case 1: /* created */
				if(CJoined(chan)) cspart(chan, chan->suspend->raison);
				log_write(LOG_CCMD, 0, "suspend %s on par %s@%s (%s)",
					c, nick->nick, nick->user->nick, chan->suspend->raison);
			case 2: /* updated */
				show_csuspend(nick, chan);
		}
	}

	else if(!strcasecmp(arg, "del"))
	{
		char tmp[300];
		if(parc < 2) return csreply(nick, "Syntaxe: %s DEL <salon> <raison>", parv[0]);

		if(!chan) return csreply(nick, GetReply(nick, L_NOSUCHCHAN), c);

		mysnprintf(tmp, sizeof tmp, "Delchan par %s@%s (%s)", nick->nick,
			nick->user->nick, (parc > 2) ? parv2msg(parc, parv, 3, 200) : "Aucune raison");

		csreply(nick, "Le salon %s a bien été supprimé.", c);
		del_chan(chan, HF_LOG, tmp);
	}

	else if(!strcasecmp(arg, "reg"))
	{
		anUser *u;
		if(parc < 3)
			return csreply(nick, "Syntaxe: %s REG <salon> <Username|%%nick> [thème]", parv[0]);

		if(chan) return csreply(nick, GetReply(nick, L_ALREADYREG), c);

		if(!chan_check(c, nick)) return 0;

		if(!(u = ParseNickOrUser(nick, parv[3]))) return 0;

		chan = add_chan(c, (parc > 3) ? parv2msg(parc, parv, 4, DESCRIPTIONLEN) : "Adminreg");
		add_access(u, c, OWNERLEVEL, A_MANAGERFLAGS, OnChanTS(u, chan));
		csjoin(chan, JOIN_FORCE);
		csreply(nick, "%s a été enregistré pour %s.", c, u->nick);
	}

	else if(!strcasecmp(arg, "list"))
	{
		int theme = getoption("-theme", parv, parc, 2, GOPT_STR);
		int modes = getoption("-modes", parv, parc, 2, GOPT_STR);
		int users = getoption("-users", parv, parc, 2, GOPT_INT);
		int defmodes = getoption("-defmodes", parv, parc, 2, GOPT_STR);
		int url = getoption("-url", parv, parc, 2, GOPT_STR);
		int max = getoption("-count", parv, parc, 2, GOPT_INT);
		int warned = getoption("-warned", parv, parc, 2, GOPT_FLAG);
		int topic = getoption("-topic", parv, parc, 2, GOPT_STR), count = 0, i = 0, m = 0;
		unsigned int cmodes = 0U, cdefmodes = 0U;

		if(parc < 2 || *parv[2] == '-') m = 1;
		if(modes) cmodes = cmodetoflag(0U, parv[modes]);
		if(defmodes) cdefmodes = cmodetoflag(0U, parv[defmodes]);
		if(!max) max = strcmp("-all", parv[parc]) ? MAXMATCHES : -1;

		for(; i < CHANHASHSIZE; ++i) for(chan = chan_tab[i]; chan; chan = chan->next)
			if((m || !match(parv[2], chan->chan))
			&& (!theme || !match(parv[theme], chan->description))
			&& (!modes || (chan->netchan && HasMode(chan->netchan, modes) == cmodes))
			&& (!defmodes || HasDMode(chan, defmodes) == cdefmodes)
			&& (!url || !match(parv[url], chan->url))
			&& (!topic || (chan->netchan && !match(parv[topic], chan->netchan->topic)))
			&& (!warned || CWarned(chan))
			&& (!users || (chan->netchan && chan->netchan->users >= users))
			&& (++count <= max || max < 0))
			{
				if(count == 1) csreply(nick, "\2Liste des salons enregistrés :");
				if(CJoined(chan)) csreply(nick, "  %s (%s) [%u]", chan->chan,
					GetCModes(chan->netchan->modes), chan->netchan->users);
				else csreply(nick, "  %s (%s) [\2%s\2]", chan->chan, GetCModes(chan->defmodes),
					CSuspend(chan) ? "Suspendu" : "Pas sur le salon");
			}

		if(count > MAXMATCHES && max == MAXMATCHES)
			csreply(nick, GetReply(nick, L_EXCESSMATCHES), count, MAXMATCHES);
		if(max > 0 && max < count)
			csreply(nick, "Un Total de %d entrées trouvées (%d listées)", count, max);
		else if(i) csreply(nick, "Un Total de %d entrées trouvées", count);
		else csreply(nick, "Aucun salon enregistré correspondant trouvé.");
	}

	else if(!strcasecmp(arg, "suspendlist"))
	{
		int from = getoption("-from", parv, parc, 2, GOPT_STR), i = 0, count = 0, m = 0;
		if(parc < 2 || *parv[2] == '-') m = 1;

		for(; i < CHANHASHSIZE; ++i) for(chan = chan_tab[i]; chan; chan = chan->next)
			if(CSuspend(chan) && (m || !match(parv[2], chan->chan))
				&& (!from || !strcasecmp(parv[from], chan->suspend->from)))
			{
				if(++count == 1) csreply(nick, "\2Liste des salons suspendus :");
				csreply(nick, "  %s Par %s Expire %s Raison: %s",
						chan->chan, chan->suspend->from,
						chan->suspend->expire ? get_time(nick, chan->suspend->expire) : "Jamais",
						chan->suspend->raison);
			}

		csreply(nick, "Un total de\2 %d\2 salons suspendus correspondants.", count);
	}

	else if(!strcasecmp(arg, "join"))
	{
		if(parc < 2) return csreply(nick, "Syntaxe: %s JOIN <salon> [-force]", parv[0]);

		if(!chan) return csreply(nick, GetReply(nick, L_NOSUCHCHAN), c);

		if(CSuspend(chan))
			return csreply(nick, "%s est actuellement suspendu pour %s, utilisez "
					"'%s SUSPEND %s' pour lever le suspend.", c, chan->suspend->raison,
					parv[0], c);

		if(CJoined(chan) && !getoption("-force", parv, parc, 3, GOPT_FLAG))
			return csreply(nick, "Je suis déjà sur %s (En cas de desynch,"
					" utilisez l'argument '-force')", c);

		csjoin(chan, JOIN_FORCE);
	}

	else if(!strcasecmp(arg, "part"))
	{
		if(parc < 2) return csreply(nick, "Syntaxe: %s PART <salon> [raison]", parv[0]);

		if(!chan) return csreply(nick, GetReply(nick, L_NOSUCHCHAN), c);

		if(CJoined(chan)) cspart(chan, (parc > 2) ? parv2msg(parc, parv, 3, RAISONLEN) : "");
	}

	else if(!strcasecmp(arg, "setowner"))
	{/* chan setowner # gna [leve]*/
		anUser *newu;
		anAccess *newo;
		int newlevel = 0;

		if(parc < 3) return csreply(nick, "Syntaxe: %s SETOWNER <chan> <nouvel owner>"
											" [nouveau level de l'ancien owner]", parv[0]);

		if(parc > 3 && !Strtoint(parv[4], &newlevel, 1, OWNERLEVEL-1))
			return csreply(nick, GetReply(nick, L_VALIDLEVEL));

		if(!chan) return csreply(nick, GetReply(nick, L_NOSUCHCHAN), c);

		if(!(newu = getuserinfo(parv[3])))
			return csreply(nick, GetReply(nick, L_NOSUCHUSER), parv[3]);

		if(chan->owner->user == newu)/* le futur owner l'est déjà */
			return csreply(nick, "%s est déjà l'owner de %s.", newu->nick, c);

		if(newlevel) chan->owner->level = newlevel;
		else del_access(chan->owner->user, chan);

		if((newo = GetAccessIbyUserI(newu, chan))) newo->level = OWNERLEVEL, chan->owner = newo;
		else chan->owner = add_access(newu, c, OWNERLEVEL, A_MANAGERFLAGS, OnChanTS(newu, chan));
		DelCWarned(chan);
		csreply(nick, "%s est le nouvel owner de %s.", newu->nick, c);
	}
	else return csreply(nick, GetReply(nick, L_UNKNOWNOPTION), arg);
	return 1;
}

int whoison(aNick *nick, aChan *chan, int parc, char **parv)
{
	unsigned int first = 1, used = 0;
	char buf[450] = {0};
	aLink *lp = chan->netchan->members;

#define ITEM_SIZE (NICKLEN + 16) /* nick + prefix *!@+ */

	if(!lp) return csreply(nick, "Aucun User sur %s.", chan->chan);

	for(; lp; lp = lp->next)
	{
		aJoin *j = lp->value.j;

		if(used + ITEM_SIZE >= sizeof buf)
		{
			if(first) csreply(nick, "Un total de %d Users sur %s:%s...",
							chan->netchan->users, chan->chan, buf);
			else csreply(nick, "              :%s...", buf);
			first = used = 0;
		}
		used += fastfmt(buf + used, " $$", GetChanPrefix(j->nick, j), j->nick->nick);
	}

	if(first && *buf) csreply(nick, "Un total de %d Users sur %s:\2%s",
							chan->netchan->users, chan->chan, buf);
	else if(*buf) csreply(nick, "              :%s", buf);

	return 1;
}
