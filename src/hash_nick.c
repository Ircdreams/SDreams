/* src/hash_nick.c - gestion des hash nick
 * Copyright (C) 2004-2005 ircdreams.org
 *
 * contact: bugs@ircdreams.org
 * site web: http://ircdreams.org
 *
 * Services pour serveur IRC. Support� sur IrcDreams V.2
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
 * $Id: hash_nick.c,v 1.26 2006/01/28 12:35:07 bugs Exp $
 */

#include <ctype.h>
#include "main.h"
#include "outils.h"
#include "admin_manage.h"
#include "hash.h"
#include "del_info.h"
#include "debug.h"
#include "cs_cmds.h"
#include "track.h"
#include "config.h"
#include "checksum.h" 
#include "multicrypt.h"

#define b64to10(x) convert2n[(unsigned char) (x)]
 
static const unsigned char convert2n[] = {
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,52,53,54,55,
   56,57,58,59,60,61, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,
   13,14,15,16,17,18,19,20,21,22,23,24,25,62, 0,63, 0, 0, 0,26,27,28,29,30,31,32,
   33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51, 0, 0, 0, 0, 0
};

static inline unsigned int do_hashn(const char *nick) 
{ 
        unsigned int checksum = 0;
	while(*nick) checksum += (checksum << 3) + tolower((unsigned char) *nick++);
        return checksum & (NICKHASHSIZE-1); 
} 

aServer *num2servinfo(const char *num) 
{ 
        unsigned int servindex = b64to10(num[1]) + 64 * b64to10(num[0]); 
        if(servindex > MAXNUM-1) return NULL; 
        else return serv_tab[servindex]; 
} 

static int hash_delnick(aNick *nick) 
{ 
        unsigned int hash = do_hashn(nick->nick); 
        aNick *tmp = nick_tab[hash];

        if(tmp == nick) nick_tab[hash] = nick->next; 
        else 
        { 
                for(;tmp && tmp->next != nick;tmp = tmp->next); 
                if(tmp) tmp->next = nick->next; 
                else Debug(W_WARN|W_MAX, "H_del_nick %s non trouv� � l'offset %u ?!", nick->nick, hash); 
        }
        return 0; 
} 
    
int switch_nick(aNick *nick, const char *newnick) 
{ 
        unsigned int hash = do_hashn(newnick); 
    
        hash_delnick(nick); 
        Strncpy(nick->nick, newnick, NICKLEN); 
        nick->next = nick_tab[hash]; 
        nick_tab[hash] = nick; 
        return 0; 
} 


aNick *num2nickinfo(const char *num)
{
	unsigned int servindex = b64to10(num[1]) + 64 * b64to10(num[0]),
	userindex = b64to10(num[4]) + 64 * b64to10(num[3]) + 4096 * b64to10(num[2]);
	return num_tab[servindex][(userindex & serv_tab[servindex]->smask)];
}

aNick *getnickbynick(const char *nick)
{
	unsigned int hash = do_hashn(nick);
	register aNick *tmp = nick_tab[hash];
	for(;tmp && strcasecmp(nick, tmp->nick);tmp = tmp->next);
	return tmp;
}

aNick *add_nickinfo(const char *nick, const char *user, const char *host, const char *base64,
			const char *num, aServer *server, const char *name, time_t ttmco, const char *umode)
{
	aNick *n = NULL;
	unsigned int hash = do_hashn(nick),
	servindex = b64to10(num[1]) + 64 * b64to10(num[0]),
	userindex = (b64to10(num[4]) + 64 * b64to10(num[3]) + 4096 * b64to10(num[2]))
		& serv_tab[servindex]->smask; /* AND userindex with server mask */

	if(num_tab[servindex][userindex])
	{
		n = num_tab[servindex][userindex];
		Debug(W_DESYNCH|W_WARN, "add_nickinfo, Offset pour %s(%s@%s) [%s] d�j� pris par %s(%s@%s) [%s]",
  	        	nick, user, host, num, n->nick, n->ident, n->host, n->numeric);
		return NULL;
	}

	if(!(n = calloc(1, sizeof *n)))
	{
		Debug(W_MAX|W_WARN, "add_nickinfo, malloc a �chou� pour aNick %s", nick);
		return NULL;
	}

	num_tab[servindex][userindex] = n; /* table des pointeurs num -> struct */

	Strncpy(n->nick, nick, NICKLEN);
	Strncpy(n->ident, user, USERLEN);
	Strncpy(n->host, host, HOSTLEN);
	Strncpy(n->base64, base64, sizeof n->base64 -1);
	strcpy(n->numeric, num);
	n->serveur = server;
	Strncpy(n->name, name, REALEN);
	n->ttmco = ttmco;
	n->floodtime = CurrentTS;
	n->floodcount = 0;

	if(umode) n->flag = parse_umode(0, umode);
	if(GetConf(CF_HAVE_CRYPTHOST)) {
		if(n->flag & N_SERVICE) strcpy(n->crypt, host);
		else hostprot(host, n->crypt);
	}	
	n->next = nick_tab[hash];/* hash table des nick -> struct*/
	nick_tab[hash] = n;
	return n;
}

void del_nickinfo(const char *num, const char *event)
{
	unsigned int servindex = b64to10(num[1]) + 64 * b64to10(num[0]),
	userindex = (b64to10(num[4]) + 64 * b64to10(num[3]) + 4096 * b64to10(num[2]))
		& serv_tab[servindex]->smask; /* AND userindex with server mask */

	aNick *nick = num_tab[servindex][userindex];
	
	if(!nick)
	{
		Debug(W_DESYNCH|W_WARN, "del_nickinfo(%s), del %s membre inconnu", event, num);
		return;
	}

	del_alljoin(nick);

	if(nick->user)	/* sauvergarde du lastlogin */
	{
		struct track *track = istrack(nick->user);
		if(GetConf(CF_TRACKSERV) && track) csreply(track->tracker, "[\2Track\2] %s (User: %s) vient de quitter (%s).",
			nick->nick, track->tracked->nick, event);

		nick->user->lastseen = CurrentTS;
		str_dup(&nick->user->lastlogin, GetNUHbyNick(nick, 0));
		if(IsAdmin(nick->user)) adm_active_del(nick);
		nick->user->n = NULL;
	}
	if(NHasKill(nick)) kill_remove(nick); /* purge de tous ses kills */

	num_tab[servindex][userindex] = NULL;/* tab des num purg�e */
	hash_delnick(nick);/* swap du nick dans la hash table*/
	free(nick);
	--nbuser;
}

void purge_network(void)
{
	int i, j;
	aNChan *c, *ct;

	for(i = 0;i < MAXNUM;++i)
		if(num_tab[i])
		{
			for(j = 0;j < serv_tab[i]->maxusers;++j)
				if(num_tab[i][j]) del_nickinfo(num_tab[i][j]->numeric, "clearing");

			free(serv_tab[i]);
			free(num_tab[i]);
			num_tab[i] = NULL;
			serv_tab[i] = NULL;
		}
	for(i = 0;i < NCHANHASHSIZE;++i) for(c = nchan_tab[i];c;c = ct) 
        { 
                ct = c->next; 
                if(c->regchan) DelCJoined(c->regchan); 
                del_nchan(c); 
        } 
}
