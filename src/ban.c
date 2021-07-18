/* src/ban.c - Commandes en rapport avec le ban
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
 * $Id: ban.c,v 1.45 2006/03/15 17:36:47 bugs Exp $
 */

#include "main.h"
#include "ban.h"
#include "outils.h"
#include "hash.h"
#include "add_info.h"
#include "del_info.h"
#include "cs_cmds.h"
#include "config.h"
#include "admin_user.h"
#include <ctype.h>

/* sous traitement des commandes ban,
 * extrait/formate les arguments du ban (+erreurs) - Cesar */
static int do_ban(aNick *nick, aChan *chan, int parc, char **parv,
	char **mask, char *raison, time_t *timeout, int *level, char **duree)
{
	int i = 2, l;

	if(parc >= 3 && is_num(parv[2])) *level = strtol(parv[i++], NULL, 10);
	*mask = parv[i++];
	if(parc >= i) /* raison ou durée*/
	{
		if(*parv[i] == '%') /* durée*/ *duree = ++parv[i++];
		if(parc >= i) parv2msgn(parc, parv, i, raison, RAISONLEN);
	}

	l = ChanLevelbyUserI(nick->user, chan);
	if(*level < 1 || (l < *level && !IsAdmin(nick->user)))
		return csreply(nick, GetReply(nick, L_MAXLEVELISYOURS));

	if(*duree && (*timeout = convert_duration(*duree)) <= 0 && (**duree != '0' || !chan->bantime))
		return csreply(nick, GetReply(nick, L_INCORRECTDURATION));

	if(CNoBans(chan))
		return csreply(nick, GetReply(nick, L_NOBANSON), chan->chan);

	if(chan->banlevel > l && !IsAdmin(nick->user))
		return csreply(nick, GetReply(nick, L_BANLVLON), chan->banlevel, chan->chan);

	return 1;
}

/* check if 'mask' is overlapped by current banlist
 * or/and try to remove overlapping bans.
 * Double match :/ -Cesar */
static int check_redundant_bans(aChan *chan, const char *mask, aNick *nick)
{
	register aBan *b, *b2;
	int n = IsAdmin(nick->user) ? OWNERLEVEL : ChanLevelbyUserI(nick->user, chan);

	for(b = chan->banhead;b;b = b2)
	{
		b2 = b->next;

		if(!strcasecmp(b->mask, mask) || !mmatch(b->mask, mask))
			return csreply(nick, GetReply(nick, L_ALREADYMATCHED), mask, b->mask, b->level);
		if(n >= b->level && !mmatch(mask, b->mask)) /* b is overlapped */
			del_ban(chan, b); /* clear it if enough level*/
	}
	return 1;
}

/*
 * ban_cmd - Progs 11/09/02 (c'est l'anniversaire des attentats qui m'y a fait penser)
 * Syntaxe: BAN <chan> [level] <mask|%nick> [%durée] [raison]
 */
int ban_cmd(aNick *nick, aChan *chan, int parc, char **parv)
{
	aBan *ban;
	char raison[RAISONLEN + 1] = {0}, *duree = NULL, *mask = NULL;
	int level = 1, mb;
	time_t timeout = 0;

	for(mb = 0, ban = chan->banhead;ban;ban = ban->next, mb++) 
		if(mb >= WARNBAN - 1) return csreply(nick, "La Liste des Bans est pleine! Maximum = %d", WARNBAN);

	if(!do_ban(nick, chan, parc, parv, &mask, raison, &timeout, &level, &duree))
		return 0;

	if(*mask == '%') /* old nickban */
	{
		aNick *cible = GetMemberIbyNick(chan, ++mask);
		if(!cible) return csreply(nick, GetReply(nick, L_NOTONCHAN), mask, parv[1]);
		if(!check_protect(nick, cible, chan)) return 0;
		mask = getbanmask(cible, chan->bantype);
	}
	else
	{
		aLink *join = chan->netchan->members;
		mask = pretty_mask(mask);

		for(; join; join = join->next)
		{
			aNick *cible = join->value.j->nick;
			if(cible != nick && !match(mask, GetNUHbyNick(cible, 1))
				&& !check_protect(nick, cible, chan)) return 0;
		}
	}

	/* on check si le mask a ban match celui du bot */
	if (!match(mask,GetNUHbyNick(getnickbynick(cs.nick),0))) return csreply(nick, "Non!");

	if(!check_redundant_bans(chan, mask, nick)) return 0;

	csmode(chan, MODE_OBVH, "+b $", mask);
	add_ban(chan, mask,	*raison ? raison : defraison, nick->user->nick, CurrentTS,
			(timeout || (duree && *duree == '0')) ? timeout : chan->bantime,
			level);
	return 1;
}

int kickban(aNick *nick, aChan *chan, int parc, char **parv)
{
	aBan *ban;
	aNick *cible;
	char raison[RAISONLEN + 1] = {0}, *mask = NULL, *duree = NULL;
	int level = 1, mb;
	time_t timeout = 0;

	for(mb = 0, ban = chan->banhead;ban;ban = ban->next, mb++)
                if(mb >= WARNBAN - 1) return csreply(nick, "La Liste des Bans est pleine! Maximum = %d", WARNBAN);

	if(!do_ban(nick, chan, parc, parv, &mask, raison, &timeout, &level, &duree))
		return 0;

	if(!strcasecmp(cs.nick, mask)) return csreply(nick, "Non!");

	if(!(cible = GetMemberIbyNick(chan, mask)))
		return csreply(nick, GetReply(nick, L_NOTONCHAN), parv[2], chan->chan);

	if(!check_protect(nick, cible, chan)) return 0;

	mask = getbanmask(cible, chan->bantype);

	csmode(chan, MODE_OBVH, "+b $", mask);
	cskick(chan->chan, cible->numeric, "(\2$\2) $", nick->user->nick, *raison ? raison : defraison);

	if(!check_redundant_bans(chan, mask, nick)) return 0;/* kick permis mais ban non ajouté*/

	add_ban(chan, mask,	*raison ? raison : defraison, nick->user->nick, CurrentTS,
			(timeout || (duree && *duree == '0')) ? timeout : chan->bantime,
			level);
	return 1;
}

/*
 * banlist #chan
 */
int banlist(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	aBan *ban;
	int i = 0, m = 0, id = 0;
	char buf[TIMELEN + 1];

	if(parc < 2) m = 1;

	for(ban = chaninfo->banhead;ban;ban = ban->next)
	{
		++id;
		if(m || !mmatch(parv[2], ban->mask))
		{
			++i;
			csreply(nick, GetReply(nick, L_BLMASK), ban->mask, ban->level, id);
			csreply(nick, GetReply(nick, L_BLFROM), ban->de, ban->raison);

			Strncpy(buf, get_time(nick, ban->debut), sizeof buf - 1);
			csreply(nick, GetReply(nick, L_BLEXPIRE), buf,
                                ban->fin ? get_time(nick, ban->fin) : "Jamais");
			csreply(nick, "-");
		}
	}

	if(i) csreply(nick, GetReply(nick, L_TOTALBAN), i, PLUR(i), chaninfo->chan);
        else csreply(nick, GetReply(nick, L_NOMATCHINGBAN), chaninfo->chan);
	return 1;
}

/*
 * clear_bans #chan -from -minlevel -maxlevel
 */
int clear_bans(aNick *nick, aChan *chan, int parc, char **parv)
{
	aBan *ban = chan->banhead, *tmp = NULL;
	char modes[7] = {0}, masks[451] = {0};
	size_t size = 0, temp;
	int n = IsAdmin(nick->user) ? OWNERLEVEL : ChanLevelbyUserI(nick->user, chan);
	int minlevel = getoption("-minlevel", parv, parc, 1, 1);
	int maxlevel = getoption("-maxlevel", parv, parc, 1, 1);
	int from = getoption("-from", parv, parc, 1, 0), i = 0, nb = 0, s;

	for(;ban;ban = tmp)
	{
		tmp = ban->next;
		if((from && strcasecmp(parv[from], ban->de)) || (minlevel && ban->level < minlevel)
			|| (maxlevel && ban->level > maxlevel)) continue;

		if(n < ban->level)
		{
			csreply(nick, GetReply(nick, L_CANTREMOVEBAN), ban->mask, ban->level);
			continue;
		}/* format la chaine de modes pour l'unban 6 -b max dans 450 bytes max */

		temp = size;/* taille actuelle */
		size += (s = strlen(ban->mask)); /* future taille */
		if(size > 450 || nb > 5) /* buf plein ou 6 modes queued */
		{
			modes[nb] = 0;
			csmode(chan, MODE_OBVH, "-$ $", modes, masks);
			size = s, temp = 0,	nb = 0;
		}
		strcpy(&masks[temp], ban->mask);
		modes[nb++] = 'b';
		masks[size++] = ' ';
		masks[size] = 0;
		del_ban(chan, ban);
		++i;
	}
	modes[nb] = 0;
	if(nb) csmode(chan, MODE_OBVH, "-$ $", modes, masks);

	if(i) csreply(nick, GetReply(nick, L_TOTALBANDEL), i, PLUR(i), chan->chan);
        else csreply(nick, GetReply(nick, L_NOMATCHINGBAN), chan->chan);
	return 1;
}

static int unban_nick(aNick *nick, aNick *bn, aChan *chan)
{
	aBan *ban, *bt = NULL;
	int i = 0, n = IsAdmin(nick->user) ? OWNERLEVEL : ChanLevelbyUserI(nick->user, chan);

	while((ban = is_ban(bn, chan, bt)))
	{
		if(n < ban->level)
			return csreply(nick, GetReply(nick, L_CANTREMOVEBAN), ban->mask, ban->level);
		++i;
		bt = ban->next;
		csmode(chan, MODE_OBVH, "-b $", ban->mask);
		csreply(nick, GetReply(nick, L_BANDELETED), ban->mask, chan->chan);
		del_ban(chan, ban);
	}

	csreply(nick, GetReply(nick, L_TOTALBANDEL), i, PLUR(i), chan->chan);

	/* invite après l'unban (si complet) pour passer les +b locaux */
	if(!GetJoinIbyNC(bn, chan->netchan))
		putserv("%s " TOKEN_INVITE " %s :%s", cs.num, bn->nick, chan->chan);

	return 1;
}

int unban(aNick *nick, aChan *chan, int parc, char **parv)
{
	aBan *ban, *bant = NULL;
	char *b = parv[2];
	int i = 1, n, items[10], count = 0, removed = 0;

	if(*b == '%')
	{
		aNick *nptr = getnickbynick(++b);
		if(!nptr) return csreply(nick, GetReply(nick, L_NOSUCHNICK), b);
		return unban_nick(nick, nptr, chan);
	}
	else if(isdigit(*b)) count = item_parselist(b, items, ASIZE(items));
	else b = pretty_mask(b); /* try to be as user-friendly as possible (enable ban a/unban a) */

	for(ban = chan->banhead, n = ChanLevelbyUserI(nick->user, chan);ban;ban = bant, ++i)
	{
		bant = ban->next;
		if((count && item_isinlist(items, count, i) != -1) || (!count && !mmatch(b, ban->mask)))
		{
			if(!IsAdmin(nick->user) && n < ban->level)
			{
				csreply(nick, GetReply(nick, L_CANTREMOVEBAN), ban->mask, ban->level);
				continue;
			}
			csreply(nick, GetReply(nick, L_BANDELETED), ban->mask, chan->chan);
			csmode(chan, MODE_OBVH, "-b $", ban->mask);
			del_ban(chan, ban);
			if(count == ++removed) break; /* gonna write the banlist at the end */
		}
	}

	if(!removed) csreply(nick, GetReply(nick, L_NOMATCHINGBAN), chan->chan);
	return 1;
}

int unbanme(aNick *nick, aChan *chan, int parc, char **parv)
{
	return unban_nick(nick, nick, chan);
}

char *getbanmask(aNick *nick, int bantype)
{
	static char mask[HOSTLEN + USERLEN + 5];
	char *host = NULL, hidden = (nick->user && IsHidden(nick));

	if(hidden && (host = malloc(NICKLEN + HOSTLEN + 3)))
		fastfmt(host, "$.$", nick->user->nick, hidden_host);
        else if(GetConf(CF_HAVE_CRYPTHOST)) host = nick->crypt; /* choix de l'host sur laquelle le ban va porter: réelle, cryptée, +x */
	else host = nick->host;      /* selon les 'protections' actuelles de nick */

	if(bantype < 2) /* ban du type *!*ident@*.host */
	{
		if(!hidden && is_ip(nick->host))
		{
			if(GetConf(CF_HAVE_CRYPTHOST)) {
				unsigned int a = strtol(nick->host, NULL, 10); /* it will stop at the first dot */
				snprintf(mask, sizeof mask, "*!*%s@%u.*", nick->ident, a);
			}
			else {
				unsigned int a, b, c;
				sscanf(host, "%u.%u.%u.", &a, &b, &c);
				snprintf(mask, sizeof mask, "*!*%s@%u.%u.%u.*", nick->ident, a, b, c);
			}
		}
        	else fastfmt(mask, "*!*$@*$", nick->ident, strchr(host, '.'));
    	}
    	else if(bantype == 2) fastfmt(mask, "*!*$@*", nick->ident); /* type : *!*ident@* */
	else if(bantype == 3) fastfmt(mask, "*$*!*@*", nick->nick); /* type : *nick*!*@* */
	else if(bantype == 4) fastfmt(mask, "*!*@$", host); /* type : *!*@host */
	else fastfmt(mask, "*!*$@$", nick->ident, host); /* type *!*ident@host*/

	if(hidden) free(host);
	return mask;
}

aBan *is_ban(aNick *nick, aChan *chan, aBan *from) /* retourne un pointeur vers le ban */
{													/* vérifie sur toutes les hosts (sauf IP) */
	aBan *b = from ? from : chan->banhead;
	char host[HOSTLEN] = "";

	for(;b;b = b->next)
	{
		 if((b->flag & BAN_ANICKS || !match(b->nick, nick->nick))
                        && (b->flag & BAN_AUSERS || !match(b->user, nick->ident))
                        && (b->flag & BAN_AHOSTS || ((GetConf(CF_HAVE_CRYPTHOST)
			&& !match(b->host, nick->crypt)) || /* le plus probable */
			(nick->user     && (*host || fastfmt(host, "$.$", nick->user->nick, hidden_host))
                        && !match(b->host, host)) || !match(b->host, nick->host))))
                                return b;
                }

	return NULL;
}

char *GetBanType(aChan *chan)
{
	static char *mask[6] = {
		"<invalide>", 	"*!*ident@*.host",
		"*!*ident@*", 	"*nick*!*@*",
		"*!*@host",		"*!*ident@host"
	};
	return mask[chan->bantype >= ASIZE(mask) ? 0 : chan->bantype];
}
