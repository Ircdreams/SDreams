/* src/del_info.c - Suppression d'informations en mémoire
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
 * $Id: del_info.c,v 1.73 2007/12/16 20:48:15 romexzf Exp $
 */

#include "main.h"
#include "outils.h"
#include "del_info.h"
#include "hash.h"
#include "timers.h"
#include "debug.h"

void del_access(anUser *user, aChan *chan)
{
	anAccess **acces = &user->accesshead, *tmp;
	aLink *lp, **lpp;

	for(; (tmp = *acces); acces = &tmp->next)
		if(tmp->c == chan)
		{
			*acces = tmp->next;
			break;
		}

	for(lpp = &chan->access; (lp = *lpp); lpp = &lp->next)
		if(lp->value.a == tmp)
		{
			*lpp = lp->next;

			if(tmp->info) free(tmp->info);
			free(tmp);
			free(lp); /* ne pas oublier de free() le link, -memory leak-*/
			break;
		}
}

#ifdef USE_NICKSERV
int kill_remove(aNick *nick)
{
	if(!nick->timer)
		return Debug(W_DESYNCH|W_WARN, "kill_remove: Timer NULL pour %s", nick->nick);
	timer_dequeue(nick->timer); 	/* remove timer from list */
	kill_free(nick->timer->data1); 	/* remove and free kill */
	timer_free(nick->timer); 		/* finally free timer's data */
	nick->timer = NULL;
	return 0;
}

void kill_free(aKill *kill) /* remove from killlist then free */
{
	if(kill->next) kill->next->last = kill->last;
	if(kill->last) kill->last->next = kill->next;
	else killhead = kill->next;
	free(kill);
}
#endif

void del_link(aNChan *chan, aLink *link)
{
	if(link->next) link->next->last = link->last;
	if(link->last) link->last->next = link->next;
	else chan->members = link->next;
	free(link);

	if(0 == --chan->users)  /* channel gets empty */
	{	/* check if Z is still in here */
		if(chan->regchan && CJoined(chan->regchan)) chan->regchan->lastact = CurrentTS;
		else del_nchan(chan); /* otherwise destruct it */
	}
}

void del_join(aNick *nick, aNChan *chan)
{
	aJoin **joinp = &nick->joinhead, *join;

	for(; (join = *joinp); joinp = &join->next)
		if(chan == join->chan)
		{
			*joinp = join->next;

			if(nick->user && chan->regchan) /* update le lastseen comme il faut */
			{
				anAccess *acces = GetAccessIbyUserI(nick->user, chan->regchan);
				if(acces) acces->lastseen = CurrentTS;
			}
			del_link(chan, join->link); /* on del le link */
			free(join);
			break;
		}
}

void del_alljoin(aNick *nick)
{
	aJoin *join = nick->joinhead, *join_t = NULL;
	anAccess *a;

	for(; join; join = join_t)
	{
		join_t = join->next;
		if(join->chan->regchan && nick->user /* channel is registered, so is nick */
			&& (a = GetAccessIbyUserI(nick->user, join->chan->regchan))) /* got an access */
				a->lastseen = CurrentTS; /* update lastseen on channel */
		del_link(join->chan, join->link); /* clean link */
		free(join);
	}
	nick->joinhead = NULL;
}

void ban_del(aChan *chan, aBan *ban)
{
	ban_remove(chan, ban);
	ban_free(ban);
}

void ban_remove(aChan *chan, aBan *ban)
{
	if(ban->next) ban->next->last = ban->last;
	if(ban->last) ban->last->next = ban->next; /* ban was the head */
	else chan->banhead = ban->next;
}

void ban_free(aBan *ban)
{
	/* It was a temporary ban, I need to clean up timer's data */
	/* Note that if it has been invoked by callback, timer == NULL */
	if(ban->timer) timer_remove(ban->timer);
	free(ban->raison);
	free(ban->mask);
	free(ban);
}
