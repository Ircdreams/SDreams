/* src/timers.c - fonctions de controle appellées periodiquement
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
 * $Id: timers.c,v 1.79 2008/02/09 19:45:49 romexzf Exp $
 */

#include "main.h"
#include "config.h"
#include "outils.h"
#include "hash.h"
#include "del_info.h"
#include "fichiers.h"
#include "cs_cmds.h"
#include "debug.h"
#include "timers.h"
#include "mylog.h"

Timer *Timers = NULL; /* Our head */

void check_accounts(void)
{
	anUser *u, *u2;
	int i = 0;

	for(; i < USERHASHSIZE; ++i) for(u = user_tab[i]; u; u = u2)
	{
		u2 = u->next;
		if(u->lastseen + cf_maxlastseen < CurrentTS && !IsAdmin(u) && !UNopurge(u))
			cswallops("Purge de l'username %s (Lastseen %s)",
				u->nick, duration(CurrentTS - u->lastseen));
		else if(UFirst(u) && (u->lastseen + cf_register_timeout) < CurrentTS)
			cswallops("Purge de l'username %s par register timeout", u->nick);
		else
		{
			if(u->n) u->lastseen = CurrentTS;
			continue; /* user is ok, go on without purging it. */
		}
		del_regnick(u, HF_LOG, "Purge de l'username de l'owner");
	}
}

void check_chans(void)
{
	aChan *c, *next;
	int i = 0;
	char raison[TOPICLEN+1];

	for(; i < CHANHASHSIZE; ++i) for(c = chan_tab[i]; c; c = next)
	{
		next = c->next;

		if(!CSuspend(c) && CJoined(c) && c->netchan->users == 0 /* empty */
		&& !HasMode(c->netchan, C_MKEY|C_MINV) && CurrentTS - c->lastact > cf_chanmaxidle)
			cspart(c, "Idle");

		if(!c->access || !c->owner)
			log_write(LOG_MAIN, 0, "chan_check: Channel %s has no %s -- Why ?!",
				c->chan, c->owner ? "access" : "owner");

		else if(!AOnChan(c->owner) && !UNopurge(c->owner->user) && !IsAdmin(c->owner->user))
		{
			if(c->owner->lastseen + cf_maxlastseen < CurrentTS)
			{
				mysnprintf(raison, sizeof raison, "Pas de visite de l'owner depuis \2%s",
					get_time(NULL, c->owner->lastseen));
				del_chan(c, HF_LOG, raison);
			}
			else if(c->owner->lastseen + cf_maxlastseen - cf_warn_purge_delay < CurrentTS
				&& !CWarned(c))
			{
				c->flag |= C_LOCKTOPIC|C_WARNED;
				mysnprintf(raison, TOPICLEN, GetUReply(c->owner->user, L_CHANNELPURGEWARN),
					get_time(NULL, c->owner->lastseen + cf_maxlastseen));

				if(CJoined(c)) cstopic(c, raison);
				else strcpy(c->deftopic, raison);
			}
		}
	} /* for() */
}

#ifdef TDEBUG
#	define TDEBUGF(x) timer_debug x
void timer_debug(const char *fmt, ...)
{
	va_list vl;

	va_start(vl, fmt);
	log_writev(LOG_RAW, 0, fmt, vl);
	va_end(vl);
}
#else
#	define TDEBUGF(x)
#endif

Timer *timer_add(time_t delay, enum TimerType type, int (*callback) (Timer *),
				void *data1, void *data2)
{
	Timer *timer = calloc(1, sizeof *timer);

	TDEBUGF(("Adding timer %d (delay=%ld) d1=[%s] d2=[%s]", type, delay, data1, data2));

	if(!timer)
	{
		Debug(W_MAX|W_WARN, "add_timer, malloc a échoué pour un timer (%lu type %d)",
			delay, type);
		return NULL;
	}

	timer->delay = delay;
	timer->callback = callback;
	timer->type = type;
	timer->data1 = data1;
	timer->data2 = data2;

	return timer_enqueue(timer);
}

Timer *timer_enqueue(Timer *timer)
{
	Timer *tmp = Timers, *last = NULL;

	switch(timer->type)
	{
		case TIMER_ABSOLU:
			timer->expire = timer->delay;
			break;
		case TIMER_RELATIF:
		case TIMER_PERIODIC: /* reenqueue it */
			timer->expire = CurrentTS + timer->delay;
			break;
	}

	if(timer->expire < CurrentTS)
	{
		Debug(W_DESYNCH|W_WARN, "timer_enqueue, ajout d'un Timer déjà expiré? (%lu < %lu)",
			(unsigned long) timer->expire, (unsigned long) CurrentTS);
		free(timer);
		return NULL;
	}

	for(; tmp && timer->expire >= tmp->expire; last = tmp, tmp = tmp->next);

	timer->last = last;
	timer->next = tmp;
	if(tmp) tmp->last = timer;
	if(last) last->next = timer;
	else Timers = timer;

	TDEBUGF(("Queueing Timer %p, (delay=%ld/%ld exp: %ld) ->last=%p ->next=%p",
		(void *) timer, timer->delay, timer->expire, timer->expire - CurrentTS,
		(void *) timer->last, (void *) timer->next));

	return timer;
}

void timer_free(Timer *timer)
{
	TDEBUGF(("Freeing Timer %p, ->last=%p ->next=%p", (void *) timer,
		(void *) timer->last, (void *) timer->next));
	free(timer);
}

void timer_dequeue(Timer *timer)
{
	TDEBUGF(("Dequeueing Timer %p, ->last=%p ->next=%p", (void *) timer,
		(void *) timer->last, (void *) timer->next));

	if(timer->next) timer->next->last = timer->last;
	if(timer->last) timer->last->next = timer->next;
	else Timers = timer->next;

	timer->next = timer->last = NULL; /* removed from list */
}

void timer_remove(Timer *timer)
{
	timer_dequeue(timer);
	timer_free(timer);
}

void timers_run(void)
{
	Timer *timer = Timers;

	TDEBUGF(("Running Timers ! ! !"));

	for(; (timer = Timers) && timer->expire <= CurrentTS;)
	{
		TDEBUGF(("Running Timer %p, ->last=%p ->next=%p (%ld)", (void *) timer,
			(void *) timer->last, (void *) timer->next, timer->expire));

		timer_dequeue(timer);
		if(timer->callback(timer)) timer_free(timer); /* callback asks for removall */
		else timer_enqueue(timer); /* otherwise it's a periodic timer, re-add it to pending list */
	}
}

void timer_clean(void)
{
	Timer *timer = Timers, *tnext;

	for(; timer; timer = tnext)
	{
		tnext = timer->next;
		free(timer);
	}
	Timers = NULL;
}

int callback_ban(Timer *timer)
{
	aBan *ban = timer->data2;
	aChan *chan = timer->data1;

	ban->timer = NULL; /* prevent ban_del from clearing it too */

	csmode(chan, MODE_OBV, "-b $", ban->mask);
	ban_del(chan, ban);
	return 1;
}

#ifdef USE_NICKSERV
int callback_kill(Timer *timer)
{
	aKill *kill = timer->data1;

	kill->nick->timer = NULL;
	if(TIMER_CHNICK == kill->type)
	{	/* generates a new nick based on current one and appends */
		char buf[NICKLEN + 1];
		do { /* a number of at max 4 digits */
			snprintf(buf, sizeof buf, "%.20s%d", kill->nick->nick, rand() & 4095);
		} while(getnickbynick(buf));

		putserv("%s "TOKEN_SVSNICK" %s :%s", bot.servnum, kill->nick->numeric, buf);
	}
	else
	{
		putserv("%s "TOKEN_KILL" %s :%s (Pseudo enregistré/Registered Nick)",
			cs.num,	kill->nick->numeric, cs.nick);
		del_nickinfo(kill->nick->numeric, "timer kill");
	}
	kill_free(kill);
	return 1;
}
#endif

int callback_check_accounts(Timer *timer)
{
	check_accounts();
	return 0;
}

int callback_check_chans(Timer *timer)
{
	check_chans();
	return 0;
}

int callback_write_dbs(Timer *timer)
{
	db_write_chans();
	db_write_users();
	CurrentTS = time(NULL); /* because db writing can take a lot */
	return 0;
}

int callback_fl_update(Timer *timer)
{
	aChan *c = timer->data1;
	aNChan *netchan = c->netchan;

	if(CurrentTS - bot.lasttime > 30) /* ugly! */
	{
		bot.lasttime = CurrentTS;	/* mise à jour des infos  */
		bot.lastbytes = bot.dataQ;	/* pour les stats traffic */
	}

	if(CFLimit(c) && CJoined(c)) /* floating limit */
	{
		unsigned int newlimit = netchan->users + 1 + c->limit_inc;
		unsigned int dif = newlimit > netchan->modes.limit ?
					newlimit - netchan->modes.limit :
					netchan->modes.limit - newlimit;

		/* only updates the limit if it is worst (dif >= grace) */
		if(netchan->modes.limit != newlimit && dif >= c->limit_min)
		{
			netchan->modes.limit = newlimit;
			if(!HasMode(netchan, C_MLIMIT)) netchan->modes.modes |= C_MLIMIT;
			putserv("%s "TOKEN_MODE" %s +l %u", cs.num, c->chan, newlimit);
		}
	}
	else /* not on chan or FL not active => remove timer */
	{
		c->fltimer = NULL;
		return 1;
	}
	return 0;
}
