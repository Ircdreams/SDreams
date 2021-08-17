/* src/track.c - Tracker un user
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
 * $Id: track.c,v 1.26 2008/01/05 18:34:14 romexzf Exp $
 */

#include "main.h"
#ifdef HAVE_TRACK
#include "timers.h"
#include "cs_cmds.h"
#include "hash.h"
#include "outils.h"
#include "debug.h"
#include "mylog.h"
#include "track.h"
#include <stdarg.h>

static struct track *trackhead = NULL;

struct track *istrack(anUser *user)
{
	register struct track *track = trackhead;
	for(; track && track->tracked != user; track = track->next);
	return track;
}

static int callback_track(Timer *timer)
{
	struct track *track = timer->data1;
	track->timer = NULL; /* timer_run() will delete it because of return's value */
	del_track(track);
	return 1;
}

static int add_track(anUser *user, aNick *from, time_t expire)
{
	struct track *track = malloc(sizeof *track);

	if(!track)
		return Debug(W_WARN|W_MAX, "add_track, malloc a échoué pour le track %s (%s)",
					user->nick, from->nick);
	/* data */
	track->tracked = user;
	track->tracker = from;
	track->expire = CurrentTS + expire;
	track->from = CurrentTS;
	track->timer = expire ?
		timer_add(track->expire, TIMER_ABSOLU, callback_track, track, NULL) : NULL;
	/* linking */
	track->last = NULL;
	track->next = trackhead;
	if(trackhead) trackhead->last = track;
	trackhead = track;
	/* mark */
	SetUTracked(user);
	return 1;
}

void del_track(struct track *track)
{
	DelUTracked(track->tracked);
	/* remove from dlinked list */
	if(track->next) track->next->last = track->last;
	if(track->last) track->last->next = track->next;
	else trackhead = track->next;
	/* free up things */
	if(track->timer) timer_remove(track->timer);
	free(track);
}

void track_admin_quit(aNick *nick)
{
	struct track *track = trackhead, *tt = NULL;

	for(; track; track = tt)
	{
		tt = track->next;
		if(nick == track->tracker) del_track(track);
	}
}

void track_notify(anUser *user, const char *fmt, ...)
{
	struct track *track = istrack(user);
	va_list vl;
	char buf[400];

	va_start(vl, fmt);
	myvsnprintf(buf, sizeof buf, fmt, vl);
	va_end(vl);

	if(track) csreply(track->tracker, "[\2Track\2] %s", buf);
	else log_write(LOG_MAIN, LOG_DOWALLOPS,
			"track: %s marked as tracked but not found [Context: %s]", user->nick, buf);
}

int cmd_track(aNick *nick, aChan *chan, int parc, char **parv)
{
	struct track *track = trackhead;

	if(!strcasecmp(parv[1], "list"))
	{
		unsigned int i = (parc < 2) ? 1 : 0;

		if(!track) return csreply(nick, "Aucun tracké actuellement.");

		csreply(nick, "\2Liste des Traqués:\2");
		for(; track; track = track->next)
			if(i || !match(parv[2], track->tracked->nick))
				csreply(nick, "> \2%s\2 (Pour \2%s\2, depuis %s, expire %s)",
					track->tracked->nick, track->tracker->nick,
					duration(CurrentTS - track->from),
					track->expire ? get_time(nick, track->expire) : "Jamais");
	}

	else if(!strcasecmp(parv[1], "add"))
	{
		anUser *user = NULL;
		time_t timeout = 0;

		if(parc < 2)
			return csreply(nick, "Syntaxe: %s ADD <username|%%nick> [expiration delay]", parv[0]);

		if(!(user = ParseNickOrUser(nick, parv[2]))) return 0;

		if((track = istrack(user)))
			return csreply(nick, "\2%s\2 est déjà tracké par %s (depuis %s)",
				user->nick, track->tracker->nick, duration(CurrentTS - track->from));

		if(nick->user == user) return csreply(nick, "Pourquoi se traquer soi-même!?");

		if(IsAdmin(user) && nick->user->level < MAXADMLVL)
			return csreply(nick, "Veuillez ne pas tracker un Admin.");

		if(parc > 2 && *parv[3] == '%' && (timeout = convert_duration(++parv[3])) < 0)
			return csreply(nick, GetReply(nick, L_INCORRECTDURATION));

		add_track(user, nick, timeout);

		csreply(nick, "\2%s\2 a été ajouté à vos trackés. (Expire dans %s)",
			user->nick,	timeout ? duration(timeout) : "Jamais");

		if(user->n) csreply(nick, "%s est connecté via %s", user->nick, GetNUHbyNick(user->n, 0));
	}

	else if(!strcasecmp(parv[1], "del"))
	{
		anUser *user = NULL;

		if(parc < 2) return csreply(nick, "Syntaxe: %s DEL <username>", parv[0]);

		if(!(user = getuserinfo(parv[2])))
			return csreply(nick,  GetReply(nick, L_NOSUCHUSER), parv[2]);

		if(!(track = istrack(user)))
			return csreply(nick, "\2%s\2 n'est pas tracké.", parv[2]);

		if(track->tracker != nick)
			return csreply(nick, "\2%s\2 est actuellement tracké par %s.",
						track->tracked->nick, track->tracker->nick);

		del_track(track);
		csreply(nick, "%s a été supprimé de vos trackés.", parv[2]);
	}
	else return csreply(nick, GetReply(nick, L_UNKNOWNOPTION), parv[1]);
	return 1;
}
#endif
