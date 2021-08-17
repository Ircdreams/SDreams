/* src/hash_nick.c - gestion des hash nick
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
 * $Id: hash_nick.c,v 1.38 2008/01/05 18:34:13 romexzf Exp $
 */

#include <ctype.h>
#include "main.h"
#include "outils.h"
#include "admin_manage.h"
#include "hash.h"
#include "del_info.h"
#include "debug.h"
#include "cs_cmds.h"
#ifdef HAVE_TRACK
#include "track.h"
#endif
#ifdef HAVE_CRYPTHOST
#include "checksum.h"
#endif

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

#ifdef HAVE_CRYPTHOST
static void hostprot(aNick *nick)
{
	UINT4 sum, digest[4];
	char key1[HOSTLEN + 1];
	const char *host = nick->host; /* real host we gonna hash */
	char *key = NULL, *key2 = NULL;

	/* mark that realhost is a nude IP for further use */
	if(is_ip(host)) nick->flag |= N_IP;

	if((nick->flag & N_SERVICE) || !(key2 = strchr(host, '.')))
	{
		Strncpy(nick->crypt, host, HOSTLEN);
		return;
	}

    Strncpy(key1, host, key2 - host);
    key = IsIP(nick) ? key2 : key1;

	if(!host[0] % 2)
	{
		MD2_CTX context;
		MD2Init(&context);
		MD2Update(&context, (unsigned char *) key, strlen(key));
		MD2Update(&context, (unsigned char *) host, strlen(host));
		MD2Final(digest, &context);
	}
	else
	{
		MD5_CTX context;
		MD5Init(&context);
		MD5Update(&context, (unsigned char *) key, strlen(key));
		MD5Update(&context, (unsigned char *) host, strlen(host));
		MD5Final(digest, &context);
	}

	sum = digest[0] + digest[1] + digest[2] + digest[3];

	if(!IsIP(nick)) mysnprintf(nick->crypt, HOSTLEN + 1, "%X%s", sum, key2);
	else mysnprintf(nick->crypt, HOSTLEN + 1, "%s.%X", key1, sum);
}
#endif

static int hash_delnick(aNick *nick)
{
	unsigned int hash = do_hashn(nick->nick);
	aNick *tmp = nick_tab[hash];

	if(tmp == nick) nick_tab[hash] = nick->next;
	else
	{
		for(; tmp && tmp->next != nick; tmp = tmp->next);
		if(tmp) tmp->next = nick->next;
		else Debug(W_WARN|W_MAX, "H_del_nick %s non trouvé à l'offset %u ?!", nick->nick, hash);
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
	unsigned int servindex = b64to10(num[1]) + 64 * b64to10(num[0]), userindex;

	if(!num[2]) return NULL; /* 2 char long numeric: server's one */

	userindex = b64to10(num[4]) + 64 * b64to10(num[3]) + 4096 * b64to10(num[2]);

	return num_tab[servindex][(userindex & serv_tab[servindex]->smask)];
}

aNick *getnickbynick(const char *nick)
{
	unsigned int hash = do_hashn(nick);
	register aNick *tmp = nick_tab[hash];
	for(; tmp && strcasecmp(nick, tmp->nick); tmp = tmp->next);
	return tmp;
}

aNick *add_nickinfo(const char *nick, const char *user, const char *host,
			const char *base64, const char *num, aServer *server, const char *name,
			time_t ttmco, const char *umode)
{
	aNick *n = NULL;
	unsigned int hash = do_hashn(nick), servindex = b64to10(num[1]) + 64 * b64to10(num[0]),
		userindex = (b64to10(num[4]) + 64 * b64to10(num[3]) + 4096 * b64to10(num[2]))
			& serv_tab[servindex]->smask; /* AND userindex with server mask */

	if(num_tab[servindex][userindex])
	{
		n = num_tab[servindex][userindex];
		Debug(W_DESYNCH|W_WARN, "add_nickinfo, Offset of %s(%s@%s) [%s] in use by %s(%s@%s) [%s]",
			nick, user, host, num, n->nick, n->ident, n->host, n->numeric);
		return NULL;
	}

	if(!(n = calloc(1, sizeof *n)))
	{
		Debug(W_MAX|W_WARN, "add_nickinfo, malloc a échoué pour aNick %s", nick);
		return NULL;
	}

	num_tab[servindex][userindex] = n; /* table des pointeurs num -> struct */

	Strncpy(n->nick, nick, NICKLEN);
	Strncpy(n->ident, user, USERLEN);
	Strncpy(n->host, host, HOSTLEN);
	Strncpy(n->base64, base64, BASE64LEN);
	strcpy(n->numeric, num);
	n->serveur = server;
	Strncpy(n->name, name, REALEN);
	n->ttmco = ttmco;
	n->floodtime = CurrentTS;
	n->floodcount = 0;

	if(umode) n->flag = parse_umode(0, umode);
#ifdef HAVE_CRYPTHOST /* cryptage de l'host en md2 ou md5 suivant certains critères */
	hostprot(n);
#endif
	base64toip(base64, &n->addr_ip);

	n->next = nick_tab[hash]; /* hash table des nick -> struct */
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

	if(nick->user) /* sauvergarde du lastlogin */
	{
#ifdef HAVE_TRACK
		if(UTracked(nick->user))
			track_notify(nick->user, "%s/%s vient de quitter (%s).",
				nick->nick, nick->user->nick, event);
#endif
		nick->user->lastseen = CurrentTS;
		str_dup(&nick->user->lastlogin, GetNUHbyNick(nick, 0));
		if(IsAdmin(nick->user)) adm_active_del(nick);
		nick->user->n = NULL;
	}

#ifdef USE_NICKSERV
	if(NHasKill(nick)) kill_remove(nick); /* purge de tous ses kills */
#endif

	num_tab[servindex][userindex] = NULL; /* tab des num purgée */
	hash_delnick(nick); /* swap du nick dans la hash table */
	free(nick);
}

void purge_network(void)
{
	int i;
	aNChan *c, *ct;

	for(i = 0; i < MAXNUM; ++i)
		if(num_tab[i])
		{
			unsigned int j = 0U;
			for(; j < serv_tab[i]->maxusers; ++j)
				if(num_tab[i][j]) del_nickinfo(num_tab[i][j]->numeric, "clearing");

			free(serv_tab[i]);
			free(num_tab[i]);
			num_tab[i] = NULL;
			serv_tab[i] = NULL;
		}

	for(i = 0; i < NCHANHASHSIZE; ++i) for(c = nchan_tab[i]; c; c = ct)
	{
		ct = c->next;
		if(c->regchan) DelCJoined(c->regchan);
		del_nchan(c);
	}
}
