/* src/outils.c - Divers outils
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
 * $Id: outils.c,v 1.202 2008/01/05 01:24:13 romexzf Exp $
 */

#include <ctype.h>
#include <limits.h>
#include "main.h"
#include "cs_cmds.h"
#include "admin_manage.h"
#include "hash.h"
#include "outils.h"
#include <arpa/inet.h>

static item_opt item_static_opts[10] = { {0} };

static const struct Modes {
	char mode;
	unsigned int value;
} UMode[] = {
	{ 'i', N_INV },
	{ 'x', N_HIDE },
	{ 'r', N_REG },
	{ 'o', N_OPER },
	{ 'k', N_SERVICE },
	{ 'a', N_ADM },
	{ 'p', N_GOD },
	{ 'f', N_FEMME },
	{ 'h', N_HOMME },
	{ 'g', N_DEBUG },
	{ 'w', N_WALLOPS },
	{ 'D', N_DIE },
	{ 'd', N_DEAF },
	{ 'H', N_SPOOF },
},
	CMode[] = {
	{ 'n', C_MMSG },
	{ 't', C_MTOPIC },
	{ 'i', C_MINV },
	{ 'l', C_MLIMIT },
	{ 'k', C_MKEY },
	{ 's', C_MSECRET },
	{ 'p', C_MPRIVATE },
	{ 'm', C_MMODERATE },
	{ 'c', C_MNOCTRL },
	{ 'C', C_MNOCTCP },
	{ 'O', C_MOPERONLY },
	{ 'r', C_MUSERONLY },
	{ 'D', C_MDELAYJOIN },
	{ 'N', C_MNONOTICE },
#ifdef HAVE_OPLEVELS
	{ 'A', C_MAPASS },
	{ 'U', C_MUPASS }
#endif
};

static const unsigned int CModesToFlag[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	0,
	0, 0, 0, 0, 0, 0, 0, 0, 0 /* ! */, 0 /* " */, 0 /* # */, 0 /* $ */,
	0 /* % */, 0 /* & */, 0 /* ' */, 0 /* ( */, 0 /* ) */, 0 /* * */,
	0 /* + */, 0 /* , */, 0 /* - */, 0 /* . */, 0 /* / */, 0 /* 0 */,
	0 /* 1 */, 0 /* 2 */, 0 /* 3 */, 0 /* 4 */, 0 /* 5 */, 0 /* 6 */,
	0 /* 7 */, 0 /* 8 */, 0 /* 9 */, 0 /* : */, 0 /* ; */, 0 /* < */,
	0 /* = */, 0 /* > */, 0 /* ? */, 0 /* @ */, C_MAPASS /* A */, 0 /* B */,
	C_MNOCTCP /* C */, C_MDELAYJOIN /* D */, 0 /* E */, 0 /* F */, 0 /* G */,
	0 /* H */, 0 /* I */, 0 /* J */, 0 /* K */, 0 /* L */, 0 /* M */,
	C_MNONOTICE /* N */, C_MOPERONLY /* O */, 0 /* P */, 0 /* Q */,
	0 /* R */, 0 /* S */, 0 /* T */, C_MUPASS /* U */, 0 /* V */,
	0 /* W */, 0 /* X */, 0 /* Y */, 0 /* Z */, 0 /* [ */, 0 /* \ */,
	0 /* ] */, 0 /* ^ */, 0 /* _ */, 0 /* ` */, 0 /* a */, 0 /* b */,
	C_MNOCTRL /* c */, 0 /* d */, 0 /* e */, 0 /* f */, 0 /* g */,
	0 /* h */, C_MINV /* i */, 0 /* j */, C_MKEY /* k */, C_MLIMIT /* l */,
	C_MMODERATE /* m */, C_MMSG /* n */, 0 /* o */, C_MPRIVATE /* p */,
	0 /* q */, C_MUSERONLY /* r */, C_MSECRET /* s */, C_MTOPIC /* t */,
	0 /* u */, 0 /* v */, 0 /* w */, 0 /* x */, 0 /* y */, 0 /* z */,
	0 /* { */, 0 /* | */, 0 /* } */, 0 /* ~ */, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

unsigned int parse_umode(unsigned int flag, const char *p)
{ /* default is '+' */
	unsigned int w = 1, i;

	if(!p) return flag;
	for(; *p; ++p)
	{
		if(*p == '+') w = 1;
		else if(*p == '-') w = 0;
		else
			for(i = 0U; i < ASIZE(UMode); ++i)
				if(*p == UMode[i].mode)
				{
					if(w) flag |= UMode[i].value;
					else flag &= ~UMode[i].value;
					break;
				}
	}
	return flag;
}

char *GetModes(unsigned int flag)
{
	static char mode[ASIZE(UMode)];
	unsigned int i = 0U, j = 0U;
	for(; i < ASIZE(UMode); ++i) if(flag & UMode[i].value) mode[j++] = UMode[i].mode;
	mode[j] = '\0';
	return mode;
}

unsigned int cmodetoflag(unsigned int flag, const char *mode)
{
	int w = 1;
	register unsigned int i;

	if(!mode) return flag;
	for(; *mode; ++mode)
	{
		if(*mode == '+') w = 1;
		else if(*mode == '-') w = 0;
		else if((i = CModesToFlag[(unsigned char) *mode]))
		{
			if(w)
			{
				flag |= i;
				if(i == C_MSECRET && flag & C_MPRIVATE) flag &= ~C_MPRIVATE;
				else if(i == C_MPRIVATE && flag & C_MSECRET) flag &= ~C_MSECRET;
			}
			else flag &= ~i;
		}
	}
	return flag;
}

unsigned int string2scmode(struct cmode *mode, const char *modes,
	const char *key, const char *limit)
{
    unsigned int newmode = cmodetoflag(mode->modes, modes);

	if(newmode & C_MLIMIT)
	{
		int lim = 0;
		if(limit && (lim = strtol(limit, NULL, 10)) > 0) mode->limit = lim;
		else if(!(mode->modes & C_MLIMIT)) newmode &= ~C_MLIMIT;/* limit actually invalid */
	}
	else mode->limit = 0U;

	if(newmode & C_MKEY)
	{
		if(key && *key) Strncpy(mode->key, key, KEYLEN);
		else if(!*mode->key) newmode &= ~C_MKEY;/* actually no key set */
	} /* -k autorisé seulement si key fournie */
	else if(mode->modes & C_MKEY && (!key || !*key)) newmode |= C_MKEY;
	else *mode->key = 0;

	return (mode->modes = newmode);
}

char *GetCModes(struct cmode modes)
{
	static char mode[ASIZE(CMode) + KEYLEN + 20];
	unsigned int i = 0U, j = 0U;

	for(; i < ASIZE(CMode); ++i) if(modes.modes & CMode[i].value) mode[j++] = CMode[i].mode;

	if(modes.modes & C_MLIMIT) j += mysnprintf(mode + j, sizeof mode - j, " %u", modes.limit);
	if(modes.modes & C_MKEY)
	{
		mode[j++] = ' ';
		strcpy(mode + j, modes.key);
	}
	else mode[j] = '\0';
	return mode;
}

void CModes2MBuf(struct cmode *modes, MBuf *mbuf, unsigned int flag)
{
	unsigned int i = 0U, j = 0U;

	*mbuf->param = 0; /* init */
	for(; i < ASIZE(CMode); ++i)
		if(modes->modes & CMode[i].value) mbuf->modes[j++] = CMode[i].mode;
	mbuf->modes[j] = 0;

	if(modes->modes & C_MLIMIT && !(flag & MBUF_NOLIMIT))
		j = mysnprintf(mbuf->param, sizeof mbuf->param, "%u ", modes->limit);
	else j = 0;
	if(modes->modes & C_MKEY && !(flag & MBUF_NOKEY))
		strcpy(mbuf->param + j, modes->key);
}

void modes_reverse(aNChan *chan, const char *modes, const char *key, const char *limit)
{
	char modesp_b[20] = "+", modesl_b[20] = "-", marg[KEYLEN + 20] = {0};
	int modesp_i = 1, modesl_i = 1, marg_i = 0, sign = 1, needed = 0;;

	for(; *modes; ++modes)
	{
		switch(*modes)
		{
			case 'o': case 'b': case 'v':
#ifdef HAVE_OPLEVELS
			case 'A': case 'U':
#endif
				break;

			case '+': sign = 1; break;
			case '-': sign = 0; break;

			case 'k': /* same key as ours */
				if(sign && key && !strcmp(chan->modes.key, key)) break;
				needed |= C_MKEY;
				break;

			case 'l': /* same limit as ours */
				if(sign && limit && strtoul(limit, NULL, 10) == chan->modes.limit) break;
				needed |= C_MLIMIT;
				break;

			default:
				if(sign && HasMode(chan, CModesToFlag[(unsigned char) *modes])) break;

				if(!sign) modesp_b[modesp_i++] = *modes;
				else modesl_b[modesl_i++] = *modes;
				break;
		} /* switch */
	} /* for */
	/* Key and limit are handled at the end so as to be sure of their respective
	 * order in a modebuf, therefore marg is in the right order too */
	if(needed & C_MLIMIT)
	{
		if(HasMode(chan, C_MLIMIT)) /* I had a limit, different than yours! */
		{	/* set back to mine */
			modesp_b[modesp_i++] = 'l';
			marg_i = mysnprintf(marg, sizeof marg, "%u ", chan->modes.limit);
		}
		else modesl_b[modesl_i++] = 'l';
	}
	if(needed & C_MKEY)
	{
		if(HasMode(chan, C_MKEY)) modesp_b[modesp_i++] = 'k';
		else modesl_b[modesl_i++] = 'k';
		strcpy(marg + marg_i, HasMode(chan, C_MKEY) ? chan->modes.key : key);
	}

	if(modesp_i > 1 || modesl_i > 1)
	{
		modesp_b[modesp_i > 1 ? modesp_i : 0] = 0;
		modesl_b[modesl_i > 1 ? modesl_i : 0] = 0;
		/* then, flush modes */
		putserv("%s "TOKEN_MODE" %s %s%s %s", cs.num, chan->chan, modesp_b, modesl_b, marg);
	}
}

#define b64to10(x) convert2n[(unsigned char) (x)]

static const unsigned char convert2n[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,52,53,54,55,
   56,57,58,59,60,61, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,
   13,14,15,16,17,18,19,20,21,22,23,24,25,62, 0,63, 0, 0, 0,26,27,28,29,30,31,32,
   33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51, 0, 0, 0, 0, 0
};

unsigned int base64toint(const char *s)
{
    unsigned int i = 0U;
    while(*s) i = (i << 6) + b64to10(*s++);
    return i;
}

int is_ip(const char *ip)
{
	char *ptr = NULL;
	int i = 0, d = 0;

	for(; i < 4; ++i) /* 4 dots expected (IPv4) */
	{	/* Note about strtol: stores in endptr either NULL or '\0' if conversion is complete */
		if(!isdigit((unsigned char) *ip) /* most current case (not ip, letter host) */
			|| (d = strtol(ip, &ptr, 10)) < 0 || d > 255 /* ok, valid number? */
			|| (ptr && *ptr != 0 && (*ptr != '.' || 3 == i) && ptr != ip)) return 0;
		if(ptr) ip = ptr + 1, ptr = NULL; /* jump the dot */
	}
	return 1;
}

char *GetUserIP(aNick *nick, char *dest)
{
	return GetIP(&nick->addr_ip, dest);
}

char *GetIP(struct irc_in_addr *ip, char *dest)
{
	static char ipbuf[DOTTEDIPLEN + 1];

	if(!dest) dest = ipbuf;

#ifdef HAVE_IPV6
	if(irc_in_addr_is_ipv4(ip))
	{
		static const char hexdigits[] = "0123456789abcdef";
		unsigned int pos, part, max_start = 1, max_zeros = 0, curr_zeros = 0, ii = 1;

		/* Find longest run of zeros. */
		for(; ii < 8; ++ii)
		{
			if(!ip->in6_16[ii]) ++curr_zeros;
			else if(curr_zeros > max_zeros)
			{
				max_start = ii - curr_zeros;
				max_zeros = curr_zeros;
				curr_zeros = 0;
			}
		}

		if(curr_zeros > max_zeros)
		{
			max_start = ii - curr_zeros;
			max_zeros = curr_zeros;
		}

		/* Print out address. */

		for(pos = ii = 0; ii < 8; ++ii)
		{
			if(max_zeros > 0 && ii == max_start)
			{
				*dest++ = ':';
				ii += max_zeros - 1;
				continue;
			}

			part = ntohs(ip->in6_16[ii]);

			if(part >= 0x1000) *dest++ = hexdigits[part >> 12];
			if(part >= 0x100) *dest++ = hexdigits[(part >> 8) & 15];
			if(part >= 0x10) *dest++ = hexdigits[(part >> 4) & 15];

			*dest++ = hexdigits[part & 15];
			if(ii < 7) *dest++ = ':';
		}
		*dest = 0;
	}
	else
#endif
	{
		unsigned char *bytes = (unsigned char *) &ip->in6_16[6];
		mysnprintf(dest, sizeof ipbuf, "%u.%u.%u.%u",
			bytes[0], bytes[1], bytes[2], bytes[3]);
	}
	return dest;
}

void base64toip(const char *base64, struct irc_in_addr *addr)
{
	memset(addr, 0, sizeof *addr);

	if(!base64[6])
	{
		unsigned int in = base64toint(base64);
		/* An all-zero address should stay that way. */

		if(in)
		{
			addr->in6_16[5] = htons(65535);
			addr->in6_16[6] = htons(in >> 16);
			addr->in6_16[7] = htons(in & 65535);
		}
	}
	else
	{
		unsigned int pos = 0;
		do {
			if(*base64 == '_')
			{
				unsigned int left = (25 - strlen(base64)) / 3 - pos;

				for(; left; --left) addr->in6_16[pos++] = 0;
				++base64;
			}
			else
			{
				unsigned short accum = convert2n[(unsigned char)*base64++];
				accum = (accum << 6) | convert2n[(unsigned char)*base64++];
				accum = (accum << 6) | convert2n[(unsigned char)*base64++];
				addr->in6_16[pos++] = ntohs(accum);
			}
		} while(pos < 8);
	}
}

/*
 * getoption:
 * recherche dans un tableau char ** (de 'parc' entrées)
 * l'expression "opt" à partir de l'élément 'deb'
 * -si exp_int est > 0: (GOPT_INT)
 *  cela veut dire qu'on attend une valeur numerique apres le char 'opt'
 *  si c'est le cas, la fct renvoit la valeur, sinon 0
 * -si exp_int est < 0: (GOPT_FLAG)
 *  cela veut dire que l'on recherche uniquement si 'opt' est dans parv,
 *  si c'est le cas, retourne 1
 * -si exp_int est = 0: (GOPT_STR)
 *  cela veut dire que l'on attend une string apres le 'opt',
 *  si c'est le cas retourne l'index de cette string dans parv, sinon 0.
 */

int getoption(const char *opt, char **parv, int parc, int deb, enum getopt_type type)
{
	for(; deb <= parc; ++deb)
		if(!strcasecmp(opt, parv[deb]))
		{
			if(type == GOPT_FLAG) return 1;
			if(++deb <= parc)
			{
				char *ptr = NULL;
				long tmp = 0L;

				if(type == GOPT_STR) return deb;

				tmp = strtol(parv[deb], &ptr, 10);
				return tmp > INT_MAX || (ptr && *ptr) ? 0 : (int) tmp;
			}
			break;
		}
	return 0;
}

char *duration(int s)
{
	static char dur[30 /* 5 + 4 + 2 + 4 + 2 + 5 + 3 + 1 */];
	int i = 1;
	dur[0] = '\002';

	if(s >= 86400)
		i += mysnprintf(dur + i, sizeof dur - i, "%d", s/86400),
				s %= 86400, strcpy(dur + i, "\2j \002"), i += 4;
	if(s >= 3600)
		i += mysnprintf(dur + i, sizeof dur - i, "%d", s/3600),
				s %= 3600, strcpy(dur + i, "\2h \002"), i += 4;
	if(s >= 60)
		i += mysnprintf(dur + i, sizeof dur - i, "%d", s/60),
				s %= 60, strcpy(dur + i, "\2mn \002"), i += 5;
	if(s) i += mysnprintf(dur + i, sizeof dur - i, "%d\2s",s);
	else dur[i-2]= 0;

	return dur;
}

time_t convert_duration(const char *p) /* extrait une durée en secondes */
{										/* d'un format XdXhXm.. - Cesar */
	const char *p1 = p;
	unsigned long int compute = 0UL;

	while(*p1)
	{
		switch(*p1)
		{
			case 'H': case 'h':
						compute += 3600 * strtoul(p, NULL, 10);
						p = p1 + 1;
						break;
			case 'm': case 'M':
						compute += 60 * strtoul(p, NULL, 10);
						p = p1 + 1;
						break;
			case 'd': case 'j':	case 'J': case 'D':
						compute += 86400 * strtoul(p, NULL, 10);
						p = p1 + 1;
						break;
			default:
				if(isdigit((unsigned char) *p1)) break;
				return 0; /* neither a time format nor a digit */
		}
		++p1;
	}
	compute += strtoul(p, NULL, 10); /* add trailing seconds */
	if(compute + CurrentTS > LONG_MAX) return 0; /* Would overflow a time_t */
	return (time_t) compute; /* finally convert it to time_t */
}

char *get_time(aNick *nick, time_t mytime)
{
	static char buftime[TIMELEN + 1];
	register struct tm *lt = localtime(&mytime);
	const char *ptr = nick ? GetReply(nick, lt->tm_wday + L_DAYS) :
							 DefaultLang->msg[lt->tm_wday + L_DAYS];
	int i = 0;

	lt->tm_year += 1900;
	++lt->tm_mon;

	while((buftime[i++] = *ptr++));

	buftime[i-1] = ',';
	buftime[i++] = ' ';
	buftime[i++] = (lt->tm_year / 1000) + '0';
	buftime[i++] = ((lt->tm_year / 100) % 10) + '0';
	buftime[i++] = ((lt->tm_year / 10) % 10) + '0';
	buftime[i++] = (lt->tm_year % 10) + '0';
	buftime[i++] = '-';
	buftime[i++] = (lt->tm_mon / 10) + '0';
	buftime[i++] = (lt->tm_mon % 10) + '0';
	buftime[i++] = '-';
	buftime[i++] = (lt->tm_mday / 10) + '0';
	buftime[i++] = (lt->tm_mday % 10) + '0';
	buftime[i++] = ' ';
	buftime[i++] = (lt->tm_hour / 10) + '0';
	buftime[i++] = (lt->tm_hour % 10) + '0';
	buftime[i++] = ':';
	buftime[i++] = (lt->tm_min / 10) + '0';
	buftime[i++] = (lt->tm_min % 10) + '0';
	buftime[i++] = ':';
	buftime[i++] = (lt->tm_sec / 10) + '0';
	buftime[i++] = (lt->tm_sec % 10) + '0';
	buftime[i] = 0;

	return buftime;
}

char *GetNUHbyNick(aNick *nick, int type)
{
	static char nuhbuf[NUHLEN];
	register char *ptr = nick->nick, *ptr2 = nuhbuf;

	while((*ptr2++ = *ptr++));
	for(*(ptr2-1) = '!', ptr = nick->ident;(*ptr2++ = *ptr++););
	for(*(ptr2-1) = '@',
#ifdef HAVE_CRYPTHOST
		ptr = (type == 1) ? nick->crypt : nick->host
#else
		ptr = nick->host
#endif
	;(*ptr2++ = *ptr++););

	return nuhbuf;
}

char *GetPrefix(aNick *nick)
{
	static char pref[12];
	int i = 0;
	pref[0] = '\0';

	if(IsOper(nick)) strcpy(pref, "\2\0037*\2\003"), i = 6;
	if(nick->user) strcpy(pref + i, IsAdmin(nick->user) ? "\00312^\003" : "\0035!\003");

	return pref;
}

char *GetChanPrefix(aNick *nick, aJoin *join)
{
	static char pref[14];
	int i = 0;

	if(IsOper(nick)) strcpy(pref, "\2\0037*\2\003"), i = 6;
	if(nick->user)
		strcpy(pref + i, IsAdmin(nick->user) ? "\00312^\003" : "\00305!\003"), i += 5;
	if(IsOp(join)) pref[i++] = '@';
	if(IsVoice(join)) pref[i++] = '+';
	pref[i] = 0;
	return pref;
}

int check_protect(aNick *from, aNick *cible, aChan *chan)
{
	anAccess *a1, *a2;

	if(from == cible) return 1;
	if(cible->flag & (N_GOD|N_SERVICE))
		return csreply(from, "L'user \2%s\2 est protégé", cible->nick);
	/* cible non auth OU from admin et cible non, ou admin de level inf */
	if(!cible->user || from->user->level > cible->user->level) return 1;
	/* from admin de niveau supérieur */
	if(IsAdmin(from->user)) return csreply(from, "L'user \2%s\2 est un Admin "
						"de niveau supérieur ou égal au votre.", cible->nick);
	if(IsAdmin(cible->user)) /* cible est admin */
		return csreply(from, "L'user \2%s\2 est un Administrateur des Services.", cible->nick);
	/* optimisation: on check dabord l'acces de l'executé! */
	if(!(a2 = GetAccessIbyUserI(cible->user, chan)) || ASuspend(a2) || !AProtect(a2)) return 1;
	/* on boucle les access de from pour voir si on en trouve 1 sur le chan */
	if(!(a1 = GetAccessIbyUserI(from->user, chan)) || ASuspend(a1)) return 0;
	/* Should not happen: from is neither Admin nor has access */

	if(a1->level > a2->level) return 1;
	/* on verifie si from a un level superieur a cible, ou si cible n'est pas protect */
	csreply(from, "L'user \2%s\2 est protégé ou a un access supérieur au votre sur \2%s\2.",
		cible->nick, chan->chan);
	return 0; /* arrive pour tous les autres cas */
}

int IsValidNick(const char *p)
{
	if(*p == '-' || isdigit((unsigned char) *p)) return 0;
	while(*p)
	{
		if(!isalnum((unsigned char) *p) && !strchr(SPECIAL_CHAR, *p)) return 0;
		++p;
	}
	return 1;
}

int IsValidMail(const char *p)
{
	int fa = 0, fd = 0, ispoint = 0;
	if(strchr("-_.@+", *p)) return 0;
	while(*p)
	{
		if(*p == '@')
		{
			if(!fa) fa = 1;
			else return 0;
		}
		if(fa && *p != '.') ispoint = 0;
		if(fa && *p == '.')
		{
			if(ispoint) return 0; /* last char was a dot too.. */
			fd = 1;
			ispoint = 1;
		}
		if(!isalnum((unsigned char) *p) && *p != '.' && *p != '-'
			&& *p != '_' && *p != '@' && (fa || *p != '+')) return 0;
		++p;
	}
	if(fa && fd) return 1;
	return 0;
}

anUser *ParseNickOrUser(aNick *nick, const char *arg)
{
	anUser *user = NULL;

	if(*arg == '%') /* recherche sur nick actif! */
	{
		aNick *n;
		if(*++arg == 0) csreply(nick, "Veuillez spécifier un nick après '%%'");
		else if(!(n = getnickbynick(arg))) csreply(nick, GetReply(nick, L_NOSUCHNICK), arg);
		else if(!n->user) csreply(nick, GetReply(nick, L_NOTLOGUED), arg);
		else return n->user;
	}
	else if(!(user = getuserinfo(arg))) csreply(nick, GetReply(nick, L_NOSUCHUSER), arg);
	return user;
}

int switch_option(aNick *nick, const char *choix, const char *opt,
	const char *targ, int *flagto, int flag)
{
	if(!choix) *flagto ^= flag;
	else if(!strcasecmp(choix, "on")) *flagto |= flag;
	else if(!strcasecmp(choix, "off")) *flagto &= ~flag;
	else return csreply(nick, GetReply(nick, L_SHOULDBEONOFF));

	if(*flagto & flag) csreply(nick, GetReply(nick, L_OPTIONNOWON), opt, targ);
	else csreply(nick, GetReply(nick, L_OPTIONNOWOFF), opt, targ);
	return 0;
}

int cswall(const char *format, ...)
{
	int i = 0;
	char buf[400];
	va_list vl;

	va_start(vl, format);
	myvsnprintf(buf, sizeof buf, format, vl);
	va_end(vl);

	for(; i < adminmax; ++i) if(adminlist[i]) csreply(adminlist[i], "%s", buf);

	return 1;
}

/*
 * if opts is NULL, use static array, size should be returned by item_parselist
 */
int item_isinlist(item_opt *opts, const int size, const int n)
{
	int i = 0;

    if(!opts) opts = item_static_opts;

	for(; i < size; ++i)
        if((opts[i].type == ITEM_NUMBER && opts[i].min == n)
        || (opts[i].type == ITEM_INTERVAL && n >= opts[i].min && n <= opts[i].max))
            return i;
	return -1;
}

/*
 * You can pass a NULL pointer to make item_parselist use a static array,
 * then you'll have to give item_isinlist() the option count returned
 * and NULL pointer
 */
int item_parselist(char *list, item_opt *opts, int size, int *count)
{
    char *ptr = NULL, *p;
    int i = 0;

    if(!opts) opts = item_static_opts, size = ASIZE(item_static_opts);

    for(p = Strtok(&ptr, list, LISTSEP); p && i < size; p = Strtok(&ptr, NULL, LISTSEP))
    {
        char *endptr = NULL;
        int min = strtol(p, &endptr, 10), max = 0;

        if(min < 0 || (endptr && *endptr && (endptr == p || *endptr != '-'))) continue;
        /* strtol failed, either at the begining (not a number), or at the end (not an interval) */
        else if(!endptr || !*endptr) opts[i].type = ITEM_NUMBER, ++*count;
        else
        {
            char *p2 = NULL;
            /* check if end of the interval is valid */
            if((max = strtol(endptr + 1, &p2, 10)) < 0 || (p2 && *p2)) continue;
            opts[i].type = ITEM_INTERVAL;

            if(max == min) opts[i].type = ITEM_NUMBER; /* fallback */
            else if(min > max) /* go back to school ! */
            {
                unsigned int tmp = max;
                max = min;
                min = tmp;
            }
            *count += max - min + 1;
        }
        opts[i].min = min;
        opts[i].max = max;
        ++i;
    }
    return i;
}
