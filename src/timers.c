/* src/timers.c - fonctions de controle appellées periodiquement
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
 * $Id: timers.c,v 1.52 2006/03/01 19:28:19 bugs Exp $
 */

#include "main.h"
#include "config.h"
#include "outils.h"
#include "hash.h"
#include "del_info.h"
#include "divers.h"
#include "fichiers.h"
#include "cs_cmds.h"
#include "debug.h"
#include "template.h"
#include "timers.h"
#include <errno.h>

void check_accounts(void)
{
	anUser *u, *u2;
	int i = 0;
	char memo[MEMOLEN +1];

	for(;i < USERHASHSIZE;++i) for(u = user_tab[i];u;u = u2)
	{
		u2 = u->next;
		if(u->lastseen + cf_maxlastseen < CurrentTS && !IsAdmin(u) && !UNopurge(u))
		{
			cswallops("Purge de l'username %s (Lastseen %s)",
				u->nick, duration(CurrentTS - u->lastseen));
			snprintf(memo, MEMOLEN, "Suite a votre inactivitée, votre username a été effacé (Dernier login: %s)", duration(CurrentTS - u->lastseen));
			tmpl_mailsend(&tmpl_mail_memo, u->mail, u->nick, NULL, NULL, cs.nick, memo);
		}
		else if(UFirst(u) && (u->lastseen + cf_register_timeout) < CurrentTS)
			cswallops("Purge de l'username %s par register timeout", u->nick);
		else
		{
			if(u->n) u->lastseen = CurrentTS;
			if(u->cantregchan > 0 && u->cantregchan < CurrentTS) u->cantregchan = -1;
			continue; /* user is ok, go on without purging it. */
		}
		del_regnick(u, 0, "Purge de l'username de l'owner");
	}
}

void check_chans(void)
{
	aChan *c, *next;
	int i = 0;
	char raison[300], memo[MEMOLEN + 1];

	for(;i < CHANHASHSIZE;++i) for(c = chan_tab[i];c;c = next)
	{
		next = c->next;

		if(!c->access || !c->owner)
			Debug(W_WARN, "chan_check: Channel %s has no %s -- Why ?!",
				c->chan, c->owner ? "access" : "owner");
		else if(!AOnChan(c->owner) && !UNopurge(c->owner->user))
		{
			if(c->owner->lastseen + cf_maxlastseen < CurrentTS)
			{
				snprintf(raison, sizeof raison, "Pas de visite de l'owner depuis \2%s",
					get_time(NULL, c->owner->lastseen));
				snprintf(memo, MEMOLEN, "Votre salon %s a été effacé. Pas de visite depuis le %s",
					c->chan, get_time(NULL, c->owner->lastseen));
				tmpl_mailsend(&tmpl_mail_memo, c->owner->user->mail, c->owner->user->nick, NULL, NULL, cs.nick, memo);
				del_chan(c, 0, raison);
			}
			else if(c->owner->lastseen + cf_maxlastseen - cf_warn_purge_delay < CurrentTS 
                                   && !CWarned(c)) 
                        { 
                                c->flag |= C_LOCKTOPIC|C_WARNED;
				snprintf(raison, TOPICLEN, "Le salon sera purgé le %s pour absence de "
					"l'owner, contactez un administrateur",
					get_time(NULL, c->owner->lastseen + cf_maxlastseen));
				snprintf(memo, MEMOLEN, "Votre salon %s semble inactif et sera effacé le %s. Pour annuler cette procédure: identifiez-vous et rejoiniez votre salon !",
					c->chan, get_time(NULL, c->owner->lastseen + cf_maxlastseen));
				tmpl_mailsend(&tmpl_mail_memo, c->owner->user->mail, c->owner->user->nick, NULL, NULL, cs.nick, memo);  
                                if(CJoined(c)) cstopic(c, raison); 
                                else strcpy(c->deftopic, raison); 
                        }
                }
		else if(!IsSuspend(c) && CJoined(c) && c->netchan->users == 0 /* empty */
		&& !HasMode(c->netchan, C_MKEY|C_MINV) && CurrentTS - c->lastact > cf_chanmaxidle)
			cspart(c, "Inactif");
	} /* for() */
}

#ifdef TDEBUG
#	define TDEBUGF(x) timer_debug x
static int timer_debug(const char * fmt, ...)
{
	FILE *fd = fopen("logs/timers.log", "a");
	va_list vl;

	if(!fd) return Debug(W_WARN, "Error while opening timers log file : %s", strerror(errno));

	va_start(vl, fmt);
	fputs(get_time(NULL, CurrentTS), fd); /* TimeStamp */
	fputc(' ', fd);
	vfprintf(fd, fmt, vl);
	fputc('\n', fd);
	fclose(fd);

	va_end(vl);
	return 0;
}
#else
#	define TDEBUGF(x)
#endif

Timer *timer_add(time_t delay, enum TimerType type, int (*callback) (Timer *),
				void *data1, void *data2)
{
	Timer *timer = calloc(1, sizeof *timer);
#ifdef TDEBUG
        static int timer_id = 0;
	timer->id = timer_id++;
#endif

	TDEBUGF(("Adding timer #%d %d (delay=%ld) d1=[%s] d2=[%s]", timer->id, type, delay, data1, data2));

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

	for(;tmp && timer->expire >= tmp->expire; last = tmp, tmp = tmp->next);

	timer->last = last;
	timer->next = tmp;
	if(tmp) tmp->last = timer;
	if(last) last->next = timer;
	else Timers = timer;

	TDEBUGF(("Queueing Timer %p (#%d), (delay=%ld/%ld exp: %ld) ->last=%p ->next=%p",
		(void *) timer, timer->id, timer->delay, timer->expire, timer->expire - CurrentTS,
		(void *) timer->last, (void *) timer->next));

	return timer;
}

void timer_free(Timer *timer)
{
	TDEBUGF(("Freeing Timer %p (#%d), ->last=%p ->next=%p", (void *) timer, timer->id,
		(void *) timer->last, (void *) timer->next));
	free(timer);
}

void timer_dequeue(Timer *timer)
{
	TDEBUGF(("Dequeueing Timer %p (#%d), ->last=%p ->next=%p", (void *) timer, timer->id,
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
	Timer *timer = Timers, *tmp;

/*	TDEBUGF(("Running Timers ! ! !")); */

	for(;timer && timer->expire <= CurrentTS;timer = tmp)
	{
		tmp = timer->next;

		TDEBUGF(("Running Timer %p (#%d), ->last=%p ->next=%p (%ld)", (void *) timer,
			timer->id, (void *) timer->last, (void *) timer->next, timer->expire));

		timer_dequeue(timer);
		if(timer->callback(timer)) timer_free(timer);
	}
}

int callback_ban(Timer *timer)
{
	aBan *ban = timer->data2;
	aChan *chan = timer->data1;

	ban->timer = NULL; /* prevent del_ban from clearing it too */

	csmode(chan, MODE_OBVH, "-b $", ban->mask);
	del_ban(chan, ban);
	return 1;
}

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
		putserv("%s "TOKEN_KILL" %s :%s (Pseudo enregistré)",
			cs.num,	kill->nick->numeric, cs.nick);
		del_nickinfo(kill->nick->numeric, "timer kill");
	}
	kill_free(kill);
	return 1;
}

int callback_check_accounts(Timer *timer)
{
	check_accounts();
	timer_enqueue(timer);
	return 0;
}

int callback_check_chans(Timer *timer)
{
	check_chans();
	timer_enqueue(timer);
	return 0;
}

int callback_write_dbs(Timer *timer)
{
	db_write_chans();
	db_write_users();
	write_maxuser();
	CurrentTS = time(NULL); /* because db writing can take a lot */
	timer_enqueue(timer);
	return 0;
}

int callback_unsuspend(Timer *timer)
{
	struct suspendinfo **suspend = timer->data1;

	(*suspend)->expire = -1;
	(*suspend)->timer = NULL; /* clean up ! */
	if((*suspend)->data)
	{
		aChan *chan = (*suspend)->data;
		if(!CJoined(chan)) csjoin(chan, 0);
	}
	return 1;
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
		int newlimit = netchan->users + 1 + c->limit_inc, dif = newlimit - netchan->modes.limit;
		if(dif < 0) dif = -dif;
		/* only updates the limit if it is worst (dif >= grace) */
		if(netchan->modes.limit != newlimit && dif >= c->limit_min)
		{
			netchan->modes.limit = newlimit;
			if(!HasMode(netchan, C_MLIMIT)) netchan->modes.modes |= C_MLIMIT;
			putserv("%s "TOKEN_MODE" %s +l %d", cs.num, c->chan, newlimit);
		}
	}
	else /* not on chan or FL not active => remove timer */
	{
		c->timer = NULL;
		return 1;
	}
	timer_enqueue(timer);
	return 0;
}
