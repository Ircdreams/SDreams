/* src/admin_chan.c - commandes admin pour gerer les salons
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
 * $Id: admin_chan.c,v 1.49 2006/03/15 17:36:47 bugs Exp $
 */

#include "main.h"
#include "outils.h"
#include "cs_cmds.h"
#include "fichiers.h"
#include "admin_chan.h"
#include "add_info.h"
#include "del_info.h"
#include "hash.h"
#include "cs_register.h"

#define MAXMATCHES 40

int admin_chan(aNick *nick, aChan *chan, int parc, char **parv)
{
	char *c = NULL;
	const char *arg = parv[1];
	int i = 0, m = 0;

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
		switch(handle_suspend(&chan->suspend, nick->user->nick, ptr, timeout))
		{
			case -1:
                                return csreply(nick, "Veuillez préciser une raison pour suspendre ce salon."); 

			case 1:
				chan->suspend->data = chan;
				if(CJoined(chan)) cspart(chan, chan->suspend->raison);
				putlog(LOG_CHANS, "SUSPEND ON %s par %s@%s (%s)", c, nick->nick,
					nick->user->nick, chan->suspend->raison);
		}
		/* report.. */ 
                if(IsSuspend(chan)) show_suspend(nick, chan); 
		else if(!CJoined(chan))
		{
			csreply(nick, "Le salon \2%s\2 n'est plus suspendu.", c);
			csjoin(chan, 0);
			putlog(LOG_CHANS, "SUSPEND OFF %s par %s@%s", c, nick->nick, nick->user->nick);
		}
	}
	else if(!strcasecmp(arg, "del"))
	{
		char tmp[300];
		if(parc < 2) return csreply(nick, "Syntaxe: %s DEL <salon> <raison>", parv[0]);

		if(!chan) return csreply(nick, GetReply(nick, L_NOSUCHCHAN), c);
		
		snprintf(tmp, sizeof tmp, "Delchan par %s@%s (%s)", nick->nick,
			nick->user->nick, (parc > 2) ? parv2msg(parc, parv, 3, 200) : "Aucune raison");

		csreply(nick, "Le salon %s a bien été supprimé.", c);
		del_chan(chan, 0, tmp);
	}
	else if(!strcasecmp(arg, "reg"))
	{
		anUser *u;
		if(parc < 3)
			return csreply(nick, "Syntaxe: %s REG <salon> <Pseudo|%%Username> [thème]", parv[0]);

		if(chan || !strcasecmp(c, bot.pchan)) return csreply(nick, GetReply(nick, L_ALREADYREG), c);

		if(!chan_check(c, nick)) return 0;

		if(!(u = ParseNickOrUser(nick, parv[3]))) return 0;

		chan = add_chan(c, (parc > 3) ? parv2msg(parc, parv, 4, 300) : "Adminreg");
		add_access(u, c, OWNERLEVEL, A_OP | A_PROTECT, CurrentTS);
		csjoin(chan, JOIN_FORCE);
		csreply(nick, "%s a été enregistré pour %s.", c, u->nick);
	}
	else if(!strcasecmp(arg, "list"))
	{
		int theme = getoption("-theme", parv, parc, 2, 0);

		int modes = getoption("-modes", parv, parc, 2, 0);

		int users = getoption("-users", parv, parc, 2, 1);
		int defmodes = getoption("-defmodes", parv, parc, 2, 0);

		int url = getoption("-url", parv, parc, 2, 0);

		int max = getoption("-count", parv, parc, 2, 1);
		int warned = getoption("-warned", parv, parc, 2, -1);
		int topic = getoption("-topic", parv, parc, 2, 0), j = 0;

		if(parc < 2 || *parv[2] == '-') m = 1;
		if(modes) modes = cmodetoflag(0, parv[modes]);
		if(defmodes) defmodes = cmodetoflag(0, parv[defmodes]);
		if(!max) max = strcmp("-all", parv[parc]) ? MAXMATCHES : -1;

		for(;j < CHANHASHSIZE;j++) for(chan = chan_tab[j];chan;chan = chan->next)
				if((m || !match(parv[2], chan->chan))
				&& (!theme || !match(parv[theme], chan->description))
				&& (!modes || (chan->netchan && HasMode(chan->netchan, modes) == modes))
				&& (!defmodes || HasDMode(chan, defmodes) == defmodes)
				&& (!url || !match(parv[url], chan->url))
				&& (!topic || (chan->netchan && !match(parv[topic], chan->netchan->topic)))
				&& (!warned || CWarned(chan))
				&& (!users || (chan->netchan && chan->netchan->users >= users)) && (++i <= max || max < 0))
				{
					if(i == 1) csreply(nick, "\2Liste des salons enregistrés :");
					if(CJoined(chan)) csreply(nick, "  %s (%s) [%d]", chan->chan,
						GetCModes(chan->netchan->modes), chan->netchan->users);
					else csreply(nick, "  %s (%s) [\2%s\2]", chan->chan, GetCModes(chan->defmodes),
						 IsSuspend(chan) ? "Suspendu" : "Pas sur le salon");
				}

		if(i > MAXMATCHES && max == MAXMATCHES)
			csreply(nick, GetReply(nick, L_EXCESSMATCHES), i, MAXMATCHES);
		if(max > 0 && max < i) 
                        csreply(nick, "Un Total de %d entrées trouvées (%d listées)", i, max); 
                else if(i) csreply(nick, "Un Total de %d entrées trouvées", i); 
                else csreply(nick, "Aucun salon enregistré correspondant trouvé."); 
	}
	else if(!strcasecmp(arg, "suspendlist"))
	{
		int from = getoption("-from", parv, parc, 2, 0), j = 0;
		if(parc < 2 || *parv[2] == '-') m = 1;

		for(;j < CHANHASHSIZE;j++) for(chan = chan_tab[j];chan;chan = chan->next)
			if(IsSuspend(chan) && (m || !match(parv[2], chan->chan))
					&& (!from || !strcasecmp(parv[from], chan->suspend->from)))
			{
				if(++i == 1) csreply(nick, "\2Liste des salons suspendus :");
				csreply(nick, "  %s Par %s Expire %s Raison: %s",
						chan->chan, chan->suspend->from,
						chan->suspend->expire ? get_time(nick, chan->suspend->expire) : "Jamais",
						chan->suspend->raison);
			}

		csreply(nick, "Il y a un total de\2 %d\2 salons suspendus correspondant à vos critères.", i);
	}
	else if(!strcasecmp(arg, "join"))
	{
		if(parc < 2) return csreply(nick, "Syntaxe: %s JOIN <salon> [-force]", parv[0]);

		if(!chan) return csreply(nick, GetReply(nick, L_NOSUCHCHAN), c);

		if(IsSuspend(chan))
			return csreply(nick, "%s est actuellement suspendu pour %s, utilisez '%s SUSPEND %s' pour lever le suspend.",
				c, chan->suspend->raison, parv[0], c);

		if(CJoined(chan) && !getoption("-force", parv, parc, 3, -1))
			return csreply(nick, "Je suis déjà sur %s (En cas de desynch, utilisez l'argument '-force')", c);

		csjoin(chan, JOIN_FORCE);
	}
	else if(!strcasecmp(arg, "part"))
	{
		if(parc < 2) return csreply(nick, "Syntaxe: %s PART <salon> [raison]", parv[0]);

		if(!chan) return csreply(nick, GetReply(nick, L_NOSUCHCHAN), c);

		if(CJoined(chan)) cspart(chan, (parc > 2) ? parv2msg(parc, parv, 3, 300) : "Aucune Raison");
	}
	else if(!strcasecmp(arg, "setowner"))
	{/* chan setowner # gna [level]*/
		anUser *newu;
		anAccess *newo;
		int newlevel = 0;

		if(parc < 3) return csreply(nick, "Syntaxe: %s SETOWNER <chan> <nouvel owner> [nouveau level de l'ancien owner]", parv[0]);
		if(parc > 3 && ((newlevel = strtol(parv[4], NULL, 10)) <= 0 || newlevel >= OWNERLEVEL))
			return csreply(nick, GetReply(nick, L_VALIDLEVEL));

		if(!chan) return csreply(nick, "\2%s\2 n'est pas un salon enregistré.", c);
		if(!(newu = getuserinfo(parv[3]))) return csreply(nick, GetReply(nick, L_NOSUCHUSER), parv[3]);

		if(chan->owner) { /* on check si il y a deja un owner */
			if(chan->owner->user == newu)/* le futur owner l'est déjà */
				return csreply(nick, "%s est déjà l'owner de %s.", newu->nick, c);
			else if(newlevel) chan->owner->level = newlevel;
			else del_access(chan->owner->user, chan);
		}
		if((newo = GetAccessIbyUserI(newu, chan))) newo->level = OWNERLEVEL, chan->owner = newo;
		else chan->owner = add_access(newu, c, OWNERLEVEL, (A_OP | A_PROTECT), CurrentTS);
		DelCWarned(chan);
		csreply(nick, "%s est le nouvel owner de %s.", newu->nick, c);
	}
	else return csreply(nick, GetReply(nick, L_UNKNOWNOPTION), arg);
	return 1;
}

int whoison(aNick *nick, aChan *chan, int parc, char **parv)
{
	int first = 1, size = 0, hide = 0, p = 0;
	char buf[401] = {0}, tmp[NICKLEN + 16];
	aLink *lp = chan->netchan->members;

	if(!lp) return csreply(nick, "Aucun User sur %s.", chan->chan);
	for(;lp;lp = lp->next)
	{
		int tmps = size;
		if(IsHiding(lp->value.j->nick) && !(IsAdmin(nick->user) || IsOper(nick)))
		{
			++hide;
			continue;
		}
		p = fastfmt(tmp, " $$", GetChanPrefix(lp->value.j->nick, lp->value.j), lp->value.j->nick->nick);
		size += p;
		if(size >= 400)
		{
			if(first) csreply(nick, "Un total de %d User%s sur %s:%s...", chan->netchan->users - hide, PLUR(chan->netchan->users), chan->chan, buf);
			else csreply(nick, "              :%s...", buf);
			first = 0;
			strcpy(buf, tmp);
			size = p;
		}
		else strcpy(&buf[tmps], tmp);
	}
	if(first && *buf) csreply(nick, "Un total de %d User%s sur %s:%s", chan->netchan->users - hide , PLUR(chan->netchan->users), chan->chan, buf);
	else if(*buf) csreply(nick, "              :%s", buf);
	else csreply(nick, "Aucun User sur %s.", chan->chan);

	return 1;
}

void show_suspend(aNick *nick, aChan *chan)
{
	if(!chan->suspend) return; /* on aurait jamais du l'envoyer là*/
	if(IsSuspend(chan))
	{
		char buf[TIMELEN + 1];
		Strncpy(buf, get_time(nick, chan->suspend->debut), sizeof buf -1);
		csreply(nick, "Le salon %s \2est\2 suspendu par %s (le %s). Expire %s",
			chan->chan, chan->suspend->from, buf,
			chan->suspend->expire ? get_time(nick, chan->suspend->expire) : "Jamais");
	}
	else csreply(nick, "Le salon était suspend par %s", chan->suspend->from);
	csreply(nick, "Raison: %s", chan->suspend->raison);
}
