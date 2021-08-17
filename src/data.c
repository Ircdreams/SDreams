/* src/data.c Create, Update, Delete or Load aData items
 *
 * Copyright (C) 2002-2006 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@ir3.org>
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
 * $Id: data.c,v 1.4 2006/09/19 17:29:34 romexzf Exp $
 */

#include "main.h"
#include "outils.h"
#include "timers.h"
#include "debug.h"
#include "cs_cmds.h"

static aData *data_create(const char *from, int flag, void *data, time_t debut)
{
	aData *d = malloc(sizeof *d);

	if(!d)
	{
		Debug(W_WARN|W_MAX, "data::create: OOM from=%", from);
		return NULL;
	}

	Strncpy(d->from, from, NICKLEN);
	d->debut = debut;
	d->flag = flag;
	d->data = data;
	d->timer = NULL;
	*d->raison = 0;
	d->expire = 0;

	return d;
}

void data_free(aData *data)
{
	if(data->timer) timer_remove(data->timer);
	free(data);
}

static void data_exec_end(aData *data)
{
	aData **dpp = NULL;

	switch(data->flag & DATA_TYPES)
	{
		anUser *u = NULL;
		aChan *c = NULL;

		case DATA_T_SUSPEND_CHAN:
			c = data->data;
			DelCSuspend(c);
			if(!CJoined(c)) csjoin(c, 0);
			dpp = &c->suspend;
			break;

		case DATA_T_SUSPEND_USER:
			u = data->data;
			DelUSuspend(u);
			dpp = &u->suspend;
			break;

		case DATA_T_NOPURGE:
			u = data->data;
			DelUNopurge(u);
			dpp = &u->nopurge;
			break;

		case DATA_T_CANTREGCHAN:
			u = data->data;
			DelUCantRegChan(u);
			dpp = &u->cantregchan;
			break;

		default: /* Who fucked the database ? */
			Debug(W_WARN, "data::exec::end: aData=%p, data=%p, type=%d not found!",
				(void *) data, data->data, data->flag);
	}

	/* Clean Up, if needed */
	if(DNeedFree(data))
	{
		data_free(data);
		*dpp = NULL;
	}
	else if(data->timer) /* Keep suspend tracked */
	{
		timer_remove(data->timer);
		data->timer = NULL;
	}
}

static int data_callback(Timer *timer)
{
	aData *d = timer->data1;

	d->timer = NULL; /* otherwise timer_run would doublefree it */
	data_exec_end(d);
	return 1;
}

/* res: error=-1, deleted=0, created=1, updated=2 */
int data_handle(aData *d, const char *user, const char *str, time_t timeout,
	int flag, void *data)
{
	time_t old_expire = 0;
	int res = 1;

	if(!d)
	{
		if((!str || !*str) && flag & DATA_T_NEEDRAISON) return -1;

		d = data_create(user, flag, data, CurrentTS);
		/* Created, now add the correct flags and link back */
		switch(d->flag & DATA_TYPES)
		{
			anUser *u = NULL;
			aChan *c = NULL;

			case DATA_T_SUSPEND_CHAN:
				c = d->data;
				SetCSuspend(c);
				c->suspend = d;
				break;

			case DATA_T_SUSPEND_USER:
				u = d->data;
				SetUSuspend(u);
				u->suspend = d;
				break;

			case DATA_T_NOPURGE:
				u = d->data;
				SetUNopurge(u);
				u->nopurge = d;
				break;

			case DATA_T_CANTREGCHAN:
				u = d->data;
				SetUCantRegChan(u);
				u->cantregchan = d;
				break;

			default: /* Who fucked the database ? */
				Debug(W_WARN, "data::create: aData=%p, data=%p, type=%d not found!",
					(void *) d, d->data, d->flag);
		}
	}
	else /* Update or delete */
	{
		if(str) /* handle special case like old toggle and nolog for suspend */
		{
			if(!strcmp(str, "off")) str = NULL;
			else if(!strcmp(str, "nolog")) str = NULL, d->flag |= DATA_FREE_ON_DEL;
		}
		if(!timeout && (!str || !*str)) /* Deleting */
		{
			data_exec_end(d);
			return 0;
		}
		old_expire = d->expire; /* Updating */
		++res;
	}
	/* Update raison/timeout or set the (creating) */
	if(str && *str) Strncpy(d->raison, str, RAISONLEN);
	if(timeout) d->expire = CurrentTS + timeout;

	/* Now do timer's stuff */
	if(old_expire != d->expire) /* Time-limited item */
	{
		if(!d->timer) /* changing not limited to limited or setting a limited */
			d->timer = timer_add(d->expire, TIMER_ABSOLU, data_callback, d, NULL);
		else /* only updating: dequeue old timer, then change expiration time */
		{
			timer_dequeue(d->timer);
			d->timer->delay = d->expire;
			timer_enqueue(d->timer);
		}
	}
	return res;
}

int data_load(aData **dpp, const char *from, const char *raison, int flag, time_t expire,
		time_t debut, void *data)
{
	aData *d;
	int active = 0;

	if(!(*dpp = data_create(from, flag, data, debut)))
		return Debug(W_WARN|W_MAX, "data::load: OOM from=%s, raison=%s", from, raison);

	d = *dpp;
	if(raison) Strncpy(d->raison, raison, RAISONLEN);
	if(expire) d->expire = expire + CurrentTS;

	if(expire > 0)
		switch(d->flag & DATA_TYPES)
		{
			case DATA_T_SUSPEND_CHAN:
				if(CSuspend((aChan *) d->data)) active = 1;
				break;

			case DATA_T_SUSPEND_USER:
				if(USuspend((anUser *) d->data)) active = 1;
				break;

			case DATA_T_NOPURGE:
				if(UNopurge((anUser *) d->data)) active = 1;
				break;

			case DATA_T_CANTREGCHAN:
				if(UCantRegChan((anUser *) d->data)) active = 1;
				break;

			default: /* Who fucked the database ? */
				Debug(W_WARN, "data::load: aData=%p, data=%p, type=%d not found!",
					(void *) d, d->data, d->flag);
		}

	d->timer = active && expire ? timer_add(d->expire, TIMER_ABSOLU,
										data_callback, d, NULL) : NULL;

	return 0;
}
