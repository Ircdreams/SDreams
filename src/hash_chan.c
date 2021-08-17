/* src/hash_chan.c - gestion des hash chan
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
 * $Id: hash_chan.c,v 1.33 2008/01/04 13:21:34 romexzf Exp $
 */

#include "main.h"
#include "hash.h"
#include "outils.h"
#include "mylog.h"
#include "debug.h"
#include "del_info.h"
#include "cs_cmds.h"
#include "fichiers.h"
#include "timers.h"
#include "config.h"
#include "data.h"
#include <ctype.h>

static inline unsigned int do_hashc(const char *chan)
{
	unsigned int checksum = 0;
	while(*chan) checksum += (checksum << 3) + tolower((unsigned char) *chan++);
	return checksum & (CHANHASHSIZE-1);
}

static int hash_delchan(aChan *chan)
{
	unsigned int hash = do_hashc(chan->chan);
	aChan *tmp = chan_tab[hash];

	if(tmp == chan) chan_tab[hash] = chan->next;
	else
	{
		for(; tmp && tmp->next != chan; tmp = tmp->next);
		if(tmp) tmp->next = chan->next;
		else Debug(W_MAX|W_WARN, "H_del_chan, %s non trouvé à l'offset %u ?!", chan->chan, hash);
	}
	return 0;
}

void switch_chan(aChan *c, const char *newchan)
{
	unsigned int hash = do_hashc(newchan);
	hash_delchan(c);
	Strncpy(c->chan, newchan, REGCHANLEN);
	c->next = chan_tab[hash];/* swap le vieux */
	chan_tab[hash] = c;		/* et le nouveau (mis en tête) */
}

aChan *getchaninfo(const char *chan)
{
	unsigned int hash = do_hashc(chan);
	register aChan *tmp = chan_tab[hash];
	for(; tmp && strcasecmp(chan, tmp->chan); tmp = tmp->next);
	return tmp;
}

aChan *add_chan(const char *chan, const char *description)
{
	aChan *c = calloc(1, sizeof *c);
	unsigned int hash = do_hashc(chan);

	if(!c)
	{
		Debug(W_MAX, "add_chan, malloc a échoué pour aChan %s", chan);
		return NULL;
	}

	Strncpy(c->chan, chan, REGCHANLEN);
	Strncpy(c->description, description, DESCRIPTIONLEN);
	c->suspend = NULL;
	c->motd = NULL;
	c->banlevel = DEFAUT_BANLEVEL;
	c->cml = DEFAUT_CMODELEVEL;
	c->bantype = C_DEFAULT_BANTYPE;
	c->flag = C_DEFAULT;
	c->creation_time = CurrentTS;
	c->defmodes.modes = C_DEFMODES;
	c->banhead = NULL;
	c->fltimer = NULL;
	c->bantime = C_DEFAULT_BANTIME;

	c->next = chan_tab[hash];/* swap le vieux*/
	chan_tab[hash] = c;		/* et le nouveau (mis en tête)*/
	return c;
}

void del_chan(aChan *chan, int flag, const char *raison)
{
	aBan *ban, *bt;
	aLink *lp, *lpp;

	/* Suppression de tous les accès du salon
	 (c chiant ça demande d'aller voir tous les users :(
	 >> plus maintenant grace aux links (voir plus bas). On doit
	 cependant pour le moment supprimer tous les waitaccess en cherchant
	 tous les usernames vu qu'on a pas encore mis de lien entre chan
	 et les waitaccess
	 >> plus maintenant grâce au flag WAIT_ACCESS */

	hash_delchan(chan); /* swap dans la hash */

	if(flag & HF_LOG)
		log_write(LOG_CCMD, 0, "unreg %s [%s]", chan->chan, raison);

	cswallops("Salon %s desenregistré - %s", chan->chan, raison);

	/* nettoyage */
	for(ban = chan->banhead; ban; ban = bt)
	{
		bt = ban->next;
		if(ban->timer) timer_remove(ban->timer);
		free(ban->raison);
		free(ban->mask);
		free(ban);
	}
	/* nettoyage des access (grace au link > économie de boucles) */
	for(lp = chan->access; lp; lp = lpp)
	{
		lpp = lp->next;
		del_access(lp->value.a->user, chan);
	}

	if(CJoined(chan)) cspart(chan, raison); /* leave it if needed */
	if(chan->netchan) chan->netchan->regchan = NULL; /* mark it as unreg */
	if(chan->suspend) data_free(chan->suspend);
	if(chan->fltimer) timer_remove(chan->fltimer);
	if(chan->motd) free(chan->motd);
	free(chan);
}

static inline unsigned int do_hashnc(const char *chan)
{
	unsigned int checksum = 0;
	while(*chan) checksum += (checksum << 3) + tolower((unsigned char) *chan++);
	return checksum & (NCHANHASHSIZE-1);
}

aNChan *GetNChan(const char *chan)
{
	unsigned int hash = do_hashc(chan);
	register aNChan *tmp = nchan_tab[hash];
	for(; tmp && strcasecmp(chan, tmp->chan); tmp = tmp->next);
	return tmp;
}

aNChan *new_chan(const char *chan, time_t timestamp)
{
	aNChan *c = calloc(1, sizeof *c);
	unsigned int hash = do_hashc(chan);

	if(!c)
	{
		Debug(W_MAX, "new_chan, malloc a échoué pour aNChan %s", chan);
		return NULL;
	}

	Strncpy(c->chan, chan, CHANLEN);
	c->regchan = NULL;
	c->members = NULL;
	c->timestamp = timestamp;
	c->next = nchan_tab[hash];	/* swap le vieux */
	nchan_tab[hash] = c;		/* et le nouveau (mis en tête) */
	return c;
}

void del_nchan(aNChan *chan)
{
	unsigned int hash = do_hashc(chan->chan);
	aNChan *tmp = nchan_tab[hash];

#ifdef HAVE_OPLEVELS
	if(!IsZannel(chan)) /* keep channel in memory as a Zannel */
	{
		if(!HasMode(chan, C_MAPASS))
		{
			chan->modes.modes = 0U;
			chan->modes.limit = 0U;
			*chan->modes.key = 0;
		}
		SetZannel(chan);
		return;
	}
#endif

	if(tmp == chan) nchan_tab[hash] = chan->next;
	else
	{
		for(; tmp && tmp->next != chan; tmp = tmp->next);
		if(tmp) tmp->next = chan->next;
		else Debug(W_MAX|W_WARN, "del_nchan, %s non trouvé à l'offset %u ?!", chan->chan, hash);
	}

	if(chan->regchan) chan->regchan->netchan = NULL;
	free(chan);
}

void do_cs_join(aChan *c, aNChan *nchan, int flag)
{
	SetCJoined(c);
	if(CFLimit(c)) floating_limit_update_timer(c); /* add FL timer on join */
	if(flag & JOIN_TOPIC && *c->deftopic
		&& (!*nchan->topic || (CLockTopic(c) && strcmp(c->deftopic, nchan->topic))))
			cstopic(c, c->deftopic); /* enforce a topic if needed */
}

void floating_limit_update_timer(aChan *chan)
{
	if(!CFLimit(chan) && chan->fltimer) timer_remove(chan->fltimer), chan->fltimer = NULL;
	else if(CFLimit(chan) && !chan->fltimer)
		chan->fltimer = timer_add(cf_limit_update, TIMER_PERIODIC, callback_fl_update, chan, NULL);
}

void modes_reset_default(aChan *chan)
{
	MBuf def_buf, net_buf;

	CModes2MBuf(&chan->defmodes, &def_buf, 0);

	if((chan->netchan->modes.modes &= ~chan->defmodes.modes)) /* net modes != defmodes */
	{
		CModes2MBuf(&chan->netchan->modes, &net_buf, MBUF_NOLIMIT);
		putserv("%s "TOKEN_MODE" %s -%s+%s %s %s", cs.num, chan->chan,
			net_buf.modes, def_buf.modes, net_buf.param, def_buf.param);
	}
	else putserv("%s "TOKEN_MODE" %s +%s %s", cs.num, chan->chan, def_buf.modes, def_buf.param);

	chan->netchan->modes = chan->defmodes;
}

void enforce_access_opts(aChan *c, aNick *n, anAccess *a, aJoin *j)
{
	char modes[3] = {0};
	int i = 0;

	if(!IsOp(j) && AOp(a) && !CNoOps(c)) /* need op ? */
	{
		modes[i++] = 'o';
		DoOp(j); /* update internal status */
	}
	if(!IsVoice(j) && AVoice(a) && !CNoVoices(c)) /* voice ? */
	{
		modes[i++] = 'v';
		DoVoice(j);
	}
	if(i) /* send pending modes, if any */
	{
		modes[i] = 0;
		csmode(c, MODE_OBV, "+$ $ $", modes, n->numeric, i > 1 ? n->numeric : "");
	}

	if(AOwner(a) && CWarned(c)) /* manager logs in */
	{
		cstopic(c, *c->deftopic ? c->deftopic : "Insert Topic"); 	/* try to restore topic */
		DelCWarned(c); 												/* and cancel the purge */
	}
}
