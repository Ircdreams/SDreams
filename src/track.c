/* src/track.c - Tracker un user
 * Copyright (C) 2004 ircdreams.org
 *
 * contact: bugs@ircdreams.org
 * site web: http://ircdreams.org
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
 * $Id: track.c,v 1.14 2006/03/16 07:01:03 bugs Exp $
 */

#include "main.h"
#include "timers.h"
#include "track.h"
#include "cs_cmds.h"
#include "hash.h"
#include "outils.h"
#include "debug.h"

struct track *trackhead = NULL;

struct track *istrack(anUser *user)
{
	register struct track *track = trackhead;
	for(;track && track->tracked != user;track = track->next);
	return track;
}

static int callback_track(Timer *timer) 
{ 
        struct track *track = timer->data1; 
        track->timer = NULL; /* timer_run() will delete it */ 
        del_track(track); 
        return 1; 
} 

static int add_track(anUser *nick, aNick *from, time_t expire)
{
	struct track *track = malloc(sizeof *track);	

	if(!track)
		return Debug(W_WARN|W_MAX, "add_track, malloc a échoué pour le track %s (%s)",
			nick->nick, from->nick);
	track->tracked = nick;
	track->tracker = from;
	track->expire = CurrentTS + expire;	
	track->from = CurrentTS;
	track->timer = expire ? timer_add(track->expire, TIMER_ABSOLU, callback_track, track, NULL) : NULL;
	track->last = NULL;
	if(trackhead) trackhead->last = track;
	trackhead = track;
	return 1;
}

void del_track(struct track *track)
{
	if(track->next) track->next->last = track->last;
	if(track->last) track->last->next = track->next;
	else trackhead = track->next;
	free(track);
}

int cmd_track(aNick *nick, aChan *chan, int parc, char **parv)
{
	struct track *track = trackhead;

	if(!strcasecmp(parv[1], "list"))
	{
		unsigned int i = (parc < 2) ? 1 : 0;

		if(!track) return csreply(nick, "Aucun tracké actuellement.");

		csreply(nick, "\2Liste des Traqués:\2");
		for(;track;track = track->next)
			if(i || !match(parv[2],track->tracked->nick))
				csreply(nick, "> \2%s\2 (Pour \2%s\2, depuis %s, expire %s)",
				track->tracked->nick, track->tracker->nick, duration(CurrentTS - track->from),
				track->expire ? get_time(nick, track->expire) : "Jamais");
	}

	else if(!strcasecmp(parv[1], "add"))
	{
		anUser *user = NULL;
		time_t timeout = 0;

		if(parc < 2) return csreply(nick, "Syntaxe: %s ADD <Pseudo|%%UserName> [expiration]", parv[0]);		

		if(!(user = ParseNickOrUser(nick, parv[2]))) return 0;

		if((track = istrack(user)))
			return csreply(nick, "\2%s\2 est déjà tracké par %s (depuis %s)",
				track->tracked->nick, track->tracker->nick, duration(CurrentTS - track->from));

		if(nick->user == user) return csreply(nick, "Pourquoi se traquer soi-même!?");

		if(IsAdmin(user) && nick->user->level < MAXADMLVL)
			return csreply(nick, "Veuillez ne pas tracker un Admin.");

		if(parc > 2 && *parv[3] == '%' && (timeout = convert_duration(++parv[3])) < 0)
			return csreply(nick, GetReply(nick, L_INCORRECTDURATION));

		add_track(user, nick, timeout);

		csreply(nick, "\2%s\2 a été ajouté a vos trackés. (Expire dans %s)", user->nick,
			timeout ? duration(timeout) : "Jamais"); 
                if(user->n) csreply(nick, "%s est connecté via %s", user->nick, GetNUHbyNick(user->n, 0)); 

	}

	else if(!strcasecmp(parv[1], "del"))
	{
		anUser *user = NULL;

		if(parc < 2) return csreply(nick, "Syntaxe: %s DEL <Pseudo|%%UserName>", parv[0]);

		if(!(user = getuserinfo(parv[2])))
			return csreply(nick, GetReply(nick, L_NOSUCHUSER), parv[2]);

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
