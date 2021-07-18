/* src/add_info.c - Ajout d'informations en mémoire
 * Copyright (C) 2004-2005 ircdreams.org
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
 * $Id: add_info.c,v 1.73 2006/03/15 17:36:47 bugs Exp $
 */

#include "main.h"
#include "debug.h"
#include "outils.h"
#include "config.h"
#include "hash.h"
#include "ban.h"
#include "add_info.h"
#include "cs_cmds.h"
#include "del_info.h"
#include "serveur.h"
#include "timers.h"

int add_server(const char *name, const char *num, const char *hop, const char *proto, const char *from) 
{ 
        aServer *links = calloc(1, sizeof *links); 
        unsigned int servindex;
	
        if(!links) 
                return Debug(W_MAX|W_WARN, "m_server, malloc() a échoué pour le Link %s", name); 
    
        Strncpy(links->serv, name, HOSTLEN); 
        Strncpy(links->num, num, NUMSERV); 
        links->maxusers = base64toint(num + NUMSERV); 
        servindex = base64toint(links->num); 
 
        for(links->smask = 16;links->smask < links->maxusers; links->smask <<= 1);
	/* we copy smask to maxusers back in case of maxusers was below 16 */ 
        links->maxusers = links->smask--;/* on passe de 010000 à 001111 */ 
 
        if(serv_tab[servindex]) return Debug(W_DESYNCH|W_WARN, "m_server: collision, " 
                                                                        "le serveur %s[%s] existe déjà?", name, links->num); 
 
        links->hub = num2servinfo(from); /* ptr vers son Hub */ 
        serv_tab[servindex] = links; 
 
	if(!(num_tab[servindex] = calloc(1, links->maxusers * sizeof(aNick *))))
                return Debug(W_MAX|W_WARN, "m_server, malloc() a échoué pour les %d Offsets de %s", 
			links->maxusers, name);
        /* 1erhub = my uplink -> je suis enregistré! */ 
        if(atoi(hop) == 1 && strcasecmp(name, bot.server)) mainhub = links; 
 
        links->flag = (*proto == 'J') ? ST_BURST : ST_ONLINE;
	
        remove(links->serv); /* on vire l'ancien motd pour mettre le nouveau */ 
        putserv("%s MO %s", cs.num, links->num); /* demande le MOTD du nouveau serveur */
        return 0; 
} 

void add_memo(anUser *nick, const char *de, time_t date, const char *message, int flag)
{
	aMemo *memo = calloc(1, sizeof *memo), *tmp = nick->memohead;

	if(!memo)
	{
		Debug(W_MAX, "add_memo, malloc a échoué pour aMemo %s (%s)", nick->nick, message);
		return;
	}

	memo->next = NULL;
	Strncpy(memo->de, de, NICKLEN);
	memo->date = date;
	memo->flag = flag;
	Strncpy(memo->message, message, MEMOLEN);

	if(!tmp) nick->memohead = memo;
	else
	{
		for(;tmp->next;tmp = tmp->next);
		tmp->next = memo;
	}
}

void add_alias(anUser *user, const char *name)
{
        anAlias *alias = calloc(1, sizeof *alias), *tmp = user->aliashead;

        if(!alias)
        {
                Debug(W_MAX, "add_alias, malloc a échoué pour anAlias %s (%s)", user->nick, name);
                return;
        }

        Strncpy(alias->name, name, NICKLEN);

        if(!tmp) user->aliashead = alias;
	else
	{
		for(;tmp->user_nextalias;tmp = tmp->user_nextalias);
		tmp->user_nextalias = alias;
	}	
	alias->user = user;
	alias->hash_next = alias;
	hash_addalias(alias);
}


aDNR *add_dnr(const char *mask, const char *from, const char *raison,
	time_t date, unsigned int flag)
{
	aDNR *dnr = calloc(1, sizeof *dnr);
	
	if(!dnr)
	{
		Debug(W_MAX|W_WARN, "add_dnr: OOM for %s[%s] (%s)", mask, from, raison);
		return NULL;
	}
	str_dup(&dnr->mask, mask);
	str_dup(&dnr->raison, raison);
	Strncpy(dnr->from, from, NICKLEN);
	dnr->date = date;
	dnr->flag = flag; 
        dnr->next = dnrhead; 
        if(dnrhead) dnrhead->last = dnr; 
        dnrhead = dnr; 
    
        return dnr; 
}

anAccess *add_access(anUser *user, const char *chan, int level, int flag, time_t lastseen)
{
	anAccess *ptr = calloc(1, sizeof *ptr);
	aChan *c;
	aLink *lp;

	if(!ptr)
	{
		Debug(W_MAX, "add_access, malloc a échoué pour l'accès %s (%s)", user->nick, chan);
		return NULL;
	}

	ptr->level = level;
	ptr->flag = flag;
	ptr->user = user;
	ptr->info = NULL;
	ptr->lastseen = lastseen ? lastseen : user->lastseen;

	if(!(c = getchaninfo(chan)))
	{
		Debug(W_DESYNCH, "add_access, accès de %s sur %s, non reg ?", user->nick, chan);
		free(ptr);
		return NULL;
	}
	ptr->c = c;
	ptr->next = user->accesshead;
	user->accesshead = ptr;

	lp = malloc(sizeof *lp);
	lp->value.a = ptr;
	lp->next = c->access;
	c->access = lp;
	if(AOwner(ptr)) c->owner = ptr;
	return ptr;
}

void add_killinfo(aNick *nick, enum TimerType type)
{
	aKill *k = malloc(sizeof *k);

	if(!k)
	{
		Debug(W_MAX|W_WARN, "add_killinfo, malloc a échoué pour aKill %s", nick->nick);
		return;
	}

	k->nick = nick;
	k->type = type;
	nick->timer = timer_add(kill_interval, TIMER_RELATIF, callback_kill, k, NULL);

	k->last = NULL;
	if(killhead) killhead->last = k;
	k->next = killhead;
	killhead = k;
}

void add_join(aNick *nick, const char *chan, int status, time_t timestamp, aNChan *netchan)
{
	aJoin *join = calloc(1, sizeof *join);
	anAccess *a = NULL;
	aLink *lp;
	aBan *ban = NULL;
	aChan *c = NULL;

	if(!join)
	{
		Debug(W_MAX|W_WARN, "add_join, malloc a échoué pour aJoin de %s sur %s", nick->nick, chan);
		return;
	}

	if(!netchan) /* empty channel, should be from m_create */ 
        {
		if(!(status & (J_BURST|J_CREATE)))
                        Debug(W_DESYNCH|W_WARN, "add_join: Join of %s on empty channel %s ?", nick->nick, chan);
                netchan = new_chan(chan, timestamp); 
                /* find out if it's registered */ 
                if((netchan->regchan = getchaninfo(chan))) netchan->regchan->netchan = netchan;
        }

        c = netchan->regchan; /* trust netchan->regchan if it comes from m_burst/m_join */ 
    
        join->chan = netchan; 
	join->status = (status & ~(J_BURST|J_CREATE));
	join->nick = nick;
	join->next = nick->joinhead; /* insert at top of the linked list */
	nick->joinhead = join;

	join->link = lp = malloc(sizeof *lp);
	lp->value.j = join;
	if(netchan->members) netchan->members->last = lp;
	lp->last = NULL;
	lp->next = netchan->members; /* insertion du link dans les chan members */
	netchan->members = lp; 
        ++netchan->users; 
 
        if(!c || IsSuspend(c)) return; /* channel not registered or suspended, nothing more to do */ 
	
	if(!CJoined(c)) /* Channel was empty [left, not joined] */
	{       /* if it's a J(oin) and I left on admin request : just stay out. */
		csjoin(c, JOIN_FORCE);
	}

	if(nick->flag & N_HIDE) return; /* oper totalement invisible */

	/* check bans if not a burst join */
	if(!(status & J_BURST) && !IsOperOrService(nick) && !IsAnAdmin(nick->user) && !IsAnHelper(nick->user)
		&& (ban = is_ban(nick, c, NULL)))
	{
			csmode(c, MODE_OBVH, "+b $", ban->mask);
			cskick(chan, nick->numeric, "Enforce: $", ban->raison);
			return; /* just got banned, nothing more to do ! */
	}

	/* Ok, he is not banned, perform join */

	/* he has an access, mark him as on channel (bursting or not) */
	if(nick->user && (a = GetAccessIbyUserI(nick->user, c)) && !ASuspend(a)) a->lastseen = 1; 
        else a = NULL; 

	if(status & J_BURST) return; /* it's a Burst, nothing more to do.. */

	if(CSetWelcome(c) && *c->welcome) /* send welcome if needed */
		csreply(nick, GetReply(nick, L_WELCOMEJOIN), c->chan, c->welcome);

	if(!a) /* he has no access, and it's outside a burst */
	{	/* if he had created this channel because I wasn't in, he doesn't deserve its +o */
		if(status & J_OP) csmode(c, MODE_OBVH, "-o$ $ $", CAutoVoice(c) ? "+v" : "", nick->numeric, CAutoVoice(c) ? nick->numeric : "");
		DeOp(join);
		if (CAutoVoice(c)) {
			if(!status & J_OP) csmode(c, MODE_OBVH, "+v $", nick->numeric);
			DoVoice(join);
		}
		return; /* remove it and return */
	}

	/* Perform priviledged(access-ed) user checks */
	enforce_access_opts(c, nick, a, join); /* auto op/halfop/voice */

	if(!CNoInfo(c) && a->info
		&& (!HasMode(netchan, C_MAUDITORIUM) || join->status)) /* finally, send infoline */
		putserv("%s "TOKEN_PRIVMSG" %s :[\2%s\2] %s", cs.num, chan, nick->user->nick, a->info);
}

void add_ban(aChan *chan, const char *mask, const char *raison, const char *de,
	time_t debut, time_t fin, int level)
{
	aBan *ban = calloc(1, sizeof *ban);
        char *ptr2; 
	
	if(!ban)
	{
		Debug(W_MAX|W_WARN, "add_ban, malloc a échoué pour aBan %s sur %s de %s (%s)",
			mask, chan->chan, de, raison);
		return;
	}
	str_dup(&ban->mask, mask);
	for(ptr2= ban->nick;*mask;++mask) /* explode the mask in nick,user,host */
		if(*mask == '!' && !*ban->user) *ptr2 = 0, ptr2 = ban->user;
		else if(*mask == '@' && !*ban->host) *ptr2 = 0, ptr2 = ban->host;
		else *ptr2++ = *mask;
	*ptr2 = 0;

	if(*ban->nick == '*' && !ban->nick[1]) ban->flag |= BAN_ANICKS;
	if(*ban->user == '*' && !ban->user[1]) ban->flag |= BAN_AUSERS;
	if(*ban->host == '*' && !ban->host[1]) ban->flag |= BAN_AHOSTS;
	
	str_dup(&ban->raison, raison); 
        Strncpy(ban->de, de, NICKLEN); 
        ban->debut = debut;
        ban->fin = fin ? debut + fin : 0;
        ban->level = level;
	ban->timer = fin ? timer_add(ban->fin, TIMER_ABSOLU, callback_ban, chan, ban) : NULL;
        ban->last = NULL;

	if(chan->banhead) chan->banhead->last = ban;
	ban->next = chan->banhead;
	chan->banhead = ban;
}
