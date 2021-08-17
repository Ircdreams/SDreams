/* src/ban.c - Commandes en rapport avec le ban
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
 * $Id: ban.c,v 1.93 2008/01/04 13:21:34 romexzf Exp $
 */

#include "main.h"
#include "ban.h"
#include "outils.h"
#include "hash.h"
#include "add_info.h"
#include "del_info.h"
#include "cs_cmds.h"
#include "config.h"
#include <ctype.h>
#include <arpa/inet.h>

/* sous traitement des commandes ban,
 * extrait/formate les arguments du ban (+erreurs) - Cesar */
static int do_ban(aNick *nick, aChan *chan, int parc, char **parv,
	char **mask, char *raison, time_t *timeout, int *level, char **duree)
{
	int i = 2, l;

	if(parc >= 3 && isdigit((unsigned char) *parv[2])) *level = strtol(parv[i++], NULL, 10);
	*mask = parv[i++];
	if(parc >= i) /* raison ou durée */
	{
		if(*parv[i] == '%') /* durée */ *duree = ++parv[i++];
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

/* return values: 0 if new is overlapped by base */

static int ban_match(aBan *base, aBan *new)
{
	if(!(base->flag & new->flag & BAN_IP))
		return mmatch(base->mask, new->mask);

	if(base->cbits > new->cbits) return 1;

	if(mmatch(base->nick, new->nick) || mmatch(base->user, new->user))
		return 1;

	return !ipmask_check(&new->ipmask, &base->ipmask, base->cbits);
}

/* check if 'mask' is overlapped by current banlist
 * or/and try to remove overlapping bans.
 * Double match :/ -Cesar */
static int check_redundant_bans(aChan *chan, aBan *ban, aNick *nick)
{
	register aBan *b, *b2;
	int n = IsAdmin(nick->user) ? OWNERLEVEL : ChanLevelbyUserI(nick->user, chan);

	for(b = chan->banhead; b; b = b2)
	{
		b2 = b->next;

		if(!strcasecmp(b->mask, ban->mask) || !ban_match(b, ban))
			return csreply(nick, GetReply(nick, L_ALREADYMATCHED),
					ban->mask, b->mask, b->level);

		if(n >= b->level && !ban_match(ban, b)) /* b is overlapped */
			ban_del(chan, b); /* clear it if enough level */
	}
	return 1;
}

/* Repris en majorité d'ircu (Carlo Wood (Run)) */
char *pretty_mask(char *mask)
{
	static char star[2] = "*", retmask[NUHLEN];
	char *last_dot = NULL, *ptr = mask;
	/* Case 1: default */
	char *nick = mask, *user = star, *host = star;

	/* Do a _single_ pass through the characters of the mask: */
	for(; *ptr; ++ptr)
	{
		if(*ptr == '!')
		{	/* Case 3 or 5: Found first '!' (without finding a '@' yet) */
			user = ++ptr;
			host = star;
		}
		else if(*ptr == '@')
		{	/* Case 4: Found last '@' (without finding a '!' yet) */
			nick = star;
			user = mask;
			host = ++ptr;
		}
		else if(*ptr == '.')
		{	/* Case 2: Found last '.' (without finding a '!' or '@' yet) */
			last_dot = ptr;
			continue;
		}
		else continue;

		for(; *ptr; ++ptr) if(*ptr == '@') host = ptr + 1;
		break;
	}
	if(user == star && last_dot)
	{	/* Case 2: */
		nick = star;
		host = mask;
	}
	/* Check lengths */
	if(nick != star)
	{
		char *nick_end = (user != star) ? user - 1 : ptr;
		if(nick_end - nick > NICKLEN) nick[NICKLEN] = 0;
		*nick_end = 0;
	}
	if(user != star)
	{
		char *user_end = (host != star) ? host - 1 : ptr;
		if(user_end - user > USERLEN)
		{
			user = user_end - USERLEN;
			*user = '*';
		}
		*user_end = 0;
	}
	if(host != star && ptr - host > HOSTLEN)
	{
		host = ptr - HOSTLEN;
		*host = '*';
	}

	/* replace missing parts by "*" */
	if(!*host) host = star; /* prevent '*!@*' or 'a!b@' */
	if(!*user) user = star;
	if(!*nick) nick = star; /* ... and '!b@c' */

	/* Finally build the mask */
	for(ptr = retmask; *nick; ++nick)
		if(*nick != '*' || ptr == retmask || nick[-1] != '*') *ptr++ = *nick;

	for(*ptr++ = '!'; *user; ++user)
		if(*user != '*' || ptr[-1] != '*') *ptr++ = *user;

	for(*ptr++ = '@'; *host; ++host)
		if(*host != '*' || ptr[-1] != '*') *ptr++ = *host;

	*ptr = 0;
	return retmask;
}

#define NUMLEN (2*(NUMSERV+1))
#define QBAN_INIT 	0x1
#define QBAN_FLUSH 	0x2

static int queue_ban(aChan *chan, const char *mask, size_t masklen, unsigned int flag)
{
	static char parambuf[512];
	static char modesbuf[7];
	static size_t paramlen, maxlen;
	static int mcount;

	if(flag & QBAN_INIT || !mcount)
	{
		/* Initialize */
		*parambuf = 0;
		*modesbuf = 0;
		paramlen = 0;
		/* AZAAA M #<> +bbbbbb m1 m2 m3 m4 m5 m6\r\n\0 4 spaces, 3 endchar, 6 modes, 1 sign */
		maxlen = sizeof parambuf - (NUMLEN + 4 + 3 + 7 + (sizeof TOKEN_MODE -1))
								- strlen(chan->chan);
	}

	if(flag & QBAN_FLUSH || mcount > 5 || paramlen + masklen + 1 > maxlen)
	{
		modesbuf[mcount] = 0;

		csmode(chan, MODE_OBV, "-$ $", modesbuf, parambuf);
		mcount = 0;
		paramlen = 0;
	}

	if(!masklen) return 0; /* flushing : no mask to append */

	if(mcount) parambuf[paramlen++] = ' ';
	modesbuf[mcount++] = 'b';

	strcpy(parambuf + paramlen, mask);
	paramlen += masklen;

	return mcount;
}

/*
 * ban_cmd - Progs 11/09/02 (c'est l'anniversaire des attentats qui m'y a fait penser)
 * Syntaxe: BAN <chan> [level] <mask|%nick> [%durée] [raison]
 */
int ban_cmd(aNick *nick, aChan *chan, int parc, char **parv)
{
	aBan *ban = NULL;
	char raison[RAISONLEN + 1] = {0}, *duree = NULL, *mask = NULL;
	int level = 1;
	time_t timeout = 0;
	aNick *cible = NULL;

	/* Parse paramaters */
	if(!do_ban(nick, chan, parc, parv, &mask, raison, &timeout, &level, &duree))
		return 0;

	/* Old nickban */
	if(*mask == '%' && !(cible = GetMemberIbyNick(chan, ++mask)))
		return csreply(nick, GetReply(nick, L_NOTONCHAN), mask, parv[1]);

	/* Try to find a nick if string has no wildcard (nickban's best effort) */
	if(cible || (!HasWildCard(mask) && (cible = GetMemberIbyNick(chan, mask))))
	{
		if(!check_protect(nick, cible, chan)) return 0;
		mask = getbanmask(cible, chan->bantype);
	}
	else /* String is a banmask (or should be..) */
	{
		aLink *join = chan->netchan->members;

		mask = pretty_mask(mask);

		for(; join; join = join->next)
			if((cible = join->value.j->nick) != nick
				&& !match(mask, GetNUHbyNick(cible, 1))
				&& !check_protect(nick, cible, chan)) return 0;
	}

	ban = ban_create(mask,	*raison ? raison : cf_defraison, nick->user->nick, CurrentTS,
			(timeout || (duree && *duree == '0')) ? timeout : chan->bantime, level);

	if(!check_redundant_bans(chan, ban, nick))
	{
		ban_free(ban);
		return 0;
	}

	ban_add(chan, ban);
	csmode(chan, MODE_OBV, "+b $", mask);
	return 1;
}

int kickban(aNick *nick, aChan *chan, int parc, char **parv)
{
	aNick *cible;
	aBan *ban = NULL;
	char raison[RAISONLEN + 1] = {0}, *mask = NULL, *duree = NULL;
	int level = 1;
	time_t timeout = 0;

	if(!do_ban(nick, chan, parc, parv, &mask, raison, &timeout, &level, &duree))
		return 0;

	if(!strcasecmp(cs.nick, mask)) return csreply(nick, "Non!");

	if(!(cible = GetMemberIbyNick(chan, mask)))
		return csreply(nick, GetReply(nick, L_NOTONCHAN), parv[2], chan->chan);

	if(!check_protect(nick, cible, chan)) return 0;

	mask = getbanmask(cible, chan->bantype);

	csmode(chan, MODE_OBV, "+b $", mask);
	cskick(chan->chan, cible->numeric, "(\2$\2) $", nick->user->nick,
		*raison ? raison : cf_defraison);

	ban = ban_create(mask, *raison ? raison : cf_defraison, nick->user->nick, CurrentTS,
			(timeout || (duree && *duree == '0')) ? timeout : chan->bantime, level);

	if(!check_redundant_bans(chan, ban, nick))
	{
		ban_free(ban);
		return 0; /* kick permis mais ban non ajouté */
	}

	ban_add(chan, ban);
	return 1;
}

/*
 * banlist #chan
 */
int banlist(aNick *nick, aChan *chan, int parc, char **parv)
{
	aBan *ban;
	int i = 0, m = 0, id = 0;
	char buf[TIMELEN + 1];

	if(parc < 2) m = 1;

	for(ban = chan->banhead; ban; ban = ban->next)
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

	if(i) csreply(nick, GetReply(nick, L_TOTALBAN), i, PLUR(i), chan->chan);
	else csreply(nick, GetReply(nick, L_NOMATCHINGBAN), chan->chan);
	return 1;
}

/*
 * clear_bans #chan -from -minlevel -maxlevel
 */
int clear_bans(aNick *nick, aChan *chan, int parc, char **parv)
{
	aBan *ban = chan->banhead, *tmp = NULL;
	int n = IsAdmin(nick->user) ? OWNERLEVEL : ChanLevelbyUserI(nick->user, chan);
	int minlevel = getoption("-minlevel", parv, parc, 1, GOPT_INT);
	int maxlevel = getoption("-maxlevel", parv, parc, 1, GOPT_INT);
	int from = getoption("-from", parv, parc, 1, GOPT_STR), i = 0, reste = 0;

	for(; ban; ban = tmp)
	{
		tmp = ban->next;
		if((from && strcasecmp(parv[from], ban->de)) || (minlevel && ban->level < minlevel)
			|| (maxlevel && ban->level > maxlevel)) continue;

		if(n < ban->level)
		{
			csreply(nick, GetReply(nick, L_CANTREMOVEBAN), ban->mask, ban->level);
			continue;
		}

		reste = queue_ban(chan, ban->mask, strlen(ban->mask), 0);
		ban_del(chan, ban);
		++i;
	}

	if(reste) queue_ban(chan, NULL, 0, QBAN_FLUSH);

	if(i) csreply(nick, GetReply(nick, L_TOTALBANDEL), i, PLUR(i), chan->chan);
	else csreply(nick, GetReply(nick, L_NOMATCHINGBAN), chan->chan);
	return 1;
}

/* find bans matching 'bn' on 'chan' and try to remove them with 'nick''s access */
static int unban_nick(aNick *nick, aNick *bn, aChan *chan)
{
	aBan *ban, *bt = NULL;
	int i = 0, n = IsAdmin(nick->user) ? OWNERLEVEL : ChanLevelbyUserI(nick->user, chan);
	int reste = 0;

	while((ban = is_ban(bn, chan, bt)))
	{
		if(n < ban->level)
			return csreply(nick, GetReply(nick, L_CANTREMOVEBAN), ban->mask, ban->level);
		++i;
		bt = ban->next;

		reste = queue_ban(chan, ban->mask, strlen(ban->mask), 0);

		csreply(nick, GetReply(nick, L_BANDELETED), ban->mask, chan->chan);
		ban_del(chan, ban);
	}

	if(reste) queue_ban(chan, NULL, 0, QBAN_FLUSH);

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
	int i = 1, n, size = 0, count = 0, removed = 0, reste = 0;

	if(*b == '%')
	{
		aNick *nptr = getnickbynick(++b);
		if(!nptr) return csreply(nick, GetReply(nick, L_NOSUCHNICK), b);
		return unban_nick(nick, nptr, chan);
	}
	else if(isdigit((unsigned char) *b)) size = item_parselist(b, NULL, 0, &count);
	else b = pretty_mask(b); /* try to be as user-friendly as possible (enable ban a/unban a) */

	for(ban = chan->banhead, n = ChanLevelbyUserI(nick->user, chan); ban; ban = bant, ++i)
	{
		bant = ban->next;
		if((size && item_isinlist(NULL, size, i) != -1) || (!size && !mmatch(b, ban->mask)))
		{
			if(!IsAdmin(nick->user) && n < ban->level)
			{
				csreply(nick, GetReply(nick, L_CANTREMOVEBAN), ban->mask, ban->level);
				continue;
			}
			csreply(nick, GetReply(nick, L_BANDELETED), ban->mask, chan->chan);

			reste = queue_ban(chan, ban->mask, strlen(ban->mask), 0);
			ban_del(chan, ban);
			if(++removed == count) break;
		}
	}

	if(reste) queue_ban(chan, NULL, 0, QBAN_FLUSH);

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
		fastfmt(host, "$.$", nick->user->nick, cf_hidden_host);
	else
#ifdef HAVE_CRYPTHOST
			host = nick->crypt;	/* choix de l'host sur laquelle */
#else							/* le ban va porter: réelle, cryptée, +x */
			host = nick->host;	/* selon les 'protections' actuelles de nick */
#endif

	if(bantype < 2) /* ban du type *!*ident@*.domain.tld */
	{
		if(!hidden && IsIP(nick))
		{
			char *lastdot = strrchr(nick->host, '.');

			if(lastdot) /* build a '*!*user@a.b.c.*' mask */
			{
				size_t i = mysnprintf(mask, sizeof mask, "*!*%s@", nick->ident);
				Strncpy(mask + i, nick->host, lastdot - nick->host); /* append a.b.c */
				i += lastdot - nick->host;
				mask[i++] = '.';
				mask[i++] = '*'; /* then the final '*' */
				mask[i] = 0;
			}
			else mysnprintf(mask, sizeof mask, "*!*%s@%s", nick->ident, nick->host);
		}
        else
        {
			char *domain = strchr(host, '.');

			if(domain) fastfmt(mask, "*!*$@*$", nick->ident, domain);
			else mysnprintf(mask, sizeof mask, "*!*%s@%s", nick->ident, nick->host);
		}
    }
    else if(bantype == 2) fastfmt(mask, "*!*$@*", nick->ident); /* type : *!*ident@* */
	else if(bantype == 3) fastfmt(mask, "*$*!*@*", nick->nick); /* type : *nick*!*@* */
	else if(bantype == 4) fastfmt(mask, "*!*@$", host); /* type : *!*@host */
	else fastfmt(mask, "*!*$@$", nick->ident, host); /* type *!*ident@host */

	if(hidden) free(host);
	return mask;
}

aBan *is_ban(aNick *n, aChan *chan, aBan *from) /* retourne un pointeur vers le ban */
{													/* vérifie sur toutes les hosts (sauf IP) */
	aBan *b = from ? from : chan->banhead;
	char host[HOSTLEN] = "";

	for(; b; b = b->next)
	{
		if((b->flag & BAN_ANICKS || !match(b->nick, n->nick))
			&& (b->flag & BAN_AUSERS || !match(b->user, n->ident))
			&& (b->flag & BAN_AHOSTS || (
				(b->flag & BAN_IP && ipmask_check(&b->ipmask, &n->addr_ip, b->cbits))
#ifdef HAVE_CRYPTHOST
				|| !match(b->host, n->crypt)/* le plus probable */
#endif
				|| (n->user && (*host || fastfmt(host, "$.$", n->user->nick, cf_hidden_host))
					&& !match(b->host, host))
				|| !match(b->host, n->host))))
				return b;
		}

	return NULL;
}

const char *GetBanType(aChan *chan)
{
	static const char *mask[6] = {
		"<invalide>", 	"*!*ident@*.host", "*!*ident@*",
		"*nick*!*@*", "*!*@host", "*!*ident@host"
	};
	return mask[(unsigned int) chan->bantype >= ASIZE(mask) ? 0 : chan->bantype];
}

/* From ircu2.10.12 */

static unsigned int ircd_aton_ip4(const char *input, unsigned int *output, unsigned char *pbits)
{
	unsigned int dots = 0, pos = 0, part = 0, ip = 0, bits = 32;

  /* Intentionally no support for bizarre IPv4 formats (plain
   * integers, octal or hex components) -- only vanilla dotted
   * decimal quads.
   */

	if(*input == '.' || !isdigit((unsigned char) *input)) return 0; /* most current case */

	while(1) switch(input[pos])
	{
		case 0: /* End of string */
			if(dots < 3) return 0;

			out:
			ip |= part << (24 - 8 * dots);
			*output = htonl(ip);
			if(pbits) *pbits = bits;
			return pos;

		case '.':
			if(input[++pos] == '.') return 0;
			ip |= part << (24 - 8 * dots++);
			part = 0;
			if(input[pos] == '*')
			{
				while(input[++pos] == '*' || input[pos] == '.');

				if(input[pos]) return 0;
				if(pbits) *pbits = dots * 8;
				*output = htonl(ip);
				return pos;
			}
			break;

		case '/':
			if(!pbits || !isdigit((unsigned char) input[pos + 1])) return 0;

			for(bits = 0; isdigit((unsigned char) input[++pos]);)
				bits = bits * 10 + input[pos] - '0';

			if(bits > 32) return 0;
			goto out;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			part = part * 10 + input[pos++] - '0';
			if(part > 255) return 0;
			break;

		default:
			return 0;
	}
}

int ipmask_parse(const char *input, struct irc_in_addr *ip, unsigned char *pbits)
{
	const char *colon = NULL, *dot = NULL, *slash = NULL, *tmp = input;

	for(; *tmp; ++tmp)
	{
		if(*tmp == ':') colon = tmp;
		else if(*tmp == '.') dot = tmp;
		else if(*tmp == '/') slash =  tmp;
	}

	if(colon && (!dot || (dot > colon)))
	{
		unsigned int part = 0, pos = 0, ii = 0, colons = 8;
		const char *part_start = NULL;

		/* Parse IPv6, possibly like ::127.0.0.1.
		 * This is pretty straightforward; the only trick is borrowed
		 * from Paul Vixie (BIND): when it sees a "::" continue as if
		 * it were a single ":", but note where it happened, and fill
		 * with zeros afterward.
		 */

		if(input[pos] == ':')
		{
			if(input[pos+1] != ':' || input[pos+2] == ':') return 0;
			colons = 0;
			pos += 2;
			part_start = input + pos;
		}

		while(ii < 8) switch(input[pos])
		{
			unsigned char chval;

			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				chval = input[pos] - '0';

				use_chval:
				part = (part << 4) | chval;
				if(part > 0xffff) return 0;
				++pos;
				break;

			case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
				chval = input[pos] - 'A' + 10;
				goto use_chval;

			case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
				chval = input[pos] - 'a' + 10;
				goto use_chval;

			case ':':
				part_start = input + ++pos;
				if(input[pos] == '.') return 0;
				ip->in6_16[ii++] = htons(part);
				part = 0;
				if(input[pos] == ':')
				{
					if(colons < 8) return 0;
					colons = ii;
					++pos;
				}
				break;

			case '.':
			{
				uint32_t ip4;
				unsigned int len = ircd_aton_ip4(part_start, &ip4, pbits);

				if(!len || ii > 6) return 0;

				ip->in6_16[ii++] = htons(ntohl(ip4) >> 16);
				ip->in6_16[ii++] = htons(ntohl(ip4) & 65535);

				if(pbits) *pbits += 96;
				pos = part_start + len - input;
				goto finish;
			}

			case '/':
				if(!pbits || !isdigit((unsigned char) input[pos + 1])) return 0;

				ip->in6_16[ii++] = htons(part);

				for(part = 0; isdigit((unsigned char) input[++pos]); )
					part = part * 10 + input[pos] - '0';

				if(part > 128) return 0;
				*pbits = part;
				goto finish;

			case '*':
				while(input[++pos] == '*' || input[pos] == ':');

				if(input[pos] || colons < 8) return 0;
				if(pbits) *pbits = ii * 16;
				return pos;

			case '\0':
				ip->in6_16[ii++] = htons(part);
				if(colons == 8 && ii < 8) return 0;
				if(pbits) *pbits = 128;
				goto finish;

			default:
				return 0;
		}

		finish:
		if(colons < 8)
		{
			unsigned int jj;
			/* Shift stuff after "::" up and fill middle with zeros. */
			for(jj = 0; jj < ii - colons; ++jj)
				ip->in6_16[7 - jj] = ip->in6_16[ii - jj - 1];
			for(jj = 0; jj < 8 - ii; ++jj)
				ip->in6_16[colons + jj] = 0;
		}
		return pos;
	}
	else if(dot || slash)
	{
		unsigned int addr;
		int len = ircd_aton_ip4(input, &addr, pbits);

		if(len)
		{
			ip->in6_16[5] = htons(65535);
			ip->in6_16[6] = htons(ntohl(addr) >> 16);
			ip->in6_16[7] = htons(ntohl(addr) & 65535);
			if(pbits) *pbits += 96;
		}
		return len;
	}
	else if(*input == '*')
	{
		unsigned int pos = 0;

		while(input[++pos] == '*');

		if(input[pos]) return 0;
		if(pbits) *pbits = 0;
		return pos;
	}
	else return 0; /* parse failed */
}

int ipmask_check(const struct irc_in_addr *addr, const struct irc_in_addr *mask,
				unsigned char bits)
{
	int k = 0;

	for(; k < 8; ++k)
	{
		if(bits < 16)
			return !(htons(addr->in6_16[k] ^ mask->in6_16[k]) >> (16-bits));
		if(addr->in6_16[k] != mask->in6_16[k])
			return 0;
		if(!(bits -= 16))
			return 1;

	}
	return -1;
}
