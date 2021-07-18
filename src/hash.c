/* src/hash.c dohash()
 * Copyright (C) 2005-2006 ircdreams.org
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
 * $Id: hash.c,v 1.17 2006/03/15 06:43:23 bugs Exp $
 */

#include "config.h"
#include "main.h"
#include "hash.h"
#include "outils.h"
#include "timers.h"
#include <ctype.h>

int ChanLevelbyUserI(anUser *user, aChan *chan)
{
	anAccess *acces = GetAccessIbyUserI(user, chan);
	return acces ? acces->level : 0;
}

aJoin *getjoininfo(aNick *nick, const char *chan)
{
	register aJoin *join = nick->joinhead;
	for(;join && strcasecmp(chan, join->chan->chan);join = join->next);
	return join;
}

aJoin *GetJoinIbyNC(aNick *nick, aNChan *chan)
{
	register aJoin *join = nick->joinhead;
	for(;join && chan != join->chan;join = join->next);
	return join;
}

aDNR *find_dnr(const char *pattern, int type)
{
	aDNR *dnr = dnrhead;

	for(;dnr;dnr = dnr->next)
		if((type & dnr->flag) & (DNR_TYPECHAN|DNR_TYPEUSER)
			&& ((type & DNR_MASK && !mmatch(dnr->mask, pattern))
			|| (!(type & DNR_MASK) && !strcasecmp(dnr->mask, pattern)))) break;
	return dnr;
}

char *IsAnOwner(anUser *user)
{
	anAccess *acces = user->accesshead;
	for(;acces && !AOwner(acces);acces = acces->next);
	return acces ? acces->c->chan : NULL;
}

anAccess *GetAccessIbyUserI(anUser *user, aChan *chan)
{
	register anAccess *a = user->accesshead;
	for(;a && a->c != chan; a = a->next);
	return (a && !AWait(a)) ? a : NULL;
}

aNick *GetMemberIbyNick(aChan *chan, const char *nick)
{
	aLink *l = chan->netchan->members;
	for(;l && strcasecmp(l->value.j->nick->nick, nick);l = l->next);
	return l ? l->value.j->nick : NULL;
}

aServer *GetLinkIbyServ(const char *serv)
{
	int i = 0;
	for(;i < MAXNUM;++i)
		if(serv_tab[i] && !strcasecmp(serv_tab[i]->serv, serv)) return serv_tab[i];
	return NULL;
}

int handle_suspend(struct suspendinfo **suspend, const char *nick, 
        const char *ptr, time_t timeout) 
{ 
	struct suspendinfo *s_p = *suspend;

        /* purge suspend details */ 
	if(ptr && !strcasecmp(ptr, "-nolog"))
	{
		if(s_p && s_p->timer) timer_remove(s_p->timer); 
                free(s_p), *suspend = NULL; 
        } 
        else if(!(s_p && (!s_p->expire || s_p->expire > CurrentTS))) 
        {       /* ACT! */ 
                if(!ptr || !*ptr) return -1; 
                do_suspend(suspend, ptr, nick, timeout, CurrentTS); 
                return 1; 
        } 
        else /* try to update some details (expiration, reason) */ 
        {       /* otherwise unsuspend it */ 
		time_t old_expire = s_p->expire;
		if(timeout) s_p->expire = timeout + CurrentTS;
		else if(ptr && *ptr) Strncpy(s_p->raison, ptr, sizeof s_p->raison -1);
		else s_p->expire = -1;

		if(old_expire != s_p->expire) 
                { 
                        if(s_p->timer) timer_dequeue(s_p->timer); /* dequeue old timer */ 
                        /* Not suspended any more, free timer */ 
                        if(s_p->expire < 0) 
                        { 
                                if(s_p->timer) timer_free(s_p->timer), s_p->timer = NULL; 
                        } 
                        else if(!old_expire) s_p->timer = timer_add(s_p->expire, TIMER_ABSOLU, 
                                     callback_unsuspend, suspend, NULL); 
                        else s_p->timer->delay = s_p->expire, timer_enqueue(s_p->timer); 
                } 
        } 
        return 0; 
} 

void do_suspend(struct suspendinfo **suspend, const char *raison, const char *from, 
        time_t expire, time_t debut)
{ 
        if(!*suspend) *suspend = calloc(1, sizeof **suspend); 
        Strncpy((*suspend)->raison, raison, sizeof (*suspend)->raison -1); 
	Strncpy((*suspend)->from, from, NICKLEN);
	(*suspend)->data = NULL;
        (*suspend)->debut = debut; 
        (*suspend)->expire = expire > 0 ? expire + debut : expire; 
	(*suspend)->timer = expire > 0 ? timer_add((*suspend)->expire, TIMER_ABSOLU,
		callback_unsuspend, suspend, NULL) : NULL;
}

char *GetUserOptions(anUser *user)
{	/* last '15' is langsize, 1 is \0 */
	static char option[8+7+12+6+6+6+9+7+9+12+12+17+19+13+11+15+6+6+15+1];
	int i = 0;
	option[0] = '\0';

	if(UNopurge(user)) strcpy(option, "NoPurge "), i += 8;
	if(UNoMemo(user)) strcpy(option + i, "NoMemo "), i += 7;
	if(UFirst(user)) strcpy(option + i, "Registering "), i += 12;
	if(UOubli(user)) strcpy(option + i, "Oubli "), i += 6;
	if(UVhost(user)) strcpy(option + i, "VHost "), i += 6;
	if(UWantX(user)) strcpy(option + i, "Modex "), i += 6;
	if(URealHost(user)) strcpy(option + i, "RealHost "), i += 9;
	if(USWhois(user)) strcpy(option + i, "SWhois "), i += 7;
	if(user->flag & U_ALREADYCHANGE) strcpy(option + i, "UserChanged "), i += 12;
	if(CantRegChan(user)) strcpy(option + i, "CantRegChan "), i += 12;
	if(UNoVote(user)) strcpy(option +i, "NoVote "), i += 7;
	if(UPMReply(user)) strcpy(option +i, "ReplyMsg "), i += 9;
	if(GetConf(CF_NICKSERV)) {
		if(UPKill(user)) strcpy(option + i, "Protection(Kill) "), i += 17;
		else if(UPNick(user)) strcpy(option + i, "Protection(ChNick) "), i += 19;
	}
	if(UPReject(user)) strcpy(option + i, "Accès(Rejet) "), i += 13;
	else if(UPAsk(user)) strcpy(option + i, "Accès(Ask) "), i += 11;
	else if(UPAccept(user)) strcpy(option + i, "Accès(Accepte) "), i += 15;
	if(UMale(user)) strcpy(option + i, "Homme "), i += 6;
	if(UFemelle(user)) strcpy(option + i, "Femme "), i += 6;
	strcpy(option + i, user->lang->langue);

	if(option[0] == '\0') strcpy(option, "Aucune");
	return option;
}

char *GetAccessOptions(anAccess *acces)
{
	static char options[100];
	int i = 0;
	options[0] = '\0';
	
	if(AOp(acces)) strcpy(options, " autoop"), i += 7;
	if(AProtect(acces)) strcpy(options + i, " protégé"), i += 8;
	if(AVoice(acces)) strcpy(options + i, " autovoice"), i += 10;
	if(AHalfop(acces)) strcpy(options +i, " autohop"), i += 9;
	if(ASuspend(acces)) strcpy(options + i, " suspendu");
	if(options[0] == '\0') strcpy(options, " Aucune");
	return options;
}
